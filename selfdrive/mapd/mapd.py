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
        self.pm = messaging.PubMaster(['liveMapData'])
        self.logger.debug("entered mapsd_thread, ... %s" % ( str(self.pm)))

        self.params = Params()

        self.second = 0
        self.map_sign = 0
        self.target_speed_map_dist = 0
        self.map_enabled = False
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

    def opkr_map_status_read(self):        
        self.second += 0.25
        if self.second > 1.0:
            self.map_enabled = self.params.get_bool("OpkrMapEnable")
            self.second = 0.0
            


    def data_send(self):
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


    def read_map_data(self, opkr_signal ):
        try:
            mapsign = self.params.get( opkr_signal, encoding="utf8")
            mapsign = int(float(mapsign.rstrip('\n')))
            self.map_sign = mapsign
        except:
            mapsign = None

        return mapsign

    def make_map_data(self):
        mapsign = self.read_map_data( "OpkrMapSign" )
        mapspeed = self.read_map_data( "LimitSetSpeedCamera" )
        mapspeeddist = self.read_map_data( "LimitSetSpeedCameraDist" )

        if mapsign is not None:
            self.raw_map_sign = mapsign
        else:
            self.raw_map_sign = 0

        if mapspeed is not None:
            self.raw_target_speed_map = mapspeed
        else:
            self.raw_target_speed_map = 0

        if mapspeeddist is not None:
            self.raw_target_speed_map_dist = mapspeeddist
        else:
            self.raw_target_speed_map_dist = 0            

        if mapspeed is not None and mapspeeddist is not None:
            self.target_speed_map = self.raw_target_speed_map
            self.target_speed_map_dist = self.raw_target_speed_map_dist
            os.system("logcat -c &")
            self.target_speed_map_counter_check = False
        else:
            self.target_speed_map = 0
            self.target_speed_map_dist = 0
              

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
            if time.time() - start < 1.0:
              time.sleep(1.0)
              continue
            else:
              start = time.time()
              self.opkr_map_status_read()


            if not self.map_enabled:
              time.sleep(1.0)
              self.data_send()
              continue

            print( "opkr_map_status_read = {}".format( self.target_speed_map_counter  ))
            if self.target_speed_map_counter_check == False:
                self.target_speed_map_counter_check = True
                os.system("logcat -d -s opkrspdlimit,opkrspd2limit | grep opkrspd | tail -n 1 | awk \'{print $7}\' > /data/params/d/LimitSetSpeedCamera &")
                os.system("logcat -d -s opkrspddist,opkrspd2dist | grep opkrspd | tail -n 1 | awk \'{print $7}\' > /data/params/d/LimitSetSpeedCameraDist &")
                os.system("logcat -d -s opkrsigntype,opkrspdsign | grep opkrspd | tail -n 1 | awk \'{print $7}\' > /data/params/d/OpkrMapSign &")
                print( "self.map_enabled = {}".format( self.map_enabled  ))
                self.target_speed_map_counter = 0
            else:
                print( "raw_map_sign= {} raw_target_speed_map={},{}".format( self.raw_map_sign, self.raw_target_speed_map, self.raw_target_speed_map_dist  ))
                print( "old_map_sign= {} old_target_speed_map={},{}".format( self.old_map_sign, self.old_target_speed_map, self.old_target_speed_map_dist  ))
                self.make_map_data()
                self.target_speed_map_counter += 1
                if self.target_speed_map_counter > 50:
                    os.system("logcat -c &")
                    self.target_speed_map_counter_check = False

                
                self.data_send()

            



def main():
    #gc.disable()
    set_realtime_priority(1)
    params = Params()
    dongle_id = params.get("DongleId")
   

    mt = MapsdThread(1, "/data/media/0/videos/MapsdThread")
    mt.start()

if __name__ == "__main__":
    main()
