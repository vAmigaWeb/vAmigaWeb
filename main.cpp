/*!
 * @file        main.cpp
 * @author      mithrendal and Dirk W. Hoffmann, www.dirkwhoffmann.de
 * @copyright   Dirk W. Hoffmann. All rights reserved.
 */

#include <stdio.h>
#include "VAmigaConfig.h"
#include "VAmiga.h"
#include "VAmigaTypes.h"
#include "Emulator.h"
#include "Amiga.h"
#include "AmigaTypes.h"
#include "RomFile.h"
#include "ExtendedRomFile.h"
#include "ADFFile.h"
#include "DMSFile.h"
#include "EXEFile.h"
#include "HDFFile.h"
#include "Snapshot.h"
#include "EADFFile.h"
#include "STFile.h"
#include "OtherFile.h"

#include "MemUtils.h"

#include <emscripten.h>

#ifdef wasm_worker
#include <emscripten/wasm_worker.h>
#include <emscripten/threading.h>
#endif

using namespace vamiga;

#define RENDER_SOFTWARE 0
#define RENDER_GPU 1
#define RENDER_SHADER 2
u8 render_method = RENDER_SOFTWARE;

#define DISPLAY_NARROW   0
#define DISPLAY_STANDARD 1
#define DISPLAY_WIDER    2
#define DISPLAY_OVERSCAN 3
#define DISPLAY_ADAPTIVE 4
#define DISPLAY_BORDERLESS 5
#define DISPLAY_EXTREME 6
u8 geometry  = DISPLAY_ADAPTIVE;


const char* display_names[] = {"narrow","standard","wider","overscan",
"viewport tracking","borderless","extreme"}; 

bool log_on=false;


//HRM: In NTSC, the fields are 262, then 263 lines and in PAL, 312, then 313 lines.
//312-50=262
#define PAL_EXTRA_VPIXEL 40 //50

#define PAL_FPS 50.0 //50.080128
#define NTSC_FPS 60.0 //59.94
bool ntsc=false;
double target_fps = PAL_FPS;

extern "C" void wasm_set_display(const char *name);
#ifdef wasm_worker
EM_JS(void, console_log, (const char* str), {
  console.log(UTF8ToString(str));
});
void log_on_main_thread(const char *msg)
{
  console_log(msg);
  //printf("%s",msg);
  //assert(!emscripten_current_thread_is_wasm_worker());
}
void main_log(const char *msg)
{
  //call function on ui thread
  emscripten_wasm_worker_post_function_sig(EMSCRIPTEN_WASM_WORKER_ID_PARENT,(void *) log_on_main_thread, "id", msg);
}
#endif

int clip_width  = 724 * TPP;
int clip_height = 568;
int clip_offset = 0;//HBLANK_MIN*4;//HBLANK_MAX*4; 
int buffer_size = 4096;

bool prevLOF = false;
bool currLOF = false;

int eat_border_width = 0;
int eat_border_height = 0;
int xOff = 12 + eat_border_width;
int yOff = 12 + eat_border_height;
int clipped_width  = HPIXELS*TPP -12 -24 -2*eat_border_width; //392
int clipped_height = VPOS_MAX -12 -24 -2*eat_border_height; //248

int bFullscreen = false;

char *filename = NULL;


bool requested_targetFrameCount_reset=false; 

//unsigned int warp_to_frame=0;
int sum_samples=0;
double last_time = 0.0 ;
double last_time_calibrated = 0.0 ;
unsigned int executed_frame_count=0;
int64_t total_executed_frame_count=0;
double start_time=0;//emscripten_get_now();
unsigned int rendered_frame_count=0;
unsigned int frames=0, seconds=0;

double speed_boost=1.0;
bool vsync = false;
signed vsync_speed=2;
u8 vframes=0;
unsigned long current_frame=100;
unsigned host_refresh_rate=60, last_host_refresh_rate=60;
unsigned host_refresh_count=0;
signed boost_param=100;
void calibrate_boost(signed boost_param);

u16 vstart_min=PAL::VBLANK_CNT;
u16 vstop_max=256;
u16 hstart_min=200;
u16 hstop_max=HPIXELS;

u16 vstart_min_tracking=NTSC::VBLANK_CNT;
u16 vstop_max_tracking=256;
u16 hstart_min_tracking=200;
u16 hstop_max_tracking=HPIXELS;
bool reset_calibration=true;
bool request_to_reset_calibration=false;

void set_viewport_dimensions()
{
//    hstart_min=0; hstop_max=HPIXELS;
    
    auto _vstop_max = vstop_max;
    if(ntsc)
    {
      if(vstop_max > NTSC::VPOS_MAX)
        _vstop_max = NTSC::VPOS_MAX;
    }
    else
    {
     if(vstop_max > PAL::VPOS_MAX) 
        _vstop_max = PAL::VPOS_MAX; //312
    }
 
    if(log_on) printf("calib: set_viewport_dimensions hmin=%d, hmax=%d, vmin=%d, vmax=%d vmax_clipped=%d\n",hstart_min,hstop_max,vstart_min, vstop_max, _vstop_max);

    if(render_method==RENDER_SHADER)
    {
      if(geometry == DISPLAY_ADAPTIVE || geometry == DISPLAY_BORDERLESS)
      {         
        xOff = hstart_min;
        yOff = vstart_min;
        clipped_width = hstop_max-hstart_min;
        clipped_height = _vstop_max-vstart_min;
      } 
    }
    else
    {  
      if(geometry== DISPLAY_ADAPTIVE || geometry == DISPLAY_BORDERLESS)
      {         
        xOff = hstart_min;
        yOff = vstart_min;
        clipped_width = hstop_max-hstart_min;
        clipped_height = _vstop_max-vstart_min;
      }
    }
    if(clipped_height>0 && clipped_width>0)
    {
      EM_ASM({js_set_display($0,$1,$2,$3); scaleVMCanvas();},xOff, yOff, clipped_width*TPP,clipped_height );
    }
}



u16 vstart_min_calib=0;
u16 vstop_max_calib=0;
u16 hstart_min_calib=0;
u16 hstop_max_calib=0;
bool calculate_viewport_dimensions(u32 *texture)
{
  if(reset_calibration)
  {
      //printf("reset_calibration %d %d\n",vstop_max_tracking, vstart_min_tracking);
      //first call after new viewport size
      //set start values to the opposite max. borderpos (scan area)  
      vstart_min_calib = vstop_max_tracking; 
      vstop_max_calib = vstart_min_tracking;
      hstart_min_calib = hstop_max_tracking;
      hstop_max_calib = hstart_min_tracking;  
      reset_calibration=false;

  }

  bool pixels_found=false;
  //top border: get vstart_min from texture
  u32 ref_pixel= texture[(HPIXELS*vstart_min_tracking + hstart_min_tracking)*TPP];
//    printf("refpixel:%u\n",ref_pixel);
  for(int y=vstart_min_tracking;y<vstart_min_calib && !pixels_found;y++)
  {
//      printf("\nvstart_line:%u\n",y);
    for(int x=hstart_min_tracking;x<hstop_max_tracking;x++){
      u32 pixel= texture[(HPIXELS*y + x)*TPP];
//        printf("%u:%u ",x,pixel);
      if(ref_pixel != pixel){
        pixels_found=true;
//        printf("\nfirst_pos=%d, vstart_min=%d, vstart_min_track=%d\n",y, vstart_min, vstart_min_tracking);
        vstart_min_calib= y<vstart_min_calib?y:vstart_min_calib;
        break;
      }
    }
  }
  
  //bottom border: get vstop_max from texture
  pixels_found=false;
  ref_pixel= texture[ (HPIXELS*vstop_max_tracking + hstart_min_tracking)*TPP];
//    printf("refpixel:%u\n",ref_pixel);
//    printf("hstart:%u,hstop:%u\n",hstart_min,hstop_max);
  
  for(int y=vstop_max_tracking;y>vstop_max_calib && !pixels_found;y--)
  {
//      printf("\nline:%u\n",y);
    for(int x=hstart_min_tracking;x<hstop_max_tracking;x++){
      u32 pixel= texture[(HPIXELS*y + x)*TPP];
//        printf("%u:%u ",x,pixel);
      if(ref_pixel != pixel){
        pixels_found=true;
        y++; //this line has pixels, so put vstop_max to the next line
//        printf("\ncalib: last_pos=%d, vstop_max=%d, vstop_max_tracking%d\n",y, vstop_max, vstop_max_tracking);
        vstop_max_calib= y>vstop_max_calib?y:vstop_max_calib;
        break;
      }
    }
  }

  //left border: get hstart_min from texture
  pixels_found=false;
  ref_pixel= texture[ (HPIXELS*vstart_min_tracking + hstart_min_tracking-1)*TPP];

  for(int x=hstart_min_tracking-1;x<hstart_min_calib;x++)
  {
//      printf("\nrow:%u\n",x);
    for(int y=vstart_min_calib;y<vstop_max_calib && !pixels_found;y++)
    {
      u32 pixel= texture[(HPIXELS*y + x)*TPP];
//        printf("%u:%u ",x,pixel);
      if(ref_pixel != pixel){
        pixels_found=true;
//          printf("\nlast_xpos=%d, hstop_max=%d\n",x, hstop_max);
        hstart_min_calib=x<hstart_min_calib?x:hstart_min_calib;
        break;
      }
    }
  }

  //right border: get hstop_max from texture
  pixels_found=false;
  ref_pixel= texture[ (HPIXELS*vstart_min_tracking + hstop_max_tracking)*TPP];
//  printf("hstop_max_tracking %d\n",hstop_max_tracking);
//  printf("hstop_max_calib %d\n",hstop_max_calib);
//  printf("ref_pixel %d\n",ref_pixel);

  for(int x=hstop_max_tracking;x>hstop_max_calib;x--)
  {
//     printf("\ncol:%u\n",x);
    for(int y=vstart_min_calib;y<vstop_max_calib && !pixels_found;y++)
    {
      u32 pixel= texture[(HPIXELS*y + x)*TPP];
//      printf("%u:%u ",x,pixel);
      if(ref_pixel != pixel){
        pixels_found=true;
        x++; //this line has pixels, so put hstop_max to the next line
//          printf("\nlast_xpos=%d, hstop_max=%d\n",x, hstop_max);
        hstop_max_calib= x>hstop_max_calib?x:hstop_max_calib;
        break;
      }
    }
  }
//  printf("->hstop_max_calib %d\n",hstop_max_calib);

  bool dimensions_changed=false;

  //handdisk start pixel 416, stop pixel 676
  #define HANDSTART_PIXEL 416
  #define HANDSTOP_PIXEL 676


  if(hstart_min_calib > HANDSTART_PIXEL-32)
    hstart_min_calib= HANDSTART_PIXEL-32;
  if(hstop_max_calib < HANDSTOP_PIXEL+32)
    hstop_max_calib= HANDSTOP_PIXEL+32;

  if(hstart_min_calib < hstop_max_calib )
  {
    if(hstart_min!=hstart_min_calib)
    {
      hstart_min=hstart_min_calib;
      dimensions_changed=true;
    }
    if(hstop_max != hstop_max_calib)
    {
      hstop_max = hstop_max_calib;
      dimensions_changed=true;
    }
  }

  if(vstart_min_calib > VPOS_MAX/4)
    vstart_min_calib= VPOS_MAX/4;
  if(vstop_max_calib < 3*VPOS_MAX/4)
    vstop_max_calib= 3*VPOS_MAX/4;

  if(vstart_min_calib< vstop_max_calib)
  {
    if(vstart_min != vstart_min_calib)
    {
      vstart_min=vstart_min_calib;
      dimensions_changed=true;
    }
    if(vstop_max != vstop_max_calib)
    {
      vstop_max=vstop_max_calib;   
      dimensions_changed=true;
    }
 
  }
//  printf("\nCALIBRATED: (%d,%d) (%d,%d) \n",hstart_min, vstart_min, hstop_max, vstop_max);

  //printf("calib dimensions changed=%d\n",dimensions_changed);
  return dimensions_changed;
}


#define MAX_GAP 5
VAmiga *emu=NULL;
u8 executed_since_last_host_frame=0;
extern "C" void wasm_execute()
{
  emu->emu->computeFrame(); //execute();
  executed_since_last_host_frame++;
  executed_frame_count++;
  total_executed_frame_count++;
}
#ifdef wasm_worker
char wasm_log_buffer[256];
#endif
extern "C" int wasm_draw_one_frame(double now)
//int draw_one_frame_into_SDL(void *thisAmiga, float now) 
{

  //this method is triggered by
  //emscripten_set_main_loop_arg(em_arg_callback_func func, void *arg, int fps, int simulate_infinite_loop) 
  //which is called inside te c64.cpp
  //fps Setting int <=0 (recommended) uses the browser’s requestAnimationFrame mechanism to call the function.

  //The number of callbacks is usually 60 times per second, but will 
  //generally match the display refresh rate in most web browsers as 
  //per W3C recommendation. requestAnimationFrame() 
  
  //double now = emscripten_get_now();  

  double elapsedTimeInSeconds = (now - start_time)/1000.0;
  int64_t targetFrameCount = (int64_t)(elapsedTimeInSeconds * target_fps *speed_boost);
  
  emu->emu->update();

  bool show_stat=true;
  if(emu->isWarping() == true)
  {
    if(log_on) printf("warping at least 25 frames at once ...\n");
    int i=25;
    while(emu->isWarping() == true && i>0)
    {
      emu->emu->computeFrame();
      i--;
    }
    start_time=now;
    total_executed_frame_count=0;
    targetFrameCount=1;
    show_stat=false;
  }

  if(requested_targetFrameCount_reset)
  {
    start_time=now;
    total_executed_frame_count=0;
    targetFrameCount=1;
    requested_targetFrameCount_reset=false;
  }

  if(show_stat)
  {
    //lost the sync
    if(targetFrameCount-total_executed_frame_count > MAX_GAP)
    {
        if(log_on) {
#ifdef wasm_worker          
          snprintf(wasm_log_buffer,sizeof(wasm_log_buffer),"lost sync target=%lld, total_executed=%lld\n", targetFrameCount, total_executed_frame_count);
          main_log(wasm_log_buffer);
#else
          printf("lost sync target=%lld, total_executed=%lld\n", targetFrameCount, total_executed_frame_count);
#endif
        }
        //reset timer
        //because we are out of sync, we do now skip max_gap-1 emulation frames 
        start_time=now;
        total_executed_frame_count=0;
        targetFrameCount=1;  //we are hoplessly behind but do at least one in this round  
    }
    
    host_refresh_count++;
    if(now-last_time>= 1000.0)
    { 
      double passed_time= now - last_time;
      last_time = now;

      seconds += 1; 
      frames += rendered_frame_count;
      if(log_on) {
#ifdef wasm_worker
       snprintf(wasm_log_buffer,sizeof(wasm_log_buffer),"time[ms]=%.0lf, audio_samples=%d, frames [executed=%u, rendered=%u] avg_fps=%u\n", 
       passed_time, sum_samples, executed_frame_count, rendered_frame_count, frames/seconds);
       main_log(wasm_log_buffer);
#else        
      printf("time[ms]=%.0lf, audio_samples=%d, frames [executed=%u, rendered=%u] avg_fps=%u\n", 
      passed_time, sum_samples, executed_frame_count, rendered_frame_count, frames/seconds);
#endif
      }
      host_refresh_rate=host_refresh_count;
      host_refresh_count=0;
      sum_samples=0; 
      rendered_frame_count=0;
      executed_frame_count=0;
    }
  }

  int behind=0;
  if(vsync)
  {
    //current_frame=0, vsync_speed=-2, vframes=0
    //printf("current_frame=%ld, vsync_speed=%d, vframes=%d\n", current_frame, vsync_speed, vframes); 
    current_frame++;
    
    if(vsync_speed<0)
    {    
      if(current_frame % (vsync_speed*-1) !=0)
      {
//        printf("skip frame %ld\n", current_frame % ((unsigned long)vsync_speed*-1) );
        return -1;
      }
      else{
        emu->emu->computeFrame();
//        printf("compute_frame \n"); 
        executed_frame_count++;
        total_executed_frame_count++;
      }
    }
    else
    {
      //0 + 0 < 0 -2
      while(current_frame+vframes  < current_frame + vsync_speed)
      {
        emu->emu->computeFrame();
        //printf("compute_frame \n"); 
        executed_frame_count++;
        total_executed_frame_count++;
        vframes++;
      }
      //printf("\n"); 
      vframes=0;
    }
    //check current frame rate +-1 in case user changed it on host system
    if(abs((long) (host_refresh_rate-last_host_refresh_rate))>1)
    {
      calibrate_boost(boost_param);
      last_host_refresh_rate=host_refresh_rate;
    }
  }
  else
  {
    behind=targetFrameCount-total_executed_frame_count;
    if(behind<=0 && executed_since_last_host_frame==0)
      return -1;   //don't render if ahead of time and everything is already drawn
  }
#ifndef wasm_worker    
  if(behind>0)
  {
    emu->emu->computeFrame();
 
    executed_frame_count++;
    total_executed_frame_count++;
    behind--;
  }
#endif
  executed_since_last_host_frame=0;
  rendered_frame_count++;

  if(geometry == DISPLAY_BORDERLESS)
  {
//    printf("calibration count=%f\n",now-last_time_calibrated);
    if(now-last_time_calibrated >= 700.0)
    {  
      last_time_calibrated=now;
      auto stable_ptr = emu->videoPort.getTexture(); 

      bool dimensions_changed=calculate_viewport_dimensions((u32 *)stable_ptr - HBLANK_MIN*4*TPP);
      if(dimensions_changed)
      {
#ifdef wasm_worker
        emscripten_wasm_worker_post_function_v(EMSCRIPTEN_WASM_WORKER_ID_PARENT,set_viewport_dimensions);
#else
        set_viewport_dimensions();
#endif
      }  

      if(request_to_reset_calibration)
      {
        reset_calibration=true;
        request_to_reset_calibration=false;
      }
    }
  }
  return behind;
}


int sample_size=0;


void send_message_to_js(const char * msg)
{
    EM_ASM(
    {
        if (typeof message_handler === 'undefined')
            return;
        message_handler( "MSG_"+UTF8ToString($0) );
    }, msg );    

}


void send_message_to_js_main_thread(const char * msg, long data1, long data2)
{
    EM_ASM(
    {
        if (typeof message_handler === 'undefined')
            return;
        message_handler( "MSG_"+UTF8ToString($0), $1, $2 );
    }, msg, data1, data2 );    

}


void send_message_to_js_with_param(const char * message_as_string, long data1, long data2)
{

    if(log_on)
    {
#ifdef wasm_worker          
      if(emscripten_current_thread_is_wasm_worker())
      {
        snprintf(wasm_log_buffer,sizeof(wasm_log_buffer),"worker: vAmiga message=%s, data1=%ld, data2=%ld\n", message_as_string, data1, data2);
        main_log(wasm_log_buffer);
      }
      else
      {
        printf("main: vAmiga message=%s\n", message_as_string);
      }
#else
      printf("vAmiga message=%s, data1=%ld, data2=%ld\n", message_as_string, data1, data2);
#endif
    }


#ifdef wasm_worker
    if(emscripten_current_thread_is_wasm_worker())
    {
      emscripten_wasm_worker_post_function_sig(EMSCRIPTEN_WASM_WORKER_ID_PARENT,(void*)send_message_to_js_main_thread, "iii", message_as_string, data1, data2);
    }
    else
      send_message_to_js_main_thread(message_as_string, data1, data2);
#else
    send_message_to_js_main_thread(message_as_string, data1, data2);
#endif
}

/*
void send_message_to_js_w(const char * msg, long data1, long data2)
{
    send_message_to_js_main_thread(msg,data1,data2);
}
*/


//bool paused_the_emscripten_main_loop=false;
bool already_run_the_emscripten_main_loop=false;
bool warp_mode=false;
//void theListener(const void * amiga, long type,  int data1, int data2, int data3, int data4){
void theListener(const void * emu, Message msg){
  int data1=msg.value;
  int data2=0;
  if(msg.type == Msg::VIEWPORT)
  {
    if(msg.viewport.hstrt==0 && msg.viewport.vstrt==0 && msg.viewport.hstop == 0 && msg.viewport.vstop == 0)
      return;
    hstart_min= msg.viewport.hstrt; 
    vstart_min= msg.viewport.vstrt;
    hstop_max=  msg.viewport.hstop;
    vstop_max=  msg.viewport.vstop;
    if(log_on) printf("tracking MSG_VIEWPORT=%d, %d, %d, %d\n",hstart_min, vstart_min, hstop_max, vstop_max);

    hstart_min *=2;
    hstop_max *=2;

    hstart_min=hstart_min<(208-48) ? (208-48):hstart_min;
    hstop_max=hstop_max>(HPIXELS+HBLANK_MAX)? (HPIXELS+HBLANK_MAX):hstop_max;

    if(vstart_min < (ntsc ? NTSC::VBLANK_CNT : PAL::VBLANK_CNT)) 
      vstart_min = (ntsc ? NTSC::VBLANK_CNT : PAL::VBLANK_CNT);

    
    if(log_on)
    {
#ifdef wasm_worker          
      snprintf(wasm_log_buffer,sizeof(wasm_log_buffer),"tracking MSG_VIEWPORT=%u %u %u %u\n",hstart_min, vstart_min, hstop_max, vstop_max);
      main_log(wasm_log_buffer);
#else
      printf("tracking MSG_VIEWPORT=%u %u %u %u\n",hstart_min, vstart_min, hstop_max, vstop_max);
#endif
    }

    vstart_min_tracking = vstart_min;
    vstop_max_tracking = vstop_max;
    hstart_min_tracking = hstart_min;
    hstop_max_tracking = hstop_max;

#ifdef wasm_worker
    emscripten_wasm_worker_post_function_v(EMSCRIPTEN_WASM_WORKER_ID_PARENT,set_viewport_dimensions);
#else
    set_viewport_dimensions();
#endif

    reset_calibration=true;
  }
  else if(msg.type == Msg::VIDEO_FORMAT)
  {
    if(log_on) printf("video format=%s data1=%d\n",TVEnum::key(TV(msg.value)),data1);

    EM_ASM({use_ntsc_pixel= $0==1?true:false},TV(msg.value) == TV::NTSC);
    wasm_set_display(TV(msg.value) == TV::NTSC? "ntsc":"pal");
    request_to_reset_calibration=true;
  }
  else if(msg.type == Msg::DRIVE_STEP || msg.type == Msg::DRIVE_POLL 
     ||msg.type == Msg::HDR_STEP)
  {
      data1=msg.drive.nr;
      data2=msg.drive.value;
  }
  
  if(msg.type == Msg::DRIVE_SELECT)
  {
  }
  else
  {
    const char *message_as_string =  (const char *)MsgEnum::key(Msg(msg.type));

    if(msg.type == Msg::SER_OUT)
    {
      int byte = ((VAmiga *)emu)->serialPort.serialPort->readOutgoingByte();
      while(byte>=0)
      {
        send_message_to_js_with_param(message_as_string, byte, data2);
        byte = ((VAmiga *)emu)->serialPort.serialPort->readOutgoingByte();
      }
    }
    else
    {
      send_message_to_js_with_param(message_as_string, data1, data2);
    }
  }
}




class vAmigaWrapper {
  public:
    VAmiga *emu;

  vAmigaWrapper()
  {
    printf("constructing vAmiga ...\n");
    this->emu = new VAmiga();
  }
  ~vAmigaWrapper()
  {
        printf("closing wrapper");
  }

  void run()
  {
    try { emu->isReady(); } catch(...) { 
      printf("***** put missing rom message\n");
       // amiga->msgQueue.put(ROM_MISSING); 
        EM_ASM({
          setTimeout(function() {message_handler( 'MSG_ROM_MISSING' );}, 0);
        });
    }
    
    printf("wrapper calls run on vAmiga->run() method\n");


    emu->defaults.defaults->setFallback(Opt::HDC_CONNECT, false, {0});

//  wrapper->emu->defaults.setFallback(OPT_FILTER_TYPE, FILTER_NONE);
//  wrapper->emu->set(OPT_FILTER_TYPE, FILTER_NONE);

  // master Volumne
    emu->set(Opt::AUD_VOLL, 100); 
    emu->set(Opt::AUD_VOLR, 100);

  //Volumne
//  wrapper->emu->set(OPT_AUD_VOL0, 100); why did I set it only on channel 0? 
//  wrapper->emu->set(OPT_AUD_PAN0, 0);


  emu->set(Opt::MEM_CHIP_RAM, 512);
  emu->set(Opt::MEM_SLOW_RAM, 512);
  emu->set(Opt::AGNUS_REVISION, (i64)AgnusRevision::OCS);

  //turn automatic hd mounting off because kick1.2 makes trouble
  emu->set(Opt::HDC_CONNECT, false, /*hd drive*/ {0});

  emu->set(Opt::DRIVE_CONNECT,true, /*df1*/ {1});

  emu->emu->update();





    printf("waiting on emulator ready in javascript ...\n");
 
  }
};

#ifdef wasm_worker
emscripten_wasm_worker_t worker;

void run_in_worker()
{
//  EM_ASM({console.log("Hello log")});
//  emscripten_wasm_worker_post_function_sig(EMSCRIPTEN_WASM_WORKER_ID_PARENT,(void *) test_success, "id");
  auto behind = wasm_draw_one_frame(emscripten_performance_now());
  while(behind>0)
  {
    emu->execute();
    executed_since_last_host_frame++;
    executed_frame_count++;
    total_executed_frame_count++;
    behind--;
  }
}
#endif

extern "C" bool wasm_is_worker_built()
{
#ifdef wasm_worker
  return true;
#else
  return false;
#endif
}

extern "C" void wasm_worker_run()
{
  #ifdef wasm_worker
    emscripten_wasm_worker_post_function_v(worker, run_in_worker); 
  #endif
}

vAmigaWrapper *wrapper = NULL;
int main(int argc, char** argv) {
#ifdef wasm_worker
  worker = emscripten_malloc_wasm_worker(/*stack size: */2048);
  printf("running on wasm webworker...\n");
#else
  printf("running on main thread...\n");
#endif
  wrapper= new vAmigaWrapper();

  printf("connecting listener to vAmiga message queue...\n");

//  try{
      wrapper->emu->launch(wrapper->emu, &theListener);
//  } catch(std::exception &exception) {
//      printf("%s\n", exception.what());
//  }
//  printf("launch completed\n");

  wrapper->run();
  return 0;
}


extern "C" Texel * wasm_pixel_buffer()
{
//  auto stable_ptr = emu->denise.denise->pixelEngine.stablePtr();
  auto stable_ptr = emu->emu->getTexture().pixels.ptr;
  return stable_ptr;
}
extern "C" u32 wasm_frame_info()
{
//  auto &stableBuffer = emu->denise.denise->pixelEngine.getStableBuffer();
  auto &stableBuffer = emu->emu->getTexture();
  u32 info = (u32)stableBuffer.nr;
  info = info<<1;

  if(stableBuffer.prevlof)
    info |= 0x0001;
  
  info = info<<1;
  
  if(stableBuffer.lof)
    info |= 0x0001;
  
  return info;
}


extern "C" int wasm_get_renderer()
{ 
  return render_method;
}

extern "C" int wasm_get_render_width()
{ 
  return clipped_width;
}
extern "C" int wasm_get_render_height()
{ 
  return clipped_height;
}

extern "C" void wasm_set_target_fps(int _target_fps)
{ 
  target_fps=_target_fps;
}


/* emulation of macos mach_absolute_time() function. */
uint64_t mach_absolute_time()
{
    uint64_t nano_now = (uint64_t)(emscripten_get_now()*1000000.0);
    //printf("emsdk_now: %lld\n", nano_now);
    return nano_now; 
}

extern "C" void wasm_key(int code, int pressed)
{
//  printf("wasm_key ( %d, %d ) \n", code, pressed);

  if(pressed==1)
  {
//    wrapper->emu->keyboard.keyboard->pressKey(code);
      wrapper->emu->keyboard.keyboard->press(KeyCode(code));
  }
  else
  {
    wrapper->emu->keyboard.release(KeyCode(code));
  }
  wrapper->emu->emu->update();

}

extern "C" void wasm_auto_type(int code, int duration, int delay)
{//obsolete
//    if(log_on) printf("auto_type ( %d, %d, %d ) \n", code, duration, delay);
//    wrapper->emu->keyboard.autoType(code, MSEC(duration), MSEC(delay));
}


extern "C" void wasm_schedule_key(int code1, int code2, int pressed, int frame_delay)
{
  if(pressed==1)
  {
//    printf("scheduleKeyPress ( %d, %d, %d ) \n", code1, code2, frame_delay);
    wrapper->emu->keyboard.press(KeyCode(code1));
//    wrapper->emu->keyboard.scheduleKeyPress(*new AmigaKey(code1,code2), frame_delay);
  }
  else
  {
//    printf("scheduleKeyRelease ( %d, %d, %d ) \n", code1, code2, frame_delay);
    wrapper->emu->keyboard.release(KeyCode(code1));
  
  //  wrapper->emu->keyboard.scheduleKeyRelease(*new C64Key(code1,code2), frame_delay);
  }
  wrapper->emu->emu->update();

}





char wasm_pull_user_snapshot_file_json_result[255];


extern "C" bool wasm_has_disk(const char *drive_name)
{
  if(strcmp(drive_name,"df0") == 0)
  {
    return wrapper->emu->df0.getInfo().hasDisk;
  }
  else if(strcmp(drive_name,"df1") == 0)
  {
    return wrapper->emu->df1.getInfo().hasDisk;
  }
  else if(strcmp(drive_name,"df2") == 0)
  {
    return wrapper->emu->df2.getInfo().hasDisk;
  }
  else if(strcmp(drive_name,"df3") == 0)
  {
    return wrapper->emu->df3.getInfo().hasDisk;
  }
  else if (strcmp(drive_name,"dh0") == 0)
  {
    return wrapper->emu->hd0.getInfo().hasDisk;
  }
  else if (strcmp(drive_name,"dh1") == 0)
  {
    return wrapper->emu->hd1.getInfo().hasDisk;
  }
  else if (strcmp(drive_name,"dh2") == 0)
  {
    return wrapper->emu->hd2.getInfo().hasDisk;
  }
  else if (strcmp(drive_name,"dh3") == 0)
  {
    return wrapper->emu->hd3.getInfo().hasDisk;
  }

  return false;
}

extern "C" void wasm_eject_disk(const char *drive_name)
{
  if(strcmp(drive_name,"df0") == 0)
  {
    if(wrapper->emu->df0.getInfo().hasDisk)
      wrapper->emu->df0.ejectDisk();
  }
  else if(strcmp(drive_name,"df1") == 0)
  {
    if(wrapper->emu->df1.getInfo().hasDisk)
      wrapper->emu->df1.ejectDisk();
  }
  else if(strcmp(drive_name,"df2") == 0)
  {
    if(wrapper->emu->df2.getInfo().hasDisk)
      wrapper->emu->df2.ejectDisk();
  }
  else if(strcmp(drive_name,"df3") == 0)
  {
    if(wrapper->emu->df3.getInfo().hasDisk)
      wrapper->emu->df3.ejectDisk();
  }
  else if (strcmp(drive_name,"dh0") == 0)
  {
    if(wrapper->emu->hd0.getInfo().hasDisk)
    {
      wrapper->emu->powerOff();wrapper->emu->emu->update();
      wrapper->emu->set(Opt::HDC_CONNECT, false, /*hd drive*/ {0});
      wrapper->emu->powerOn();
    }
  }
  else if (strcmp(drive_name,"dh1") == 0)
  {
    if(wrapper->emu->hd1.getInfo().hasDisk)
    {
      wrapper->emu->powerOff();wrapper->emu->emu->update();
      wrapper->emu->set(Opt::HDC_CONNECT, false, /*hd drive*/ {1});
      wrapper->emu->powerOn();
    }
  }
  else if (strcmp(drive_name,"dh2") == 0)
  {
    if(wrapper->emu->hd2.getInfo().hasDisk)
    {
      wrapper->emu->powerOff();wrapper->emu->emu->update();
      wrapper->emu->set(Opt::HDC_CONNECT, false, /*hd drive*/ {2});
      wrapper->emu->powerOn();
    }
  }
  else if (strcmp(drive_name,"dh3") == 0)
  {
    if(wrapper->emu->hd3.getInfo().hasDisk)
    {
      wrapper->emu->powerOff();wrapper->emu->emu->update();
      wrapper->emu->set(Opt::HDC_CONNECT, false, /*hd drive*/ {3});
      wrapper->emu->powerOn();
    }
  }

}

DiskFile *export_disk=NULL;
extern "C" void wasm_delete_disk()
{
  if(export_disk!=NULL)
  {
    delete export_disk;
    export_disk=NULL;
    if(log_on) printf("disk memory deleted\n");
  }
}
extern "C" char* wasm_export_disk(const char *drive_name)
{
  wasm_delete_disk();
  Buffer<u8> *data=NULL;
  sprintf(wasm_pull_user_snapshot_file_json_result, "{\"size\": 0 }");

  if(strcmp(drive_name,"df0") == 0)
  {
    if(!wrapper->emu->df0.getInfo().hasDisk)
    {
      return wasm_pull_user_snapshot_file_json_result;
    }
    export_disk = new ADFFile(wrapper->emu->df0.getDisk());
    data=&(export_disk->data);
  }
  else if(strcmp(drive_name,"df1") == 0)
  {
    if(!wrapper->emu->df1.getInfo().hasDisk)
    {
      return wasm_pull_user_snapshot_file_json_result;
    }
    export_disk = new ADFFile(wrapper->emu->df1.getDisk());
    data=&(export_disk->data);
  }
  else if(strcmp(drive_name,"df2") == 0)
  {
    if(!wrapper->emu->df2.getInfo().hasDisk)
    {
      return wasm_pull_user_snapshot_file_json_result;
    }
    export_disk = new ADFFile(wrapper->emu->df2.getDisk());
    data=&(export_disk->data);
  }
  else if(strcmp(drive_name,"df3") == 0)
  {
    if(!wrapper->emu->df3.getInfo().hasDisk)
    {
      return wasm_pull_user_snapshot_file_json_result;
    }
    export_disk = new ADFFile(wrapper->emu->df3.getDisk());
    data=&(export_disk->data);
  }
  else if (strcmp(drive_name,"dh0") == 0)
  {
    if(!wrapper->emu->hd0.getInfo().hasDisk)
    {
      return wasm_pull_user_snapshot_file_json_result;
    }

    export_disk = new HDFFile(wrapper->emu->hd0.getDrive());
    data=&(export_disk->data);
  }
  else if (strcmp(drive_name,"dh1") == 0)
  {
    if(!wrapper->emu->hd1.getInfo().hasDisk)
    {
      return wasm_pull_user_snapshot_file_json_result;
    }

    export_disk = new HDFFile(wrapper->emu->hd1.getDrive());
    data=&(export_disk->data);
  }
  else if (strcmp(drive_name,"dh2") == 0)
  {
    if(!wrapper->emu->hd2.getInfo().hasDisk)
    {
      return wasm_pull_user_snapshot_file_json_result;
    }

    export_disk = new HDFFile(wrapper->emu->hd2.getDrive());
    data=&(export_disk->data);
  }
  else if (strcmp(drive_name,"dh3") == 0)
  {
    if(!wrapper->emu->hd3.getInfo().hasDisk)
    {
      return wasm_pull_user_snapshot_file_json_result;
    }

    export_disk = new HDFFile(wrapper->emu->hd3.getDrive());
    data=&(export_disk->data);
  }
  
  sprintf(wasm_pull_user_snapshot_file_json_result, "{\"address\":%lu, \"size\": %lu }",
    (unsigned long)data->ptr, data->size);

  printf("return => %s\n",wasm_pull_user_snapshot_file_json_result);

  return wasm_pull_user_snapshot_file_json_result;
}


MediaFile *snapshot=NULL;
extern "C" void wasm_delete_user_snapshot()
{
//  printf("request to free user_snapshot memory\n");

  if(snapshot!=NULL)
  {
    delete snapshot;
    snapshot=NULL;
    if(log_on) printf("freed user_snapshot memory\n");
  }
}

extern "C" char* wasm_take_user_snapshot()
{
  printf("wasm_pull_user_snapshot_file\n");

  wasm_delete_user_snapshot();
  snapshot = wrapper->emu->amiga.takeSnapshot(); //wrapper->emu->userSnapshot(nr);

//  printf("got snapshot %u.%u.%u\n", snapshot->getHeader()->major,snapshot->getHeader()->minor,snapshot->getHeader()->subminor );
  u8 *data = (u8*)(((Snapshot *)snapshot)->getHeader());

  printf("data header bytes= %x, %x, %x\n", data[0],data[1],data[2]);
 



  sprintf(wasm_pull_user_snapshot_file_json_result, "{\"address\":%lu, \"size\": %lu, \"width\": %lu, \"height\":%lu }",
  (unsigned long)data,//snapshot->getData(), 
  snapshot->getSize(),
  snapshot->previewImageSize().first,
  snapshot->previewImageSize().second
  );
  printf("return => %s\n",wasm_pull_user_snapshot_file_json_result);

  return wasm_pull_user_snapshot_file_json_result;
}


float sound_buffer[16384 * 2];
extern "C" float* wasm_get_sound_buffer_address()
{
  return sound_buffer;
}

extern "C" unsigned wasm_copy_into_sound_buffer()
{
  auto count=wrapper->emu->audioPort.port->stream.count();
  
  auto copied_samples=0;
  for(unsigned ipos=1024;ipos<=count;ipos+=1024)
  {
    wrapper->emu->audioPort.copyStereo(
    sound_buffer+copied_samples,
     sound_buffer+copied_samples+1024, 
     1024); 
    copied_samples+=1024*2;
//  printf("fillLevel (%lf)",wrapper->emu->paula.muxer.stream.fillLevel());
  }
  sum_samples += copied_samples/2; 

/*  printf("copyMono[%d]: ", 16);
  for(int i=0; i<16; i++)
  {
    //printf("%hhu,",stream[i]);
    printf("%f,",sound_buffer[i]);
  }
  printf("\n"); 
*/
  return copied_samples/2;
}


extern "C" void wasm_set_warp(unsigned on)
{
  wrapper->emu->set(Opt::AMIGA_WARP_MODE,(i64) (on == 1 ?Warp::AUTO:Warp::NEVER)); 
}


extern "C" bool wasm_is_warping()
{
  return wrapper->emu->isWarping();
}




extern "C" void wasm_set_display(const char *name)
{
  if(log_on) printf("wasm_set_display('%s')\n",name);

  if( strcmp(name,"ntsc") == 0)
  {
    name= display_names[geometry];
    if(log_on) printf("resetting new display %s\n",name);
    if(!ntsc)
    {
      if(log_on) printf("was not yet ntsc\n");

      if(wrapper->emu->get(Opt::AMIGA_VIDEO_FORMAT)!= (i64)TV::NTSC)
      {
        if(log_on) printf("was not yet ntsc so we have to configure it\n");
        wrapper->emu->set(Opt::AMIGA_VIDEO_FORMAT, (i64)TV::NTSC);
      }
      target_fps=NTSC_FPS;
      total_executed_frame_count=0;
      ntsc=true;
    }
  }
  else if( strcmp(name,"pal") == 0)
  {
    name= display_names[geometry];
    if(log_on) printf("resetting  new display %s\n",name);
    if(ntsc)
    {
      if(log_on) printf("was not yet PAL\n");
      if(wrapper->emu->get(Opt::AMIGA_VIDEO_FORMAT)!=(i64) TV::PAL)
      {
        if(log_on) printf("was not yet PAL so we have to configure it\n");
        wrapper->emu->set(Opt::AMIGA_VIDEO_FORMAT, (i64)TV::PAL);
      }
      target_fps=PAL_FPS;
      total_executed_frame_count=0;
      ntsc=false;
    }
  }
  else if( strcmp(name,"") == 0)
  {
    name= display_names[geometry];
    if(log_on) printf("reset display=%s\n",name);
  }


  if( strcmp(name,"adaptive") == 0 || 
      strcmp(name,"auto") == 0 || 
      strcmp(name,"viewport tracking") == 0)
  {
    geometry=DISPLAY_ADAPTIVE;
    wrapper->emu->set(Opt::DENISE_VIEWPORT_TRACKING, true); 
  }
  else if( strcmp(name,"borderless") == 0)
  {
    geometry=DISPLAY_BORDERLESS;
    wrapper->emu->set(Opt::DENISE_VIEWPORT_TRACKING, true); 
    return;
  }
  else if( strcmp(name,"narrow") == 0)
  {
    wrapper->emu->set(Opt::DENISE_VIEWPORT_TRACKING, false); 
    geometry=DISPLAY_NARROW;
    xOff=252 + 4;
    yOff=PAL::VBLANK_CNT +16;
    clipped_width=HPIXELS-xOff - 8;   
    //clipped_height=312-yOff -2*24 -2; 
    clipped_height=(3*clipped_width/4 +(ntsc?0:32) /*32 due to PAL?*/)/2 & 0xfffe;
    if(ntsc){clipped_height-=PAL_EXTRA_VPIXEL;}
  }
  else if( strcmp(name,"standard") == 0)
  {
    wrapper->emu->set(Opt::DENISE_VIEWPORT_TRACKING, false); 
  
    geometry=DISPLAY_STANDARD;
    xOff=208+HBLANK_MAX;
    yOff=PAL::VBLANK_CNT +10;
    clipped_width=HPIXELS-xOff;
//    clipped_height=312-yOff -2*4  ;
//    clipped_height=(4*clipped_width/5 )/2 & 0xfffe;
    clipped_height=(3*clipped_width/4 +(ntsc?0:32) /*32 due to PAL?*/)/2 & 0xfffe;
    if(ntsc){clipped_height-=PAL_EXTRA_VPIXEL-10;}
  }
  else if( strcmp(name,"wider") == 0)
  {
    wrapper->emu->set(Opt::DENISE_VIEWPORT_TRACKING, false); 
  
    geometry=DISPLAY_WIDER;
    xOff=208+ HBLANK_MAX/2;
    yOff=PAL::VBLANK_CNT + 2;
    clipped_width=(HPIXELS+HBLANK_MAX/2 )-xOff;
//    clipped_height=312-yOff -2*2;
    clipped_height=(3*clipped_width/4 +(ntsc?0:32) /*32 due to PAL?*/)/2 & 0xfffe;
    if(ntsc){clipped_height-=PAL_EXTRA_VPIXEL-8;}
  }
  else if( strcmp(name,"overscan") == 0)
  {
    wrapper->emu->set(Opt::DENISE_VIEWPORT_TRACKING, false); 
  
    geometry=DISPLAY_OVERSCAN;

    xOff=208; //208 is first pixel in dpaint iv,overscan=max
    yOff=PAL::VBLANK_CNT; //must be even
    clipped_width=(HPIXELS+HBLANK_MAX)-xOff;
    //clipped_height=312-yOff; //must be even
    clipped_height=(3*clipped_width/4 +(ntsc?0:24) /*32 due to PAL?*/)/2 & 0xfffe;
    if(ntsc){clipped_height-=PAL_EXTRA_VPIXEL;}
  } 
  else if( strcmp(name,"extreme") == 0)
  {
    wrapper->emu->set(Opt::DENISE_VIEWPORT_TRACKING, false); 
  
    geometry=DISPLAY_EXTREME;

    xOff=208-48; //208 is first pixel in dpaint iv,overscan=max
    yOff=ntsc?NTSC::VBLANK_CNT:PAL::VBLANK_CNT; //must be even
    clipped_width=(HPIXELS+HBLANK_MAX)-xOff;
    //clipped_height=312-yOff; //must be even
    if(ntsc)
    {
      clipped_height=(NTSC::VPOS_MAX - yOff) & 0xfffe;
    }
    else
    {
      clipped_height=(PAL::VPOS_MAX - yOff) & 0xfffe;
    }
  }
  
  if(log_on) printf("width=%d, height=%d, ratio=%f\n", clipped_width, clipped_height, (float)clipped_width/(float)clipped_height);

  EM_ASM({js_set_display($0,$1,$2,$3); scaleVMCanvas();},xOff, yOff, clipped_width*TPP,clipped_height );
}

std::unique_ptr<FloppyDisk> load_disk(const char* filename, u8 *blob, long len)
{
  printf("file content starts with %.*s\n",8, blob );

  if (DMSFile::isCompatible(filename)) {
    printf("%s - Loading DMS file\n", filename);
    DMSFile dms{blob, len};
    return std::make_unique<FloppyDisk>(dms);
  }

  if (strcmp((char*)blob, "UAE--ADF")==0 || strcmp((char*)blob, "UAE-1ADF")==0) {
      printf("compatible extadf\n");
      EADFFile ext{blob, len};
      return std::make_unique<FloppyDisk>(ext);
  }

  if (ADFFile::isCompatible(filename)) {
    printf("%s - Loading ADF file\n", filename);
    ADFFile adf{blob, len};
    return std::make_unique<FloppyDisk>(adf);
  }

  if (EXEFile::isCompatible(filename)) {
    printf("%s - Loading EXE file\n", filename);
    EXEFile exe{blob, len};
    return std::make_unique<FloppyDisk>(exe);
  }
  if (STFile::isCompatible(filename)) {
    printf("%s - Loading ST file\n", filename);
    STFile st{blob, len};
    return std::make_unique<FloppyDisk>(st);
  }
  if (OtherFile::isCompatible(filename)) {
    if(len > 1710000)
    { 
      EM_ASM(
      {
        alert(`Error loading ${UTF8ToString($0)} to disk - sorry, only files below 1.71MB can be mounted as disk.`);
      }, filename);
    }
    else {
      printf("%s - import as disk\n", filename);
      OtherFile other{filename,blob, len};
      return std::make_unique<FloppyDisk>(other);
    }
  }
  return {};
}


extern "C" const void wasm_mem_patch(u32 amiga_mem_address, u8 *blob, isize len)
{
  printf("wasm_mem_patch addr=0x%x, len=%ld, header bytes= %x, %x, %x\n", amiga_mem_address, len, blob[0],blob[1],blob[2]);
  if(wrapper == NULL) return;

  wrapper->emu->mem.mem->patch(amiga_mem_address, blob, len);
  return;
}

string
extractSuffix(const string &s)
{
    auto idx = s.rfind('.');
    auto pos = idx != string::npos ? idx + 1 : 0;
    auto len = string::npos;
    return s.substr(pos, len);
}

extern "C" const char* _wasm_loadFile(char* name, u8 *blob, long len, u8 drive_number)
{
  printf("load drive=%d, file=%s len=%ld, header bytes= %x, %x, %x\n", drive_number, name, len, blob[0],blob[1],blob[2]);
  filename=name;
  if(wrapper == NULL)
  {
    return "";
  }
  try{
    if (auto disk = load_disk(name, blob, len)) {
      if(drive_number>0)
      {//configure correct disk drive type (df0 does only accept DD, no HD)
        wrapper->emu->set(Opt::DRIVE_TYPE, (i64)(disk->density==Density::DD? FloppyDriveType::DD_35:FloppyDriveType::HD_35), {drive_number} );
        wrapper->emu->emu->update();
      }

      if(drive_number==0){
        if(disk->density == Density::DD)
        {
          wrapper->emu->df0.drive->swapDisk(std::move(disk));
        }
        else
          EM_ASM(
          {
            let_drive_select_stay_open=true;
            alert(`'${UTF8ToString($0)}' is a HD disk which is not compatible with df0. AmigaOS only supports DD disks in df0, please mount disk in df1 - df3, which beside DD also support HD disks.`);
          }, filename);
      }
      else if(drive_number==1)
        wrapper->emu->df1.drive->swapDisk(std::move(disk));
      else if(drive_number==2)
        wrapper->emu->df2.drive->swapDisk(std::move(disk));
      else if(drive_number==3)
        wrapper->emu->df3.drive->swapDisk(std::move(disk));

      return "";
    }
  } catch (const CoreError& e) {
    printf("Error loading %s - %s\n", filename, e.what());
    EM_ASM(
    {
      alert(`Error loading ${UTF8ToString($0)} - ${UTF8ToString($1)}`);
    }, filename, e.what());    
  }

  if (HDFFile::isCompatible(filename)) 
  {
    printf("is hdf\n");

    HDFFile *hdf;

    try{    
      hdf = new HDFFile(blob, len);
    }
    catch(CoreError &exception) {
      printf("Failed to create HDF image file %s\n", name);
      Fault ec=Fault(exception.data);
      printf("%s - %s\n", FaultEnum::key(ec), exception.what());
      EM_ASM(
      {
        alert(`${UTF8ToString($0)} - ${UTF8ToString($1)}`);
      }, FaultEnum::key(ec), exception.what());
      return FaultEnum::key(ec); 
    }    

    auto hard_drive = wrapper->emu->hd0.drive;
    if(drive_number==1)
    {
      hard_drive = wrapper->emu->hd1.drive;
    }
    else if(drive_number==2)
    {
      hard_drive = wrapper->emu->hd2.drive;
    }
    else if(drive_number==3)
    {
      hard_drive = wrapper->emu->hd3.drive;
    }

    try
    {
        hard_drive->init(*hdf);
        if(!hard_drive->getInfo().hasDisk)
        {
          throw Fault(Fault::OUT_OF_MEMORY);
        }
    }
    catch(CoreError &exception) {
      printf("Failed to init HDF image file %s\n", name);
      EM_ASM(
      {
        alert(`${UTF8ToString($0)}`);
      }, exception.what());
      delete hdf;
      return exception.what();
    }
 
    delete hdf;

    wrapper->emu->powerOff(); wrapper->emu->emu->update();
    wrapper->emu->set(Opt::HDC_CONNECT, true, {drive_number});
    wrapper->emu->emu->update();
    wrapper->emu->powerOn(); wrapper->emu->emu->update();
    return "";
  }
  bool file_still_unprocessed=true;
  if (Snapshot::isCompatible(blob,len) && extractSuffix(filename)!="rom")
  {  
    try
    {
      if(log_on) printf("try to build Snapshot\n");
      Snapshot *file = new Snapshot(blob, len);      
      printf("isSnapshot\n");
      wrapper->emu->amiga.loadSnapshot(*file);
      file_still_unprocessed=false;
      delete file;
//      wasm_set_display(wrapper->emu->agnus.isNTSC()?"ntsc":"pal");

/*      if(geometry==DISPLAY_BORDERLESS || geometry == DISPLAY_ADAPTIVE)
      {//it must determine the viewport again, i.e. we need a new message for calibration
        //enforce this by calling
        wrapper->emu->set(OPT_VIEWPORT_TRACKING, true); 
      }
*/
      printf("run snapshot at %f Hz, isPAL=%d\n", target_fps, !ntsc);
    }
    catch(CoreError &exception) {
      Fault ec=Fault(exception.data);
      printf("%s\n", FaultEnum::key(ec));
    }
  }

  if(file_still_unprocessed && extractSuffix(filename)=="rom_file")
  {
    bool wasRunnable = true;
    try { wrapper->emu->isReady(); } catch(...) { wasRunnable=false; }

    RomFile *rom = NULL;
    try
    {
      printf("try to build RomFile\n");
      rom = new RomFile(blob, len);
    }
    catch(CoreError &exception) {
      printf("Failed to read ROM image file %s\n", name);
      Fault ec=Fault(exception.data);
      printf("%s\n", FaultEnum::key(ec));
      return "";
    }

    if(wrapper->emu->isPoweredOn())
    {
      wrapper->emu->powerOff(); wrapper->emu->emu->update();
    }
//    wrapper->emu->suspend();
    try { 
      wrapper->emu->mem.loadRom(*rom); 
      
      printf("Loaded ROM image %s. %s\n", name, wrapper->emu->mem.getRomTraits().title);
      if(strncmp("EmuTOS",wrapper->emu->mem.getRomTraits().title,strlen("EmuTOS"))==0)
      {
        printf("detected EmuTOS rom, setting drive speed to -1\n");
        wrapper->emu->set(Opt::DC_SPEED, -1);
      }
/*      wrapper->emu->set(OPT_HDC_CONNECT,
        //hd drive
        0, 
        //enable if not AROS
        strcmp(wrapper->emu->mem.romTitle(),"AROS Kickstart replacement")!=0
      );
*/
    }  
    catch(CoreError &exception) { 
      printf("Failed to flash ROM image %s.\n", name);
      Fault ec=Fault(exception.data);
      printf("%s\n", FaultEnum::key(ec));
      return "";
    }
    const char *rom_type="rom";
    try
    {
        bool is_ready_now = true;
        try { wrapper->emu->isReady(); } catch(...) { is_ready_now=false; }

        if (!wasRunnable && is_ready_now)
        {
          printf("was not runnable is ready now (rom)\n");
          wrapper->emu->powerOn();
        
          //wrapper->emu->putMessage(MSG_READY_TO_RUN);
          const char* ready_msg= "READY_TO_RUN";
          printf("sending ready message %s.\n", ready_msg);
          send_message_to_js(ready_msg);    
        }

        delete rom;
        wrapper->emu->powerOn();
//        wrapper->emu->resume();
    }    
    catch(CoreError &exception) { 
      Fault ec=Fault(exception.data);
      printf("%s\n", FaultEnum::key(ec));
    } 

    return rom_type;    
  }
  
  if(file_still_unprocessed && extractSuffix(filename)=="rom_ext_file")
  {
    bool wasRunnable = true;
    try { wrapper->emu->isReady(); } catch(...) { wasRunnable=false; }

    ExtendedRomFile *rom = NULL;
    try
    {
      printf("try to build ExtendedRomFile\n");
      rom = new ExtendedRomFile(blob, len);
    }
    catch(CoreError &exception) {
      printf("Failed to read ROM_EXT image file %s\n", name);
      Fault ec=Fault(exception.data);
      printf("%s\n", FaultEnum::key(ec));
      return "";
    }

    if(wrapper->emu->isPoweredOn())
    {
      wrapper->emu->powerOff(); wrapper->emu->emu->update();
    }
    //wrapper->emu->suspend();
    try { 
      wrapper->emu->mem.loadExt(*rom); 
      
      printf("Loaded ROM_EXT image %s.\n", name);
      
    }  
    catch(CoreError &exception) { 
      printf("Failed to flash ROM_EXT image %s.\n", name);
      Fault ec=Fault(exception.data);
      printf("%s\n", FaultEnum::key(ec));
    }
  

    bool is_ready_now = true;
    try { wrapper->emu->isReady(); } catch(...) { is_ready_now=false; }

    if (!wasRunnable && is_ready_now)
    {
      printf("was not runnable is ready now (romext)\n");
      wrapper->emu->powerOn();

       //wrapper->emu->putMessage(MSG_READY_TO_RUN);
      const char* ready_msg= "READY_TO_RUN";
      printf("sending ready message %s.\n", ready_msg);
      send_message_to_js(ready_msg);    
    }

    const char *rom_type="rom_ext";
    delete rom;
    //wrapper->emu->resume();
    wrapper->emu->powerOn();
    return rom_type;    
  }

  return "";
}


extern "C" const char* wasm_loadFile(char* name, u8 *blob, long len, u8 drive_number)
{
  try
  {
    return _wasm_loadFile(name, blob, len, drive_number);
  }
  catch (const CoreError& e) {
    EM_ASM(
    {
      alert(`Error loading ${UTF8ToString($0)} - ${UTF8ToString($1)}`);
    }, name, e.what());    
  }
  return "";
}

extern "C" void wasm_reset()
{
  wrapper->emu->hardReset();
}


extern "C" void wasm_halt()
{
  printf("wasm_halt\n");
  wrapper->emu->pause();

//  printf("emscripten_pause_main_loop() at MSG_PAUSE\n");
//  paused_the_emscripten_main_loop=true;
  //emscripten_pause_main_loop();
//  printf("after emscripten_set_main_loop_arg() at MSG_RUN\n");

}

extern "C" void wasm_run()
{
  if(log_on) printf("wasm_run\n");
  
  if(log_on) printf("is running = %u\n",wrapper->emu->isRunning());

  wrapper->emu->run();
  emu=wrapper->emu;
/*
  if(paused_the_emscripten_main_loop || already_run_the_emscripten_main_loop)
  {
    if(log_on) printf("emscripten_resume_main_loop at MSG_RUN %u, %u\n", paused_the_emscripten_main_loop, already_run_the_emscripten_main_loop);
//    emscripten_resume_main_loop();
  }
  else
  {
    if(log_on) printf("emscripten_set_main_loop_arg() at MSG_RUN %u, %u\n", paused_the_emscripten_main_loop, already_run_the_emscripten_main_loop);
    already_run_the_emscripten_main_loop=true;
    thisAmiga=wrapper->emu;
    //emscripten_set_main_loop_arg(draw_one_frame_into_SDL, (void *)wrapper->emu, 0, 1);
    if(log_on) printf("after emscripten_set_main_loop_arg() at MSG_RUN\n");
  }
*/
}


extern "C" void wasm_mouse(int port, int x, int y)
{
  //printf("wasm_mouse port%d x=%d, y=%d\n", port, x, y);

  /*if(port==1)
    wrapper->emu->controlPort1.mouse->setDxDy(x,y); 
  else if(port==2)
    wrapper->emu->controlPort2.mouse->setDxDy(x,y);
  */
  wrapper->emu->put(Cmd::MOUSE_MOVE_REL, CoordCommand(port-1, x, y));
}

extern "C" void wasm_mouse_button(int port, int button_id, int pressed)
{ 
    if(button_id==1)
      wrapper->emu->put(Cmd::MOUSE_BUTTON, GamePadCommand(port-1, (pressed==1?GamePadAction::PRESS_LEFT:GamePadAction::RELEASE_LEFT)));
      //wrapper->emu->put(CMD_MOUSE_EVENT, GamePadCmd(port-1,(pressed==1?PRESS_LEFT:RELEASE_LEFT)));
    else if(button_id==2)
    wrapper->emu->put(Cmd::MOUSE_BUTTON, GamePadCommand(port-1, (pressed==1?GamePadAction::PRESS_MIDDLE:GamePadAction::RELEASE_MIDDLE)));
    //      wrapper->emu->put(CMD_MOUSE_EVENT, GamePadCmd(port-1,(pressed==1?PRESS_MIDDLE:RELEASE_MIDDLE)));
    else if(button_id==3)
    //      wrapper->emu->put(CMD_MOUSE_EVENT, GamePadCmd(port-1,(pressed==1?PRESS_RIGHT:RELEASE_RIGHT)));
    wrapper->emu->put(Cmd::MOUSE_BUTTON, GamePadCommand(port-1, (pressed==1?GamePadAction::PRESS_RIGHT:GamePadAction::RELEASE_RIGHT)));

}

extern "C" void wasm_joystick(char* port_plus_event)
{
//    printf("wasm_joystick event=%s\n", port_plus_event);
  /*
  from javascript
    // port, o == oben, r == rechts, ..., f == feuer
    //states = '1', '1o', '1or', '1r', '1ur', '1u', '1ul', '1l', '1ol' 
    //states mit feuer = '1f', '1of', '1orf', '1rf', '1urf', ...
  */
/*
outgoing
PULL_UP
PRESS_FIRE

RELEASE_X
RELEASE_Y
RELEASE_XY
RELEASE_FIRE
*/
  char joyport = port_plus_event[0];
  char* event  = port_plus_event+1;

  GamePadAction code;
  if( strcmp(event,"PULL_UP") == 0)
  {
    code = GamePadAction::PULL_UP;
  }
  else if( strcmp(event,"PULL_LEFT") == 0)
  {
    code = GamePadAction::PULL_LEFT;
  }
  else if( strcmp(event,"PULL_DOWN") == 0)
  {
    code = GamePadAction::PULL_DOWN;
  }
  else if( strcmp(event,"PULL_RIGHT") == 0)
  {
    code = GamePadAction::PULL_RIGHT;
  }
  else if( strcmp(event,"PRESS_FIRE") == 0)
  {
    code = GamePadAction::PRESS_FIRE;
  }
  else if( strcmp(event,"PRESS_FIRE2") == 0)
  {
    code = GamePadAction::PRESS_FIRE2;
  }
  else if( strcmp(event,"PRESS_FIRE3") == 0)
  {
    code = GamePadAction::PRESS_FIRE3;
  }
  else if( strcmp(event,"RELEASE_XY") == 0)
  {
    code = GamePadAction::RELEASE_XY;
  }
  else if( strcmp(event,"RELEASE_X") == 0)
  {
    code = GamePadAction::RELEASE_X;
  }
  else if( strcmp(event,"RELEASE_Y") == 0)
  {
    code = GamePadAction::RELEASE_Y;
  }
  else if( strcmp(event,"RELEASE_FIRE") == 0)
  {
    code = GamePadAction::RELEASE_FIRE;
  }
  else if( strcmp(event,"RELEASE_FIRE2") == 0)
  {
    code = GamePadAction::RELEASE_FIRE2;
  }
  else if( strcmp(event,"RELEASE_FIRE3") == 0)
  {
    code = GamePadAction::RELEASE_FIRE3;
  }
  else
  {
    return;    
  }

  if(joyport == '1')
  {
    wrapper->emu->controlPort1.joystick.trigger(code);
  }
  else if(joyport == '2')
  {
    wrapper->emu->controlPort2.joystick.trigger(code);
  }

}

char buffer[50];
extern "C" char* wasm_sprite_info()
{
  if(!wrapper->emu->isTracking())
  {
    wrapper->emu->trackOn();
  }
//   wrapper->emu->setInspectionTarget(INSPECTION_DENISE, MSEC(250));
//   wrapper->emu->denise.debugger.recordSprite(0);

   Denise *denise = wrapper->emu->denise.denise;
   auto spriteinfo0 = denise->debugger.getSpriteInfo(0);
   auto spriteinfo1 = denise->debugger.getSpriteInfo(1);
   auto spriteinfo2 = denise->debugger.getSpriteInfo(2);
   auto spriteinfo3 = denise->debugger.getSpriteInfo(3);
   auto spriteinfo4 = denise->debugger.getSpriteInfo(4);
   auto spriteinfo5 = denise->debugger.getSpriteInfo(5);
   auto spriteinfo6 = denise->debugger.getSpriteInfo(6);
   auto spriteinfo7 = denise->debugger.getSpriteInfo(7);


   sprintf(buffer, "%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu", 
     spriteinfo0.hstrt*2,
     spriteinfo0.vstrt, 
     spriteinfo1.hstrt*2,
     spriteinfo1.vstrt, 
     spriteinfo2.hstrt*2,
     spriteinfo2.vstrt, 
     spriteinfo3.hstrt*2,
     spriteinfo3.vstrt, 
     spriteinfo4.hstrt*2,
     spriteinfo4.vstrt, 
     spriteinfo5.hstrt*2,
     spriteinfo5.vstrt, 
     spriteinfo6.hstrt*2,
     spriteinfo6.vstrt, 
     spriteinfo7.hstrt*2,
     spriteinfo7.vstrt
     );  

   return buffer;
}


extern "C" void wasm_cut_layers(unsigned cut_layers)
{
  wrapper->emu->set(Opt::DENISE_HIDDEN_LAYER_ALPHA,255);
//  wrapper->emu->set(OPT_HIDDEN_SPRITES, 0x100 | (SPR0|SPR1|SPR2|SPR3|SPR4|SPR5|SPR6|SPR7)); 
  wrapper->emu->set(Opt::DENISE_HIDDEN_LAYERS, cut_layers); 
}



char json_result[1024];
extern "C" const char* wasm_rom_info()
{

  sprintf(json_result, "{\"hasRom\":\"%s\",\"hasExt\":\"%s\", \"romTitle\":\"%s\", \"romVersion\":\"%s\", \"romReleased\":\"%s\", \"romModel\":\"%s\", \"extTitle\":\"%s\", \"extVersion\":\"%s\", \"extReleased\":\"%s\", \"extModel\":\"%s\" }",
    wrapper->emu->mem.mem->hasRom()?"true":"false",
    wrapper->emu->mem.mem->hasExt()?"true":"false",
    wrapper->emu->mem.getRomTraits().title,
    wrapper->emu->mem.getRomTraits().revision,
    wrapper->emu->mem.getRomTraits().released,
    wrapper->emu->mem.getRomTraits().model,
    wrapper->emu->mem.getExtTraits().title,
    wrapper->emu->mem.getExtTraits().revision,
    wrapper->emu->mem.getExtTraits().released,
    wrapper->emu->mem.getExtTraits().model
  );

/*
  printf("%s, %s, %s, %s\n",      wrapper->emu->mem.romTitle(),
    wrapper->emu->mem.romVersion(),
    wrapper->emu->mem.romReleased(),
    ""
//    wrapper->emu->mem.romModel()
  );
*/
  return json_result;
}

extern "C" const char* wasm_get_core_version()
{
  sprintf(json_result, "%s",
    wrapper->emu->version().c_str() 
  );

  return json_result;
}



extern "C" void wasm_set_color_palette(char* palette)
{

  if( strcmp(palette,"color") == 0)
  {
    wrapper->emu->set(Opt::MON_PALETTE, (i64)Palette::COLOR);
  }
  else if( strcmp(palette,"black white") == 0)
  { 
    wrapper->emu->set(Opt::MON_PALETTE, (i64)Palette::BLACK_WHITE); 
  }
  else if( strcmp(palette,"paper white") == 0)
  { 
    wrapper->emu->set(Opt::MON_PALETTE, (i64)Palette::PAPER_WHITE); 
  }
  else if( strcmp(palette,"green") == 0)
  { 
    wrapper->emu->set(Opt::MON_PALETTE, (i64)Palette::GREEN); 
  }
  else if( strcmp(palette,"amber") == 0)
  { 
    wrapper->emu->set(Opt::MON_PALETTE, (i64)Palette::AMBER); 
  }
  else if( strcmp(palette,"sepia") == 0)
  { 
    wrapper->emu->set(Opt::MON_PALETTE, (i64)Palette::SEPIA); 
  }

}


extern "C" u64 wasm_get_cpu_cycles()
{
  return wrapper->emu->cpu.cpu->getClock();
}

char config_result[512];
extern "C" const char* wasm_power_on(unsigned power_on)
{
  try{
    bool was_powered_on=wrapper->emu->isPoweredOn();
    if(power_on == 1 && !was_powered_on)
    {
        wrapper->emu->powerOn();
    }
    else if(power_on == 0 && was_powered_on)
    {
        wrapper->emu->powerOff();wrapper->emu->emu->update();
    }
  }  
  catch(CoreError &exception) {   
    sprintf(config_result,"%s", exception.what());
  }
  return config_result; 
}


extern "C" void wasm_set_sample_rate(unsigned sample_rate)
{
    printf("set paula.muxer to freq= %d\n", sample_rate);

    wrapper->emu->set(Opt::HOST_SAMPLE_RATE,sample_rate);
    wrapper->emu->emu->update();
    auto got_sample_rate=wrapper->emu->get(Opt::HOST_SAMPLE_RATE);

    printf("amiga.host.getSampleRate()==%lld\n", got_sample_rate);
}



extern "C" i32 wasm_get_config_item(char* item_name, unsigned data)
{  
  if(strcmp(item_name,"DRIVE_CONNECT") == 0 )
  {
    return wrapper->emu->get(Opt::DRIVE_CONNECT,data);
  }
  else
  {
    return wrapper->emu->get(Opt(util::parseEnum <OptEnum>(std::string(item_name))));
  }
}

extern "C" const char* wasm_configure_key(char* option, char* key, char* _value)
{
 // printf("----->wasm_configure_key %s %s = %s\n", option, key, _value);
 // return config_result;
  sprintf(config_result,""); 
  auto value = std::string(_value);
  if(log_on) printf("wasm_configure_key %s %s = %s\n", option, key, value.c_str());

  try {
/*
    setFallback(OPT_DMA_DEBUG_ENABLE, false);
    setFallback(OPT_DMA_DEBUG_MODE, DMA_DISPLAY_MODE_FG_LAYER);
    setFallback(OPT_DMA_DEBUG_OPACITY, 50);
    setFallback(OPT_DMA_DEBUG_CHANNEL, DMA_CHANNEL_COPPER, true);
    setFallback(OPT_DMA_DEBUG_CHANNEL, DMA_CHANNEL_BLITTER, true);
    setFallback(OPT_DMA_DEBUG_CHANNEL, DMA_CHANNEL_DISK, true);
    setFallback(OPT_DMA_DEBUG_CHANNEL, DMA_CHANNEL_AUDIO, true);
    setFallback(OPT_DMA_DEBUG_CHANNEL, DMA_CHANNEL_SPRITE, true);
    setFallback(OPT_DMA_DEBUG_CHANNEL, DMA_CHANNEL_BITPLANE, true);
    setFallback(OPT_DMA_DEBUG_CHANNEL, DMA_CHANNEL_CPU, false);
    setFallback(OPT_DMA_DEBUG_CHANNEL, DMA_CHANNEL_REFRESH, true);
    setFallback(OPT_DMA_DEBUG_COLOR, DMA_CHANNEL_COPPER, 0xFFFF0000);
    setFallback(OPT_DMA_DEBUG_COLOR, DMA_CHANNEL_BLITTER, 0xFFCC0000);
    setFallback(OPT_DMA_DEBUG_COLOR, DMA_CHANNEL_DISK, 0x00FF0000);
    setFallback(OPT_DMA_DEBUG_COLOR, DMA_CHANNEL_AUDIO, 0xFF00FF00);
    setFallback(OPT_DMA_DEBUG_COLOR, DMA_CHANNEL_SPRITE, 0x0088FF00);
    setFallback(OPT_DMA_DEBUG_COLOR, DMA_CHANNEL_BITPLANE, 0x00FFFF00);
    setFallback(OPT_DMA_DEBUG_COLOR, DMA_CHANNEL_CPU, 0xFFFFFF00);
    setFallback(OPT_DMA_DEBUG_COLOR, DMA_CHANNEL_REFRESH, 0xFF000000);
*/
        wrapper->emu->set(Opt::DMA_DEBUG_ENABLE, true);   
        wrapper->emu->set(
         //OPT_DMA_DEBUG_CHANNEL6,
         Opt(util::parseEnum <OptEnum>(std::string(option))),
//         util::parseEnum <DmaChannelEnum>(std::string(key)), 
         util::parseBool(std::string(key))); 
      
  }
  catch(CoreError &exception) {
      printf("unknown key %s %s = %s\n", option, key, value.c_str());

//    ErrorCode ec=exception.data;
//    sprintf(config_result,"%s", ErrorCodeEnum::key(ec));
    sprintf(config_result,"%s", exception.what());
  }
  return config_result; 
}



void calibrate_boost(signed boost_param){
      if(boost_param >4)
        return;
      vsync_speed=boost_param;
      unsigned boost = boost_param<0 ?
        host_refresh_rate / (boost_param*-1)
        :
        host_refresh_rate * (boost_param);
      vframes=0;
      speed_boost= ((double)boost / target_fps /*which is PAL_FPS or NTSC_FPS */);
      unsigned speed_boost_param=(unsigned)(speed_boost*100);
      printf("host_refresh_rate=%d, boost=%d, speed_boost=%lf, speed_param=%d\n",host_refresh_rate, boost, speed_boost, speed_boost_param);
      
      wrapper->emu->set(Opt::AMIGA_SPEED_BOOST,speed_boost_param );
      
      EM_ASM({$("#host_fps").html(`${$0} Hz`)},
        vsync_speed <0 ? host_refresh_rate/(vsync_speed*-1) : host_refresh_rate*vsync_speed );
}

extern "C" const char* wasm_configure(char* option, char* _value)
{  
  sprintf(config_result,""); 
  auto value = std::string(_value);
  if(log_on) printf("wasm_configure %s = %s\n", option, value.c_str());

  if(strcmp(option,"warp_to_frame") == 0 )
  {
    auto warp_to_frame= util::parseNum(value);
    wrapper->emu->set(Opt::AMIGA_WARP_BOOT, warp_to_frame/(wrapper->emu->agnus.agnus->isPAL()?50:60));
    wrapper->emu->emu->update();
    wrapper->emu->softReset(); //agnus.reset() schedules warp_off therefore we have to reset here after changing warp_boot 
    return config_result;
  }
  else if(strcmp(option,"log_on") == 0 )
  {
    log_on= util::parseBool(value);
    return config_result;
  }
  else if(strcmp(option,"floppy_drive_count") == 0 )
  {
    auto df_count= util::parseNum(value);
    int i=0;
    while(i<df_count)
    {
      wrapper->emu->set(Opt::DRIVE_CONNECT, true, {i});
      i++;
    }
    while(i<4)
    {
      wrapper->emu->set(Opt::DRIVE_CONNECT, false, {i});
      i++;
    }
    wrapper->emu->emu->update();
    return config_result;
  }

  bool was_powered_on=wrapper->emu->isPoweredOn();

  bool must_be_off= strcmp(option,"AGNUS_REVISION") == 0 || 
                    strcmp(option,"DENISE_REVISION") == 0 ||
                    strcmp(option,"CHIP_RAM") == 0 ||
                    strcmp(option,"SLOW_RAM") == 0 ||
                    strcmp(option,"FAST_RAM") == 0 ||
                    strcmp(option,"CPU_REVISION") == 0;
 
  if(was_powered_on && must_be_off)
  {
      wrapper->emu->powerOff();wrapper->emu->emu->update();
  }

  try{
    if( strcmp(option,"AGNUS_REVISION") == 0)
    {
      wrapper->emu->set(Opt::AGNUS_REVISION, util::parseEnum <AgnusRevisionEnum>(value)); 
    }
    else if( strcmp(option,"DENISE_REVISION") == 0)
    {
      wrapper->emu->set(Opt::DENISE_REVISION, util::parseEnum <DeniseRevEnum>(value));
    }
    else if( strcmp(option,"WARP_MODE") == 0)
    {
      wrapper->emu->set(Opt::AMIGA_WARP_MODE, util::parseEnum <WarpEnum>(
        value.size() > 5 && value.substr(0, 5) == "WARP_" ? 
        value.substr(5) //legacy: some websites still use the WARP_ prefix variant 
        : 
        value
      ));
    }
    else if( strcmp(option,"SER_DEVICE") == 0)
    {
      wrapper->emu->set(Opt(util::parseEnum <OptEnum>(std::string(option))), util::parseEnum<SerialPortDeviceEnum>(value));
    }
    else if ( strcmp(option,"BLITTER_ACCURACY") == 0 ||
              strcmp(option,"DRIVE_SPEED") == 0  ||
              strcmp(option,"CHIP_RAM") == 0  ||
              strcmp(option,"SLOW_RAM") == 0  ||
              strcmp(option,"FAST_RAM") == 0  ||
              strcmp(option,"CPU_OVERCLOCKING") == 0 ||
              strcmp(option,"CPU_REVISION") == 0
    )
    {
      if(strcmp(option,"BLITTER_ACCURACY") == 0)
      {//TODO kann nicht so bleiben
        wrapper->emu->set(Opt::BLITTER_ACCURACY, util::parseNum(value));
      }
      else if(strcmp(option,"DRIVE_SPEED") == 0)
      {//TODO kann nicht so bleiben
        wrapper->emu->set(Opt::DC_SPEED, util::parseNum(value));
      }
      else if(strcmp(option,"CPU_REVISION") == 0)
      {//TODO kann nicht so bleiben
        wrapper->emu->set(Opt::CPU_REVISION, util::parseNum(value));
      }
      else if(strcmp(option,"CPU_OVERCLOCKING") == 0)
      {//TODO kann nicht so bleiben
        wrapper->emu->set(Opt::CPU_OVERCLOCKING, util::parseNum(value));
      }
      else
        wrapper->emu->set(Opt(util::parseEnum <OptEnum>(std::string(option))), util::parseNum(value));
    }
    else if ( strcmp(option,"DMA_DEBUG_CHANNEL") == 0 )
    {
//todo
      wrapper->emu->set(Opt(util::parseEnum <OptEnum>(std::string(option))),  util::parseBool(value));
    }
    else if(strcmp(option,"OPT_EMU_RUN_AHEAD") == 0)
    {
      auto frames=util::parseNum(value);
      printf("calling amiga->configure %s = %ld\n", option, frames);
      wrapper->emu->set(Opt::AMIGA_RUN_AHEAD, frames);
    }
    else if( strcmp(option,"OPT_AMIGA_SPEED_BOOST") == 0)
    {
      boost_param=(signed) util::parseNum(value);
      /* setting
        sync mode: { vsync x1/4=-4, ..., vsync=1, vsync x2=2, 50%=50, 75%=75, 100%, 150%, 200% }
      */
      if(boost_param <= 4)
      {
        vsync=true;
        calibrate_boost(boost_param);
      }
      else
      {
        vsync=false;
        wrapper->emu->set(Opt::AMIGA_SPEED_BOOST, boost_param);
        speed_boost= ((double) boost_param) / 100.0;
      }
      requested_targetFrameCount_reset=true; 
    }
    else
    {
      wrapper->emu->set(Opt(util::parseEnum <OptEnum>(std::string(option))), util::parseBool(value)); 
      wrapper->emu->emu->update();
    }

    if(was_powered_on && must_be_off)
    {
        wrapper->emu->powerOn();
    }
  }
  catch(CoreError &exception) {    
//    ErrorCode ec=exception.data;
//    sprintf(config_result,"%s", ErrorCodeEnum::key(ec));
    printf("unknown key wasm_configure %s = %s\n", option, value.c_str());

    sprintf(config_result,"%s", exception.what());
  }
  return config_result; 
}
extern "C" void wasm_print_error(unsigned exception_ptr)
{
  if(exception_ptr!=0)
  {
    string s= std::string(reinterpret_cast<std::exception *>(exception_ptr)->what());
    printf("uncaught exception %u: %s\n",exception_ptr, s.c_str());
  }
}


Buffer<float> leftChannel;
extern "C" u32 wasm_leftChannelBuffer()
{
    if (leftChannel.size == 0)
        leftChannel.init(2048, 0);
    return (u32)leftChannel.ptr;
}

Buffer<float> rightChannel;
extern "C" u32 wasm_rightChannelBuffer()
{
    if (rightChannel.size == 0)
        rightChannel.init(2048, 0);
    return (u32)rightChannel.ptr;
}
extern "C" void wasm_update_audio(int offset)
{
//    assert(offset == 0 || offset == leftChannel.size / 2);

  float *left = leftChannel.ptr + offset;
  float *right = rightChannel.ptr + offset;
  auto samples = leftChannel.size / 2;
  wrapper->emu->audioPort.copyStereo(left, right, samples);
  sum_samples += samples; 
}

extern "C" void wasm_write_string_to_ser(char* chars_to_send)
{
    if(wrapper->emu->agnus.agnus->id[SLOT_SER] != SER_RECEIVE)
    {
      wrapper->emu->remoteManager.remoteManager->serServer.didConnect();
    }
    wrapper->emu->remoteManager.remoteManager->serServer.doProcess(chars_to_send);
}

/*extern "C" void wasm_write_bytes_to_ser(u8 *bytes_to_send, u32 length)
{
    if(wrapper->emu->agnus.id[SLOT_SER] != SER_RECEIVE)
    {
      wrapper->emu->remoteManager.serServer.didConnect();
    }
    auto &serserver = wrapper->emu->remoteManager.serServer;
    for (u32 i=0; i<length; i++) { 
      serserver.processIncomingByte((u8)bytes_to_send[i]); 
    }
}*/

extern "C" void wasm_write_byte_to_ser(u8 byte_to_send)
{
    if(wrapper->emu->agnus.agnus->id[SLOT_SER] != SER_RECEIVE)
    {
      wrapper->emu->remoteManager.remoteManager->serServer.didConnect();
    }
    wrapper->emu->remoteManager.remoteManager->serServer.processIncomingByte(byte_to_send);
}

extern "C" double wasm_activity(u8 id, u8 read_or_write)
{
    double value=0.0;
    auto dma = wrapper->emu->agnus.getStats();
    if(id==0)
      value= dma.copperActivity /(313 *120);
    else if(id==1)
      value= dma.blitterActivity /(313 *120);
    else if(id==2)
      value= dma.diskActivity /(313 *3);
    else if(id==3)
      value= dma.audioActivity /(313 *4);
    else if(id==4)
      value= dma.spriteActivity /(313 *16);
    else if(id==5)
      value= dma.bitplaneActivity /(39330);
    else if(id==6)
    {
      /* let mem = amiga.mem.getStats()
        let max = Float((HPOS_CNT_PAL * VPOS_CNT) / 2)
        let chipR = Float(mem.chipReads.accumulated) / max
        let chipW = Float(mem.chipWrites.accumulated) / max
        let slowR = Float(mem.slowReads.accumulated) / max
        let slowW = Float(mem.slowWrites.accumulated) / max
        let fastR = Float(mem.fastReads.accumulated) / max
        let fastW = Float(mem.fastWrites.accumulated) / max
        let kickR = Float(mem.kickReads.accumulated) / max
        let kickW = Float(mem.kickWrites.accumulated) / max
        
        addValues(Monitors.Monitor.chipRam, chipR, chipW)
        addValues(Monitors.Monitor.slowRam, slowR, slowW)
        addValues(Monitors.Monitor.fastRam, fastR, fastW)
        addValues(Monitors.Monitor.kickRom, kickR, kickW)*/
        auto mem = wrapper->emu->mem.getStats();
        auto max = float(PAL::HPOS_CNT * VPOS_CNT) / 2.0;
        value = float(
          read_or_write == 0 ? mem.chipReads.accumulated : mem.chipWrites.accumulated
          )  / max;
    }
    else if(id==7)
    {
        auto mem = wrapper->emu->mem.getStats();
        auto max = float(PAL::HPOS_CNT * VPOS_CNT) / 2.0;
        value= float(
          read_or_write == 0 ? mem.slowReads.accumulated : mem.slowWrites.accumulated
          )  / max;
    }
    else if(id==8)
    {
        auto mem = wrapper->emu->mem.getStats();
        auto max = float(PAL::HPOS_CNT * VPOS_CNT) / 2.0;
        value = float(
          read_or_write == 0 ? mem.fastReads.accumulated : mem.fastWrites.accumulated
          )  / max;
    }
    else if(id==9)
    {
        auto mem = wrapper->emu->mem.getStats();
        auto max = float(PAL::HPOS_CNT * VPOS_CNT) / 2.0;
        value = float(
          read_or_write == 0 ? mem.kickReads.accumulated : mem.kickWrites.accumulated
          )  / max;
    }

  //  printf("activity_id: %u, rw: %u =%lf\n",id, read_or_write,value);

    return value;
}


Thumbnail preview;
extern "C" char* wasm_save_workspace(char* path)
{
  try{
    //save with DMA_DEBUG_ENABLE=false otherwise vAmiga.app for macOS would trigger minimized debug screen 
    auto debug_enable = wrapper->emu->get(Opt::DMA_DEBUG_ENABLE);
    wrapper->emu->set(Opt::DMA_DEBUG_ENABLE,false);
    wrapper->emu->emu->update();

    wrapper->emu->amiga.saveWorkspace(path);

    wrapper->emu->set(Opt::DMA_DEBUG_ENABLE,debug_enable);
    wrapper->emu->emu->update();
  }
  catch (const CoreError& e) {
    printf("Error %s\n", e.what());
    EM_ASM(
    {
      alert(`Error - ${UTF8ToString($0)}`);
    }, e.what());    
  }

  preview.take(*(wrapper->emu->amiga.amiga));

  sprintf(wasm_pull_user_snapshot_file_json_result, "{\"address\":%lu, \"size\": %u, \"width\": %d, \"height\":%d }",
    (unsigned long)preview.screen, 
    preview.width*preview.height*4,
    preview.width,
    preview.height
    );
  return wasm_pull_user_snapshot_file_json_result;
}

extern "C" void wasm_load_workspace(char* path)
{
  try{
    //don't respect the DMA_DEBUG_ENABLE setting in the workspace file
    //instead keep current user choice
    auto debug_enable = wrapper->emu->get(Opt::DMA_DEBUG_ENABLE);
    auto channel0 = wrapper->emu->get(Opt::DMA_DEBUG_CHANNEL0);
    auto channel1 = wrapper->emu->get(Opt::DMA_DEBUG_CHANNEL1);
    auto channel2 = wrapper->emu->get(Opt::DMA_DEBUG_CHANNEL2);
    auto channel3 = wrapper->emu->get(Opt::DMA_DEBUG_CHANNEL3);
    auto channel4 = wrapper->emu->get(Opt::DMA_DEBUG_CHANNEL4);
    auto channel5 = wrapper->emu->get(Opt::DMA_DEBUG_CHANNEL5);
    auto channel6 = wrapper->emu->get(Opt::DMA_DEBUG_CHANNEL6);
    auto channel7 = wrapper->emu->get(Opt::DMA_DEBUG_CHANNEL7);

    wrapper->emu->amiga.loadWorkspace(path);

    wrapper->emu->set(Opt::DMA_DEBUG_ENABLE,debug_enable);
    wrapper->emu->set(Opt::DMA_DEBUG_CHANNEL0,channel0);
    wrapper->emu->set(Opt::DMA_DEBUG_CHANNEL1,channel1);
    wrapper->emu->set(Opt::DMA_DEBUG_CHANNEL2,channel2);
    wrapper->emu->set(Opt::DMA_DEBUG_CHANNEL3,channel3);
    wrapper->emu->set(Opt::DMA_DEBUG_CHANNEL4,channel4);
    wrapper->emu->set(Opt::DMA_DEBUG_CHANNEL5,channel5);
    wrapper->emu->set(Opt::DMA_DEBUG_CHANNEL6,channel6);
    wrapper->emu->set(Opt::DMA_DEBUG_CHANNEL7,channel7);
    wrapper->emu->emu->update();

  }
  catch (const CoreError& e) {
    printf("Error %s\n", e.what());
    EM_ASM(
    {
      alert(`Error - ${UTF8ToString($0)}`);
    }, e.what());    
  }
}

extern "C" void wasm_retro_shell(char* cmd)
{
  wrapper->emu->retroShell.execScript(cmd);  
}
