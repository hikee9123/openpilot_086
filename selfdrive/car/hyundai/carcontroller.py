from cereal import car
from common.realtime import DT_CTRL
from selfdrive.car import apply_std_steer_torque_limits
from selfdrive.car.hyundai.hyundaican import create_lkas11, create_clu11, create_lfahda_mfc, create_mdps12
from selfdrive.car.hyundai.values import Buttons, CarControllerParams, CAR
from opendbc.can.packer import CANPacker


import copy
import common.log as trace1
import common.CTime1000 as tm
from common.numpy_fast import interp
from selfdrive.config import Conversions as CV

VisualAlert = car.CarControl.HUDControl.VisualAlert



class CarController():
  def __init__(self, dbc_name, CP, VM):
    self.p = CarControllerParams(CP)
    self.packer = CANPacker(dbc_name)

    self.apply_steer_last = 0
    self.car_fingerprint = CP.carFingerprint
    self.steer_rate_limited = False
    self.last_resume_frame = 0

    self.model_speed = 0

  """
  def atom_steerRatio( self, v_ego_kph, cv_value,  atomTuning ):  
    self.sr_KPH = atomTuning.cvKPH
    self.sr_BPV = atomTuning.cvBPV
    self.cv_steerRatioV = atomTuning.cvsteerRatioV
    self.sr_SteerRatio = []

    nPos = 0
    for steerRatio in self.sr_BPV:  # steerRatio
      self.sr_SteerRatio.append( interp( cv_value, steerRatio, self.cv_steerRatioV[nPos] ) )
      nPos += 1
      if nPos > 20:
        break

    steerRatio = interp( v_ego_kph, self.sr_KPH, self.sr_SteerRatio )

    return steerRatio

  def atom_actuatorDelay( self, v_ego_kph, cv_value, atomTuning ):
    self.sr_KPH = atomTuning.cvKPH
    self.sr_BPV = atomTuning.cvBPV
    self.cv_ActuatorDelayV = atomTuning.cvsteerActuatorDelayV
    self.sr_ActuatorDelay = []

    nPos = 0
    for steerRatio in self.sr_BPV:
      self.sr_ActuatorDelay.append( interp( cv_value, steerRatio, self.cv_ActuatorDelayV[nPos] ) )
      nPos += 1
      if nPos > 10:
        break

    actuatorDelay = interp( v_ego_kph, self.sr_KPH, self.sr_ActuatorDelay )

    return actuatorDelay
  """

  def atom_tune( self, v_ego_kph, cv_value ):  # cV
    self.cv_KPH = self.CP.atomTuning.cvKPH
    self.cv_BPV = self.CP.atomTuning.cvBPV
    self.cv_sMaxV  = self.CP.atomTuning.cvsMaxV
    self.cv_sdUpV = self.CP.atomTuning.cvsdUpV
    self.cv_sdDnV = self.CP.atomTuning.cvsdDnV

    self.steerMAX = []
    self.steerdUP = []
    self.steerdDN = []

    # Max
    nPos = 0
    for sCV in self.cv_BPV:  
      self.steerMAX.append( interp( cv_value, sCV, self.cv_sMaxV[nPos] ) )
      self.steerdUP.append( interp( cv_value, sCV, self.cv_sdUpV[nPos] ) )
      self.steerdDN.append( interp( cv_value, sCV, self.cv_sdDnV[nPos] ) )
      nPos += 1
      if nPos > 20:
        break

    MAX = interp( v_ego_kph, self.cv_KPH, self.steerMAX )
    UP  = interp( v_ego_kph, self.cv_KPH, self.steerdUP )
    DN  = interp( v_ego_kph, self.cv_KPH, self.steerdDN )

    str_log1 = 'ego={:.1f} /{:.1f}/{:.1f}/{:.1f}'.format(v_ego_kph,  MAX, UP, DN )
    trace1.printf2( '{}'.format( str_log1 ) )      
    return MAX, UP, DN


  def steerParams_torque(self, CS, actuators ):
    param = copy.copy(self.p)
    v_ego_kph = CS.out.vEgo * CV.MS_TO_KPH

    nMAX, nUP, nDN = self.atom_tune( v_ego_kph, self.model_speed )
    param.STEER_MAX = min( param.STEER_MAX, nMAX)
    param.STEER_DELTA_UP = min( param.STEER_DELTA_UP, nUP)
    param.STEER_DELTA_DOWN = min( param.STEER_DELTA_DOWN, nDN )
    return  param    

  def process_hud_alert(self, enabled, c ):
    visual_alert = c.hudControl.visualAlert
    left_lane = c.hudControl.leftLaneVisible
    right_lane = c.hudControl.rightLaneVisible

    sys_warning = (visual_alert == VisualAlert.steerRequired)

    if left_lane:
      self.hud_timer_left = 100

    if right_lane:
      self.hud_timer_right = 100

    if self.hud_timer_left:
      self.hud_timer_left -= 1
 
    if self.hud_timer_right:
      self.hud_timer_right -= 1


    # initialize to no line visible
    sys_state = 1

    if self.hud_timer_left and self.hud_timer_right or sys_warning:  # HUD alert only display when LKAS status is active
      if (self.steer_torque_ratio > 0.7) and (enabled or sys_warning):
        sys_state = 3
      else:
        sys_state = 4
    elif self.hud_timer_left:
      sys_state = 5
    elif self.hud_timer_right:
      sys_state = 6

    return sys_warning, sys_state    

  def update(self, c, CS, frame ):
    enabled = c.enabled
    actuators  = c.actuators
    pcm_cancel_cmd  = c.cruiseControl.cancel
    self.model_speed = c.modelSpeed

    # Steering Torque
    param = self.steerParams_torque( CS, c.actuators )
    new_steer = int(round(actuators.steer * param.STEER_MAX))
    apply_steer = apply_std_steer_torque_limits(new_steer, self.apply_steer_last, CS.out.steeringTorque, param)
    self.steer_rate_limited = new_steer != apply_steer

    # disable if steer angle reach 90 deg, otherwise mdps fault in some models
    lkas_active = enabled and abs(CS.out.steeringAngleDeg) < CS.CP.maxSteeringAngleDeg

    # fix for Genesis hard fault at low speed
    if CS.out.vEgo < 16.7 and self.car_fingerprint == CAR.HYUNDAI_GENESIS:
      lkas_active = False

    if not lkas_active:
      apply_steer = 0

    self.apply_steer_last = apply_steer

    sys_warning, self.hud_sys_state = self.process_hud_alert( lkas_active, c )

    can_sends = []
    can_sends.append(create_lkas11(self.packer, frame, self.car_fingerprint, apply_steer, lkas_active,
                                   CS.lkas11, sys_warning, self.hud_sys_state, c))

    if apply_steer:
      can_sends.append( create_mdps12(self.packer, frame, CS.mdps12) )


    str_log1 = 'torg:{:5.0f} dn={:.1f} up={:.1f}'.format( apply_steer, param.STEER_DELTA_DOWN, param.STEER_DELTA_UP   )
    str_log2 = 'gas={:.1f}'.format(  CS.out.gas  )
    trace1.printf( '{} {}'.format( str_log1, str_log2 ) )

    if pcm_cancel_cmd:
      can_sends.append(create_clu11(self.packer, frame, CS.clu11, Buttons.CANCEL))
    elif CS.out.cruiseState.standstill:

      # send resume at a max freq of 10Hz
      if CS.lead_distance != self.last_lead_distance and (frame - self.last_resume_frame) * DT_CTRL > 0.1:
        # send 25 messages at a time to increases the likelihood of resume being accepted
        can_sends.append(create_clu11(self.packer, frame, CS.clu11, Buttons.RES_ACCEL))
        self.last_resume_frame = frame
    else:
        self.last_lead_distance = CS.lead_distance

    # 20 Hz LFA MFA message
    if frame % 5 == 0 and self.car_fingerprint in [CAR.SONATA, CAR.PALISADE, CAR.IONIQ, CAR.KIA_NIRO_EV,
                                                   CAR.IONIQ_EV_2020, CAR.KIA_CEED, CAR.KIA_SELTOS]:
      can_sends.append(create_lfahda_mfc(self.packer, enabled))

    return can_sends
