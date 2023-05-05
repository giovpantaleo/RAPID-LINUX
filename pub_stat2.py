"""
    Hello world `send.py` example implementation using aioamqp.
    See the documentation for more informations.

"""

import asyncio
import requests
from datetime import time
import time


#parser = argparse.ArgumentParser()
#parser.add_argument("--rnti","-r",help="RNTI of the UE",required=True)
#args = parser.parse_args()
def get_ue_info():
    #URL="https://httpbin.org/get"
    URL="http://127.0.0.1:9999/stats/"
    r = requests.get(url = URL)
    data = r.json()
    time.sleep(0.0005)
    return data
    
async def send():
    # insert the name of the file CSV
    csv_file = "output.csv"

    # open file CSV
    with open(csv_file, 'w+') as file:

        while 1:
            obj = get_ue_info()
            file.write(obj+'\n')
        
            
asyncio.get_event_loop().run_until_complete(send())
#send()
