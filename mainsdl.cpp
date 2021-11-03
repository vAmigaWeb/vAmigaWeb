/*!
 * @file        mainsdl.cpp
 * @author      mithrendal and Dirk W. Hoffmann, www.dirkwhoffmann.de
 * @copyright   Dirk W. Hoffmann. All rights reserved.
 */

#include <stdio.h>
#include "config.h"
#include "Amiga.h"
#include "AmigaTypes.h"
#include "RomFile.h"
#include "ADFFile.h"
#include "DMSFile.h"
#include "EXEFile.h"
#include "Snapshot.h"

#include "MemUtils.h"


#include <emscripten.h>
#include <SDL2/SDL.h>
#include <emscripten/html5.h>

/* SDL2 start*/
SDL_Window * window = NULL;
SDL_Surface * window_surface = NULL;
unsigned int * pixels;

SDL_Renderer * renderer = NULL;
SDL_Texture * screen_texture = NULL;

/* SDL2 end */

void PrintEvent(const SDL_Event * event)
{
    if (event->type == SDL_WINDOWEVENT) {
        switch (event->window.event) {
        case SDL_WINDOWEVENT_SHOWN:
            printf("Window %d shown", event->window.windowID);
            break;
        case SDL_WINDOWEVENT_HIDDEN:
            printf("Window %d hidden", event->window.windowID);
            break;
        case SDL_WINDOWEVENT_EXPOSED:
            printf("Window %d exposed", event->window.windowID);
            break;
        case SDL_WINDOWEVENT_MOVED:
            printf("Window %d moved to %d,%d",
                    event->window.windowID, event->window.data1,
                    event->window.data2);
            break;
        case SDL_WINDOWEVENT_RESIZED:
            printf("Window %d resized to %dx%d",
                    event->window.windowID, event->window.data1,
                    event->window.data2);
            break;
        case SDL_WINDOWEVENT_SIZE_CHANGED:
            printf("Window %d size changed to %dx%d",
                    event->window.windowID, event->window.data1,
                    event->window.data2);
            break;
        case SDL_WINDOWEVENT_MINIMIZED:
            printf("Window %d minimized", event->window.windowID);
            break;
        case SDL_WINDOWEVENT_MAXIMIZED:
            printf("Window %d maximized", event->window.windowID);
            break;
        case SDL_WINDOWEVENT_RESTORED:
            printf("Window %d restored", event->window.windowID);
            break;
        case SDL_WINDOWEVENT_ENTER:
            printf("Mouse entered window %d",
                    event->window.windowID);
            break;
        case SDL_WINDOWEVENT_LEAVE:
            printf("Mouse left window %d", event->window.windowID);
            break;
        case SDL_WINDOWEVENT_FOCUS_GAINED:
            printf("Window %d gained keyboard focus",
                    event->window.windowID);
            break;
        case SDL_WINDOWEVENT_FOCUS_LOST:
            printf("Window %d lost keyboard focus",
                    event->window.windowID);
            break;
        case SDL_WINDOWEVENT_CLOSE:
            printf("Window %d closed", event->window.windowID);
            break;
#if SDL_VERSION_ATLEAST(2, 0, 5)
        case SDL_WINDOWEVENT_TAKE_FOCUS:
            printf("Window %d is offered a focus", event->window.windowID);
            break;
        case SDL_WINDOWEVENT_HIT_TEST:
            printf("Window %d has a special hit test", event->window.windowID);
            break;
#endif
        default:
            printf("Window %d got unknown event %d",
                    event->window.windowID, event->window.event);
            break;
        }
        printf("\n");
    }
}

int emu_width  = HPIXELS; //TEX_WIDTH; //NTSC_PIXELS; //428
int emu_height = VPIXELS; //PAL_RASTERLINES; //284
int eat_border_width = 0;
int eat_border_height = 0;
int xOff = 12 + eat_border_width;
int yOff = 12 + eat_border_height;
int clipped_width  = HPIXELS -12 -24 -2*eat_border_width; //392
int clipped_height = VPIXELS -12 -24 -2*eat_border_height; //248

int bFullscreen = false;


EM_BOOL emscripten_window_resized_callback(int eventType, const void *reserved, void *userData){
/*
	double width, height;
	emscripten_get_element_css_size("canvas", &width, &height);
	int w = (int)width, h = (int)height;
*/
  // resize SDL window
    SDL_SetWindowSize(window, clipped_width, clipped_height);
    /*
    SDL_Rect SrcR;
    SrcR.x = 0;
    SrcR.y = 0;
    SrcR.w = emu_width;
    SrcR.h = emu_height;
    SDL_RenderSetViewport(renderer, &SrcR);
    */
	return true;
}

char *filename = NULL;

extern "C" void wasm_toggleFullscreen()
{
    if(!bFullscreen)
    {
      bFullscreen=true;

      EmscriptenFullscreenStrategy strategy;
      strategy.scaleMode = EMSCRIPTEN_FULLSCREEN_CANVAS_SCALE_STDDEF;
      strategy.filteringMode = EMSCRIPTEN_FULLSCREEN_FILTERING_DEFAULT;
      strategy.canvasResizedCallback = emscripten_window_resized_callback;
      emscripten_enter_soft_fullscreen("canvas", &strategy);    
    }
    else
    {
      bFullscreen=false;
      emscripten_exit_soft_fullscreen(); 
    }
}

int eventFilter(void* thisC64, SDL_Event* event) {
    //C64 *c64 = (C64 *)thisC64;
    switch(event->type){
      case SDL_WINDOWEVENT:
        PrintEvent(event);
        if (event->window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
        {//zuerst
            window_surface = SDL_GetWindowSurface(window);
            pixels = (unsigned int *)window_surface->pixels;
            int width = window_surface->w;
            int height = window_surface->h;
            printf("Size changed: %d, %d\n", width, height);  
        }
        else if(event->window.event==SDL_WINDOWEVENT_RESIZED)
        {//this event comes after SDL_WINDOWEVENT_SIZE_CHANGED
              //SDL_SetWindowSize(window, emu_width, emu_height);   
              //SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
              //window_surface = SDL_GetWindowSurface(window);
        }
        break;
      case SDL_KEYDOWN:
        if ( event->key.keysym.sym == SDLK_RETURN &&
             event->key.keysym.mod & KMOD_ALT )
        {
            wasm_toggleFullscreen();
        }
        break;
      case SDL_FINGERDOWN:
      case SDL_MOUSEBUTTONDOWN:
        /* on some browsers (chrome, safari) we have to resume Audio on users action 
           https://developers.google.com/web/updates/2017/09/autoplay-policy-changes
        */
        
        EM_ASM({
            if (typeof Module === 'undefined'
                || typeof Module.SDL2 == 'undefined'
                || typeof Module.SDL2.audioContext == 'undefined')
                return;
            if (Module.SDL2.audioContext.state == 'suspended') {
                Module.SDL2.audioContext.resume();
            }
        });
        break;
        default:
          //printf("unhandeld event %d",event->type);
          break;
    }
    return 1;
}

int sum_samples=0;
double last_time = 0.0 ;
unsigned int executed_frame_count=0;
int64_t total_executed_frame_count=0;
double start_time=emscripten_get_now();
unsigned int rendered_frame_count=0;
unsigned int frames=0, seconds=0;
// The emscripten "main loop" replacement function.
void draw_one_frame_into_SDL_noise(void *thisAmiga) 
{

  Amiga *amiga = (Amiga *)thisAmiga;

  Uint8 *texture = (Uint8 *)amiga->denise.pixelEngine.getNoise(); //screenBuffer();
 
//  int surface_width = window_surface->w;
//  int surface_height = window_surface->h;

//  SDL_RenderClear(renderer);
  SDL_Rect SrcR;
  SrcR.x = xOff;
  SrcR.y = yOff;
  SrcR.w = clipped_width;
  SrcR.h = clipped_height;
  SDL_UpdateTexture(screen_texture, &SrcR, texture+ (4*emu_width*SrcR.y) + SrcR.x*4, 4*emu_width);

  SDL_RenderCopy(renderer, screen_texture, &SrcR, NULL);

  SDL_RenderPresent(renderer);

  return;
}
void draw_one_frame_into_SDL(void *thisAmiga) 
{

  //this method is triggered by
  //emscripten_set_main_loop_arg(em_arg_callback_func func, void *arg, int fps, int simulate_infinite_loop) 
  //which is called inside te c64.cpp
  //fps Setting int <=0 (recommended) uses the browser’s requestAnimationFrame mechanism to call the function.

  //The number of callbacks is usually 60 times per second, but will 
  //generally match the display refresh rate in most web browsers as 
  //per W3C recommendation. requestAnimationFrame() 
  
  double now = emscripten_get_now();  
 
  double elapsedTimeInSeconds = (now - start_time)/1000.0;
  int64_t targetFrameCount = (int64_t)(elapsedTimeInSeconds * 50);
 
  int max_gap = 8;


  Amiga *amiga = (Amiga *)thisAmiga;

  if(amiga->inWarpMode() == true)
  {
    printf("warping at least 25 frames at once ...\n");
    int i=25;
    while(amiga->inWarpMode() == true && i>0)
    {
      amiga->execute();
      i--;
    }
    start_time=now;
    total_executed_frame_count=0;
    targetFrameCount=1;  
  }

  //lost the sync
  if(targetFrameCount-total_executed_frame_count > max_gap)
  {
      printf("lost sync target=%lld, total_executed=%lld\n", targetFrameCount, total_executed_frame_count);
      //reset timer
      //because we are out of sync, we do now skip max_gap-1 emulation frames 
      start_time=now;
      total_executed_frame_count=0;
      targetFrameCount=1;  //we are hoplessly behind but do at least one in this round  
  }

  if(now-last_time>= 1000.0)
  { 
    double passed_time= now - last_time;
    last_time = now;

    seconds += 1; 
    frames += rendered_frame_count;
    printf("time[ms]=%.0lf, audio_samples=%d, frames [executed=%u, rendered=%u] avg_fps=%u\n", 
    passed_time, sum_samples, executed_frame_count, rendered_frame_count, frames/seconds);
    sum_samples=0; 
    rendered_frame_count=0;
    executed_frame_count=0;
  }

  while(total_executed_frame_count < targetFrameCount) {
    executed_frame_count++;
    total_executed_frame_count++;

    amiga->execute();
  }

  rendered_frame_count++;  

  EM_ASM({
 //     if (typeof draw_one_frame === 'undefined')
 //         return;
      draw_one_frame(); // to gather joystick information for example 
  });
  
  Uint8 *texture = (Uint8 *)amiga->denise.pixelEngine.getStableBuffer().data; //screenBuffer();

//  Uint8 *texture = (Uint8 *)amiga->denise.pixelEngine.getNoise(); //screenBuffer();
/*  for(unsigned int i=0;i<32;i++)
  {
    printf("%u,", texture[i]);
  }
  printf("\n");
*/
//  int surface_width = window_surface->w;
//  int surface_height = window_surface->h;

//  SDL_RenderClear(renderer);
  SDL_Rect SrcR;
  SrcR.x = xOff;
  SrcR.y = yOff;
  SrcR.w = clipped_width;
  SrcR.h = clipped_height;
  SDL_UpdateTexture(screen_texture, &SrcR, texture+ (4*emu_width*SrcR.y) + SrcR.x*4, 4*emu_width);

  SDL_RenderCopy(renderer, screen_texture, &SrcR, NULL);

  SDL_RenderPresent(renderer);

}


int sample_size=0;
void MyAudioCallback(void*  thisAmiga,
                       Uint8* stream,
                       int    len)
{
    Amiga *amiga = (Amiga *)thisAmiga;
    //calculate number of fitting samples
    //we are copying stereo interleaved floats in LR format, therefore we must divide by 2
    //int n = len /  (2 * sizeof(float));
    amiga->paula.muxer.copy((float *)stream, sample_size); 
/*    printf("copyMono[%d]: ", n);
    for(int i=0; i<n; i++)
    {
      printf("%hhu,",stream[i]);
    }
    printf("\n");
  */ 
    sum_samples += sample_size;
}

extern "C" void wasm_create_renderer(char* name)
{ 
  printf("try to create %s renderer\n", name);
  window = SDL_CreateWindow("",
   SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        clipped_width, clipped_height,
        SDL_WINDOW_RESIZABLE);

  if(0==strcmp("webgl", name))
  {
    renderer = SDL_CreateRenderer(window,
          -1, 
          SDL_RENDERER_PRESENTVSYNC|SDL_RENDERER_ACCELERATED);
    if(renderer == NULL)
    {
      printf("can not get hardware accelerated renderer going with software renderer instead...\n");
    }
    else
    {
      printf("got hardware accelerated renderer ...\n");
    }
  }
  if(renderer == NULL)
  {
      renderer = SDL_CreateRenderer(window,
          -1, 
          SDL_RENDERER_SOFTWARE
          );

    if(renderer == NULL)
    {
      printf("can not get software renderer ...\n");
      return;
    }
    else
    {
      printf("got software renderer ...\n");
    }
  }

    // Since we are going to display a low resolution buffer,
    // it is best to limit the window size so that it cannot
    // be smaller than our internal buffer size.
  SDL_SetWindowMinimumSize(window, clipped_width, clipped_height);
  SDL_RenderSetLogicalSize(renderer, clipped_width, clipped_height); 
  SDL_RenderSetIntegerScale(renderer, SDL_TRUE);

  screen_texture = SDL_CreateTexture(renderer,
        /*SDL_PIXELFORMAT_ARGB32*/ SDL_PIXELFORMAT_ABGR8888 
        , SDL_TEXTUREACCESS_STREAMING,
        emu_width, emu_height);

  window_surface = SDL_GetWindowSurface(window);
}

void initSDL(void *thisAmiga)
{
//    Amiga *amiga = (Amiga *)thisAmiga;
    if(SDL_Init(SDL_INIT_VIDEO)==-1)
    {
        printf("Could not initialize SDL:%s\n", SDL_GetError());
    } 
/*
    SDL_AudioSpec want, have;
    SDL_AudioDeviceID device_id;

    SDL_memset(&want, 0, sizeof(want)); // or SDL_zero(want)
    want.freq = 44100;  //44100; // 22050;
    want.format = AUDIO_F32;
    want.channels = 2;
    //sample buffer 512 in original vc64, vc64web=512 under macOs ok, but iOS needs 2048;
    want.samples = 2048;
    want.callback = MyAudioCallback;
    want.userdata = thisAmiga;   //will be passed to the callback
    device_id = SDL_OpenAudioDevice(NULL, 0, &want, &have, SDL_AUDIO_ALLOW_FORMAT_CHANGE);
    if(device_id == 0)
    {
        printf("Failed to open audio: %s\n", SDL_GetError());
    }
*/
//    printf("set paula.muxer to freq= %d\n", have.freq);
//    amiga->paula.muxer.setSampleRate(have.freq);
  //  sample_size=have.samples;
//    printf("paula.muxer.getSampleRate()==%f\n", amiga->paula.muxer.getSampleRate());
 

//    SDL_PauseAudioDevice(device_id, 0); //unpause the audio device
    
    //listen to mouse, finger and keys
//    SDL_SetEventFilter(eventFilter, thisAmiga);
}


void send_message_to_js(const char * msg)
{
    EM_ASM(
    {
        if (typeof message_handler === 'undefined')
            return;
        message_handler( "MSG_"+UTF8ToString($0) );
    }, msg );    

}

void send_message_to_js(const char * msg, long data)
{
    EM_ASM(
    {
        if (typeof message_handler === 'undefined')
            return;
        message_handler( "MSG_"+UTF8ToString($0), $1 );
    }, msg, data );    

}

bool paused_the_emscripten_main_loop=false;
bool warp_mode=false;
void theListener(const void * amiga, long type, long data){
/*  
  if(warp_mode && type == MSG_IEC_BUS_BUSY && !((Amiga *)amiga)->inWarpMode())
  {
    ((Amiga *)amiga)->warpOn(); //setWarp(true);
  }
  else if(type == MSG_IEC_BUS_IDLE && ((Amiga *)amiga)->inWarpMode())
  {
    ((Amiga *)amiga)->warpOff(); //setWarp(false);
  }
*/
  const char *message_as_string =  (const char *)MsgTypeEnum::key((MsgType)type);
  printf("vAmiga message=%s, data=%ld\n", message_as_string, data);
  send_message_to_js(message_as_string, data);

  if(type == MSG_DISK_INSERT)
  {
//    ((Amiga *)amiga)->drive8.dump();
  }

}



class C64Wrapper {
  public:
    Amiga *amiga;

  C64Wrapper()
  {
    printf("constructing vAmiga ...\n");

    this->amiga = new Amiga();

    printf("adding a listener to vAmiga message queue...\n");

    amiga->msgQueue.setListener(this->amiga, &theListener);
  }
  ~C64Wrapper()
  {
        printf("closing wrapper");
  }

  void run()
  {
/*    printf("wrapper calls 4x c64->loadRom(...) method\n");
    c64->loadRom(ROM_KERNAL ,"roms/kernal.901227-03.bin");
    c64->loadRom(ROM_BASIC, "roms/basic.901226-01.bin");
    c64->loadRom(ROM_CHAR, "roms/characters.901225-01.bin");
    c64->loadRom(ROM_VC1541, "roms/1541-II.251968-03.bin");
*/

    try { amiga->isReady(); } catch(...) { 
      printf("***** put missing rom message\n");
       // amiga->msgQueue.put(ROM_MISSING); 
        EM_ASM({
          setTimeout(function() {message_handler( 'MSG_ROM_MISSING' );}, 0);
        });
    }
    

    printf("v4 wrapper calls run on vAmiga->run() method\n");

    //c64->setTakeAutoSnapshots(false);
    //c64->setWarpLoad(true);

//    c64->configure(OPT_SID_ENGINE, SIDENGINE_RESID);
//    c64->configure(OPT_SID_SAMPLING, SID_SAMPLE_INTERPOLATE);


    // master Volumne
    amiga->configure(OPT_AUDVOLL, 100); 
    amiga->configure(OPT_AUDVOLR, 100);

    //SID0 Volumne
    amiga->configure(OPT_AUDVOL, 0, 100); 
    amiga->configure(OPT_AUDPAN, 0, 0);


    amiga->configure(OPT_CHIP_RAM, 512);
    amiga->configure(OPT_SLOW_RAM, 512);
    amiga->configure(OPT_AGNUS_REVISION, AGNUS_OCS_PLCC);


//    c64->configure(OPT_DRV_AUTO_CONFIG,DRIVE8,1);
    //SID1 Volumne
/*    c64->configure(OPT_AUDVOL, 1, 100);
    c64->configure(OPT_AUDPAN, 1, 50);
    c64->configure(OPT_SID_ENABLE, 1, true);
    c64->configure(OPT_SID_ADDRESS, 1, 0xd420);
*/


    //c64->configure(OPT_HIDE_SPRITES, true); 
    //c64->dump();

 //printf("is running = %u\n",c64->isRunning()); 
 //   c64->dump();
 //   c64->drive1.dump();
 //   c64->setDebugLevel(2);
    //c64->sid.setDebugLevel(4);
 //   c64->drive1.setDebugLevel(3);
 //   c64->sid.dump();


/*
    c64->configure(OPT_DRV_POWER_SAVE, 8, true); 
    c64->configure(OPT_SID_POWER_SAVE, true); 
    c64->configure(OPT_VIC_POWER_SAVE, true); 

*/

    printf("waiting on emulator ready in javascript ...\n");
 
  }
};

C64Wrapper *wrapper = NULL;
extern "C" int main(int argc, char** argv) {
  wrapper= new C64Wrapper();
  initSDL(wrapper->amiga);
  wrapper->run();
  return 0;
}

/* emulation of macos mach_absolute_time() function. */
uint64_t mach_absolute_time()
{
    uint64_t nano_now = (uint64_t)(emscripten_get_now()*1000000.0);
    //printf("emsdk_now: %lld\n", nano_now);
    return nano_now; 
}

extern "C" void wasm_key(int code1, int code2, int pressed)
{
  printf("wasm_key ( %d, %d, %d ) \n", code1, code2, pressed);

  if(pressed==1)
  {
    wrapper->amiga->keyboard.pressKey(code1);
  }
  else
  {
    wrapper->amiga->keyboard.releaseKey(code1);
  }
}

extern "C" void wasm_schedule_key(int code1, int code2, int pressed, int frame_delay)
{
  if(pressed==1)
  {
    printf("scheduleKeyPress ( %d, %d, %d ) \n", code1, code2, frame_delay);
    wrapper->amiga->keyboard.pressKey(code1);
//    wrapper->amiga->keyboard.scheduleKeyPress(*new AmigaKey(code1,code2), frame_delay);
  }
  else
  {
    printf("scheduleKeyRelease ( %d, %d, %d ) \n", code1, code2, frame_delay);
    wrapper->amiga->keyboard.releaseKey(code1);
  
  //  wrapper->amiga->keyboard.scheduleKeyRelease(*new C64Key(code1,code2), frame_delay);
  }
}


char wasm_pull_user_snapshot_file_json_result[255];


extern "C" bool wasm_has_disk()
{
  return wrapper->amiga->df0.hasDisk();
}
 
extern "C" char* wasm_export_disk()
{
  if(!wrapper->amiga->df0.hasDisk())
  {
    printf("no disk in df0\n");
    sprintf(wasm_pull_user_snapshot_file_json_result, "{\"size\": 0 }");
    return wasm_pull_user_snapshot_file_json_result;
  }

  ADFFile *adf = new ADFFile(wrapper->amiga->df0);
  sprintf(wasm_pull_user_snapshot_file_json_result, "{\"address\":%lu, \"size\": %lu }",
  (unsigned long)adf->data, 
  adf->size
  );
  printf("return => %s\n",wasm_pull_user_snapshot_file_json_result);

  return wasm_pull_user_snapshot_file_json_result;
}


Snapshot *snapshot=NULL;
extern "C" void wasm_delete_user_snapshot()
{
//  printf("request to free user_snapshot memory\n");

  if(snapshot!=NULL)
  {
    delete snapshot;
    snapshot=NULL;
    printf("freed user_snapshot memory\n");
  }
}

extern "C" char* wasm_pull_user_snapshot_file()
{
  printf("wasm_pull_user_snapshot_file\n");

  wasm_delete_user_snapshot();
  snapshot = wrapper->amiga->latestUserSnapshot(); //wrapper->amiga->userSnapshot(nr);

//  printf("got snapshot %u.%u.%u\n", snapshot->getHeader()->major,snapshot->getHeader()->minor,snapshot->getHeader()->subminor );

  sprintf(wasm_pull_user_snapshot_file_json_result, "{\"address\":%lu, \"size\": %lu, \"width\": %lu, \"height\":%lu }",
  (unsigned long)snapshot->data, 
  snapshot->size,
  snapshot->getHeader()->screenshot.width,
  snapshot->getHeader()->screenshot.height
  );
  printf("return => %s\n",wasm_pull_user_snapshot_file_json_result);

  return wasm_pull_user_snapshot_file_json_result;
}

extern "C" void wasm_take_user_snapshot()
{
  wrapper->amiga->requestUserSnapshot();
}

float sound_buffer[4096 * 2];
extern "C" float* wasm_get_sound_buffer()
{
  wrapper->amiga->paula.muxer.copy(sound_buffer, sound_buffer+4096, 4096); 
  sum_samples += 4096;
/*  printf("copyMono[%d]: ", 16);
  for(int i=0; i<16; i++)
  {
    //printf("%hhu,",stream[i]);
    printf("%f,",sound_buffer[i]);
  }
  printf("\n"); 
*/
  return sound_buffer;
}

extern "C" void wasm_set_warp(unsigned on)
{
  warp_mode = (on == 1);
/*
  if(wrapper->amiga->iec.isTransferring() && 
      (
        (wrapper->amiga->inWarpMode() && warp_mode == false)
        ||
        (wrapper->amiga->inWarpMode() == false && warp_mode)
      )
  )
  {
*/
    if(warp_mode)
      wrapper->amiga->warpOn();
    else
      wrapper->amiga->warpOff();
/*  }*/
}


extern "C" void wasm_set_borderless(float on)
{
  eat_border_width = 4 * on;
  xOff = 252 + eat_border_width ;
  clipped_width  = HPIXELS - xOff -2*eat_border_width;

  eat_border_height = 24 * on ;
  yOff = 26 + eat_border_height;
  clipped_height = VPIXELS -yOff  -2*eat_border_height- 2; 

/*
  printf("eat_border w=%d, eat_border h=%d\n", eat_border_width, eat_border_height);
  printf("clipped w=%d, clipped h=%d\n", clipped_width, clipped_height);
  printf("emu w=%d, emu h=%d\n", emu_width, emu_height);
  printf("xOff w=%d, yOff h=%d\n", xOff, yOff);

  printf("xoff+clipped+eatborder=%d == %d emuwidth\n",xOff+eat_border_width+clipped_width, emu_width);
  printf("yoff+clipped+eatborder=%d == %d emuheight\n",yOff+eat_border_height+clipped_height, emu_height);
*/
  SDL_SetWindowMinimumSize(window, clipped_width, clipped_height);
  SDL_RenderSetLogicalSize(renderer, clipped_width, clipped_height); 
  SDL_SetWindowSize(window, clipped_width, clipped_height);
}

std::unique_ptr<Disk> load_disk(const char* filename, Uint8 *blob, long len)
{
  try {
    if (DMSFile::isCompatible(filename)) {
      printf("%s - Loading DMS file\n", filename);
      DMSFile dms{blob, len};
      return std::make_unique<Disk>(dms);
    }
    if (ADFFile::isCompatible(filename)) {
      printf("%s - Loading ADF file\n", filename);
      ADFFile adf{blob, len};
      return std::make_unique<Disk>(adf);
    }
    if (EXEFile::isCompatible(filename)) {
      printf("%s - Loading EXE file\n", filename);
      EXEFile exe{blob, len};
      return std::make_unique<Disk>(exe);
    }
  } catch (const VAError& e) {
    printf("Error loading %s - %s\n", filename, e.what());
  }
  return {};
}

extern "C" const char* wasm_loadFile(char* name, Uint8 *blob, long len)
{
  printf("load file=%s len=%ld, header bytes= %x, %x, %x\n", name, len, blob[0],blob[1],blob[2]);
  filename=name;
  if(wrapper == NULL)
  {
    return "";
  }

  if (auto disk = load_disk(name, blob, len)) {
    wrapper->amiga->paula.diskController.insertDisk(std::move(disk), 0, (Cycle)SEC(1.8));
    return "";
  }

  bool file_still_unprocessed=true;
  if (Snapshot::isCompatible(filename) && util::extractSuffix(filename)!="rom")
  {
    try
    {
      printf("try to build Snapshot\n");
      Snapshot *file = new Snapshot(blob, len);      
      printf("isSnapshot\n");
      wrapper->amiga->loadSnapshot(*file);
      file_still_unprocessed=false;
    }
    catch(VAError &exception) {
      ErrorCode ec=exception.data;
      printf("%s\n", ErrorCodeEnum::key(ec));
    }
  }

  if(file_still_unprocessed)
  {
    bool wasRunnable = true;
    try { wrapper->amiga->isReady(); } catch(...) { wasRunnable=false; }

    RomFile *rom = NULL;
    try
    {
      printf("try to build RomFile\n");
      rom = new RomFile(blob, len);
    }
    catch(VAError &exception) {
      printf("Failed to read ROM image file %s\n", name);
      ErrorCode ec=exception.data;
      printf("%s\n", ErrorCodeEnum::key(ec));
      return "";
    }
    
    wrapper->amiga->suspend();
    try { 
      wrapper->amiga->mem.loadRom(*rom); 
      
      printf("Loaded ROM image %s.\n", name);
    }  
    catch(VAError &exception) { 
      printf("Failed to flash ROM image %s.\n", name);
      ErrorCode ec=exception.data;
      printf("%s\n", ErrorCodeEnum::key(ec));
    }
    wrapper->amiga->resume();


    bool is_ready_now = true;
    try { wrapper->amiga->isReady(); } catch(...) { is_ready_now=false; }

    if (!wasRunnable && is_ready_now)
    {
       //wrapper->amiga->putMessage(MSG_READY_TO_RUN);
      const char* ready_msg= "READY_TO_RUN";
      printf("sending ready message %s.\n", ready_msg);
      send_message_to_js(ready_msg);    
    }

    const char *rom_type="rom";
    delete rom;
    return rom_type;    
  }
  return "";
}


extern "C" void wasm_reset()
{
  wrapper->amiga->reset(true);
}


extern "C" void wasm_halt()
{
  printf("wasm_halt\n");
  wrapper->amiga->pause();

  printf("emscripten_pause_main_loop() at MSG_PAUSE\n");
  paused_the_emscripten_main_loop=true;
  emscripten_pause_main_loop();
  printf("after emscripten_set_main_loop_arg() at MSG_RUN\n");

}

extern "C" void wasm_run()
{
  printf("wasm_run\n");
  
  printf("is running = %u\n",wrapper->amiga->isRunning());

  wrapper->amiga->run();

  if(paused_the_emscripten_main_loop)
  {
    printf("emscripten_resume_main_loop at MSG_RUN\n");
    emscripten_resume_main_loop();
  }
  else
  {
    printf("emscripten_set_main_loop_arg() at MSG_RUN\n");
    emscripten_set_main_loop_arg(draw_one_frame_into_SDL, (void *)wrapper->amiga, 0, 1);
    printf("after emscripten_set_main_loop_arg() at MSG_RUN\n");

  }

}


extern "C" void wasm_press_play()
{
/*  printf("wasm_press_play\n");
  wrapper->amiga->datasette.pressPlay();
*/
}


extern "C" void wasm_joystick(char* port_plus_event)
{
    printf("wasm_joystick event=%s\n", port_plus_event);
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
    code = PULL_UP;
  }
  else if( strcmp(event,"PULL_LEFT") == 0)
  {
    code = PULL_LEFT;
  }
  else if( strcmp(event,"PULL_DOWN") == 0)
  {
    code = PULL_DOWN;
  }
  else if( strcmp(event,"PULL_RIGHT") == 0)
  {
    code = PULL_RIGHT;
  }
  else if( strcmp(event,"PRESS_FIRE") == 0)
  {
    code = PRESS_FIRE;
  }
  else if( strcmp(event,"RELEASE_XY") == 0)
  {
    code = RELEASE_XY;
  }
  else if( strcmp(event,"RELEASE_X") == 0)
  {
    code = RELEASE_X;
  }
  else if( strcmp(event,"RELEASE_Y") == 0)
  {
    code = RELEASE_Y;
  }
  else if( strcmp(event,"RELEASE_FIRE") == 0)
  {
    code = RELEASE_FIRE;
  }


  else
  {
    return;    
  }

  if(joyport == '1')
  {
    wrapper->amiga->controlPort1.joystick.trigger(code);
  }
  else if(joyport == '2')
  {
    wrapper->amiga->controlPort2.joystick.trigger(code);
  }

}

char buffer[50];
extern "C" char* wasm_sprite_info()
{
/*   sprintf(buffer, "%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u", 
     wrapper->amiga->vic.reg.current.sprX[0],
     wrapper->amiga->vic.reg.current.sprY[0],
     wrapper->amiga->vic.reg.current.sprX[1],
     wrapper->amiga->vic.reg.current.sprY[1],
     wrapper->amiga->vic.reg.current.sprX[2],
     wrapper->amiga->vic.reg.current.sprY[2],
     wrapper->amiga->vic.reg.current.sprX[3],
     wrapper->amiga->vic.reg.current.sprY[3],
     wrapper->amiga->vic.reg.current.sprX[4],
     wrapper->amiga->vic.reg.current.sprY[4],
     wrapper->amiga->vic.reg.current.sprX[5],
     wrapper->amiga->vic.reg.current.sprY[5],
     wrapper->amiga->vic.reg.current.sprX[6],
     wrapper->amiga->vic.reg.current.sprY[6],
     wrapper->amiga->vic.reg.current.sprX[7],
     wrapper->amiga->vic.reg.current.sprY[7]
     );  
*/
   return buffer;
}

extern "C" void wasm_set_sid_model(unsigned SID_Model)
{
/*
  bool wasRunning=false;
  if(wrapper->amiga->isRunning()){
    wasRunning= true;
    wrapper->amiga->pause();
  }
  if(SID_Model == 6581)
  {
    wrapper->amiga->configure(OPT_SID_REVISION, MOS_6581);
  }
  else if(SID_Model == 8580)
  {
    wrapper->amiga->configure(OPT_SID_REVISION, MOS_8580);  
  }
  if(wasRunning)
  {
    wrapper->amiga->run();
  }
  */
}

extern "C" void wasm_cut_layers(unsigned cut_layers)
{
//  wrapper->amiga->configure(OPT_CUT_LAYERS, 0x100 | (SPR0|SPR1|SPR2|SPR3|SPR4|SPR5|SPR6|SPR7)); 
//  wrapper->amiga->configure(OPT_CUT_LAYERS, cut_layers); 
}



char json_result[1024];
extern "C" const char* wasm_rom_info()
{

  sprintf(json_result, "{\"hasRom\":\"%s\", \"romTitle\":\"%s\", \"romVersion\":\"%s\", \"romReleased\":\"%s\", \"romModel\":\"%s\" }",
    wrapper->amiga->mem.hasRom()?"true":"false",
    wrapper->amiga->mem.romTitle(),
    wrapper->amiga->mem.romVersion(),
    wrapper->amiga->mem.romReleased(),
    ""
//    wrapper->amiga->mem.romModel()
  );

  printf("%s, %s, %s, %s\n",      wrapper->amiga->mem.romTitle(),
    wrapper->amiga->mem.romVersion(),
    wrapper->amiga->mem.romReleased(),
    ""
//    wrapper->amiga->mem.romModel()
  );

  return json_result;
}


extern "C" void wasm_set_2nd_sid(long address)
{
  /*
  if(address == 0)
  {
    wrapper->amiga->configure(OPT_AUDVOL, 1, 0);
    wrapper->amiga->configure(OPT_SID_ENABLE, 1, false);
  }
  else
  {
    wrapper->amiga->configure(OPT_AUDVOL, 1, 100);
    wrapper->amiga->configure(OPT_AUDPAN, 1, 50);
    wrapper->amiga->configure(OPT_SID_ENABLE, 1, true);
    wrapper->amiga->configure(OPT_SID_ADDRESS, 1, address);
  }
  */
}


extern "C" void wasm_set_sid_engine(char* engine)
{
  printf("wasm_set_sid_engine %s\n", engine);
/*
  bool wasRunning=false;
  if(wrapper->amiga->isRunning()){
    wasRunning= true;
    wrapper->amiga->pause();
  }


  if( strcmp(engine,"FastSID") == 0)
  {
    printf("c64->configure(OPT_SID_ENGINE, ENGINE_FASTSID);\n");
    wrapper->amiga->configure(OPT_SID_ENGINE, SIDENGINE_FASTSID);
  }
  else if( strcmp(engine,"ReSID fast") == 0)
  { 
    printf("c64->configure(OPT_SID_SAMPLING, SID_SAMPLE_FAST);\n");
    wrapper->amiga->configure(OPT_SID_ENGINE, SIDENGINE_RESID);
    wrapper->amiga->configure(OPT_SID_SAMPLING, reSID::SAMPLE_FAST);
  }
  else if( strcmp(engine,"ReSID interpolate") == 0)
  {
    printf("c64->configure(OPT_SID_SAMPLING, SID_SAMPLE_INTERPOLATE);\n");
    wrapper->amiga->configure(OPT_SID_ENGINE, SIDENGINE_RESID);
    wrapper->amiga->configure(OPT_SID_SAMPLING, reSID::SAMPLE_INTERPOLATE);
  }
  else if( strcmp(engine,"ReSID resample") == 0)
  {
    printf("c64->configure(OPT_SID_SAMPLING, SID_SAMPLE_RESAMPLE);\n");
    wrapper->amiga->configure(OPT_SID_ENGINE, SIDENGINE_RESID);
    wrapper->amiga->configure(OPT_SID_SAMPLING, reSID::SAMPLE_RESAMPLE);
  }


  if(wasRunning)
  {
    wrapper->amiga->run();
  }
*/
}


extern "C" void wasm_set_color_palette(char* palette)
{
/*
  if( strcmp(palette,"color") == 0)
  {
    wrapper->amiga->configure(OPT_PALETTE, PALETTE_COLOR);
  }
  else if( strcmp(palette,"black white") == 0)
  { 
    wrapper->amiga->configure(OPT_PALETTE, PALETTE_BLACK_WHITE); 
  }
  else if( strcmp(palette,"paper white") == 0)
  { 
    wrapper->amiga->configure(OPT_PALETTE, PALETTE_PAPER_WHITE); 
  }
  else if( strcmp(palette,"green") == 0)
  { 
    wrapper->amiga->configure(OPT_PALETTE, PALETTE_GREEN); 
  }
  else if( strcmp(palette,"amber") == 0)
  { 
    wrapper->amiga->configure(OPT_PALETTE, PALETTE_AMBER); 
  }
  else if( strcmp(palette,"sepia") == 0)
  { 
    wrapper->amiga->configure(OPT_PALETTE, PALETTE_SEPIA); 
  }
*/
}


extern "C" u64 wasm_get_cpu_cycles()
{
  return wrapper->amiga->cpu.getClock();
}

extern "C" u8 wasm_peek(u16 addr)
{
  //return wrapper->amiga->mem.spypeek(addr);
  return 0;
}


extern "C" void wasm_write_string_to_ser(char* chars_to_send)
{
//  wrapper->amiga->write_string_to_ser(chars_to_send);
}

//const char chars_to_send[] = "HELLOMYWORLD!";
extern "C" void wasm_poke(u16 addr, u8 value)
{
    //wrapper->amiga->mem.poke(addr, value);
}

/*
char wasm_translate_json[255];
extern "C" char* wasm_translate(char c)
{
  auto key = C64Key::translate(c);
  sprintf(wasm_translate_json, "{\"size\": "+key.+" }");
  sprintf( wasm_translate_json);
  
  return wasm_translate_json;
}
*/


char config_result[512];
extern "C" const char* wasm_power_on(unsigned power_on)
{
  try{
    bool was_powered_on=wrapper->amiga->isPoweredOn();
    if(power_on == 1 && !was_powered_on)
    {
        wrapper->amiga->powerOn();
    }
    else if(power_on == 0 && was_powered_on)
    {
        wrapper->amiga->powerOff();
    }
  }  
  catch(VAError &exception) {   
    sprintf(config_result,"%s", exception.what());
  }
  return config_result; 
}


extern "C" void wasm_set_sample_rate(unsigned sample_rate)
{
    printf("set paula.muxer to freq= %d\n", sample_rate);
    wrapper->amiga->paula.muxer.setSampleRate(sample_rate);
    printf("paula.muxer.getSampleRate()==%f\n", wrapper->amiga->paula.muxer.getSampleRate());
}




extern "C" const char* wasm_configure(char* option, char* _value)
{
  sprintf(config_result,""); 
  auto value = std::string(_value);
  printf("wasm_configure %s = %s\n", option, value.c_str());

  bool was_powered_on=wrapper->amiga->isPoweredOn();

  bool must_be_off= strcmp(option,"AGNUS_REVISION") == 0 || 
                    strcmp(option,"CHIP_RAM") == 0 ||
                    strcmp(option,"SLOW_RAM") == 0 ||
                    strcmp(option,"FAST_RAM") == 0;
 
  if(was_powered_on && must_be_off)
  {
      wrapper->amiga->powerOff();
  }

  try{
    if( strcmp(option,"AGNUS_REVISION") == 0)
    {
      wrapper->amiga->configure(util::parseEnum <OptionEnum>(std::string(option)), util::parseEnum <AgnusRevisionEnum>(value)); 
      //wrapper->amiga->agnus.dump();
    }
    else if ( strcmp(option,"BLITTER_ACCURACY") == 0 ||
              strcmp(option,"DRIVE_SPEED") == 0  ||
              strcmp(option,"CHIP_RAM") == 0  ||
              strcmp(option,"SLOW_RAM") == 0  ||
              strcmp(option,"FAST_RAM") == 0  
    )
    {
      wrapper->amiga->configure(util::parseEnum <OptionEnum>(std::string(option)), util::parseNum(value));
    }
    else
    {
      wrapper->amiga->configure(util::parseEnum <OptionEnum>(std::string(option)), util::parseBool(value)); 
    }

    if(was_powered_on && must_be_off)
    {
        wrapper->amiga->powerOn();
    }
  }
  catch(VAError &exception) {    
//    ErrorCode ec=exception.data;
//    sprintf(config_result,"%s", ErrorCodeEnum::key(ec));
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

