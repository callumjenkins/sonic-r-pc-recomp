#ifndef SONICR_GL_H
#define SONICR_GL_H

#if defined(SONICR_GLES)
#include <GLES2/gl2.h>
#elif defined(__APPLE__)
#include <OpenGL/gl.h>
#elif defined(_WIN32)
#include <GL/glew.h>
#include <GL/gl.h>
#else
#include <GL/gl.h>
#endif

#endif
