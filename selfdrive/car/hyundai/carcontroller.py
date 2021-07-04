from cereal import car, log
from common.realtime import DT_CTRL
from selfdrive.car import apply_std_steer_torque_limits
from selfdrive.car.hyundai.hyundaican import create_lkas11, create_clu11, create_lfahda_mfc, create_mdps12
from selfdrive.car.hyundai.values import Buttons, CarControllerParams, CAR, FEATURES
from opendbc.can.packer import CANPacker
from selfdrive.config import Conversions as CV
from common.numpy_fast import clip, interp


# long controller
from selfdrive.car.hyundai.longcontrol  import CLongControl

from common.params import Params
import common.log as trace1
import common.CTime1000 as tm
import common.MoveAvg as moveavg1
import copy

VisualAlert = car.CarControl.HUDControl.VisualAlert
LaneChangeState = log.LateralPlan.LaneChangeState




class CarController():
  def __init__(self, dbc_name, CP, VM):
    self.CP = CP
    self.p = CarControllerParams(CP)
    self.packer = CANPacker(dbc_name)

    self.apply_steer_last = 0
    self.car_fingerprint = CP.carFingerprint
    self.steer_rate_limited = False
    self.last_resume_frame = 0
    self.last_lead_distance = 0

    self.resume_cnt = 0
    self.lkas11_cnt = 0


    self.steer_torque_over_timer = 0
    self.steer_torque_ratio = 1

    self.timer1 = tm.CTime1000("time1")
    
    # hud
    self.hud_timer_left = 0
    self.hud_timer_right = 0
    self.hud_sys_state = 0


    self.params = Params()

    # long control
    self.longCtrl = CLongControl(self.p)
    self.liveMapData = None




  def limit_ctrl(self, value, limit, offset ):
    p_limit = offset + limit
    m_limit = offset - limit
    if value > p_limit:
        value = p_limit
    elif  value < m_limit:
        value = m_limit
    return value


  def process_hud_alert(self, enabled, c ):
    visual_alert = c.hudControl.visualAlert
    left_lane = c.hudControl.leftLaneVisible
    right_lane = c.hudControl.rightLaneVisible

    sys_warning = (visual_alert in [VisualAlert.steerRequired, VisualAlert.ldw])  
    #(visual_alert == VisualAlert.steerRequired)

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



  def atom_tune( self, v_ego_kph, angleDeg ):  # cV
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
      self.steerMAX.append( interp( angleDeg, sCV, self.cv_sMaxV[nPos] ) )
      self.steerdUP.append( interp( angleDeg, sCV, self.cv_sdUpV[nPos] ) )
      self.steerdDN.append( interp( angleDeg, sCV, self.cv_sdDnV[nPos] ) )
      nPos += 1
      if nPos > 20:
        break

    MAX = interp( v_ego_kph, self.cv_KPH, self.steerMAX )
    UP  = interp( v_ego_kph, self.cv_KPH, self.steerdUP )
    DN  = interp( v_ego_kph, self.cv_KPH, self.steerdDN )
    return MAX, UP, DN    


  def steerParams_torque(self, CS, actuators, path_plan ):
    param = copy.copy(self.p)
    v_ego_kph = CS.out.vEgo * CV.MS_TO_KPH
    dst_steer = actuators.steer * param.STEER_MAX


    self.enable_time = self.timer1.sampleTime()
    if self.enable_time < 50:
      self.steer_torque_over_timer = 0
      self.steer_torque_ratio = 1
      return param, dst_steer

    
    nMAX, nUP, nDN = self.atom_tune( v_ego_kph, CS.out.steeringAngleDeg )
    param.STEER_MAX = min( param.STEER_MAX, nMAX)
    param.STEER_DELTA_UP = min( param.STEER_DELTA_UP, nUP)
    param.STEER_DELTA_DOWN = min( param.STEER_DELTA_DOWN, nDN )


             
    if abs(CS.out.steeringAngleDeg) >= CS.CP.maxSteeringAngleDeg: # and CS.out.steeringPressed:
      sec_mval = 5  # 오파 => 운전자.  (sec)
      sec_pval = 20   #  운전자 => 오파  (sec)
      self.timer1.startTime( 10000 )
      self.steer_torque_over_timer = 50
    elif self.timer1.endTime():
      sec_mval = 5    # 오파 => 운전자.  (sec)
      sec_pval = 3    #  운전자 => 오파  (sec)  
    else:
      sec_mval = 0.5  # 오파 => 운전자.  (sec)
      sec_pval = 10   #  운전자 => 오파 

    
    if path_plan.laneChangeState != LaneChangeState.off:
      self.steer_torque_over_timer = 0
      if path_plan.laneChangeState in [LaneChangeState.laneChangeStarting,  LaneChangeState.preLaneChange]:
        param.STEER_MAX = min( param.STEER_MAX, 255)
    elif CS.out.leftBlinker or CS.out.rightBlinker:
      sec_mval = 0.5  # 오파 => 운전자.
      sec_pval = 10 # 운전자 => 오파  (sec)

    if CS.out.cruiseState.cruiseSwState == Buttons.CANCEL:
      self.steer_torque_over_timer = 0
    elif not CS.out.cruiseState.enabled:
      self.steer_torque_over_timer = 0
    elif v_ego_kph > 5 and abs(CS.out.steeringTorque) > 300:  #사용자 핸들 토크
      sec_mval = 0.5  # 오파 => 운전자.  (sec)
      sec_pval = 10   #  운전자 => 오파       
      self.steer_torque_over_timer = 50
    elif self.steer_torque_over_timer:
      self.steer_torque_over_timer -= 1

    ratio_pval = 1/(100*sec_pval)
    ratio_mval = 1/(100*sec_mval)

    if self.steer_torque_over_timer:
      self.steer_torque_ratio -= ratio_mval
    else:
      self.steer_torque_ratio += ratio_pval

    if self.steer_torque_ratio < 0:
      self.steer_torque_ratio = 0
    elif self.steer_torque_ratio > 1:
      self.steer_torque_ratio = 1

    return  param, dst_steer


  def update_navi(self, sm, CS ):
    v_ego_kph = CS.out.vEgo * CV.MS_TO_KPH    
    cruise_set_speed_kph = self.cruise_set_speed_kph
    self.liveMapData = sm['liveMapData']    
    speedLimit = self.liveMapData.speedLimit
    speedLimitDistance = self.liveMapData.speedLimitDistance
    safetySign  = self.liveMapData.safetySign
    mapValid = self.liveMapData.mapValid
    trafficType = self.liveMapData.trafficType


   
    if not mapValid:
      return  cruise_set_speed_kph
    elif trafficType:
      mapValid = True
    else:
      mapValid = False
      return  cruise_set_speed_kph

    if CS.is_highway:
      spdTarget = interp( speedLimitDistance, [300,1000], [ speedLimit, v_ego_kph ] )
    else:
      spdTarget = interp( speedLimitDistance, [300,600], [ speedLimit, v_ego_kph ] )
    #deltaSpd = speedLimit - v_ego_kph
    cruise_set_speed_kph = min( spdTarget, cruise_set_speed_kph )

    return  cruise_set_speed_kph


  def update_longctrl(self, c, CS, frame, sm, CP ):  
    # send scc to car if longcontrol enabled and SCC not on bus 0 or ont live
    # atom

    btn_signal = None
    vCruise = sm['longitudinalPlan'].vCruise
    kph_set_vEgo = vCruise * CV.MS_TO_KPH

    self.cruise_set_speed_kph = CS.out.cruiseState.speed * CV.MS_TO_KPH


    if self.longCtrl.update_btn( CS  ) == 0:
      pass
    elif CS.out.cruiseState.modeSel == 1:
      kph_set_vEgo = self.update_navi( sm, CS )
      self.ctrl_speed = min( self.cruise_set_speed_kph, kph_set_vEgo)
      btn_signal = self.longCtrl.update_scc( CS, self.ctrl_speed )      
    elif CS.acc_active and (CS.out.cruiseState.modeSel == 4 or CS.out.cruiseState.modeSel == 2):
      self.ctrl_speed = min( self.cruise_set_speed_kph, kph_set_vEgo)      
      btn_signal = self.longCtrl.update_scc( CS, self.ctrl_speed )     

    return btn_signal

  def update(self, c, CS, frame, sm, CP ):
    if self.CP != CP:
      self.CP = CP

    enabled = c.enabled
    actuators = c.actuators
    pcm_cancel_cmd = c.cruiseControl.cancel

    # Steering Torque
    path_plan = sm['lateralPlan']    
    param, dst_steer = self.steerParams_torque( CS, actuators, path_plan )
    new_steer = actuators.steer * param.STEER_MAX
    apply_steer = apply_std_steer_torque_limits(new_steer, self.apply_steer_last, CS.out.steeringTorque, param)
    if new_steer != apply_steer:
      self.steer_rate_limited = True
    else:
      self.steer_rate_limited = False

    apply_steer_limit = param.STEER_MAX
    if self.steer_torque_ratio < 1:
      apply_steer_limit = int(self.steer_torque_ratio * param.STEER_MAX)
      apply_steer = self.limit_ctrl( apply_steer, apply_steer_limit, 0 )


    # disable if steer angle reach 90 deg, otherwise mdps fault in some models
    lkas_active = enabled   #and abs(CS.out.steeringAngleDeg) < CP.maxSteeringAngleDeg

    if not lkas_active:
      apply_steer = 0

    steer_req = 1 if apply_steer else 0
    self.apply_steer_last = apply_steer

    sys_warning, self.hud_sys_state = self.process_hud_alert( lkas_active, c )

    if frame == 0: # initialize counts from last received count signals
      self.lkas11_cnt = CS.lkas11["CF_Lkas_MsgCount"] + 1
      self.longCtrl.reset(CS)

    self.lkas11_cnt %= 0x10

    apply_steer = int(round(float(apply_steer)))
    can_sends = []
    can_sends.append( create_lkas11(self.packer, self.lkas11_cnt, self.car_fingerprint, apply_steer, steer_req,
                                   CS.lkas11, sys_warning, self.hud_sys_state, c ) )

    if steer_req:
      can_sends.append( create_mdps12(self.packer, frame, CS.mdps12) )


    model_speed = self.longCtrl.cal_curve_speed( sm, CS.out.vEgo )
    
    str_log1 = 'CV={:3.0f} torg:{:5.0f} steer={:5.0f} '.format( model_speed, apply_steer, CS.out.steeringTorque  )
    trace1.printf( '  {}'.format( str_log1 ) )



    str_log1 = 'gas={:.3f} gap={:.0f} Value={:.3f} raw={:.3f} '.format( CS.out.gas, CS.cruiseGapSet, CS.aReqValue, CS.aReqRaw )
    trace1.printf2( '{}'.format( str_log1 ) )

    str_log1 = 'LKAS={:.0f} hold={:.0f}'.format( CS.lkas_button_on, CS.auto_hold )
    str_log2 = 'limit={:.0f} tm={:.1f} '.format( apply_steer_limit, self.timer1.sampleTime()  )               
    #trace1.printf3( '{} {}'.format( str_log1, str_log2 ) )

    if pcm_cancel_cmd:
      can_sends.append(create_clu11(self.packer, frame, CS.clu11, Buttons.CANCEL))
    elif CS.out.cruiseState.standstill:
      # run only first time when the car stopped
      if self.last_lead_distance == 0:  
        # get the lead distance from the Radar
        self.last_lead_distance = CS.lead_distance
        self.resume_cnt = 0
      # when lead car starts moving, create 6 RES msgs
      elif CS.lead_distance != self.last_lead_distance and (frame - self.last_resume_frame) > 5:
        can_sends.append(create_clu11(self.packer, self.resume_cnt, CS.clu11, Buttons.RES_ACCEL))
        self.resume_cnt += 1
        # interval after 6 msgs
        if self.resume_cnt > 5:
          self.last_resume_frame = frame
          self.resume_cnt = 0
    # reset lead distnce after the car starts moving
    elif self.last_lead_distance != 0:
        self.last_lead_distance = 0
    elif CP.openpilotLongitudinalControl:
      if not CS.out.cruiseState.accActive:
        trace1.printf3( '{} {}'.format( str_log1, str_log2 ) )
      else:      
        btn_signal = self.update_longctrl( c, CS, frame, sm, CP )
        if btn_signal != None:
          can_sends.append(create_clu11(self.packer, self.resume_cnt, CS.clu11, btn_signal ))
          self.resume_cnt += 1
        else:
          self.resume_cnt = 0

        str_log1 = 'NAVI set_speed={:.0f} '.format( self.cruise_set_speed_kph )
        # dec 제어.
        if CS.out.cruiseState.modeSel == 2:
          data = self.longCtrl.update( self.packer, CS, c, frame )
          if data != None:
            can_sends.append( data )
          else:
            trace1.printf3( '{} {}'.format( str_log1, str_log2 ) )
        else:
          trace1.printf3( '{}'.format( str_log1) )




    # 20 Hz LFA MFA message
    if frame % 5 == 0 and self.car_fingerprint in FEATURES["use_lfa_mfa"]:
      can_sends.append(create_lfahda_mfc(self.packer, enabled))

    # counter inc
    self.lkas11_cnt += 1
    self.update_navibtn( sm, CS )
    return can_sends


  def update_navibtn(self, sm, CS):
    if CS.cruise_buttons != Buttons.CANCEL:     
      return

    self.liveMapData = sm['liveMapData']
    if self.liveMapData.mapEnable != 2:
       self.params.put("OpkrMapEnable", "2")

    return