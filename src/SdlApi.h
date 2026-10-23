// The SDL 1.2, SDL_ttf, SDL_mixer and SDL_image functions MainUI calls, declared once. The
// recovered code uses its own minimal structs (SDLTypes.h) instead of the SDL headers, so fonts,
// sounds and music are plain pointers and events are passed as void * (each file keeps the
// SDL_Event layout it needs). C and C++.
#pragma once

#ifdef __cplusplus
struct SDL_Surface;
struct SDL_Rect;
struct SDL_PixelFormat;
struct SDL_mutex;
struct SDL_cond;
struct SDL_Thread;
extern "C" {
#else
typedef struct SDL_Surface SDL_Surface;
typedef struct SDL_Rect SDL_Rect;
typedef struct SDL_PixelFormat SDL_PixelFormat;
typedef struct SDL_mutex SDL_mutex;
typedef struct SDL_cond SDL_cond;
typedef struct SDL_Thread SDL_Thread;
#endif
typedef void TTF_Font;
typedef void Mix_Chunk;
typedef void Mix_Music;

// Core
int SDL_Init(unsigned flags);
void SDL_Quit(void);
const char *SDL_GetError(void);
void SDL_Delay(unsigned ms);
int SDL_ShowCursor(int toggle);
int SDL_EnableKeyRepeat(int delay, int interval);

// Video
SDL_Surface *SDL_SetVideoMode(int width, int height, int bpp, unsigned flags);
int SDL_Flip(SDL_Surface *screen);
void SDL_UpdateRect(SDL_Surface *screen, int x, int y, unsigned w, unsigned h);
SDL_Surface *SDL_CreateRGBSurface(unsigned flags, int width, int height, int depth,
                                  unsigned rMask, unsigned gMask, unsigned bMask, unsigned aMask);
SDL_Surface *SDL_ConvertSurface(SDL_Surface *src, SDL_PixelFormat *format, unsigned flags);
void SDL_FreeSurface(SDL_Surface *surface);
int SDL_UpperBlit(SDL_Surface *src, SDL_Rect *srcRect, SDL_Surface *dst, SDL_Rect *dstRect);
int SDL_FillRect(SDL_Surface *dst, SDL_Rect *rect, unsigned color);
unsigned SDL_MapRGB(SDL_PixelFormat *format, unsigned char r, unsigned char g, unsigned char b);
unsigned SDL_MapRGBA(SDL_PixelFormat *format, unsigned char r, unsigned char g, unsigned char b,
                     unsigned char a);

// Events (SDL_Event *)
int SDL_PollEvent(void *event);
int SDL_WaitEvent(void *event);
int SDL_PushEvent(void *event);
void SDL_PumpEvents(void);
int SDL_PeepEvents(void *events, int count, int action, unsigned mask);

// Timers, threads and locks
void *SDL_AddTimer(unsigned interval, unsigned (*callback)(unsigned, void *), void *param);
int SDL_RemoveTimer(void *timer);
SDL_Thread *SDL_CreateThread(int (*fn)(void *), void *data);
void SDL_WaitThread(SDL_Thread *thread, int *status);
SDL_mutex *SDL_CreateMutex(void);
void SDL_DestroyMutex(SDL_mutex *mutex);
int SDL_mutexP(SDL_mutex *mutex);
int SDL_mutexV(SDL_mutex *mutex);
SDL_cond *SDL_CreateCond(void);
void SDL_DestroyCond(SDL_cond *cond);
int SDL_CondSignal(SDL_cond *cond);
int SDL_CondWait(SDL_cond *cond, SDL_mutex *mutex);
int SDL_CondWaitTimeout(SDL_cond *cond, SDL_mutex *mutex, unsigned ms);
void *SDL_RWFromFile(const char *file, const char *mode);

// SDL_image
SDL_Surface *IMG_Load(const char *file);

// SDL_ttf
int TTF_Init(void);
void TTF_Quit(void);
TTF_Font *TTF_OpenFont(const char *file, int size);
void TTF_CloseFont(TTF_Font *font);
void TTF_SetFontStyle(TTF_Font *font, int style);
int TTF_FontHeight(TTF_Font *font);
int TTF_SizeText(TTF_Font *font, const char *text, int *w, int *h);

// SDL_mixer
int Mix_OpenAudio(int frequency, unsigned short format, int channels, int chunkSize);
void Mix_CloseAudio(void);
int Mix_AllocateChannels(int count);
Mix_Music *Mix_LoadMUS(const char *file);
void Mix_FreeMusic(Mix_Music *music);
void Mix_FreeChunk(Mix_Chunk *chunk);
int Mix_PlayMusic(Mix_Music *music, int loops);
void Mix_PauseMusic(void);
int Mix_VolumeMusic(int volume);
int Mix_PlayChannelTimed(int channel, Mix_Chunk *chunk, int loops, int ticks);
int Mix_HaltChannel(int channel);
int Mix_Volume(int channel, int volume);

#ifdef __cplusplus
}
#endif
