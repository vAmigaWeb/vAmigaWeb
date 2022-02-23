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
#include "ExtendedRomFile.h"
#include "ADFFile.h"
#include "DMSFile.h"
#include "EXEFile.h"
#include "Snapshot.h"

#include "MemUtils.h"


#include <emscripten.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengles2.h> 
#include <emscripten/html5.h>


#define RENDER_SOFTWARE 0
#define RENDER_GPU 1
#define RENDER_SHADER 2
u8 render_method = RENDER_SOFTWARE;


/********* shaders ***********/
GLuint basic;
GLuint merge;
GLuint longf;
GLuint shortf;

const GLchar *vertexSource =
  "#version 300 es      \n"
  "precision mediump float;    \n"
  "in vec4 a_position;  \n"
  "in vec2 a_texcoord;  \n"
  "uniform vec2 u_diw_size;  \n"
  "out vec2 v_texcoord;    \n"
  "out vec2 amiga_pos;    \n"
  "void main() {               \n"
  "  amiga_pos = a_position.xy * u_diw_size.xy; \n"
  "  gl_Position = a_position; \n"
  "  v_texcoord = a_texcoord;  \n"
  "}                           \n";

const GLchar *basicSource =
  "#version 300 es      \n"
  "precision mediump float;                        \n"
  "uniform sampler2D u_long;                       \n"
  "in vec2 v_texcoord;                        \n"
  "out vec4 color;                                 \n"
  "void main() {                                   \n"
  "  color = texture(u_long, v_texcoord); \n"
  "}                                               \n";

const GLchar *mergeSource =
  "#version 300 es      \n"
  "precision mediump float;                           \n"
  "uniform sampler2D u_long;                          \n"
  "uniform sampler2D u_short;                         \n"
  "in vec2 amiga_pos;                           \n"
  "in vec2 v_texcoord;                           \n"
  "out vec4 color;                                 \n"
  "void main() {                                      \n"
  "  if (mod(amiga_pos.y, 2.0) < 1.0) {                        \n"
  "    color = texture(u_short, v_texcoord); \n"
  "  } else {                                         \n"
  "    color = texture(u_long, v_texcoord);  \n"
  "  }                                                \n"
  "}                                                  \n";


int clip_width  = 724;
int clip_height = 568;
int clip_offset = 0; 
int buffer_size = 4096;

bool prevLOF = false;
bool currLOF = false;


GLuint compileShader(const GLenum type, const GLchar *source) {
  GLint result;
  GLint length;

  GLuint shader = glCreateShader(type);
  glShaderSource(shader, 1, &source, NULL);
  glCompileShader(shader);

  glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
  glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);

  if (result != GL_TRUE) {
    std::vector<char> error(length + 1);
    glGetShaderInfoLog(shader, length, NULL, &error[0]);
    printf("Failed to compile shader: %s\n", &error[0]);

    glDeleteShader(shader);
    return 0;
  }

  return shader;
}

GLuint compileProgram(const GLchar *source) {
  GLint result;
  GLint length;

  GLuint vertex = compileShader(GL_VERTEX_SHADER, vertexSource);
  GLuint fragment = compileShader(GL_FRAGMENT_SHADER, source);

  if (vertex == 0 || fragment == 0) {
    return 0;
  }

  GLuint program = glCreateProgram();
  glAttachShader(program, vertex);
  glAttachShader(program, fragment);
  glLinkProgram(program);

  glGetProgramiv(program, GL_LINK_STATUS, &result);
  glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);

  if (result != GL_TRUE) {
    std::vector<char> error(length + 1);
    glGetProgramInfoLog(program, length, NULL, &error[0]);
    printf("Failed to compile program: %s\n", &error[0]);

    glDeleteProgram(program);
    return 0;
  }

  glDeleteShader(vertex);
  glDeleteShader(fragment);
  return program;
}

void set_texture_display_window(const GLuint program, float hstart, float hstop, float vstart, float vstop)
{
  const float x1 = hstart / HPIXELS;
  const float x2 = hstop / HPIXELS;
  const float y1 = vstart  / VPIXELS;
  const float y2 = vstop / VPIXELS;

  const GLfloat coords[] = {
    x1,y1, x2,y1, x1,y2, x2,y2
  };

  GLuint corBuffer;
  glGenBuffers(1, &corBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, corBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(coords), coords, GL_STATIC_DRAW);

  GLint corAttrib = glGetAttribLocation(program, "a_texcoord");
  glEnableVertexAttribArray(corAttrib);
  glVertexAttribPointer(corAttrib, 2, GL_FLOAT, GL_FALSE, 0, 0);

  glUniform2f(glGetUniformLocation(merge, "u_diw_size"), hstop-hstart, vstop-vstart);
}

void initGeometry(const GLuint program, float eat_x, float eat_y) {
  //--- add a_position
  const GLfloat position[] = {
    -1.0f,  1.0f,
     1.0f,  1.0f,
    -1.0f, -1.0f,
     1.0f, -1.0f
  };
  GLuint posBuffer;
  glGenBuffers(1, &posBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, posBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(position), position, GL_STATIC_DRAW);

  GLint posAttrib = glGetAttribLocation(program, "a_position");
  glEnableVertexAttribArray(posAttrib);
  glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 0, 0);

  //--- add a_texcoord

  set_texture_display_window(program, 168.0f,892.0f,27.0f,311.0f);
}

GLuint initTexture(const GLuint *source) {
  GLuint texture = 0;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, HPIXELS, VPIXELS, 0, GL_RGBA, GL_UNSIGNED_BYTE, source);

  return texture;
}

/********************************************************/






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
unsigned int warp_to_frame=0;
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
  bool show_stat=true;
  if(amiga->inWarpMode() == true)
  {
    printf("warping at least 25 frames at once ...\n");
    int i=25;
    while(amiga->inWarpMode() == true && i>0)
    {
      amiga->execute();
      i--;
      if(warp_to_frame>0 && amiga->agnus.frame.nr > warp_to_frame)
      {
        printf("reached warp_to_frame count\n");
        amiga->warpOff();
        warp_to_frame=0;
      }
    }
    start_time=now;
    total_executed_frame_count=0;
    targetFrameCount=1;
    show_stat=false;
  }
  if(show_stat)
  {
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
  }

  int skipped=-1;  // -1, 0, 1 , 2 
  while(skipped % 2 == 1 //when skip then skip twice because of interlace detection
        || 
        total_executed_frame_count < targetFrameCount) {
    executed_frame_count++;
    total_executed_frame_count++;
    amiga->execute();
    skipped++;
  }

  rendered_frame_count++;  
  
  if(skipped==-1)
    return;   //in case no execute was called 

  EM_ASM({
 //     if (typeof draw_one_frame === 'undefined')
 //         return;
      draw_one_frame(); // to gather joystick information for example 
  });
  
  if(render_method==RENDER_SHADER)
  {
    ScreenBuffer stable = amiga->denise.pixelEngine.getStableBuffer();
    prevLOF = currLOF;
    currLOF = stable.longFrame;

    if (currLOF) {
      glActiveTexture(GL_TEXTURE0);
    } else if (prevLOF) {
      glActiveTexture(GL_TEXTURE1);
    } else {
      glActiveTexture(GL_TEXTURE0);
    }  

    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, HPIXELS, VPIXELS, GL_RGBA, GL_UNSIGNED_BYTE, stable.data + clip_offset);

    if (currLOF != prevLOF) {
      // Case 1: Interlace drawing
      glUseProgram(merge);
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, longf);

      glActiveTexture(GL_TEXTURE1);
      glBindTexture(GL_TEXTURE_2D, shortf);
    } 
    else 
    {
      // Case 2: Non-interlace drawing (two long frames in a row)
      glUseProgram(basic);
      glBindTexture(GL_TEXTURE_2D, longf);
    } 




  auto deniseInfo = amiga->denise.getInfo();
  float vstop = (float)deniseInfo.vstop;
  float vstart = (float)deniseInfo.vstrt;
  float hstop = (float)deniseInfo.hstop*2;
  float hstart = (float)deniseInfo.hstrt*2;

  if(vstart < 27.0f)
    vstart=27.0f;
  if(vstop > 311.0f || vstop < 27.0f)
    vstop=311.0f;
  if(hstart < 168.0f)
    hstart=168.0f;    
  if(hstop > 892.0f || hstop <168.0f)
    hstop=892.0f;   

//  glUseProgram(basic);
  set_texture_display_window(basic, hstart, hstop, vstart, vstop);

//  glUseProgram(merge);
  set_texture_display_window(merge, hstart, hstop, vstart, vstop);

//  printf("%f, %f, %f, %f \n", hstart, hstop, vstart, vstop);



    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    SDL_GL_SwapWindow(window);
  }
  else
  {

    Uint8 *texture = (Uint8 *)amiga->denise.pixelEngine.getStableBuffer().data;

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




void initSDL(void *thisAmiga)
{
    if(SDL_Init(SDL_INIT_VIDEO)==-1)
    {
        printf("Could not initialize SDL:%s\n", SDL_GetError());
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
 
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

bool already_run_the_emscripten_main_loop=false;
bool warp_mode=false;
void theListener(const void * amiga, long type, long data){
  if(warp_to_frame>0 && ((Amiga *)amiga)->agnus.frame.nr < warp_to_frame)
  {
    //skip automatic warp mode on disk load
  }
  else
  {
    if(warp_mode && type == MSG_DRIVE_MOTOR_ON && !((Amiga *)amiga)->inWarpMode())
    {
      ((Amiga *)amiga)->warpOn(); //setWarp(true);
    }
    else if(type == MSG_DRIVE_MOTOR_OFF && ((Amiga *)amiga)->inWarpMode())
    {
      ((Amiga *)amiga)->warpOff(); //setWarp(false);
    }
  }

  const char *message_as_string =  (const char *)MsgTypeEnum::key((MsgType)type);
  if(type == MSG_SER_IN || type == MSG_SER_OUT)
  {
  }
  else
  {
    printf("vAmiga message=%s, data=%ld\n", message_as_string, data);
    send_message_to_js(message_as_string, data);
  }
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


bool create_shader()
{
    printf("try to create shader renderer\n");

    window = SDL_CreateWindow("", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, clip_width, clip_height, SDL_WINDOW_OPENGL);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_CreateContext(window);
    SDL_GL_SetSwapInterval(0);

    basic = compileProgram(basicSource);
    merge = compileProgram(mergeSource);

    if (basic == 0 || merge == 0)
      return false;
    glViewport(0, 0, clip_width, clip_height);
    initGeometry(basic, 0,0);
    initGeometry(merge, 0,0);

    ScreenBuffer stable = wrapper->amiga->denise.pixelEngine.getStableBuffer();

    longf  = initTexture(stable.data + clip_offset);
    shortf = initTexture(stable.data + clip_offset);

    glUseProgram(merge);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, shortf);
    glUniform1i(glGetUniformLocation(merge, "u_short"), 1);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, longf);
    glUniform1i(glGetUniformLocation(merge, "u_long"), 0);
    glUniform2f(glGetUniformLocation(merge, "u_diw_size"), 724.0f, 284.0f);

    glUseProgram(basic);
    glUniform1i(glGetUniformLocation(basic, "u_long"), 0);
    glUniform2f(glGetUniformLocation(merge, "u_diw_size"), 724.0f, 284.0f);

    return true;
}
bool create_renderer_webgl()
{
    printf("try to create web_gl renderer\n");
      //mach den anderen webgl renderer 
    window = SDL_CreateWindow("",
    SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
    clipped_width, clipped_height,
    SDL_WINDOW_RESIZABLE);

    renderer = SDL_CreateRenderer(window,
          -1, 
          SDL_RENDERER_PRESENTVSYNC|SDL_RENDERER_ACCELERATED);

    return renderer != NULL; 
}
bool create_renderer_software()
{
    printf("create software renderer\n");
    if(window == NULL)
      window = SDL_CreateWindow("",
      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      clipped_width, clipped_height,
      SDL_WINDOW_RESIZABLE);

    renderer = SDL_CreateRenderer(window,
          -1, 
          SDL_RENDERER_SOFTWARE
          );
    return renderer != NULL; 
}
void create_texture()
{
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


extern "C" void wasm_create_renderer(char* name)
{ 
  render_method=RENDER_SOFTWARE;
  printf("try to create %s renderer\n", name);
  if(0==strcmp("shader", name))
  {
    if(create_shader())
    {
      render_method=RENDER_SHADER;
    }
    else
    {
      if(create_renderer_webgl())
      {
        render_method=RENDER_GPU;
      }
      else
      {
        create_renderer_software();
      }
      create_texture();
    }

  }
  else if(0==strcmp("gpu", name))
  {
    if(create_renderer_webgl())
    {
      printf("got hardware accelerated renderer ...\n");
      render_method=RENDER_GPU;
    }
    else
    {
      printf("can not get hardware accelerated renderer going with software renderer instead...\n");
      create_renderer_software();
    }
    create_texture();
  } else
  {
      create_renderer_software();
      create_texture();
  }

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

  sprintf(wasm_pull_user_snapshot_file_json_result, "{\"address\":%lu, \"size\": %lu, \"width\": %u, \"height\":%u }",
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

float sound_buffer[16384 * 2];
extern "C" float* wasm_get_sound_buffer_address()
{
  return sound_buffer;
}

extern "C" unsigned wasm_copy_into_sound_buffer()
{
  auto count=wrapper->amiga->paula.muxer.stream.count();
  
  auto copied_samples=0;
  for(unsigned ipos=1024;ipos<=count;ipos+=1024)
  {
    wrapper->amiga->paula.muxer.copy(
    sound_buffer+copied_samples,
     sound_buffer+copied_samples+1024, 
     1024); 
    copied_samples+=1024*2;
//  printf("fillLevel (%lf)",wrapper->amiga->paula.muxer.stream.fillLevel());
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
  if(warp_mode == false && wrapper->amiga->inWarpMode())
  {
      wrapper->amiga->warpOff();
  }
/*  }*/
}


extern "C" void wasm_set_borderless(float on)
{
  if(render_method==RENDER_SHADER)
  {
    if(on==1.0f)
    {
      //adaptive
      wrapper->amiga->setInspectionTarget(INSPECTION_DENISE, 5);
    }
    else
    {
      set_texture_display_window(basic, 168.0f,892.0f,27.0f,311.0f);
      set_texture_display_window(merge, 168.0f,892.0f,27.0f,311.0f);
    }


    return;
  }

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
    //wrapper->amiga->paula.diskController.insertDisk(std::move(disk), 0, (Cycle)SEC(1.8));
    wrapper->amiga->df0.swapDisk(std::move(disk));
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

  if(file_still_unprocessed && util::extractSuffix(filename)=="rom_file")
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

    if(wrapper->amiga->isPoweredOn())
    {
      wrapper->amiga->powerOff();
    }
//    wrapper->amiga->suspend();
    try { 
      wrapper->amiga->mem.loadRom(*rom); 
      
      printf("Loaded ROM image %s.\n", name);
      
    }  
    catch(VAError &exception) { 
      printf("Failed to flash ROM image %s.\n", name);
      ErrorCode ec=exception.data;
      printf("%s\n", ErrorCodeEnum::key(ec));
      return "";
    }
    const char *rom_type="rom";
    try
    {
        bool is_ready_now = true;
        try { wrapper->amiga->isReady(); } catch(...) { is_ready_now=false; }

        if (!wasRunnable && is_ready_now)
        {
          printf("was not runnable is ready now (rom)\n");
          wrapper->amiga->powerOn();
        
          //wrapper->amiga->putMessage(MSG_READY_TO_RUN);
          const char* ready_msg= "READY_TO_RUN";
          printf("sending ready message %s.\n", ready_msg);
          send_message_to_js(ready_msg);    
        }

        delete rom;
        wrapper->amiga->powerOn();
//        wrapper->amiga->resume();
    }    
    catch(VAError &exception) { 
      ErrorCode ec=exception.data;
      printf("%s\n", ErrorCodeEnum::key(ec));
    } 

    return rom_type;    
  }
  
  if(file_still_unprocessed && util::extractSuffix(filename)=="rom_ext_file")
  {
    bool wasRunnable = true;
    try { wrapper->amiga->isReady(); } catch(...) { wasRunnable=false; }

    ExtendedRomFile *rom = NULL;
    try
    {
      printf("try to build ExtendedRomFile\n");
      rom = new ExtendedRomFile(blob, len);
    }
    catch(VAError &exception) {
      printf("Failed to read ROM_EXT image file %s\n", name);
      ErrorCode ec=exception.data;
      printf("%s\n", ErrorCodeEnum::key(ec));
      return "";
    }

    if(wrapper->amiga->isPoweredOn())
    {
      wrapper->amiga->powerOff();
    }
    //wrapper->amiga->suspend();
    try { 
      wrapper->amiga->mem.loadExt(*rom); 
      
      printf("Loaded ROM_EXT image %s.\n", name);
      
    }  
    catch(VAError &exception) { 
      printf("Failed to flash ROM_EXT image %s.\n", name);
      ErrorCode ec=exception.data;
      printf("%s\n", ErrorCodeEnum::key(ec));
    }
  

    bool is_ready_now = true;
    try { wrapper->amiga->isReady(); } catch(...) { is_ready_now=false; }

    if (!wasRunnable && is_ready_now)
    {
      printf("was not runnable is ready now (romext)\n");
      wrapper->amiga->powerOn();

       //wrapper->amiga->putMessage(MSG_READY_TO_RUN);
      const char* ready_msg= "READY_TO_RUN";
      printf("sending ready message %s.\n", ready_msg);
      send_message_to_js(ready_msg);    
    }

    const char *rom_type="rom_ext";
    delete rom;
    //wrapper->amiga->resume();
    wrapper->amiga->powerOn();
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

  if(paused_the_emscripten_main_loop || already_run_the_emscripten_main_loop)
  {
    printf("emscripten_resume_main_loop at MSG_RUN %u, %u\n", paused_the_emscripten_main_loop, already_run_the_emscripten_main_loop);
    emscripten_resume_main_loop();
  }
  else
  {
    printf("emscripten_set_main_loop_arg() at MSG_RUN %u, %u\n", paused_the_emscripten_main_loop, already_run_the_emscripten_main_loop);
    already_run_the_emscripten_main_loop=true;

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

extern "C" void wasm_mouse(int port, int x, int y)
{
  //printf("wasm_mouse port%d x=%d, y=%d\n", port, x, y);
  if(port==1)
    wrapper->amiga->controlPort1.mouse.setDxDy(x,y);
  else if(port==2)
    wrapper->amiga->controlPort2.mouse.setDxDy(x,y);
}

extern "C" void wasm_mouse_button(int port, int button_id, int pressed)
{ 
  if(port==1)
  {
    if(button_id==1)
      wrapper->amiga->controlPort1.mouse.setLeftButton(pressed==1);
    else if(button_id==3)
      wrapper->amiga->controlPort1.mouse.setRightButton(pressed==1);
  }
  else if(port==2)
  {
    if(button_id==1)
      wrapper->amiga->controlPort2.mouse.setLeftButton(pressed==1);
    else if(button_id==3)
      wrapper->amiga->controlPort2.mouse.setRightButton(pressed==1);
  }
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

  sprintf(json_result, "{\"hasRom\":\"%s\",\"hasExt\":\"%s\", \"romTitle\":\"%s\", \"romVersion\":\"%s\", \"romReleased\":\"%s\", \"romModel\":\"%s\", \"extTitle\":\"%s\", \"extVersion\":\"%s\", \"extReleased\":\"%s\", \"extModel\":\"%s\" }",
    wrapper->amiga->mem.hasRom()?"true":"false",
    wrapper->amiga->mem.hasExt()?"true":"false",
    wrapper->amiga->mem.romTitle(),
    wrapper->amiga->mem.romVersion(),
    wrapper->amiga->mem.romReleased(),
    wrapper->amiga->mem.romModel(),
    wrapper->amiga->mem.extTitle(),
    wrapper->amiga->mem.extVersion(),
    wrapper->amiga->mem.extReleased(),
    wrapper->amiga->mem.extModel()
  );

/*
  printf("%s, %s, %s, %s\n",      wrapper->amiga->mem.romTitle(),
    wrapper->amiga->mem.romVersion(),
    wrapper->amiga->mem.romReleased(),
    ""
//    wrapper->amiga->mem.romModel()
  );
*/
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

  if(strcmp(option,"warp_to_frame") == 0 )
  {
    warp_to_frame= util::parseNum(value);
    wrapper->amiga->warpOn();
    return config_result;
  }

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

