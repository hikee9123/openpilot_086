#include <sys/time.h>
#include <sys/resource.h>

#include <android/log.h>
#include <log/logger.h>
#include <log/logprint.h>

#include "cereal/messaging/messaging.h"
#include "selfdrive/common/util.h"



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

  ExitHandler do_exit;
  PubMaster pm({"liveMapData"});

  log_time last_log_time = {};
  logger_list *logger_list = android_logger_list_alloc(ANDROID_LOG_RDONLY | ANDROID_LOG_NONBLOCK, 0, 0);

  while (!do_exit) {
    // setup android logging
    if (!logger_list) {
      logger_list = android_logger_list_alloc_time(ANDROID_LOG_RDONLY | ANDROID_LOG_NONBLOCK, last_log_time, 0);
    }
    assert(logger_list);

    struct logger *main_logger = android_logger_open(logger_list, 1474);
    assert(main_logger);
  //  struct logger *radio_logger = android_logger_open(logger_list, LOG_ID_RADIO);
  //  assert(radio_logger);
  //  struct logger *system_logger = android_logger_open(logger_list, LOG_ID_SYSTEM);
  //  assert(system_logger);
  //  struct logger *crash_logger = android_logger_open(logger_list, LOG_ID_CRASH);
  //  assert(crash_logger);
  //  struct logger *kernel_logger = android_logger_open(logger_list, (log_id_t)5); // LOG_ID_KERNEL
  //  assert(kernel_logger);

    while (!do_exit) {
      log_msg log_msg;
      int err = android_logger_list_read(logger_list, &log_msg);
      if (err <= 0) break;

      AndroidLogEntry entry;
      err = android_log_processLogBuffer(&log_msg.entry_v1, &entry);
      if (err < 0) continue;
      last_log_time.tv_sec = entry.tv_sec;
      last_log_time.tv_nsec = entry.tv_nsec;
      

     //   entry.tag   =>  opkrspd
     //   entry.messageLen;
     //    entry.message; 
     //    entry.message

      if( entry.tag )
          printf( "mapd_tag  id:%d  tag:%s", entry.tid, entry.tag );

    if( entry.message )
          printf( "mapd_message  id:%d  tag:%s", entry.messageLen, entry.message );

      LiveMapDataResult  res;

      res.wayId =  og_msg.id();

      MessageBuilder msg;
      auto framed = msg.initEvent().initLiveMapData();
      framed.setSpeedLimitValid( res.speedLimitValid );    // Bool;
      framed.setSpeedLimit( res.speedLimit );  // Float32;
      framed.setSpeedAdvisoryValid( res.speedAdvisoryValid );  // Bool;
      framed.setSpeedAdvisory( res.speedAdvisory );  // Float32;
      framed.setSpeedLimitAheadValid( res.speedLimitAheadValid );  // Bool;
      framed.setSpeedLimitAhead( res.speedLimitAhead ); // Float32;
      framed.setSpeedLimitAheadDistance( res.speedLimitAheadDistance );  // Float32;
      framed.setCurvatureValid( res.curvatureValid ); //  @2 :Bool;
      framed.setCurvature( res.curvature );    // @3 :Float32;
      framed.setWayId( res.wayId );    //   @4 :UInt64;
      framed.setDistToTurn( res.distToTurn );    // @10 :Float32;
      framed.setMapValid( res.mapValid );    //@11 :Bool;
      framed.setMapEnable( res.mapEnable );    // @17 :Int32;

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
