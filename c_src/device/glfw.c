/*
#  Created by Boyd Multerer on 2021-09-18
#  Adapted from the old scenic_driver_glfw from 2018...
#  Copyright © 2018-2021 Kry10 Limited. All rights reserved.
#
*/

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER
#include <fcntl.h> //O_BINARY
#endif

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define NANOVG_GL2_IMPLEMENTATION
#include "../nanovg/nanovg.h"
#include "../nanovg/nanovg_gl.h"

#include "../types.h"
#include "../utils.h"
#include "../comms.h"
#include "device.h"

#define STDIN_FILENO 0



typedef struct {
  float       last_x;
  float       last_y;

  int         window_width;
  int         window_height;
  int         frame_width;
  int         frame_height;
  float       ratio_x;
  float       ratio_y;
  bool        glew_ok;

  GLFWwindow* p_window;

  device_info_t* p_info;
} glfw_data_t;

glfw_data_t g_glfw_data = {0};


//=============================================================================
// window callbacks

void errorcb(int error, const char* desc)
{
  put_sn( desc, error );
}

//---------------------------------------------------------
void reshape_framebuffer(GLFWwindow* window, int w, int h)
{
  g_glfw_data.frame_width  = w;
  g_glfw_data.frame_height = h;
}

//---------------------------------------------------------
void reshape_window(GLFWwindow* window, int w, int h)
{
  // calculate the framebuffer to window size ratios
  // this will be used for things like oversampling fonts
  g_glfw_data.window_width  = w;
  g_glfw_data.window_height = h;

  int fw, fh;
  glfwGetFramebufferSize( window, &fw, &fh );
  g_glfw_data.frame_width  = fw;
  g_glfw_data.frame_height = fh;

  g_glfw_data.ratio_x = (float) fw / (float) w;
  g_glfw_data.ratio_y = (float) fh / (float) h;

  glViewport( 0, 0, fw, fh );
  glClear( GL_COLOR_BUFFER_BIT );

  g_glfw_data.p_info->width = w;
  g_glfw_data.p_info->height = h;
  g_glfw_data.p_info->ratio = g_glfw_data.ratio_x;

  send_reshape( w, h );
}

//---------------------------------------------------------
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
  send_key(key, scancode, action, mods);
}

//---------------------------------------------------------
void charmods_callback(GLFWwindow* window, unsigned int codepoint, int mods)
{
    send_codepoint(codepoint, mods);
}

//---------------------------------------------------------
static void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos)
{
  float          x, y;
  x = xpos;
  y = ypos;
  // only send the message if the postion changed
  if ((g_glfw_data.last_x != x) || (g_glfw_data.last_y != y))
  {
    send_cursor_pos(x, y);
    g_glfw_data.last_x = x;
    g_glfw_data.last_y = y;
  }
}

//---------------------------------------------------------
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
  double         x, y;
  glfwGetCursorPos(window, &x, &y);
  send_mouse_button(button, action, mods, x, y);
}

//---------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
  double         x, y;
  glfwGetCursorPos(window, &x, &y);
  send_scroll(xoffset, yoffset, x, y);
}
//---------------------------------------------------------
void cursor_enter_callback(GLFWwindow* window, int entered)
{
  double         x, y;
  glfwGetCursorPos(window, &x, &y);
  send_cursor_enter(entered, x, y);
}

//---------------------------------------------------------
void window_close_callback(GLFWwindow* window)
{
// send_puts("window_close_callback");

  // let the calling app filter the close event. Send a message up.
  // if the app wants to let the window close, it needs to send a close back
  // down.
  send_close( 0 );
  glfwSetWindowShouldClose(window, false);
}

// //=============================================================================
// // main setup

//---------------------------------------------------------
// done before the window is created
void set_window_hints(bool f_resizable)
{
  // is the window resizable
  glfwWindowHint(GLFW_RESIZABLE, f_resizable);

  // claim the focus right on creation
  glfwWindowHint(GLFW_FOCUSED, true);

  // we want OpenGL 2.1
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
}

//---------------------------------------------------------
// set up one-time features of the window
NVGcontext* setup_window(GLFWwindow* window, const device_opts_t* p_opts)
{
  // start position tracking with values that are obviously out of the window
  g_glfw_data.last_x      = -1.0f;
  g_glfw_data.last_y      = -1.0f;

  g_glfw_data.window_width  = p_opts->width;
  g_glfw_data.window_height = p_opts->height;
  g_glfw_data.ratio_x = 1.0f;
  g_glfw_data.ratio_y = 1.0f;

  g_glfw_data.glew_ok = false;

  // Make the window's context current
  glfwMakeContextCurrent(window);

  // initialize glew - do after setting up window and making current
  g_glfw_data.glew_ok = glewInit() == GLEW_OK;

  // get the actual framebuffer size to set it up
  // glfwGetFramebufferSize(window, &g_glfw_data.frame_width, &g_glfw_data.frame_height);
  // reshape_framebuffer(window, g_glfw_data.frame_width, g_glfw_data.frame_height);

//   // get the actual window size to set it up
  int window_width, window_height;
  glfwGetWindowSize(window, &window_width, &window_height);
  reshape_window( window, window_width, window_height );

  uint32_t nvg_opts = 0;
  if ( p_opts->antialias ) nvg_opts |= NVG_ANTIALIAS;
  if ( p_opts->debug_mode ) nvg_opts |= NVG_DEBUG;
  NVGcontext* p_ctx = nvgCreateGL2( nvg_opts );

  // set up callbacks
  glfwSetFramebufferSizeCallback(window, reshape_framebuffer);
  glfwSetWindowSizeCallback(window, reshape_window);
  glfwSetKeyCallback(window, key_callback);
  glfwSetCharModsCallback(window, charmods_callback);
  glfwSetCursorPosCallback(window, cursor_pos_callback);
  glfwSetCursorEnterCallback(window, cursor_enter_callback);
  glfwSetMouseButtonCallback(window, mouse_button_callback);
  glfwSetScrollCallback(window, scroll_callback);
  glfwSetWindowCloseCallback(window, window_close_callback);

  // set the initial clear color
  glClearColor(0.0, 0.0, 0.0, 1.0);

  // return the nvg context
  return p_ctx;
}


//---------------------------------------------------------
int device_init( const device_opts_t* p_opts, device_info_t* p_info ) {
  // Initialize GLFW
  if ( !glfwInit() )
  {
    log_error( "Unable to initialize GLFW" );
    return -1;
  }

  // set the error callback
  glfwSetErrorCallback(errorcb);

  // set the glfw window hints - done before window creation
  set_window_hints(
    true           // resizable?
  );

  // Create a windowed mode window and its OpenGL context
  g_glfw_data.p_window = glfwCreateWindow(
    p_opts->width, p_opts->height,
    p_opts->title,
    NULL,                   // which monitor
    NULL
  );
  if ( !g_glfw_data.p_window ) {
    log_error( "Unable to create GLFW window" );
    glfwTerminate();
    return -1;
  }

  // set up one-time features of the window
  g_glfw_data.p_info = p_info;
  p_info->p_ctx = setup_window( g_glfw_data.p_window, p_opts );
  p_info->width = g_glfw_data.window_width;
  p_info->height = g_glfw_data.window_height;


#ifdef __APPLE__
  // heinous hack to get around macOS Mojave GL issues
  // without this, the window is blank until manually resized
  glfwPollEvents();
  int w, h;
  glfwGetWindowSize(g_glfw_data.p_window, &w, &h);
  glfwSetWindowSize(g_glfw_data.p_window, w++, h);
  glfwSetWindowSize(g_glfw_data.p_window, w, h);
#endif

#ifdef _MSC_VER
  _setmode(_fileno(stdin), O_BINARY);
  _setmode(_fileno(stdout), O_BINARY);
#endif

  // see if any errors happened during startup
  check_gl_error();


  return 0;
}


//---------------------------------------------------------
// tear down one-time features of the window
int device_close( device_info_t* p_info )
{
  return 0;
}

//---------------------------------------------------------
void device_clear_color( float red, float green, float blue, float alpha ) {
  glClearColor( red, green, blue, alpha );
}

//---------------------------------------------------------
void device_begin_render() {
  glClear( GL_COLOR_BUFFER_BIT );
}

void device_end_render() {
  glfwSwapBuffers( g_glfw_data.p_window );
}


//---------------------------------------------------------
void device_poll() {
  glfwPollEvents();
}

//---------------------------------------------------------
char* device_gl_error() {
  GLenum err;
  while (true)
  {
    err = glGetError();
    // check if there was a gl error
    switch (err)
    {
      case GL_NO_ERROR:
        return NULL;
      case GL_INVALID_ENUM:
        send_puts( "GL_INVALID_ENUM" );
        break;
      case GL_INVALID_VALUE:
        send_puts( "GL_INVALID_VALUE" );
        break;
      case GL_INVALID_OPERATION:
        send_puts( "GL_INVALID_OPERATION" );
        break;
      case GL_OUT_OF_MEMORY:
        send_puts( "GL_OUT_OF_MEMORY" );
        break;

#ifdef GL_STACK_UNDERFLOW
      case GL_STACK_UNDERFLOW:
        send_puts( "GL_STACK_UNDERFLOW" );
        break;
#endif

#ifdef GL_STACK_OVERFLOW
      case GL_STACK_OVERFLOW:
        send_puts( "GL_STACK_OVERFLOW" );
        break;
#endif
        
      case GL_INVALID_FRAMEBUFFER_OPERATION:
        send_puts( "GL_INVALID_FRAMEBUFFER_OPERATION" );
        break;
      default:
        put_sn( "GL_OTHER:", err );
        break;
    }
  }
}
