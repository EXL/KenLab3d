#ifndef WIN32
#ifdef _WIN32
#define WIN32
#endif
#endif

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#ifdef _MSC_VER
#include <windows.h>
#else
/* No point in including windows.h just to get this. Saves a lot of bloat. */
#include <windef.h>
#endif
#endif

#ifdef USE_SDL2
#ifndef ANDROID_NDK
#include <SDL2/SDL.h>
#else
#include <SDL.h>

// Unix Compatibility
#define S_IREAD		S_IRUSR
#define S_IWRITE	S_IWUSR

#include <android/log.h>

#define LOG_TAG "KenLab3D"
#define LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#endif // !ANDROID_NDK

#include "sdl2keyhelper.h"

SDL_Window *globalWindow;
//SDL_GLContext *glContext;
#include <GLES/egl.h>
extern EGLDisplay  glDisplay;
extern EGLSurface  glSurface;
extern EGLConfig   glConfig;
extern EGLContext  glContext;

extern Uint16 *gammaRamp;
#else
#include <SDL/SDL.h>
#endif // USE_SDL2

#ifndef ANDROID_NDK
#define TO_DEBUG_LOG(...) fprintf(stderr, __VA_ARGS__)
#else
#define TO_DEBUG_LOG(...) LOGI(__VA_ARGS__)
#endif // !ANDROID_NDK

#ifndef OPENGLES
#include <GL/gl.h>
#include <GL/glu.h>
#else
#include <GLES/gl.h>
#ifndef ANDROID_NDK
#include <GLES/egl.h>
#include "../3rdparty/Glues/glues.h"
#else
#include <glues.h>
#endif // !ANDROID_NDK

#define GLdouble GLfloat
#define GL_CLAMP GL_CLAMP_TO_EDGE
#define GL_BGR GL_RGB
#endif // !OPENGLES

#ifndef GL_BGR
/* MSVC compatibility hack (some versions have out-of-date OpenGL headers). */
#define GL_BGR GL_BGR_EXT
#endif

#ifdef WIN32
#include "mmsystem.h"
#define S_IRGRP 0
#define S_IROTH 0
#else
#ifdef USE_OSS
#include "linux/soundcard.h"
#endif
#include <sys/ioctl.h>
/* WIN32 needs this flag to avoid data corruption, POSIX doesn't have it at
   all. So we define it ourselves on non-Windows systems. Yes, I know this is
   messy. */
#define O_BINARY 0
#endif

#ifdef DEBUG_BUFFERS
#ifdef MAIN
void SDL_GL_SwapBuffersDebug() {
#ifndef USE_SDL2
    SDL_GL_SwapBuffers();
#else
    SDL_GL_SwapWindow(globalWindow);
#endif // !USE_SDL2
    glClearColor(255,0,0,0);
    glClear(GL_COLOR_BUFFER_BIT);
}
#endif
#define SDL_GL_SwapBuffers SDL_GL_SwapBuffersDebug
#endif

extern void drawOnScreen();
extern void clearScreen();

/* Assume that SDL has fixed data types... */

typedef Uint32 K_UINT32;
typedef Uint16 K_UINT16;
typedef Sint32 K_INT32;
typedef Sint16 K_INT16;

#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <time.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif

/* Some (obsolete) port numbers... */

#define dataport 0x330
#define statport 0x331

/* Some other constants... */

#define numwalls 448
#define numoptions 4
#define numkeys 18

#define numaxes 4
#define numjoybuttons 32
#define numjoyaxes 6

#define initialwalls 0
#define gifbuflen 4096

/* Some texture names... */

//SORTWALLSTART
#define fountain 12        //l = 3
#define safe 16            //l = 3
#define clock 20
#define introstoryback 32
#define kenface 74
#define kenfaceouch 75
#define andy 76
#define andygone 77
#define map (lab3dversion?80:78)
#define invisible (lab3dversion?18:79)
#define goldlock 80
#define silverlock 81
#define doorside1 84
#define doorside2 84
#define doorside5 85
#define doorside3 86
#define doorside4 87
#define door1 88           //l = 6
#define door2 94           //l = 6
#define door3 100           //l = 8
#define door4 108          //l = 7
#define door5 115          //l = 8
#define bricksfull 130
#define brickshalf 131
#define slotto 134
#define slotup 135
#define slotinfo 137
#define soda 138
#define stairs 141
#define exitsign 143       //l = 2
#define tentacles 145      //l = 2
#define stairtop 151
#define minicolumn 152     //l = 4
#define tablecandle 163    //l = 2
#define target 166
#define net 167
#define fan 168            //l = 3
#define warp 170           //l = 2
#define coin 171
#define diamond 172
#define diamonds3 173
#define fries 174
#define emptyfries 175
#define meal 176
#define emptymeal 177
#define firstaid 178
#define emptyfirst 179
#define purple 180
#define emptypurple 181
#define green 182
#define emptygreen 183
#define gray 184
#define blue 185
#define grayblue 186
#define emptycoat 187
#define all4coats 188
#define emptyall4 189
#define goldkey 190
#define silverkey 191
#define emptykey 192
#define bul1get 193
#define bul1fly 194
#define bul2get 197
#define bul2fly 198
#define bul3get 200
#define bul3fly 201        //l = 6
#define bul3halfly 207     //l = 2
#define emptybulstand 209
#define lightning 210
#define extralife 211
#define getcompass 212
#define miniexplosion 213
#define explosion 214
#define hive 215
#define hivetohoney 216    //l = 6
#define honey 222
#define monbat 223         //l = 3
#define monske 226         //l = 3
#define monken 229         //l = 5
#define monwit 234
#define monbee 239         //l = 10
#define monspi 245         //l = 10
#define mongr2 251         //l = 18
#define mongre 262         //l = 6
#define monear 268         //l = 5
#define monand 273         //l = 5
#define monali 278         //l = 2
#define monbal 280         //l = 5
#define hole 285
#define monhol 286         //l = 4
#define mongho 290         //l = 3
#define monrob 293         //l = 10
#define monro2 303         //l = 8
#define mondog 311         //l = 15
#define monzor 341         //l = 10
#define monke2 356         //l = 14
#define monan2 370         //l = 9
#define monan3 376         //l = 9
#define bul4fly 385
#define bul5fly 386        //l = 2
#define bul6fly 388        //l = 4
#define bul7fly 390        //l = 3
#define bul8fly 393        //l = 4
#define bul10fly 395       //l = 3
#define bul9fly 397
#define bul11fly 398       //l = 7
#define monmum 407         //l = 8
#define skilblank 415
#define skilevel1 416
#define skilevel2 417
#define sodapics 418
#define copyright 419
#define labysign 420
#define congratsign 423
#define earth 424
#define youcheated 428
#define gameover 429
#define sharewaremessage 430
#define compassplc 431     //l = 4
#define statusbarback 435
#define statusbarinfo 436
#define coatfade 437
#define scorebox 438
#define menu 439
#define textwall 440       //l = 2
#define episodesign1off 442
#define episodesign2off 443
#define episodesign3off 444
#define episodesign1on 445
#define episodesign2on 446
#define episodesign3on 447
#define endtext 448
//SORTWALLEND

#define SDLKEYS 322

/* Global variables are defined in lab3d.c and extern in all other modules. */

#define TICKS_PER_SPRITE_FRAME 8

#ifdef MAIN
#define EXTERN
#else
#define EXTERN extern
#endif

/* Global variables that actually get initialised here... */

EXTERN const char *globalDataDir;
#ifdef ANDROID_NDK
EXTERN const char *globalAndroidRWdir;
#endif // ANDROID_NDK

#ifdef MAIN
EXTERN unsigned char bultype[26] =
    {0,1,1,1,1,2,1,2,1,2,1,2,1,2,1,2,2,2,2,2,2,2,2,1,2,1};
EXTERN unsigned char paldef[16][3] =
{
    {0,30,63},{28,34,60},{0,50,20},{41,52,28},{63,63,25},{63,63,63},
    {63,20,20},{63,0,63},
    {63,48,27},{63,40,25},{63,48,48},{45,63,45},{55,55,63},{63,40,63},
    {63,30,20},{55,25,30}
};

EXTERN unsigned char opaldef[16][3] =
{
  {0,30,63},{28,34,60},{0,50,20},{15,60,30},{63,63,25},{63,63,63},
  {63,20,20},{63,0,63},
  {63,32,0},{63,40,25},{63,48,48},{45,63,45},{0,0,63},{63,40,63},
  {63,30,20},{63,63,63}
};

EXTERN K_UINT16 pcfreq[63] =
{
    0,
    65,69,73,78,82,87,92,98,104,110,117,123,
    131,139,147,156,165,175,185,196,208,220,233,247,
    262,277,294,311,330,349,370,392,415,440,466,494,
    523,554,587,622,659,698,740,784,831,880,932,988,
    1047,1109,1175,1245,1319,1397,1480,1568,1661,1760,1864,1976,
    2094
};
EXTERN K_UINT16 adlibfreq[63] =
{
    0,
    2390,2411,2434,2456,2480,2506,2533,2562,2592,2625,2659,2695,
    3414,3435,3458,3480,3504,3530,3557,3586,3616,3649,3683,3719,
    4438,4459,4482,4504,4528,4554,4581,4610,4640,4673,4707,4743,
    5462,5483,5506,5528,5552,5578,5605,5634,5664,5697,5731,5767,
    6486,6507,6530,6552,6576,6602,6629,6658,6688,6721,6755,6791,
    7510
};
EXTERN K_UINT16 firstime = 1, quitgame = 0;
EXTERN K_INT16 midiinst = 0, cheatenable = 0, capturecount = 0;
EXTERN K_INT16 mainmenuplace = 0, loadsavegameplace = 0, gameheadstat = 0;
EXTERN K_INT16 newgameplace = 0, sodaplace = 0;
#else
EXTERN unsigned char paldef[16][3];
EXTERN unsigned char opaldef[16][3];
EXTERN K_UINT16 pcfreq[63];
EXTERN K_UINT16 adlibfreq[63];
EXTERN K_UINT16 firstime, quitgame;
EXTERN K_INT16 midiinst, cheatenable, capturecount;
EXTERN K_INT16 mainmenuplace, loadsavegameplace, gameheadstat;
EXTERN K_INT16 newgameplace, sodaplace;
#endif

/* Amount of split textures (transitions). */

#define numsplits 7

/* Graphics stuff... */
EXTERN unsigned char *lzwbuf, *pic, *walseg[numwalls+1];
EXTERN K_UINT16 *lzwbuf2;
EXTERN GLuint texName[numwalls+1];
EXTERN GLuint gameoversprite;
EXTERN GLuint splitTexName[numsplits][2];
EXTERN K_INT16 splitTexNum[numsplits];
EXTERN K_INT16 lborder[numwalls+1], rborder[numwalls+1];
EXTERN K_INT16 boleng[30*2], tileng[numwalls+1];
EXTERN K_INT32 tioffs[numwalls+1], walrec[numwalls+1];
EXTERN K_INT16 numcells, totalsortcnt, sortcnt;
EXTERN K_UINT16 sortx[512], sorty[512];
EXTERN K_INT16 sorti[512], sortbnum[512];
EXTERN char wallheader[numwalls+1], bmpkind[numwalls+1];

/* Monster stuff... */

EXTERN K_UINT16 mposx[512], mposy[512], mnum;
EXTERN K_UINT16 mgolx[512], mgoly[512], mrotbuf[2048];
EXTERN K_UINT16 moldx[512], moldy[512];
EXTERN K_INT16 mshock[512], mstat[512];

/* Screen stuff and a lot of other things... */

EXTERN K_INT16 vidmode, dside, scrsize;
EXTERN K_INT16 board[64][64], videotype, mute, numboards, skilevel;
EXTERN unsigned char option[numoptions], keydefs[numkeys];
EXTERN unsigned char tempbuf[4096], palette[768], mshot[512];
EXTERN char hiscorenam[16], hiscorenamstat, namrememberstat;
EXTERN K_INT16 cliptowall;
EXTERN K_UINT16 explox[16];
EXTERN K_UINT16 exploy[16];
EXTERN K_UINT32 explotime[16];
EXTERN K_UINT16 explostat[16];
EXTERN K_UINT16 explonum;

/* Temporary table... */
EXTERN K_INT16 lincalc[360];

EXTERN K_INT32 tantable[1025], sintable[2050];
EXTERN K_INT16 radarang[360];
EXTERN K_UINT16 halfheight;
EXTERN K_UINT16 times90[240], less64inc[64];
EXTERN K_INT16 moustat;
EXTERN int mousx, mousy, bstatus;
EXTERN K_INT16 joyx1, joyy1, joyx2, joyy2, joyx3, joyy3, joystat;
EXTERN unsigned char readch, oldreadch, keystatus[256];
EXTERN volatile K_INT16 clockspeed;
EXTERN K_INT16 slottime, slotpos[3], owecoins, owecoinwait;
EXTERN K_INT16 statusbar, statusbargoal, doorx, doory, doorstat;
EXTERN K_INT16 fadehurtval, fadewarpval;
EXTERN K_INT32 ototclock, totalclock, purpletime, greentime, capetime[2];
EXTERN K_INT32 scoreclock, scorecount;
EXTERN char textbuf[41];
EXTERN K_INT16 musicsource, midiscrap;
EXTERN K_UINT32 musicstatus, count, countstop;
EXTERN K_UINT16 numnotes, speed, drumstat, numchans, nownote;
EXTERN K_UINT32 *note;
EXTERN K_UINT32 chanage[18];
EXTERN unsigned char inst[256][11], databuf[512];
EXTERN K_INT32 gminst[256];
EXTERN unsigned char chanfreq[18], chantrack[18];
EXTERN unsigned char trinst[16], trquant[16], trchan[16];
EXTERN unsigned char trprio[16], trvol[16];
EXTERN K_UINT16 ksayfreq;
EXTERN unsigned char speechstatus;
EXTERN K_INT16 boardnum, oldlife, life, death;
EXTERN K_INT16 lifevests, lightnings, firepowers[3], keys[2];
EXTERN K_INT16 compass, cheated, coins, waterstat;
EXTERN K_INT16 animate2, animate3, animate4, oscillate3, oscillate5;
EXTERN K_INT16 animate6, animate7, animate8, animate10;
EXTERN K_INT16 animate11, animate15;
EXTERN char xwarp[16], ywarp[16], numwarps, justwarped;
EXTERN K_INT16 bulnum, bulang[64], bulkind[64], bulchoose;
EXTERN K_UINT16 bulx[64], buly[64];
EXTERN K_UINT32 bulstat[64], lastbulshoot, lastbarchange;
EXTERN K_UINT16 startx, starty, posx, posy, oldposx, oldposy;
EXTERN K_INT16 ang, startang, angvel, yourhereoldpos;
EXTERN K_INT16 vel, mxvel, myvel, svel, maxvel;
EXTERN K_INT16 posz, hvel, lastunlock, lastshoot, saidwelcome;
EXTERN K_UINT16 convavailpages, convwalls;
EXTERN char gamehead[8][27], gamexist[8];

/* SDL timer... */

EXTERN SDL_TimerID timer;

/* Overlay buffer... */
EXTERN unsigned char *screenbuffer;
EXTERN unsigned int *screenbuffer32;
EXTERN int screenbufferwidth,screenbufferheight;
EXTERN int screenwidth,screenheight;
EXTERN float virtualscreenwidth,virtualscreenheight;
EXTERN GLuint screenbuffertexture; /* One big texture. */
EXTERN GLuint screenbuffertextures[72]; /* 72 small textures. */
EXTERN int largescreentexture;
EXTERN int walltol,neardist;

/* MIDI stuff... */
#ifdef WIN32
EXTERN HMIDIOUT sequencerdevice;
#endif
#ifdef USE_OSS
EXTERN int sequencerdevice;
EXTERN int nrmidis;
#endif

/* Palette for graphics uploads... */
EXTERN unsigned char spritepalette[768];

EXTERN K_INT16 fadelevel,ingame;

void oldmain(void);
void UploadOverlay(void);
void UploadPartialOverlay(int x,int y,int w,int h);
void initialize();
K_INT16 vline(K_INT16,K_INT16,K_INT16,K_INT16,K_INT16);
void picrot(K_UINT16, K_UINT16, K_INT16, K_INT16);
void spridraw(K_INT16, K_INT16, K_INT16, K_INT16);
void pictur(K_INT16, K_INT16, K_INT16, K_INT16, K_INT16);
void doordraw(K_UINT16 x,K_UINT16 y,K_INT16 walnume,K_UINT16 posxs,
          K_UINT16 posys);
void statusbardraw(K_UINT16, K_UINT16, K_UINT16, K_UINT16, K_UINT16,
           K_UINT16, K_INT16);
void loadboard();
void loadtables();
K_INT16 ksay(K_UINT16);
void checkobj(K_UINT16, K_UINT16, K_UINT16, K_UINT16, K_INT16, K_INT16);
void addexplosion(K_UINT16 x, K_UINT16 y, K_UINT16 stat);
void linecompare(K_UINT16);
void drawmeter(int life, int oldlife, int x, int y);
void drawlife();
void loadwalls(int replace);
K_INT16 loadgame(K_INT16);
K_INT16 savegame(K_INT16);
void introduction(K_INT16);
K_INT16 loadmusic(char*);
void outdata(unsigned char, unsigned char, unsigned char);
void musicon();
void musicoff();
void setinst(unsigned char, K_INT16, unsigned char, unsigned char,
         unsigned char, unsigned char, unsigned char, unsigned char,
         unsigned char, unsigned char,
         unsigned char, unsigned char, unsigned char);
void setmidiinsts();
void checkhitwall(K_UINT16, K_UINT16, K_UINT16, K_UINT16);
void fade(K_INT16);
void showcompass(K_INT16);
K_INT16 kgif(K_INT16);
void setgamevideomode();
void textprint(K_INT16, K_INT16, char);
K_INT16 loadstory(K_INT16);
K_INT16 setupmouse();
void statusbaralldraw();
void hiscorecheck();
void setuptextbuf(K_INT32);
void getname();
void drawscore(K_INT32);
void drawtime(K_INT32);
void screencapture();
K_INT16 mainmenu();
K_INT16 getselection(K_INT16, K_INT16, K_INT16, K_INT16);
void drawmenu(K_INT16, K_INT16, K_INT16);
void finalisemenu();
void creditsmenu();
void helpmenu();
void orderinfomenu();
K_INT16 loadsavegamemenu(K_INT16);
K_INT16 newgamemenu();
void pressakey();
void wingame(K_INT16);
void winallgame();
K_INT16 areyousure();
void copyslots(K_INT16);
void youarehere();
void bigstorymenu();
void sodamenu();
Uint32 tickhandler(Uint32 interval, void *param);
void ksmhandler();
void SetVisibleScreenOffset(K_UINT16 offset);
void ShowPartialOverlay(int x,int y,int w,int h,int statusbar);
void PollInputs();
void checkGLStatus(char *file, int line);
void floorsprite(K_UINT16 x, K_UINT16 y, K_INT16 walnume);
void flatsprite(K_UINT16 x, K_UINT16 y,K_INT16 ang,K_INT16 playerang,
        K_INT16 walnume);
#ifdef MAIN

/* SDL to PC key mapping table... */

int PCkey[SDLKEYS]={
    -1,-1,-1,-1,-1,-1,-1,-1, /* 0-7 */
    0x0e, 0x0f, -1, -1, 0xfb, 0x1c, -1, -1, /* 8-15 */
    -1, -1, -1, 0, -1, -1, -1, -1, /* 16-23 */
    -1, -1, -1, 0x1, -1, -1, -1, -1, /* 24-31 */
    0x39, 0x2, 0x28, 0x4, 0x5, -1, 0x8, 0x28, /* 32-39 */
    0x0a, 0x0b, 0x09, 0x0d, 0x33, 0x0c, 0x34, 0x35, /* 40-47 */
    0x0b, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, /* 48-55 */
    0x09, 0x0a, 0x27, 0x27, 0x33, 0x0d, 0x34, 0x35, /* 56-63 */
    0x03, -1, -1, -1, -1, -1, -1, -1, /* 64-71 */
    -1, -1, -1, -1, -1, -1, -1, -1, /* 72-79 */
    -1, -1, -1, -1, -1, -1, -1, -1, /* 80-87 */
    -1, -1, -1, 0x1a, 0x2b, 0x1b, 0x07, 0x0c, /* 88-95 */
    0x29, 0x1e, 0x30, 0x2e, 0x20, 0x12, 0x21, 0x22, /* 96-103 */
    0x23, 0x17, 0x24, 0x25, 0x26, 0x32, 0x31, 0x18, /* 104-111 */
    0x19, 0x10, 0x13, 0x1f, 0x14, 0x16, 0x2f, 0x11, /* 112-119 */
    0x2d, 0x15, 0x2c, -1, -1, -1, -1, 0x53, /* 120-127 */
    /* 128-255 */
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    0x52, 0x4f, 0x50, 0x51, 0x4b, 0x4c, 0x4d, 0x47, /* 256-263 */
    0x48, 0x49, 0x53, 0xe0, 0x37, 0x4a, 0x4e, 0xe0, /* 264-271 */
    -1, 0x48+0x80, 0x50+0x80, 0x4d+0x80, 0x4b+0x80, 0x52+0x80, 0x47+0x80,
    0x4f+0x80, /* 272-279 */
    0x49+0x80, 0x51+0x80, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40, /* 280-287 */
    0x41, 0x42, 0x43, 0x44, 87, 88, 0xec, 0xed, /* 288-295 */
    0xee, -1, -1, -1, 69, 58, 70, 54, /* 296-303 */
    42, 0x80+29, 29, 0x80+56, 56, -1, -1, -1, /* 304-311 */
    -1, 0x80+56, 0x80+56, -1, 0x80+55, -1, 0, -1, /* 312-319 */
    -1, -1
};
#else
extern int PCkey[SDLKEYS];
#endif

/* Sound data. */
EXTERN unsigned char *SoundFile;

/* Sound output buffer... */
EXTERN K_INT16 *SoundBuffer;
EXTERN int FeedPoint;

/* Various graphics kludges. */
EXTERN int statusbaryoffset,spriteyoffset,visiblescreenyoffset,mixing;
EXTERN int statusbaryvisible;

/* Faster menu drawing... */
EXTERN int menuleft,menutop,menuwidth,menuheight,menuing;

#ifdef OPENGLES
enum MenuStates {
    eNoMenu,
    eMainMenu,
    eAbortMenu,
    eNewGameMenu,
    eHardnessMenu,
    eLoadSaveGameMenu,
    eSodaMenu,
    eSettingsMenu,
    eInputDevicesMenu,
    eSetupConfigureMenu,
    eSetupKeys,
    eSetupJButtons,
    eSetupJAxes,
    eSetupFiltering,
    eSetupMusic,
    eSetupSound,
    eSetupCheats,
    eSetupMusicCh,
    eSetupSoundCh,
    eSetupSoundBlock,
    eSetupScaling
};

EXTERN void clearCurrentMenuState();
EXTERN int currentMenuState;
EXTERN int qStateSaveOrLoad;

// extern settings
extern int inputdevice,resolutionnumber,nearest;
extern int music,sound,fullscr,cheat,channel,musicchannel;
extern int soundblock,timing,texturedepth,scaling;

extern char channelmenu[2][30];
extern char cheatmenu[3][30];
extern char filtermenu[3][30];
extern char fullscreenmenu[2][30];
extern char inputdevicemenu[4][30];
extern char musicmenu[3][30];
extern char resolutionspecialmenu[11][30];
extern char resolutionstandardmenu[8][30];
extern char scalingtypemenu[4][30];
extern char soundblockmenu[10][30];
extern char soundmenu[2][30];
extern char texturedepthmenu[3][30];
extern char configureinputmenu[4][30];
extern char keynames[numkeys][30];
extern char axisnames[numaxes][30];
extern char axisinst[numaxes][30];
extern char jaxisnames[numjoyaxes][30];

EXTERN void fillSettingMenuHelper(int count, char titles[][30]);
EXTERN void o_drawSetupJAxesHelper();
EXTERN void o_drawSetupJButtonsHelper();
EXTERN void o_drawSetupKeysHelper();
EXTERN void o_drawSetupConfigureMenuHelper();
EXTERN void o_drawSettingsMenuHelper();
EXTERN void o_drawSodaMenuHelper();
EXTERN void o_drawLoadSaveGameMenuHelper();
EXTERN void o_drawHardnessMenuHelper();
EXTERN void o_drawNewGameMenuHelper();
EXTERN void o_drawAbortMenuHelper();

EXTERN void copyFileFromAssetsToInternalRWDirAndroid(const char *source_file, const char *desc1, const char *desc2);
EXTERN void doVibrateFromJNI(int duration);
EXTERN int getHiresSettingsValue();
EXTERN int getSoundSettingsValue();
EXTERN int getMusicSettingsValue();
EXTERN int getVibarateDelayFromJNI();

EXTERN int c_VIBRATE_DELAY;
#endif // OPENGLES

/* Fade level... */
EXTERN GLfloat redfactor,bluefactor,greenfactor;

/* Shall we go fullscreen? */
EXTERN int fullscreen;

/* Filtering modes. */
EXTERN GLint fullfilter,partialfilter,anisotropic;

/* Joystick device. */
EXTERN SDL_Joystick *joystick;

void DumpSound(unsigned char *sound, K_UINT16 leng,K_UINT32 playpoint,int pos);
void AudioCallback(void *userdata, Uint8 *stream, int len);
void TextureConvert(unsigned char *from, unsigned char *to, K_INT16 type);
Uint16 getkeypress();
void drawtooverlay(K_UINT16 picx, K_UINT16 picy, K_UINT16 w,
           K_UINT16 h, K_UINT16 x, K_UINT16 y, K_INT16 walnum,
           unsigned char coloff);
void wipeoverlay(K_UINT16 x,K_UINT16 y,K_UINT16 w, K_UINT16 h);
void settransferpalette();
void setdarkenedpalette();
void updateoverlaypalette(K_UINT16 start,K_UINT16 amount,unsigned char *cols);
void TransitionTexture(int left,int texture,int right);
unsigned char readmouse(int *x,int *y);
unsigned char readjoystick(int *x, int *y);
void quit();
void loadsettings();
void savesettings();
void configure();
void reset_dsp();
void updatemap();
void updategameover();
void ShowStatusBar();
void setup();
void drawinputbox();
int getkeydefstat(int keydef);
void clearkeydefstat(int keydef);
int getaxispos(int axis, int keydef);

SDL_mutex *soundmutex,*timermutex;
EXTERN int newkeydefs[numkeys],newkeystatus[SDLKEYS];
EXTERN int buttondefs[numkeys],buttonstatus[numjoybuttons];
EXTERN int axisdefs[numaxes],axispos[numjoyaxes];

EXTERN int musicvolume,soundvolume;
EXTERN int channels;
K_INT16 ksaystereo(K_UINT16 filenum,K_UINT16 x,K_UINT16 y);
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
#define readLE16 read
#define readLE32 read
#define writeLE16 write
#define writeLE32 write
#else
ssize_t readLE16(int fd, void *buf, size_t count);
ssize_t readLE32(int fd, void *buf, size_t count);
ssize_t writeLE16(int fd, void *buf, size_t count);
ssize_t writeLE32(int fd, void *buf, size_t count);
#endif
EXTERN float gammalevel;


/* Data from wallparam.ini */


EXTERN int shadow[numwalls];
EXTERN double walltexcoord[numwalls][2];

#if SDL_BYTE_ORDER==SDL_LITTLE_ENDIAN
#define RED_SHIFT 0
#define GREEN_SHIFT 8
#define BLUE_SHIFT 16
#define ALPHA_SHIFT 24
#define REDCMP(X) ((X)&255)
#define GREENCMP(X) (((X)>>8)&255)
#define BLUECMP(X) (((X)>>16)&255)
#define ALPHACMP(X) (((X)>>24)&255)
#else
#define RED_SHIFT 24
#define GREEN_SHIFT 16
#define BLUE_SHIFT 8
#define ALPHA_SHIFT 0
#define REDCMP(X) (((X)>>24)&255)
#define GREENCMP(X) (((X)>>16)&255)
#define BLUECMP(X) (((X)>>8)&255)
#define ALPHACMP(X) ((X)&255)
#endif


/* Size of block for sound device. */
#define SOUNDBLOCKSIZE11KHZ 256
#define SOUNDBLOCKSIZE44KHZ 1024
EXTERN int soundblocksize;

/* Experimental sound buffer timing method variables. */
EXTERN int soundtimer, soundbytespertick,soundtimerbytes;

/* Sound effect and music pan enable toggles. */

EXTERN int soundpan,musicpan;

/* Colour depth. */
EXTERN GLint colourformat;

/* Toggle extreme debug output (slows things down if everything works).*/
EXTERN int debugmode;

/* LAB3D version to emulate (0 = v2.1 (Epic reg.), 1 = v1.1 (AdvSys reg.)). */
EXTERN int lab3dversion;

/* Things that aren't constants anymore really... */
EXTERN int rnumwalls;

/* Monster board (v1.x only)... */
EXTERN unsigned char mboard[64][64];

/* Some v1.x flags... */
EXTERN K_INT16 fanpos, kenpos, kenpos2, ballpos, heatpos, warpos, rogermode;

/* Read clock speed. */
EXTERN K_INT16 clockspd;

void updateclock(void);

/* Aspect ratio correction factors for ingame view (extra width and height). */
EXTERN GLdouble aspw, asph;
