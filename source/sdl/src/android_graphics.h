#ifndef SONICR_ANDROID_GRAPHICS_H
#define SONICR_ANDROID_GRAPHICS_H

#if defined(__ANDROID__) && defined(SONICR_GLES)
void AndroidGraphics_BeginFrame(int *width, int *height);
void AndroidGraphics_Present(void);
void AndroidGraphics_Shutdown(void);
int AndroidGraphics_SmoothTextures(void);
#endif

#endif
