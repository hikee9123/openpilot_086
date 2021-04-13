import math
import numpy as np



from selfdrive.config import Conversions as CV
from selfdrive.car.hyundai.hyundaican import create_acc_commands
from common.numpy_fast import clip, interp


import common.log as trace1




class CLongControl():
    def __init__(self, p ):
        self.p = p
        


    def accel_hysteresis( self, accel, accel_steady):
        # for small accel oscillations within ACCEL_HYST_GAP, don't change the accel command
        if accel > accel_steady + self.p.ACCEL_HYST_GAP:
        accel_steady = accel - self.p.ACCEL_HYST_GAP
        elif accel < accel_steady - self.p.ACCEL_HYST_GAP:
        accel_steady = accel + self.p.ACCEL_HYST_GAP
        accel = accel_steady

        return accel, accel_steady

    def accel_applay( self, actuators):
        # gas and brake
        apply_accel = actuators.gas - actuators.brake
        apply_accel, self.accel_steady = self.accel_hysteresis(apply_accel, self.accel_steady)
        apply_accel = clip(apply_accel * self.p.ACCEL_SCALE, self.p.ACCEL_MIN, self.p.ACCEL_MAX)
        return  apply_accel


    def update( self, enabled, c, kph_vEgo, frame, scc12_cnt ):
        # send scc to car if longcontrol enabled and SCC not on bus 0 or ont live
        actuators = c.actuators
        set_speed = c.hudControl.setSpeed
        lead_visible = c.hudControl.leadVisible
        stopping = kph_vEgo <= 1
        apply_accel = self.accel_applay(  actuators )
        can_send = create_acc_commands(self.packer, enabled, apply_accel, frame, lead_visible, set_speed, stopping, scc12_cnt  )

        str_log2 = 'accel={:.0f}  speed={:.0f} lead={} stop={:.0f}'.format( apply_accel, set_speed,  lead_visible, stopping )
        trace1.printf2( '{}'.format( str_log2 ) )     
        return can_send
