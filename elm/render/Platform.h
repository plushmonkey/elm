#pragma once

#include <windows.h>
//
#include <glad/glad.h>
#include <stdio.h>

#define GL_DEBUG_TYPE_ERROR 0x824C
#define GL_DEBUG_OUTPUT 0x92E0

typedef void(APIENTRYP PFNGLDEBUGMESSAGECALLBACKPROC)(GLDEBUGPROC callback, const void* userParam);

static PFNGLDEBUGMESSAGECALLBACKPROC glDebugMessageCallback;

inline void GLAPIENTRY OnOpenGLError(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
                                     const GLchar* message, const void* userParam) {
  if (type == GL_DEBUG_TYPE_ERROR) {
    fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
            (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""), type, severity, message);
  }
}

inline void OGLErrorCheck() {
  const GLenum errorCode = glGetError();
  if (errorCode != GL_NO_ERROR) {
    int a = 0;
  }
}
