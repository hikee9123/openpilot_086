#include <sys/time.h>
#include <sys/resource.h>

#include <android/log.h>
#include <log/logger.h>
#include <log/logprint.h>

#include "cereal/messaging/messaging.h"
#include "selfdrive/common/util.h"
#include "selfdrive/common/params.h"


typedef struct LiveMapDataResult {
      float speedLimit;  // Float32;
      float speedLimitDistance;  // Float32;
      float safetySign;    // Float32;
      float roadCurvature;    // Float32;
      bool  mapValid;    // bool;
      int   mapEnable;    // bool;
      long  tv_sec;
      long  tv_nsec;
} LiveMapDataResult;


int main() {
  setpriority(PRIO_PROCESS, 0, -15);
   long  nDelta = 0;
   long  nDelta_nsec = 0;
  int     opkr =0;
  int     nTime = 0;
  long    tv_nsec;
  float tv_nsec2;

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


      tv_nsec2 = entry.tv_nsec / 1000000;
      tv_nsec =  entry.tv_sec * 1000ULL + long(tv_nsec2);
      printf("nsec  %.1f  %ld \n", tv_nsec2, tv_nsec);

      nTime++;
      if( nTime > 10 )
      {
        nTime = 0;
        res.mapEnable = Params().getInt("OpkrMapEnable");
      }
      

      
     // code based from atom
     nDelta_nsec = tv_nsec - res.tv_nsec;
     nDelta = entry.tv_sec - res.tv_sec;
      if( strcmp( entry.tag, "Connector" ) == 0 )
      {
         if( opkr == 1 )
           opkr = 5;
      }     
      else if( strcmp( entry.tag, "opkrspdlimit" ) == 0 )
      {
        res.speedLimit = atoi( entry.message );
        opkr = 1;
      }
      else if( strcmp( entry.tag, "opkrspddist" ) == 0 )
      {
        res.speedLimitDistance = atoi( entry.message );
        opkr = 1;
      }
      else if( strcmp( entry.tag, "opkrsigntype" ) == 0 )
      {
        res.safetySign = atoi( entry.message );
        opkr = 1;
      }
      else if( strcmp( entry.tag, "opkrcurvangle" ) == 0 )
      {
        res.roadCurvature = atoi( entry.message );
        opkr = 1;
      }
      else if( strcmp( entry.tag, "audio_hw_primary" ) == 0 )
      {
          opkr = 2;
      }
      else if( strcmp( entry.tag, "msm8974_platform" ) == 0 )
      {
          opkr = 2;
      }      
      else if( opkr == 1 )
      {
         opkr = 5;
      }

      if ( opkr == 1 )
      {
        res.tv_sec = entry.tv_sec;
        res.tv_nsec = tv_nsec;
      }
      else if ( opkr == 2 )
      {
        if( nDelta_nsec > 500 ) opkr = 0;
      }
      else if ( opkr )
      {
        if( nDelta >= opkr ) opkr = 0;
      }

      if ( opkr )
         res.mapValid = 1;
      else
         res.mapValid = 0;

      MessageBuilder msg;
      auto framed = msg.initEvent().initLiveMapData();
      framed.setId(log_msg.id());
      framed.setTs( res.tv_sec );
      framed.setSpeedLimit( res.speedLimit );  // Float32;
      framed.setSpeedLimitDistance( res.speedLimitDistance );  // raw_target_speed_map_dist Float32;
      framed.setSafetySign( res.safetySign ); // map_sign Float32;
      framed.setRoadCurvature( res.roadCurvature ); // road_curvature Float32;

      framed.setMapEnable( res.mapEnable );
      framed.setMapValid( res.mapValid );

     
      
      if( opkr )
      {
        printf("logcat ID(%d) - PID=%d tag=%d.[%s] \n", log_msg.id(), entry.pid,  entry.tid, entry.tag);
        printf("entry.message=[%s]\n", entry.message);
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


/*
MAPPY
   124 : 과속방지턱
   131 : 단속카메라(신호위반카메라)
   200 : 단속구간(고정형 이동식)
   165 : 구간단속
    
*/


// TMAP
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

 // 1. 안심모드
    // - opkrspd2dist, opkrspdsign, opkrspdlimit,

//  2. overlay mode
    // - opkrspd2limit, opkrsigntype, opkrspddist
