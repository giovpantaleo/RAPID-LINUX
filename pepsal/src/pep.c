/*
 * PEPsal : A Performance Enhancing Proxy for Satellite Links
 *
 * Copyleft Daniele Lacamera 2005
 * Copyleft Dan Kruchining <dkruchinin@acm.com> 2010
 * Copyleft Joaquin Muguerza <jmuguerza@toulouse.viveris.fr> 2016
 * See AUTHORS and COPYING before using this software.
 *
 *
 *
 */

#include "config.h"
#include "pepsal.h"
#include "pepqueue.h"
#include "syntab.h"

#include <unistd.h>
#include <assert.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/user.h>

#define __USE_XOPEN_EXTENDED
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ether.h>
#include <netdb.h>
#include <getopt.h>
#include <linux/netfilter.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <net/if.h>

#include <sys/poll.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <syslog.h>

#include <sys/time.h>
#include <linux/socket.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include<sys/ioctl.h>
 
#define WR_VALUE _IOW('a','a',int32_t*)
#define RD_VALUE _IOR('a','b',int32_t*)

// RNIS INCLUDE
#include <rabbitmq-c/tcp_socket.h>
#include <assert.h>
#include "utils.h"
// END RNIS INCLUDE
/* RNIS subscribe to UE info */
/*****************************/
typedef struct rnis_t {
  char *hostname;
  char *queuename;
  int cqi;
  int txq;
} rnis_t;

/**************** amqp utils *************/

void *rnis_subscribe(void *parg)
{
  int port, status;
  amqp_socket_t *socket = NULL;
  amqp_connection_state_t conn;
  struct rnis_t* argx = (struct rnis_t*)parg;
  port = 5672;
  conn = amqp_new_connection();
  socket = amqp_tcp_socket_new(conn);
  printf("hostname [thread] = %s",argx->hostname);
  if (!socket) {
    die("creating TCP socket");
  }

  status = amqp_socket_open(socket, argx->hostname, port);
  if (status) {
    //die("opening TCP socket");
    printf("[RNIS][ERROR] cannot open TCP socket ");
    return NULL;
  }
  die_on_amqp_error(amqp_login(conn, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN,
                               "guest", "guest"),
                    "Logging in");
  amqp_channel_open(conn, 1);
  die_on_amqp_error(amqp_get_rpc_reply(conn), "Opening channel");

  amqp_basic_consume(conn, 1, amqp_cstring_bytes(argx->queuename), amqp_empty_bytes,
                     0, 0, 0, amqp_empty_table);
  die_on_amqp_error(amqp_get_rpc_reply(conn), "Consuming");

  for (;;) {
    amqp_rpc_reply_t res;
    amqp_envelope_t envelope;
    //printf("[RNIS] I AM CONSUMING A TOPIC");
    amqp_maybe_release_buffers(conn);

    res = amqp_consume_message(conn, &envelope, NULL, 0);

    if (AMQP_RESPONSE_NORMAL != res.reply_type) {
      break;
    }
    //amqp_dump(envelope.message.body.bytes, envelope.message.body.len);
    unsigned char *buf = (unsigned char *)envelope.message.body.bytes;
    //printf("%s",buf);
    char delim[]=",";
    char *ptr1 = strtok((char *)buf, delim);
    char *ptr2 = strtok(NULL, delim);
    char *ptr3 = strtok(NULL, delim);
    char *ptr4 = strtok(NULL, delim);
    int dlcqi=atoi(ptr1);
    int txq=atoi(ptr2);
    int tbsDL=atoi(ptr3);
    int prbDL=atoi(ptr4);
    double TTI = 0.001;
    int32_t rbw = ceil(((double)(tbsDL))/(TTI));
    //DLCQI=dlcqi;
    //TXQ=txq;
    //TBS=tbsDL;
    //PRB=prbDL;
    argx->cqi=dlcqi;
    argx->txq=txq;

        int fd;
        fd = open("/dev/etx_device", O_RDWR);
        if(fd < 0) {
	  //printf("Cannot open device file...\n");
          //return 0;
	  //ioctl(fd, WR_VALUE, (int32_t*) &rbw);
          //close(fd);
        }
	else{
        ioctl(fd, WR_VALUE, (int32_t*) &rbw); 
        close(fd);
	}
    //printf("[CQI: %d] [TXQ %d] [TBS %d]",dlcqi,txq,tbsDL);
    

    amqp_destroy_envelope(&envelope);
  }
 die_on_amqp_error(amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS),"Closing channel");
 die_on_amqp_error(amqp_connection_close(conn, AMQP_REPLY_SUCCESS),"Closing connection");
 die_on_error(amqp_destroy_connection(conn), "Ending connection");
 return NULL;    
}

/**************** end amqp utils *********/


/*
 * Data structure to fill with packet headers when we
 * get a new syn:
 *
 * struct ipv4_packet
 *      iph : ip header for the packet
 *      tcph: tcp header for the segment
 *
 */
struct ipv4_packet{
    struct iphdr iph;
    struct tcphdr tcph;
};

static int DEBUG = 0;
static int background = 0;
static int fastopen = 0;
static int gcc_interval = PEP_GCC_INTERVAL;
static int pending_conn_lifetime = PEP_PENDING_CONN_LIFETIME;
static int portnum = PEP_DEFAULT_PORT;
static int max_conns = (PEP_MIN_CONNS + PEP_MAX_CONNS) / 2;
static char pepsal_ip_addr[20] = "0.0.0.0";

/*
 * The main aim of this structure is to reduce search time
 * of pep_proxy instance corresponding to the returned by poll()
 * file descriptor. Typically to find pep_proxy by one of its FDs
 * takes linear time(because pep_proxys are arranged into one list),
 * but this structure reduce search time to O(1).
 * pollfds is an array of file descriptors and events used by poll.
 * pdescrs is an array of pointers to corresponding pep_endpoint entries.
 * Each Ith FD corresponds to Ith pep_proxy. Both array have the same
 * size equal to num_pollfds items.
 */
static struct {
    struct pollfd        *pollfds;
    struct pep_endpoint **endpoints;
    int                   num_pollfds;
} poll_resources;

/*
 * PEP logger dumps all connections in the syn table to
 * the file specified by filename every PEPLOGGER_INTERVAL
 * seconds.
 */
struct pep_logger {
    FILE *file;
    timer_t timer;
    char *filename;
};

/*
 * Main queues for connections and work synchronization
 * active_queue is used to transfer read/write jobs to
 * worker threads from PEP threads pool. After all jobs in
 * active_queue are done, they're moved to the ready_queue
 * which is used by poller thread. After poller thread wakes up,
 * it cheks out all connections from ready_queue, checks theier status,
 * updates metainformation and restarts polling loop.
 */
static struct pep_queue active_queue, ready_queue;
static struct pep_logger logger;

static pthread_t listener;
static pthread_t poller;
static pthread_t timer_sch;
static pthread_t *workers = NULL;
static pthread_t change_win;

#define pep_error(fmt, args...)                       \
    syslog(LOG_ERR, "%s():%d: " fmt " (errno %d)",    \
           __FUNCTION__, __LINE__, ##args, errno);    \
    __pep_error(__FUNCTION__, __LINE__, fmt, ##args)  

#define pep_warning(fmt, args...)                     \
    syslog(LOG_WARNING, "%s():%d: " fmt,              \
           __FUNCTION__, __LINE__, ##args);           \
    __pep_warning(__FUNCTION__, __LINE__, fmt, ##args)

#define PEP_DEBUG(fmt, args...)                       \
    if (DEBUG) {                                      \
        fprintf(stderr, "[DEBUG] %s(): " fmt "\n",    \
                __FUNCTION__, ##args);                \
        syslog(LOG_DEBUG, "%s(): " fmt, __FUNCTION__, \
              ##args);                                \
    }

#define PEP_DEBUG_DP(proxy, fmt, args...)                           \
    if (DEBUG) {                                                    \
        char __buf[17];                                             \
        toip(__buf, (proxy)->src.addr);                             \
        fprintf(stderr, "[DEBUG] %s(): {%s:%d} " fmt "\n",          \
                __FUNCTION__, __buf, (proxy)->src.port, ##args);    \
        syslog(LOG_DEBUG, "%s(): {%s:%d} " fmt, __FUNCTION__,       \
               __buf, (proxy)->src.port, ##args);                   \
    }

void *rapid_win(void *parm)
{
  int *p = (int *)parm;
  printf("parm = %d\n", *p);
  for (;;)
    {
     int test = syscall(332,*p,1000);
     }
  //free(p);   
 
  return NULL;
}


int ipcalc(char *ip, int port)
{
        char *str = (char *) malloc(sizeof(ip)*strlen(ip));
        strcpy(str, ip);
        //str = ip;
	int init_size = strlen(str);
	char delim[] = ".";
       // printf("%s\n", str);
	char *ptr = strtok(str, delim);
        int val = 0;
	while (ptr != NULL)
	{
		//printf("%s\n", ptr);
                val+=atoi(ptr);
		ptr = strtok(NULL, delim);
                
	}

	return val+port;
}
int stohi(char *ip){
	char c;
	c = *ip;
	unsigned int integer=0;
	int val;
	int i,j=0;
	for (j=0;j<4;j++) {
		if (!isdigit(c)){  //first char is 0
			return (0);
		}
		val=0;
		for (i=0;i<3;i++) {
			if (isdigit(c)) {
				val = (val * 10) + (c - '0');
				c = *++ip;
			} else
				break;
		}
		if(val<0 || val>255){
			return (0);	
		}	
		if (c == '.') {
			//integer=(integer<<8) | val;
                        integer+=val;
			c = *++ip;
		} 
		else if(j==3 && c == '\0'){
			//integer=(integer<<8) | val;
                        integer+=val;
			break;
		}
			
	}
	if(c != '\0'){
		return (0);	
	}
	return (htonl(integer));
}

static void __pep_error(const char *function, int line, const char *fmt, ...)
{
    va_list ap;
    char buf[PEP_ERRBUF_SZ];
    int err = errno;
    size_t len;

    va_start(ap, fmt);

    len = snprintf(buf, PEP_ERRBUF_SZ, "[ERROR]: ");
    len += vsnprintf(buf + len, PEP_ERRBUF_SZ - len, fmt, ap);
    if (err && (PEP_ERRBUF_SZ - len) > 1) {
        snprintf(buf + len, PEP_ERRBUF_SZ - len,
                 "\n      ERRNO: [%s:%d]", strerror(err), err);
    }

    fprintf(stderr, "%s\n         AT: %s:%d\n", buf, function, line);
    va_end(ap);
    closelog();
    exit(EXIT_FAILURE);
}

static void __pep_warning(const char *function, int line, const char *fmt, ...)
{
    va_list ap;
    char buf[PEP_ERRBUF_SZ];
    size_t len;

    va_start(ap, fmt);
    len = snprintf(buf, PEP_ERRBUF_SZ, "[WARNING]: ");
    if (PEP_ERRBUF_SZ - len > 1) {
        len += vsnprintf(buf + len, PEP_ERRBUF_SZ - len, fmt, ap);
    }

    fprintf(stderr, "%s\n       AT: %s:%d\n", buf, function, line);
    va_end(ap);
}

static void usage(char *name)
{
    fprintf(stderr,"Usage: %s [-V] [-h] [-v] [-d] [-f]"
            " [-a address] [-p port]"
            " [-c max_conn] [-l logfile] [-t proxy_lifetime]"
            " [-g garbage collector interval]\n", name);
    exit(EXIT_SUCCESS);
}

/*
 * Check if error @err is related to nonblocking I/O.
 * If it is in a set of nonblocking erros, it may handled
 * properly without program termination.
 */
static int nonblocking_err_p(int err)
{
    static int nb_errs[] = {
        EAGAIN,
        EINPROGRESS,
        EALREADY,
    };
    int i;

    for (i = 0; i < sizeof(nb_errs); i++) {
        if (err == nb_errs[i])
            return 1;
    }

    return 0;
}

/*
 * Secure routine to translate a hex address in a
 * readable ip number:
 */
static void toip(char *ret, int address)
{
    int a,b,c,d;

    a = (0xFF000000 & address) >> 24;
    b = (0x00FF0000 & address) >> 16;
    c = (0x0000FF00 & address) >> 8;
    d = 0x000000FF & address;

    snprintf(ret,16,"%d.%d.%d.%d",a,b,c,d);
}

static char *conn_stat[] = {
    "PST_CLOSED",
    "PST_OPEN",
    "PST_CONNECT",
    "PST_PENDING",
};

static void logger_fn(void)
{
    struct pep_proxy *proxy;
    time_t tm;
    char ip_src[17], ip_dst[17], timebuf[128];
    int i = 1, len;

    PEP_DEBUG("Logger invoked!");
    SYNTAB_LOCK_READ();
    tm = time(NULL);
    ctime_r(&tm, timebuf);
    len = strlen(timebuf);
    timebuf[len - 1] = ']';
    fprintf(logger.file, "=== [%s ===\n", timebuf);
    syntab_foreach_connection(proxy) {
        toip(ip_src, proxy->src.addr);
        toip(ip_dst, proxy->dst.addr);
        fprintf(logger.file, "[%d] Proxy %s:%d <-> %s:%d\n", i++,
                ip_src, proxy->src.port, ip_dst, proxy->dst.port);
        fprintf(logger.file, "    Status: %s\n", conn_stat[proxy->status]);
        ctime_r(&proxy->syn_time, timebuf);
        fprintf(logger.file, "    SYN received: %s", timebuf);
        if (proxy->last_rxtx != 0) {
            ctime_r(&proxy->last_rxtx, timebuf);
            fprintf(logger.file, "    Last Rx/Tx activity: %s", timebuf);
        }

    }
    if (i == 1) {
        fprintf(logger.file, " No connections\n");
    }

    SYNTAB_UNLOCK_READ();
    fflush(logger.file);
}

static void setup_socket(int fd)
{
    struct timeval t= { 0, 10000 };
    int flags;

    flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &t, sizeof(struct timeval));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &t, sizeof(struct timeval));
    PEP_DEBUG("Socket %d: Setting up timeouts and syncronous mode.", fd);
}

#define ENDPOINT_POLLEVENTS (POLLIN | POLLHUP | POLLERR | POLLNVAL)
static struct pep_proxy *alloc_proxy(void)
{
    struct pep_proxy *proxy = calloc(1, sizeof(*proxy));
    int i;
    struct pep_endpoint *endp;

    if (!proxy) {
        errno = ENOMEM;
        return NULL;
    }

    list_init_node(&proxy->lnode);
    list_init_node(&proxy->qnode);
    proxy->status = PST_INVAL;
    atomic_set(&proxy->refcnt, 1);

    for (i = 0; i < PROXY_ENDPOINTS; i++) {
        endp = &proxy->endpoints[i];
        endp->fd = -1;
        endp->owner = proxy;
        endp->iostat = 0;
        endp->poll_events = ENDPOINT_POLLEVENTS;
    }

    return proxy;
}

static void free_proxy(struct pep_proxy *proxy)
{
    assert(atomic_read(&proxy->refcnt) == 0);
    free(proxy);
}

static inline void pin_proxy(struct pep_proxy *proxy)
{
    atomic_inc(&proxy->refcnt);
}

static inline void unpin_proxy(struct pep_proxy *proxy)
{
    if (atomic_dec(&proxy->refcnt) == 1) {
        PEP_DEBUG_DP(proxy, "Free proxy");
        free_proxy(proxy);
    }
}

static void destroy_proxy(struct pep_proxy *proxy)
{
    int i;

    if (proxy->status == PST_CLOSED) {
        goto out;
    }

    proxy->status = PST_CLOSED;
    PEP_DEBUG_DP(proxy, "Destroy proxy");

    SYNTAB_LOCK_WRITE();
    syntab_delete(proxy);
    proxy->status = PST_CLOSED;
    SYNTAB_UNLOCK_WRITE();

    for (i = 0; i < PROXY_ENDPOINTS; i++) {
        if (proxy->endpoints[i].fd >= 0) {
            fcntl(proxy->endpoints[i].fd, F_SETFL, O_SYNC);
            close(proxy->endpoints[i].fd);
        }
        if (pepbuf_initialized(&proxy->endpoints[i].buf)) {
            pepbuf_deinit(&proxy->endpoints[i].buf);
        }
    }

out:
    unpin_proxy(proxy);
}

/*
 * Garbage connections collector handler is periodically invoked
 * with gcc_interval interval(in seconds) and cleans dead(or garbage)
 * connections.
 * When PEPsal catches SYN packet from the source endpoint,
 * it creates new pep_proxy instance, markes it with PST_PENDING status
 * and saves into the SYN table. After some time(actually after ACK is received)
 * this proxy shold be
 * activated, connection should be established and endpoints set up.
 * If everything is going alright, the proxy will be marked with PST_CONNECT
 * status. But the client might endup abnormally after SYN is sent. In this case
 * PEPsal has no chance to know about it. Thus PEPsal monitors all pending
 * connections in SYN table and closes them if a connection hasn't have any
 * activity for a long time.
 */
static void garbage_connections_collector(void)
{
    struct pep_proxy *proxy;
    struct list_node *item, *safe;
    time_t t_now, t_diff;

    PEP_DEBUG("Garbage connections collector activated!");

    SYNTAB_LOCK_WRITE();
    t_now = time(NULL);
    list_for_each_safe(&GET_SYNTAB()->conns, item, safe) {
        proxy = list_entry(item, struct pep_proxy, lnode);
        if (proxy->status != PST_PENDING) {
            continue;
        }

        t_diff = t_now - proxy->syn_time;
        if (t_diff >= pending_conn_lifetime) {
            PEP_DEBUG_DP(proxy, "Marked as garbage. Destroying...");
            destroy_proxy(proxy);
        }
    }

    SYNTAB_UNLOCK_WRITE();
}

static ssize_t pep_receive(struct pep_endpoint *endp)
{
    int iostat;
    ssize_t rb;

    if (endp->iostat & (PEP_IORDONE | PEP_IOERR | PEP_IOEOF) ||
        pepbuf_full(&endp->buf)) {
        return 0;
    }

    rb = read(endp->fd, PEPBUF_RPOS(&endp->buf),
              PEPBUF_SPACE_LEFT(&endp->buf));
    if (rb < 0) {
        if (nonblocking_err_p(errno)) {
            endp->iostat |= PEP_IORDONE;
            return 0;
        }

        endp->iostat |= PEP_IOERR;
        return -1;
    }
    else if (rb == 0) {
        endp->iostat |= PEP_IOEOF;
        return 0;
    }

    pepbuf_update_rpos(&endp->buf, rb);
    return rb;
}

static ssize_t pep_send(struct pep_endpoint *from, int to_fd)
{
    ssize_t wb;
#if 0
/**********MADI**************/
// struct ipv4_packet *ipH;
 unsigned int myPort;
 char myIP[16];
 struct sockaddr_in  my_addr;
 bzero(&my_addr, sizeof(my_addr));
 socklen_t len = sizeof(my_addr);
 getsockname(to_fd, (struct sockaddr *) &my_addr, &len);
 inet_ntop(AF_INET, &my_addr.sin_addr, myIP, sizeof(myIP));
 myPort = ntohs(my_addr.sin_port);
 if (myPort != 8080)
  {
 printf("Local ip address: %s\n", myIP);
 printf("Local port : %u\n", myPort);
 int rcvbuf = 2048;
 setsockopt(to_fd, SOL_SOCKET, SO_RCVBUF, (char *)& rcvbuf, sizeof(rcvbuf));
 int clamp = 1024;
 setsockopt(to_fd, SOL_SOCKET, TCP_WINDOW_CLAMP, (char *)& clamp, sizeof(clamp));
 }
/*************************/
#endif

    if (from->iostat & (PEP_IOERR | PEP_IOWDONE) ||
        (pepbuf_empty(&from->buf) && !(from->iostat & PEP_IOEOF))) {
        return 0;
    }
#if 0
/***********MADI***************/
//struct ipv4_packet *iph = (struct ipv4_packet *)(from->buf);
unsigned short iphdrlen;
unsigned char *buffer = (unsigned char *) malloc(/*PEPBUF_SPACE_FILLED(&from->buf)*/1460);
memcpy (buffer, PEPBUF_WPOS(&from->buf), 1460);
/*struct ethhdr *eth = (struct ethhdr *)(buffer);
printf("Ethernet Header\n");
printf("\t|-Source Address : %.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n",eth->h_source[0],eth->h_source[1],eth->h_source[2],eth->h_source[3],eth->h_source[4],eth->h_source[5]);
printf("\t|-Destination Address : %.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n",eth->h_dest[0],eth->h_dest[1],eth->h_dest[2],eth->h_dest[3],eth->h_dest[4],eth->h_dest[5]);
printf("\t|-Protocol : %d\n",eth->h_proto);*/
struct iphdr *ip = (struct iphdr*)(buffer + sizeof(struct ethhdr));
//struct iphdr *ip = (struct iphdr*)(PEPBUF_WPOS(&from->buf) /*+ sizeof(struct ethhdr)*/);
struct sockaddr_in source,dest;
memset(&source, 0, sizeof(source));
source.sin_addr.s_addr = ip->saddr;
memset(&dest, 0, sizeof(dest));
dest.sin_addr.s_addr = ip->daddr;
printf("Source IP : %s\n", inet_ntoa(source.sin_addr));
printf("Destination IP : %s\n",inet_ntoa(dest.sin_addr));
/*******************************/
#endif
//int test = syscall(332,to_fd,1000);
  //printf("fd = %d",to_fd);
  //int *parm = (int *)malloc(sizeof(int));
  //*parm = to_fd;
   //pthread_create(&change_win, NULL, rapid_win, (void *)parm);
    wb = write(to_fd, PEPBUF_WPOS(&from->buf),
               PEPBUF_SPACE_FILLED(&from->buf));
    //int test = syscall(332,to_fd,1000);
    if (wb < 0) {
        if (nonblocking_err_p(errno)) {
            from->iostat |= PEP_IOWDONE;
            return 0;
        }

        from->iostat |= PEP_IOERR;
        return -1;
    }

    pepbuf_update_wpos(&from->buf, wb);
    return wb;
}

static void pep_proxy_data(struct pep_endpoint *from, struct pep_endpoint *to)
{
    ssize_t rb, wb;

    rb = wb = 1;
    while ((wb > 0) || (rb > 0)) {
        rb = pep_receive(from);
        wb = pep_send(from, to->fd);
    }

    if (from->iostat & PEP_IOERR) {
        return;
    }

    /*
     * Receiving buffer has no space or EOF was reached from the peer.
     * Stop wait for incomming data on this FD.
     */
    if (pepbuf_full(&from->buf) || (from->iostat & PEP_IOEOF)) {
        from->poll_events &= ~POLLIN;
    }
    else if (from->iostat & PEP_IORDONE) {
        from->poll_events |= POLLIN;
    }

    /*
     * All available data was transmitted to the peer
     * Stop wait when FD will be ready for write.
     */
    if (pepbuf_empty(&from->buf)) {
        to->poll_events &= ~POLLOUT;
    }
    else { /* There exists some data to write. Wait until we can transmit it. */
        to->poll_events |= POLLOUT;
    }
}

static int save_proxy_from_socket(int sockfd, struct sockaddr_in cliaddr)
{
    char *buffer;
    struct ipv4_packet *ip4;
    struct pep_proxy *proxy, *dup;
    struct syntab_key key;
    int id = 0, ret, added = 0;
    struct sockaddr_in orig_dst;
    int addrlen = sizeof(orig_dst);
    
    /** Mark RAN socket with 255 **/
    int marked_pep_port = 255;
        setsockopt(sockfd,SOL_SOCKET,SO_MARK,(void *)(&marked_pep_port),sizeof(marked_pep_port));

    PEP_DEBUG("Saving new SYN...");

    proxy = NULL;
    proxy = alloc_proxy();
    if (!proxy) {
        pep_warning("Failed to allocate new pep_proxy instance! [%s:%d]",
                    strerror(errno), errno);
        ret = -1;
        goto err;
    }

    /* Socket is bound to original destination */
    if(getsockname(sockfd, (struct sockaddr *) &orig_dst, &addrlen) < 0){
        pep_warning("Failed to get original dest from socket! [%s:%d]",
                    strerror(errno), errno);
        ret = -1;
        goto err;
    }

    /* Setup source and destination endpoints */
    proxy->src.addr = ntohl(cliaddr.sin_addr.s_addr);
    proxy->src.port = ntohs(cliaddr.sin_port);
    proxy->dst.addr = ntohl(orig_dst.sin_addr.s_addr);
    proxy->dst.port = ntohs(orig_dst.sin_port);
    proxy->syn_time = time(NULL);
    syntab_format_key(proxy, &key);

    /* Check for duplicate syn, and drop it.
     * This happens when RTT is too long and we
     * still didn't establish the connection.
     */
    SYNTAB_LOCK_WRITE();
    dup = syntab_find(&key);
    if (dup != NULL) {
        PEP_DEBUG_DP(dup, "Duplicate SYN. Dropping...");
        SYNTAB_UNLOCK_WRITE();
        goto err;
    }

    /* add to the table... */
    proxy->status = PST_PENDING;
    ret = syntab_add(proxy);
    SYNTAB_UNLOCK_WRITE();
    if (ret < 0) {
        pep_warning("Failed to insert pep_proxy into a hash table!");
        goto err;
    }

    added = 1;
    PEP_DEBUG_DP(proxy, "Registered new SYN");
    if (ret < 0) {
        pep_warning("nfq_set_verdict to NF_ACCEPT failed! [%s:%d]",
                    strerror(errno), errno);
        goto err;
    }

    return ret;

err:
    if (added) {
        syntab_delete(proxy);
    }
    if (proxy != NULL) {
        unpin_proxy(proxy);
    }

    return ret;
}

#if 0
void *rapid_win(void *parm)
{
  int *p = (int *)parm;
  printf("parm = %d\n", *p);
  for (;;)
    {
     int test = syscall(332,*p,1000);
     }
  //free(p);
  
  return NULL;
}
#endif

void *listener_loop(void UNUSED(*unused))
{
    int                 listenfd, optval, ret, connfd, out_fd,retval;
    struct sockaddr_in  cliaddr, servaddr,
                        r_servaddr,leftaddr;
    socklen_t           len;
    struct pep_proxy   *proxy;
    struct hostent     *host;
    char                ipbuf[17], ipbuf1[17];
    unsigned short      r_port, c_port;
    struct syntab_key   key;
    unsigned int myPort;
    char myIP[16];
    int ipsum;
    int marked_cli_port = 255;
    int	marked_pep_port	= 255;
    //struct sockaddr_in  my_addr;
    //bzero(&my_addr, sizeof(my_addr));
    //socklen_t len = sizeof(my_addr);
    //getsockname(to_fd, (struct sockaddr *) &my_addr, &len);
 

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        pep_error("Failed to create listener socket!");
    }
    
    /** Mark RAN socket with 255 **/
        setsockopt(listenfd,SOL_SOCKET,SO_MARK,(void *)(&marked_pep_port),sizeof(marked_pep_port));

    PEP_DEBUG("Opened listener socket: %d", listenfd);
    memset(&servaddr, 0, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(portnum);

    optval = 1;
    ret = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,
                     &optval, sizeof(optval));
    if (ret < 0) {
        pep_error("Failed to set SOL_REUSEADDR option! [RET = %d]", ret);
    }
 
    /* Set socket transparent (able to bind to external address) */
    ret = setsockopt(listenfd, SOL_IP, IP_TRANSPARENT,
                     &optval, sizeof(optval));
    if (ret < 0) {
        pep_error("Failed to set IP_TRANSPARENT option! [RET = %d]", ret);
    }

    /* Set TCP_FASTOPEN socket option */
    if (fastopen) {
      optval = 5;
      ret = setsockopt(listenfd, SOL_TCP, TCP_FASTOPEN,
                       &optval, sizeof(optval));
      if (ret < 0) {
          pep_error("Failed to set TCP_FASTOPEN option! [RET = %d]", ret);
      }
    }
    
    /** Mark RAN socket with 255 **/
        setsockopt(listenfd,SOL_SOCKET,SO_MARK,(void *)(&marked_pep_port),sizeof(marked_pep_port));
    ret = bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    if (ret < 0) {
        pep_error("Failed to bind socket! [RET = %d]", ret);
    }

    ret = listen(listenfd, LISTENER_QUEUE_SIZE);
    if (ret < 0) {
        pep_error("Failed to set quesize of listenfd to %d! [RET = %d]",
                  LISTENER_QUEUE_SIZE, ret);
    }

   fcntl(listenfd, F_SETOWN, getpid()); // MADI, set PID for left socket
        /** Mark RAN socket with 255 **/
        setsockopt(listenfd,SOL_SOCKET,SO_MARK,(void *)(&marked_pep_port),sizeof(marked_pep_port));

    /* Accept loop */
    PEP_DEBUG("Entering lister main loop...");
    for (;;) {
        out_fd = -1;
        proxy = NULL;

        len = sizeof(struct sockaddr_in);
        connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &len);
	fcntl(connfd, F_SETOWN, getpid()); // MADI, set PID for left socket
	/** Mark RAN socket with 255 **/
	setsockopt(connfd,SOL_SOCKET,SO_MARK,(void *)(&marked_pep_port),sizeof(marked_pep_port));
	
        if (connfd < 0) {
            pep_warning("accept() failed! [Errno: %s, %d]",
                        strerror(errno), errno);
            continue;
        }

        /*
         * Try to find incomming connection in our SYN table
         * It must be already there waiting for activation.
         */
        key.addr = ntohl(cliaddr.sin_addr.s_addr);
        key.port = ntohs(cliaddr.sin_port);
	marked_cli_port = key.port;
        toip(ipbuf, key.addr);
        PEP_DEBUG("New incomming connection: %s:%d", ipbuf, key.port);
         
        SYNTAB_LOCK_READ();
        proxy = syntab_find(&key);



        /*
         * If the proxy is not in the table, add the entry.
         */
        if (!proxy) {
            SYNTAB_UNLOCK_READ();
            save_proxy_from_socket(connfd, cliaddr);
            SYNTAB_LOCK_READ();
            proxy = syntab_find(&key);
        }

        /*
         * If still can't find key in the table, there is an error.
         */
        if (!proxy) {
            pep_warning("Can not find the connection in SYN table. "
                        "Terminating!");
            SYNTAB_UNLOCK_READ();
            goto close_connection;
        }

        /*
         * The proxy we fetched from the SYN table is in PST_PENDING state.
         * Now we're going to setup connection for it and configure endpoints.
         * While the proxy is in PST_PENDING state it may be possibly removed
         * by the garbage connections collector. Collector is invoked every N
         * seconds and removes from SYN table all pending connections
         * that were not activated during predefined interval. Thus we have
         * to pin our proxy to protect ourself from segfault.
         */
        pin_proxy(proxy);
        assert(proxy->status == PST_PENDING);
        SYNTAB_UNLOCK_READ();

        toip(ipbuf, proxy->dst.addr);
        r_port = proxy->dst.port;
        PEP_DEBUG("Connecting to %s:%d...", ipbuf, r_port);
        host = gethostbyname(ipbuf);
        if (!host) {
            pep_warning("Failed to get host %s!", ipbuf);
            goto close_connection;
        }

        memset(&r_servaddr, 0, sizeof(r_servaddr));
        r_servaddr.sin_family = AF_INET;
        r_servaddr.sin_addr.s_addr = ((struct in_addr *)(host->h_addr))->s_addr;
        r_servaddr.sin_port = htons(r_port);

        ret = socket(AF_INET, SOCK_STREAM, 0);
        if (ret < 0) {
            pep_warning("Failed to create socket! [%s:%d]",
                        strerror(errno), errno);
            goto close_connection;
        }
        //struct socket *ss = sockfd_lookup(ret, NULL);
        //int test = syscall(332,ret,1000);
        //int *parm = (int *)malloc(sizeof(int));
        //*parm = ret;
        //pthread_create(&change_win, NULL, rapid_win, (void *)parm);
        out_fd = ret;
        fcntl(out_fd, F_SETFL, O_NONBLOCK);

        /*
         * Set outbound endpoint to transparent mode
         * (bind to external address)
         */
        ret = setsockopt(out_fd, SOL_IP, IP_TRANSPARENT,
                         &optval, sizeof(optval));
        if (ret < 0) {
            pep_error("Failed to set IP_TRANSPARENT option! [RET = %d]", ret);
        }

        toip(ipbuf, proxy->src.addr);
        toip(ipbuf1, proxy->dst.addr);
        
        /************** RAPID hash*************/
        //inet_ntop(AF_INET, &cliaddr.sin_addr, myIP, sizeof(myIP));
        
        ipsum = ipcalc(ipbuf,proxy->src.port);
        //PEP_DEBUG("IP SUM: %s:%d", ipbuf, ipsum);
        retval = 1;
        ret = setsockopt(out_fd, SOL_SOCKET, SO_REUSEADDR,&retval, sizeof(retval));
        //ret = setsockopt(out_fd,SOL_SOCKET,SO_REUSEADDR, 1)
         memset(&leftaddr, 0, sizeof(leftaddr));
         leftaddr.sin_family = AF_INET;
         leftaddr.sin_addr.s_addr = htonl(INADDR_ANY);
         leftaddr.sin_port = htons(ipsum);

       // ret = bind(out_fd,("192.168.2.1", ipsum));
	 //ret = bind(out_fd, (struct sockaddr *)&leftaddr, sizeof(leftaddr));
	 fcntl(out_fd, F_SETOWN, getpid()); // MADI, set PID for right-side socket
	 /** Mark WAN socket with client original src port **/
	 setsockopt(out_fd,SOL_SOCKET,SO_MARK,(void *)(&marked_cli_port),sizeof(marked_cli_port));
        /***************************************/
        int mymark = 0;
        unsigned int optlen = sizeof(int);
        getsockopt(out_fd, SOL_SOCKET,SO_MARK,/*reinterpret_cast<void *>*/(void *)(&mymark), &optlen);
        PEP_DEBUG("[SOCK][MARK]: WAN MARK = %d", mymark);
        //std::cout<<"[SOCK][MARK]: Mark = "<<static_cast<int>(mymark)<<std::endl;
        /********** END TEST *******************/ 

        if (fastopen) {
          ret = sendto(out_fd, PEPBUF_WPOS(&proxy->src.buf), 0, MSG_FASTOPEN,
                       (struct sockaddr *)&r_servaddr, sizeof(r_servaddr));
        }
        else {
          ret = connect(out_fd, (struct sockaddr *)&r_servaddr,
                        sizeof(r_servaddr));
        }
        if ((ret < 0) && !nonblocking_err_p(errno)) {
            pep_warning("Failed to connect! [%s:%d]", strerror(errno), errno);
            goto close_connection;
        }

        proxy->src.fd = connfd;
        proxy->dst.fd = out_fd;
        if (proxy->status == PST_CLOSED) {
            unpin_proxy(proxy);
            goto close_connection;
        }

        proxy->status = PST_CONNECT;
        unpin_proxy(proxy);
        PEP_DEBUG("Sending signal to poller [%d, %d]!", connfd, out_fd);
        if (pthread_kill(poller, POLLER_NEWCONN_SIG) != 0) {
            pep_error("Failed to send %d siganl to poller thread",
                      POLLER_NEWCONN_SIG);
        }

        continue;

close_connection:
        /*
         * Ok. Some error occured and we have to properly cleanup
         * all resources. Client socket must be closed and server
         * socket (if any) as well. Also it would be good if we
         * remove pep_proxy instance which caused an error from SYN
         * table.
         */

        close(connfd);
        if (out_fd >= 0) {
            close(out_fd);
        }
        if (proxy) {
            destroy_proxy(proxy);
        }
    }

    /* Normally this code won't be executed */
    PEP_DEBUG("Exiting...");
    pthread_exit(NULL);
}

static int prepare_poll_resources(void)
{
    struct pep_proxy *proxy;
    struct pep_endpoint *endp;
    struct pollfd *pfd;
    int i, j;
    enum proxy_status stat;

    i = 0;
    SYNTAB_LOCK_READ();
    syntab_foreach_connection(proxy) {
        /*
         * Proxy status field may be changed from
         * another thread(for example from listener thread)
         * So to prevent conflicts we copy it to the @stat
         * variable(reads are atomic) and only then compare the copy
         * with our patterns.
         */
        stat = proxy->status;
        if (stat == PST_PENDING || stat == PST_CLOSED)
            continue;

        for (j = 0; j < PROXY_ENDPOINTS; j++) {
            endp = &proxy->endpoints[j];
            pfd = &poll_resources.pollfds[i];
            pfd->fd = endp->fd;
            pfd->events = endp->poll_events;
            pfd->revents = 0;
            poll_resources.endpoints[i] = endp;
            i++;
        }
    }

    SYNTAB_UNLOCK_READ();
    return i;
}

/* An empty signal handler. It only needed to interrupt poll() */
static void poller_sighandler(int signo)
{
    PEP_DEBUG("Received signal %d", signo);
}

static void *poller_loop(void  __attribute__((unused)) *unused)
{
    int pollret, num_works, i, num_clients, iostat;
    struct pep_proxy *proxy;
    struct pep_endpoint *endp, *target;
    struct pollfd *pollfd;
    struct list_node *entry, *safe;
    struct list_head local_list;
    sigset_t sigset;
    struct sigaction sa;

    sigemptyset(&sigset);
    sigaddset(&sigset, POLLER_NEWCONN_SIG);
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = poller_sighandler;
    sa.sa_mask = sigset;
    if (sigaction(POLLER_NEWCONN_SIG, &sa, NULL) < 0) {
        pep_error("sigaction() error!");
    }

    sigprocmask(SIG_UNBLOCK, &sigset, NULL);

    for (;;) {
        list_init_head(&local_list);

        /*
         * At first we prepare file descriptors of all existing
         * connections in SYN table for poll. Whilie we're doing
         * it some new connection may appear in listener thread.
         * When listener accpets new connection and configures proxy
         * relevant proxy, it sends the POLLER_NEWCONN_SIG signal
         * to the poller thread. Thus while poller is busy with preparing
         * descriptors for poll, POLLER_NEWCONN_SIG should be blocked.
         * It performs poll() be preperly interrupted and renew descriptors.
         */
        sigprocmask(SIG_BLOCK, &sigset, NULL);
        num_clients = prepare_poll_resources();
        if (!num_clients) {
            sigprocmask(SIG_UNBLOCK, &sigset, NULL);
            sigwaitinfo(&sigset, NULL);
        continue;
        }

        sigprocmask(SIG_UNBLOCK, &sigset, NULL);
        pollret = poll(poll_resources.pollfds, num_clients, -1);
        if (pollret < 0) {
            if (errno == EINTR) {
                /* It seems that new client just appered. Renew descriptors. */
        continue;
            }

            pep_error("poll() error!");
        }
        else if (pollret == 0) {
            continue;
        }

        num_works = 0;
        for (i = 0; i < num_clients; i++) {
            pollfd = &poll_resources.pollfds[i];
            if (!pollfd->revents) {
                continue;
            }

            endp = poll_resources.endpoints[i];
            proxy = endp->owner;
            if (proxy->enqueued) {
                continue;
            }
            switch (proxy->status) {
                case PST_CONNECT:
                {
                    int ret, connerr, errlen = sizeof(int);

                    getsockopt(proxy->dst.fd, SOL_SOCKET, SO_ERROR,
                               &connerr, &errlen);
                    if (connerr != 0) {
                        destroy_proxy(proxy);
                        break;
                    }

                    ret = pepbuf_init(&proxy->src.buf);
                    if (ret < 0) {
                        pep_error("Failed to allocate PEP IN buffer!");
                    }

                    ret = pepbuf_init(&proxy->dst.buf);
                    if (ret < 0) {
                        pepbuf_deinit(&proxy->src.buf);
                        pep_error("Failed to allocate PEP OUT buffer!");
                    }

                    proxy->status = PST_OPEN;
                    setup_socket(proxy->src.fd);
                    setup_socket(proxy->dst.fd);
                }
                case PST_OPEN:
                {
                    if (pollfd->revents & (POLLHUP | POLLERR | POLLNVAL)) {
                        if (proxy->enqueued) {
                            list_del(&proxy->qnode);
                        }

                        destroy_proxy(proxy);
                        continue;
                    }

                    if (pollfd->revents & (POLLIN | POLLOUT)) {
                        list_add2tail(&local_list, &proxy->qnode);
                        num_works++;
                        proxy->enqueued = 1;
                    }

                    break;
                }
            }
        }
        if (list_is_empty(&local_list)) {
            continue;
        }

        /*
         * Now we're able to give connections with ready I/O status
         * to worker threads. Worker threads from PEPsal threads pool
         * will preform the I/O according to state of given connection
         * and move it back to the ready_queue when I/O job is finished.
         * Poller loop will wait until all connections it gave to worker
         * threads will be fully handled.
         */
        PEPQUEUE_LOCK(&active_queue);
        pepqueue_enqueue_list(&active_queue, &local_list, num_works);

        PEPQUEUE_LOCK(&ready_queue);
        PEPQUEUE_WAKEUP_WAITERS(&active_queue);
        PEPQUEUE_UNLOCK(&active_queue);

        /* Wait until connections are fully handled */
        while (ready_queue.num_items != num_works) {
            PEPQUEUE_WAIT(&ready_queue);
        }

        list_init_head(&local_list);
        pepqueue_dequeue_list(&ready_queue, &local_list);
        PEPQUEUE_UNLOCK(&ready_queue);

        /*
         * Now it's a time to handle connections after I/O is completed.
         * There are only two possible ways to do it:
         * 1) Close the connection if an I/O error occured or EOF was reached
         * 2) Continue work with connection and renew its I/O status
         */
        list_for_each_safe(&local_list, entry, safe) {
            proxy = list_entry(entry, struct pep_proxy, qnode);
            proxy->enqueued = 0;
            for (i = 0; i < PROXY_ENDPOINTS; i++) {
                endp = &proxy->endpoints[i];
                iostat = endp->iostat;
                if ((iostat & PEP_IOERR) ||
                    ((iostat & PEP_IOEOF) && pepbuf_empty(&endp->buf))) {
                    list_del(&proxy->qnode);
                    destroy_proxy(proxy);
                    break;
                }

                endp->iostat &= ~(PEP_IOWDONE | PEP_IORDONE | PEP_IOEOF);
            }
        }
    }
}

static void *workers_loop(void __attribute__((unused)) *unused)
{
    struct pep_proxy *proxy;
    struct list_head local_list;
    int ret, ready_items;

    PEPQUEUE_LOCK(&active_queue);
    for (;;) {
        list_init_head(&local_list);
        ready_items = 0;
        PEPQUEUE_WAIT(&active_queue);

        while (active_queue.num_items > 0) {
            proxy = pepqueue_dequeue(&active_queue);
            PEPQUEUE_UNLOCK(&active_queue);

            pep_proxy_data(&proxy->src, &proxy->dst);
            pep_proxy_data(&proxy->dst, &proxy->src);

            proxy->last_rxtx = time(NULL);
            list_add2tail(&local_list, &proxy->qnode);
            ready_items++;

            PEPQUEUE_LOCK(&active_queue);
        }

        PEPQUEUE_LOCK(&ready_queue);
        pepqueue_enqueue_list(&ready_queue, &local_list, ready_items);
        PEPQUEUE_UNLOCK(&ready_queue);
        PEPQUEUE_WAKEUP_WAITERS(&ready_queue);
    }
}

static void *timer_sch_loop(void __attribute__((unused)) *unused)
{
    struct timeval last_log_evt_time = {0U, 0U}, last_gc_evt_time = {0U, 0U}, now;

    if (logger.filename) {
        PEP_DEBUG("Setting up PEP logger");
        logger.file = fopen(logger.filename, "w+");
        if (!logger.file) {
            pep_error("Failed to open log file %s!", logger.filename);
        }
        gettimeofday(&last_log_evt_time, 0);
        gettimeofday(&last_gc_evt_time, 0);
    }
    
    for(;;) { 
        gettimeofday(&now, 0);
        if (logger.file && now.tv_sec > last_log_evt_time.tv_sec + PEPLOGGER_INTERVAL) {
            logger_fn();
            gettimeofday(&last_log_evt_time, 0);
        }

        if (now.tv_sec > last_gc_evt_time.tv_sec + gcc_interval) {
            garbage_connections_collector();
            gettimeofday(&last_gc_evt_time, 0);
        }
        sleep(2);
    }
}

static void init_pep_threads(void)
{
    int ret;
    PEP_DEBUG("Creating listener thread");
    ret = pthread_create(&listener, NULL, listener_loop, NULL);
    if (ret) {
        pep_error("Failed to create the listener thread! [RET = %d]", ret);
    }

    PEP_DEBUG("Creating poller thread");
    ret = pthread_create(&poller, NULL, poller_loop, NULL);
    if (ret < 0) {
        pep_error("Failed to create the poller thread! [RET = %d]", ret);
    }
    PEP_DEBUG("Creating timer_sch thread");
    ret = pthread_create(&timer_sch, NULL, timer_sch_loop, NULL);
    if (ret < 0) {
        pep_error("Failed to create the timer_sch thread! [RET = %d]", ret);
    }
}

static void init_pep_queues(void)
{
    PEP_DEBUG("Initialize PEP queue for active connections...");
    pepqueue_init(&active_queue);

    PEP_DEBUG("Initialize PEP queue for handled connections...");
    pepqueue_init(&ready_queue);
}

static void create_threads_pool(int num_threads)
{
    int ret, i;

    workers = calloc(num_threads, sizeof(pthread_t));
    if (!workers) {
        pep_error("Failed to create threads pool of %d threads!",
                  num_threads);
    }
    for (i = 0; i < num_threads; i++) {
        ret = pthread_create(&workers[i], NULL,
                             workers_loop, NULL);
        if (ret) {
            pep_error("Failed to create %d thread in pool!", i + 1);
        }
    }
}

int main(int argc, char *argv[])
{
    int c, ret, numfds;
    void *valptr;
    sigset_t sigset;

    memset(&logger, 0, sizeof(logger));
    while (1) {
        int option_index = 0;
        static struct option long_options[] = {
            {"daemon", 1, 0, 'd'},
            {"verbose", 1, 0, 'v'},
            {"help", 0, 0, 'h'},
            {"fastopen", 0, 0, 'f'},
            {"port", 1, 0, 'p'},
            {"version", 0, 0, 'V'},
            {"address", 1, 0, 'a'},
            {"logfile", 1, 0, 'l'},
            {"gcc_interval", 1, 0, 'g'},
            {"plifetime", 1, 0,'t'},
            {"conns", 1, 0, 'c'},
            {0, 0, 0, 0}
        };

        c = getopt_long(argc, argv, "dvVhfp:a:l:g:t:c:",
                        long_options, &option_index);
        if (c == -1)
            break;

        switch (c) {
            case 'd':
                background = 1;
                break;
            case 'v':
                DEBUG = 1;
                break;
            case 'h':
                usage(argv[0]); //implies exit
                break;
            case 'f':
                fastopen = 1;
                break;
            case 'p':
                portnum = atoi(optarg);
                break;
            case 'a':
                strncpy(pepsal_ip_addr, optarg, 19);
                break;
            case 'l':
                logger.filename = optarg;
                break;
            case 't':
                pending_conn_lifetime = atoi(optarg);
                break;
            case 'g':
                gcc_interval = atoi(optarg);
                break;
            case 'c':
                max_conns = atoi(optarg);
                if ((max_conns < PEP_MIN_CONNS) ||
                    (max_conns > PEP_MAX_CONNS)) {
                    usage(argv[0]);
                }

                break;
            case 'V':
                printf("PEPSal ver. %s\n", VERSION);
                exit(0);
        }
    }
    openlog(PROGRAM_NAME, LOG_PID, LOG_DAEMON);

    if (background) {
        PEP_DEBUG("Daemonizing...");
        if (daemon(0, 1) < 0) {
            pep_error("daemon() failed!");
        }
    }

    PEP_DEBUG("Init SYN table with %d max connections", max_conns);
    ret = syntab_init(max_conns);
    if (ret < 0) {
        pep_error("Failed to initialize SYN table!");
    }

    poll_resources.num_pollfds = numfds = max_conns * 2;
    poll_resources.pollfds = calloc(numfds, sizeof(struct pollfd));
    if (!poll_resources.pollfds) {
        pep_error("Failed to allocate %zd bytes for pollfds array!",
                  numfds * sizeof(struct pollfd));
    }

    poll_resources.endpoints = calloc(numfds, sizeof(struct pep_endpoint *));
    if (!poll_resources.endpoints) {
        free(poll_resources.pollfds);
        pep_error("Failed to allocate %zd bytes for pdescrs array!",
                  numfds * sizeof(struct pep_endpoint *));
    }

    sigemptyset(&sigset);
    sigaddset(&sigset, POLLER_NEWCONN_SIG);
    sigaddset(&sigset, SIGPIPE);
    sigprocmask(SIG_BLOCK, &sigset, NULL);
 /**************  Create AMQP THREAD ***************/
    pthread_t thread_id;
          struct rnis_t *params = malloc(sizeof(struct rnis_t));
          params->hostname="localhost";
          params->queuename="hello";
          params->cqi=0;
          params->txq=0;
          //printf("hostname [main] = %s",params->hostname);                                                                                                                                 
          pthread_create(&thread_id,NULL,rnis_subscribe,params);
 /**************  END AMQP THREAD *******************/

    init_pep_queues();
    init_pep_threads();
    create_threads_pool(PEPPOOL_THREADS);

    PEP_DEBUG("Pepsal started...");
    pthread_join(listener, &valptr);
    pthread_join(change_win, &valptr);
    pthread_join(poller, &valptr);
    pthread_join(timer_sch, &valptr);
    //pthread_join(thread_id, &valptr);
    //pthread_create(&thread_id,NULL,rnis_subscribe,params);
    PEP_DEBUG("exiting...\n");
    closelog();
    return 0;
}
