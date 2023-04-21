To execute this environment with MECPerf, the procedure is:
1. Clone the repository;
2. Execute the command "python3 deploy.py -s inira_rnis -P0 -Q32 -l";
3. Then you need to add the rules specified in "After deploy.py.txt";
4. You need to open a terminal for each host (and change directory in /home/MECPerf/MECPerf-master);

Now, on fit26 (Aggregator) you can execute the following instructions:

cd /home/MECPerf/MECPerf-master/RESTAggregator

./compileRest.sh

./startRest.sh

cd ..

./buildDB.sh psw psw

./startAggregator.sh

Fit07 (Remote server) --> ./startServer.sh

Fit09 (Observer) --> ./startObserver.sh

Fit32 (Client) --> ./startLocal.sh
