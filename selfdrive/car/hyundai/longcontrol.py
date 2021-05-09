import math
import numpy as np



from selfdrive.config import Conversions as CV
from selfdrive.car.hyundai.hyundaican import create_scc11, create_scc12
from selfdrive.car.hyundai.values import Buttons
from common.numpy_fast import clip, interp


import common.log as trace1




class CLongControl():
  def __init__(self, p ):
    self.p = p
    self.accel_steady = 0
    self.scc12_cnt = 0
    #self.scc12_cnt %= 0xF    
      
  def reset( self, CS ):
    self.scc12_cnt = CS.scc12["CR_VSM_Alive"] + 1     

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



  def update( self, packer, CS, c, frame ):
    enabled = CS.acc_active
    kph_vEgo = CS.out.vEgo * CV.MS_TO_KPH    
    # send scc to car if longcontrol enabled and SCC not on bus 0 or ont live
    actuators = c.actuators
    set_speed = c.hudControl.setSpeed
    lead_visible = c.hudControl.leadVisible
    stopping = kph_vEgo <= 1
    apply_accel = self.accel_applay(  actuators )
    scc_live = True

    if CS.aReqValue > 0:  
      apply_accel = 0
    else:
      apply_accel = CS.aReqValue
    


    can_sends = create_scc12(packer, apply_accel, enabled, self.scc12_cnt, scc_live, CS.scc12)
    can_sends.append( create_scc11(packer, frame, enabled, set_speed, lead_visible, scc_live, CS.scc11) )
    self.scc12_cnt += 1 

    str_log2 = 'accel={:.3f}  speed={:.0f} lead={} stop={:.0f}'.format( apply_accel, set_speed,  lead_visible, stopping )
    trace1.printf3( '{}'.format( str_log2 ) )
    return can_sends
