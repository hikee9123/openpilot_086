#!/usr/bin/env python3
from cereal import car
from selfdrive.config import Conversions as CV
from selfdrive.car.hyundai.values import CAR, EV_CAR, HYBRID_CAR, Buttons
from selfdrive.car import STD_CARGO_KG, scale_rot_inertia, scale_tire_stiffness, gen_empty_fingerprint
from selfdrive.car.interfaces import CarInterfaceBase, MAX_CTRL_SPEED


from selfdrive.conf_atom import ConfAtom
from common.params import Params

ATOMC = ConfAtom()
params = Params()

EventName = car.CarEvent.EventName
ButtonType = car.CarState.ButtonEvent.Type


class CarInterface(CarInterfaceBase):
  def __init__(self, CP, CarController, CarState):
    super().__init__(CP, CarController, CarState )
    self.meg_timer = 0
    self.meg_name = 0
    self.pre_button = 0
    self.pcm_enable_cmd = False

  @staticmethod
  def compute_gb(accel, speed):
    return float(accel) / 3.0

  @staticmethod
  def get_params(candidate, fingerprint=gen_empty_fingerprint(), car_fw=[]):  # pylint: disable=dangerous-default-value
    global ATOMC
    ret = CarInterfaceBase.get_std_params(candidate, fingerprint)

    ret.carName = "hyundai"
    ret.safetyModel = car.CarParams.SafetyModel.hyundai
    ret.radarOffCan = False

    # Most Hyundai car ports are community features for now
    ret.communityFeature = candidate not in [CAR.SONATA, CAR.PALISADE]

    ret.steerActuatorDelay = 0.1  # Default delay
    ret.steerRateCost = 0.6
    ret.steerLimitTimer = 0.4
    tire_stiffness_factor = 1.

    ret.maxSteeringAngleDeg = 90.
    ret.startAccel = 1.0

    eps_modified = False
    for fw in car_fw:
      if fw.ecu == "eps" and b"," in fw.fwVersion:
        eps_modified = True

    if candidate == CAR.GRANDEUR_HEV_19:
      ret.mass = 1675. + STD_CARGO_KG
      ret.wheelbase = 2.845
      ret.steerRatio = 16.5  #13.96   #12.5
      ret.steerMaxBP = [1.,8.]
      ret.steerMaxV = [0.5,1.0]
      ret.steerRateCost = 0.6

      ret.lateralTuning.pid.kf = 0.000005
      ret.lateralTuning.pid.kpBP, ret.lateralTuning.pid.kpV = [[0.], [0.20]]
      ret.lateralTuning.pid.kiBP, ret.lateralTuning.pid.kiV = [[0.], [0.02]]


      
      ret.lateralTuning.init('lqr')
      ret.lateralTuning.lqr.scale = 2000.0
      ret.lateralTuning.lqr.ki = 0.01
      ret.lateralTuning.lqr.dcGain = 0.00295   # 0.002237852961363602

      ret.lateralTuning.lqr.a = [0., 1., -0.22619643, 1.21822268]
      ret.lateralTuning.lqr.b = [-1.92006585e-04, 3.95603032e-05]
      ret.lateralTuning.lqr.c = [1., 0.]
      ret.lateralTuning.lqr.k = [-110., 451.]
      ret.lateralTuning.lqr.l = [0.33, 0.318]      
    elif candidate == CAR.GRANDEUR_HEV_20:
      ret.mass = 1675. + STD_CARGO_KG
      ret.wheelbase = 2.885
      ret.steerRatio = 12.5
      ret.steerMaxBP = [1.,8.]
      ret.steerMaxV = [0.5,1.0]
      ret.steerRateCost = 0.5

      ret.lateralTuning.pid.kf = 0.000005
      ret.lateralTuning.pid.kpBP, ret.lateralTuning.pid.kpV = [[0.], [0.20]]
      ret.lateralTuning.pid.kiBP, ret.lateralTuning.pid.kiV = [[0.], [0.02]]


      
      ret.lateralTuning.init('lqr')
      ret.lateralTuning.lqr.scale = 1500.0
      ret.lateralTuning.lqr.ki = 0.02
      ret.lateralTuning.lqr.dcGain = 0.0029

      ret.lateralTuning.lqr.a = [0., 1., -0.22619643, 1.21822268]
      ret.lateralTuning.lqr.b = [-1.92006585e-04, 3.95603032e-05]
      ret.lateralTuning.lqr.c = [1., 0.]
      ret.lateralTuning.lqr.k = [-110., 451.]
      ret.lateralTuning.lqr.l = [0.33, 0.318]           
    elif candidate == CAR.SANTA_FE:
      ret.lateralTuning.pid.kf = 0.00005
      ret.mass = 3982. * CV.LB_TO_KG + STD_CARGO_KG
      ret.wheelbase = 2.766
      # Values from optimizer
      ret.steerRatio = 16.55  # 13.8 is spec end-to-end
      tire_stiffness_factor = 0.82
      ret.lateralTuning.pid.kiBP, ret.lateralTuning.pid.kpBP = [[9., 22.], [9., 22.]]
      ret.lateralTuning.pid.kpV, ret.lateralTuning.pid.kiV = [[0.2, 0.35], [0.05, 0.09]]
    elif candidate == CAR.SONATA:
      ret.lateralTuning.pid.kf = 0.00005
      ret.mass = 1513. + STD_CARGO_KG
      ret.wheelbase = 2.84
      ret.steerRatio = 13.27 * 1.15   # 15% higher at the center seems reasonable
      tire_stiffness_factor = 0.65
      ret.lateralTuning.pid.kiBP, ret.lateralTuning.pid.kpBP = [[0.], [0.]]
      ret.lateralTuning.pid.kpV, ret.lateralTuning.pid.kiV = [[0.25], [0.05]]
    elif candidate == CAR.SONATA_LF:
      ret.lateralTuning.pid.kf = 0.00005
      ret.mass = 4497. * CV.LB_TO_KG
      ret.wheelbase = 2.804
      ret.steerRatio = 13.27 * 1.15   # 15% higher at the center seems reasonable
      ret.lateralTuning.pid.kiBP, ret.lateralTuning.pid.kpBP = [[0.], [0.]]
      ret.lateralTuning.pid.kpV, ret.lateralTuning.pid.kiV = [[0.25], [0.05]]
    elif candidate == CAR.PALISADE:
      ret.lateralTuning.pid.kf = 0.00005
      ret.mass = 1999. + STD_CARGO_KG
      ret.wheelbase = 2.90
      ret.steerRatio = 13.75 * 1.15
      ret.lateralTuning.pid.kiBP, ret.lateralTuning.pid.kpBP = [[0.], [0.]]
      ret.lateralTuning.pid.kpV, ret.lateralTuning.pid.kiV = [[0.3], [0.05]]
    elif candidate in [CAR.ELANTRA, CAR.ELANTRA_GT_I30]:
      ret.lateralTuning.pid.kf = 0.00006
      ret.mass = 1275. + STD_CARGO_KG
      ret.wheelbase = 2.7
      ret.steerRatio = 15.4            # 14 is Stock | Settled Params Learner values are steerRatio: 15.401566348670535
      tire_stiffness_factor = 0.385    # stiffnessFactor settled on 1.0081302973865127
      ret.lateralTuning.pid.kiBP, ret.lateralTuning.pid.kpBP = [[0.], [0.]]
      ret.lateralTuning.pid.kpV, ret.lateralTuning.pid.kiV = [[0.25], [0.05]]
      ret.minSteerSpeed = 32 * CV.MPH_TO_MS
    elif candidate == CAR.ELANTRA_2021:
      ret.lateralTuning.pid.kf = 0.00005
      ret.mass = (2800. * CV.LB_TO_KG) + STD_CARGO_KG
      ret.wheelbase = 2.72
      ret.steerRatio = 13.27 * 1.15   # 15% higher at the center seems reasonable
      tire_stiffness_factor = 0.65
      ret.lateralTuning.pid.kiBP, ret.lateralTuning.pid.kpBP = [[0.], [0.]]
      ret.lateralTuning.pid.kpV, ret.lateralTuning.pid.kiV = [[0.25], [0.05]]
    elif candidate == CAR.HYUNDAI_GENESIS:
      ret.lateralTuning.pid.kf = 0.00005
      ret.mass = 2060. + STD_CARGO_KG
      ret.wheelbase = 3.01
      ret.steerRatio = 16.5
      ret.lateralTuning.init('indi')
      ret.lateralTuning.indi.innerLoopGainBP = [0.]
      ret.lateralTuning.indi.innerLoopGainV = [3.5]
      ret.lateralTuning.indi.outerLoopGainBP = [0.]
      ret.lateralTuning.indi.outerLoopGainV = [2.0]
      ret.lateralTuning.indi.timeConstantBP = [0.]
      ret.lateralTuning.indi.timeConstantV = [1.4]
      ret.lateralTuning.indi.actuatorEffectivenessBP = [0.]
      ret.lateralTuning.indi.actuatorEffectivenessV = [2.3]
      ret.minSteerSpeed = 60 * CV.KPH_TO_MS
    elif candidate == CAR.KONA:
      ret.lateralTuning.pid.kf = 0.00005
      ret.mass = 1275. + STD_CARGO_KG
      ret.wheelbase = 2.7
      ret.steerRatio = 13.73 * 1.15  # Spec
      tire_stiffness_factor = 0.385
      ret.lateralTuning.pid.kiBP, ret.lateralTuning.pid.kpBP = [[0.], [0.]]
      ret.lateralTuning.pid.kpV, ret.lateralTuning.pid.kiV = [[0.25], [0.05]]
    elif candidate == CAR.KONA_EV:
      ret.lateralTuning.pid.kf = 0.00006
      ret.mass = 1685. + STD_CARGO_KG
      ret.wheelbase = 2.7
      ret.steerRatio = 13.73  # Spec
      tire_stiffness_factor = 0.385
      ret.lateralTuning.pid.kiBP, ret.lateralTuning.pid.kpBP = [[0.], [0.]]
      ret.lateralTuning.pid.kpV, ret.lateralTuning.pid.kiV = [[0.25], [0.05]]
    elif candidate in [CAR.IONIQ, CAR.IONIQ_EV_LTD, CAR.IONIQ_EV_2020, CAR.IONIQ_PHEV]:
      ret.lateralTuning.pid.kf = 0.00006
      ret.mass = 1490. + STD_CARGO_KG  # weight per hyundai site https://www.hyundaiusa.com/ioniq-electric/specifications.aspx
      ret.wheelbase = 2.7
      ret.steerRatio = 13.73  # Spec
      tire_stiffness_factor = 0.385
      ret.lateralTuning.pid.kiBP, ret.lateralTuning.pid.kpBP = [[0.], [0.]]
      ret.lateralTuning.pid.kpV, ret.lateralTuning.pid.kiV = [[0.25], [0.05]]
      if candidate not in [CAR.IONIQ_EV_2020, CAR.IONIQ_PHEV]:
        ret.minSteerSpeed = 32 * CV.MPH_TO_MS
    elif candidate == CAR.VELOSTER:
      ret.lateralTuning.pid.kf = 0.00005
      ret.mass = 3558. * CV.LB_TO_KG
      ret.wheelbase = 2.80
      ret.steerRatio = 13.75 * 1.15
      tire_stiffness_factor = 0.5
      ret.lateralTuning.pid.kiBP, ret.lateralTuning.pid.kpBP = [[0.], [0.]]
      ret.lateralTuning.pid.kpV, ret.lateralTuning.pid.kiV = [[0.25], [0.05]]

    # Kia
    elif candidate == CAR.KIA_SORENTO:
      ret.lateralTuning.pid.kf = 0.00005
      ret.mass = 1985. + STD_CARGO_KG
      ret.wheelbase = 2.78
      ret.steerRatio = 14.4 * 1.1   # 10% higher at the center seems reasonable
      ret.lateralTuning.pid.kiBP, ret.lateralTuning.pid.kpBP = [[0.], [0.]]
      ret.lateralTuning.pid.kpV, ret.lateralTuning.pid.kiV = [[0.25], [0.05]]
    elif candidate == CAR.KIA_NIRO_EV:
      ret.lateralTuning.pid.kf = 0.00006
      ret.mass = 1737. + STD_CARGO_KG
      ret.wheelbase = 2.7
      ret.steerRatio = 13.73  # Spec
      tire_stiffness_factor = 0.385
      ret.lateralTuning.pid.kiBP, ret.lateralTuning.pid.kpBP = [[0.], [0.]]
      ret.lateralTuning.pid.kpV, ret.lateralTuning.pid.kiV = [[0.25], [0.05]]
    elif candidate == CAR.KIA_SELTOS:
      ret.mass = 1337. + STD_CARGO_KG
      ret.wheelbase = 2.63
      ret.steerRatio = 14.56
      tire_stiffness_factor = 1
      ret.lateralTuning.init('indi')
      ret.lateralTuning.indi.innerLoopGainBP = [0.]
      ret.lateralTuning.indi.innerLoopGainV = [4.]
      ret.lateralTuning.indi.outerLoopGainBP = [0.]
      ret.lateralTuning.indi.outerLoopGainV = [3.]
      ret.lateralTuning.indi.timeConstantBP = [0.]
      ret.lateralTuning.indi.timeConstantV = [1.4]
      ret.lateralTuning.indi.actuatorEffectivenessBP = [0.]
      ret.lateralTuning.indi.actuatorEffectivenessV = [1.8]
    elif candidate in [CAR.KIA_OPTIMA, CAR.KIA_OPTIMA_H]:
      ret.lateralTuning.pid.kf = 0.00005
      ret.mass = 3558. * CV.LB_TO_KG
      ret.wheelbase = 2.80
      ret.steerRatio = 13.75
      tire_stiffness_factor = 0.5
      ret.lateralTuning.pid.kiBP, ret.lateralTuning.pid.kpBP = [[0.], [0.]]
      ret.lateralTuning.pid.kpV, ret.lateralTuning.pid.kiV = [[0.25], [0.05]]
    elif candidate == CAR.KIA_STINGER:
      ret.lateralTuning.pid.kf = 0.00005
      ret.mass = 1825. + STD_CARGO_KG
      ret.wheelbase = 2.78
      ret.steerRatio = 14.4 * 1.15   # 15% higher at the center seems reasonable
      ret.lateralTuning.pid.kiBP, ret.lateralTuning.pid.kpBP = [[0.], [0.]]
      ret.lateralTuning.pid.kpV, ret.lateralTuning.pid.kiV = [[0.25], [0.05]]
    elif candidate == CAR.KIA_FORTE:
      ret.lateralTuning.pid.kf = 0.00005
      ret.mass = 3558. * CV.LB_TO_KG
      ret.wheelbase = 2.80
      ret.steerRatio = 13.75
      tire_stiffness_factor = 0.5
      ret.lateralTuning.pid.kiBP, ret.lateralTuning.pid.kpBP = [[0.], [0.]]
      ret.lateralTuning.pid.kpV, ret.lateralTuning.pid.kiV = [[0.25], [0.05]]
    elif candidate == CAR.KIA_CEED:
      ret.lateralTuning.pid.kf = 0.00005
      ret.mass = 1450. + STD_CARGO_KG
      ret.wheelbase = 2.65
      ret.steerRatio = 13.75
      tire_stiffness_factor = 0.5
      ret.lateralTuning.pid.kf = 0.00005
      ret.lateralTuning.pid.kiBP, ret.lateralTuning.pid.kpBP = [[0.], [0.]]
      ret.lateralTuning.pid.kpV, ret.lateralTuning.pid.kiV = [[0.25], [0.05]]

    # Genesis
    elif candidate == CAR.GENESIS_G70:
      ret.lateralTuning.init('indi')
      ret.lateralTuning.indi.innerLoopGainBP = [0.]
      ret.lateralTuning.indi.innerLoopGainV = [2.5]
      ret.lateralTuning.indi.outerLoopGainBP = [0.]
      ret.lateralTuning.indi.outerLoopGainV = [3.5]
      ret.lateralTuning.indi.timeConstantBP = [0.]
      ret.lateralTuning.indi.timeConstantV = [1.4]
      ret.lateralTuning.indi.actuatorEffectivenessBP = [0.]
      ret.lateralTuning.indi.actuatorEffectivenessV = [1.8]
      ret.steerActuatorDelay = 0.1
      ret.mass = 1640.0 + STD_CARGO_KG
      ret.wheelbase = 2.84
      ret.steerRatio = 13.56
    elif candidate == CAR.GENESIS_G80:
      ret.lateralTuning.pid.kf = 0.00005
      ret.mass = 2060. + STD_CARGO_KG
      ret.wheelbase = 3.01
      ret.steerRatio = 16.5
      ret.lateralTuning.pid.kiBP, ret.lateralTuning.pid.kpBP = [[0.], [0.]]
      ret.lateralTuning.pid.kpV, ret.lateralTuning.pid.kiV = [[0.16], [0.01]]
    elif candidate == CAR.GENESIS_G90:
      ret.mass = 2200
      ret.wheelbase = 3.15
      ret.steerRatio = 12.069
      ret.lateralTuning.pid.kiBP, ret.lateralTuning.pid.kpBP = [[0.], [0.]]
      ret.lateralTuning.pid.kpV, ret.lateralTuning.pid.kiV = [[0.16], [0.01]]



    # atom  START
    ret.atomTuning.cvKPH    = ATOMC.cv_KPH
    ret.atomTuning.cvBPV    = ATOMC.cv_BPV
    ret.atomTuning.cvsMaxV  = ATOMC.cv_sMaxV
    ret.atomTuning.cvsdUpV  = ATOMC.cv_sdUPV
    ret.atomTuning.cvsdDnV  = ATOMC.cv_sdDNV

    ret.atomTuning.cvsteerRatioV = ATOMC.cv_steerRatioV
    ret.atomTuning.cvsteerActuatorDelayV = ATOMC.cv_ActuatorDelayV
    ret.atomTuning.cvSteerRateCostV = ATOMC.cv_SteerRateCostV
    # atom  END


    # set safety_hyundai_community only for non-SCC, MDPS harrness or SCC harrness cars or cars that have unknown issue
    if ret.radarOffCan  or Params().get('CommunityFeaturesToggle') == b'1':
      ret.safetyModel = car.CarParams.SafetyModel.hyundaiCommunity
    # these cars require a special panda safety mode due to missing counters and checksums in the messages
    elif candidate in [CAR.HYUNDAI_GENESIS, CAR.IONIQ_EV_2020, CAR.IONIQ_EV_LTD, CAR.IONIQ, CAR.KONA_EV, CAR.KIA_SORENTO,
                     CAR.SONATA_LF, CAR.KIA_NIRO_EV, CAR.KIA_OPTIMA, CAR.VELOSTER, CAR.KIA_STINGER, CAR.KIA_SELTOS,
                     CAR.GENESIS_G70, CAR.GENESIS_G80, CAR.KIA_CEED, CAR.GRANDEUR_HEV_19]:
      ret.safetyModel = car.CarParams.SafetyModel.hyundaiLegacy

    # set appropriate safety param for gas signal
    if candidate in HYBRID_CAR:
      ret.safetyParam = 2
    elif candidate in EV_CAR:
      ret.safetyParam = 1

    ret.centerToFront = ret.wheelbase * 0.4

    # TODO: get actual value, for now starting with reasonable value for
    # civic and scaling by mass and wheelbase
    ret.rotationalInertia = scale_rot_inertia(ret.mass, ret.wheelbase)

    # TODO: start from empirically derived lateral slip stiffness for the civic and scale by
    # mass and CG position, so all cars will have approximately similar dyn behaviors
    ret.tireStiffnessFront, ret.tireStiffnessRear = scale_tire_stiffness(ret.mass, ret.wheelbase, ret.centerToFront,
                                                                         tire_stiffness_factor=tire_stiffness_factor)

    ret.enableCamera = True
    ret.enableBsm = 0x58b in fingerprint[0]

    
    
    # stock ACC by neokil
    ret.openpilotLongitudinalControl = Params().get('LongitudinalControl') == b'1'  #False
    ret.enableCruise = True
    ret.minEnableSpeed = -1.  # enable is done by stock ACC, so ignore this
    ret.steerRatioRear = 0.  # no rear steering, at least on the listed cars aboveA
    ret.gasMaxBP = [0., 10.*CV.KPH_TO_MS, 20.*CV.KPH_TO_MS, 70.*CV.KPH_TO_MS, 130.*CV.KPH_TO_MS ]
    ret.gasMaxV = [0.48, 0.32, 0.23, 0.14, 0.097]  # half max brake
    ret.brakeMaxBP = [0.]
    ret.brakeMaxV = [1.]
    ret.startAccel = 1.7
    ret.minSpeedCan = 0.3
    ret.stoppingBrakeRate = 0.2 # brake_travel/s while trying to stop
    ret.startingBrakeRate = 0.8 # brake_travel/s while releasing on restart
    ret.stoppingControl = False
    ret.longitudinalTuning.deadzoneBP = [0., 100.*CV.KPH_TO_MS]
    ret.longitudinalTuning.deadzoneV = [0., 0.015]
    ret.longitudinalTuning.kpBP = [0., 10. * CV.KPH_TO_MS, 20. * CV.KPH_TO_MS, 40. * CV.KPH_TO_MS, 100. * CV.KPH_TO_MS]
    ret.longitudinalTuning.kpV = [0.97, 0.82, 0.63, 0.58, 0.43]
    ret.longitudinalTuning.kiBP = [0.]
    ret.longitudinalTuning.kiV = [0.015]
    return ret


  @staticmethod
  def live_tune(CP, read=False):
    global ATOMC 

    if read:
      ATOMC.read_tune()

    # param
    CP.atomTuning.cvKPH    = ATOMC.cv_KPH
    CP.atomTuning.cvBPV    = ATOMC.cv_BPV
    CP.atomTuning.cvsMaxV  = ATOMC.cv_sMaxV
    CP.atomTuning.cvsdUpV  = ATOMC.cv_sdUPV
    CP.atomTuning.cvsdDnV  = ATOMC.cv_sdDNV

    CP.atomTuning.cvsteerRatioV = ATOMC.cv_steerRatioV
    CP.atomTuning.cvsteerActuatorDelayV = ATOMC.cv_ActuatorDelayV
    CP.atomTuning.cvSteerRateCostV = ATOMC.cv_SteerRateCostV
    return CP




  def update(self, c, can_strings):
    self.cp.update_strings(can_strings)
    self.cp_cam.update_strings(can_strings)

    ret = self.CS.update(self.cp, self.cp_cam)
    ret.canValid = self.cp.can_valid and self.cp_cam.can_valid
    ret.steeringRateLimited = self.CC.steer_rate_limited if self.CC is not None else False

    events = self.create_common_events(ret)
    # TODO: addd abs(self.CS.angle_steers) > 90 to 'steerTempUnavailable' event

    if not self.cruise_enabled_prev:
      self.meg_timer = 0
      self.meg_name =  None
    else:
      meg_timer = 100
      if self.meg_timer:
        self.meg_timer -= 1
        meg_timer = 0
      #elif not self.CS.lkas_button_on:
      #  self.meg_name = EventName.invalidLkasSetting
      elif ret.cruiseState.standstill:
        self.meg_name = EventName.resumeRequired
      elif self.CC.steer_torque_over_timer and self.CC.steer_torque_ratio < 0.1:
        self.meg_name = EventName.steerTorqueOver
      elif self.CC.steer_torque_ratio < 0.5 and self.CS.clu_Vanz > 5:
        self.meg_name = EventName.steerTorqueLow
      elif ret.vEgo > MAX_CTRL_SPEED:
        self.meg_name = EventName.speedTooHigh
      elif ret.steerError:
        self.meg_name = EventName.steerUnavailable
      elif ret.steerWarning:
        self.meg_name = EventName.steerTempUnavailable
      elif self.CS.Lkas_LdwsSysState == 15:
        self.meg_name = EventName.invalidLkasSetting        
      else:
        meg_timer = 0
        self.meg_name =  None

      if meg_timer != 0:
        self.meg_timer = 100

      if self.meg_timer and self.meg_name != None:
        events.add( self.meg_name )


    # low speed steer alert hysteresis logic (only for cars with steer cut off above 10 m/s)
    if ret.vEgo < (self.CP.minSteerSpeed + 2.) and self.CP.minSteerSpeed > 10.:
      self.low_speed_alert = True
    if ret.vEgo > (self.CP.minSteerSpeed + 4.):
      self.low_speed_alert = False
    if self.low_speed_alert:
      events.add(car.CarEvent.EventName.belowSteerSpeed)

    ret.events = events.to_msg()

    self.CS.out = ret.as_reader()
    return self.CS.out

  def apply(self, c, sm, CP):
    can_sends = self.CC.update(c, self.CS, self.frame, sm, CP )
    self.frame += 1
    return can_sends
