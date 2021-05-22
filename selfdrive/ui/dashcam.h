#include <time.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "selfdrive/common/params.h"
#include "selfdrive/ui/qt/home.h"

#define CAPTURE_STATE_NONE 0
#define CAPTURE_STATE_CAPTURING 1
#define CAPTURE_STATE_NOT_CAPTURING 2
#define CAPTURE_STATE_PAUSED 3
#define CLICK_TIME 0.2
#define RECORD_INTERVAL 180 // Time in seconds to rotate recordings; Max for screenrecord is 3 minutes
#define RECORD_FILES 40     // Number of files to create before looping over

typedef struct dashcam_element
{
  int pos_x;
  int pos_y;
  int width;
  int height;
} dashcam_element;

dashcam_element lock_button;

extern float  fFontSize;

int captureState = CAPTURE_STATE_NOT_CAPTURING;
int captureNum = 0;
int start_time = 0;
int stop_time = 0;
int elapsed_time = 0; // Time of current recording
int click_elapsed_time = 0;
int click_time = 0;
char filenames[RECORD_FILES][50]; // Track the filenames so they can be deleted when rotating

bool lock_current_video = false; // If true save the current video before rotating
bool locked_files[RECORD_FILES]; // Track which files are locked
int lock_image;                  // Stores reference to the PNG
int files_created = 0;
int  capture_cnt = 0;
int  program_start = 1;


void ui_print(UIState *s, int x, int y,  const char* fmt, ... )
{
  //char speed_str[512];  
  char* msg_buf = NULL;
  va_list args;
  va_start(args, fmt);
  vasprintf( &msg_buf, fmt, args);
  va_end(args);

  nvgText(s->vg, x, y, msg_buf, NULL);
}

static void ui_draw_text1(const UIState *s, float x, float y, const char* string, float size, NVGcolor color, const char *font_name)
{
  nvgFontFace(s->vg, font_name);
  nvgFontSize(s->vg, size);
  nvgFillColor(s->vg, color);
  nvgText(s->vg, x, y, string, NULL);
}

int get_time()
{
  // Get current time (in seconds)

  int iRet;
  struct timeval tv;
  int seconds = 0;

  iRet = gettimeofday(&tv, NULL);
  if (iRet == 0)
  {
    seconds = (int)tv.tv_sec;
  }
  return seconds;
}

struct tm get_time_struct()
{
  time_t t = time(NULL);
  struct tm tm = *localtime(&t);
  return tm;
}

void remove_file(char *videos_dir, char *filename)
{
  if (filename[0] == '\0')
  {
    // Don't do anything if no filename is passed
    return;
  }

  int status;
  char fullpath[64];
  snprintf(fullpath, sizeof(fullpath), "%s/%s", videos_dir, filename);
  status = remove(fullpath);
  if (status == 0)
  {
    printf("Removed file: %s\n", fullpath);
  }
  else
  {
    printf("Unable to remove file: %s\n", fullpath);
    perror("Error message:");
  }
}

void save_file(char *videos_dir, char *filename)
{
  if (!strlen(filename))
  {
    return;
  }

  // Rename file to save it from being overwritten
  char cmd[128];
  snprintf(cmd, sizeof(cmd), "mv %s/%s %s/saved_%s", videos_dir, filename, videos_dir, filename);
  printf("save: %s\n", cmd);
  system(cmd);
}

void stop_capture() {
  char videos_dir[50] = "/storage/emulated/0/videos";

  

  if (captureState == CAPTURE_STATE_CAPTURING)
  {
    printf("stop_capture()\n ");
    system("killall -SIGINT screenrecord");
    captureState = CAPTURE_STATE_NOT_CAPTURING;
    elapsed_time = get_time() - start_time;
    if (elapsed_time < 3)
    {
      remove_file(videos_dir, filenames[captureNum]);
    }
    else
    {
      //printf("Stop capturing screen\n");
      captureNum++;

      if (captureNum > RECORD_FILES - 1)
      {
        captureNum = 0;
      }
    }
  }
}

void start_capture()
{
  captureState = CAPTURE_STATE_CAPTURING;
  char cmd[128] = "";
  char videos_dir[50] = "/storage/emulated/0/videos";

  printf("start_capture()\n ");

  //////////////////////////////////
  // NOTE: make sure videos_dir folder exists on the device!
  //////////////////////////////////
  struct stat st = {0};
  if (stat(videos_dir, &st) == -1)
  {
    mkdir(videos_dir, 0700);
  }

  if (strlen(filenames[captureNum]) && files_created >= RECORD_FILES)
  {
    if (locked_files[captureNum] > 0)
    {
      save_file(videos_dir, filenames[captureNum]);
    }
    else
    {
      // remove the old file
      remove_file(videos_dir, filenames[captureNum]);
    }
    locked_files[captureNum] = 0;
  }

  char filename[64];
  struct tm tm = get_time_struct();
  snprintf(filename, sizeof(filename), "%04d%02d%02d-%02d%02d%02d.mp4", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
  snprintf(cmd, sizeof(cmd), "screenrecord --size 1280x720 --bit-rate 5000000 %s/%s&", videos_dir, filename);
  //snprintf(cmd,sizeof(cmd),"screenrecord --size 960x540 --bit-rate 5000000 %s/%s&",videos_dir,filename);
  strcpy(filenames[captureNum], filename);

  printf("Capturing to file: %s\n", cmd);
  start_time = get_time();
  system(cmd);

  if (lock_current_video)
  {
    // Lock is still on so mark this file for saving
    locked_files[captureNum] = 1;
  }
  else
  {
    locked_files[captureNum] = 0;
  }

  files_created++;
}


bool screen_button_clicked(int touch_x, int touch_y, int x, int y, int cx, int cy )
{
   int   cx_half = cx * 0.5;
   int   cy_half = cy * 0.5;

   int min_x = x - cx_half;
   int min_y = y - cy_half;
   int max_x = x + cx_half;
   int max_y = y + cy_half;

  if (touch_x >= min_x && touch_x <= max_x)
  {
    if (touch_y >= min_y && touch_y <= max_y)
    {
      return true;
    }
  }
  return false;
}

void draw_date_time(UIState *s)
{
  // Get local time to display
  char now[50];
  struct tm tm = get_time_struct();
  snprintf(now, sizeof(now), "%04d/%02d/%02d  %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);


  nvgFontSize(s->vg, 40*fFontSize);
  nvgFontFace(s->vg, "sans-semibold");
 // nvgFillColor(s->vg, nvgRGBA(255, 255, 255, 200));
  nvgText(s->vg, 1602, 23, now, NULL);
}


static void rotate_video()
{
  // Overwrite the existing video (if needed)
  elapsed_time = 0;
  stop_capture();
  captureState = CAPTURE_STATE_CAPTURING;
  start_capture();
}


void screen_toggle_record_state()
{
  //if (captureState == CAPTURE_STATE_CAPTURING)
  if( lock_current_video == true )
  {
    stop_capture();
    lock_current_video = false;
  }
  else
  {
    // start_capture();
    lock_current_video = true;
  }
}


static void screen_draw_button(UIState *s, int touch_x, int touch_y, int touched)
{
  // Set button to bottom left of screen
  nvgTextAlign(s->vg, NVG_ALIGN_LEFT | NVG_ALIGN_BASELINE);


    int btn_w = 150;
    int btn_h = 150;
    int bb_dmr_x = s->viz_rect.x + s->viz_rect.w + 100;
    int btn_x = bb_dmr_x - btn_w;
    int btn_y = 1080 - btn_h;    

  if ( touched && screen_button_clicked(touch_x, touch_y, btn_x, btn_y, btn_w, btn_h) )
  {
    click_elapsed_time = get_time() - click_time;

    printf( "screen_button_clicked %d  captureState = %d \n", click_elapsed_time, captureState );
    if (click_elapsed_time > 0)
    {
      click_time = get_time() + 1;
      screen_toggle_record_state();
    }
  }  


    nvgBeginPath(s->vg);
    nvgRoundedRect(s->vg, btn_x - 110, btn_y - 45, btn_w, btn_h, 100);
    nvgStrokeColor(s->vg, nvgRGBA(255, 255, 255, 80));
    nvgStrokeWidth(s->vg, 6);
    nvgStroke(s->vg);

    nvgFontSize(s->vg, 60*fFontSize);

    if ( lock_current_video == false )
    {
       nvgFillColor(s->vg, nvgRGBA( 50, 50, 100, 200));
    }
    else if (captureState == CAPTURE_STATE_CAPTURING)
    {
      NVGcolor fillColor = nvgRGBA(255, 0, 0, 150);
      nvgFillColor(s->vg, fillColor);
      nvgFill(s->vg);
      nvgFillColor(s->vg, nvgRGBA(255, 255, 255, 200));
    }
    else
    {
      nvgFillColor(s->vg, nvgRGBA(255, 150, 150, 200));
    }
    nvgText(s->vg, btn_x - 75, btn_y + 50, "REC", NULL);


  if (captureState == CAPTURE_STATE_CAPTURING)
  {
    draw_date_time(s);
    elapsed_time = get_time() - start_time;
    if (elapsed_time >= RECORD_INTERVAL)
    {
      capture_cnt++;
      if( capture_cnt > 10 )
      {
        stop_capture();
        lock_current_video = false;
      }
      else
      {
        rotate_video(); 
      }
    }    
  }
}


static void focus_menu_button(UIState *s, int touch_x, int touch_y, int touched)
{
  // Set button to bottom left of screen

  nvgTextAlign(s->vg, NVG_ALIGN_CENTER | NVG_ALIGN_BASELINE);


    if( touched && screen_button_clicked(touch_x, touch_y, 600, 500, 150, 150) )
    {
        int value = Params::param_value.autoFocus++;
        if( value > 100)
        {
          Params::param_value.autoFocus = 100;
        }
        QString values = QString::number(value);
        Params().put("OpkrAutoFocus", values.toStdString());
    }
    else if( touched && screen_button_clicked(touch_x, touch_y, 900, 500, 150, 150) )
    {
        int value = Params::param_value.autoFocus--;
        if( value < 0)
        {
          Params::param_value.autoFocus = 0;
        }
        QString values = QString::number(value);
        Params().put("OpkrAutoFocus", values.toStdString());
    }
    nvgText(s->vg, 600, 500, "[ + ]", NULL);
    nvgText(s->vg, 900, 500, "[ - ]", NULL);
}

static void screen_menu_button(UIState *s, int touch_x, int touch_y, int touched)
{
  // Set button to bottom left of screen
  UIScene &scene = s->scene;

  nvgTextAlign(s->vg, NVG_ALIGN_LEFT | NVG_ALIGN_BASELINE);


    int btn_w = 150;
    int btn_h = 150;
    int bb_dmr_x = s->viz_rect.x + s->viz_rect.w + 100 - 170;
    int btn_x = bb_dmr_x - btn_w;
    int btn_y = 1080 - btn_h;

    if( touched && screen_button_clicked(touch_x, touch_y, btn_x, btn_y, btn_w, btn_h) )
    {
      scene.dash_menu_no++;
      if( scene.dash_menu_no > 2 )
         scene.dash_menu_no = 0;
    }
    

    nvgBeginPath(s->vg);
    nvgRoundedRect(s->vg, btn_x - 110, btn_y - 45, btn_w, btn_h, 100);
    nvgStrokeColor(s->vg, nvgRGBA(255, 255, 255, 80));
    nvgStrokeWidth(s->vg, 6);
    nvgStroke(s->vg);
    nvgFontSize(s->vg, 60*fFontSize);


    NVGcolor fillColor = nvgRGBA(0, 0, 255, 150);
    if( scene.dash_menu_no == 0)
    {
        fillColor = nvgRGBA(0, 0, 255, 50);
    }
    else
    {
        fillColor = nvgRGBA(0, 0, 255, 250);
    }

    nvgFillColor(s->vg, fillColor);
    nvgFill(s->vg);
    nvgFillColor(s->vg, nvgRGBA(255, 255, 255, 200));


    char  szText[50];
    sprintf( szText, "%d", scene.dash_menu_no );
    nvgText(s->vg, btn_x - 50, btn_y + 50, szText, NULL);
}

static void ui_draw_modeSel(UIState *s) 
{
  UIScene &scene = s->scene;
  NVGcolor nColor = COLOR_WHITE;
  char str_msg[512];

  int ui_viz_rx = s->viz_rect.x;
  int ui_viz_rw = s->viz_rect.w; 
  const int viz_speed_x = ui_viz_rx+((ui_viz_rw/2)-(280/2));
  int x_pos = viz_speed_x + 430;
  int y_pos = 120;


  auto  cruiseState = scene.car_state.getCruiseState();
  int modeSel = cruiseState.getModeSel();
  nvgFontSize(s->vg, 80);
  switch( modeSel  )
  {
    case 0: strcpy( str_msg, "0.OPM" ); nColor = COLOR_WHITE; break;
    case 1: strcpy( str_msg, "1.CVS" );    nColor = nvgRGBA(200, 200, 255, 255);  break;
    case 2: strcpy( str_msg, "2.FCAR" );  nColor = nvgRGBA(200, 255, 255, 255);  break;
    case 3: strcpy( str_msg, "3.HYUN" );  nColor = nvgRGBA(200, 255, 255, 255);  break;
    case 4: strcpy( str_msg, "4.CURV" );   nColor = nvgRGBA(200, 255, 255, 255);  break;
    default :  sprintf( str_msg, "%d.NORMAL", modeSel ); nColor = COLOR_WHITE;  break;
  }
  nvgFillColor(s->vg, nColor);  
  ui_print( s, x_pos, y_pos+80, str_msg );
}


static void ui_draw_debug1(UIState *s) 
{
  UIScene &scene = s->scene;
  
  nvgTextAlign(s->vg, NVG_ALIGN_LEFT | NVG_ALIGN_BASELINE);
  //  1035, 1078
  ui_draw_text1(s, 0, 30, scene.alert.alertTextMsg1.c_str(), 45, COLOR_WHITE, "sans-regular");
  ui_draw_text1(s, 0, 1040, scene.alert.alertTextMsg2.c_str(), 45, COLOR_WHITE, "sans-regular");
  ui_draw_text1(s, 0, 1078, scene.alert.alertTextMsg3.c_str(), 45, COLOR_WHITE, "sans-regular");
}


static void ui_draw_debug2(UIState *s) 
{
  UIScene &scene = s->scene;
  
  int  ui_viz_rx = s->viz_rect.x;
  int  y_pos = ui_viz_rx + 300;
  int  x_pos = 100+250; 

  float  steerRatio = scene.liveParameters.getSteerRatio();
  float  steerRatioCV = scene.liveParameters.getSteerRatioCV();
  float  steerActuatorDelayCV =  scene.liveParameters.getSteerActuatorDelayCV();
  float  steerRateCostCV =  scene.liveParameters.getSteerRateCostCV();
  float  fanSpeed = scene.deviceState.getFanSpeedPercentDesired();


  float  angleOffset = scene.liveParameters.getAngleOffsetDeg();
  float  angleOffsetAverage = scene.liveParameters.getAngleOffsetAverageDeg();
  float  stiffnessFactor = scene.liveParameters.getStiffnessFactor();

  float  modelSpeed = scene.controls_state.getModelSpeed();

  float  laneWidth = scene.lateralPlan.getLaneWidth();
  //float  cpuPerc = scene.deviceState.getCpuUsagePercent();

  float  vCruise = scene.longitudinalPlan.getVCruise();
//  float  aCruise = scene.longitudinalPlan.getACruise();
//  float  vTarget = scene.longitudinalPlan.getVTarget();
 // float  aTarget = scene.longitudinalPlan.getATarget();


  auto lane_line_probs = scene.modelDataV2.getLaneLineProbs();

    nvgTextAlign(s->vg, NVG_ALIGN_LEFT | NVG_ALIGN_BASELINE);
    nvgFontSize(s->vg, 36*1.5*fFontSize);

    //ui_print( s, ui_viz_rx+10, 50, "S:%d",  s->awake_timeout );

    x_pos = ui_viz_rx + 250;
    y_pos = 100; 

    ui_print( s, x_pos, y_pos+0,   "sR:%.2f, %.2f", steerRatio,  steerRatioCV );
    ui_print( s, x_pos, y_pos+50,   "SC:%.2f, SD:%.2f", steerRateCostCV,  steerActuatorDelayCV );
    
    ui_print( s, x_pos, y_pos+100,  "aO:%.2f, %.2f", angleOffset, angleOffsetAverage );
    ui_print( s, x_pos, y_pos+150, "sF:%.2f Fan:%.0f", stiffnessFactor, fanSpeed/1000. );
    ui_print( s, x_pos, y_pos+200, "lW:%.2f CV:%.0f", laneWidth, modelSpeed );
   // ui_print( s, x_pos, y_pos+250, "time:%d" , scene.scr.nTime/20 );

    ui_print( s, x_pos, y_pos+300, "prob:%.2f, %.2f, %.2f, %.2f", lane_line_probs[0], lane_line_probs[1], lane_line_probs[2], lane_line_probs[3] );
    ui_print( s, x_pos, y_pos+350, "vCruise:%.1f",  vCruise*3.6 );//,  aCruise, vTarget*3.6,  aTarget);

    
    int  lensPos = scene.camera_state.getLensPos();
    float  lensTruePos = scene.camera_state.getLensTruePos();
    float  lensErr = scene.camera_state.getLensErr();
    ui_print( s, x_pos, y_pos+450, "frame:%d,%3.0f,%3.0f, %3d", lensPos, lensTruePos, lensErr, Params::param_value.autoFocus );
  //float  dPoly = scene.pathPlan.lPoly + scene.pathPlan.rPoly;
  //ui_print( s, x_pos, y_pos+300, "Poly:%.2f, %.2f = %.2f", scene.pathPlan.lPoly, scene.pathPlan.rPoly, dPoly );
  // ui_print( s, x_pos, y_pos+350, "map:%d,cam:%d", scene.live.map_valid, scene.live.speedlimitahead_valid  );


    // tpms
    auto tpms = scene.car_state.getTpms();
    float fl = tpms.getFl();
    float fr = tpms.getFr();
    float rl = tpms.getRl();
    float rr = tpms.getRr();
    ui_print( s, x_pos, y_pos+400, "tpms:%.0f,%.0f,%.0f,%.0f", fl, fr, rl, rr );
}

static void ui_draw_debug(UIState *s) 
{
  UIScene &scene = s->scene;

  ui_draw_modeSel(s);

  if( scene.dash_menu_no == 0 ) return;
  ui_draw_debug1(s);
  
  if( scene.dash_menu_no == 2 ) 
  {
    ui_draw_debug2(s);
  }




   // ui_print( s, 0, 1020, "%s", scene.alert.text1 );
   // ui_print( s, 0, 1078, "%s", scene.alert.text2 );
}


/*
  park @1;
  drive @2;
  neutral @3;
  reverse @4;
  sport @5;
  low @6;
  brake @7;
  eco @8;
*/
void ui_draw_gear( UIState *s, int center_x, int center_y )
{
  UIScene &scene = s->scene;
  NVGcolor nColor = COLOR_WHITE;

  cereal::CarState::GearShifter  getGearShifter = scene.car_state.getGearShifter();

  int  ngetGearShifter = int(getGearShifter);
  char str_msg[512];

  nvgFontSize(s->vg, 150 );
  switch( ngetGearShifter )
  {
    case 1 : strcpy( str_msg, "P" ); nColor = nvgRGBA(200, 200, 255, 255); break;
    case 2 : strcpy( str_msg, "D" ); nColor = nvgRGBA(200, 200, 255, 255); break;
    case 3 : strcpy( str_msg, "N" ); nColor = COLOR_WHITE; break;
    case 4 : strcpy( str_msg, "R" ); nColor = COLOR_RED;  break;
    case 7 : strcpy( str_msg, "B" ); break;
    default: sprintf( str_msg, "N" ); nColor = nvgRGBA(255, 100, 100, 255); break;
  }

  //nvgTextAlign(s->vg, NVG_ALIGN_CENTER | NVG_ALIGN_BASELINE);
  nvgFillColor(s->vg, nColor);
  ui_print( s, center_x, center_y, str_msg );
}

int get_param( const std::string &key )
{
    auto str = QString::fromStdString(Params().get( key ));
    int value = str.toInt();


    return value;
}



void update_dashcam(UIState *s, int draw_vision)
{
  if (!s->awake) return;
  int touch_x = s->scene.mouse.touch_x;
  int touch_y = s->scene.mouse.touch_y;
  int touched = s->scene.mouse.touched;

  if ( program_start )
  {
    program_start = 0;

    Params::param_value.autoFocus = get_param("OpkrAutoFocus");
    s->scene.scr.autoScreenOff = get_param("OpkrAutoScreenOff");
    s->scene.scr.brightness = get_param("OpkrUIBrightness");

    s->scene.scr.nTime = s->scene.scr.autoScreenOff * 60 * UI_FREQ;
    printf("autoScreenOff=%d, brightness=%d \n", s->scene.scr.autoScreenOff, s->scene.scr.brightness);       
  }
  else if ( touched  ) 
  {
    s->scene.mouse.touched = 0; 
  }


  if (!draw_vision) return;
  if (!s->scene.started) return;
  if (s->scene.driver_view) return;


  screen_draw_button(s, touch_x, touch_y, touched);
  screen_menu_button(s, touch_x, touch_y, touched);

  if( s->scene.dash_menu_no == 1 ) 
    focus_menu_button(s, touch_x, touch_y, touched);

  if( lock_current_video == true  )
  {
    float v_ego = s->scene.car_state.getVEgo();
    int engaged = s->scene.controls_state.getEngageable();
    if(  (v_ego < 0.1 || !engaged) )
    {
      elapsed_time = get_time() - stop_time;
      if( captureState == CAPTURE_STATE_CAPTURING && elapsed_time > 2 )
      {
        capture_cnt = 0;
        stop_capture();
      }
    }    
    else if( captureState != CAPTURE_STATE_CAPTURING )
    {
      capture_cnt = 0;
      start_capture();
    }
    else
    {
      stop_time = get_time();
    }
    
  }
  else  if( captureState == CAPTURE_STATE_CAPTURING )
  {
    capture_cnt = 0;
    stop_capture();
  }
  
 
  ui_draw_debug( s ); 
}





