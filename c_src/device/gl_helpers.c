#include <stddef.h>

#ifdef SCENIC_GLES2
  #include <GLES3/gl2.h>
#else
  #include <GLES3/gl3.h>
#endif

#include "comms.h"

//---------------------------------------------------------
char* device_gl_error()
{
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
      log_error("GL_INVALID_ENUM");
      break;
    case GL_INVALID_VALUE:
      log_error("GL_INVALID_VALUE");
      break;
    case GL_INVALID_OPERATION:
      log_error("GL_INVALID_OPERATION");
      break;
    case GL_OUT_OF_MEMORY:
      log_error("GL_OUT_OF_MEMORY");
      break;
#ifdef GL_STACK_UNDERFLOW
    case GL_STACK_UNDERFLOW:
      log_error("GL_STACK_UNDERFLOW");
      break;
#endif
#ifdef GL_STACK_OVERFLOW
    case GL_STACK_OVERFLOW:
      log_error("GL_STACK_OVERFLOW");
      break;
#endif
    case GL_INVALID_FRAMEBUFFER_OPERATION:
      log_error("GL_INVALID_FRAMEBUFFER_OPERATION");
      break;
    default:
      log_error("GL_OTHER: %d", err);
      break;
    }
  }
}
