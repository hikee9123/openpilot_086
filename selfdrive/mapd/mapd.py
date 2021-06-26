#!/usr/bin/env python3
import gc

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




class MapsdThread(threading.Thread):
    def __init__(self):
        threading.Thread.__init__(self)
        self.threadID =  1
        self.name  =  'mapd'
        self.logger = logging.getLogger( self.name )
        self.pm = messaging.PubMaster(['liveMapData'])
        self.logger.debug("entered mapsd_thread, ... %s" % ( str(self.pm)))

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
            if time.time() - start < 1.5:
                time.sleep(1.0)
                continue
            else:
                start = time.time()

            dat = messaging.new_message()
            dat.init('liveMapData')



            dat.liveMapData.wayId = 1


            # Speed limit
            max_speed = 10


            #dat.liveMapData.speedLimitAheadValid = True
            #dat.liveMapData.speedLimitAhead = float(max_speed_ahead)
            #dat.liveMapData.speedLimitAheadDistance = float(max_speed_ahead_dist)

            # Curvature
            #dat.liveMapData.curvatureValid = curvature_valid
            #dat.liveMapData.curvature = float(upcoming_curvature)
            #dat.liveMapData.distToTurn = float(dist_to_turn)

            #dat.liveMapData.roadCurvatureX = [float(x) for x in dists]
            #dat.liveMapData.roadCurvature = [float(x) for x in curvature]
            dat.liveMapData.speedLimitValid = True
            dat.liveMapData.speedLimit = max_speed

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
