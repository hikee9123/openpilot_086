import math
import numpy as np



from selfdrive.config import Conversions as CV
from selfdrive.car.hyundai.hyundaican import create_scc11, create_scc12
from selfdrive.car.hyundai.values import Buttons
from selfdrive.controls.lib.lane_planner import TRAJECTORY_SIZE
from common.numpy_fast import clip, interp


import common.log as trace1



MAX_SPEED = 255.0
MIN_CURVE_SPEED = 30.


class CLongControl():
  def __init__(self, p = None ):
    self.p = p
    self.accel_steady = 0
    self.scc12_cnt = 0
    
    self.btn_cnt = 0
    self.seq_command = 0
    self.target_speed = 0
    self.set_point = 0
    self.wait_timer2 = 0

    self.prev_clu_CruiseSwState = 0       
    self.prev_VSetDis  = 0
    self.curise_set_first = 0
    self.curise_sw_check = False
    self.cruise_set_mode = 1      # 초기 선택 모델.
    self.cruise_set_speed_kph = 30

    self.curve_speed = 0
    self.curvature_gain = 1



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



  def update_btn(self, CS ): 
    if CS.driverOverride == 2 or not CS.acc_active or CS.cruise_buttons == Buttons.RES_ACCEL or CS.cruise_buttons == Buttons.SET_DECEL or CS.cruise_buttons == Buttons.CANCEL: 
      self.wait_timer2 = 50 
    elif self.wait_timer2: 
      self.wait_timer2 -= 1
    else:
      return 1
    return 0


  def update_cruiseSW(self, CS ):
    set_speed_kph = self.cruise_set_speed_kph
    delta_vsetdis = 0
    if CS.acc_active:
        delta_vsetdis = abs(CS.VSetDis - self.prev_VSetDis)            
        if self.prev_clu_CruiseSwState != CS.cruise_buttons:
          if CS.cruise_buttons:
            self.prev_VSetDis = int(CS.VSetDis)
          elif CS.driverOverride:
            set_speed_kph = int(CS.VSetDis)          
          elif self.prev_clu_CruiseSwState == Buttons.RES_ACCEL:   # up 
            if self.curise_set_first:
                self.curise_set_first = 0
                set_speed_kph =  int(CS.VSetDis)
            elif delta_vsetdis > 5:
                set_speed_kph = CS.VSetDis
            elif not self.curise_sw_check:
                set_speed_kph += 1
          elif self.prev_clu_CruiseSwState == Buttons.SET_DECEL:  # dn
            if self.curise_set_first:
                self.curise_set_first = 0
                set_speed_kph = int(CS.clu_Vanz)
            elif delta_vsetdis > 5:
                set_speed_kph = int(CS.VSetDis)
            elif not self.curise_sw_check:
                set_speed_kph -= 1

          self.prev_clu_CruiseSwState = CS.cruise_buttons
        elif CS.cruise_buttons and delta_vsetdis > 0:
          self.curise_sw_check = True
          set_speed_kph = int(CS.VSetDis)
    else:
        self.curise_sw_check = False
        self.curise_set_first = 1
        self.prev_VSetDis = int(CS.VSetDis)
        set_speed_kph = CS.VSetDis
        if not CS.acc_active and self.prev_clu_CruiseSwState != CS.cruise_buttons:  # MODE GAP
          if CS.cruise_buttons == Buttons.GAP_DIST: 
            self.cruise_set_mode += 1
          if self.cruise_set_mode > 4:
            self.cruise_set_mode = 0
          self.prev_clu_CruiseSwState = CS.cruise_buttons


    if set_speed_kph < 30:
      set_speed_kph = 30

    self.cruise_set_speed_kph = set_speed_kph
    return self.cruise_set_mode, set_speed_kph



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

    #if kph_vEgo < 30:
    #  apply_accel = min( apply_accel, CS.aReqValue )
    if CS.aReqValue > apply_accel:
      apply_accel = apply_accel
    else:
      apply_accel = CS.aReqValue
      self.scc12_cnt += 1
      return None
    
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
      #if self.btn_cnt == 1:
      #  btn_signal = Buttons.NONE
      if self.btn_cnt > 5: 
        self.seq_command = 0
      return btn_signal


  def update_scc( self, CS, set_speed ):
    self.set_point = max(30,set_speed)
    self.curr_speed = CS.out.vEgo * CV.MS_TO_KPH
    self.VSetDis   = CS.VSetDis
    btn_signal = self.switch( self.seq_command )

    return btn_signal
