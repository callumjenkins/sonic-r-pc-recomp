#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <assert.h>
#include <stdio.h>

static int forceLimit, forceIncomplete;
static GLenum limitedProperty;
static int limitedValue;
static int failShader, failLink, noDepth24;
static void GetInteger(GLenum name, GLint *value)
{
    glGetIntegerv(name, value);
    if (forceLimit && (name == GL_MAX_TEXTURE_SIZE || name == GL_MAX_RENDERBUFFER_SIZE)) *value = forceLimit;
    if (name == limitedProperty) *value = limitedValue;
    if (name == GL_MAX_VIEWPORT_DIMS && limitedProperty == GL_VIEWPORT) value[1] = limitedValue;
}
static GLenum FramebufferStatus(GLenum target)
{
    return forceIncomplete ? GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT : glCheckFramebufferStatus(target);
}
static void ShaderStatus(GLuint shader, GLenum name, GLint *value)
{
    glGetShaderiv(shader, name, value);
    if (failShader && name == GL_COMPILE_STATUS) *value = GL_FALSE;
}
static void ProgramStatus(GLuint program, GLenum name, GLint *value)
{
    glGetProgramiv(program, name, value);
    if (failLink && name == GL_LINK_STATUS) *value = GL_FALSE;
}
static const GLubyte *Extensions(GLenum name)
{
    return noDepth24 && name == GL_EXTENSIONS ? (const GLubyte *)"" : glGetString(name);
}
#define glGetShaderiv ShaderStatus
#define glGetProgramiv ProgramStatus
#define glGetString Extensions
#define glGetIntegerv GetInteger
#define glCheckFramebufferStatus FramebufferStatus
#include "../source/sdl/src/android_graphics.c"
#undef glGetIntegerv
#undef glCheckFramebufferStatus
#undef glGetShaderiv
#undef glGetProgramiv
#undef glGetString
#include "../source/sdl/src/r_gl_backend.c"

static void Request(int height, int smooth)
{
    Java_org_sonicr_recomp_GameActivity_nativeGraphics(NULL, NULL, height, smooth);
}

static void Begin(int width, int height)
{
    AndroidGraphics_BeginFrame(&width, &height);
}

static void Pixel(int x, int y, int red, int green, int blue)
{
    unsigned char pixel[4];
    glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    assert(pixel[0] == red && pixel[1] == green && pixel[2] == blue);
}

static void NativeAndPresets(void)
{
    int w = 900, h = 400;
    AndroidGraphics_BeginFrame(&w, &h);
    assert(w == 900 && h == 400 && s_framebuffer == 0);
    assert(strstr(s_status, "533 x 400 (Native display)") != NULL);
    AndroidGraphics_Present();
    GLint bound;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &bound);
    assert(bound == 0);
    int heights[] = {480, 720, 1080, 1440, 2160};
    for (int i = 0; i < 5; i++) {
        Request(heights[i], i % 2);
        w = 900; h = 400;
        AndroidGraphics_BeginFrame(&w, &h);
        assert(w == heights[i] * 4 / 3 && h == heights[i]);
        assert(s_framebuffer && glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &bound);
        assert(bound == (GLint)s_framebuffer);
        assert(AndroidGraphics_SmoothTextures() == i % 2);
        GLuint target = s_framebuffer;
        Begin(800, 700);
        assert(s_framebuffer == target && s_displayWidth == 800 && s_displayHeight == 700);
    }
    GLuint target = s_framebuffer, color = s_color, depth = s_depth;
    Request(0, 1);
    Begin(800, 700);
    assert(s_framebuffer == 0 && !glIsFramebuffer(target) && !glIsTexture(color) && !glIsRenderbuffer(depth));
    assert(strstr(s_status, "800 x 600 (Native display); smooth textures") != NULL);
    assert(AndroidGraphics_SmoothTextures() == 1);
    for (int i = 0; i < 2; i++) {
        Request(i ? 9999 : -1, 0);
        Begin(900, 400);
        assert(s_framebuffer == 0 && strstr(s_status, "Native display"));
    }
}

static void Failures(void)
{
    forceLimit = 2048;
    Request(2160, 0);
    Begin(900, 400);
    assert(!s_framebuffer && strstr(s_status, "Native fallback"));
    forceLimit = 0;
    Begin(900, 400);
    assert(!s_framebuffer);
    Request(480, 0);
    forceIncomplete = 1;
    Begin(900, 400);
    assert(!s_framebuffer && !s_color && !s_depth && strstr(s_status, "Native fallback"));
    forceIncomplete = 0;
    Request(720, 0);
    Begin(900, 400);
    assert(s_framebuffer);
}

static void IndependentLimits(void)
{
    GLenum properties[] = {GL_MAX_TEXTURE_SIZE, GL_MAX_RENDERBUFFER_SIZE, GL_MAX_VIEWPORT_DIMS, GL_VIEWPORT};
    for (int i = 0; i < 4; i++) {
        int boundary = properties[i] == GL_VIEWPORT ? 720 : 960;
        for (int exact = 0; exact <= 1; exact++) {
            Request(0, 0);
            Begin(900, 400);
            limitedProperty = properties[i];
            limitedValue = boundary - 1 + exact;
            Request(720, 0);
            Begin(900, 400);
            assert((s_framebuffer != 0) == exact);
            assert(exact || strstr(s_status, "Native fallback"));
            limitedProperty = 0;
        }
    }
}

static void Presentation(void)
{
    Request(480, 0);
    Begin(900, 400);
    glDisable(GL_SCISSOR_TEST);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glClearColor(1, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_SCISSOR_TEST);
    glScissor(0, 240, 640, 240);
    glClearColor(0, 1, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glViewport(7, 9, 101, 103);
    GLuint texture, buffer;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glUseProgram(0);
    AndroidGraphics_Present();
    GLint value, viewport[4];
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &value);
    assert(value == (GLint)s_framebuffer);
    glGetIntegerv(GL_CURRENT_PROGRAM, &value);
    assert(value == 0);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &value);
    assert(value == (GLint)texture);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &value);
    assert(value == (GLint)buffer);
    glGetIntegerv(GL_VIEWPORT, viewport);
    assert(viewport[0] == 7 && viewport[1] == 9 && viewport[2] == 101 && viewport[3] == 103);
    assert(glIsEnabled(GL_BLEND) && glIsEnabled(GL_DEPTH_TEST) && glIsEnabled(GL_SCISSOR_TEST) && glIsEnabled(GL_CULL_FACE));
    GLfloat clear[4];
    glGetFloatv(GL_COLOR_CLEAR_VALUE, clear);
    assert(clear[0] == 0 && clear[1] == 1 && clear[2] == 0 && clear[3] == 1);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    Pixel(450, 100, 255, 0, 0);
    Pixel(450, 300, 0, 255, 0);
    Pixel(10, 200, 0, 0, 0);
    Begin(800, 700);
    AndroidGraphics_Present();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    Pixel(400, 100, 255, 0, 0);
    Pixel(400, 600, 0, 255, 0);
    Pixel(400, 10, 0, 0, 0);
    glDeleteTextures(1, &texture);
    glDeleteBuffers(1, &buffer);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
}

static void ShaderAndDepthFallback(void)
{
    for (int link = 0; link <= 1; link++) {
        AndroidGraphics_Shutdown();
        failShader = !link;
        failLink = link;
        Request(480, 0);
        Begin(900, 400);
        assert(!s_framebuffer && !s_program && strstr(s_status, "Native fallback"));
        failShader = failLink = 0;
        Request(720, 0);
        Begin(900, 400);
        assert(s_framebuffer && s_program);
    }
    AndroidGraphics_Shutdown();
    noDepth24 = 1;
    Request(480, 0);
    Begin(900, 400);
    assert(s_framebuffer);
    GLint format;
    glBindRenderbuffer(GL_RENDERBUFFER, s_depth);
    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_INTERNAL_FORMAT, &format);
    assert(format == GL_DEPTH_COMPONENT16);
    noDepth24 = 0;
    AndroidGraphics_Shutdown();
    AndroidGraphics_Shutdown();
    Begin(900, 400);
    assert(s_framebuffer && s_program);
}

static void Filtering(void)
{
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    s_desired.filter = R_FILTER_NEAREST;
    s_tpageFilter[0] = R_FILTER_NEAREST + 1;
    s_tpageFilter[1] = R_FILTER_LINEAR + 1;
    s_tpageFilter[2] = 0;
    for (int smooth = 0; smooth <= 1; smooth++) {
        Request(0, smooth);
        Begin(900, 400);
        for (int page = 0; page < 3; page++) {
            s_glTextureFilter[page] = 0;
            GL_ApplyTpageFilter(page);
            GLint filter;
            glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, &filter);
            assert(filter == ((smooth || page == 1) ? GL_LINEAR : GL_NEAREST));
            glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, &filter);
            assert(filter == ((smooth || page == 1) ? GL_LINEAR : GL_NEAREST));
        }
    }
    Request(0, 0);
    Begin(900, 400);
    GL_ApplyTpageFilter(2);
    GLint filter;
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, &filter);
    assert(filter == GL_NEAREST);
    glDeleteTextures(1, &texture);
}

#include "../source/sdl/src/render_gl.c"
void *g_tpagePixelBuf[52];
int g_tpageWidth[52], g_tpageHeight[52];
static int displayWidth = 900, displayHeight = 400, swaps;
void platform_get_drawable_size(int *w, int *h) { *w = displayWidth; *h = displayHeight; }
void platform_gl_swap(void) { swaps++; }
static void RealLifecycle(void)
{
    g_screenWidth = 640;
    g_screenHeight = g_screenHeightFull = 480;
    int heights[] = {480, 720, 0, 480};
    for (int i = 0; i < 4; i++) {
        Request(heights[i], i % 2);
        for (int j = 0; j < 2; j++) {
            BeginFrame();
            ProcessTpageStates();
            R_SetBlendMode(R_BLEND_NONE);
            R_SetDepthTest(0);
            R_SetAlphaTest(0);
            R_DrawQuad2DSolid(0, 0, 640, 480, 0, j ? 0xff00ff00 : 0xffff0000);
            EndFrame();
            FlipD3D();
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            Pixel(450, 200, j ? 0 : 255, j ? 255 : 0, 0);
            Pixel(10, 200, 0, 0, 0);
            assert(glGetError() == GL_NO_ERROR);
        }
    }
    assert(swaps == 8);
}


static void RealFiltering(void)
{
    unsigned short pixels[] = {0xf800, 0x001f, 0xf800, 0x001f};
    g_tpagePixelBuf[0] = pixels;
    g_tpageWidth[0] = g_tpageHeight[0] = 2;
    R_SetTpageFilter(0, R_FILTER_NEAREST);
    int smooths[] = {0, 1, 1, 0};
    for (int i = 0; i < 4; i++) {
        Request(480, smooths[i]);
        BeginFrame();
        ProcessTpageStates();
        R_SetTexture(0);
        R_SetDepthTest(0);
        R_SetAlphaTest(0);
        if (i == 2) R_MarkTextureDirty(0);
        R_DrawQuad2D(0, 0, 640, 480, 0, 0, 1, 1, 0, 0xffffffff);
        GLint filter;
        glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, &filter);
        assert(filter == (smooths[i] ? GL_LINEAR : GL_NEAREST));
        FlipD3D();
        assert(glGetError() == GL_NO_ERROR);
    }
    g_tpagePixelBuf[0] = NULL;
}

/* A 2048 page drawn at 640 wide minifies, so it samples the mip chain and draws black if the chain is incomplete. */
static void HdPages(void)
{
    unsigned short stock[] = {0xf800, 0xf800, 0xf800, 0xf800};
    g_tpagePixelBuf[1] = stock;
    g_tpageWidth[1] = g_tpageHeight[1] = 2;
    for (int smooth = 0; smooth <= 1; smooth++) {
        unsigned char *page = malloc(2048 * 2048 * 4);
        assert(page);
        for (int i = 0; i < 2048 * 2048; i++) {
            page[i * 4] = 0;
            page[i * 4 + 1] = 255;
            page[i * 4 + 2] = 0;
            page[i * 4 + 3] = 255;
        }
        R_SetTpageFilter(1, R_FILTER_NEAREST);
        R_SetTpageHD(1, page, 2048);
        R_MarkTextureDirty(1);
        Request(480, smooth);
        BeginFrame();
        ProcessTpageStates();
        R_SetTexture(1);
        R_SetDepthTest(0);
        R_SetAlphaTest(0);
        R_DrawQuad2D(0, 0, 640, 480, 0, 0, 1, 1, 0, 0xffffffff);
        GLint filter;
        glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, &filter);
        assert(filter == (smooth ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_NEAREST));
        EndFrame();
        FlipD3D();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        Pixel(450, 200, 0, 255, 0);

        R_SetTpageHD(1, NULL, 0);
        R_MarkTextureDirty(1);
        BeginFrame();
        ProcessTpageStates();
        R_SetTexture(1);
        R_DrawQuad2D(0, 0, 640, 480, 0, 0, 1, 1, 0, 0xffffffff);
        glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, &filter);
        assert(filter == (smooth ? GL_LINEAR : GL_NEAREST));
        EndFrame();
        FlipD3D();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        Pixel(450, 200, 255, 0, 0);
        assert(glGetError() == GL_NO_ERROR);
    }
    g_tpagePixelBuf[1] = NULL;
}

static unsigned char *GreenPage(void)
{
    unsigned char *page = malloc(512 * 512 * 4);
    assert(page);
    for (int i = 0; i < 512 * 512; i++) {
        page[i * 4] = 0;
        page[i * 4 + 1] = 255;
        page[i * 4 + 2] = 0;
        page[i * 4 + 3] = 255;
    }
    return page;
}

static void DrawPage(int tpage)
{
    R_SetTexture(tpage);
    R_DrawQuad2D(0, 0, 640, 480, 0, 0, 1, 1, 0, 0xffffffff);
}

static void FinishFrame(void)
{
    EndFrame();
    FlipD3D();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static void HdDropKeepsTheBoundPage(void)
{
    unsigned short red[] = {0xf800, 0xf800, 0xf800, 0xf800};
    unsigned short blue[] = {0x001f, 0x001f, 0x001f, 0x001f};
    g_tpagePixelBuf[0] = blue;
    g_tpagePixelBuf[1] = red;
    g_tpageWidth[0] = g_tpageHeight[0] = g_tpageWidth[1] = g_tpageHeight[1] = 2;
    R_SetTpageHD(1, GreenPage(), 512);
    R_MarkTextureDirty(0);
    R_MarkTextureDirty(1);
    Request(480, 0);
    BeginFrame();
    ProcessTpageStates();
    R_SetDepthTest(0);
    R_SetAlphaTest(0);
    DrawPage(1);
    DrawPage(0);
    R_SetTpageHD(1, NULL, 0);
    R_MarkTextureDirty(1);
    DrawPage(0);
    FinishFrame();
    Pixel(450, 200, 0, 0, 255);
    g_tpagePixelBuf[0] = g_tpagePixelBuf[1] = NULL;
}

static void HdDropOnAFrozenPageShowsStock(void)
{
    unsigned short red[] = {0xf800, 0xf800, 0xf800, 0xf800};
    g_tpagePixelBuf[1] = red;
    g_tpageWidth[1] = g_tpageHeight[1] = 2;
    R_SetTpageHD(1, GreenPage(), 512);
    R_MarkTextureDirty(1);
    Request(480, 0);
    BeginFrame();
    ProcessTpageStates();
    R_SetDepthTest(0);
    R_SetAlphaTest(0);
    DrawPage(1);
    FinishFrame();
    Pixel(450, 200, 0, 255, 0);
    R_SetTpageHD(1, NULL, 0);
    R_MarkTextureDirty(1);
    R_ClearTextureDirty(1);
    BeginFrame();
    ProcessTpageStates();
    DrawPage(1);
    FinishFrame();
    Pixel(450, 200, 255, 0, 0);
    g_tpagePixelBuf[1] = NULL;
}

int main(void)
{
    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    assert(eglInitialize(display, NULL, NULL));
    assert(eglBindAPI(EGL_OPENGL_ES_API));
    EGLint configAttrs[] = {EGL_SURFACE_TYPE, EGL_PBUFFER_BIT, EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_NONE};
    EGLConfig config;
    EGLint count;
    assert(eglChooseConfig(display, configAttrs, &config, 1, &count) && count == 1);
    EGLint attrs[] = {EGL_WIDTH, 1000, EGL_HEIGHT, 800, EGL_NONE};
    EGLSurface surface = eglCreatePbufferSurface(display, config, attrs);
    EGLint ctxAttrs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
    EGLContext context = eglCreateContext(display, config, EGL_NO_CONTEXT, ctxAttrs);
    assert(eglMakeCurrent(display, surface, surface, context));
    NativeAndPresets();
    Failures();
    IndependentLimits();
    Presentation();
    Filtering();
    RealLifecycle();
    RealFiltering();
    HdPages();
    HdDropKeepsTheBoundPage();
    HdDropOnAFrozenPageShowsStock();
    ShaderAndDepthFallback();
    AndroidGraphics_Shutdown();
    assert(s_framebuffer == 0 && s_color == 0 && s_depth == 0 && s_program == 0);
    assert(glGetError() == GL_NO_ERROR);
    eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(display, context);
    eglDestroySurface(display, surface);
    eglTerminate(display);
    puts("Android graphics checks passed");
}
