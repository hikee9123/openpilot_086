#include <sys/time.h>
#include <sys/resource.h>

#include <android/log.h>
#include <log/logger.h>
#include <log/logprint.h>

#include "cereal/messaging/messaging.h"
#include "selfdrive/common/util.h"
#include "selfdrive/common/params.h"


typedef struct LiveMapDataResult {
      bool  speedLimitValid;
      float speedLimit;
      bool  speedAdvisoryValid;
      float speedAdvisory;
      bool  speedLimitAheadValid;
      float speedLimitAhead; // Float32;
      float speedLimitAheadDistance;  // Float32;
      bool  curvatureValid; //  @2 :Bool;
      float curvature;    // @3 :Float32;
      int   wayId;    //   @4 :UInt64;
  //    framed.setRoadX();    // @5 :List(Float32);
 //     framed.setRoadY();    // @6 :List(Float32);
//      framed.setLastGps();    // @7: GpsLocationData;
//      framed.setRoadCurvatureX();    // @8 :List(Float32);
//      framed.setRoadCurvature();    // @9 :List(Float32);
      float distToTurn;    // @10 :Float32;
      bool  mapValid;    //@11 :Bool;
      int   mapEnable;    // @17 :Int32;
} LiveMapDataResult;


int main() {
  setpriority(PRIO_PROCESS, 0, -15);
   
  int     opkr =0;
  int     nTime = 0;
  ExitHandler do_exit;
  PubMaster pm({"liveMapData"});
  LiveMapDataResult  res;

  log_time last_log_time = {};
  logger_list *logger_list = android_logger_list_alloc(ANDROID_LOG_RDONLY | ANDROID_LOG_NONBLOCK, 0, 0);

  while (!do_exit) {
    // setup android logging
    if (!logger_list) {
      logger_list = android_logger_list_alloc_time(ANDROID_LOG_RDONLY | ANDROID_LOG_NONBLOCK, last_log_time, 0);
    }
    assert(logger_list);

    struct logger *main_logger = android_logger_open(logger_list, LOG_ID_MAIN);
    assert(main_logger);
  //  struct logger *radio_logger = android_logger_open(logger_list, LOG_ID_RADIO);
   // assert(radio_logger);
   // struct logger *system_logger = android_logger_open(logger_list, LOG_ID_SYSTEM);
   // assert(system_logger);
   // struct logger *crash_logger = android_logger_open(logger_list, LOG_ID_CRASH);
   // assert(crash_logger);
   // struct logger *kernel_logger = android_logger_open(logger_list, (log_id_t)5); // LOG_ID_KERNEL
   // assert(kernel_logger);

    while (!do_exit) {
      log_msg log_msg;
      int err = android_logger_list_read(logger_list, &log_msg);
      if (err <= 0) break;

      AndroidLogEntry entry;
      err = android_log_processLogBuffer(&log_msg.entry_v1, &entry);
      if (err < 0) continue;
      last_log_time.tv_sec = entry.tv_sec;
      last_log_time.tv_nsec = entry.tv_nsec;

      nTime++;
      if( nTime > 10 )
      {
        nTime = 0;
        res.mapEnable = Params().getBool("OpkrMapEnable");
      }
      
      res.mapValid = 0;

      MessageBuilder msg;
      auto framed = msg.initEvent().initLiveMapData();
      framed.setWayId(log_msg.id());


   //  opkrspdlimit,opkrspd2limit
   //  opkrspddist,opkrspd2dist
   //  opkrsigntype,opkrspdsign
/*
   opkrsigntype  값정리
    1 과속 단속 카메라
    2. 구간단속 시작
    3  구간단속 종료
    4. 구간간속 진향중
    7. 이동식 카메라
    8. Box형 카메라
    13.
    29. 사고다발  x
    30. 급커브     x
    46.
    63  졸움쉼터  x
*/
      //  안심모드
      opkr = 0;
      if( strcmp( entry.tag, "opkrspd2dist" ) == 0 )
      {
        opkr = 1;
        res.speedLimitAheadDistance = atoi( entry.message );
      }
      if( strcmp( entry.tag, "opkrspdsign" ) == 0 )
      {
        opkr = 1;
        res.speedLimitAhead = atoi( entry.message );
      }

      if( strcmp( entry.tag, "opkrspdlimit" ) == 0 )
      {
        opkr = 1;
        res.speedLimit = atoi( entry.message );
      }


      // overlay mode
      if( strcmp( entry.tag, "opkrspd2limit" ) == 0 )
      {
        opkr = 1;
        res.speedLimit = atoi( entry.message );
      }

      if( strcmp( entry.tag, "opkrsigntype") == 0 )
      {
         opkr = 1;
        res.speedLimitAhead = atoi( entry.message );
      }

      if( strcmp( entry.tag, "opkrspddist") == 0 )
      {
        opkr = 1;
        res.speedLimitAheadDistance = atoi( entry.message );
      }      

      res.mapValid = opkr;

      framed.setSpeedLimit( res.speedLimit );  // Float32;
      framed.setSpeedLimitAheadDistance( res.speedLimitAheadDistance );  // raw_target_speed_map_dist Float32;
      framed.setSpeedLimitAhead( res.speedLimitAhead ); // map_sign Float32;

      framed.setMapEnable( res.mapEnable );
      framed.setMapValid( res.mapValid );
      
      
      if( opkr )
      {
        printf("logcat ID(%d) - PID=%d tag=%d.[%s] \n", log_msg.id(), entry.pid,  entry.tid, entry.tag);
        printf("entry.message=[%s]\n", entry.message);
        printf("spd = %f\n", res.speedLimit );
      }

      pm.send("liveMapData", msg);
    }

    android_logger_list_free(logger_list);
    logger_list = NULL;
    util::sleep_for(500);
  }

  if (logger_list) {
    android_logger_list_free(logger_list);
  }

  return 0;
}
