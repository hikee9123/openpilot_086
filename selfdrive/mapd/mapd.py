#!/usr/bin/env python3
import gc
import os
import time
import math
import overpy
import socket
import requests
import threading
import numpy as np
# setup logging
import logging
import logging.handlers
import selfdrive.crash as crash
from common.params import Params
from collections import defaultdict
import cereal.messaging as messaging
from common.realtime import set_realtime_priority

from selfdrive.version import version, dirty

# OPKR 코드 참조.

class MapsdThread(threading.Thread):
    def __init__(self, threadID, name):
        threading.Thread.__init__(self)
        self.threadID =  threadID
        self.name  =  name
        self.logger = logging.getLogger( self.name )
        #self.pm = messaging.PubMaster(['liveMapData'])
        #self.logger.debug("entered mapsd_thread, ... %s" % ( str(self.pm)))

        self.params = Params()
        self.params.put("OpkrMapEnable", "0")
        self.second = 0
        self.map_sign = 0
        self.target_speed_map_dist = 0
        self.map_enabled = 0
        self.old_map_enable = 0        
        self.target_speed_map = 0

        self.raw_map_sign = 0
        self.raw_target_speed_map = 0
        self.raw_target_speed_map_dist = 0

        self.old_map_sign = 0
        self.old_target_speed_map = 0
        self.old_target_speed_map_dist = 0        

        self.target_speed_map_counter = 0
        self.target_speed_map_counter1 = 0
        self.target_speed_map_counter2 = 0
        self.target_speed_map_counter3 = 0
        self.target_speed_map_counter_check = False

        self.map_exec_status= False
        self.programRun = True


    def opkr_map_status_read(self):        
        self.map_enabled = int(self.params.get("OpkrMapEnable"))


        if self.map_enabled == 2 and self.target_speed_map_counter2 > 0:
            self.target_speed_map_counter2 -= 1
            print( " target_speed_map_counter2 = {}".format( self.target_speed_map_counter2  ))
            if self.target_speed_map_counter2  == 0:
                print( "am start --activity-task-on-home com.opkr.maphack/com.opkr.maphack.MainActivity" )
                os.system("am start --activity-task-on-home com.opkr.maphack/com.opkr.maphack.MainActivity")
                self.params.put("OpkrMapEnable", "1")
                self.programRun = False
                return

        if( self.map_enabled == self.old_map_enable ):
            return

        self.old_map_enable = self.map_enabled
        self.target_speed_map_counter1 = 3

        if self.map_enabled == 0:
            self.map_exec_status = False
            # os.system("pkill com.mnsoft.mappyobn")
            # map return
            os.system("am start --activity-task-on-home com.mnsoft.mappyobn/com.mnsoft.mappy.MainActivity")
        elif self.map_exec_status == False: 
            self.map_exec_status = True
            #print( "am start com.skt.tmap.ku/com.skt.tmap.activity.TmapNaviActivity" )
            #os.system("am start com.skt.tmap.ku/com.skt.tmap.activity.TmapNaviActivity")
            os.system("am start com.mnsoft.mappyobn/com.mnsoft.mappy.MainActivity &")
            if self.map_enabled == 2:  # map 실행후 2초후 Overlay mode로 변경합니다.
                self.target_speed_map_counter2 = 2
        elif self.map_enabled == 2:
               #os.system("am start com.mnsoft.mappyobn/com.mnsoft.mappy.MainActivity &")
               os.system("am start --activity-task-on-home com.opkr.maphack/com.opkr.maphack.MainActivity")
               self.params.put("OpkrMapEnable", "1")




              

    def run(self):
        # OPKR
        self.navi_on_boot = self.params.get_bool("OpkrRunNaviOnBoot")
        
        if self.navi_on_boot:
            self.params.put("OpkrMapEnable", "2")

        start = time.time()
        while True:
            if time.time() - start < 1.0:
              time.sleep(1.0)
              continue
            else:
              start = time.time()
              self.opkr_map_status_read()

          



def main():
    #gc.disable()
    #set_realtime_priority(1)
    params = Params()
    dongle_id = params.get("DongleId")
   

    mt = MapsdThread(1, "/data/media/0/videos/MapsdThread")
    mt.start()

if __name__ == "__main__":
    main()
