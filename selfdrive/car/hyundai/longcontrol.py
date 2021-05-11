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
    
    self.btn_cnt = 0
    self.seq_command = 0
    self.target_speed = 0
    self.set_point = 0
      
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

    if CS.aReqValue > -0.04:
      apply_accel = -0.04
    else:
      apply_accel = CS.aReqValue
    
    can_sends = create_scc12(packer, apply_accel, enabled, self.scc12_cnt, scc_live, CS.scc12)
    can_sends.append( create_scc11(packer, frame, enabled, set_speed, lead_visible, scc_live, CS.scc11) )
    self.scc12_cnt += 1 

    str_log2 = 'accel={:.3f}  speed={:.0f} lead={} stop={:.0f}'.format( apply_accel, set_speed,  lead_visible, stopping )
    trace1.printf3( '{}'.format( str_log2 ) )
    return can_sends


  # buttn acc,dec control
  def switch(self, seq_cmd):
      self.case_name = "case_" + str(seq_cmd)
      self.case_func = getattr( self, self.case_name, lambda:"default")
      return self.case_func()

  def reset_btn(self):
      if self.seq_command != 3:
        self.seq_command = 0


  def case_default(self):
      self.seq_command = 0
      return None

  def case_0(self):
      self.btn_cnt = 0
      self.target_speed = self.set_point
      delta_speed = self.target_speed - self.VSetDis
      if delta_speed > 1:
        self.seq_command = 1
      elif delta_speed < -1:
        self.seq_command = 2
      return None

  def case_1(self):  # acc
      btn_signal = Buttons.RES_ACCEL
      self.btn_cnt += 1
      if self.target_speed == self.VSetDis:
        self.btn_cnt = 0
        self.seq_command = 3            
      elif self.btn_cnt > 10:
        self.btn_cnt = 0
        self.seq_command = 3
      return btn_signal


  def case_2(self):  # dec
      btn_signal = Buttons.SET_DECEL
      self.btn_cnt += 1
      if self.target_speed == self.VSetDis:
        self.btn_cnt = 0
        self.seq_command = 3            
      elif self.btn_cnt > 10:
        self.btn_cnt = 0
        self.seq_command = 3
      return btn_signal

  def case_3(self):  # None
      btn_signal = None  # Buttons.NONE
      
      self.btn_cnt += 1
      if self.btn_cnt == 1:
        btn_signal = Buttons.NONE
      elif self.btn_cnt > 5: 
        self.seq_command = 0
      return btn_signal


  def update_scc( self, CS, set_speed ):
    self.set_point = max(30,set_speed)
    self.curr_speed = CS.out.vEgo * CV.MS_TO_KPH
    self.VSetDis   = CS.VSetDis
    btn_signal = self.switch( self.seq_command )

    return btn_signal
