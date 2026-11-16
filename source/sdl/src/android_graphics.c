#if defined(__ANDROID__) && defined(SONICR_GLES)

#include "android_graphics.h"
#include "sonicr_gl.h"
#include <GLES2/gl2ext.h>
#include <SDL.h>
#include <jni.h>
#include <stdio.h>
#include <string.h>

static SDL_SpinLock s_lock;
static int s_requestedHeight, s_requestedSmooth;
static char s_status[160] = "Renderer starting";
static int s_height = -1, s_smooth;
static int s_width, s_targetHeight, s_displayWidth, s_displayHeight;
static GLuint s_framebuffer, s_color, s_depth, s_program;

static int ValidHeight(int height)
{
    return height == 0 || height == 480 || height == 720 || height == 1080 ||
           height == 1440 || height == 2160;
}

JNIEXPORT void JNICALL Java_org_sonicr_recomp_GameActivity_nativeGraphics(
    JNIEnv *env, jclass type, jint height, jboolean smooth)
{
    (void)env;
    (void)type;
    SDL_AtomicLock(&s_lock);
    s_requestedHeight = ValidHeight(height) ? height : 0;
    s_requestedSmooth = smooth != 0;
    SDL_AtomicUnlock(&s_lock);
}

JNIEXPORT jstring JNICALL Java_org_sonicr_recomp_GameActivity_nativeGraphicsStatus(
    JNIEnv *env, jclass type)
{
    (void)type;
    char status[sizeof(s_status)];
    SDL_AtomicLock(&s_lock);
    memcpy(status, s_status, sizeof(status));
    SDL_AtomicUnlock(&s_lock);
    return (*env)->NewStringUTF(env, status);
}

static void DeleteTarget(void)
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    if (s_framebuffer) glDeleteFramebuffers(1, &s_framebuffer);
    if (s_color) glDeleteTextures(1, &s_color);
    if (s_depth) glDeleteRenderbuffers(1, &s_depth);
    s_framebuffer = s_color = s_depth = 0;
    s_width = s_targetHeight = 0;
}

static GLuint Shader(GLenum type, const char *source)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (ok) return shader;
    glDeleteShader(shader);
    return 0;
}

static int InitProgram(void)
{
    if (s_program) return 1;
    GLuint vertex = Shader(GL_VERTEX_SHADER,
        "attribute vec2 a_position; varying mediump vec2 v_uv;"
        "void main(){gl_Position=vec4(a_position,0.0,1.0);v_uv=(a_position+1.0)*0.5;}");
    GLuint fragment = Shader(GL_FRAGMENT_SHADER,
        "precision mediump float; uniform sampler2D u_texture; varying mediump vec2 v_uv;"
        "void main(){gl_FragColor=texture2D(u_texture,v_uv);}");
    if (vertex && fragment) {
        s_program = glCreateProgram();
        glAttachShader(s_program, vertex);
        glAttachShader(s_program, fragment);
        glBindAttribLocation(s_program, 0, "a_position");
        glLinkProgram(s_program);
        GLint ok = 0;
        glGetProgramiv(s_program, GL_LINK_STATUS, &ok);
        if (!ok) {
            glDeleteProgram(s_program);
            s_program = 0;
        }
    }
    if (vertex) glDeleteShader(vertex);
    if (fragment) glDeleteShader(fragment);
    return s_program != 0;
}

static void CreateTarget(int height)
{
    int width = height * 4 / 3;
    GLint textureLimit, depthLimit, viewportLimit[2], texture, depth;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &textureLimit);
    glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &depthLimit);
    glGetIntegerv(GL_MAX_VIEWPORT_DIMS, viewportLimit);
    if (width > textureLimit || height > textureLimit || width > depthLimit ||
        height > depthLimit || width > viewportLimit[0] || height > viewportLimit[1]) return;
    if (!InitProgram()) return;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &texture);
    glGetIntegerv(GL_RENDERBUFFER_BINDING, &depth);
    glGenTextures(1, &s_color);
    glBindTexture(GL_TEXTURE_2D, s_color);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glGenRenderbuffers(1, &s_depth);
    glBindRenderbuffer(GL_RENDERBUFFER, s_depth);
    const char *extensions = (const char *)glGetString(GL_EXTENSIONS);
    GLenum depthFormat = extensions && strstr(extensions, "GL_OES_depth24") ? GL_DEPTH_COMPONENT24_OES : GL_DEPTH_COMPONENT16;
    glRenderbufferStorage(GL_RENDERBUFFER, depthFormat, width, height);
    glGenFramebuffers(1, &s_framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, s_framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, s_color, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, s_depth);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE) {
        s_width = width;
        s_targetHeight = height;
    } else {
        DeleteTarget();
        SDL_LogWarn(SDL_LOG_CATEGORY_RENDER, "Cannot allocate %dx%d render target; using Native display", width, height);
    }
    glBindTexture(GL_TEXTURE_2D, texture);
    glBindRenderbuffer(GL_RENDERBUFFER, depth);
}

void AndroidGraphics_BeginFrame(int *width, int *height)
{
    SDL_AtomicLock(&s_lock);
    int requested = s_requestedHeight;
    s_smooth = s_requestedSmooth;
    SDL_AtomicUnlock(&s_lock);
    s_displayWidth = *width;
    s_displayHeight = *height;
    if (requested != s_height) {
        DeleteTarget();
        s_height = requested;
        if (requested) CreateTarget(requested);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, s_framebuffer);
    if (s_framebuffer) {
        *width = s_width;
        *height = s_targetHeight;
    }
    int sceneW = *width, sceneH = *height;
    if (sceneW * 3 > sceneH * 4) sceneW = sceneH * 4 / 3;
    else sceneH = sceneW * 3 / 4;
    char status[sizeof(s_status)] = {0};
    snprintf(status, sizeof(status), "Rendering %d x %d%s%s", sceneW, sceneH,
        requested && !s_framebuffer ? " (Native fallback: target unavailable)" :
        !requested ? " (Native display)" : "",
        s_smooth ? "; smooth textures" : "; game texture filtering");
    SDL_AtomicLock(&s_lock);
    memcpy(s_status, status, sizeof(s_status));
    SDL_AtomicUnlock(&s_lock);
}

int AndroidGraphics_SmoothTextures(void)
{
    return s_smooth;
}

void AndroidGraphics_Present(void)
{
    if (!s_framebuffer) return;
    GLint program, texture, buffer, viewport[4];
    GLfloat clearColor[4];
    glGetFloatv(GL_COLOR_CLEAR_VALUE, clearColor);
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &texture);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &buffer);
    glGetIntegerv(GL_VIEWPORT, viewport);
    GLboolean blend = glIsEnabled(GL_BLEND), depth = glIsEnabled(GL_DEPTH_TEST);
    GLboolean scissor = glIsEnabled(GL_SCISSOR_TEST), cull = glIsEnabled(GL_CULL_FACE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_CULL_FACE);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    int w = s_displayWidth, h = s_displayHeight;
    if (w * 3 > h * 4) w = h * 4 / 3;
    else h = w * 3 / 4;
    glViewport((s_displayWidth - w) / 2, (s_displayHeight - h) / 2, w, h);
    glUseProgram(s_program);
    glUniform1i(glGetUniformLocation(s_program, "u_texture"), 0);
    glBindTexture(GL_TEXTURE_2D, s_color);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    const GLfloat vertices[] = {-1, -1, 1, -1, -1, 1, 1, 1};
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vertices);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDisableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUseProgram(program);
    if (blend) glEnable(GL_BLEND);
    if (depth) glEnable(GL_DEPTH_TEST);
    if (scissor) glEnable(GL_SCISSOR_TEST);
    if (cull) glEnable(GL_CULL_FACE);
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    glClearColor(clearColor[0], clearColor[1], clearColor[2], clearColor[3]);
    glBindFramebuffer(GL_FRAMEBUFFER, s_framebuffer);
}

void AndroidGraphics_Shutdown(void)
{
    DeleteTarget();
    if (s_program) glDeleteProgram(s_program);
    s_program = 0;
    s_height = -1;
}

#endif
