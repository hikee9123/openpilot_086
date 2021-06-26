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


# OPKR 코드 참고.

class MapsdThread(threading.Thread):
    def __init__(self):
        threading.Thread.__init__(self)
        self.threadID =  1
        self.name  =  'mapd'
        self.logger = logging.getLogger( self.name )
        self.pm = messaging.PubMaster(['liveMapData'])
        self.logger.debug("entered mapsd_thread, ... %s" % ( str(self.pm)))

        self.params = Params()

        self.map_sign = 0
        self.target_speed_map_dist = 0
        self.map_enabled = False
        self.target_speed_map = 0



    def run(self):
        self.logger.debug("Entered run method for thread :" + str(self.name))
        cur_way = None
        curvature_valid = False
        curvature = None
        upcoming_curvature = 0.
        dist_to_turn = 0.
        road_points = None
        max_speed = None
        max_speed_ahead = None
        max_speed_ahead_dist = None
        max_speed_prev = 0
        start = time.time()
        while True:
            if time.time() - start < 0.1:
              time.sleep(0.1)
              continue
            else:
              start = time.time()

            self.second += 0.25
            if self.second > 1.0:
              self.map_enabled = self.params.get_bool("OpkrMapEnable")
              self.second = 0.0

            if not self.map_enabled:
              time.sleep(1.0)
              continue


            self.target_speed_map_counter += 1
            if self.target_speed_map_counter >= (50+self.target_speed_map_counter1) and self.target_speed_map_counter_check == False:
                self.target_speed_map_counter_check = True
                os.system("logcat -d -s opkrspdlimit,opkrspd2limit | grep opkrspd | tail -n 1 | awk \'{print $7}\' > /data/params/d/LimitSetSpeedCamera &")
                os.system("logcat -d -s opkrspddist,opkrspd2dist | grep opkrspd | tail -n 1 | awk \'{print $7}\' > /data/params/d/LimitSetSpeedCameraDist &")
                os.system("logcat -d -s opkrsigntype,opkrspdsign | grep opkrspd | tail -n 1 | awk \'{print $7}\' > /data/params/d/OpkrMapSign &")
                self.target_speed_map_counter3 += 1
                if self.target_speed_map_counter3 > 2:
                    self.target_speed_map_counter3 = 0
                    os.system("logcat -c &")
            elif self.target_speed_map_counter >= (75+self.target_speed_map_counter1):
                self.target_speed_map_counter1 = 0
                self.target_speed_map_counter = 0
                self.target_speed_map_counter_check = False
                try:
                    mapspeed = self.params.get("LimitSetSpeedCamera", encoding="utf8")
                    mapspeeddist = self.params.get("LimitSetSpeedCameraDist", encoding="utf8")
                    mapsign = self.params.get("OpkrMapSign", encoding="utf8")
                    if mapsign is not None:

                        try:
                            mapsign = int(float(mapsign.rstrip('\n')))
                            self.map_sign = mapsign
                        except:
                            pass

                    else:
                        mapsign = 0
                        self.map_sign = mapsign

                    if mapspeed is not None and mapspeeddist is not None:

                        try:
                            mapspeed = int(float(mapspeed.rstrip('\n')))
                            mapspeeddist = int(float(mapspeeddist.rstrip('\n')))
                        except:
                            pass

                        if mapspeed > 29:
                            self.target_speed_map = mapspeed
                            self.target_speed_map_dist = mapspeeddist
                            if self.target_speed_map_dist > 1001:
                                self.target_speed_map_block = True
                            self.target_speed_map_counter1 = 80
                            os.system("logcat -c &")
                        else:
                            self.target_speed_map = 0
                            self.target_speed_map_dist = 0
                            self.target_speed_map_block = False
                    elif mapspeed is None and mapspeeddist is None and self.target_speed_map_counter2 < 2:
                        self.target_speed_map_counter2 += 1
                        self.target_speed_map_counter = 51
                        self.target_speed_map = 0
                        self.target_speed_map_dist = 0
                        self.target_speed_map_counter_check = True
                        self.target_speed_map_block = False
                        self.target_speed_map_sign = False
                    else:
                        self.target_speed_map_counter = 49
                        self.target_speed_map_counter2 = 0
                        self.target_speed_map = 0
                        self.target_speed_map_dist = 0
                        self.target_speed_map_counter_check = False
                        self.target_speed_map_block = False
                        self.target_speed_map_sign = False
                except:
                    pass


            dat = messaging.new_message()
            dat.init('liveMapData')
            dat.liveMapData.wayId = 1
            # Speed limit
            #dat.liveMapData.speedLimitAheadValid = True
            dat.liveMapData.speedLimitAhead = self.map_sign
            dat.liveMapData.speedLimitAheadDistance = self.target_speed_map_dist

            # Curvature
            #dat.liveMapData.curvatureValid = curvature_valid
            #dat.liveMapData.curvature = float(upcoming_curvature)
            #dat.liveMapData.distToTurn = float(dist_to_turn)

            #dat.liveMapData.roadCurvatureX = [float(x) for x in dists]
            #dat.liveMapData.roadCurvature = [float(x) for x in curvature]
            dat.liveMapData.speedLimitValid = self.map_enabled
            dat.liveMapData.speedLimit = self.target_speed_map

            dat.liveMapData.mapValid = True
        
            self.logger.debug("Sending ... liveMapData ... %s", str(dat))
            self.pm.send('liveMapData', dat)



def main():
    gc.disable()
    set_realtime_priority(54)    
    params = Params()


    mt = MapsdThread()
    mt.start()

if __name__ == "__main__":
    main()
