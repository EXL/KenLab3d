#include "lab3d.h"
#include "adlibemu.h"
#include <math.h>
#include <ctype.h>

#ifdef USE_SDL2
#ifndef ANDROID_NDK
#include <SDL2/SDL_endian.h>
#include <SDL2/SDL_image.h>
#else
#include <SDL_endian.h>

#include "lodepng.h" // Use LodePNG library
#endif // !ANDROID_NDK
#else
#include <SDL/SDL_endian.h>
#include <SDL/SDL_image.h>
#endif // USE_SDL2

/* Various constants that really should be stored in the data files... */

#define giflen1 7649
#define giflen2 8258

/* Stack size in LZW. Probably overkill. */

#define LZW_STACK_SIZE 4096

/* Last time tick handler was called... */
static Uint32 lastTick=0;

/* Fractional part of milliseconds. */
static Uint32 tickFrac=0;

/* Sequencer buffer. Completely overkill, unless Ken wants to play hundreds of
   notes at once. */

#ifdef USE_OSS
SEQ_DEFINEBUF (2048);
#endif

/* Palette for OpenGL transfer... */

static GLfloat Red[256],Blue[256],Green[256];
static GLfloat Alpha[256];

static unsigned char ipalr[256],ipalg[256],ipalb[256];

static char BADNAME[13]="MESTINXBADLY";

#ifdef USE_OSS
/* Linux sequencer write routine... */

void seqbuf_dump ()
{
    if (_seqbufptr)
    if (write (sequencerdevice, _seqbuf, _seqbufptr) == -1)
    {
        fprintf (stderr,"write /dev/sequencer failed.\n");
        SDL_Quit();
        exit (-1);
    }
    _seqbufptr = 0;
}
#endif

/* Check OpenGL status and complain if necessary. */
void checkGLStatus(char* file, int line)
{
    GLenum errCode;
    const GLubyte *errString;

    if ((errCode=glGetError())!=GL_NO_ERROR) {
    errString=gluErrorString(errCode);
    TO_DEBUG_LOG("OpenGL Error: %s file: %s line: %d\n", errString, file, line);
    }
}

void ConvertPartialOverlay(int x, int y, int w, int h);

/* Endian-converting reads. */

#if SDL_BYTEORDER != SDL_LIL_ENDIAN
#define SWAPBLOCK16 for(a=0;a<(cnt>>1);a++) *(b+a)=SDL_Swap16(*(b+a));
#define SWAPBLOCK32 for(a=0;a<(cnt>>2);a++) *(b+a)=SDL_Swap32(*(b+a));

ssize_t readLE16(int fd, void *buf, size_t cnt) {
    ssize_t r=read(fd,buf,cnt);
    K_UINT16 *b=buf;
    int a;

    SWAPBLOCK16;
    return r;
}
ssize_t readLE32(int fd, void *buf, size_t cnt) {
    ssize_t r=read(fd,buf,cnt);
    K_UINT32 *b=buf;
    int a;

    SWAPBLOCK32;
    return r;
}

ssize_t writeLE16(int fd, void *buf, size_t cnt) {
    ssize_t r;
    K_UINT16 *b=buf;
    int a;

    SWAPBLOCK16;
    r=write(fd,buf,cnt);
    SWAPBLOCK16;

    return r;
}
ssize_t writeLE32(int fd, void *buf, size_t cnt) {
    ssize_t r;
    K_UINT32 *b=buf;
    int a;

    SWAPBLOCK32;
    r=write(fd,buf,cnt);
    SWAPBLOCK32;
    return r;
}
#endif

K_INT32 readlong(unsigned char *t) {
    return((*t)|((*(t+1))<<8)|((*(t+2))<<16)|((*(t+3))<<24));
}

void writelong(unsigned char *t,K_INT32 v) {
    *t=v&255;
    *(t+1)=(v>>8)&255;
    *(t+2)=(v>>16)&255;
    *(t+3)=(v>>24)&255;
}

K_UINT16 readshort(unsigned char *t) {
    return((*t)|((*(t+1))<<8));
}

void writeshort(unsigned char *t,K_UINT16 v) {
    *t=v&255;
    *(t+1)=(v>>8)&255;
}

int getkeydefstat(int keydef) {
    int key=newkeystatus[newkeydefs[keydef]];
    if (key) return key;
    if (keydef == 8 || keydef == 12) return 0;
    if (buttondefs[keydef]!=-1) {
    return buttonstatus[buttondefs[keydef]];
    }
    return 0;
}
int getaxispos(int axis, int deadzone) {
    if (axisdefs[axis]!=0) {
    int ret=axispos[abs(axisdefs[axis])-1];
    if (abs(ret)<deadzone)
        return 0;
    return axisdefs[axis]<0?-ret:ret;
    }
    return 0;
}
void clearkeydefstat(int keydef) {
    newkeystatus[newkeydefs[keydef]] = 0;
    if (buttondefs[keydef]!=-1) {
    buttonstatus[buttondefs[keydef]] = 0;
    }
}

void BuildMipmaps(Uint32* pix, int w, int h, int hasalpha, int maxlevel);

/* Load a board. Uses LZW compression. Get a lawyer. */

void loadboard()
{
    unsigned char bitcnt, numbits;
    K_INT16 i, j, k, fil, bytecnt1, bytecnt2;
    K_INT16 currstr, strtot, compleng, dat, goalstr;
    K_INT32 templong;
    K_INT16 prepdie=0;

    K_UINT16 stack[LZW_STACK_SIZE];
    K_UINT16 stackp=0;

#if SDL_BYTEORDER != SDL_LIL_ENDIAN
    int a; K_UINT16 *b; size_t cnt;
#endif

    unsigned char *RGBATexture=malloc(64*64*4);

    if (lab3dversion) {
        char path1[256];
        char path2[256];
        snprintf(path1, sizeof(path1), "%s/boards.dat", globalDataDir);
        snprintf(path2, sizeof(path2), "%s/BOARDS.DAT", globalDataDir);
#ifdef ANDROID_NDK
    SDL_RWops *io = SDL_RWFromFile(path2, "rb");
    if (!io) {
        io = SDL_RWFromFile(path1, "rb");
    }
    if (io) {
        SDL_RWseek(io, ((long)boardnum)<<13,SEEK_SET);
        SDL_RWread(io, &board[0],8192, 1);
        SDL_FreeRW(io);
#else
    if (((fil = open(path1,O_RDONLY|O_BINARY,0)) != -1)||
        ((fil = open(path2,O_RDONLY|O_BINARY,0)) != -1)) {
        lseek(fil,((long)boardnum)<<13,SEEK_SET);
        read(fil,&board[0],8192);
        close(fil);
#endif // ANDROID_NDK
        numwarps=0;
        justwarped=0;
    } else {
        TO_DEBUG_LOG("Can't find boards.dat.\n");
        SDL_Quit();
        exit(1);
    }
    } else {
        char path1[256];
        char path2[256];
        snprintf(path1, sizeof(path1), "%s/boards.kzp", globalDataDir);
        snprintf(path2, sizeof(path2), "%s/BOARDS.KZP", globalDataDir);
#ifdef ANDROID_NDK
        SDL_RWops *io = SDL_RWFromFile(path2, "rb");
        if (!io) {
            io = SDL_RWFromFile(path1, "rb");
        }
        if (io) {
#else
        if (((fil = open(path1,O_RDONLY|O_BINARY,0)) != -1)||
        ((fil = open(path2,O_RDONLY|O_BINARY,0)) != -1))
        {
#endif // ANDROID_NDK
        prepdie = 0;
        numwarps = 0;
        justwarped = 0;
        readLE16(fil,&boleng[0],30*4);
        templong = (long)(30*4);
        for(i=0;i<(boardnum<<1);i++)
        templong += ((long)(boleng[i]+2));
#ifndef ANDROID_NDK
        lseek(fil,templong,SEEK_SET);
#else
        SDL_RWseek(io, templong, SEEK_SET);
#endif // ANDROID_NDK
        for(i=1;i<=256;i++) {
        lzwbuf[i]=i&255;
        lzwbuf2[i]=i;
        }
        lzwbuf2[0]=0;
        lzwbuf[0]=0;

        for(i=0;i<2;i++)
        {
        compleng = boleng[(boardnum<<1)+i];
#ifndef ANDROID_NDK
        readLE16(fil,&strtot,2);
        read(fil,&tempbuf[0],compleng);
#else
        SDL_RWread(io, &strtot, 2, 1);
        SDL_RWread(io, &tempbuf[0],compleng, 1);
#endif // ANDROID_NDK
        if (strtot > 0)
        {
            tempbuf[compleng] = 0;
            tempbuf[compleng+1] = 0;
            tempbuf[compleng+2] = 0;
            bytecnt2 = 0;
            bytecnt1 = 0;
            bitcnt = 0;
            currstr = 256;
            goalstr = 512;
            numbits = 9;
            do
            {
            dat=(((tempbuf[bytecnt2])|
                  ((K_UINT16)(tempbuf[bytecnt2+1]))<<8)>>bitcnt)&
                ((1<<numbits)-1);

/*		    dat=((*((K_UINT16 *)(tempbuf+bytecnt2)))
            >>bitcnt)&
            ((1<<numbits)-1);*/
            if (bitcnt+numbits>16) {
                dat+=(((K_UINT16)tempbuf[bytecnt2+2])&
                  ((1<<((bitcnt+numbits)&15))-1))<<
                (16-bitcnt);
            }

            bitcnt+=numbits;
            bytecnt2+=bitcnt>>3;
            bitcnt&=7;

            lzwbuf2[currstr]=dat;

            while(dat>=256) {
                stack[stackp++]=lzwbuf[dat];

                dat=lzwbuf2[dat];
            }

            lzwbuf[currstr-1]=dat;
            lzwbuf[currstr]=dat;

            dat=lzwbuf2[dat];
            stack[stackp++]=dat;

            while(stackp>0) {
                stackp--;
                if (bytecnt1<4096)
                ((unsigned char *)(board[32*i]))[bytecnt1++]=
                    stack[stackp];
            }

            currstr++;
            if (currstr == goalstr)
            {
                numbits++;
                goalstr = (goalstr<<1);
            }
            }
            while (currstr <= strtot);
        }
        else {
            memcpy(board[32*i], tempbuf, 4096);
        }
        }
#ifndef ANDROID_NDK
        close(fil);
#else
        SDL_FreeRW(io);
#endif // ANDROID_NDK
    } else {
        TO_DEBUG_LOG("Can't find boards.kzp.\n");
        SDL_Quit();
        exit(1);
    }

    }
#if SDL_BYTEORDER != SDL_LIL_ENDIAN
    b=(K_UINT16 *)board;
    cnt=8192;
    SWAPBLOCK16;
#endif

    /* Generate map texture... */

    for(i=0;i<4096;i++)
    walseg[map-1][i]=board[0][i]&255;

    if (lab3dversion) {
    spritepalette[0]=63;
    spritepalette[1]=63;
    spritepalette[2]=63;
    }
    glBindTexture(GL_TEXTURE_2D,texName[map-1]);
    checkGLStatus( __FILE__, __LINE__ );

    TextureConvert(walseg[map-1], RGBATexture, bmpkind[map]);

    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,fullfilter);

#ifndef OPENGLES
    glPixelStorei(GL_UNPACK_ROW_LENGTH,0);
#endif // !OPENGLES
    checkGLStatus( __FILE__, __LINE__ );
    glPixelStorei(GL_UNPACK_ALIGNMENT,1);
    checkGLStatus( __FILE__, __LINE__ );

    BuildMipmaps((Uint32*)RGBATexture,64,64,1,10);
    checkGLStatus( __FILE__, __LINE__ );

    /* Place warps and monsters... */

    mnum = 0;
    if (lab3dversion)
    for(i=0;i<63;i++)
        for(j=0;j<63;j++)
        {
        mboard[i][j] = 0;
        k = board[i][j]&1023;
        if ((k == 123) && (numwarps < 16))
        {
            xwarp[(int)numwarps] = (char)i;
            ywarp[(int)numwarps] = (char)j;
            numwarps++;
        }
        if ((k == 66) || (k == 38) || (k == 54) || (k == 68) || (k == 94) || (k == 98) || (k == 109) || (k == 160) || (k == 165) || (k == 187))
        {
            mposx[mnum] = (i<<10)+512;
            mposy[mnum] = (j<<10)+512;
            mgolx[mnum] = mposx[mnum];
            mgoly[mnum] = mposy[mnum];
            moldx[mnum] = mposx[mnum];
            moldy[mnum] = mposy[mnum];
            mstat[mnum] = board[i][j]&255;
            mshock[mnum] = 0;
            if (k == 66)
            mshot[mnum] = 1;
            if (k == 68)
            mshot[mnum] = 255;
            if (k == 38)
            mshot[mnum] = 2;
            if (k == 54)
            mshot[mnum] = 5;
            if (k == 94)
            {
            mshot[mnum] = 10;
            prepdie |= 2;
            }
            if (k == 98)
            mshot[mnum] = 255;
            if (k == 109)
            {
            mshot[mnum] = 100;
            prepdie |= 1;
            }
            if (k == 160)
            mshot[mnum] = 1;
            if (k == 165)
            mshot[mnum] = 1;
            if (k == 187)
            mshot[mnum] = 1;
            mnum++;
            mboard[i][j] = k;
            board[i][j] = 1024;
        }
        if ((board[i][j]&4096) > 0)
        {
            posx = (i<<10)+512;
            posy = (j<<10)+512;
            ang = ((board[i][j]&3)<<9);
            startx = posx;
            starty = posy;
            startang = ang;
            board[i][j] = 62+1024;
        }
        }
    else
    for(i=0;i<63;i++)
        for(j=0;j<63;j++)
        {
        board[i][j] &= 0xbfff;
        k = board[i][j]&1023;
        if ((k == warp) && (numwarps < 16))
        {
            xwarp[(int)numwarps] = (char)i;
            ywarp[(int)numwarps] = (char)j;
            numwarps++;
        }
        if ((k == monken) || (k == mongre) || (k == monand) ||
            (k == monbal) || (k == monali) || (k == monhol) ||
            (k == monzor) || (k == monbat) || (k == mongho) ||
            (k == monske) || (k == monke2) || (k == monan2) ||
            (k == monan3) || (k == monwit) || (k == monbee) ||
            (k == monspi) || (k == mongr2) || (k == monear) ||
            (k == monrob) || (k == monro2) || (k == mondog) ||
            (k == monmum) || (k == hive))
        {
            mposx[mnum] = (i<<10)+512;
            mposy[mnum] = (j<<10)+512;
            mgolx[mnum] = mposx[mnum];
            mgoly[mnum] = mposy[mnum];
            moldx[mnum] = mposx[mnum];
            moldy[mnum] = mposy[mnum];
            mstat[mnum] = k;
            mshock[mnum] = 0;
            if (k == monken) mshot[mnum] = 1;
            if (k == monbal) mshot[mnum] = 255;
            if (k == mongre) mshot[mnum] = 2;
            if (k == mongr2) mshot[mnum] = 3;
            if (k == monrob) mshot[mnum] = 5;
            if (k == monwit) mshot[mnum] = 1;
            if (k == monand) mshot[mnum] = 5;
            if (k == monali) mshot[mnum] = 10, prepdie |= 2;
            if (k == monhol) mshot[mnum] = 255;
            if (k == monzor) mshot[mnum] = 48, prepdie |= 1;
            if (k == monbat) mshot[mnum] = 1;
            if (k == monear) mshot[mnum] = 1;
            if (k == monbee) mshot[mnum] = 1;
            if (k == monspi) mshot[mnum] = 1;
            if (k == mongho) mshot[mnum] = 1;
            if (k == monske) mshot[mnum] = 1;
            if (k == monmum) mshot[mnum] = 2;
            if (k == monke2) mshot[mnum] = 48, prepdie |= 1;
            if (k == monan2) mshot[mnum] = 48, prepdie |= 1;
            if (k == monan3) mshot[mnum] = 1;
            if (k == hive) mshot[mnum] = 15;
            if (k == monro2) mshot[mnum] = 2;
            if (k == mondog) mshot[mnum] = 1;
            mnum++;
            board[i][j] = 16384+1024;
        }
        if ((board[i][j]&4096) > 0)
        {
            posx = (i<<10)+512;
            posy = (j<<10)+512;
            yourhereoldpos = ((posx>>10)<<6)+(posy>>10);
            youarehere();
            ang = ((board[i][j]&3)<<9);
            startx = posx;
            starty = posy;
            startang = ang;
            board[i][j] = stairtop+1024;
        }
        }
    if ((prepdie&1) > 0)
    ksay(21);
    else if ((prepdie&2) > 0)
    ksay(20);
    posz = 32;
    angvel = 0;
    vel = 0;
    mxvel = 0;
    myvel = 0;
    svel = 0;
    hvel = 0;
    for(i=0;i<32;i++)
    bulstat[i] = 0;
    lastbulshoot = 0;
    bulnum = 0;
    keys[0] = 0;
    keys[1] = 0;
    death = 4095;
    free(RGBATexture);
}

/* Load tables and settings. The settings are ignored. */

void loadtables()
{
    K_INT16 fil;

    char path1[256];
    char path2[256];
    snprintf(path1, sizeof(path1), "%s/tables.dat", globalDataDir);
    snprintf(path2, sizeof(path2), "%s/TABLES.DAT", globalDataDir);
#ifdef ANDROID_NDK
    SDL_RWops *io = SDL_RWFromFile(path2, "rb");
    if (!io) {
        io = SDL_RWFromFile(path1, "rb");
    }
    if (io) {
    SDL_RWread(io, &sintable[0], 8192, 1);
    SDL_RWread(io, &tantable[0], 4096, 1);
    SDL_RWread(io, &radarang[0], 720, 1);
    SDL_FreeRW(io);
#else
    if (((fil = open(path1,O_RDONLY|O_BINARY,0)) != -1)||
    ((fil = open(path2,O_RDONLY|O_BINARY,0)) != -1))
    {
    readLE32(fil,&sintable[0],8192);
    readLE32(fil,&tantable[0],4096);
    readLE16(fil,&radarang[0],720);

/*	read(fil,&option[0],numoptions);
    read(fil,&keydefs[0],numkeys);
    readLE16(fil,&joyx1,2);
    readLE16(fil,&joyy1,2);
    readLE16(fil,&joyx2,2);
    readLE16(fil,&joyy2,2);
    readLE16(fil,&joyx3,2);
    readLE16(fil,&joyy3,2);
    readLE16(fil,&ksayfreq,2);*/
    close(fil);
#endif // ANDROID_NDK
    /* Override joystick values with SDL limits... */

    joyx1=joyy1=-32768;
    joyx2=joyy2=0;
    joyx3=joyy3=32767;

    } else {
    TO_DEBUG_LOG("Can't find tables.dat.\n");
    SDL_Quit();
    exit(1);
    }
}

/* ksay sound filenum. pan = 0 for left, 128 centre, 256 right. */

K_INT16 ksaypan(K_UINT16 filenum,K_UINT16 pan) {
    K_INT16 numfiles;
    K_UINT16 leng;
    K_INT32 sndfiloffs;
    K_INT32 blocksize=(musicsource==2)?SOUNDBLOCKSIZE44KHZ:SOUNDBLOCKSIZE11KHZ;

    if (!soundpan) pan=128;

    SDL_LockMutex(soundmutex);
    if ((speechstatus == 0) || (mute == 1)) {
    SDL_UnlockMutex(soundmutex);
    return(-1);
    }
    numfiles=readshort(SoundFile);
    if (filenum >= numfiles) {
    SDL_UnlockMutex(soundmutex);
    return(-1);
    }

    sndfiloffs=readlong(SoundFile+(2+filenum*6));
    leng=readshort(SoundFile+(6+filenum*6));

#ifdef ANDROID_NDK
    if (filenum == 14) { /* 14 - ouch! sound */
        doVibrateFromJNI(c_VIBRATE_DELAY / 3);
    } else if (filenum == 10) { /* 10 - explosion sound */
        doVibrateFromJNI(c_VIBRATE_DELAY);
    }
#endif // ANDROID_NDK

    DumpSound(SoundFile+sndfiloffs, leng,((FeedPoint+blocksize)&65535),pan);
    SDL_UnlockMutex(soundmutex);
    return 0;
}

/* ksay sound filenum from position x,y in game. */

K_INT16 ksaystereo(K_UINT16 filenum,K_UINT16 x,K_UINT16 y) {
    K_INT32 k,m,dir,pan,templong;

    if ((x==posx)&&(y==posy)) return(ksay(filenum));

    k = 512;
    if (x != posx)
    {
    templong = (((((K_INT32)y-(K_INT32)posy)<<12)/
             ((K_INT32)x-(K_INT32)posx))<<4);
    if (templong < 0)
        k = 768;
    else
        k = 256;
    for (m=128;m>0;m>>=1)
    {
        if (tantable[k] < templong)
        k += m;
        else
        k -= m;
    }
    }
    if (y > posy)
    k += 1024;

    dir=(k+2048-ang)&2047;
    dir^=2047;

    if (dir>=1536)
    pan=(dir-1536)>>2; /* Forward left */
    else if (dir>=512)
    pan=(1536-dir)>>2; /* Back */
    else pan=128+(dir>>2); /* Forward right */

    return ksaypan(filenum,pan);
}

/* Play a digital sound... */

K_INT16 ksay(K_UINT16 filenum)
{
    return(ksaypan(filenum,128));
}

/* Wipe digital sound buffer... */

void reset_dsp()
{
    memset(SoundBuffer,0,65536*2);
    FeedPoint=0;
}

#ifndef min
int min(long a,long b) {
    return((a<b)?a:b);
}
#endif

static long minicnt = 0;
void preparesound(void *dasnd, long numbytestoprocess)
{
    long i, prepcnt;

    prepcnt = numbytestoprocess;
    while (prepcnt > 0)
    {
    i = min(prepcnt,(minicnt/speed+4)&~3);
    adlibgetsample(dasnd,i);
    dasnd = (void *)(((long)dasnd)+i);
    prepcnt -= i;

    minicnt -= speed*i;
    while (minicnt < 0)
    {
        minicnt += 44100*channels*2;
        ksmhandler();
    }
    }
}

/* SDL audio callback. Feed a chunk from sound buffer. */

void AudioCallback(void *userdata, Uint8 *stream, int len) {
    int rl;
    int i;
    int t=0;
    int j1,j2;

    if (soundtimer) {
    soundtimerbytes+=len;
    SDL_LockMutex(timermutex);
    while(soundtimerbytes>=soundbytespertick) {
        soundtimerbytes-=soundbytespertick;
        clockspeed++;
    }
    SDL_UnlockMutex(timermutex);
    }

    SDL_LockMutex(soundmutex);

    if (musicsource==2) len>>=2;

    len>>=1;
    rl=len;

    if (FeedPoint+rl>=65536) rl=65536-FeedPoint;

    if (musicsource==2) {
    /* mute=2: stop music, but don't mute. */
    if ((mute!=1)&&musicstatus)
        preparesound (stream, rl*2*4);
    else
        memset(stream,0,rl*2*4);
    j1=FeedPoint;
    j2=(FeedPoint+channels)&65535;

    if (mute!=1) {
        for(i=0;i<(len<<2);i++) {
        if (channels==1)
            switch(i&3) {
            case 0:
                t=SoundBuffer[j1]+((Sint16 *)stream)[i];
                break;
            case 1:
                t=3*(SoundBuffer[j1]>>2)+(SoundBuffer[j2]>>2)+
                ((Sint16 *)stream)[i];
                break;
            case 2:
                t=(SoundBuffer[j1]>>1)+(SoundBuffer[j2]>>1)+
                ((Sint16 *)stream)[i];
                break;
            case 3:
                t=(SoundBuffer[j1]>>2)+3*(SoundBuffer[j2]>>2)+
                ((Sint16 *)stream)[i];
                j1++;
                j2++;
                j2&=65535;
                break;
            }
        else
            switch(i&7) {
            case 0:
            case 1:
                t=SoundBuffer[j1+(i&1)]+((Sint16 *)stream)[i];
                break;
            case 2:
            case 3:
                t=3*(SoundBuffer[j1+(i&1)]>>2)+
                (SoundBuffer[j2+(i&1)]>>2)+
                ((Sint16 *)stream)[i];
                break;
            case 4:
            case 5:
                t=(SoundBuffer[j1+(i&1)]>>1)+
                (SoundBuffer[j2+(i&1)]>>1)+
                ((Sint16 *)stream)[i];
                break;
            case 6:
            case 7:
                t=(SoundBuffer[j1+(i&1)]>>2)+
                3*(SoundBuffer[j2+(i&1)]>>2)+
                ((Sint16 *)stream)[i];
                if ((i&7)==7) {
                j1+=2;
                j2+=2;
                j2&=65535;
                }
                break;
            }
        if (t<-32768) t=-32768;
        if (t>32767) t=32767;
        ((Sint16 *)stream)[i]=t;
        }
    }
    }
    else
    memcpy(stream, SoundBuffer+FeedPoint, rl*2);
    memset(SoundBuffer+FeedPoint,0,rl*2);

    FeedPoint+=rl;
    FeedPoint&=65535;

    SDL_UnlockMutex(soundmutex);

    if (rl<len) {
    if (musicsource==2)
        AudioCallback(userdata, stream+rl, len-rl);
    else
        AudioCallback(userdata, stream+(rl<<2), (len-rl)<<2);
    }
}

/* Copy sound to sound buffer. */

void DumpSound(unsigned char *sound,K_UINT16 leng,K_UINT32 playpoint,int pan) {
    K_INT32 a,t;

    K_UINT32 e1,pl;

    if (channels==1) {
    e1=playpoint+leng;
    if (e1>=65536) e1=65536;
    pl=e1-playpoint;

    for(a=playpoint;a<e1;a++) {
        t=SoundBuffer[a]+(sound[a-playpoint]-128)*soundvolume;
        if (t<-32768) t=-32768;
        if (t>32767) t=32767;
        SoundBuffer[a]=t;
    }
    } else {
    e1=playpoint+(leng<<1);
    if (e1>=65536) e1=65536;
    pl=(e1-playpoint)>>1;

    for(a=playpoint;a<e1;a++) {
        t=((a-playpoint)&1)?pan:(256-pan);
        t=SoundBuffer[a]+
        (((sound[(a-playpoint)>>1]-128)*soundvolume*t)>>7);
        if (t<-32768) t=-32768;
        if (t>32767) t=32767;
        SoundBuffer[a]=t;
    }
    }

    if (pl<leng) DumpSound(sound+pl, leng-pl, 0,pan);
}

/* Check if object is visible... */

void checkobj(K_UINT16 x,K_UINT16 y,K_UINT16 posxs,K_UINT16 posys,
          K_INT16 angs,K_INT16 num)
{
    K_INT16 angle, siz, ysiz;

    if (shadow[num&1023]) {
    checkobj(x,y,posxs,posys,angs,shadow[num&1023]);
    }
    siz = (int)(((((long)x-(long)posxs)>>2)*sintable[(angs+512)&2047]+(((long)y-(long)posys)>>2)*sintable[angs])>>16);
    if (siz != 0)
    {
        ysiz = (int)(((((long)x-(long)posxs)>>2)*sintable[angs]-(((long)y-(long)posys)>>2)*sintable[(angs+512)&2047])>>16);
        angle = (K_INT16)(180.0-((180.0*(double)ysiz)/(double)siz/aspw));
        siz = (int)(163840L/((long)siz));
        sortx[sortcnt] = x;
        sorty[sortcnt] = y;
        if (bmpkind[num&1023]==5) {
        sorti[sortcnt]=0;
        } else {
        sorti[sortcnt]=163840L/sqrt((((long)x-(long)posxs)*
                         ((long)x-(long)posxs))+
                        (((long)y-(long)posys)*
                         ((long)y-(long)posys)))*4;
        }
        sortbnum[sortcnt] = (num&1023);
        if (((angle+(siz>>3)) >= -2) && ((angle-(siz>>3)) <= 362) && ((siz&0xc000) == 0))
        sortcnt++;
    }
}

void addexplosion(K_UINT16 x, K_UINT16 y, K_UINT16 stat) {
    explotime[explonum]=totalclock+16;
    explox[explonum]=x;
    exploy[explonum]=y;
    explostat[explonum]=stat;
    explonum++;
}


/* Set status bar position... */

void linecompare(K_UINT16 lin)
{

    statusbaryvisible=240-((lin+1)>>1);

}

/* Draw energy meter... */

void drawmeter(int life, int oldlife, int x, int y)
{
    K_INT16 lifespot, olifespot;

    if (life < 0) life = 0;
    if (life > 4095) life = 4095;
    lifespot = (life>>6);
    olifespot = (oldlife>>6);
    if (lifespot < olifespot)
    statusbardraw(lifespot,57,olifespot-lifespot+1,7,lifespot+x,y,statusbarinfo);
    else if (lifespot > olifespot)
    statusbardraw(olifespot,50,lifespot-olifespot+1,7,olifespot+x,y,statusbarinfo);
}
void drawlife()
{
    if (statusbar == 479)
    return;
    drawmeter(life,oldlife,128,4+statusbaryoffset);
    oldlife = life;
}

int powerof2 (int in)
{
    int i = 0;
    in--;
    while (in) {
    in >>= 1;
    i++;
    }
    return 1 << i;
}


typedef struct imgcache {
    char* name;
    int w,h;
    //double tcx,tcy;
    GLuint texnum;
    struct imgcache* next;
} imgcache;


static imgcache* img_cache=NULL;

#ifndef OPENGLES
static inline int AverageColour32 (Uint32 * p, int x, int y, int w, int h)
{
    int a, c, n = 0;
    int r = 0, g = 0, b = 0;
    for (a = -1; a <= 1; a++) {
    if (x + a < 0)
        continue;
    if (x + a >= w)
        continue;
    for (c = -1; c <= 1; c++) {
        if (y + c < 0)
        continue;
        if (y + c >= h)
        continue;
        if (ALPHACMP (p[(x + a) + w * (y + c)]) > 0) {
        Uint32 t = p[(x + a) + w * (y + c)];
        b += BLUECMP (t);
        g += GREENCMP (t);
        r += REDCMP (t);
        n++;
        }
    }
    }
    if (n > 0)
    return ((b / n) << BLUE_SHIFT) + ((g / n) << GREEN_SHIFT) +
        ((r / n) << RED_SHIFT);
    else
    return 0;
}

static inline void TextureAvg32 (Uint32 * pic, int w, int h)
{
    Uint32 *f = pic;

    int x, y;

    for (y = 0; y < h; y++)
    for (x = 0; x < w; x++) {
        if (ALPHACMP (*f) == 0) {
        *f = AverageColour32 (pic, x, y, w, h);
        }
        f++;
    }
}
#endif // !OPENGLES

#ifndef GL_EXT_texture_filter_anisotropic
#define GL_TEXTURE_MAX_ANISOTROPY_EXT     0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#endif

static void SetAnisotropic(void) {
#ifndef OPENGLES
    checkGLStatus( __FILE__, __LINE__ );

    //#ifdef GL_EXT_texture_filter_anisotropic
    GLfloat aniso;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &aniso);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso);
    if (glGetError()!=GL_NO_ERROR) {
    TO_DEBUG_LOG("Warning: Anisotropic filtering not supported by driver, using trilinear filtering.\n");
    anisotropic=0;
    }
    //#else
    //anisotropic=0;
    //TO_DEBUG_LOG("Warning: Anisotropic filtering not supported at compile time, using trilinear filtering.\n");
    //#endif
#else
    anisotropic = 0;
#endif // !OPENGLES
}

#define USE_GLU_MIPMAPS

#ifdef USE_GLU_MIPMAPS

void BuildMipmaps(Uint32* pix, int w, int h, int hasalpha, int maxlevel) {
#ifndef OPENGLES
    if (hasalpha) TextureAvg32(pix,w,h);
    glPixelStorei (GL_UNPACK_ROW_LENGTH, w);
    gluBuild2DMipmaps(GL_TEXTURE_2D,hasalpha?GL_RGBA:GL_RGB,w,h,GL_RGBA,GL_UNSIGNED_BYTE,pix);
#else
    GLint format = GL_RGBA;
    gluBuild2DMipmaps(GL_TEXTURE_2D,format,w,h,format,GL_UNSIGNED_BYTE,pix);
#endif // !OPENGLES
}
#else

void ShrinkImageWeight (Uint32* src, Uint32* dest, int sw, int sh, int xs, int ys) {
    int dw=sw/xs;
    int dh=sh/ys;
    int sx,sy, dx,dy, xi, yi;
    double r,g,b,a;
    double rt,gt,bt,at;
    double wrt,wgt,wbt;
    double pixscl=(255.0/(xs*ys));
    double cpixscl;
    int ir,ig,ib,ia;
    for (sy=0,dy=0;dy<dh;sy+=ys,dy++) {
    for (sx=0,dx=0;dx<dw;sx+=xs,dx++) {
        rt=0; gt=0;
        bt=0; at=0;
        for (yi=0;yi<ys;yi++)
        for (xi=0;xi<xs;xi++) {
            Uint32 sp=src[sx+xi + (sy+yi)*sw];
            a = ALPHACMP(sp)/255.0;
            r = REDCMP(sp)/255.0;
            g = GREENCMP(sp)/255.0;
            b = BLUECMP(sp)/255.0;
            rt += r; gt += g;
            bt += b; at += a;
            wrt += r*a;
            wgt += g*a;
            wbt += b*a;
        }
        ia=(at*pixscl);
        if (at==0.0) {
        ir=rt*pixscl;
        ig=gt*pixscl;
        ib=bt*pixscl;
        } else {
        cpixscl=255.0/at;
        ir=rt*cpixscl;
        ig=gt*cpixscl;
        ib=bt*cpixscl;
        }
        *(dest++) = ir<<RED_SHIFT | ib<<BLUE_SHIFT | ig<<GREEN_SHIFT | ia<<ALPHA_SHIFT;
    }
    }
}
#ifndef min
#define min(x,y) ({ typeof(x) _x_; typeof(y) _y_; _x_=(x); _y_=(y); _x_ < _y_ ? _x_ : _y_ })
#endif

void ShrinkImage (Uint32* src, Uint32* dest, int sw, int sh, int xs, int ys) {
    int dw=sw>>xs;
    int dh=sh>>ys;
    int sx,sy, dx,dy, xi, yi;
    int rt,gt,bt,at;
    int xsk=1<<xs;
    int ysk=1<<ys;
    int scale=xs+ys;
    for (sy=0,dy=0;dy<dh;sy+=ysk,dy++) {
    for (sx=0,dx=0;dx<dw;sx+=xsk,dx++) {
        rt=0; gt=0;
        bt=0; at=0;
        for (yi=0;yi<ysk;yi++)
        for (xi=0;xi<xsk;xi++) {
            Uint32 sp=src[sx+xi + (sy+yi)*sw];
            rt += REDCMP(sp);
            gt += GREENCMP(sp);
            bt += BLUECMP(sp);
            at += ALPHACMP(sp);
        }
        rt>>=scale;
        gt>>=scale;
        bt>>=scale;
        at>>=scale;
        *(dest++) = rt<<RED_SHIFT | bt<<BLUE_SHIFT | gt<<GREEN_SHIFT | at<<ALPHA_SHIFT;
    }
    }
}
#define TWO_BUFFERS
void BuildMipmaps(Uint32* pix, int w, int h, int hasalpha, int maxmips) {
    int format=hasalpha?GL_RGBA:GL_RGB;
    Uint32* bufs[16];
    bufs[0]=pix;

#ifdef TWO_BUFFERS
    Uint32* buf1=malloc((w>>1)*(h>>1)*4);
    Uint32* buf2=malloc((w>>1)*(h>>1)*4);
    bufs[1] = bufs[3] = bufs[5] = bufs[7] = bufs[9] = bufs[11]= buf1;
    bufs[2] = bufs[4] = bufs[6] = bufs[8] = bufs[10]= bufs[12]= buf2;
#else
    int z;
    for (z=1;z<16;z++) {
    bufs[z]=malloc((w>>1)*(h>>1)*4);
    }
#endif
    int cw=w;
    int ch=h;
    int ow, oh;
    int xs, ys;
    int level=0;

    while (1) {
#ifndef OPENGLES
    if (hasalpha) TextureAvg32(bufs[level],cw,ch);
    glPixelStorei (GL_UNPACK_ROW_LENGTH, cw);
    glTexImage2D (GL_TEXTURE_2D,level, format, cw, ch, 0, GL_RGBA,
              GL_UNSIGNED_BYTE, bufs[level]);
#else
    glTexImage2D (GL_TEXTURE_2D,level, format, cw, ch, 0, format,
                  GL_UNSIGNED_BYTE, bufs[level]);
#endif // !OPENGLES

    glFinish();
    if (cw<=1 && ch<=1 ) break;
    ow=cw; oh=ch;
    xs=0; ys=0;
    if (cw>1) { cw >>= 1; xs=1; }
    if (ch>1) { ch >>= 1; ys=1; }
    level++;
    if (!(maxmips!=-1 && level>=maxmips)) ShrinkImage(bufs[level-1],bufs[level],ow,oh,xs,ys);
    }
#ifdef TWO_BUFFERS
    free(buf1); free(buf2);
#else
    for (z=1;z<16;z++) {
    free(bufs[z]);
    }
#endif
}

#endif /*USE_GLU_MIPMAPS*/


void UploadTexture(GLuint tex, void* pixels, int w, int h,  int repx, int repy, int hasalpha, int minfilt, int magfilt) {
    int mipmaps=1;

    glBindTexture(GL_TEXTURE_2D, tex);
    checkGLStatus( __FILE__, __LINE__ );
#ifndef OPENGLES
    glPixelStorei (GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei (GL_UNPACK_SKIP_ROWS, 0);
#endif // !OPENGLES
    glPixelStorei (GL_UNPACK_ALIGNMENT, 1);
    checkGLStatus( __FILE__, __LINE__ );

    switch (minfilt) {
    case GL_NEAREST:
    case GL_LINEAR:
    mipmaps=0;
    if (!magfilt) magfilt=minfilt;
    break;
    case GL_NEAREST_MIPMAP_NEAREST:
    case GL_NEAREST_MIPMAP_LINEAR:
    if (!magfilt) magfilt=GL_NEAREST;
    break;
    case GL_LINEAR_MIPMAP_NEAREST:
    case GL_LINEAR_MIPMAP_LINEAR:
    if (!magfilt) magfilt=GL_LINEAR;
    break;
    }
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, repy?GL_REPEAT:GL_CLAMP_TO_EDGE);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, repx?GL_REPEAT:GL_CLAMP_TO_EDGE);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magfilt);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minfilt);

    if (mipmaps) {
    if (anisotropic)
        SetAnisotropic();
    BuildMipmaps (pixels, w, h, hasalpha,20);
    } else {
#ifndef OPENGLES
    glTexImage2D (GL_TEXTURE_2D,0, hasalpha?GL_RGBA:GL_RGB, w, h, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, pixels);
#else
    GLint format = GL_RGBA;

    glTexImage2D (GL_TEXTURE_2D, 0, format, w, h, 0, format,
                  GL_UNSIGNED_BYTE, pixels);
#endif // !OPENGLES
    }
    checkGLStatus( __FILE__, __LINE__ );
}

int checkalpha(Uint32* tex, int w, int cw, int ch) {
    int skip=w-cw;
    int i,j;
    for (i=0;i<ch;i++,tex += skip) {
    for (j=0;j<cw;j++,tex++) {
        if (ALPHACMP(*tex)!=0xFF) return 1;
    }
    }
    return 0;
}
imgcache* LoadImageCache(const char* fname, int repeatx, int minfilt, int magfilt) {

    imgcache* cur=img_cache;
    while (cur) {
    if (!strcmp(fname,cur->name)) {
        return cur;
    }
    cur=cur->next;
    }
#ifndef ANDROID_NDK
    SDL_Surface* tsurf=IMG_Load(fname);

    if (!tsurf) {
    TO_DEBUG_LOG("Could not load image %s: %s\n",fname,SDL_GetError());
    SDL_Quit();
    exit(1);

    }

    imgcache* newImg=(imgcache*)malloc(sizeof(imgcache));
    newImg->name=strdup(fname);

    glGenTextures (1, &newImg->texnum);

#ifndef USE_SDL2
    SDL_SetAlpha(tsurf,0,255);
#else
    SDL_SetSurfaceAlphaMod(tsurf, 255);
#endif // !USE_SDL2
    SDL_Surface* conv=SDL_CreateRGBSurface(SDL_SWSURFACE,tsurf->w,tsurf->h,32,0x000000FF,0x0000FF00,0x00FF0000,0xFF000000);
    Uint32* temptex=(Uint32*)malloc(tsurf->w*tsurf->h*4);

#ifndef USE_SDL2
    SDL_SetAlpha(conv,0,255);
#else
    SDL_SetSurfaceAlphaMod(conv, 255);
#endif // !USE_SDL2
    SDL_BlitSurface(tsurf,NULL,conv,NULL);
    int xx,yy;
    Uint32* temp=(Uint32*)temptex;
    for(xx=0;xx<tsurf->w;xx++)
    for(yy=0;yy<tsurf->h;yy++)
        *temp++=((Uint32*)conv->pixels)[xx+tsurf->w*yy];

    int hasalph=checkalpha((Uint32*) tsurf->pixels, tsurf->w, tsurf->w, tsurf->h);

    newImg->w=tsurf->w;
    newImg->h=tsurf->h;
    UploadTexture(newImg->texnum,temptex,tsurf->h,tsurf->w,0,repeatx,hasalph,minfilt,magfilt);
    SDL_FreeSurface(tsurf);
    SDL_FreeSurface(conv);
    free(temptex);
#else
    unsigned int png_error;
    unsigned char *png_image;
    char *png_file;
    unsigned int png_width, png_height;

    // Load PNG from assets to RAM
    SDL_RWops *imgRAM = SDL_RWFromFile(fname, "rb");
    if (!imgRAM) {
        TO_DEBUG_LOG("ERROR LOADING PNG FILE: %s\n", fname);
        SDL_Quit();
        exit(1);
    }
    long int png_size = SDL_RWsize(imgRAM);
    png_file = (char *)malloc(png_size);
    SDL_RWread(imgRAM, png_file, png_size, 1);

    // Use LodePNG library
    png_error = lodepng_decode32(&png_image, &png_width, &png_height, png_file, png_size);
    if (png_error) {
        TO_DEBUG_LOG("LODEPNG Error %d: %s.\n", png_error, lodepng_error_text(png_error));
    }

    imgcache* newImg=(imgcache*)malloc(sizeof(imgcache));
    newImg->name=strdup(fname);

    glGenTextures (1, &newImg->texnum);

    // Temp Buffer for convert PNG
    Uint32* temptex=(Uint32*)malloc(png_width*png_height*4);

    // Convert loop
    int xx,yy;
    Uint32* temp=(Uint32*)temptex;
    for(xx=0;xx<png_width;xx++)
    for(yy=0;yy<png_height;yy++)
        *temp++=((Uint32*)png_image)[xx+png_width*yy];

    newImg->w=png_width;
    newImg->h=png_height;
    UploadTexture(newImg->texnum,temptex,png_height,png_width,0,repeatx,1,minfilt,magfilt);

    free(png_file);
    SDL_FreeRW(imgRAM);

    free(temptex);
    free(png_image);
#endif // !ANDROID_NDK
    newImg->next=img_cache;
    img_cache=newImg;
    return newImg;
}

typedef struct {
    char* name;
    int value;
} enumpair;


enumpair filtermodes[] = {
    {"GL_NEAREST",GL_NEAREST},
    {"GL_LINEAR",GL_LINEAR},
    {"GL_NEAREST_MIPMAP_NEAREST",GL_NEAREST_MIPMAP_NEAREST},
    {"GL_NEAREST_MIPMAP_LINEAR",GL_NEAREST_MIPMAP_LINEAR},
    {"GL_LINEAR_MIPMAP_NEAREST",GL_LINEAR_MIPMAP_NEAREST},
    {"GL_LINEAR_MIPMAP_LINEAR",GL_LINEAR_MIPMAP_LINEAR},
    {NULL,-1},
};

enumpair wrapmodes[] = {
    {"clamp",0},
    {"repeat",1},
    {NULL,-1},
};
static int GetEnum(char* str,enumpair* cur) {
    while (cur->name) {
    if (!strcasecmp(str,cur->name)) {
        return cur->value;
    }
    cur++;
    }
    return cur->value;
}

typedef struct {
    int wrapmode;
    int tcl, tch;
    int minfilt,magfilt;
    int bmpkind_override;
    char* texreplace;
} wallparam;

wallparam default_wallparam={
    -1,
    0, -1,
    GL_LINEAR_MIPMAP_LINEAR,GL_LINEAR,
    -1,
    NULL
};

/* Load textures. Use LZW without a license. */

void loadwalls(int replace)
{
    unsigned char bitcnt, numbits;
    wallparam wparams[numwalls];
    //imgcache* cache=NULL;

    FILE* params;

    K_INT16 i, j, fil, bytecnt1, bytecnt2;
#ifdef COLOURTEST
    K_INT16 k;
#endif
    K_INT16 currstr, strtot, compleng, dat, goalstr;
    unsigned char *walsegg;

    K_UINT16 stack[LZW_STACK_SIZE];
    K_UINT16 stackp=0;

    unsigned char *RGBATexture=malloc(64*64*4);

    for (i=0;i<numwalls;i++) {
    wparams[i]=default_wallparam;
    }
    int dotransition=1;
    wallparam* cwparam=NULL;
    memset(shadow,0,sizeof(shadow));

    char path[256];
#ifndef ANDROID_NDK
    snprintf(path, sizeof(path), "%s/wallparams.ini", globalDataDir);
#else
    snprintf(path, sizeof(path), "%s/wallparams.ini", globalAndroidRWdir);
#endif // !ANDROID_NDK
    if (replace && (params=fopen(path,"rt"))!=NULL) {
    dotransition=0;
    int curwall=0;
    char buf[256];
    char *lin,*key,*val,*tmp;
    int ln=0;
    while (fgets(buf,256,params)) {
        ln++;
        lin=buf;
        if (!*lin) continue;
        while (*lin && isspace(*lin)) *lin++;
        if (*lin == '#') continue;
        tmp=lin+strlen(lin)-1;
        while (isspace(*tmp)) *tmp--=0;
        if (*lin=='[' && *tmp == ']') {
        *tmp--=0;
        lin++;
        curwall=atoi(lin);
        cwparam=&wparams[curwall];
        if (curwall >= numwalls || curwall<0) {
            cwparam=NULL;
        }
        //printf("section='%s'\n",lin);

        }
        if ((tmp=strchr(lin,'='))) {
        if (!cwparam) {
            TO_DEBUG_LOG("wallparams.ini:%d: Key/value outside of section\n",ln);
            SDL_Quit();
            exit(1);
        }

        *tmp=0;
        val=tmp+1;
        key=lin;
        while (*val && isspace(*val)) *val++;
        while (*key && isspace(*key)) *key++;
        tmp--;
        while (isspace(*tmp)) *tmp--=0;
        int ival=atoi(val);
        if (!strcmp(key,"src")) {
            cwparam->texreplace=strdup(val);
        } else if (!strcmp(key,"range")) {
            i=sscanf(val,"%d %d",&cwparam->tcl,&cwparam->tch);
            if (i!=2) {
            TO_DEBUG_LOG("wallparams.ini:%d: Expected 2 values for range\n",ln);
            SDL_Quit();
            exit(1);
            }
        } else if (!strcmp(key,"shadow")) {
            shadow[curwall+1]=ival;
            if (ival<0 || ival>numwalls) {
            TO_DEBUG_LOG("wallparams.ini:%d: Invalid value for shadow: %d\n",ln,ival);
            SDL_Quit();
            exit(1);
            }
            //printf("Set shadow for %d to %d (%s)\n",curwall,ival,val);
        } else if (!strcmp(key,"bmpkind")) {
            cwparam->bmpkind_override=ival;
            if (ival<0 || ival>5) {
            TO_DEBUG_LOG("wallparams.ini:%d: Invalid value for bmpkind: %d\n",ln,ival);
            SDL_Quit();
            exit(1);
            }
        } else if (!strcmp(key,"minfilt")) {
            cwparam->minfilt=GetEnum(val,filtermodes);
            if (cwparam->minfilt==-1) {
            TO_DEBUG_LOG("wallparams.ini:%d: Invalid value for filter: %s\n",ln,val);
            SDL_Quit();
            exit(1);
            }
        } else if (!strcmp(key,"magfilt")) {
            cwparam->magfilt=GetEnum(val,filtermodes);
            if (cwparam->magfilt==-1) {
            TO_DEBUG_LOG("wallparams.ini:%d: Invalid value for filter: %s\n",ln,val);
            SDL_Quit();
            exit(1);
            }
        } else if (!strcmp(key,"wrap")) {
            cwparam->wrapmode=GetEnum(val,wrapmodes);
            if (cwparam->wrapmode==-1) {
            TO_DEBUG_LOG("wallparams.ini:%d: Invalid value for wrap mode: %s\n",ln,val);
            SDL_Quit();
            exit(1);
            }
        } else {
            TO_DEBUG_LOG("wallparams.ini:%d: Invalid key: %s\n",ln,key);
            SDL_Quit();
            exit(1);
        }
        //printf("key='%s' val='%s'\n",key,val);

        }
    }
    fclose(params);

    }

    char path1[256];
    char path2[256];
    snprintf(path1, sizeof(path1), "%s/walls.kzp", globalDataDir);
    snprintf(path2, sizeof(path2), "%s/WALLS.KZP", globalDataDir);
#ifdef ANDROID_NDK
    SDL_RWops *io = SDL_RWFromFile(path2, "rb");
    if (!io) {
        io = SDL_RWFromFile(path1, "rb");
    }
    if (io) {
#else
    if (((fil = open(path1,O_RDONLY|O_BINARY,0)) != -1)||
    ((fil = open(path2,O_RDONLY|O_BINARY,0)) != -1))
    {
#endif // ANDROID_NDK
    bmpkind[0] = 0;
    wallheader[0] = 8;
#ifndef ANDROID_NDK
    read(fil,&wallheader[1],rnumwalls);
#else
    SDL_RWread(io, &wallheader[1], rnumwalls, 1);
#endif // !ANDROID_NDK
    if (lab3dversion==0)
#ifndef ANDROID_NDK
        readLE16(fil,&tileng[0],numwalls*2);
#else
        SDL_RWread(io, &tileng[0], numwalls*2, 1);
#endif // !ANDROID_NDK
    tioffs[0] = (long)(numwalls+numwalls+numwalls);
    for(i=1;i<=rnumwalls;i++)
    {
        tioffs[i] = tioffs[i-1]+((long)(tileng[i-1]+2));
        bmpkind[i] = 1+(wallheader[i]&7);
        if (bmpkind[i] == 3)
        bmpkind[i] = 4;
    }

    bmpkind[sharewaremessage]=4;

    for(i=1;i<=256;i++) {
        lzwbuf[i]=i&255;
        lzwbuf2[i]=i;
    }
    lzwbuf2[0]=0;
    lzwbuf[0]=0;

    for(i=0;i<rnumwalls;i++)
    {
#ifndef ANDROID_NDK
        readLE16(fil,&strtot,2);
#else
        SDL_RWread(io, &strtot, 2, 1);
#endif // !ANDROID_NDK
        if (lab3dversion)
#ifndef ANDROID_NDK
            readLE16(fil, &compleng, 2);
#else
            SDL_RWread(io, &compleng, 2, 1);
#endif // !ANDROID_NDK
        else
        compleng = tileng[i];
#ifndef ANDROID_NDK
        read(fil,&tempbuf[0],compleng);
#else
        SDL_RWread(io, &tempbuf[0], compleng, 1);
#endif // !ANDROID_NDK
        walsegg=walseg[i];

        if (strtot > 0)
        {
        tempbuf[compleng] = 0;
        tempbuf[compleng+1] = 0;
        tempbuf[compleng+2] = 0;
        bytecnt2 = 0;
        bytecnt1 = 0;
        bitcnt = 0;
        currstr = 256;
        goalstr = 512;
        numbits = 9;
        do
        {
            dat=(((tempbuf[bytecnt2])|
              ((K_UINT16)(tempbuf[bytecnt2+1]))<<8)>>bitcnt)&
            ((1<<numbits)-1);
/*		    dat=((*((K_UINT16 *)(tempbuf+bytecnt2)))
            >>bitcnt)&
            ((1<<numbits)-1);*/
            if (bitcnt+numbits>16) {
            dat+=(((K_UINT16)tempbuf[bytecnt2+2])&
                  ((1<<((bitcnt+numbits)&15))-1))<<
                (16-bitcnt);
            }

            bitcnt+=numbits;
            bytecnt2+=bitcnt>>3;
            bitcnt&=7;

            lzwbuf2[currstr]=dat;

            while(dat>=256) {
            stack[stackp++]=lzwbuf[dat];

            dat=lzwbuf2[dat];
            }

            lzwbuf[currstr-1]=dat;
            lzwbuf[currstr]=dat;

            dat=lzwbuf2[dat];
            stack[stackp++]=dat;

            while(stackp>0) {
            stackp--;
            if (bytecnt1<4096)
                walsegg[bytecnt1++]=stack[stackp];
            }
            currstr++;
            if (currstr == goalstr)
            {
            numbits++;
            goalstr = (goalstr<<1);
            }
        }
        while (currstr <= strtot);
        }
        else
        memcpy(walsegg, tempbuf, 4096);
        if (bmpkind[i+1] >= 2)
        {
        j=0;
        while((j<4096)&&(walsegg[j]!=255))
            j++;
        j&=0xfc0;
        lborder[i+1]=j;
        j=4095;
        while((j>=0)&&(walsegg[j]!=255))
            j--;
        j&=0xfc0;
        j+=64;
        rborder[i+1]=j;
        }
        else
        {
        lborder[i+1] = 0;
        rborder[i+1] = 4096;
        }
#ifndef OPENGLES
        if ((i < 127) && ((i&1)==0)) {
#else
        if ((i < numwalls) && ((i&1)==0)) {
#endif // !OPENGLES
        if (debugmode)
            TO_DEBUG_LOG("Trying to draw screen buffer.\n");
#ifndef OPENGLES
        glDrawBuffer(GL_BACK);
#endif // !OPENGLES
#ifdef OPENGLES
        if (i < 127)
#endif // OPENGLES
        fade(64+(i>>1));
        SetVisibleScreenOffset(0);
        if (debugmode)
            TO_DEBUG_LOG("Screen buffer draw OK.\n");
        } else {
#ifndef OPENGLES
        glDrawBuffer(GL_FRONT);
#endif // !OPENGLES
        if (i==128) {
            fade(63);
        }
        }

        j=(160-(rnumwalls>>2)+i);

        if (lab3dversion) {
        if (i < (rnumwalls>>1)) {
            screenbuffer[screenbufferwidth*219+j]=255;
        }
        else {
            j-=rnumwalls>>1;
            screenbuffer[screenbufferwidth*219+j]=0;
        }
        UploadPartialOverlay(j,219,1,1);
        } else {
        if (i < (rnumwalls>>1)) {
            screenbuffer[screenbufferwidth*199+j]=255;
        }
        else {
            j-=rnumwalls>>1;
            screenbuffer[screenbufferwidth*199+j]=63;
        }
        if (debugmode)
            TO_DEBUG_LOG("Trying to update screen buffer.\n");
        UploadPartialOverlay(j,199,1,1);
        if (debugmode)
            TO_DEBUG_LOG("Screen buffer update OK.\n");
        }

        /* Use double buffer when fading, single buffer when not.
           Yes, I know I'm too clever for my own good. */

#ifndef OPENGLES
        if ((i<127) && ((i&1)==0))
#else
        if ((i<numwalls) && ((i&1)==0))
#endif // !OPENGLES
        drawOnScreen();
        else
        glFlush();
        cwparam=&wparams[i];
        int minfilt=cwparam->minfilt;
        int magfilt=cwparam->magfilt;

        if (fullfilter==GL_NEAREST) {
        magfilt=GL_NEAREST;
        minfilt=GL_NEAREST;
        }
        if (cwparam->bmpkind_override!=-1)
        bmpkind[i+1] = cwparam->bmpkind_override;

        int wrapmode=(bmpkind[i+1]<2?1:0);

        if (cwparam->wrapmode!=-1)
        wrapmode=cwparam->wrapmode;

#if 0
        printf("Wall information for wall %d:\n",i);
        printf("Replacement image: %s\n",cwparam->texreplace);
        printf("Range: %d-%d\n",cwparam->tcl,cwparam->tch);
        printf("Wrap Mode: %d\n",wrapmode);
        printf("Min Filter: %d\n",minfilt);
        printf("Mag Filter: %d\n",magfilt);
        printf("bmpkind: %d\n",bmpkind[i+1]);
        printf("Shadow: %d\n",shadow[i+1]);
        printf("\n");
#endif
        if (cwparam->texreplace) {
        imgcache* cache=LoadImageCache(cwparam->texreplace,wrapmode,minfilt,magfilt);
        texName[i]=cache->texnum;
        if (cwparam->tch==-1) cwparam->tch=cache->w;
        walltexcoord[i][0]= ((double)cwparam->tcl/(double)cache->w);
        walltexcoord[i][1]= ((double)cwparam->tch/(double)cache->w);
        //printf("TexCoords for %d: %d %d /%d %.6f %.6f\n",i,cwparam->tcl,cwparam->tch,cache->w,walltexcoord[i][0],walltexcoord[i][1]);
        } else {
        walltexcoord[i][0]=0.0;
        walltexcoord[i][1]=1.0;

        glGenTextures(1,&texName[i]);

        glBindTexture(GL_TEXTURE_2D,texName[i]);
        checkGLStatus( __FILE__, __LINE__ );

        if (wrapmode) {
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
        } else {
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
        }
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,magfilt);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,minfilt);
        checkGLStatus( __FILE__, __LINE__ );

#ifndef OPENGLES
        glPixelStorei(GL_UNPACK_ROW_LENGTH,0);
#endif // !OPENGLES
        checkGLStatus( __FILE__, __LINE__ );
        glPixelStorei(GL_UNPACK_ALIGNMENT,1);
        checkGLStatus( __FILE__, __LINE__ );

        /* Replace door1 with colour test image. */

#ifdef COLOURTEST
        if (i==door1-1) {
            for(j=0;j<64;j++)
            for(k=0;k<64;k++)
                walsegg[j*64+k]=((j>>2)<<4)|(k>>2);
        }
#endif

        TextureConvert(walsegg, RGBATexture, bmpkind[i+1]);

        if (debugmode)
            TO_DEBUG_LOG("Trying to upload texture.\n");

        if (anisotropic)
            SetAnisotropic();
        BuildMipmaps((Uint32*)RGBATexture,64,64,1,10);

        if (debugmode)
            TO_DEBUG_LOG("Upload texture complete.\n");

        if (i==gameover-1) {
            /* Keep two copies of this; one for walls, the other for spinning
               overlay text. */
            glGenTextures(1,&gameoversprite);

            glBindTexture(GL_TEXTURE_2D,gameoversprite);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,partialfilter);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,
                    fullfilter);
            TextureConvert(walsegg, RGBATexture, 4);

            if (anisotropic)
            SetAnisotropic();
            BuildMipmaps((Uint32*)RGBATexture,64,64,1,10);
        }

        checkGLStatus( __FILE__, __LINE__ );

        /*	    printf("Wall number %d:\n",i);

for(k=0;k<64;k++) {
for(j=0;j<64;j++) {
printf("%2x",walsegg[k+j*64]);
}
printf("\n");
}
printf("\n");*/
        }
    }
#ifndef ANDROID_NDK
    close(fil);
#else
    SDL_FreeRW(io);
#endif // !ANDROID_NDK
    } else {
    TO_DEBUG_LOG("Can't find walls.kzp.\n");
    SDL_Quit();
    exit(1);
    }
    free(RGBATexture);


//      Examine textures...
/*
    for(i=0;i<rnumwalls;i++) {
    glClear(GL_COLOR_BUFFER_BIT);
    printf("Texture number %d.\n",i);
    spridraw(180,50,512,i+1);
    SDL_GL_SwapBuffers();
    pressakey();
    }
*/

    if (lab3dversion) {
    visiblescreenyoffset=0;
    strcpy(textbuf,
           "\"LAB3D/SDL\" conversion");
    textprint(30,222,0);

    strcpy(textbuf,
           ".");
    textprint(260,226,0);
    textprint(264,226,0);
    strcpy(textbuf,
           "Copyright (c) 2002-2004 Jan Lonnberg");
    textprint(30,232,0);
    }

    if (dotransition && lab3dversion==0) {
    /* Set up transition textures between walls properly. */

    TransitionTexture(0,1,2);
    TransitionTexture(2,3,0);
    TransitionTexture(6,7,8);
    TransitionTexture(8,9,6);

    /* Set up end of game rainbow. */

    TransitionTexture(424,425,426);
    TransitionTexture(47,424,425);
    TransitionTexture(425,426,47);
    } else {
    for (i=0;i<numsplits;i++) {
        splitTexNum[i]=-1;
    }

    }
#ifndef OPENGLES
    glDrawBuffer(GL_BACK);
#endif // !OPENGLES
}

#define COPYLINE \
    for(y=0;y<64;y++) { \
    *(t++)=spritepalette[(*f)*3]<<2; \
    *(t++)=spritepalette[(*f)*3+1]<<2; \
    *(t++)=spritepalette[(*f)*3+2]<<2; \
    *(t++)=255; \
    f++; \
    }

/* Create a smoother transition texture. */

void TransitionTexture(int left,int texture,int right) {
    unsigned char texdata[66*64*4];

    unsigned char *f=walseg[left]+(63*64),*t=texdata;

    static int texnum=0;

    int x,y;

    COPYLINE;

    f=walseg[texture];

    for(x=0;x<64;x++)
    COPYLINE;

    f=walseg[right];

    COPYLINE;

    for(x=0;x<2;x++) {

    glGenTextures(1,&splitTexName[texnum][x]);

    glBindTexture(GL_TEXTURE_2D,splitTexName[texnum][x]);
    checkGLStatus( __FILE__, __LINE__ );

    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,partialfilter);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,
            fullfilter);
    checkGLStatus( __FILE__, __LINE__ );

#ifndef OPENGLES
    glPixelStorei(GL_UNPACK_ROW_LENGTH,0);
#endif // !OPENGLES
    checkGLStatus( __FILE__, __LINE__ );
    glPixelStorei(GL_UNPACK_ALIGNMENT,1);
    checkGLStatus( __FILE__, __LINE__ );

    /* Add code here to upload two textures from texdata, one with cols
       0-63, other 2-65. */

#ifndef OPENGLES
    glPixelStorei(GL_UNPACK_SKIP_PIXELS,0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS,0);
    glPixelStorei(GL_UNPACK_ROW_LENGTH,0);
#endif // !OPENGLES

    BuildMipmaps((Uint32*)(texdata+x*512),64,64,1,10);

    checkGLStatus( __FILE__, __LINE__ );
    }
    splitTexNum[texnum++]=texture;

}

/* Get average of neighbouring pixels... */

int AverageColour(unsigned char *p, int x, int y,int colour) {
    int a,b,c=0,n=0;

    for(a=-1;a<=1;a++) {
    if (x+a<0) continue;
    if (x+a>63) continue;
    for(b=-1;b<=1;b++) {
        if (y+b<0) continue;
        if (y+b>63) continue;
        if (p[(x+a)*64+(y+b)]!=255) {
        c+=spritepalette[p[(x+a)*64+(y+b)]*3+colour]<<2;
        n++;
        }
    }
    }
    if (n>0) return c/n; else return 0;
}

/* Convert a texture from 8-bit indexed to 32-bit RGBA. */
void TextureConvert(unsigned char *from, unsigned char *to, Sint16 type) {

    unsigned char *f=from,*t=to;

    int x,y;

    for(x=0;x<64;x++)
    for(y=0;y<64;y++) {
        if ((type>1)&&(*(f))==255) {
        *(t++)=0;
        *(t++)=0;
        *(t++)=0;
        *(t++)=0;
        } else {
        *(t++)=spritepalette[(*f)*3]<<2;
        *(t++)=spritepalette[(*f)*3+1]<<2;
        *(t++)=spritepalette[(*f)*3+2]<<2;
        *(t++)=255;
        }
        f++;
    }
    //TextureAvg32((Uint32*)to,64,64);
}

/*void TextureConvert(unsigned char *from, unsigned char *to, K_INT16 type) {

    unsigned char *f=from,*t=to;

    int x,y;

    for(x=0;x<64;x++)
    for(y=0;y<64;y++) {
        if ((type>1)&&(*(f))==255) {
        *(t++)=AverageColour(from,x,y,0);
        *(t++)=AverageColour(from,x,y,1);
        *(t++)=AverageColour(from,x,y,2);
        *(t++)=0;
        } else {
        *(t++)=spritepalette[(*f)*3]<<2;
        *(t++)=spritepalette[(*f)*3+1]<<2;
        *(t++)=spritepalette[(*f)*3+2]<<2;
        *(t++)=255;
        }
        f++;
    }
    }*/

/* Load a saved game... */

K_INT16 loadgame(K_INT16 gamenum)
{
    char path[256];
    char filename[20];
    K_INT16 i;
    int fil;

    filename[0] = 'S', filename[1] = 'A', filename[2] = 'V';
    filename[3] = 'G', filename[4] = 'A', filename[5] = 'M';
    filename[6] = 'E', filename[7] = gamenum+48;
    filename[8] = '.', filename[9] = 'D', filename[10] = 'A';
    filename[11] = 'T', filename[12] = 0;

#ifndef ANDROID_NDK
    snprintf(path, sizeof(path), "%s/%s", globalDataDir, filename);
#else
    snprintf(path, sizeof(path), "%s/%s", globalAndroidRWdir, filename);
#endif
    if((fil=open(path,O_RDONLY|O_BINARY,0))==-1) {
    filename[0] = 's', filename[1] = 'a', filename[2] = 'v';
    filename[3] = 'g', filename[4] = 'a', filename[5] = 'm';
    filename[6] = 'e', filename[7] = gamenum+48;
    filename[8] = '.', filename[9] = 'd', filename[10] = 'a';
    filename[11] = 't', filename[12] = 0;

#ifndef ANDROID_NDK
    snprintf(path, sizeof(path), "%s/%s", globalDataDir, filename);
#else
    snprintf(path, sizeof(path), "%s/%s", globalAndroidRWdir, filename);
#endif
    if((fil=open(path,O_RDONLY|O_BINARY,0))==-1)
        return -1;
    }
    musicoff();
    read(fil,&hiscorenam[0],16);
    read(fil,&hiscorenamstat,1);
    readLE16(fil,&boardnum,2);

    if (boardnum >= numboards)
    quitgame = 2;
    filename[0] = 'L', filename[1] = 'A', filename[2] = 'B';
    filename[3] = 'S', filename[4] = 'N', filename[5] = 'G';
    filename[6] = (boardnum/10)+48, filename[7] = (boardnum%10)+48;
    filename[8] = 0;
    mute = 1;
    loadmusic(filename);
    musicon();

    readLE32(fil,&scorecount,4);
    readLE32(fil,&scoreclock,4);

    readLE16(fil,&board[0][0],8192);
    readLE16(fil,&skilevel,2);
    readLE16(fil,&life,2);
    readLE16(fil,&death,2);
    readLE16(fil,&lifevests,2);
    readLE16(fil,&lightnings,2);
    readLE16(fil,&firepowers[0],6);
    readLE16(fil,&bulchoose,2);
    readLE16(fil,&keys[0],4);
    readLE16(fil,&coins,2);
    readLE16(fil,&compass,2);
    readLE16(fil,&cheated,2);
    readLE16(fil,&animate2,2);
    readLE16(fil,&animate3,2);
    readLE16(fil,&animate4,2);
    readLE16(fil,&oscillate3,2);
    readLE16(fil,&oscillate5,2);
    readLE16(fil,&animate6,2);
    readLE16(fil,&animate7,2);
    readLE16(fil,&animate8,2);
    readLE16(fil,&animate10,2);
    readLE16(fil,&animate11,2);
    readLE16(fil,&animate15,2);
    readLE16(fil,&statusbar,2);
    readLE16(fil,&statusbargoal,2);
    readLE16(fil,&posx,2);
    readLE16(fil,&posy,2);
    readLE16(fil,&posz,2);
    readLE16(fil,&ang,2);
    readLE16(fil,&startx,2);
    readLE16(fil,&starty,2);
    readLE16(fil,&startang,2);
    readLE16(fil,&angvel,2);
    readLE16(fil,&vel,2);
    readLE16(fil,&mxvel,2);
    readLE16(fil,&myvel,2);
    readLE16(fil,&svel,2);
    readLE16(fil,&hvel,2);
    readLE16(fil,&oldposx,2);
    readLE16(fil,&oldposy,2);
    readLE16(fil,&bulnum,2);
    readLE16(fil,&bulang[0],bulnum<<1);
    readLE16(fil,&bulkind[0],bulnum<<1);
    readLE16(fil,&bulx[0],bulnum<<1);
    readLE16(fil,&buly[0],bulnum<<1);
    readLE32(fil,&bulstat[0],bulnum<<2);
    readLE32(fil,&lastbulshoot,4);
    readLE16(fil,&mnum,2);
    readLE16(fil,&mposx[0],mnum<<1);
    readLE16(fil,&mposy[0],mnum<<1);
    readLE16(fil,&mgolx[0],mnum<<1);
    readLE16(fil,&mgoly[0],mnum<<1);
    readLE16(fil,&moldx[0],mnum<<1);
    readLE16(fil,&moldy[0],mnum<<1);
    readLE16(fil,&mstat[0],mnum<<1);
    readLE16(fil,&mshock[0],mnum<<1);
    read(fil,&mshot[0],mnum);
    readLE16(fil,&doorx,2);
    readLE16(fil,&doory,2);
    readLE16(fil,&doorstat,2);
    read(fil,&numwarps,1);
    read(fil,&justwarped,1);
    read(fil,&xwarp[0],numwarps);
    read(fil,&ywarp[0],numwarps);
    readLE32(fil,&totalclock,4);
    ototclock = totalclock;
    readLE32(fil,&purpletime,4);
    readLE32(fil,&greentime,4);
    readLE32(fil,&capetime[0],8);
    readLE32(fil,&musicstatus,4);
    readLE16(fil,(K_INT16 *)(&clockspeed),2); /* volatile warning here. */
    readLE32(fil,&count,4);
    readLE32(fil,&countstop,4);
    readLE16(fil,&nownote,2);
    readLE16(fil,&i,2);
    readLE32(fil,&chanage,18<<2);
    read(fil,&chanfreq,18);
    readLE16(fil,&midiinst,2);
    readLE16(fil,&mute,2);
    read(fil,&namrememberstat,1);
    readLE16(fil,&fadewarpval,2);
    readLE16(fil,&fadehurtval,2);
    readLE16(fil,&slottime,2);
    readLE16(fil,&slotpos[0],6);
    readLE16(fil,&owecoins,2);
    readLE16(fil,&owecoinwait,2);
    close(fil);
    if (((i == -1) || (i > 2)) && (musicsource >= 0))
    {
    musicoff();
    loadmusic(filename);
    musicon();
    }
    totalclock -= 2;
    copyslots(slotto);
    totalclock++;
    copyslots(slotto+1);
    totalclock++;

    yourhereoldpos = ((posx>>10)<<6)+(posy>>10);

    for(i=0;i<4096;i++)
    walseg[map-1][i]=board[0][i]&255;
    walseg[map-1][yourhereoldpos]=255;

    if ((vidmode == 0) && (statusbargoal > 400))
    {
    statusbar -= 80;
    statusbargoal -= 80;
    }
    if ((vidmode == 1) && (statusbargoal < 400))
    {
    statusbar += 80;
    statusbargoal += 80;
    }
    if (vidmode == 0)
    {
    scrsize = 18000-2880;
    if (statusbar == 399)
        scrsize += 2880;
    }
    else
    {
    scrsize = 21600-2880;
    if (statusbar == 479)
    {
        scrsize += 2880;
    }
    }
    linecompare(statusbar);
    statusbaralldraw();
    if (compass>0) showcompass(ang);
    fade(63);
    return 0;
}

/* Save game... */

K_INT16 savegame(K_INT16 gamenum)
{
    char path[256];
    char filename[20];
    int i, fil;

    /* If we have a lower case copy of this save game, destroy it. */

    filename[0] = 's', filename[1] = 'a', filename[2] = 'v';
    filename[3] = 'g', filename[4] = 'a', filename[5] = 'm';
    filename[6] = 'e', filename[7] = gamenum+48;
    filename[8] = '.', filename[9] = 'd', filename[10] = 'a';
    filename[11] = 't', filename[12] = 0;

    unlink(filename);

    filename[0] = 'S', filename[1] = 'A', filename[2] = 'V';
    filename[3] = 'G', filename[4] = 'A', filename[5] = 'M';
    filename[6] = 'E', filename[7] = gamenum+48;
    filename[8] = '.', filename[9] = 'D', filename[10] = 'A';
    filename[11] = 'T', filename[12] = 0;
#ifndef ANDROID_NDK
    snprintf(path, sizeof(path), "%s/%s", globalDataDir, filename);
#else
    snprintf(path, sizeof(path), "%s/%s", globalAndroidRWdir, filename);
#endif
    if((fil=open(path,O_CREAT|O_WRONLY|O_BINARY,
         S_IWRITE|S_IREAD|S_IRGRP|S_IROTH))==-1) {
    return(-1);
    }
    write(fil,&hiscorenam[0],16);
    write(fil,&hiscorenamstat,1);
    writeLE16(fil,&boardnum,2);
    writeLE32(fil,&scorecount,4);
    writeLE32(fil,&scoreclock,4);

    for(i=0;i<16;i++)
    gamehead[gamenum][i] = hiscorenam[i];
    gamehead[gamenum][16] = hiscorenamstat;

    writeshort((unsigned char *)&(gamehead[gamenum][17]),boardnum);
    writelong((unsigned char *)&(gamehead[gamenum][19]),scorecount);
    writelong((unsigned char *)&(gamehead[gamenum][23]),scoreclock);

    gamexist[gamenum] = 1;
    writeLE16(fil,&board[0][0],8192);
    writeLE16(fil,&skilevel,2);
    writeLE16(fil,&life,2);
    writeLE16(fil,&death,2);
    writeLE16(fil,&lifevests,2);
    writeLE16(fil,&lightnings,2);
    writeLE16(fil,&firepowers[0],6);
    writeLE16(fil,&bulchoose,2);
    writeLE16(fil,&keys[0],4);
    writeLE16(fil,&coins,2);
    writeLE16(fil,&compass,2);
    writeLE16(fil,&cheated,2);
    writeLE16(fil,&animate2,2);
    writeLE16(fil,&animate3,2);
    writeLE16(fil,&animate4,2);
    writeLE16(fil,&oscillate3,2);
    writeLE16(fil,&oscillate5,2);
    writeLE16(fil,&animate6,2);
    writeLE16(fil,&animate7,2);
    writeLE16(fil,&animate8,2);
    writeLE16(fil,&animate10,2);
    writeLE16(fil,&animate11,2);
    writeLE16(fil,&animate15,2);
    writeLE16(fil,&statusbar,2);
    writeLE16(fil,&statusbargoal,2);
    writeLE16(fil,&posx,2);
    writeLE16(fil,&posy,2);
    writeLE16(fil,&posz,2);
    writeLE16(fil,&ang,2);
    writeLE16(fil,&startx,2);
    writeLE16(fil,&starty,2);
    writeLE16(fil,&startang,2);
    writeLE16(fil,&angvel,2);
    writeLE16(fil,&vel,2);
    writeLE16(fil,&mxvel,2);
    writeLE16(fil,&myvel,2);
    writeLE16(fil,&svel,2);
    writeLE16(fil,&hvel,2);
    writeLE16(fil,&oldposx,2);
    writeLE16(fil,&oldposy,2);
    writeLE16(fil,&bulnum,2);
    writeLE16(fil,&bulang[0],bulnum<<1);
    writeLE16(fil,&bulkind[0],bulnum<<1);
    writeLE16(fil,&bulx[0],bulnum<<1);
    writeLE16(fil,&buly[0],bulnum<<1);
    writeLE32(fil,&bulstat[0],bulnum<<2);
    writeLE32(fil,&lastbulshoot,4);
    writeLE16(fil,&mnum,2);
    writeLE16(fil,&mposx[0],mnum<<1);
    writeLE16(fil,&mposy[0],mnum<<1);
    writeLE16(fil,&mgolx[0],mnum<<1);
    writeLE16(fil,&mgoly[0],mnum<<1);
    writeLE16(fil,&moldx[0],mnum<<1);
    writeLE16(fil,&moldy[0],mnum<<1);
    writeLE16(fil,&mstat[0],mnum<<1);
    writeLE16(fil,&mshock[0],mnum<<1);
    write(fil,&mshot[0],mnum);
    writeLE16(fil,&doorx,2);
    writeLE16(fil,&doory,2);
    writeLE16(fil,&doorstat,2);
    write(fil,&numwarps,1);
    write(fil,&justwarped,1);
    write(fil,&xwarp[0],numwarps);
    write(fil,&ywarp[0],numwarps);
    writeLE32(fil,&totalclock,4);
    writeLE32(fil,&purpletime,4);
    writeLE32(fil,&greentime,4);
    writeLE32(fil,&capetime[0],8);
    writeLE32(fil,&musicstatus,4);
    writeLE16(fil,(K_INT16 *)(&clockspeed),2);
    writeLE32(fil,&count,4);
    writeLE32(fil,&countstop,4);
    writeLE16(fil,&nownote,2);
    writeLE16(fil,&musicsource,2);
    writeLE32(fil,&chanage,18<<2);
    write(fil,&chanfreq,18);
    writeLE16(fil,&midiinst,2);
    writeLE16(fil,&mute,2);
    write(fil,&namrememberstat,1);
    writeLE16(fil,&fadewarpval,2);
    writeLE16(fil,&fadehurtval,2);
    writeLE16(fil,&slottime,2);
    writeLE16(fil,&slotpos[0],6);
    writeLE16(fil,&owecoins,2);
    writeLE16(fil,&owecoinwait,2);
    close(fil);
    ksay(16);
    return 0;
}

/* Common variables between introduction() and drawintroduction().
   Messy, but works. */

static K_INT32 dalasti, pickskiltime;
static K_INT16 animater2,sharemessplc, newgamepisode;
static K_UINT16 introplc;

/* Draw introduction frame... */

void drawintroduction() {
    K_INT32 dai=totalclock>>2;
    K_INT16 k,m;

    if ((dai >= 0) && (dai < 300))
    {
    introplc = 5+times90[dai>>2];
    if (introplc > 3785)
        introplc = 3785;

    SetVisibleScreenOffset(introplc);

    if (dai < 240)
    {
        pictur(180+(240-((int)dai)),164,64*4,((((unsigned)dai)<<8)/15)&2047,copyright);
        pictur(180-(240-((int)dai)),164,64*4,(((((unsigned)dai)<<8)/15)&2047)^2047,copyright);
    }
    else
    {
        if (vidmode == 0)
        m = 20;
        else
        m = 0;
        statusbardraw(0,44,63,20,128+m,112,sodapics);
        spridraw(180-32,164-32,64*4,copyright);
    }
    }
    if ((dai >= 300) && (dai < 364))
    {
    SetVisibleScreenOffset(introplc);

    if (vidmode == 0)
        m = 20;
    else
        m = 0;
    statusbardraw(0,44,63,20,128+m,112,sodapics);
    m = (364-dai);
    spridraw(180-(m>>1),164-(m>>1),m<<2,copyright);
    }
    if ((dai >= 364) && (dai < 424))
    {
    SetVisibleScreenOffset(introplc);

    wipeoverlay(0,174,361,20);
    if (vidmode == 0)
        m = 20;
    else
        m = 0;
    statusbardraw(0,36,63,8,128+m,112,sodapics);
    strcpy(&textbuf[0],"Select Episode");
    textprint(124,112,(char)96);

    if (vidmode == 0)
        m = 20;
    else
        m = 0;
/*	statusbardraw(0,44,63,20,128+m,112,sodapics);*/
    m = (dai-360);
    spridraw(180-96-(m>>1),152-(m>>1),m<<2,episodesign1off);
    spridraw(180-(m>>1),152-(m>>1),m<<2,episodesign2off);
    spridraw(180+96-(m>>1),152-(m>>1),m<<2,episodesign3off);
    }
    if (dai >= 424)
    {
    if (sharemessplc >= 0)
    {
        if (vidmode == 0)
        m = 20;
        else
        m = 0;
        k = 0;
        if (numboards == 10)
        k = 32;
        /*
          statusbardraw(sharemessplc,k,64-sharemessplc,32,m,32,sharewaremessage);
          statusbardraw(0,k,64-sharemessplc,32,256+m+sharemessplc,32,sharewaremessage);*/
        sharemessplc--;
    }
    if (pickskiltime < 0)
    {
        SetVisibleScreenOffset(introplc);

/*	    if (dalasti < 424)
        {*/
        wipeoverlay(0,174,361,20);
        if (vidmode == 0)
        m = 20;
        else
        m = 0;
        statusbardraw(0,36,63,8,128+m,112,sodapics);
        strcpy(&textbuf[0],"Select Episode");
        textprint(124,112,(char)96);
/*	    }*/
        animater2 = (dai&16);
        if ((newgamepisode != 1) || (animater2 == 0))
        spridraw(180-128,120,64*4,episodesign1off);
        else
        spridraw(180-128,120,64*4,episodesign1on);
        if ((newgamepisode != 2) || (animater2 == 0))
        spridraw(180-32,120,64*4,episodesign2off);
        else
        spridraw(180-32,120,64*4,episodesign2on);
        if ((newgamepisode != 3) || (animater2 == 0))
        spridraw(180+64,120,64*4,episodesign3off);
        else
        spridraw(180+64,120,64*4,episodesign3on);
    }
    else if (dai >= pickskiltime+128)
    {
        SetVisibleScreenOffset(introplc);

/*	    if (dalasti < pickskiltime+128)
        {*/
        wipeoverlay(0,174,361,8);
        strcpy(&textbuf[0],"Select Difficulty");
        textprint(112,112,(char)96);
/*	    }*/
        animater2 = (dai&16);
        if ((skilevel != 0) || (animater2 == 0))
        spridraw(180-48-32,120,64*4,skilevel1);
        else {
        spridraw(180-48-32,120,64*4,skilevel1);
        spridraw(180-48-32,120,64*4,skilblank);
        }
        if ((skilevel != 1) || (animater2 == 0))
        spridraw(180+48-32,120,64*4,skilevel2);
        else {
        spridraw(180+48-32,120,64*4,skilevel2);
        spridraw(180+48-32,120,64*4,skilblank);
        }
    }
    }
    if ((pickskiltime >= 0) && (dai < pickskiltime+64))
    {
    SetVisibleScreenOffset(introplc);

    wipeoverlay(0,174,361,20);
    if (vidmode == 0)
        m = 20;
    else
        m = 0;
    statusbardraw(0,36,63,8,128+m,112,sodapics);
    strcpy(&textbuf[0],"Select Episode");
    textprint(124,112,(char)96);

    m = ((pickskiltime+63)-dai);
    spridraw(180-96-(m>>1),152-(m>>1),m<<2,episodesign1off);
    spridraw(180-(m>>1),152-(m>>1),m<<2,episodesign2off);
    spridraw(180+96-(m>>1),152-(m>>1),m<<2,episodesign3off);
    }
    if ((pickskiltime >= 0) && (dai >= pickskiltime+64) && (dai < pickskiltime+128))
    {
    SetVisibleScreenOffset(introplc);

    wipeoverlay(0,174,361,8);
    strcpy(&textbuf[0],"Select Difficulty");
    textprint(112,112,(char)96);

    m = (dai-(pickskiltime+64));
    spridraw(180-48-(m>>1),152-(m>>1),m<<2,skilevel1);
    spridraw(180+48-(m>>1),152-(m>>1),m<<2,skilevel2);
    }
}

/* Introduction... */

void introduction(K_INT16 songnum)
{
    K_INT16 j;
    K_INT16 leaveintro;
    K_INT32 dai=0;

    ingame=0;
    dside = 200;
    halfheight = 100;
    scrsize = 18000;
    dalasti = 0;

    spriteyoffset=20;

    glClearColor(0,0,0,0);
    glClear(GL_COLOR_BUFFER_BIT);

    if (songnum == 0)
    loadmusic("INTRO");
    else
    loadmusic("INTRO2");
    clockspeed = 0;
    ototclock = -1;
    totalclock = 1;
    introplc = 5;

    musicon();
    if (saidwelcome == 0)
    {
    ksay(18);
    saidwelcome = 1;
    }
/*    for(i=0;i<360;i++)
      height[i] = 0;*/
    boardnum = 0;
    keystatus[57] = 0;
    keystatus[28] = 0;
    keystatus[1] = 0;
#ifndef USE_SDL2
    newkeystatus[SDLK_ESCAPE]=0;
    newkeystatus[SDLK_RETURN]=0;
    newkeystatus[SDLK_SPACE]=0;
#else
    newkeystatus[getOldAsciiKeyCode(SDLK_ESCAPE)]=0;
    newkeystatus[getOldAsciiKeyCode(SDLK_RETURN)]=0;
    newkeystatus[getOldAsciiKeyCode(SDLK_SPACE)]=0;
#endif // !USE_SDL2
    sharemessplc = 64;
    newgamepisode = 1;
    animater2 = 0;
    totalclock = 0;
    leaveintro = 0;
    pickskiltime = -32768;

    while (leaveintro == 0)
    {
    PollInputs();
    bstatus = 0;
    if (moustat == 0)
    {
        bstatus=readmouse(NULL, NULL);
    }
    if (joystat == 0) {
        bstatus|=readjoystick(NULL,NULL);
    }
    j = (((int)labs((totalclock%120)-60))>>3);

    SDL_LockMutex(timermutex);
    while(clockspeed<4) {
        SDL_UnlockMutex(timermutex);
        SDL_Delay(10);
        SDL_LockMutex(timermutex);
    }
    totalclock += clockspeed;
    clockspeed = 0;
    SDL_UnlockMutex(timermutex);

    dalasti = dai;
    dai = (totalclock>>2);
    if ((keystatus[0xc9]|keystatus[0xc8]|keystatus[0xcb]) != 0)
    {
        if (pickskiltime < 0)
        {
        newgamepisode--;
        if (newgamepisode == 0)
            newgamepisode = 3;
        }
        else
        skilevel = 1 - skilevel;
        ksay(27);
        keystatus[0xc9] = 0;
        keystatus[0xc8] = 0;
        keystatus[0xcb] = 0;
#ifndef USE_SDL2
        newkeystatus[SDLK_UP]=0;
        newkeystatus[SDLK_PAGEUP]=0;
        newkeystatus[SDLK_LEFT]=0;
#else
        newkeystatus[getOldAsciiKeyCode(SDLK_UP)]=0;
        newkeystatus[getOldAsciiKeyCode(SDLK_PAGEUP)]=0;
        newkeystatus[getOldAsciiKeyCode(SDLK_LEFT)]=0;
#endif // !USE_SDL2
    }
    if ((keystatus[0xd1]|keystatus[0xd0]|keystatus[0xcd]) != 0)
    {
        if (pickskiltime < 0)
        {
        newgamepisode++;
        if (newgamepisode == 4)
            newgamepisode = 1;
        }
        else
        skilevel = 1 - skilevel;
        ksay(27);
        keystatus[0xd1] = 0;
        keystatus[0xd0] = 0;
        keystatus[0xcd] = 0;
#ifndef USE_SDL2
        newkeystatus[SDLK_DOWN]=0;
        newkeystatus[SDLK_PAGEDOWN]=0;
        newkeystatus[SDLK_RIGHT]=0;
#else
        newkeystatus[getOldAsciiKeyCode(SDLK_DOWN)]=0;
        newkeystatus[getOldAsciiKeyCode(SDLK_PAGEDOWN)]=0;
        newkeystatus[getOldAsciiKeyCode(SDLK_RIGHT)]=0;
#endif // !USE_SDL2
    }
    if (getkeydefstat(17) > 0)
    {
        lastunlock = 1;
        lastshoot = 1;
        lastbarchange = 1;
        sortcnt = -1;
        saidwelcome = introplc;
        spriteyoffset=0;
        j = mainmenu();
        spriteyoffset=20;
        saidwelcome = 1;
        sortcnt = 0;
        if ((j == 0) && (newgameplace >= 0))
        {
        if (newgameplace == 0) boardnum = 0, newgamepisode = 1;
        if (newgameplace == 1) boardnum = 10, newgamepisode = 2;
        if (newgameplace == 2) boardnum = 20, newgamepisode = 3;
        leaveintro = 1;
        }
        if ((j == 1) && (loadsavegameplace >= 0))
        {
        musicoff();
        setgamevideomode();
        if (vidmode == 0)
        {
            dside = 200;
            halfheight = 100;
            scrsize = 18000-2880;
        }
        else
        {
            dside = 240;
            halfheight = 120;
            scrsize = 21600-2880;
        }
        spriteyoffset=0;
        visiblescreenyoffset=0;
        ingame=1;
        settransferpalette();
        loadgame(loadsavegameplace);
        wipeoverlay(0,0,361,statusbaryoffset);
        fade(63);
        return;
        }
        if (j == 8)
        {
        newgamepisode = 1;
        leaveintro = 1;
        quitgame = 1;
        }
        clearkeydefstat(17);
        keystatus[1] = 0;
        keystatus[57] = 0;
        keystatus[28] = 0;
#ifndef USE_SDL2
        newkeystatus[SDLK_ESCAPE]=0;
        newkeystatus[SDLK_RETURN]=0;
        newkeystatus[SDLK_SPACE]=0;
#else
        newkeystatus[getOldAsciiKeyCode(SDLK_ESCAPE)]=0;
        newkeystatus[getOldAsciiKeyCode(SDLK_RETURN)]=0;
        newkeystatus[getOldAsciiKeyCode(SDLK_SPACE)]=0;
#endif // !USE_SDL2
        SDL_LockMutex(timermutex);
        clockspeed = 0;
        SDL_UnlockMutex(timermutex);
    }
    if ((keystatus[1] != 0) || (keystatus[57] != 0) || (keystatus[28] != 0) || (bstatus != 0))
    {
        if (dai < 420)
        leaveintro = 1;
        else if (((newgamepisode == 2) && (numboards < 20)) || ((newgamepisode == 3) && (numboards < 30)))
        {
        ksay(12);
        SDL_LockMutex(timermutex);
        clockspeed = 0;
        SDL_UnlockMutex(timermutex);
        }
        else
        {
        if (pickskiltime == -32768)
            pickskiltime = dai;
        else
            leaveintro = 1;
        }
        keystatus[1] = 0;
        keystatus[57] = 0;
        keystatus[28] = 0;
#ifndef USE_SDL2
        newkeystatus[SDLK_ESCAPE]=0;
        newkeystatus[SDLK_RETURN]=0;
        newkeystatus[SDLK_SPACE]=0;
#else
        newkeystatus[getOldAsciiKeyCode(SDLK_ESCAPE)]=0;
        newkeystatus[getOldAsciiKeyCode(SDLK_RETURN)]=0;
        newkeystatus[getOldAsciiKeyCode(SDLK_SPACE)]=0;
#endif // !USE_SDL2
    }
    if ((dai >= 0) && (dai < 128))
        fade(64+((int)(dai>>1)));
    if ((dai >= 128) && (dalasti < 128))
        fade(63);
    drawintroduction();
    drawOnScreen();
    }

    spriteyoffset=0;

    if (newgamepisode == 1) boardnum = 0;
    if (newgamepisode == 2) boardnum = 10;
    if (newgamepisode == 3) boardnum = 20;
    if ((newgamepisode == 2) && (numboards < 20))
    newgamepisode = 1, boardnum = 0;
    if ((newgamepisode == 3) && (numboards < 30))
    newgamepisode = 1, boardnum = 0;
    musicoff();
    setgamevideomode();
    fade(0);
    keystatus[57] = 0;
    keystatus[28] = 0;
    keystatus[1] = 0;
#ifndef USE_SDL2
    newkeystatus[SDLK_ESCAPE]=0;
    newkeystatus[SDLK_RETURN]=0;
    newkeystatus[SDLK_SPACE]=0;
#else
    newkeystatus[getOldAsciiKeyCode(SDLK_ESCAPE)]=0;
    newkeystatus[getOldAsciiKeyCode(SDLK_RETURN)]=0;
    newkeystatus[getOldAsciiKeyCode(SDLK_SPACE)]=0;
#endif // !USE_SDL2
    if (vidmode == 0)
    {
    dside = 200;
    halfheight = 100;
    scrsize = 18000-2880;
    }
    else
    {
    dside = 240;
    halfheight = 120;
    scrsize = 21600-2880;
    }

    glClear(GL_COLOR_BUFFER_BIT);

    loadboard();
    owecoins = 0;
    sortcnt = 0;
    oldlife = 0;
    life = 4095;
    death = 4095;
    lifevests = 1;
    switch (boardnum)
    {
    case 0: firepowers[0] = 0; firepowers[1] = 0;
        firepowers[2] = 0; lightnings = 0;
        break;
    case 10: firepowers[0] = 3; firepowers[1] = 2;
        firepowers[2] = 0; lightnings = 1;
        break;
    case 20: firepowers[0] = 4; firepowers[1] = 3;
        firepowers[2] = 2; lightnings = 2;
        break;
    }
    coins = 0;
    bulchoose = 0;
    animate2 = 0;
    animate3 = 0;
    animate4 = 0;
    oscillate3 = 0;
    oscillate5 = 0;
    animate6 = 0;
    animate7 = 0;
    animate8 = 0;
    animate10 = 0;
    animate11 = 0;
    animate15 = 0;
    ototclock = -1;
    totalclock = 1;
    purpletime = 0;
    greentime = 0;
    capetime[0] = 0;
    capetime[1] = 0;
    compass = 0;
    if (boardnum >= 10) compass = 1;
    cheated = 0;
    doorstat = 0;
    statusbar = 335;
    if (vidmode == 1)
    statusbar += 80;
    statusbargoal = statusbar;
    linecompare(statusbar);
    ingame=1;
    settransferpalette();
    namrememberstat = hiscorenamstat;
    hiscorenamstat = 0;
    hiscorenam[0] = 0;
    SDL_LockMutex(timermutex);
    clockspeed = 0;
    SDL_UnlockMutex(timermutex);
    scoreclock = 0;
    scorecount = 0;
    fadewarpval = 63;
    fadehurtval = 0;
    visiblescreenyoffset=0;
    wipeoverlay(0,0,361,statusbaryoffset);
#ifndef OPENGLES
    glDrawBuffer(GL_BACK);
#endif // !OPENGLES
    statusbaralldraw();
}

/* Load KSM file... */

K_INT16 loadmusic(char *filename)
{
    char buffer[256], instbuf[11];
    int infile;
    K_INT16 i, j, k, numfiles;
    K_INT32 filoffs;

    FILE *file;

    if (musicsource == -1)
    return(-1);
    if (firstime == 1)
    {
    if (musicsource == 1)
    {
        /* Open KSM->MIDI instrument translation table... */
        char path[256];
#ifndef ANDROID_NDK
        snprintf(path, sizeof(path), "%s/ksmmidi.txt", globalDataDir);
#else
        snprintf(path, sizeof(path), "%s/ksmmidi.txt", globalAndroidRWdir);
#endif
        file=fopen(path,"rt");
        if (file==NULL) {
        TO_DEBUG_LOG("ksmmidi.txt not found; music disabled.\n");
        musicsource=-1;
        return -1;
        }
        for(i=0;i<256;i++)
        fscanf(file,"%d",&gminst[i]);
        fclose(file);
    }
    if (musicsource == 2)
    {
        char path1[256];
        char path2[256];
        snprintf(path1, sizeof(path1), "%s/insts.dat", globalDataDir);
        snprintf(path2, sizeof(path2), "%s/INSTS.DAT", globalDataDir);
#ifdef ANDROID_NDK
    SDL_RWops *io = SDL_RWFromFile(path2, "rb");
    if (!io) {
        io = SDL_RWFromFile(path1, "rb");
    }
    if (!io)
#else
        if(((infile=open(path1,O_RDONLY|O_BINARY,0))==-1)&&
           ((infile=open(path2,O_RDONLY|O_BINARY,0))==-1))
#endif // ANDROID_NDK
        return(-1);
        for(i=0;i<256;i++)
        {
#ifndef ANDROID_NDK
        read(infile,&buffer,33);
#else
        SDL_RWread(io, &buffer, 33, 1);
#endif // !ANDROID_NDK
        for(j=0;j<11;j++)
            inst[i][j] = buffer[j+20];
        }
#ifndef ANDROID_NDK
        close(infile);
#else
        SDL_FreeRW(io);
#endif // !ANDROID_NDK
        numchans = 9;

        outdata((char)0,(char)0x1,(char)32);  //clear test stuff
        outdata((char)0,(char)0x4,(char)0);   //reset
        outdata((char)0,(char)0x8,(char)0);   //2-operator synthesis

        firstime = 0;
    }
    }
    char path1[256];
    char path2[256];
    snprintf(path1, sizeof(path1), "%s/songs.kzp", globalDataDir);
    snprintf(path2, sizeof(path2), "%s/SONGS.KZP", globalDataDir);
#ifdef ANDROID_NDK
    SDL_RWops *io = SDL_RWFromFile(path2, "rb");
    if (!io) {
        io = SDL_RWFromFile(path1, "rb");
    }
    if (!io)
#else
    if (((infile=open(path1,O_RDONLY|O_BINARY,0))==-1)&&
    ((infile=open(path2,O_RDONLY|O_BINARY,0))==-1))
#endif // ANDROID_NDK
    return(-1);
#ifndef ANDROID_NDK
    readLE16(infile,&numfiles,2);
#else
    SDL_RWread(io, &numfiles,2, 1);
#endif // !ANDROID_NDK
    i = 0;
    j = 1;
    while ((j == 1) && (i < numfiles))
    {
#ifndef ANDROID_NDK
    read(infile,&buffer[0],12);
#else
    SDL_RWread(io, &buffer[0],12, 1);
#endif // !ANDROID_NDK
    j = 0;
    k = 0;
    while ((filename[k] != 0) && (k < 8))
    {
        if (buffer[k] != filename[k])
        j = 1;
        k++;
    }
    i++;
    }
    if (j == 1)
    {
#ifndef ANDROID_NDK
    close(infile);
#else
    SDL_FreeRW(io);
#endif // !ANDROID_NDK
    return(-1);
    }

    filoffs=readlong((unsigned char *)buffer+8);
#ifndef ANDROID_NDK
    lseek(infile,filoffs,SEEK_SET);
    read(infile,&trinst[0],16);
    read(infile,&trquant[0],16);
    read(infile,&trchan[0],16);
    read(infile,&trprio[0],16);
    read(infile,&trvol[0],16);
    readLE16(infile,&numnotes,2);
    readLE32(infile,note,numnotes*4);
    close(infile);
#else
    SDL_RWseek(io, filoffs,SEEK_SET);
    SDL_RWread(io, &trinst[0],16, 1);
    SDL_RWread(io, &trquant[0],16, 1);
    SDL_RWread(io, &trchan[0],16, 1);
    SDL_RWread(io, &trprio[0],16, 1);
    SDL_RWread(io, &trvol[0],16, 1);
    SDL_RWread(io, &numnotes,2, 1);
    SDL_RWread(io, note,numnotes*4, 1);
    SDL_FreeRW(io);
#endif // !ANDROID_NDK
    numchans = 9-trchan[11]*3;
    if (musicsource == 1)
    setmidiinsts();
    if (musicsource == 2)
    {
    if (trchan[11] == 0)
    {
        drumstat = 0;
        outdata((char)0,(unsigned char)0xbd,(unsigned char)drumstat);
    }
    if (trchan[11] == 1)
    {
        for(i=0;i<11;i++)
        instbuf[i] = inst[trinst[11]][i];
        instbuf[1] = ((instbuf[1]&192)|((trvol[11])^63));
        setinst(0,6,instbuf[0],instbuf[1],instbuf[2],instbuf[3],instbuf[4],instbuf[5],instbuf[6],instbuf[7],instbuf[8],instbuf[9],instbuf[10]);
        for(i=0;i<5;i++)
        instbuf[i] = inst[trinst[12]][i];
        for(i=5;i<11;i++)
        instbuf[i] = inst[trinst[15]][i];
        instbuf[1] = ((instbuf[1]&192)|((trvol[12])^63));
        instbuf[6] = ((instbuf[6]&192)|((trvol[15])^63));
        setinst(0,7,instbuf[0],instbuf[1],instbuf[2],instbuf[3],instbuf[4],instbuf[5],instbuf[6],instbuf[7],instbuf[8],instbuf[9],instbuf[10]);
        for(i=0;i<5;i++)
        instbuf[i] = inst[trinst[14]][i];
        for(i=5;i<11;i++)
        instbuf[i] = inst[trinst[13]][i];
        instbuf[1] = ((instbuf[1]&192)|((trvol[14])^63));
        instbuf[6] = ((instbuf[6]&192)|((trvol[13])^63));
        setinst(0,8,instbuf[0],instbuf[1],instbuf[2],instbuf[3],instbuf[4],instbuf[5],instbuf[6],instbuf[7],instbuf[8],instbuf[9],instbuf[10]);
        outdata((char)0,(unsigned char)0xa6,(unsigned char)(600&255));
        outdata((char)0,(unsigned char)0xb6,(unsigned char)((600>>8)&223));
        outdata((char)0,(unsigned char)0xa7,(unsigned char)(400&255));
        outdata((char)0,(unsigned char)0xb7,(unsigned char)((400>>8)&223));
        outdata((char)0,(unsigned char)0xa8,(unsigned char)(5510&255));
        outdata((char)0,(unsigned char)0xb8,(unsigned char)((5510>>8)&223));
        drumstat = 32;
        outdata((char)0,(unsigned char)0xbd,(unsigned char)drumstat);
    }
    }
    return 0;
}

void outdata(unsigned char synth,unsigned char index,unsigned char data)
{
    adlib0(index,data);
}

/* Start music (and start timer!). */

void musicon()
{
    K_INT16 i, j, k, ksaystat;
    unsigned char instbuf[11];
    K_UINT32 templong;

    if (musicsource >= 0)
    {
    for(i=0;i<numchans;i++)
    {
        chantrack[i] = 0;
        chanage[i] = 0;
    }
    j = 0;
    for(i=0;i<16;i++)
        if ((trchan[i] > 0) && (j < numchans))
        {
        k = trchan[i];
        while ((j < numchans) && (k > 0))
        {
            chantrack[j] = i;
            k--;
            j++;
        }
        }
    if (musicsource==1)
        setmidiinsts();
    for(i=0;i<numchans;i++)
    {
        if (musicsource == 2)
        {
        for(j=0;j<11;j++)
            instbuf[j] = inst[trinst[chantrack[i]]][j];
        instbuf[1] = ((instbuf[1]&192)|(63-trvol[chantrack[i]]));
        setinst(0,i,instbuf[0],instbuf[1],instbuf[2],instbuf[3],instbuf[4],instbuf[5],instbuf[6],instbuf[7],instbuf[8],instbuf[9],instbuf[10]);
        }
        chanfreq[i] = 0;
    }
    }
    templong=note[0];
    count = (templong>>12)-1;
    countstop = (templong>>12)-1;
    nownote = 0;
    SDL_LockMutex(soundmutex);
    musicstatus = 1;
    ksaystat = 0;
    if (musicpan) randoinsts();

    if (ksaystat == 0)
    {
    lastTick=SDL_GetTicks();
    if (timer==NULL)
        timer=SDL_AddTimer(4,tickhandler, NULL);/* 250 Hz, should be 240.*/
    }
    SDL_UnlockMutex(soundmutex);
}

/* Stop music (and timer!). */

void musicoff()
{
    K_INT16 i, ksaystat;

    ksaystat = 0;
    if (ksaystat == 0)
    {
    if (timer!=NULL) {
        SDL_RemoveTimer(timer);
        timer=NULL;
    }
    }
    if (musicsource == 1) {
#ifdef WIN32
    midiOutReset(sequencerdevice);
#endif
#ifdef USE_OSS
    ioctl(sequencerdevice, SNDCTL_SEQ_PANIC);
    ioctl(sequencerdevice, SNDCTL_SEQ_RESET);
#endif
    setmidiinsts();
    }
    if (musicsource == 2)
    for(i=0;i<numchans;i++)
    {
        outdata((char)0,(unsigned char)(0xa0+i),(char)0);
        outdata((char)0,(unsigned char)(0xb0+i),(char)0);
    }
    SDL_LockMutex(soundmutex);
    musicstatus = 0;
    SDL_UnlockMutex(soundmutex);
}

/* SDL tick handler, complete with routine to even out timer ticks at funny
   intervals... */

void updateclock(void) {
    Uint32 now;

    now=SDL_GetTicks();

    while(((lastTick+(4+(tickFrac==0)))<=now)||(lastTick>now)) {
    if (!soundtimer) clockspeed++;
    if (musicsource!=2) ksmhandler();
    lastTick+=4+(tickFrac==0);
    tickFrac++;
    if (tickFrac==6) tickFrac=0;
    }
}

Uint32 tickhandler(Uint32 interval, void *param) {
    SDL_LockMutex(timermutex);
    updateclock();
    SDL_UnlockMutex(timermutex);
    return interval;
}

/* Update clock and music... */

void ksmhandler(void)
{
    K_INT16 i, j, quanter, bufnum, chan, drumnum, freq;
    K_UINT32 temp, templong;

    count++;
    if ((count >= countstop) && (musicsource >= 0))
    {
    bufnum = 0;
    while (count >= countstop)
    {
        templong=note[nownote];
        if (musicsource == 0)
        if ((((templong>>8)&15) == 0) && ((templong&64) > 0))
            databuf[bufnum++] = (unsigned char)(templong&63);
        if (musicsource > 0)
        {
        if (((templong&255) >= 1) && ((templong&255) <= 61))
        {
            i = 0;
            while (((chanfreq[i] != (templong&63)) || (chantrack[i] != ((templong>>8)&15))) && (i < numchans))
            i++;
            if (i < numchans)
            {
            if (musicsource == 1)
            {
                databuf[bufnum++] = (unsigned char)(0x80)+i;
                databuf[bufnum++] = (unsigned char)(templong&63)+
                23;
                databuf[bufnum++] = (unsigned char)0;
            }
            if (musicsource == 2)
            {
                databuf[bufnum++] = (unsigned char)(0xa0+i);
                databuf[bufnum++] = (unsigned char)(adlibfreq[templong&63]&255);
                databuf[bufnum++] = (unsigned char)(0xb0+i);
                databuf[bufnum++] = (unsigned char)((adlibfreq[templong&63]>>8)&223);
            }
            chanfreq[i] = 0;
            chanage[i] = 0;
            }
        }
        else if (((templong&255) >= 65) && ((templong&255) <= 125))
        {
            if (((templong>>8)&15) < 11)
            {
            temp = 0;
            i = numchans;
            for(j=0;j<numchans;j++)
                if ((countstop - chanage[j] >= temp) && (chantrack[j] == ((templong>>8)&15)))
                {
                temp = countstop - chanage[j];
                i = j;
                }
            if (i < numchans)
            {
                if (musicsource == 1)
                {
                if (chanfreq[i]!=0) {
                    databuf[bufnum++] = (unsigned char)
                    (0x80+i);
                    databuf[bufnum++] = (unsigned char)
                    chanfreq[i]+23;
                    databuf[bufnum++] = (unsigned char)0;
                }
                databuf[bufnum++] = (unsigned char)(0x90+i);
                databuf[bufnum++] = (unsigned char)(templong&63)+23;
                databuf[bufnum++] = (unsigned char)
                    trvol[chantrack[i]]<<1;
                }
                if (musicsource == 2)
                {
                databuf[bufnum++] = (unsigned char)(0xa0+i);
                databuf[bufnum++] = (unsigned char)0;
                databuf[bufnum++] = (unsigned char)(0xb0+i);
                databuf[bufnum++] = (unsigned char)0;
                databuf[bufnum++] = (unsigned char)(0xa0+i);
                databuf[bufnum++] = (unsigned char)(adlibfreq[templong&63]&255);
                databuf[bufnum++] = (unsigned char)(0xb0+i);
                databuf[bufnum++] = (unsigned char)((adlibfreq[templong&63]>>8)|32);
                }
                chanfreq[i] = templong&63;
                chanage[i] = countstop;
            }
            }
            else
            {
            if (musicsource == 1)
            {
                databuf[bufnum++] = (unsigned char)(0x99);
                switch((templong>>8)&15)
                {
                case 11: drumnum = 36; break;
                case 12: drumnum = 38; break;
                case 13: drumnum = 37; break;
                case 14: drumnum = 51; break;
                case 15: drumnum = 42; break;
                default: drumnum = 36; break;
                }
                databuf[bufnum++] = (unsigned char)drumnum;
                databuf[bufnum++] = (unsigned char)64;
            }
            if (musicsource == 2)
            {
                freq = adlibfreq[templong&63];
                switch((unsigned char)((templong>>8)&15))
                {
                case 11: drumnum = 16; chan = 6; freq -= 2048; break;
                case 12: drumnum = 8; chan = 7; freq -= 2048; break;
                case 13: drumnum = 4; chan = 8; break;
                case 14: drumnum = 2; chan = 8; break;
                case 15: drumnum = 1; chan = 7; freq -= 2048; break;
                default: drumnum = 16; chan=6; freq-=2048; break;
                }
                databuf[bufnum++] = (unsigned char)(0xa0+chan);
                databuf[bufnum++] = (unsigned char)(freq&255);
                databuf[bufnum++] = (unsigned char)(0xb0+chan);
                databuf[bufnum++] = (unsigned char)((freq>>8)&223);
                databuf[bufnum++] = (unsigned char)(0xbd);
                databuf[bufnum++] = (((unsigned char)(drumstat))&((unsigned char)(255-drumnum)));
                drumstat |= drumnum;
                databuf[bufnum++] = (unsigned char)(0xbd);
                databuf[bufnum++] = (unsigned char)(drumstat);
            }
            }
        }
        }
        nownote++;
        if (nownote >= numnotes)
        nownote = 0;
        templong=note[nownote];
        if (nownote == 0)
        count = (templong>>12)-1;
        quanter = (240/trquant[(templong>>8)&15]);
        countstop = (((templong>>12)+(quanter>>1)) / quanter) * quanter;
    }
    if (mute == 0)
    {
        if (musicsource == 0)
        {
        j = 0;
        for(i=0;i<bufnum;i++)
            if (databuf[i] > j)
            j = databuf[i];
        if (j == 0)
        {
            /* Removed: Mute beeper. */
        }
        if (j > 0)
        {
            /* Removed: Beep at pcfreq[j]. */
        }
        }
        if (musicsource == 1) {
#ifdef WIN32
        for(i=0;i<bufnum;i+=3) {
            midiOutShortMsg(sequencerdevice,databuf[i]|(databuf[i+1]<<8)|(databuf[i+2]<<16));
        }
#endif
#ifdef USE_OSS
        for(i=0;i<bufnum;i++)
        {
            SEQ_MIDIOUT(nrmidis-1, databuf[i]);
        }
        SEQ_DUMPBUF();
#endif
        }
        if (musicsource == 2)
        for(i=0;i<bufnum;i+=2)
        {
            outdata(0,databuf[i],databuf[i+1]);
        }
    }
    }
}

/* Adlib set instrument... */

void setinst(unsigned char synth,K_INT16 chan,unsigned char v0,
         unsigned char v1,unsigned char v2,unsigned char v3,
         unsigned char v4,unsigned char v5,unsigned char v6,
         unsigned char v7,unsigned char v8,unsigned char v9,
         unsigned char v10)
{
    K_INT16 offs;

    outdata(synth,(unsigned char)(0xa0+chan),(unsigned char)0);
    outdata(synth,(unsigned char)(0xb0+chan),(unsigned char)0);
    outdata(synth,(unsigned char)(0xc0+chan),v10);
/*    if (chan == 0)*/
    offs = 0;
    if (chan == 1)
    offs = 1;
    if (chan == 2)
    offs = 2;
    if (chan == 3)
    offs = 8;
    if (chan == 4)
    offs = 9;
    if (chan == 5)
    offs = 10;
    if (chan == 6)
    offs = 16;
    if (chan == 7)
    offs = 17;
    if (chan == 8)
    offs = 18;
    outdata(synth,(unsigned char)(0x20+offs),v5);
    outdata(synth,(unsigned char)(0x40+offs),v6);
    outdata(synth,(unsigned char)(0x60+offs),v7);
    outdata(synth,(unsigned char)(0x80+offs),v8);
    outdata(synth,(unsigned char)(0xe0+offs),v9);
    offs+=3;
    outdata(synth,(unsigned char)(0x20+offs),v0);
    outdata(synth,(unsigned char)(0x40+offs),v1);
    outdata(synth,(unsigned char)(0x60+offs),v2);
    outdata(synth,(unsigned char)(0x80+offs),v3);
    outdata(synth,(unsigned char)(0xe0+offs),v4);
}

/* MIDI set instruments. */

void setmidiinsts()
{
#ifdef WIN32
    int i;

    midiOutReset(sequencerdevice);
    for(i=0;i<16;i++)
        midiOutShortMsg(sequencerdevice,0xc0|i|(((i<numchans)?gminst[trinst[chantrack[i]]]:0)<<8));
#endif
#ifdef USE_OSS
    int i;

    _seqbufptr = 0;
    ioctl(sequencerdevice, SNDCTL_SEQ_RESET);
    for (i = 0; i < 16; i++) {
    SEQ_CONTROL(nrmidis-1,i,0,0);
    SEQ_CONTROL(nrmidis-1,i,32,0);
        SEQ_MIDIOUT(nrmidis-1, MIDI_PGM_CHANGE + i);
/*	if (i<numchans) {
    printf("Channel %d: KSM inst %d -> GM inst %d.\n", i,
    trinst[chantrack[i]],gminst[trinst[chantrack[i]]]);
    }*/
    SEQ_MIDIOUT(nrmidis-1, (i<numchans)?gminst[trinst[chantrack[i]]]:0);
    }
    SEQ_DUMPBUF();
#endif
}

/* Did we hit a wall? If so, push us out of it... */

void checkhitwall(K_UINT16 oposx,K_UINT16 oposy,K_UINT16 posix,
          K_UINT16 posiy)
{
    K_INT16 i, j, k, m, xdir, ydir, cntx, cnty, xpos, ypos, xinc, yinc;
    K_UINT16 x1, y1, x2, y2, x3, y3, xspan, yspan;
    K_INT32 templong;

    if (!cliptowall) {
    posx=posix;
    posy=posiy;
    if (posx<256) posx=256;
    if (posy<256) posy=256;
    if (posx>65536-256) posx=65536-256;
    if (posy>65536-256) posy=65536-256;
    return;
    }
    if (oposx < posix)
    {
    x1 = (oposx>>10);
    xdir = 1;
    xinc = posix-oposx;
    }
    else
    {
    x1 = (posix>>10);
    xdir = -1;
    xinc = oposx-posix;
    }
    if (oposy < posiy)
    {
    y1 = (oposy>>10);
    ydir = 1;
    yinc = posiy-oposy;
    }
    else
    {
    y1 = (posiy>>10);
    ydir = -1;
    yinc = oposy-posiy;
    }
    xspan = abs(((int)(posix>>10))-((int)(oposx>>10)))+1;
    yspan = abs(((int)(posiy>>10))-((int)(oposy>>10)))+1;
    xpos = oposx-(x1<<10);
    ypos = oposy-(y1<<10);
    for(i=0;i<=(xspan<<2);i++)
    {
    x2 = ((i+3)>>2)+x1-1;
    x3 = ((i+5)>>2)+x1-1;
    if (((x2|x3)&0xffc0) == 0)
        for(j=0;j<=(yspan<<2);j++)
        {
        y2 = ((j+3)>>2)+y1-1;
        y3 = ((j+5)>>2)+y1-1;
        k = i+(j<<6);
        tempbuf[k] = 0;
        if (((y2|y3)&0xffc0) == 0)
        {
            m = board[x2][y2];
            if (((m&3072) != 1024) && ((m&1023) != 0)) tempbuf[k] = 1;
            m = board[x2][y3];
            if (((m&3072) != 1024) && ((m&1023) != 0)) tempbuf[k] = 1;
            m = board[x3][y2];
            if (((m&3072) != 1024) && ((m&1023) != 0)) tempbuf[k] = 1;
            m = board[x3][y3];
            if (((m&3072) != 1024) && ((m&1023) != 0)) tempbuf[k] = 1;
        }
        else
            tempbuf[k] = 1;
        }
    }
    cntx = 0, cnty = 0;
    for(i=0;i<256;i++)
    {
    cntx+=xinc;
    j=cntx>>8;
    cntx&=255;
    for(k=0;k<j;k++) {
        if (tempbuf[((xpos+xdir)>>8)+((ypos&0xff00)>>2)]==0)
        xpos+=xdir;
    }

    cnty+=yinc;
    j=cnty>>8;
    cnty&=255;
    for(k=0;k<j;k++) {
        if (tempbuf[(xpos>>8)+(((ypos+ydir)&0xff00)>>2)]==0)
        ypos+=ydir;
    }
    }
    posx = xpos + (((unsigned)x1)<<10);
    posy = ypos + (((unsigned)y1)<<10);
    if (((posx&0xfc00) != (oposx&0xfc00)) || ((posy&0xfc00) != (oposy&0xfc00)))
    {
    if ((board[posx>>10][posy>>10]&1023) == stairs)
    {
        if ((boardnum >= 10) && (boardnum < 20))
        {
        j = -1;
        for(i=0;i<mnum;i++)
            if (mstat[i] == mondog)
            {
            if (mposx[i] > posx) templong = (long)(mposx[i]-posx);
            else templong = (long)(posx-mposx[i]);
            if (mposy[i] > posy) templong += (long)(mposy[i]-posy);
            else templong += (long)(posy-mposy[i]);
            j = 0;
            if (templong >= 4096)
                j = 1;
            }
        if (j != 0)
        {
            posx = oposx;
            posy = oposy;
            mixing=1;
            strcpy(&textbuf[0],"Where's your dog?");
            wipeoverlay(112,40,200,10);
            textprint(112,40+1,(char)96);
            mixing=0;
        }
        }
    }
    youarehere();
    }
}

/* Palette for text in endgame sequences... */

unsigned char textpalette[48]={
    3,3,3,
    7,7,7,
    11,11,11,
    15,15,15,
    19,19,19,
    23,23,23,
    27,27,27,
    31,31,31,
    35,35,35,
    39,39,39,
    43,43,43,
    47,47,47,
    51,51,51,
    55,55,55,
    59,59,59,
    63,63,63
};

/* Win episode 1 or 2... */

void wingame(K_INT16 episode)
{
    K_UINT16 l, startx, starty;
    K_INT16 oldmute;
    K_INT32 revtotalclock, revototclock, templong;

    K_INT32 tempototclock=ototclock,temptotalclock=totalclock;

    oldmute = mute;
    revototclock = -1;
    revtotalclock = 0;
    musicoff();
    if (episode == 1) loadmusic("WINGAME0");
    if (episode == 2) loadmusic("WINGAME1");
    musicon();
    posz = 32;
    if (episode == 1)
    {
    startx = (((unsigned)58)<<10);
    starty = (((unsigned)19)<<10);
    }
    else
    {
    startx = (((unsigned)5)<<10);
    starty = (((unsigned)12)<<10);
    }
    while ((death != 4096) || ((keystatus[1] == 0) && (keystatus[57] == 0) && (keystatus[28] == 0) && (bstatus == 0)))
    {
    PollInputs();
    bstatus = 0;
    if (moustat == 0)
    {
        bstatus=readmouse(NULL, NULL);
    }
    if (joystat == 0)
    {
        bstatus|=readjoystick(NULL,NULL);
    }
    if (death != 4096)
    {
        ang = 512;
        posx = (startx+512);
        if (revtotalclock < 3840)
        posy = starty+512+(unsigned)((revtotalclock<<6)/15);
        else
        posy = starty+16384+512;
        picrot(posx,posy,posz,ang);
        drawOnScreen();
        sortcnt = 0;
        SDL_LockMutex(soundmutex);
        SDL_LockMutex(timermutex);
        if ((musicstatus == 1) && (clockspeed >= 0) && (clockspeed < 3))
        while(clockspeed<3) {
            SDL_UnlockMutex(timermutex);
            SDL_UnlockMutex(soundmutex);
            SDL_Delay(10);
            SDL_LockMutex(soundmutex);
            SDL_LockMutex(timermutex);
        }
        SDL_UnlockMutex(timermutex);
        SDL_UnlockMutex(soundmutex);
    }
    if (episode == 2)
    {
        if ((revtotalclock >= 840) && (revototclock < 840))
        {
        life += 1280;
        if (life > 4095)
            life = 4095;
        drawlife();
        ksay(7);
        }
        if ((revtotalclock >= 1560) && (revototclock < 1560))
        {
        life += 1280;
        if (life > 4095)
            life = 4095;
        drawlife();
        ksay(7);
        }
    }
    if (revtotalclock >= 3840)
    {
        if (revototclock < 3840)
        {
        death = 4094;
        if (episode == 1) ksay(6);
        if (episode == 2) ksay(9);
        mute=mute?1:2;
        }
        if (death < 4095)
        {
        if (death > 0)
        {
            posz+=2;
            if (posz > 64)
            posz = 64;
            death -= (((((int)(revtotalclock-revototclock))<<5)&0xff80)+128);
            if (death <= 0)
            {
            death = 0;
            posz = 32;
            }
            fade(death>>6);
        }
        if ((death == 0) && (revtotalclock >= 4800))
        {
            templong=note[0];

            SDL_LockMutex(timermutex);
            count = (templong>>12)-1;
            countstop = (templong>>12)-1;
            nownote = 0;
            mute = oldmute;
            SDL_UnlockMutex(timermutex);

            death = 4096;
            if ((vidmode == 0) && (statusbar < 399))
            {
            l = times90[((unsigned)statusbar+1)>>1]+5;
            }
            if ((vidmode == 1) && (statusbar < 479))
            {
            l = times90[((unsigned)statusbar+1)>>1];
            }
            fade(63);
            setdarkenedpalette();
            glClearColor( 0,0,0,0 );
            glClear(GL_COLOR_BUFFER_BIT);
            wipeoverlay(0,0,361,241);
            drawmenu(304,192,menu);

            updateoverlaypalette(240,16,textpalette);

            if (episode == 1) loadstory(-21);
            if (episode == 2) loadstory(-19);

            finalisemenu();

            ksay(23);
            drawOnScreen();
            pressakey();
            glClear(GL_COLOR_BUFFER_BIT);
            drawmenu(304,192,menu);
            keystatus[1] = 0;
            keystatus[57] = 0;
            keystatus[28] = 0;
#ifndef USE_SDL2
            newkeystatus[SDLK_ESCAPE]=0;
            newkeystatus[SDLK_RETURN]=0;
            newkeystatus[SDLK_SPACE]=0;
#else
            newkeystatus[getOldAsciiKeyCode(SDLK_ESCAPE)]=0;
            newkeystatus[getOldAsciiKeyCode(SDLK_RETURN)]=0;
            newkeystatus[getOldAsciiKeyCode(SDLK_SPACE)]=0;
#endif // !USE_SDL2
            if (episode == 1) loadstory(-20);
            if (episode == 2) loadstory(-18);
            finalisemenu();
            settransferpalette();
            drawOnScreen();
            pressakey();
        }
        }
    }
    revototclock = revtotalclock;
    SDL_LockMutex(timermutex);
    if (clockspeed==0) {
        SDL_UnlockMutex(timermutex);
        SDL_Delay(10); /* To avoid soaking up all CPU. */
        SDL_LockMutex(timermutex);
    }
    revtotalclock += clockspeed;
    totalclock += clockspeed;
    animate2 = animate2 ^ 1;
    clockspeed = 0;
    SDL_UnlockMutex(timermutex);
    }
    musicoff();
    fade(0);

    /* Previously -1 and 1, but this gave the player all the cloaks and potions
       he had in the previous episode for a time corresponding to the time to
       the time until he last ran out of them. Probably for half an hour or so.
       I hope this wasn't a feature. */

    ototclock = tempototclock;
    totalclock = temptotalclock;
    linecompare(statusbar);
    fade(63);
    statusbaralldraw();
    fadewarpval = 63;
    fadehurtval = 0;
    keystatus[57] = 0;
    keystatus[28] = 0;
    keystatus[1] = 0;
#ifndef USE_SDL2
    newkeystatus[SDLK_ESCAPE]=0;
    newkeystatus[SDLK_RETURN]=0;
    newkeystatus[SDLK_SPACE]=0;
#else
    newkeystatus[getOldAsciiKeyCode(SDLK_ESCAPE)]=0;
    newkeystatus[getOldAsciiKeyCode(SDLK_RETURN)]=0;
    newkeystatus[getOldAsciiKeyCode(SDLK_SPACE)]=0;
#endif // !USE_SDL2
    bstatus = 0;
    lastunlock = 1;
    lastshoot = 1;
    lastbarchange = 1;
    death = 4095;
}

/* Win episode 3... */

void winallgame()
{
    K_INT16 leavewin;
    K_INT32 revtotalclock, revototclock;

    ingame=0;
    revototclock = 1;
    revtotalclock = 1;
    linecompare(479);
    musicoff();
    loadmusic("WINGAME2");
    musicon();

    fade(63);
    leavewin = 0;
    bstatus = 0;
    keystatus[1] = 0;
    keystatus[57] = 0;
    keystatus[28] = 0;
#ifndef USE_SDL2
    newkeystatus[SDLK_ESCAPE]=0;
    newkeystatus[SDLK_RETURN]=0;
    newkeystatus[SDLK_SPACE]=0;
#else
    newkeystatus[getOldAsciiKeyCode(SDLK_ESCAPE)]=0;
    newkeystatus[getOldAsciiKeyCode(SDLK_RETURN)]=0;
    newkeystatus[getOldAsciiKeyCode(SDLK_SPACE)]=0;
#endif // !USE_SDL2
    while ((leavewin == 0) || ((keystatus[1] == 0) && (keystatus[57] == 0) && (keystatus[28] == 0) && (bstatus == 0)))
    {
    PollInputs();
    bstatus = 0;
    if (moustat == 0) {
        bstatus=readmouse(NULL,NULL);
    }
    if (joystat == 0)
    {
        bstatus|=readjoystick(NULL,NULL);
    }
    if (revtotalclock < 3260)
    {
        /*
          for(i=lside;i<rside;i++)
          height[i] = 0;
        */
        glClearColor( 0,0,0,0 );
        glClear(GL_COLOR_BUFFER_BIT);
        pictur(180,halfheight,4+(((int)revtotalclock)>>2),((int)((revtotalclock<<2))&2047)^2047,earth);
        drawOnScreen();
    }
    else
    {
        glClearColor( 0,0,0,0 );
        glClear(GL_COLOR_BUFFER_BIT);
        wipeoverlay(0,0,361,241);
        setdarkenedpalette();
        drawmenu(304,192,menu);
        updateoverlaypalette(240,16,textpalette);
        fade(63);
        loadstory(-17);
        finalisemenu();
        ksay(23);
        drawOnScreen();
        pressakey();
        glClear(GL_COLOR_BUFFER_BIT);
        drawmenu(304,192,menu);
        keystatus[1] = 0;
        keystatus[57] = 0;
        keystatus[28] = 0;
#ifndef USE_SDL2
        newkeystatus[SDLK_ESCAPE]=0;
        newkeystatus[SDLK_RETURN]=0;
        newkeystatus[SDLK_SPACE]=0;
#else
        newkeystatus[getOldAsciiKeyCode(SDLK_ESCAPE)]=0;
        newkeystatus[getOldAsciiKeyCode(SDLK_RETURN)]=0;
        newkeystatus[getOldAsciiKeyCode(SDLK_SPACE)]=0;
#endif // !USE_SDL2
        loadstory(-16);
        finalisemenu();
        settransferpalette();
        drawOnScreen();
        pressakey();
        leavewin = 1;
    }
    revototclock = revtotalclock;
    SDL_LockMutex(timermutex);
    if (clockspeed==0) {
        SDL_UnlockMutex(timermutex);
        SDL_Delay(10); /* To avoid soaking up all CPU. */
        SDL_LockMutex(timermutex);
    }
    revtotalclock += clockspeed;
    totalclock += clockspeed;
    clockspeed = 0;
    SDL_UnlockMutex(timermutex);
    }
    musicoff();
    fade(0);
    ototclock = -1;
    totalclock = 1;
    linecompare(statusbar);
    fade(63);
    fadewarpval = 63;
    fadehurtval = 0;
    keystatus[57] = 0;
    keystatus[28] = 0;
    keystatus[1] = 0;
#ifndef USE_SDL2
    newkeystatus[SDLK_ESCAPE]=0;
    newkeystatus[SDLK_RETURN]=0;
    newkeystatus[SDLK_SPACE]=0;
#else
    newkeystatus[getOldAsciiKeyCode(SDLK_ESCAPE)]=0;
    newkeystatus[getOldAsciiKeyCode(SDLK_RETURN)]=0;
    newkeystatus[getOldAsciiKeyCode(SDLK_SPACE)]=0;
#endif // !USE_SDL2
    bstatus = 0;
    lastunlock = 1;
    lastshoot = 1;
    lastbarchange = 1;
    death = 4095;
}

/* Darkened palette for end of game sequences... */

void setdarkenedpalette() {
    K_INT16 a;

    for(a=0;a<256;a++) {
    ipalr[a]=(palette[a*3]*27)>>3;
    ipalg[a]=(palette[a*3+1]*27)>>3;
    ipalb[a]=(palette[a*3+2]*27)>>3;

    Red[a]=palette[a*3]*27/4096.0;
    Green[a]=palette[a*3+1]*27/4096.0;
    Blue[a]=palette[a*3+2]*27/4096.0;
    Alpha[a]=1.0;
    }

    if (ingame)
    Red[255]=Green[255]=Blue[255]=Alpha[255]=0.0;

#ifndef OPENGLES
    glPixelMapfv(GL_PIXEL_MAP_I_TO_R,256,Red);
    glPixelMapfv(GL_PIXEL_MAP_I_TO_G,256,Green);
    glPixelMapfv(GL_PIXEL_MAP_I_TO_B,256,Blue);
    glPixelMapfv(GL_PIXEL_MAP_I_TO_A,256,Alpha);
#endif // OPENGLES

    ConvertPartialOverlay(0,0,360,240);
}

/* Normal palette... */

void settransferpalette() {
    K_INT16 a;

    for(a=0;a<256;a++) {
    ipalr[a]=(palette[a*3]*27)>>3;
    ipalg[a]=(palette[a*3+1]*27)>>3;
    ipalb[a]=(palette[a*3+2]*27)>>3;

    Red[a]=palette[a*3]/64.0;
    Green[a]=palette[a*3+1]/64.0;
    Blue[a]=palette[a*3+2]/64.0;
    Alpha[a]=1.0;
    }

    if (ingame)
    Red[255]=Green[255]=Blue[255]=Alpha[255]=0.0;

#ifndef OPENGLES
    glPixelMapfv(GL_PIXEL_MAP_I_TO_R,256,Red);
    glPixelMapfv(GL_PIXEL_MAP_I_TO_G,256,Green);
    glPixelMapfv(GL_PIXEL_MAP_I_TO_B,256,Blue);
    glPixelMapfv(GL_PIXEL_MAP_I_TO_A,256,Alpha);
#endif // !OPENGLES

    ConvertPartialOverlay(0,0,360,240);
}

/* Change some of the palette... */

void updateoverlaypalette(K_UINT16 start,K_UINT16 amount,unsigned char *cols) {
    K_INT16 i;

    for(i=0;i<amount;i++) {
    ipalr[i+start]=(cols[i*3]*27)>>3;
    ipalg[i+start]=(cols[i*3+1]*27)>>3;
    ipalb[i+start]=(cols[i*3+2]*27)>>3;

    Red[i+start]=cols[i*3]/64.0;
    Green[i+start]=cols[i*3+1]/64.0;
    Blue[i+start]=cols[i*3+2]/64.0;
    Alpha[i+start]=1.0;
    }

    if (ingame)
    Red[255]=Green[255]=Blue[255]=Alpha[255]=0.0;

#ifndef OPENGLES
    glPixelMapfv(GL_PIXEL_MAP_I_TO_R,256,Red);
    glPixelMapfv(GL_PIXEL_MAP_I_TO_G,256,Green);
    glPixelMapfv(GL_PIXEL_MAP_I_TO_B,256,Blue);
    glPixelMapfv(GL_PIXEL_MAP_I_TO_A,256,Alpha);
#endif // !OPENGLES

    ConvertPartialOverlay(0,0,360,240);
}

/* Fade... */

void fade(K_INT16 brightness)
{
    int old;
    old=fadelevel;

    fadelevel=brightness;

    if (brightness == 63) {
    redfactor=greenfactor=bluefactor=1.0;
    }
    else if (brightness < 64) {
    redfactor=greenfactor=bluefactor=brightness/64.0;
    }
    else if (brightness < 128) {
    redfactor=greenfactor=bluefactor=(brightness-64)/64.0;
    }
    else {
    redfactor=(brightness-65)/64.0;
    greenfactor=bluefactor=1.0;

    /* Factors can't exceed 1.0, so we scale them all (makes the hurting
       a bit darker, but a lot easier/faster). */

    greenfactor/=redfactor;
    bluefactor/=redfactor;
    redfactor=1.0;

    }
}

/* Show the compass. */

void showcompass(K_INT16 compang)
{
    K_INT16 i;

    i = (((compang+64)&2047)>>7);
    statusbardraw((i&2)<<4,((i&1)<<5)+2,29,29,238,1+statusbaryoffset,(i>>2)+compassplc);
}

/* Load a GIF. Violate a patent.
   0-2 = GIFs in LAB3D.KZP, -1 = lab3d.gif, -2 to -4 = endn.gif. */
K_INT16 kgif(K_INT16 filenum)
{
    unsigned char header[13], imagestat[10], bitcnt, numbits;
    unsigned char globalflag, localflag, backg, chunkind;
    K_INT16 i, j, k, x, y, xdim, ydim;
    K_INT16 numcols, numpals, fil, blocklen;
    K_UINT16 rowpos,numlines, gifdatacnt,firstring,currstr,bytecnt,numbitgoal;

    K_UINT16 stack[LZW_STACK_SIZE];
    K_UINT16 stackp=0,dat;

    memset(screenbuffer,(filenum==2)?255:((filenum==1)?0x50:0),
       screenbufferwidth*screenbufferheight);

#ifdef ANDROID
    SDL_RWops *io = NULL;
#endif

    if (filenum<0) {
    switch(filenum) {
        case -1: {
        char path1[256];
        char path2[256];
        snprintf(path1, sizeof(path1), "%s/lab3d.gif", globalDataDir);
        snprintf(path2, sizeof(path2), "%s/LAB3D.GIF", globalDataDir);
#ifdef ANDROID_NDK
        io = SDL_RWFromFile(path2, "rb");
        if (!io) {
            io = SDL_RWFromFile(path1, "rb");
        }
        if (!io)
#else
        if (((fil = open(path1,O_RDONLY|O_BINARY,0)) == -1)&&
            ((fil = open(path2,O_RDONLY|O_BINARY,0)) == -1))
#endif // ANDROID_NDK
            return(-1);
        break;
    }
    case -2: {
        char path1[256];
        char path2[256];
        snprintf(path1, sizeof(path1), "%s/end1.gif", globalDataDir);
        snprintf(path2, sizeof(path2), "%s/END1.GIF", globalDataDir);
#ifdef ANDROID_NDK
        io = SDL_RWFromFile(path2, "rb");
        if (!io) {
            io = SDL_RWFromFile(path1, "rb");
        }
        if (!io)
#else
        if (((fil = open(path1,O_RDONLY|O_BINARY,0)) == -1)&&
            ((fil = open(path2,O_RDONLY|O_BINARY,0)) == -1))
#endif // ANDROID_NDK
            return(-1);
        break;
    }
    case -3: {
        char path1[256];
        char path2[256];
        snprintf(path1, sizeof(path1), "%s/end2.gif", globalDataDir);
        snprintf(path2, sizeof(path2), "%s/END2.GIF", globalDataDir);
#ifdef ANDROID_NDK
        io = SDL_RWFromFile(path2, "rb");
        if (!io) {
            io = SDL_RWFromFile(path1, "rb");
        }
        if (!io)
#else
        if (((fil = open(path1,O_RDONLY|O_BINARY,0)) == -1)&&
            ((fil = open(path2,O_RDONLY|O_BINARY,0)) == -1))
#endif // ANDROID_NDK
            return(-1);
        break;
    }
    case -4: {
        char path1[256];
        char path2[256];
        snprintf(path1, sizeof(path1), "%s/end3.gif", globalDataDir);
        snprintf(path2, sizeof(path2), "%s/END3.GIF", globalDataDir);
#ifdef ANDROID_NDK
        io = SDL_RWFromFile(path2, "rb");
        if (!io) {
            io = SDL_RWFromFile(path1, "rb");
        }
        if (!io)
#else
        if (((fil = open(path1,O_RDONLY|O_BINARY,0)) == -1)&&
            ((fil = open(path2,O_RDONLY|O_BINARY,0)) == -1))
#endif // ANDROID_NDK
            return(-1);
        break;
        default:
        return(-1);
    }
    }
    } else {
        char path1[256];
        char path2[256];
        snprintf(path1, sizeof(path1), "%s/lab3d.kzp", globalDataDir);
        snprintf(path2, sizeof(path2), "%s/LAB3D.KZP", globalDataDir);
#ifdef ANDROID_NDK
        io = SDL_RWFromFile(path2, "rb");
        if (!io) {
            io = SDL_RWFromFile(path1, "rb");
        }
        if (!io)
#else
    if (((fil = open(path1,O_RDONLY|O_BINARY,0)) == -1)&&
        ((fil = open(path2,O_RDONLY|O_BINARY,0)) == -1))
#endif // ANDROID_NDK
        return(-1);
    }

    rowpos = 0;
    if (filenum == 1)
    {
#ifndef ANDROID_NDK
        lseek(fil,giflen1,SEEK_SET);
#else
        SDL_RWseek(io, giflen1,SEEK_SET);
#endif
    rowpos = 5;
    }
    if (filenum == 2)
#ifndef ANDROID_NDK
    lseek(fil,giflen1+giflen2,SEEK_SET);
#else
    SDL_RWseek(io, giflen1+giflen2,SEEK_SET);
#endif
    gifdatacnt = 0;
#ifndef ANDROID_NDK
    read(fil,&tempbuf[0],(unsigned)gifbuflen);
#else
    SDL_RWread(io, &tempbuf[0],(unsigned)gifbuflen, 1);
#endif
    for(j=0;j<13;j++)
    header[j] = tempbuf[j+gifdatacnt];
    gifdatacnt += 13;
/*    if ((header[0] != 'L') || (header[1] != 'A') || (header[2] != 'B'))
      return(-1);
      if ((header[3] != '3') || (header[4] != 'D') || (header[5] != '!'))
      return(-1);*/
    globalflag = header[10];
    numcols = (1<<((globalflag&7)+1));
    firstring = numcols+2;
    backg = header[11];
    if (header[12] != 0)
    return(-1);
    if ((globalflag&128) > 0)
    {
    numpals = numcols+numcols+numcols;
    for(j=0;j<numpals;j++)
        palette[j]=tempbuf[j+gifdatacnt]>>2;
    gifdatacnt += numpals;
    }
    chunkind = tempbuf[gifdatacnt], gifdatacnt++;
    while (chunkind == '!')
    {
    gifdatacnt++;
    do
    {
        chunkind = tempbuf[gifdatacnt], gifdatacnt++;
        if (chunkind > 0)
        gifdatacnt += chunkind;
    }
    while (chunkind > 0);
    chunkind = tempbuf[gifdatacnt], gifdatacnt++;
    }
    if (chunkind == ',')
    {
    for(j=0;j<9;j++)
        imagestat[j] = tempbuf[j+gifdatacnt];
    gifdatacnt += 9;
    xdim = imagestat[4]+(imagestat[5]<<8);
    ydim = imagestat[6]+(imagestat[7]<<8);
    localflag = imagestat[8];
    if ((localflag&128) > 0)
    {
        numpals = numcols+numcols+numcols;
        for(j=0;j<numpals;j++)
        palette[j]=tempbuf[j+gifdatacnt];

        gifdatacnt += numpals;
    }
    gifdatacnt++;
    numlines = 200;


    x = 20, y = (filenum==2)?0:20;
    bitcnt = 0;
    for(i=1;i<=numcols;i++) {
        lzwbuf[i]=i&255;
        lzwbuf2[i]=i;
    }
    lzwbuf2[0]=0;
    lzwbuf[0]=0;

    currstr = firstring;
    numbits = (globalflag&7)+2;
    numbitgoal = (numcols<<1);
    blocklen = 0;
    blocklen = tempbuf[gifdatacnt], gifdatacnt++;

    for(j=0;j<blocklen;j++) {
        ((unsigned char *)(lincalc))[j]=tempbuf[j+gifdatacnt];
    }

    gifdatacnt += blocklen;
    bytecnt = 0;

    while (y < ydim+((filenum==2)?0:20))
    {
        dat=(((((unsigned char *)lincalc)[bytecnt])|
          ((K_UINT16)(((unsigned char *)lincalc)[bytecnt+1]))<<8)>>bitcnt)&
        ((1<<numbits)-1);
/*	    dat=((*((K_UINT16 *)(((unsigned char *)lincalc)+bytecnt)))
        >>bitcnt)&
        ((1<<numbits)-1);*/
        if (bitcnt+numbits>16) {
        dat+=(((K_UINT16)((unsigned char *)lincalc)[bytecnt+2])&
              ((1<<((bitcnt+numbits)&15))-1))<<
            (16-bitcnt);
        }

        bitcnt+=numbits;
        bytecnt+=bitcnt>>3;
        bitcnt&=7;

        if (bytecnt > blocklen-3)
        {
        writeshort((unsigned char *)lincalc,
               readshort(&(((unsigned char *)lincalc)[bytecnt])));

        i = blocklen-bytecnt;

        blocklen=tempbuf[gifdatacnt++];

        if (gifdatacnt+blocklen < gifbuflen)
        {
            memcpy(((unsigned char *)lincalc)+i,
               tempbuf+gifdatacnt,
               (blocklen+1)&~1);
            gifdatacnt += blocklen;
        }
        else
        {
            k = gifbuflen-gifdatacnt;
            memcpy(((unsigned char *)lincalc)+i,
               tempbuf+gifdatacnt,
               (k+1)&~1);
#ifndef ANDROID_NDK
            read(fil,&tempbuf[0],
             (unsigned)gifbuflen);
#else
            SDL_RWread(io, &tempbuf[0],
             (unsigned)gifbuflen, 1);
#endif
            memcpy(((unsigned char *)lincalc)+i+k,
               tempbuf,
               ((gifdatacnt=blocklen-k)+1)&~1);

        }
        bytecnt = 0;
        blocklen += i;
        }
        if (currstr == numbitgoal)
        if (numbits < 12)
        {
            numbits++;
            numbitgoal <<= 1;
        }
        if (dat == numcols)
        {
        currstr = firstring;
        numbits = (globalflag&7)+2;
        numbitgoal = (numcols<<1);
        }
        else
        {
        lzwbuf2[currstr]=dat;
        while(dat>=firstring) {
            stack[stackp++]=lzwbuf[dat];

            dat=lzwbuf2[dat];
        }

        lzwbuf[currstr-1]=dat&255;
        lzwbuf[currstr]=dat&255;

        dat=lzwbuf2[dat];
        stack[stackp++]=dat;

        while(stackp>0) {
            stackp--;

            screenbuffer[y*screenbufferwidth+x]=stack[stackp];

            x++;
            if (x>=340) {
            y++;
            x-=320;
            }
        }

        currstr++;
        }
    }
    }
#ifndef ANDROID_NDK
    close(fil);
#else
    SDL_FreeRW(io);
#endif

    UploadOverlay();

    return 0;
}

/* Upload rectangular part of overlay from memory to specified texture. */

void UploadPartialOverlayToTexture(int x,int y,int dx,int dy,int w,int h,
                   GLuint tex,int create) {
#ifdef OPENGLES
    int yy;
#endif

    glBindTexture(GL_TEXTURE_2D,tex);
    checkGLStatus( __FILE__, __LINE__ );
#ifndef OPENGLES
    glPixelStorei(GL_UNPACK_ROW_LENGTH,screenbufferwidth);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS,x);
    glPixelStorei(GL_UNPACK_SKIP_ROWS,y);
#endif // !OPENGLES
    glPixelStorei(GL_UNPACK_ALIGNMENT,1);
    checkGLStatus( __FILE__, __LINE__ );

    if (create) {
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,partialfilter);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,partialfilter);
    }

    //glPixelTransferi(GL_MAP_COLOR,GL_TRUE);

    if (debugmode)
    TO_DEBUG_LOG("Partial overlay upload (%d %d %d %d)... ",
        w,h,dx,dy);


    if (create) {
    if (debugmode)
        TO_DEBUG_LOG("(create) ");
#ifdef OPENGLES
    glTexImage2D(GL_TEXTURE_2D,0,colourformat,w,
                 h,0,colourformat,
                 GL_UNSIGNED_BYTE,
                 NULL);
#else
    glTexImage2D(GL_TEXTURE_2D,0,colourformat,w,
             h,0,GL_RGBA,
             GL_UNSIGNED_BYTE,
             screenbuffer32);
#endif // OPENGLES
    }
#ifndef OPENGLES
    else {
    glTexSubImage2D(GL_TEXTURE_2D,0,dx,dy,w,h,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            screenbuffer32);
    }
#endif // !OPENGLES

#ifdef OPENGLES
    for( yy = 0; yy < h; yy++ ) {
        char *row = (char*)screenbuffer32 + ((yy + y)*screenbufferwidth + x) * 4;
        glTexSubImage2D(GL_TEXTURE_2D, 0, dx, dy+yy, w, 1, GL_RGBA, GL_UNSIGNED_BYTE, row);
    }
#endif

    //checkGLStatus( __FILE__, __LINE__ );
    if (debugmode)
    TO_DEBUG_LOG("done.\n");
    //glPixelTransferi(GL_MAP_COLOR,GL_FALSE);
#ifndef OPENGLES
    glPixelStorei(GL_UNPACK_SKIP_PIXELS,0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS,0);
    glPixelStorei(GL_UNPACK_ROW_LENGTH,0);
#endif // !OPENGLES
}

void ConvertPartialOverlay(int sx, int sy, int w, int h) {


    unsigned char *f=&screenbuffer[sx+screenbufferwidth*sy],*t=(unsigned char*)&screenbuffer32[sx+screenbufferwidth*sy];

    int x,y;
    int skip=screenbufferwidth-w;

    //printf("sx=%d, sy=%d, w=%d, h=%d, spos=%d skip=%d\n",sx,sy,w,h,sx+screenbufferwidth*sy,skip);
    for(y=0;y<h;y++,f+=skip,t+=(skip*4))
    for(x=0;x<w;x++) {
        if ((*(f))==255 && ingame) {
        *(t++)=0;
        *(t++)=0;
        *(t++)=0;
        *(t++)=0;
        } else {
        *(t++)=ipalr[*f];
        *(t++)=ipalg[*f];
        *(t++)=ipalb[*f];
        *(t++)=255;
        }
        f++;
    }
    //TextureAvg32((Uint32*)to,64,64);

}

/* Upload rectangular part of overlay from memory to overlay texture... */

void UploadPartialOverlay(int x,int y,int w,int h) {
    int left,right,top,bottom,i,j;
    int lr,rr,tr,br;

    ConvertPartialOverlay(x,y,w,h);
    if (menuing) return;

    if (largescreentexture) {
    /* On my nVidia Riva TNT, uploading 1 pixel high subimages is very slow
       (driver bug?), so I upload an extra row. Very odd. Probably a driver
       issue (nVidia driver version 1.0-1541 on Linux 2.4.4-4GB).

       This only seems to affect the large textures. Very odd. */

    UploadPartialOverlayToTexture(x,y,x,y,w,(h>1)?h:2,
                      screenbuffertexture,0);
    } else {
    left=(x-2)/62;
    if (left<0) left=0;
    right=(x+w-1)/62;
    if (right>5) right=5;
    top=(y-2)/62;
    if (top<0) top=0;
    bottom=(y+h-1)/62;
    if (bottom>11) bottom=11;

    for(i=top;i<=bottom;i++)
        for(j=left;j<=right;j++) {
        lr=x-62*j;
        rr=lr+w-1;
        tr=y-62*i;
        br=tr+h-1;

        if (rr<0) continue;
        if (lr>63) continue;
        if (br<0) continue;
        if (tr>63) continue;

        if (lr<0) lr=0;
        if (rr>63) rr=63;
        if (tr<0) tr=0;
        if (br>63) br=63;

        UploadPartialOverlayToTexture(lr+62*j,tr+62*i,lr,tr,rr-lr+1,
                          br-tr+1,
                          screenbuffertextures[i*6+j],0);
        }
    }
    ShowPartialOverlay(x-1,y-1,w+2,h+2,0);
}

/* Upload entire overlay from memory to texture (creates textures)... */

void UploadOverlay(void) {
    int i,j;
    static int c=1;

    settransferpalette();
    if (largescreentexture)
    UploadPartialOverlayToTexture(0,0,0,0,screenbufferwidth,
                      screenbufferheight,
                      screenbuffertexture,c);
    else {
    for(i=0;i<12;i++)
        for(j=0;j<6;j++)
        UploadPartialOverlayToTexture(62*j,62*i,0,0,64,64,
                          screenbuffertextures[i*6+j],c);
    }
    c=0;
}

/* Display rectangular part of overlay... */

void ShowPartialOverlay(int x,int y,int w,int h,int statusbar) {

    float tx1,tx2,ty1,ty2;
    int i,j,tr,br,lr,rr,left,right,top,bottom;

    float vl,vt1,vt2;

    if (statusbar==0) {
    y-=visiblescreenyoffset;
    if (x+w>360) w=360-x;
    if (y+h>240) h=240-y;
    if (x<0) {w+=x; x=0;}
    if (y<0) {h+=y; y=0;}
    if ((w<=0)||(h<=0)) return;
    y+=visiblescreenyoffset;
    }

    if (mixing)
    glEnable(GL_BLEND);
    else {
    glAlphaFunc(GL_GEQUAL,0.99);
    glEnable(GL_ALPHA_TEST);
    }
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    vl=floor(-((float)virtualscreenwidth-360.0)/2.0);
    vt1=floor(240.0+(virtualscreenheight-240.0)/2.0);
    vt2=floor(statusbaryoffset+statusbaryvisible+statusbaryoffset-y);

    if (statusbar==1)
    gluOrtho2D(vl,
           vl+virtualscreenwidth,
           vt2,
           vt2-virtualscreenheight);
    else if (statusbar==2) {
    gluOrtho2D(vl+340.0-x,
           vl+virtualscreenwidth+340.0-x,
           vt2,
           vt2-virtualscreenheight);
    x=340; y=statusbaryoffset;
    }
    else
    gluOrtho2D(vl,
           vl+virtualscreenwidth,
           vt1,
           vt1-virtualscreenheight);

//    gluOrtho2D(0.0, 360.0, 0.0, 240.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    if (largescreentexture) {
    tx1=((float)x)/(float)(screenbufferwidth);
    tx2=((float)(x+w))/(float)screenbufferwidth;

    ty1=((float)y)/(float)(screenbufferheight);
    ty2=((float)(y+h))/(float)screenbufferheight;

    y-=visiblescreenyoffset;

    glBindTexture(GL_TEXTURE_2D,screenbuffertexture);
#ifndef OPENGLES
    glBegin(GL_QUADS);
    glColor3f(redfactor,greenfactor,bluefactor);
    glTexCoord2f(tx1,ty2);
    glVertex2s(x,y+h);
    glTexCoord2f(tx2,ty2);
    glVertex2s(x+w,y+h);
    glTexCoord2f(tx2,ty1);
    glVertex2s(x+w,y);
    glTexCoord2f(tx1,ty1);
    glVertex2s(x,y);
    glEnd();
#else
    GLfloat vtx[] = {
        x   ,y+h,
        x+w ,y+h,
        x+w ,y  ,
        x   ,y
    };

    GLfloat tex[] = {
        tx1,ty2,
        tx2,ty2,
        tx2,ty1,
        tx1,ty1
    };

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    glColor4f(redfactor,greenfactor,bluefactor,1.0f);

    glVertexPointer(2, GL_FLOAT, 0, vtx);
    glTexCoordPointer(2, GL_FLOAT, 0, tex);
    glDrawArrays( GL_TRIANGLE_FAN, 0, 4 );

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
#endif // !OPENGLES
    } else {
    left=(x-1)/62;
    if (left<0) left=0;
    right=(x+w-2)/62;
    if (right>5) right=5;
    top=(y-1)/62;
    if (top<0) top=0;
    bottom=(y+h-2)/62;
    if (bottom>11) bottom=11;

//	printf("Drawing %d %d %d %d\n",x,y,w,h);

    for(i=top;i<=bottom;i++)
        for(j=left;j<=right;j++) {
        lr=x-62*j;
        rr=lr+w-1;
        tr=y-62*i;
        br=tr+h-1;

        if (rr<(j>0)) continue;
        if (lr>(63-(j<5))) continue;
        if (br<(i>0)) continue;
        if (tr>(63-(i<11))) continue;

        if (lr<(j>0)) lr=(j>0);
        if (rr>(63-(j<5))) rr=63-(j<5);
        if (tr<(i>0)) tr=(i>0);
        if (br>(63-(i<11))) br=63-(i<11);

        tx1=((float)lr)/64.0;
        tx2=((float)(rr+1))/64.0;

        ty1=((float)tr)/64.0;
        ty2=((float)(br+1))/64.0;

        if (debugmode) {
            TO_DEBUG_LOG("Partial overlay display... ");
            TO_DEBUG_LOG("%d %d %d %d %d %d... ",i,j,lr,tr,rr,br);
        }

        glBindTexture(GL_TEXTURE_2D,screenbuffertextures[i*6+j]);
#ifndef OPENGLES
        glBegin(GL_QUADS);
        glColor3f(redfactor,greenfactor,bluefactor);
        glTexCoord2f(tx1,ty2);
        glVertex2s(lr+62*j,br+1+62*i-visiblescreenyoffset);
        glTexCoord2f(tx2,ty2);
        glVertex2s(rr+1+62*j,br+1+62*i-visiblescreenyoffset);
        glTexCoord2f(tx2,ty1);
        glVertex2s(rr+1+62*j,tr+62*i-visiblescreenyoffset);
        glTexCoord2f(tx1,ty1);
        glVertex2s(lr+62*j,tr+62*i-visiblescreenyoffset);
        glEnd();
#else
        GLfloat vtx[] = {
            lr+62*j,br+1+62*i-visiblescreenyoffset,
            rr+1+62*j,br+1+62*i-visiblescreenyoffset,
            rr+1+62*j,tr+62*i-visiblescreenyoffset,
            lr+62*j,tr+62*i-visiblescreenyoffset
        };

        GLfloat tex[] = {
            tx1,ty2,
            tx2,ty2,
            tx2,ty1,
            tx1,ty1
        };

        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);

        glColor4f(redfactor,greenfactor,bluefactor,1.0f);

        glVertexPointer(2, GL_FLOAT, 0, vtx);
        glTexCoordPointer(2, GL_FLOAT, 0, tex);
        glDrawArrays( GL_TRIANGLE_FAN, 0, 4 );

        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
#endif // !OPENGLES
        if (debugmode)
            TO_DEBUG_LOG("done.\n");

        }
    }
    if (mixing)
    glDisable(GL_BLEND);
    else
    glDisable(GL_ALPHA_TEST);
    checkGLStatus( __FILE__, __LINE__ );

    if (statusbar==1) {
    for(i=0;i<(virtualscreenwidth-319)/2;i+=20) {
        ShowPartialOverlay(340+i,statusbaryoffset,20,statusbaryvisible,2);
        ShowPartialOverlay(0-i,statusbaryoffset,20,statusbaryvisible,2);
    }
    }

}

/* Draw status bar if necessary. */

void ShowStatusBar() {
//    if (statusbaryoffset>=240) return;
    mixing=1;
    ShowPartialOverlay(20,statusbaryoffset,320,statusbaryvisible,1);
    mixing=0;
}

/* Redraw overlay as if screen offset were offset bytes... */

void SetVisibleScreenOffset(K_UINT16 offset) {

    float y=offset/90;

    glClearColor(0,0,0,0);
    glClear( GL_COLOR_BUFFER_BIT);

    visiblescreenyoffset=y;

    ShowPartialOverlay(0,0+y,360,240,0);
}

void setgamevideomode()
{
    fade(63);
    videotype=0;
}

/* Draw text string in textbuf at (x,y) with colour offset coloffs... */

void textprint(K_INT16 x,K_INT16 y,char coloffs)
{
    unsigned char character;
    K_INT16 charcnt, walnume;

    if ((vidmode == 1) && (y>=statusbaryoffset))
    x += 20;

    charcnt = 0;
    while ((textbuf[charcnt] != 0) && (charcnt < 40))
    {
    character = (textbuf[charcnt])&127;
    if (lab3dversion)
        walnume = (character>>6)+125;
    else
        walnume = (character>>6)+textwall;

    drawtooverlay((character&7)<<3,((character&63)>>3)<<3,
              8,8,x,y,walnume-1,coloffs);
    x+=8;
    charcnt++;
    }
}

/* Load and display some text (story, help, whatever). */

K_INT16 loadstory(K_INT16 boardnume)
{
    unsigned char xordat, otextcol, textcol;
    K_UINT16 storyoffs[128];
    K_INT16 fil, i, textbufcnt, textypos;

    ototclock = totalclock;
    char path1[256];
    char path2[256];
    snprintf(path1, sizeof(path1), "%s/story.kzp", globalDataDir);
    snprintf(path2, sizeof(path2), "%s/STORY.KZP", globalDataDir);
#ifdef ANDROID_NDK
    SDL_RWops *io = SDL_RWFromFile(path2, "rb");
    if (!io) {
        io = SDL_RWFromFile(path1, "rb");
    }
    if (!io)
#else
    if (((fil = open(path1,O_RDONLY|O_BINARY,0)) == -1)&&
    ((fil = open(path2,O_RDONLY|O_BINARY,0)) == -1))
#endif // ANDROID_NDK
    return(-1);
#ifndef ANDROID_NDK
    readLE16(fil,&storyoffs[0],256);
    lseek(fil,(long)(storyoffs[boardnume+34]),SEEK_SET);
    read(fil,&tempbuf[0],4096);
#else
    SDL_RWread(io, &storyoffs[0],256, 1);
    SDL_RWseek(io, (long)(storyoffs[boardnume+34]),SEEK_SET);
    SDL_RWread(io, &tempbuf[0],4096, 1);
#endif // !ANDROID_NDK
    i = 0;
    xordat = 0;
    if (vidmode == 0)
    textypos = 3;
    else
    textypos = 23;
    if ((boardnume >= 0) || ((boardnume >= -21) && (boardnume <= -16)))
    otextcol = 0;
    else
    otextcol = 96;
    textcol = otextcol;
    if ((boardnume >= -14) && (boardnume <= -2))
    textypos += 12;
    textbufcnt = 0;
    while ((tempbuf[i] != 0) && (textbufcnt < 40))
    {
    xordat ^= tempbuf[i];
    if (tempbuf[i] >= 32)
    {
        if ((otextcol == 96) && (textbufcnt == 0))
        {
        textcol = 96;
        if (tempbuf[i] == '!')
            textcol = 16, textbuf[textbufcnt++] = '@';
        else if (tempbuf[i] == '%')
            textcol = 80, textbuf[textbufcnt++] = '@';
        else
            textbuf[textbufcnt++] = tempbuf[i];
        }
        else
        textbuf[textbufcnt++] = tempbuf[i];
    }
    if (tempbuf[i] == 13)
    {
        textbuf[textbufcnt] = 0;
        textprint(180-(textbufcnt<<2),textypos+1,textcol);
        textypos += 12;
        textbufcnt = 0;
    }
    i++;
    tempbuf[i] ^= xordat;
    }
    textbuf[textbufcnt] = 0;
    textprint(180-(textbufcnt<<2),textypos+1,textcol);
#ifndef ANDROID_NDK
    close(fil);
#else
    SDL_FreeRW(io);
#endif // ANDROID_NDK
    return 0;
}

K_INT16 setupmouse()
{
    return(0); /* Assume mouse always exists. */
}

/* Redraw status bar... */

void statusbaralldraw()
{
    K_INT16 i, j;

    for(i=0;i<361;i++)
    screenbuffer[i+240*screenbufferwidth]=0x50;
    UploadPartialOverlay(0,240,361,1);
    statusbardraw(0,0,32,32,0,0+statusbaryoffset,statusbarback);
    statusbardraw(32,0,32,32,288,0+statusbaryoffset,statusbarback);
    for(i=32;i<288;i+=32)
    statusbardraw(16,0,32,32,i,0+statusbaryoffset,statusbarback);
    textbuf[0] = 'S', textbuf[1] = 'C', textbuf[2] = 'O';
    textbuf[3] = 'R', textbuf[4] = 'E', textbuf[5] = 0;
    textprint(3,4+statusbaryoffset,(char)176);
    textbuf[0] = 'T', textbuf[1] = 'I', textbuf[2] = 'M';
    textbuf[3] = 'E', textbuf[4] = 0;
    textprint(3,12+statusbaryoffset,(char)176);
    textbuf[0] = 'B', textbuf[1] = 'O', textbuf[2] = 'A';
    textbuf[3] = 'R', textbuf[4] = 'D', textbuf[5] = 0;
    textprint(3,20+statusbaryoffset,(char)176);
    textbuf[0] = ':', textbuf[1] = 0;
    textprint(41,4+statusbaryoffset,(char)176);
    textprint(33,12+statusbaryoffset,(char)176);
    textprint(41,20+statusbaryoffset,(char)176);
    textbuf[0] = 'L', textbuf[1] = 'I', textbuf[2] = 'F';
    textbuf[3] = 'E', textbuf[4] = 0;
    textprint(96,4+statusbaryoffset,(char)176);
    textbuf[0] = 'W', textbuf[1] = 'E', textbuf[2] = 'A';
    textbuf[3] = 'P', textbuf[4] = 'O', textbuf[5] = 'N';
    textbuf[6] = 0;
    textprint(272,3+statusbaryoffset,(char)176);
    if (hiscorenamstat == 1)
    {
    for(i=0;i<9;i++)
        textbuf[i] = hiscorenam[i];
    textbuf[9] = 0;
    textprint(70,20+statusbaryoffset,(char)176);
    }
    statusbardraw(50,30,7,7,282,12+statusbaryoffset,statusbarinfo);
    statusbardraw(50,37,7,7,282,21+statusbaryoffset,statusbarinfo);
    statusbardraw(57,30,7,7,306,12+statusbaryoffset,statusbarinfo);
    statusbardraw(57,37,7,7,306,21+statusbaryoffset,statusbarinfo);
    statusbardraw(36,37,7,7,104,12+statusbaryoffset,statusbarinfo);
    statusbardraw(43,37,7,7,136,12+statusbaryoffset,statusbarinfo);
    statusbardraw(32,28,1,8,94,4+statusbaryoffset,statusbarinfo);
    statusbardraw(32,28,1,8,94,12+statusbaryoffset,statusbarinfo);
    oldlife = 0;
    drawlife();
    oldlife = 4095;
    drawlife();
    drawscore(scorecount);
    drawtime(scoreclock);
    textbuf[0] = ((boardnum+1)/10)+48;
    textbuf[1] = ((boardnum+1)%10)+48;
    textbuf[2] = 0;
    if (textbuf[0] == 48)
    textbuf[0] = 32;
    textprint(46,20+statusbaryoffset,(char)176);
    if (vidmode == 1)
    {
    statusbardraw(0,32,20,32,-20,0+statusbaryoffset,statusbarback);
    statusbardraw(0,32,20,32,320,0+statusbaryoffset,statusbarback);
    }
    textbuf[0] = 9, textbuf[1] = 0;
    textprint(96,12+statusbaryoffset,(char)0);
    textbuf[0] = lifevests+48, textbuf[1] = 0;
    textprint(96,12+statusbaryoffset,(char)176);
    textbuf[0] = 9, textbuf[1] = 0;
    textprint(296,21+statusbaryoffset,(char)0);
    textbuf[0] = lightnings+48, textbuf[1] = 0;
    textprint(296,21+statusbaryoffset,(char)176);
    textbuf[0] = 9, textbuf[1] = 0;
    textprint(272,12+statusbaryoffset,(char)0);
    textbuf[0] = firepowers[0]+48, textbuf[1] = 0;
    textprint(272,12+statusbaryoffset,(char)176);
    textbuf[0] = 9, textbuf[1] = 0;
    textprint(272,21+statusbaryoffset,(char)0);
    textbuf[0] = firepowers[1]+48, textbuf[1] = 0;
    textprint(272,21+statusbaryoffset,(char)176);
    textbuf[0] = 9, textbuf[1] = 0;
    textprint(296,12+statusbaryoffset,(char)0);
    textbuf[0] = firepowers[2]+48, textbuf[1] = 0;
    textprint(296,12+statusbaryoffset,(char)176);
    if (purpletime >= totalclock)
    {
    statusbardraw(0,0,16,15,159,13+statusbaryoffset,statusbarinfo);
    if (purpletime < totalclock+3840)
    {
        i = ((3840-(int)(purpletime-totalclock))>>8);
        if ((i >= 0) && (i <= 15))
        statusbardraw(0,30,16,i,159,13+statusbaryoffset,statusbarinfo);
    }
    }
    if (greentime >= totalclock)
    {
    statusbardraw(0,15,16,15,176,13+statusbaryoffset,statusbarinfo);
    if (greentime < totalclock+3840)
    {
        i = ((3840-(int)(greentime-totalclock))>>8);
        if ((i >= 0) && (i <= 15))
        statusbardraw(16,30,16,i,176,13+statusbaryoffset,statusbarinfo);
    }
    }
    if (capetime[0] >= totalclock)
    {
    statusbardraw(16,0,21,28,194,2+statusbaryoffset,statusbarinfo);
    if (capetime[0] < totalclock+3072)
    {
        i = (int)((capetime[0]-totalclock)>>9);
        if ((i >= 0) && (i <= 5))
        {
        if (i == 5) statusbardraw(0,0,21,28,194,2+statusbaryoffset,coatfade);
        if (i == 4) statusbardraw(21,0,21,28,194,2+statusbaryoffset,coatfade);
        if (i == 3) statusbardraw(42,0,21,28,194,2+statusbaryoffset,coatfade);
        if (i == 2) statusbardraw(0,32,21,28,194,2+statusbaryoffset,coatfade);
        if (i == 1) statusbardraw(21,32,21,28,194,2+statusbaryoffset,coatfade);
        if (i == 0) statusbardraw(42,32,21,28,194,2+statusbaryoffset,coatfade);
        }
    }
    }
    if (capetime[1] >= ototclock)
    {
    statusbardraw(37,0,21,28,216,2+statusbaryoffset,statusbarinfo);
    if (capetime[1] < totalclock+3072)
    {
        i = (int)((capetime[1]-totalclock)>>9);
        if ((i >= 0) && (i <= 5))
        {
        if (i == 5) statusbardraw(0,0,21,28,216,2+statusbaryoffset,coatfade);
        if (i == 4) statusbardraw(21,0,21,28,216,2+statusbaryoffset,coatfade);
        if (i == 3) statusbardraw(42,0,21,28,216,2+statusbaryoffset,coatfade);
        if (i == 2) statusbardraw(0,32,21,28,216,2+statusbaryoffset,coatfade);
        if (i == 1) statusbardraw(21,32,21,28,216,2+statusbaryoffset,coatfade);
        if (i == 0) statusbardraw(42,32,21,28,216,2+statusbaryoffset,coatfade);
        }
    }
    }
    if (keys[0] > 0)
    statusbardraw(36,44,14,6,144,13+statusbaryoffset,statusbarinfo);
    if (keys[1] > 0)
    statusbardraw(50,44,14,6,144,21+statusbaryoffset,statusbarinfo);
    statusbardraw(41,28,8,9,292,20+statusbaryoffset,statusbarinfo);
    statusbardraw(43,28,6,9,300,20+statusbaryoffset,statusbarinfo);
    statusbardraw(43,28,6,9,306,20+statusbaryoffset,statusbarinfo);
    statusbardraw(45,28,5,9,312,20+statusbaryoffset,statusbarinfo);
    /*if (bulchoose == 0) */
    i = 268, j = 11;
    if (bulchoose == 1) i = 268, j = 20;
    if (bulchoose == 2) i = 292, j = 11;
    statusbardraw(32,28,8,9,i,j+statusbaryoffset,statusbarinfo);
    statusbardraw(34,28,6,9,i+8,j+statusbaryoffset,statusbarinfo);
    statusbardraw(34,28,6,9,i+14,j+statusbaryoffset,statusbarinfo);
    statusbardraw(36,28,5,9,i+20,j+statusbaryoffset,statusbarinfo);
    textbuf[0] = 9, textbuf[1] = 9;
    textbuf[2] = 9, textbuf[3] = 0;
    textprint(112,12+statusbaryoffset,(char)0);
    textbuf[0] = (coins/100)+48;
    textbuf[1] = ((coins/10)%10)+48;
    textbuf[2] = (coins%10)+48;
    textbuf[3] = 0;
    if (textbuf[0] == 48)
    {
    textbuf[0] = 32;
    if (textbuf[1] == 48)
        textbuf[1] = 32;
    }
    textprint(112,12+statusbaryoffset,(char)176);
}

/* Draw the hiscore box to the overlay... */

void drawscorebox() {
    int x,y;
    float ox=0.0,oy=0.0,od;
    unsigned char *orgl;
    unsigned char *org;

    unsigned char *dest;

    od=64.0/320.0;

    ox=0.0;
    for(x=0;(x<320)&&(ox<64.0);x++) {
    if (lab3dversion)
        orgl=walseg[75]+(((int)ox)<<6);
    else
        orgl=walseg[scorebox-1]+(((int)ox)<<6);
    oy=0.0;
    dest=screenbuffer+(18+x)+screenbufferwidth*20;

    for(y=0;(y<200)&&(oy<64.0);y++) {
        org=orgl+((int)oy);
        if ((*org)!=255)
        *dest=*org;

        dest+=screenbufferwidth;
        oy+=od;
    }
    ox+=od;
    }

    menuleft=18; menutop=20; menuwidth=320; menuheight=200;
    menuing=1;

#ifdef OPENGLES
    drawOnScreen();
#endif
}

/* Hiscore sequence; check for new hiscore and display hiscore box. */

void hiscorecheck()
{
    K_INT16 i, j, k, m, inse, namexist, fil;
    K_INT32 hiscore[8], scorexist, templong;

    char path1[256];
    char path2[256];
#ifndef ANDROID_NDK
    snprintf(path1, sizeof(path1), "%s/hiscore.dat", globalDataDir);
    snprintf(path2, sizeof(path2), "%s/HISCORE.DAT", globalDataDir);
#else
    snprintf(path1, sizeof(path1), "%s/hiscore.dat", globalAndroidRWdir);
    snprintf(path2, sizeof(path2), "%s/HISCORE.DAT", globalAndroidRWdir);
#endif // ANDROID_NDK

    if (((fil = open(path1,O_RDWR|O_BINARY,0)) == -1)&&
    ((fil = open(path2,O_RDWR|O_BINARY,0)) == -1)) {
        TO_DEBUG_LOG("Warning, highscores disabled.");
        return;
    }
#ifndef OPENGLES
    glDrawBuffer(GL_FRONT);
#endif //!OPENGLES
    wipeoverlay(0,0,361,statusbaryoffset);
    lseek(fil,(long)(boardnum<<7),SEEK_SET);
    read(fil,&tempbuf[0],128);
    for(i=0;i<8;i++)
    {
//	hiscore[i]=*((K_INT32 *)(&tempbuf[i*16+12]));
    hiscore[i]=readlong(&tempbuf[i*16+12]);
    }
    /*
      for(i=lside;i<rside;i++)
      height[i] = 0;
    */

    /* Score box looks awful when interpolated, and we need it in the
       overlay for proper anti-aliasing, so software scale. */

    drawscorebox();
/*    spridraw(20,0,320*4,scorebox);*/
    for(i=0;i<8;i++)
    if (tempbuf[i<<4] != 0)
    {
        textbuf[0] = i+49, textbuf[1] = '.', textbuf[2] = 32, textbuf[3] = 32;
        for(j=0;j<12;j++)
        textbuf[j+4] = tempbuf[(i<<4)+j];
        textbuf[16] = 0;
        if (lab3dversion) {
        if (strcmp(textbuf+4, BADNAME)==0)
            continue;
        textprint(55,60+(i<<3)+i+1,(char)128);
        } else
        textprint(55,60+(i<<3)+i+1,(char)130);
        setuptextbuf(hiscore[i]);
        if (lab3dversion)
        textprint(215,60+(i<<3)+i+1,(char)0);
        else
        textprint(215,60+(i<<3)+i+1,(char)96);
    }
    sprintf(&textbuf[0],"Time penalty: 10 * ");
    templong = (scoreclock*5)/12;
    textbuf[19] = (char)((templong/10000000L)%10L)+48;
    textbuf[20] = (char)((templong/1000000L)%10L)+48;
    textbuf[21] = (char)((templong/100000L)%10L)+48;
    textbuf[22] = (char)((templong/10000L)%10L)+48;
    textbuf[23] = (char)((templong/1000L)%10L)+48;
    textbuf[24] = (char)((templong/100L)%10L)+48;
    textbuf[25] = '.';
    textbuf[26] = (char)((templong/10L)%10L)+48;
    textbuf[27] = (char)(templong%10L)+48;
    textbuf[28] = 0;
    j = 19;
    while ((textbuf[j] == 48) && (j < 24))
    j++;
    for(i=19;i<28;i++)
    textbuf[i] = textbuf[i+j-19];
    if (lab3dversion)
    textprint(180-(strlen(textbuf)<<2),155+1,(char)106);
    else
    textprint(180-(strlen(textbuf)<<2),155+1,(char)114);
    scorecount -= (scoreclock/24);
    if (scorecount < 0)
    scorecount = 0;
    setuptextbuf(scorecount);
    if (lab3dversion)
    textprint(215,145+1,(char)33);
    else
    textprint(215,145+1,(char)35);
    if ((scorecount > hiscore[7]) && (cheated == 0))
    {
    if (hiscorenamstat == 0) {
        getname();
    } else
        finalisemenu();

    if (hiscorenamstat == 0)
    {
        for(i=0;i<22;i++)
        textbuf[i] = 8;
        textbuf[22] = 0;
        textprint(180-(strlen(textbuf)<<2),135+1,(char)1);
    }
    else
    {
        m = 0;
        while ((hiscorenam[m] != 0) && (m < 11))
        m++;
        namexist = -1;
        for(i=0;i<8;i++)
        {
        j = 0;
        for(k=0;k<=m;k++)
            if (hiscorenam[k] != tempbuf[(i<<4)+k])
            j = 1;
        if (j == 0)
            namexist = i;
        }
        scorexist = scorecount;
        if (namexist != -1)
        {
        if (hiscore[namexist] > scorexist)
            scorexist = hiscore[namexist];
        for(j=namexist;j<7;j++)
        {
            for(k=0;k<16;k++)
            tempbuf[(j<<4)+k]=tempbuf[((j+1)<<4)+k];
            hiscore[j] = hiscore[j+1];
        }
        inse = 6;
        }
        else
        inse = 7;
        while ((scorexist > hiscore[inse]) && (inse >= 0))
        inse--;
        inse++;
        for(j=7;j>inse;j--)
        {
        for(k=0;k<16;k++)
            tempbuf[(j<<4)+k]=tempbuf[((j-1)<<4)+k];
        hiscore[j] = hiscore[j-1];
        }
        for(k=0;k<12;k++)
        tempbuf[(inse<<4)+k] = hiscorenam[k];
        hiscore[inse] = scorexist;
        writelong(&tempbuf[inse*16+12],scorexist);

        wipeoverlay(0,0,361,statusbaryoffset);
        drawscorebox();
        if (scorecount == scorexist)
        {
        switch(inse)
        {
            case 0: strcpy(&textbuf[0],"Congratulations!!! You're #1!"); break;
            case 1: strcpy(&textbuf[0],"Good Job!! Second place!"); break;
            case 2: strcpy(&textbuf[0],"Not Bad - You got third place!"); break;
            case 3: strcpy(&textbuf[0],"Fourth place! Getting there..."); break;
            case 4: strcpy(&textbuf[0],"Fifth place! Could be improved..."); break;
            case 5: strcpy(&textbuf[0],"Sixth Place...  Keep trying..."); break;
            case 6: strcpy(&textbuf[0],"Seventh? Really want your name here?"); break;
            case 7: strcpy(&textbuf[0],"Eighth? Have a load in your pants?"); break;
        };
        }
        else
        strcpy(&textbuf[0],"Are those reflexes dying?");
        textprint(180-(strlen(textbuf)<<2),45+1,lab3dversion?64:(char)65);
        for(i=0;i<8;i++)
        if (tempbuf[i<<4] != 0)
        {
            textbuf[0] = i+49, textbuf[1] = '.', textbuf[2] = 32, textbuf[3] = 32;
            for(j=0;j<12;j++)
            textbuf[j+4] = tempbuf[(i<<4)+j];
            textbuf[16] = 0;
            if (lab3dversion)
            if (strcmp(textbuf+4, BADNAME)==0)
                continue;
            if (i == inse)
            {
            textprint(55,60+(i<<3)+i+1,lab3dversion?0:(char)98);
            for(j=0;j<12;j++)
                textbuf[j] = textbuf[j+4];
            textprint(87,145+1,lab3dversion?96:(char)98);
            }
            else
            textprint(55,60+(i<<3)+i+1,lab3dversion?128:(char)130);
            setuptextbuf(hiscore[i]);
            textprint(215,60+(i<<3)+i+1,lab3dversion?0:(char)96);
        }
        sprintf(&textbuf[0],"Time penalty: 10 * ");
        textbuf[19] = (char)((templong/10000000L)%10L)+48;
        textbuf[20] = (char)((templong/1000000L)%10L)+48;
        textbuf[21] = (char)((templong/100000L)%10L)+48;
        textbuf[22] = (char)((templong/10000L)%10L)+48;
        textbuf[23] = (char)((templong/1000L)%10L)+48;
        textbuf[24] = (char)((templong/100L)%10L)+48;
        textbuf[25] = '.';
        textbuf[26] = (char)((templong/10L)%10L)+48;
        textbuf[27] = (char)(templong%10L)+48;
        textbuf[28] = 0;
        j = 19;
        while ((textbuf[j] == 48) && (j < 24))
        j++;
        for(i=19;i<28;i++)
        textbuf[i] = textbuf[i+j-19];
        textprint(180-(strlen(textbuf)<<2),155+1,lab3dversion?106:114);
        setuptextbuf(scorecount);
        textprint(215,145+1,lab3dversion?33:(char)35);
        lseek(fil,(long)(boardnum<<7),SEEK_SET);
        write(fil,&tempbuf[0],128);
    }
    }
    else
    {
    i = rand()&3;
    if (cheated == 0)
    {
        switch(i)
        {
        case 0: strcpy(&textbuf[0],"You rot if you can't beat ");
            j = 0;
            while (tempbuf[(i<<4)+j] != 0)
            {
            textbuf[j+26] = tempbuf[(i<<4)+j];
            j++;
            }
            textbuf[j+26] = 0;
            break;
        case 1: strcpy(&textbuf[0],"Try this level again if you're good."); break;
        case 2: strcpy(&textbuf[0],"Nice job, but no score like Ken's."); break;
        case 3: strcpy(&textbuf[0],"Try playing for speed next time."); break;
        };
    }
    else
    {
        switch(i)
        {
        case 0: strcpy(&textbuf[0],"Try playing for real next time."); break;
        case 1: strcpy(&textbuf[0],"Don't touch those cheat keys!"); break;
        case 2: strcpy(&textbuf[0],"Cheating doesn't pay."); break;
        case 3: strcpy(&textbuf[0],"You can't cheat a score."); break;
        };
    }
    textprint(180-(strlen(textbuf)<<2),45+1,(char)161);
    }
    close(fil);
    sprintf(&textbuf[0],"Press any key to continue.");
    textprint(180-(strlen(textbuf)<<2),135+1,(char)65);
    finalisemenu();
    glFlush();
#ifdef OPENGLES
    drawOnScreen();
#endif
#ifndef OPENGLES
    glDrawBuffer(GL_BACK);
#endif // !OPENGLES
    while ((keystatus[1] == 0) && (keystatus[57] == 0) && (keystatus[28] == 0) && (bstatus == 0)) {
    PollInputs();
    SDL_Delay(10);
    }
}

/* Convert integer to string. */

void setuptextbuf(K_INT32 templong)
{
    int i;

    textbuf[0] = (char)+48;
    textbuf[1] = (char)+48;
    textbuf[2] = (char)+48;
    textbuf[3] = (char)((templong/10000000L)%10L)+48;
    textbuf[4] = (char)((templong/1000000L)%10L)+48;
    textbuf[5] = (char)((templong/100000L)%10L)+48;
    textbuf[6] = (char)((templong/10000L)%10L)+48;
    textbuf[7] = (char)((templong/1000L)%10L)+48;
    textbuf[8] = (char)((templong/100L)%10L)+48;
    textbuf[9] = (char)((templong/10L)%10L)+48;
    textbuf[10] = (char)(templong%10L)+48;
    i = 0;
    while ((textbuf[i] == 48) && (i < 10))
    textbuf[i++] = 32;
    textbuf[11] = 0;
}

/* Get player's name... */

void getname()
{
    unsigned char ch;
    K_INT16 i, j;

    /* Apparently, the program is supposed to remember the name you used last
       time you played and allow you to modify that.

       In practice, this code is so buggy and useless that I think it's best to
       just leave it out. */

/*    if (namrememberstat == 0)
      {*/
    for(j=0;j<16;j++)
    hiscorenam[j] = 0;
    for(j=0;j<12;j++)
    textbuf[j] = 8;
    textbuf[12] = 0;
    textprint(94,145+1,(char)0);
    j = 0;
/*    }
      else
      {
      for(j=0;j<12;j++)
      textbuf[j] = hiscorenam[j];
      textbuf[12] = 0;
      j = 12;
      while ((hiscorenam[j] == 0) && (j > 0))
      j--;
      textprint(94,125+1,(char)97);
      }*/

    ch = 0;
    finalisemenu();
#ifndef OPENGLES
    glDrawBuffer(GL_FRONT);
#endif // !OPENGLES
    sprintf(&textbuf[0],"Please type your name!");

    textprint(180-(strlen(textbuf)<<2),135+1,(char)161);
    ch = 0;
#ifndef USE_SDL2
    SDL_EnableUNICODE(1);
#endif // !USE_SDL2

#ifdef ANDROID_NDK
    strncpy( hiscorenam, "APlayer", sizeof(textbuf)-1 );
    j = strlen( hiscorenam );
#else
    while ((ch != 13) && (ch != 27))
    {
#ifdef OPENGLES
        drawOnScreen();
#endif // !OPENGLES
    while ((ch=getkeypress()) == 0)
    {
        textbuf[0] = 95;
        textbuf[1] = 0;
        textprint(94+(j<<3),145,(char)97);
        glFlush();
        SDL_Delay(10); /* Just to avoid soaking all CPU. */
        textbuf[0] = 8;
        textbuf[1] = 0;
        textprint(94+(j<<3),145,(char)0);
        glFlush();
        SDL_Delay(10); /* Just to avoid soaking all CPU. */
    }
#ifndef USE_SDL2
    if (ch == SDLK_DELETE)
#else
    if (ch == getOldAsciiKeyCode(SDLK_DELETE))
#endif // !USE_SDL2
    {
        hiscorenam[j] = ch;
        for(j=0;j<16;j++)
        hiscorenam[j] = 0;
        for(j=0;j<12;j++)
        textbuf[j] = 8;
        textbuf[12] = 0;
        textprint(94,145+1,(char)0);
        j = 0;
        ch = 0;
    }
    if ((ch == 8) && (j > 0))
    {
        j--, hiscorenam[j] = 0;
        textbuf[0] = ch;
        textbuf[1] = 0;
        textprint(94+(j<<3),145+1,(char)0);
    }
    if ((ch >= 32) && (ch <= 127) && (j < 12))
    {
        textbuf[0] = ch;
        textbuf[1] = 0;
        textprint(94+(j<<3),145+1,(char)97);
        hiscorenam[j] = ch;
        if ((ch != 32) || (j > 0))
        j++;
    }
    }
#endif // ANDROID_NDK
#ifndef USE_SDL2
    SDL_EnableUNICODE(0);
#endif // !USE_SDL2
    for(i=0;i<256;i++)
    keystatus[i] = 0;
    hiscorenam[j] = 0;
    i = j-1;
    while ((hiscorenam[i] == 32) && (i >= 0))
    hiscorenam[i--] = 0;
    if ((hiscorenam[0] == 0) || (ch == 27))
    {
    hiscorenamstat = 0;
    hiscorenam[0] = 0;
    }
    else
    {
    if (hiscorenam[0] == 0)
        hiscorenamstat = 0;
    else
    {
        hiscorenamstat = 1;
        for(i=0;i<9;i++)
        textbuf[i] = hiscorenam[i];
        textbuf[9] = 0;
        if (lab3dversion==0)
        textprint(70,20+statusbaryoffset,(char)177);
    }
    }

#ifdef OPENGLES
    drawOnScreen();
#endif
}

/* Draw score... */

void drawscore(K_INT32 thescore)
{
    K_INT16 i;

    for(i=0;i<6;i++)
    textbuf[i] = 9;
    textbuf[6] = 0;
    textprint(46,4+statusbaryoffset,(char)0);
    textbuf[0] = (char)((thescore/100000L)%10L)+48;
    textbuf[1] = (char)((thescore/10000L)%10L)+48;
    textbuf[2] = (char)((thescore/1000L)%10L)+48;
    textbuf[3] = (char)((thescore/100L)%10L)+48;
    textbuf[4] = (char)((thescore/10L)%10L)+48;
    textbuf[5] = (char)(thescore%10L)+48;
    textbuf[6] = 0;
    i = 0;
    while ((textbuf[i] == 48) && (i < 5))
    textbuf[i++] = 32;
    textprint(46,4+statusbaryoffset,(char)192);
}

/* Draw time... */

void drawtime(K_INT32 thetime)
{
    K_INT16 i;

    for(i=0;i<7;i++)
    textbuf[i] = 9;
    textbuf[7] = 0;
    textprint(38,12+statusbaryoffset,(char)0);
    thetime = thetime/240;
    textbuf[0] = (char)((thetime/1000000L)%10L)+48;
    textbuf[1] = (char)((thetime/100000L)%10L)+48;
    textbuf[2] = (char)((thetime/10000L)%10L)+48;
    textbuf[3] = (char)((thetime/1000L)%10L)+48;
    textbuf[4] = (char)((thetime/100L)%10L)+48;
    textbuf[5] = (char)((thetime/10L)%10L)+48;
    textbuf[6] = (char)(thetime%10L)+48;
    textbuf[7] = 0;
    i = 0;
    while ((textbuf[i] == 48) && (i < 6))
    textbuf[i++] = 32;
    textprint(38,12+statusbaryoffset,(char)192);
}

/* Precalculated BMP header; RGB, 24-bit. */

unsigned char BMPHeader[54]={0x42,0x4d,0,0,0,0,0,0,
                 0,0,0x36,0,0,0,0x28,0,
                 0,0,0,0,0,0,0,0,
                 0,0,1,0,0x18,0,0,0,
                 0,0,0,0,0,0,0,0,
                 0,0,0,0,0,0,0,0,
                 0,0,0,0,0,0};

/* Write a DWORD (32-bit int) to BMP header. */

void bmpheaderwrite(int offset, K_UINT32 value) {
    int a;

    for(a=0;a<4;a++)
    BMPHeader[offset+a]=(value>>(a<<3))&255;
}

/* Save screen to capturxx.bmp, where xx counts up from 00. */

void screencapture()
{
    char filename[15];

    unsigned char *screen=malloc(3*screenwidth*screenheight);

    char success=0;
    int file;

    K_UINT32 size=54+3*screenwidth*screenheight;

    strcpy(filename,"capturxx.bmp");
    filename[6] = (capturecount/10)+48;
    filename[7] = (capturecount%10)+48;
    capturecount++;
    if (capturecount==100)
    capturecount=0; /* Just to keep the file names sane. */

    bmpheaderwrite(2,size);

    bmpheaderwrite(0x12,screenwidth);
    bmpheaderwrite(0x16,screenheight);

    if (screen!=NULL) {
    glReadPixels(0,0,screenwidth,screenheight,GL_BGR,GL_UNSIGNED_BYTE,
             screen);
    unlink(filename);
    file=open(filename,O_CREAT|O_WRONLY|O_BINARY,
          S_IREAD|S_IWRITE|S_IRGRP|S_IROTH);
    if (file!=-1) {
        if (write(file,BMPHeader,54)==54) {
        if (write(file,screen,size-54)==size-54) {
            if (close(file)!=-1)
            success=1;
        }
        }
    }
    free(screen);
    }

    if (success)
    ksay(7);
    else
    ksay(8);
}

/* Draw main menu. Separated to eliminate need for saving screen contents. */

void drawmainmenu() {
    K_INT16 n;

    if (vidmode == 0)
    n = 0;
    else
    n = 20;

    drawmenu(192,128,menu);
    strcpy(&textbuf[0],"New game");
    textprint(131,47+n+1,32);
    strcpy(&textbuf[0],"Load game");
    textprint(131,59+n+1,32);
    strcpy(&textbuf[0],"Save game");
    textprint(131,71+n+1,32);
    strcpy(&textbuf[0],"Return to game");
    textprint(131,83+n+1,32);
    strcpy(&textbuf[0],"Help");
    textprint(131,95+n+1,126);
    strcpy(&textbuf[0],"Story");
    textprint(131,107+n+1,126);
    strcpy(&textbuf[0],"Copyright notice");
    textprint(131,119+n+1,126);
    strcpy(&textbuf[0],"Credits");
    textprint(131,131+n+1,126);
    strcpy(&textbuf[0],"Exit");
    textprint(131,143+n+1,126);
    finalisemenu();
}

/* Main menu... */

K_INT16 mainmenu()
{
    K_INT16 j, k, done;

    spriteyoffset=0;

#ifdef OPENGLES
    currentMenuState = eNoMenu; // First run.
#endif // OPENGLES

    ksay(27);
    fade(63);
    if (sortcnt == -1) {
    /* Emulating the original a bit too closely, this... */
#ifndef OPENGLES
    glDrawBuffer(GL_BACK);
#endif // !OPENGLES
    spriteyoffset=20;
    drawintroduction();
    spriteyoffset=0;
    drawOnScreen();
    } else {
#ifndef OPENGLES
    glDrawBuffer(GL_FRONT);
#endif // !OPENGLES
    ShowStatusBar();
    }
#ifndef OPENGLES
    glDrawBuffer(GL_FRONT);
#endif // !OPENGLES
    done = 0;
    if (sortcnt != -1)
    {
    wipeoverlay(0,0,361,statusbaryoffset);
    }
    j = scrsize;
    if (j < 18000)
    j = 18000;
    if (vidmode == 1)
    j = 21600;

    drawmainmenu();

    while ((mainmenuplace >= 0) && (done == 0))
    {
    if ((mainmenuplace = getselection(88,47,mainmenuplace,9)) >= 0)
    {
        if (mainmenuplace == 0)
        if ((k = newgamemenu()) >= 0)
        {
            done = 1;
            if ((k == 1) && (numboards < 20))
            {
            ksay(12);
            done = 0;
            }
            else if ((k == 2) && (numboards < 30))
            {
            ksay(12);
            done = 0;
            }
        }
        if (mainmenuplace == 1)
        if (loadsavegamemenu(1) >= 0)
            done = 1;
        if (mainmenuplace == 2)
        {
        if (sortcnt == -1)
            ksay(12);
        else
        {
            if (loadsavegamemenu(2) >= 0)
            done = 1;
        }
        }
        if (mainmenuplace == 3)
        mainmenuplace = (-mainmenuplace)-1;
        if (mainmenuplace == 4) helpmenu();
        if (mainmenuplace == 5) bigstorymenu();
        if (mainmenuplace == 6) orderinfomenu();
        if (mainmenuplace == 7) creditsmenu();
        if (mainmenuplace == 8) done = areyousure();
        if (done == 0)
        {
#ifndef OPENGLES
        /* Redraw whatever was beneath the menu. Double buffer to
           avoid annoying flicker. */
        if (sortcnt == -1) {
            spriteyoffset=20;
            glDrawBuffer(GL_BACK);
            checkGLStatus( __FILE__, __LINE__ );
            kgif(1);
            drawintroduction();
            spriteyoffset=0;
            drawmainmenu();
            checkGLStatus( __FILE__, __LINE__ );
            drawOnScreen();
            glDrawBuffer(GL_FRONT);
        }
        else {
            glDrawBuffer(GL_BACK);
            checkGLStatus( __FILE__, __LINE__ );
            wipeoverlay(0,0,361,statusbaryoffset);
            statusbaralldraw();
            if (compass>0) showcompass(ang);
            picrot(posx,posy,posz,ang);
            drawmainmenu();
            checkGLStatus( __FILE__, __LINE__ );
            drawOnScreen();
            glDrawBuffer(GL_FRONT);
        }
#endif // OPENGLES
        }
    }
    }
    if (sortcnt != -1) {
    linecompare(statusbar);
    wipeoverlay(0,0,361,statusbaryoffset);
    }
    else
    kgif(1);
    j = mainmenuplace;
    if (mainmenuplace < 0)
    mainmenuplace = (-mainmenuplace)-1;
#ifndef OPENGLES
    glDrawBuffer(GL_BACK);
#endif // !OPENGLES
    return(j);

#ifdef OPENGLES
    currentMenuState = eNoMenu;
#endif // OPENGLES
}

/* Get a selection from a menu with totselectors choices, defaulting to
   nowselector, at screen position (xoffs,yoffs). */

K_INT16 getselection(K_INT16 xoffs, K_INT16 yoffs, K_INT16 nowselector,
             K_INT16 totselectors)
{
    K_INT16 animater6, n, esckeystate;
    int mousx, mousy;
    K_INT16 bstatus, obstatus;
#ifndef OPENGLES
    glDrawBuffer(GL_FRONT);
#endif // !OPENGLES
    if (vidmode == 0)
    n = 0;
    else
    n = 20;
    keystatus[0x48] = 0, keystatus[0xc8] = 0, keystatus[0xcb] = 0;
    keystatus[0xd0] = 0, keystatus[0x50] = 0, keystatus[0xcd] = 0;
    keystatus[0x01] = 0, keystatus[0x1c] = 0, keystatus[0x9c] = 0;
    keystatus[0x39] = 0;
#ifndef USE_SDL2
    newkeystatus[SDLK_KP_ENTER]=0;
    newkeystatus[SDLK_RETURN]=0;
    newkeystatus[SDLK_SPACE]=0;
    newkeystatus[SDLK_ESCAPE]=0;
    newkeystatus[SDLK_UP]=newkeystatus[SDLK_DOWN]=newkeystatus[SDLK_LEFT]=
    newkeystatus[SDLK_RIGHT]=0;
    newkeystatus[SDLK_KP2]=newkeystatus[SDLK_KP8]=0;
#else
    newkeystatus[getOldAsciiKeyCode(SDLK_KP_ENTER)]=0;
    newkeystatus[getOldAsciiKeyCode(SDLK_RETURN)]=0;
    newkeystatus[getOldAsciiKeyCode(SDLK_SPACE)]=0;
    newkeystatus[getOldAsciiKeyCode(SDLK_ESCAPE)]=0;
    newkeystatus[getOldAsciiKeyCode(SDLK_UP)]=newkeystatus[getOldAsciiKeyCode(SDLK_DOWN)]=newkeystatus[getOldAsciiKeyCode(SDLK_LEFT)]=
    newkeystatus[getOldAsciiKeyCode(SDLK_RIGHT)]=0;
    newkeystatus[getOldAsciiKeyCode(SDLK_KP_2)]=newkeystatus[getOldAsciiKeyCode(SDLK_KP_8)]=0;
#endif // !USE_SDL2
    animater6 = 0;
    esckeystate = 0;
    bstatus = 1;
    obstatus = 1;
    mousx = 0;
    mousy = 0;
#ifdef OPENGLES
    switch (currentMenuState) {
    // Wipe Screen after all Menus.
    default: {
        if (sortcnt == -1) {
            wipeoverlay(0,0,361,260);
        } else {
            wipeoverlay(0,0,361,statusbaryoffset);
        }
        break;
    }
    case eNoMenu: {
        if (sortcnt == -1) {
            wipeoverlay(0,0,361,260);
        }
        currentMenuState = eMainMenu;
        break;
    }
    }

    // Now Render necessary menus
    switch (currentMenuState) {
    case eMainMenu:
        drawmainmenu();
        break;
    case eAbortMenu:
        o_drawAbortMenuHelper();
        break;
    case eNewGameMenu:
        o_drawNewGameMenuHelper();
        break;
    case eHardnessMenu:
        o_drawHardnessMenuHelper();
        break;
    case eLoadSaveGameMenu:
        o_drawLoadSaveGameMenuHelper();
        break;
    case eSodaMenu:
        o_drawSodaMenuHelper();
        break;
    case eSettingsMenu:
        o_drawSettingsMenuHelper();
        break;
    case eInputDevicesMenu:
        fillSettingMenuHelper(4, inputdevicemenu);
        break;
    case eSetupConfigureMenu:
        o_drawSetupConfigureMenuHelper();
        break;
    case eSetupKeys:
        o_drawSetupKeysHelper();
        break;
    case eSetupJButtons:
        o_drawSetupJButtonsHelper();
        break;
    case eSetupJAxes:
        o_drawSetupJAxesHelper();
        break;
    case eSetupFiltering:
        fillSettingMenuHelper(3, filtermenu);
        break;
    case eSetupMusic:
        fillSettingMenuHelper(2, musicmenu);
        break;
    case eSetupSound:
        fillSettingMenuHelper(2, soundmenu);
        break;
    case eSetupSoundCh:
        fillSettingMenuHelper(2, channelmenu);
        break;
    case eSetupMusicCh:
        fillSettingMenuHelper(2, channelmenu);
        break;
    case eSetupCheats:
        fillSettingMenuHelper(3, cheatmenu);
        break;
    case eSetupSoundBlock:
        fillSettingMenuHelper(10, soundblockmenu);
        break;
    case eSetupScaling:
        fillSettingMenuHelper(4, scalingtypemenu);
        break;
    default:
        break;
    }

    // Enter to Draw Loop
#endif // OPENGLES
    while (esckeystate == 0)
    {
    PollInputs();
    animater6++;
    if (animater6 == 6)
        animater6 = 0;

    SDL_Delay(10); /* Let's not soak up all CPU... */

    if (lab3dversion) {
        statusbardraw(16+(animater6/2)*16,0,15,15,xoffs+20-n,nowselector*12+yoffs+n-1,85);
    } else {
        if (animater6 < 3)
        statusbardraw(20+animater6*14,32,13,13,xoffs+20-n,nowselector*12+yoffs+n-1+1,statusbarback);
        else
        statusbardraw(20+(animater6-3)*14,46,13,13,xoffs+20-n,nowselector*12+yoffs+n-1+1,statusbarback);
    }
    obstatus = bstatus;
    if (moustat == 0) {
        bstatus=readmouse(&mousx,&mousy);
    }
    if (((keystatus[0x48]|keystatus[0xc8]|keystatus[0xcb]) != 0) || (mousy < -128))
    {
        if (mousy < -128)
        mousy += 128;
        if (lab3dversion)
        wipeoverlay(xoffs+39-n,nowselector*12+yoffs+n-1,15,15);
        else
        statusbardraw(16,15,13,13,xoffs+20-n,nowselector*12+yoffs+n-1+1,menu);
        nowselector--;
        ksay(27);
        if (nowselector < 0)
        nowselector = totselectors-1;
        keystatus[0x48] = 0, keystatus[0xc8] = 0, keystatus[0xcb] = 0;
#ifndef USE_SDL2
        newkeystatus[SDLK_UP]=newkeystatus[SDLK_KP8]=
        newkeystatus[SDLK_LEFT]=0;
#else
        newkeystatus[getOldAsciiKeyCode(SDLK_UP)]=newkeystatus[getOldAsciiKeyCode(SDLK_KP_8)]=
        newkeystatus[getOldAsciiKeyCode(SDLK_LEFT)]=0;
#endif // !USE_SDL2
    }
    if (((keystatus[0xd0]|keystatus[0x50]|keystatus[0xcd]) != 0) || (mousy > 128))
    {
        if (mousy > 128)
        mousy -= 128;
        if (lab3dversion)
        wipeoverlay(xoffs+39-n,nowselector*12+yoffs+n-1,15,15);
        else
        statusbardraw(16,15,13,13,xoffs+20-n,nowselector*12+yoffs+n-1+1,menu);
        nowselector++;
        ksay(27);
        if (nowselector == totselectors)
        nowselector = 0;
        keystatus[0xd0] = 0, keystatus[0x50] = 0, keystatus[0xcd] = 0;
#ifndef USE_SDL2
        newkeystatus[SDLK_DOWN]=newkeystatus[SDLK_KP2]=
        newkeystatus[SDLK_RIGHT]=0;
#else
        newkeystatus[getOldAsciiKeyCode(SDLK_DOWN)]=newkeystatus[getOldAsciiKeyCode(SDLK_KP_2)]=
        newkeystatus[getOldAsciiKeyCode(SDLK_RIGHT)]=0;
#endif // !USE_SDL2
    }
    esckeystate = (keystatus[1]|(keystatus[0x1c]<<1)|(keystatus[0x9c]<<1)|(keystatus[0x39]<<1));
    if ((obstatus == 0) && (bstatus > 0))
        esckeystate |= (bstatus^3);
    glFlush();

#ifdef OPENGLES
    // Draw All on Texture
    ShowPartialOverlay(-1, -1, 363, 262, 0);
    drawOnScreen();
#endif // OPENGLES
    }
    ksay(27);
    if (lab3dversion)
    wipeoverlay(xoffs+39-n,nowselector*12+yoffs+n-1,15,15);
    else
    statusbardraw(36-n,15,13,13,xoffs+20-n,nowselector*12+yoffs+n-1+1,menu);
    if ((esckeystate&2) > 0)
    return(nowselector);
    else
    return((-nowselector)-1);
}

#ifdef OPENGLES
void fillSettingMenuHelper(int count, char titles[][30]) {
    int i;
    int j=12*count+24;
    drawmenu(304,j,menu);
    for(i=0;i<count;i++) {
    strcpy(textbuf,titles[i]);
    textprint(71,120-6*count+12*i,lab3dversion?32:34);
    }
    finalisemenu();
}

void o_drawAbortMenuHelper() {
    K_INT16 n;
    if (vidmode == 0)
    n = 0;
    else
    n = 20;
    drawmenu(224,64,menu);
    strcpy(&textbuf[0],"Really want to quit?");
    textprint(99,84+n+1,112);
    strcpy(&textbuf[0],"Yes");
    textprint(105,96+n+1,32);
    strcpy(&textbuf[0],"No");
    textprint(105,108+n+1,32);
    finalisemenu();
}

void o_drawNewGameMenuHelper() {
    K_INT16 j, n;
    if (vidmode == 0)
    n = 0;
    else
    n = 20;
    drawmenu(288,64,menu);
    strcpy(&textbuf[0],"New game");
    textprint(137,74+n+1,112);
    strcpy(&textbuf[0],"Episode 1: Search for Sparky");
    textprint(67,88+n+1,32);
    if (numboards >= 20) j = 32; else j = 28;
    strcpy(&textbuf[0],"Episode 2: Sparky's Revenge");
    textprint(67,100+n+1,((char)j));
    if (numboards >= 30) j = 32; else j = 28;
    strcpy(&textbuf[0],"Episode 3: Find the Way Home");
    textprint(67,112+n+1,((char)j));
    if (newgameplace < 0) newgameplace = 0;
    if (newgameplace > 2) newgameplace = 2;
    finalisemenu();
}


void o_drawHardnessMenuHelper() {
    K_INT16 n;
    if (vidmode == 0)
    n = 0;
    else
    n = 20;
    drawmenu(288,64,menu);
    strcpy(&textbuf[0],"New game");
    textprint(137,74+n+1,112);
    strcpy(&textbuf[0],"Easy: Don't touch me.");
    textprint(67,92+n+1,32);
    strcpy(&textbuf[0],"Hard: OUCH!");
    textprint(67,104+n+1,32);
    finalisemenu();
}

void o_drawLoadSaveGameMenuHelper() {
    char filename[20];
    K_INT16 fil, i, j, k, n;
    K_INT32 templong;
    if (vidmode == 0)
    n = 0;
    else
    n = 20;
    drawmenu(320,160,menu);
    if (qStateSaveOrLoad == 1)
    {
    strcpy(&textbuf[0],"Load game");
    textprint(137,26+n+1,32);
    }
    else
    {
    strcpy(&textbuf[0],"Save game");
    textprint(137,26+n+1,112);
    }
    strcpy(&textbuf[0],"#: Name:       Board: Score: Time:");
    textprint(55,52+n+1,48);
    if (gameheadstat == 0)
    {
    for(j=0;j<8;j++)
    {
        char path[256];
        filename[0] = 'S', filename[1] = 'A', filename[2] = 'V';
        filename[3] = 'G', filename[4] = 'A', filename[5] = 'M';
        filename[6] = 'E', filename[7] = j+48;
        filename[8] = '.', filename[9] = 'D', filename[10] = 'A';
        filename[11] = 'T', filename[12] = 0;
#ifndef ANDROID_NDK
        snprintf(path, sizeof(path), "%s/%s", globalDataDir, filename);
#else
        snprintf(path, sizeof(path), "%s/%s", globalAndroidRWdir, filename);
#endif // ANDROID_NDK
        if((fil=open(path,O_RDONLY|O_BINARY,0))!=-1)
        {
        gamexist[j] = 1;
        read(fil,&gamehead[j][0],27);
        close(fil);
        }
        else {
        filename[0] = 's', filename[1] = 'a', filename[2] = 'v';
        filename[3] = 'g', filename[4] = 'a', filename[5] = 'm';
        filename[6] = 'e', filename[7] = j+48;
        filename[8] = '.', filename[9] = 'd', filename[10] = 'a';
        filename[11] = 't', filename[12] = 0;

#ifndef ANDROID_NDK
        snprintf(path, sizeof(path), "%s/%s", globalDataDir, filename);
#else
        snprintf(path, sizeof(path), "%s/%s", globalAndroidRWdir, filename);
#endif // ANDROID_NDK
        if((fil=open(path,O_RDONLY|O_BINARY,0))!=-1)
        {
            gamexist[j] = 1;
            read(fil,&gamehead[j][0],27);
            close(fil);
        }
        else
            gamexist[j] = 0;
        }
    }
    gameheadstat = 1;
    }
    j = 0;
    for(i=70+n;i<166+n;i+=12)
    {
    if (gamexist[j] == 1)
    {
        textbuf[0] = j+49, textbuf[1] = 32, textbuf[2] = 32;
        for(k=0;k<12;k++)
        {
        textbuf[k+3] = gamehead[j][k];
        if (textbuf[k+3] == 0)
            textbuf[k+3] = 32;
        }
        textbuf[15] = 32;
        textbuf[16] = ((gamehead[j][17]+1)/10)+48;
        if (textbuf[16] == 48)
        textbuf[16] = 32;
        textbuf[17] = ((gamehead[j][17]+1)%10)+48;
        textbuf[18] = 32;
        textbuf[19] = 32;
        textbuf[20] = 32;
        k = j*27;
        templong=readlong((unsigned char *)&gamehead[j][19]);

        textbuf[21] = (char)((templong/100000L)%10L)+48;
        textbuf[22] = (char)((templong/10000L)%10L)+48;
        textbuf[23] = (char)((templong/1000L)%10L)+48;
        textbuf[24] = (char)((templong/100L)%10L)+48;
        textbuf[25] = (char)((templong/10L)%10L)+48;
        textbuf[26] = (char)(templong%10L)+48;
        textbuf[27] = 32;
        k = 21;
        while ((textbuf[k] == 48) && (k < 26))
        textbuf[k++] = 32;
        k = j*27;
        templong=readlong((unsigned char *)&gamehead[j][23]);

        templong /= 240;
        textbuf[28] = (char)((templong/10000L)%10L)+48;
        textbuf[29] = (char)((templong/1000L)%10L)+48;
        textbuf[30] = (char)((templong/100L)%10L)+48;
        textbuf[31] = (char)((templong/10L)%10L)+48;
        textbuf[32] = (char)(templong%10L)+48;
        textbuf[33] = 0;
        k = 28;
        while ((textbuf[k] == 48) && (k < 32))
        textbuf[k++] = 32;
        textprint(56,i-1+1,30);
        textprint(55,i-1+1,32);
    }
    else
    {
        textbuf[0] = j+49;
        textbuf[1] = 0;
        textprint(56,i-1+1,28);
        textprint(55,i-1+1,30);
    }
    j++;
    }
    finalisemenu();
}

void o_drawSodaMenuHelper() {
    K_INT16 n;
    if (vidmode == 0)
    n = 0;
    else
    n = 20;
    drawmenu(256,160,menu);
    if (boardnum < 10)
    loadstory(-34);
    else
    loadstory(-33);
    statusbardraw(0,0,12,36,85-n,49+n+1,sodapics);
    statusbardraw(12,0,12,36,85-n,85+n+1,sodapics);
    statusbardraw(24,0,12,36,85-n,121+n+1,sodapics);
    statusbardraw(36,0,12,12,85-n,157+n+1,sodapics);
    finalisemenu();
}

void o_drawSettingsMenuHelper() {
    drawmenu(360,240,menu);
    int offs = 31;
    strcpy(textbuf,"Ken's Labyrinth (LAB3D/SDL2) setup menu");
    textprint(offs,22,126);
    strcpy(textbuf,"Input: ");
    strcat(textbuf,inputdevicemenu[inputdevice]);
    textprint(51,36,lab3dversion?32:34);
    strcpy(textbuf,"Configure Input");
    textprint(51,48,lab3dversion?32:34);
    strcpy(textbuf,"Resolution: ");
    if (resolutionnumber<8)
        strcat(textbuf,resolutionstandardmenu[resolutionnumber]);
    else if (resolutionnumber<19)
        strcat(textbuf,resolutionspecialmenu[resolutionnumber-8]);
    else sprintf(textbuf,"Resolution: %dx%d",resolutionnumber/10000,
             resolutionnumber%10000);
    textprint(51,60,64);
        strcpy(textbuf,"Display type: ");
    strcat(textbuf,fullscreenmenu[fullscr]);
    textprint(51,72,64);
        strcpy(textbuf,"Filtering: ");
    strcat(textbuf,filtermenu[nearest]);
    textprint(51,84,64);
        strcpy(textbuf,"Music: ");
    strcat(textbuf,musicmenu[music]);
    textprint(51,96,96);
        strcpy(textbuf,"Effects: ");
    strcat(textbuf,soundmenu[sound]);
    textprint(51,108,96);
        strcpy(textbuf,"Sound channels: ");
    strcat(textbuf,channelmenu[channel]);
    textprint(51,120,96);
        strcpy(textbuf,"Music channels: ");
    strcat(textbuf,channelmenu[musicchannel]);
    textprint(51,132,96);
        strcpy(textbuf,"Cheats: ");
    strcat(textbuf,cheatmenu[cheat]);
    textprint(51,144,96);
        strcpy(textbuf,"Sound block size: ");
    strcat(textbuf,soundblockmenu[soundblock]);
    textprint(51,156,lab3dversion?32:34);
        strcpy(textbuf,"Texture colour depth: ");
    strcat(textbuf,texturedepthmenu[texturedepth]);
    textprint(51,168,lab3dversion?32:34);
        strcpy(textbuf,"View: ");
    strcat(textbuf,scalingtypemenu[scaling]);
    textprint(51,180,lab3dversion?32:34);
        strcpy(textbuf,"Exit setup");
    textprint(51,192,lab3dversion?128:130);
    strcpy(textbuf,"Use cursor keys and Return to select.");
    textprint(31,220,lab3dversion?32:34);
    finalisemenu();
}

void o_drawSetupConfigureMenuHelper() {
    int i;
    int j=12*4+24;
    drawmenu(304,j,menu);
    for(i=0;i<4;i++) {
    strcpy(textbuf,configureinputmenu[i]);
    textprint(71,120-6*4+12*i,lab3dversion?32:34);
    }
    finalisemenu();
}

void o_drawSetupKeysHelper() {
    int j;
    drawmenu(360,240,menu);
    for(j=0;j<numkeys;j++) {
        strcpy(textbuf,keynames[j]);
        textprint(31,13+12*j,lab3dversion?32:34);
        strncpy(textbuf,SDL_GetKeyName(newkeydefs[j]),11);
        textbuf[11]=0;
        textprint(261,13+12*j,lab3dversion?32:34);
    }
    finalisemenu();
}

void o_drawSetupJButtonsHelper() {
    int j, cb;
    drawmenu(360,240,menu);
    for(j=0;j<numkeys;j++) {
        strcpy(textbuf,keynames[j]);
        textprint(31,13+12*j,lab3dversion?32:34);
        cb=buttondefs[j];
        if (cb==-1) {
        strcpy(textbuf,"None");
        } else {
        sprintf(textbuf,"Button %d",cb+1);
        }
        textbuf[11]=0;
        textprint(261,13+12*j,lab3dversion?32:34);
    }
    finalisemenu();
}

void o_drawSetupJAxesHelper() {
    int j, cb;
    drawmenu(360,240,menu);
    for(j=0;j<numaxes;j++) {
        strcpy(textbuf,axisnames[j]);
        textprint(31,13+12*j,lab3dversion?32:34);
        cb=axisdefs[j];
        if (cb==0) {
        strcpy(textbuf,"None");
        } else {
        strncpy(textbuf,jaxisnames[abs(cb)-1],11);
        }
        textbuf[11]=0;
        if (cb < 0)
        strcat(textbuf," INV");
        textbuf[11]=0;
        textprint(261,13+12*j,lab3dversion?32:34);
    }
    finalisemenu();
}
#endif // OPENGLES

void finalisemenu() {
    menuing=0;
    UploadPartialOverlay(menuleft,menutop,menuwidth,menuheight);
}

/* Draw menu background and frame... */

void drawmenu(K_INT16 xsiz, K_INT16 ysiz, K_INT16 walnume)
{
    K_INT16 ycent, i, j, x1, y1, x2, y2;
    unsigned char *buf;

    if (vidmode == 0)
    ycent = 100;
    else
    ycent = 120;

    if (vidmode == 0)
    x1 = 180-(xsiz>>1);
    else
    x1 = 160-(xsiz>>1);
    y1 = ycent-(ysiz>>1)-1;
    x2 = x1+xsiz-16;
    y2 = y1+ysiz-16;

    menuleft=x1+20;
    menutop=y1+1+visiblescreenyoffset+spriteyoffset;
    menuwidth=xsiz;
    menuheight=ysiz;
    menuing=1;

    if (ysiz>240) {menutop=0; menuheight=240;}

    if (lab3dversion) {
    wipeoverlay(menuleft, menutop, menuwidth, menuheight);
    buf=screenbuffer+(screenbufferwidth*menutop+(menuleft));
    for(i=0;i<menuwidth;i++)
        *(buf++)=239;
    buf=screenbuffer+(screenbufferwidth*(menutop+menuheight-1)+menuleft);
    for(i=0;i<menuwidth;i++)
        *(buf++)=239;
    buf=screenbuffer+(screenbufferwidth*menutop+(menuleft));
    for(i=0;i<menuwidth;i++)
        *(buf+=screenbufferwidth)=239;
    buf=screenbuffer+(screenbufferwidth*menutop+(menuleft+menuwidth-1));
    for(i=0;i<menuwidth;i++)
        *(buf+=screenbufferwidth)=239;
    UploadPartialOverlay(menuleft,menutop,menuwidth,menuheight);

    return;
    }

    if (ysiz<=240) {
    statusbardraw(0,0,16,16,x1,y1+1,walnume);
    statusbardraw(48,0,16,16,x2,y1+1,walnume);
    statusbardraw(0,48,16,16,x1,y2+1,walnume);
    statusbardraw(48,48,16,16,x2,y2+1,walnume);
    for(i=x1+16;i<x2;i+=16)
    {
        statusbardraw(16,0,16,16,i,y1+1,walnume);
        statusbardraw(16,48,16,16,i,y2+1,walnume);
    }
    } else {
    y1=-17; y2=239;
    }

    for(j=y1+16;j<y2;j+=16)
    {
    statusbardraw(0,16,16,16,x1,j+1,walnume);
    statusbardraw(48,16,16,16,x2,j+1,walnume);
    }
    for(i=x1+16;i<x2;i+=16)
    for(j=y1+16;j<y2;j+=16)
        statusbardraw(16,16,16,16,i,j+1,walnume);
}

/* Show credits. */

void creditsmenu()
{
    K_INT16 n;
    if (vidmode == 0)
    n = 0;
    else
    n = 20;
    drawmenu(320,176,menu);
    strcpy(&textbuf[0],"Credits");
    textprint(149,20+n+1,32);
    loadstory(-1);
    finalisemenu();
#ifdef OPENGLES
    drawOnScreen();
#endif // !OPENGLES
    pressakey();
}

/* Show story for episode... */

void bigstorymenu()
{
    K_INT16 i, j, k, n, nowenterstate, lastenterstate, quitstat, bstatus, obstatus;

    if (vidmode == 0)
    n = 0;
    else
    n = 20;
    if (boardnum < 10) j = -32, k = -27;
    else if (boardnum < 20) j = -26, k = -24;
    else j = -23, k = -22;
    quitstat = 0;
    bstatus = 1;
    obstatus = 1;
    i = j;
    while (quitstat == 0)
    {
    drawmenu(304,192,menu);
    loadstory(i);
    finalisemenu();
    nowenterstate = 1;
    lastenterstate = 1;
    glFlush();
#ifdef OPENGLES
    drawOnScreen();
#endif // !OPENGLES
    while ((nowenterstate <= lastenterstate) && (bstatus <= obstatus))
    {
        PollInputs();
        lastenterstate = nowenterstate;
        nowenterstate = keystatus[0x1c];
        nowenterstate |= keystatus[0x9c];
        nowenterstate |= keystatus[1];
        nowenterstate |= getkeydefstat(17);
        nowenterstate |= keystatus[0xc9];
        nowenterstate |= keystatus[0xc8];
        nowenterstate |= keystatus[0xcb];
        nowenterstate |= keystatus[0xd1];
        nowenterstate |= keystatus[0xd0];
        nowenterstate |= keystatus[0xcd];
        obstatus = bstatus;
        if (moustat == 0) {
        bstatus=readmouse(NULL,NULL);
        }
    }
    if (((keystatus[0xc9]|keystatus[0xc8]|keystatus[0xcb]) > 0) && (i > j))
        i--;
    if (((keystatus[0xd1]|keystatus[0xd0]|keystatus[0xcd]) > 0) && (i < k))
        i++;
    quitstat = (getkeydefstat(17)|keystatus[1]);
    if (((keystatus[0x1c]|keystatus[0x9c]) > 0) || (bstatus > obstatus))
    {
        bstatus = 1;
        obstatus = 1;
        i++;
        if (i > k)
        quitstat = 1;
    }
    ksay(27);
    }
}

/* Quit (Y/N)? */

K_INT16 areyousure()
{

    K_INT16 i, n;
#ifndef OPENGLES
    if (vidmode == 0)
    n = 0;
    else
    n = 20;
    drawmenu(224,64,menu);
    strcpy(&textbuf[0],"Really want to quit?");
    textprint(99,84+n+1,112);
    strcpy(&textbuf[0],"Yes");
    textprint(105,96+n+1,32);
    strcpy(&textbuf[0],"No");
    textprint(105,108+n+1,32);
    finalisemenu();
#else
    currentMenuState = eAbortMenu;
#endif // OPENGLES
    i = getselection(60,95,0,2);
#ifdef OPENGLES
    currentMenuState = eMainMenu;
#endif // OPENGLES
    if (i == 0)
    return(1);
    else
    return(0);
}

/* Help screen. */

void helpmenu()
{
    K_INT16 n;

    if (vidmode == 0)
    n = 0;
    else
    n = 20;
    drawmenu(256,176,menu);
    loadstory(-15);
    strcpy(&textbuf[0],"Help");
    textprint(161,18+n+1,32);
    finalisemenu();
#ifdef OPENGLES
    drawOnScreen();
#endif // !OPENGLES
    pressakey();
}

/* Soda menu. */

void sodamenu()
{
    K_INT32 ototclocker;
    K_INT16 n, valid;
#ifndef OPENGLES
    wipeoverlay(0,0,361,statusbaryoffset);
#ifndef OPENGLES
    glDrawBuffer(GL_FRONT);
#endif // !OPENGLES
    if (vidmode == 0)
    n = 0;
    else
    n = 20;
    ksay(27);
    drawmenu(256,160,menu);
    if (boardnum < 10)
    loadstory(-34);
    else
    loadstory(-33);
    statusbardraw(0,0,12,36,85-n,49+n+1,sodapics);
    statusbardraw(12,0,12,36,85-n,85+n+1,sodapics);
    statusbardraw(24,0,12,36,85-n,121+n+1,sodapics);
    statusbardraw(36,0,12,12,85-n,157+n+1,sodapics);
    finalisemenu();
#else
    currentMenuState = eSodaMenu;
#endif // !OPENGLES
    ksay(27);
    ototclocker = totalclock;
    valid = 0;
    while (valid == 0)
    {
    sodaplace = getselection(46,49,sodaplace,10);
    if (sodaplace >= 0)
    {
        valid = 1;
        if ((sodaplace == 0) && (coins < 1)) valid = 0;
        if ((sodaplace == 1) && (coins < 2)) valid = 0;
        if ((sodaplace == 2) && (coins < 2)) valid = 0;
        if ((sodaplace == 3) && (coins < 5)) valid = 0;
        if ((sodaplace == 4) && ((coins < 5) || (boardnum < 10))) valid = 0;
        if ((sodaplace == 5) && (coins < 75)) valid = 0;
        if ((sodaplace == 6) && (coins < 100)) valid = 0;
        if ((sodaplace == 7) && (coins < 150)) valid = 0;
        if ((sodaplace == 8) && ((coins < 200) || (boardnum < 10))) valid = 0;
        if ((sodaplace == 9) && (coins < 250)) valid = 0;
        if (valid == 0)
        ksay(12);
    }
    else
        valid = 1;
    }
    if ((sodaplace >= 0) && (valid == 1))
    {
    ksay(24);
    switch(sodaplace)
    {
        case 0:
        coins--;
        life += 320;
        if (life > 4095)
            life = 4095;
        drawlife();
        break;
        case 1:
        coins -= 2;
        if (purpletime < totalclock)
            purpletime = totalclock+9600;
        else purpletime += 9600;
        statusbardraw(0,0,16,15,159,13+statusbaryoffset,statusbarinfo);
        break;
        case 2:
        coins -= 2;
        if (greentime < totalclock)
            greentime = totalclock + 9600;
        else greentime += 9600;
        statusbardraw(0,15,16,15,176,13+statusbaryoffset,statusbarinfo);
        break;
        case 3:
        coins -= 5;
        if (capetime[0] < totalclock)
            capetime[0] = totalclock + 7200;
        else
            capetime[0] += 7200;
        statusbardraw(16,0,21,28,194,2+statusbaryoffset,statusbarinfo);
        break;
        case 4:
        coins -= 5;
        if (capetime[1] < totalclock)
            capetime[1] = totalclock + 4800;
        else
            capetime[1] += 4800;
        statusbardraw(37,0,21,28,216,2+statusbaryoffset,statusbarinfo);
        break;
        case 5:
        coins -= 75;
        lightnings++;
        if (lightnings > 6)
            lightnings = 6;
        textbuf[0] = 9, textbuf[1] = 0;
        textprint(296,21+statusbaryoffset,(char)0);
        textbuf[0] = lightnings+48, textbuf[1] = 0;
        textprint(296,21+statusbaryoffset,(char)176);
        break;
        case 6:
        coins -= 100;
        firepowers[0]++;
        if (firepowers[0] > 6)
            firepowers[0] = 6;
        textbuf[0] = 9, textbuf[1] = 0;
        textprint(272,12+statusbaryoffset,(char)0);
        textbuf[0] = firepowers[0]+48, textbuf[1] = 0;
        textprint(272,12+statusbaryoffset,(char)176);
        break;
        case 7:
        coins -= 150;
        firepowers[1]++;
        if (firepowers[1] > 6)
            firepowers[1] = 6;
        textbuf[0] = 9, textbuf[1] = 0;
        textprint(272,21+statusbaryoffset,(char)0);
        textbuf[0] = firepowers[1]+48, textbuf[1] = 0;
        textprint(272,21+statusbaryoffset,(char)176);
        break;
        case 8:
        coins -= 200;
        firepowers[2]++;
        if (firepowers[2] > 6)
            firepowers[2] = 6;
        textbuf[0] = 9, textbuf[1] = 0;
        textprint(296,12+statusbaryoffset,(char)0);
        textbuf[0] = firepowers[2]+48, textbuf[1] = 0;
        textprint(296,12+statusbaryoffset,(char)176);
        break;
        case 9:
        coins -= 250;
        compass = 1;
        break;
    }
    textbuf[0] = 9, textbuf[1] = 9;
    textbuf[2] = 9, textbuf[3] = 0;
    textprint(112,12+statusbaryoffset,(char)0);
    textbuf[0] = (coins/100)+48;
    textbuf[1] = ((coins/10)%10)+48;
    textbuf[2] = (coins%10)+48;
    textbuf[3] = 0;
    if (textbuf[0] == 48)
    {
        textbuf[0] = 32;
        if (textbuf[1] == 48)
        textbuf[1] = 32;
    }
    textprint(112,12+statusbaryoffset,(char)176);
    }
    if (sodaplace < 0)
    {
    sodaplace = (-sodaplace)-1;
    ksay(26);
    }
    totalclock = ototclocker;
    SDL_LockMutex(timermutex);
    clockspeed = 0;
    SDL_UnlockMutex(timermutex);
#ifndef OPENGLES
    glDrawBuffer(GL_BACK);
#endif // !OPENGLES
    wipeoverlay(0,0,361,statusbaryoffset);
    linecompare(statusbar);
#ifdef OPENGLES
    currentMenuState = eNoMenu;
#endif
}

/* New credits instead of ordering info. */

void orderinfomenu() {
    K_INT16 n;
    if (vidmode == 0)
    n = 0;
    else
    n = 20;
    drawmenu(320,216,menu);
    strcpy(&textbuf[0],"Copyright notice");
    textprint(129,n+1,32);

    strcpy(textbuf,
       "\"Ken's Labyrinth\"");
    textprint(30,36,32);
    strcpy(textbuf,"(c) 1992-1993 Ken Silverman");

    textprint(30,46,32);

    strcpy(textbuf,
       "\"LAB3D/SDL\" conversion");
    textprint(30,56,176);

    /* Fonts only have 7-bit ASCII, and my surname needs a character not in
       this set. Fake the dots. */

    strcpy(textbuf,
       ".");
    textprint(180,60,176);
    textprint(184,60,176);
    strcpy(textbuf,
       "(c) 2002-2004 Jan Lonnberg");
    textprint(30,66,176);

    strcpy(textbuf,
       "http://icculus.org/LAB3D/");
    textprint(30,76,176);

    strcpy(textbuf,
       "\"LAB3D-SDL-enhanced\"");
    textprint(30,86,176);
    strcpy(textbuf,
       "(c) 2011-2015 Jared Stafford");
    textprint(30,96,176);
    strcpy(textbuf,
       "http://jspenguin.org/software/lab3d/");
    textprint(30,106,176);

    strcpy(textbuf,
       "OpenGL ES port");
    textprint(30,116,176);
    strcpy(textbuf,
       "(c) 2014 Scott \"Pickle\" Smith");
    textprint(30,126,176);
    strcpy(textbuf,
       "https://github.com/Pickle/Lab3dGLES");
    textprint(30,136,176);

    strcpy(textbuf,
       "Android port");
    textprint(30,146,176);
    strcpy(textbuf,
       "(c) 2015-2016 Serg \"EXL\" Koles");
    textprint(30,156,176);
    strcpy(textbuf,
       "http://exlmoto.ru/kenlab3d-droid");
    textprint(30,166,176);

    strcpy(textbuf,
       "See readme.txt for license");
    textprint(30,186,32);

    strcpy(textbuf,
       "Ken Silverman's official web site:");
    textprint(30,196,48);

    strcpy(textbuf,
       "http://www.advsys.net/ken");
    textprint(30,206,48);

    finalisemenu();
#ifdef OPENGLES
    drawOnScreen();
#endif // !OPENGLES
    pressakey();
}

/* Save/load game selector. */

K_INT16 loadsavegamemenu(K_INT16 whichmenu)
{
    char filename[20];
    K_INT16 fil, i, j, k, n;
    K_INT32 templong;
#ifndef OPENGLES
    if (vidmode == 0)
    n = 0;
    else
    n = 20;
    drawmenu(320,160,menu);
    if (whichmenu == 1)
    {
    strcpy(&textbuf[0],"Load game");
    textprint(137,26+n+1,32);
    }
    else
    {
    strcpy(&textbuf[0],"Save game");
    textprint(137,26+n+1,112);
    }
    strcpy(&textbuf[0],"#: Name:       Board: Score: Time:");
    textprint(55,52+n+1,48);
    if (gameheadstat == 0)
    {
    for(j=0;j<8;j++)
    {
        char path[256];
        filename[0] = 'S', filename[1] = 'A', filename[2] = 'V';
        filename[3] = 'G', filename[4] = 'A', filename[5] = 'M';
        filename[6] = 'E', filename[7] = j+48;
        filename[8] = '.', filename[9] = 'D', filename[10] = 'A';
        filename[11] = 'T', filename[12] = 0;

        snprintf(path, sizeof(path), "%s/%s", globalDataDir, filename);
        if((fil=open(path,O_RDONLY|O_BINARY,0))!=-1)
        {
        gamexist[j] = 1;
        read(fil,&gamehead[j][0],27);
        close(fil);
        }
        else {
        filename[0] = 's', filename[1] = 'a', filename[2] = 'v';
        filename[3] = 'g', filename[4] = 'a', filename[5] = 'm';
        filename[6] = 'e', filename[7] = j+48;
        filename[8] = '.', filename[9] = 'd', filename[10] = 'a';
        filename[11] = 't', filename[12] = 0;

        snprintf(path, sizeof(path), "%s/%s", globalDataDir, filename);
        if((fil=open(path,O_RDONLY|O_BINARY,0))!=-1)
        {
            gamexist[j] = 1;
            read(fil,&gamehead[j][0],27);
            close(fil);
        }
        else
            gamexist[j] = 0;
        }
    }
    gameheadstat = 1;
    }
    j = 0;
    for(i=70+n;i<166+n;i+=12)
    {
    if (gamexist[j] == 1)
    {
        textbuf[0] = j+49, textbuf[1] = 32, textbuf[2] = 32;
        for(k=0;k<12;k++)
        {
        textbuf[k+3] = gamehead[j][k];
        if (textbuf[k+3] == 0)
            textbuf[k+3] = 32;
        }
        textbuf[15] = 32;
        textbuf[16] = ((gamehead[j][17]+1)/10)+48;
        if (textbuf[16] == 48)
        textbuf[16] = 32;
        textbuf[17] = ((gamehead[j][17]+1)%10)+48;
        textbuf[18] = 32;
        textbuf[19] = 32;
        textbuf[20] = 32;
        k = j*27;
        templong=readlong((unsigned char *)&gamehead[j][19]);
//	    templong=*((K_INT32 *)(&gamehead[j][19]));

        textbuf[21] = (char)((templong/100000L)%10L)+48;
        textbuf[22] = (char)((templong/10000L)%10L)+48;
        textbuf[23] = (char)((templong/1000L)%10L)+48;
        textbuf[24] = (char)((templong/100L)%10L)+48;
        textbuf[25] = (char)((templong/10L)%10L)+48;
        textbuf[26] = (char)(templong%10L)+48;
        textbuf[27] = 32;
        k = 21;
        while ((textbuf[k] == 48) && (k < 26))
        textbuf[k++] = 32;
        k = j*27;
        templong=readlong((unsigned char *)&gamehead[j][23]);
//	    templong=*((K_INT32 *)(&gamehead[j][23]));

        templong /= 240;
        textbuf[28] = (char)((templong/10000L)%10L)+48;
        textbuf[29] = (char)((templong/1000L)%10L)+48;
        textbuf[30] = (char)((templong/100L)%10L)+48;
        textbuf[31] = (char)((templong/10L)%10L)+48;
        textbuf[32] = (char)(templong%10L)+48;
        textbuf[33] = 0;
        k = 28;
        while ((textbuf[k] == 48) && (k < 32))
        textbuf[k++] = 32;
        textprint(56,i-1+1,30);
        textprint(55,i-1+1,32);
    }
    else
    {
        textbuf[0] = j+49;
        textbuf[1] = 0;
        textprint(56,i-1+1,28);
        textprint(55,i-1+1,30);
    }
    j++;
    }
    finalisemenu();
#else
    qStateSaveOrLoad = whichmenu;
    currentMenuState = eLoadSaveGameMenu;
#endif // !OPENGLES
    if (whichmenu == 1)
    {
    do
    {
        loadsavegameplace = getselection(16,67,loadsavegameplace,8);
    }
    while ((loadsavegameplace >= 0) && (gamexist[loadsavegameplace] == 0));
    j = loadsavegameplace;
    if ((loadsavegameplace < 0) || (gamexist[loadsavegameplace] == 0))
        loadsavegameplace = (-loadsavegameplace)-1;
    }
    else
    {
    loadsavegameplace = getselection(16,67,loadsavegameplace,8);
    j = loadsavegameplace;
    if (loadsavegameplace < 0)
        loadsavegameplace = (-loadsavegameplace)-1;
    }
#ifdef OPENGLES
    currentMenuState = eMainMenu;
#endif
    return(j);
}

/* Choose episode... */

K_INT16 newgamemenu()
{
    K_INT16 j, n;
#ifndef OPENGLES
    if (vidmode == 0)
    n = 0;
    else
    n = 20;
    drawmenu(288,64,menu);
    strcpy(&textbuf[0],"New game");
    textprint(137,74+n+1,112);
    strcpy(&textbuf[0],"Episode 1: Search for Sparky");
    textprint(67,88+n+1,32);
    if (numboards >= 20) j = 32; else j = 28;
    strcpy(&textbuf[0],"Episode 2: Sparky's Revenge");
    textprint(67,100+n+1,((char)j));
    if (numboards >= 30) j = 32; else j = 28;
    strcpy(&textbuf[0],"Episode 3: Find the Way Home");
    textprint(67,112+n+1,((char)j));
    if (newgameplace < 0) newgameplace = 0;
    if (newgameplace > 2) newgameplace = 2;
    finalisemenu();
#else
    currentMenuState = eNewGameMenu;
#endif // !OPENGLES
    newgameplace = getselection(28,87,newgameplace,3);
#ifdef OPENGLES
    currentMenuState = eMainMenu;
#endif // OPENGLES
    if ((newgameplace == 1) && (numboards < 20))
    return(newgameplace);
    if ((newgameplace == 2) && (numboards < 30))
    return(newgameplace);
    if (newgameplace<0) {
    newgameplace=(-newgameplace)-1;
    return -1;
    }
#ifndef OPENGLES
    drawmenu(288,64,menu);
    strcpy(&textbuf[0],"New game");
    textprint(137,74+n+1,112);
    strcpy(&textbuf[0],"Easy: Don't touch me.");
    textprint(67,92+n+1,32);
    strcpy(&textbuf[0],"Hard: OUCH!");
    textprint(67,104+n+1,32);
    finalisemenu();
#else
    currentMenuState = eHardnessMenu;
#endif // !OPENGLES
    if (skilevel < 0) skilevel = 0;
    if (skilevel > 1) skilevel = 1;
    skilevel = getselection(28,91,skilevel,2);
#ifdef OPENGLES
    currentMenuState = eMainMenu;
#endif // OPENGLES
    if (skilevel<0) {
    skilevel=(-skilevel)-1;
    return -1;
    }
    j = newgameplace;
    if (newgameplace < 0)
    newgameplace = (-newgameplace)-1;
    return(j);
}

/* Wait for keypress (or mouse click). */

void pressakey()
{
    K_INT16 bstatus, obstatus;

    glFlush();

    bstatus = 1;
    obstatus = 1;
    keystatus[0x1c] = 0, keystatus[0x9c] = 0, keystatus[0x1] = 0, keystatus[0x39] = 0;
#ifndef USE_SDL2
    newkeystatus[SDLK_RETURN]=newkeystatus[SDLK_KP_ENTER]=0;
    newkeystatus[SDLK_ESCAPE]=newkeystatus[SDLK_SPACE]=0;
#else
    newkeystatus[getOldAsciiKeyCode(SDLK_RETURN)]=newkeystatus[getOldAsciiKeyCode(SDLK_KP_ENTER)]=0;
    newkeystatus[getOldAsciiKeyCode(SDLK_ESCAPE)]=newkeystatus[getOldAsciiKeyCode(SDLK_SPACE)]=0;
#endif // !USE_SDL2
    while (((keystatus[0x1c]|keystatus[0x9c]|keystatus[1]|keystatus[0x39]) == 0) && (bstatus <= obstatus))
    {
    PollInputs();
    obstatus = bstatus;
    if (moustat == 0) {
        bstatus=readmouse(NULL,NULL);
    }
    }
    ksay(27);
}

/* Update slot machine... */

void copyslots(K_INT16 slotnum)
{
    K_INT16 i, j, k;
    unsigned char *l;
    unsigned char *RGBATexture=malloc(64*64*4);

    l = walseg[slotinfo-1];
    for(i=0;i<3;i++)
    for(j=0;j<8;j++)
    {
        k = ((slotpos[i]+j)&63)+(((slotpos[i]+j)&64)<<3)+(i<<10);
        tempbuf[(i<<8)+(j<<3)]=l[k];
        tempbuf[(i<<8)+(j<<3)+1]=l[k+64];
        tempbuf[(i<<8)+(j<<3)+2]=l[k+128];
        tempbuf[(i<<8)+(j<<3)+3]=l[k+192];
        tempbuf[(i<<8)+(j<<3)+4]=l[k+256];
        tempbuf[(i<<8)+(j<<3)+5]=l[k+320];
        tempbuf[(i<<8)+(j<<3)+6]=l[k+384];
        tempbuf[(i<<8)+(j<<3)+7]=l[k+448];
    }
    l = walseg[slotnum-1];
    for(i=0;i<3;i++)
    for(j=0;j<8;j++)
    {
        k = 1296+(((i<<3)+i)<<6)+j;
        l[k]=tempbuf[(i<<8)+(j<<3)];
        l[k+64]=tempbuf[(i<<8)+(j<<3)+1];
        l[k+128]=tempbuf[(i<<8)+(j<<3)+2];
        l[k+192]=tempbuf[(i<<8)+(j<<3)+3];
        l[k+256]=tempbuf[(i<<8)+(j<<3)+4];
        l[k+320]=tempbuf[(i<<8)+(j<<3)+5];
        l[k+384]=tempbuf[(i<<8)+(j<<3)+6];
        l[k+448]=tempbuf[(i<<8)+(j<<3)+7];
    }
    glBindTexture(GL_TEXTURE_2D,texName[slotnum-1]);
    checkGLStatus( __FILE__, __LINE__ );

    TextureConvert(l, RGBATexture, bmpkind[slotnum]);

    BuildMipmaps((Uint32*)RGBATexture,64,64,1,10);
    checkGLStatus( __FILE__, __LINE__ );
    free(RGBATexture);
}

/* Update map position... */

void youarehere()
{

    walseg[map-1][yourhereoldpos]=board[0][yourhereoldpos];

    yourhereoldpos = ((posx>>10)<<6)+(posy>>10);

    walseg[map-1][yourhereoldpos]=255;
}

/* Update map texture... */

void updatemap() {
    unsigned char *RGBATexture=malloc(64*64*4);

    glBindTexture(GL_TEXTURE_2D,texName[map-1]);
    checkGLStatus( __FILE__, __LINE__ );

    TextureConvert(walseg[map-1], RGBATexture, bmpkind[map]);

    BuildMipmaps((Uint32*)RGBATexture,64,64,1,10);

    checkGLStatus( __FILE__, __LINE__ );
    free(RGBATexture);
}

/* Update game over texture... */

void updategameover() {
    unsigned char *RGBATexture=malloc(64*64*4);

    glBindTexture(GL_TEXTURE_2D,texName[gameover-1]);
    checkGLStatus( __FILE__, __LINE__ );

    TextureConvert(walseg[gameover-1], RGBATexture, bmpkind[gameover]);

    BuildMipmaps((Uint32*)RGBATexture,64,64,1,10);

    checkGLStatus( __FILE__, __LINE__ );
    free(RGBATexture);
}

/* Get keypress in Unicode form. */

Uint16 getkeypress() {
    SDL_Event event;
    int sk;

    while(SDL_PollEvent(&event))
    {
    switch(event.type)
    {
        case SDL_QUIT:
        quitgame=1;
        break;
        case SDL_KEYDOWN:
        sk=event.key.keysym.sym;
        // TO_DEBUG_LOG("Pushed Key is %d, code: %d.\n", sk, event.key.keysym.scancode );
#ifdef USE_SDL2
        sk=getOldAsciiKeyCode(sk);
#ifdef ANDROID_NDK
        sk=patchAndroidKeysDpadSDL2(sk);
#endif // ANDROID_NDK
#endif // USE_SDL2
        if ((sk<SDLKEYS)&&(PCkey[sk]>=0)) {
            keystatus[PCkey[sk]]=1;
        }
        if (sk<SDLKEYS)
            newkeystatus[sk]=1;
#ifndef USE_SDL2
        sk=event.key.keysym.unicode;
#else
        // Check if SHIFT keys pushed
        if (sk == getOldAsciiKeyCode(SDLK_RSHIFT) || sk == getOldAsciiKeyCode(SDLK_LSHIFT)) {
            return 0;
        }

        // If pushed, Upper Char
        if (keystatus[PCkey[getOldAsciiKeyCode(SDLK_RSHIFT)]] || keystatus[PCkey[getOldAsciiKeyCode(SDLK_LSHIFT)]]) {
            // There is char?
            if (sk >= getOldAsciiKeyCode(SDLK_a) && sk <= getOldAsciiKeyCode(SDLK_z)) {
                sk = getUpperChar(sk);
            }
        }
#endif // !USE_SDL2
        return sk;
        case SDL_KEYUP:
        sk=event.key.keysym.sym;
#ifdef USE_SDL2
        sk=getOldAsciiKeyCode(sk);
#ifdef ANDROID_NDK
        sk=patchAndroidKeysDpadSDL2(sk);
#endif // ANDROID_NDK
#endif // USE_SDL2
        if ((sk<SDLKEYS)&&(PCkey[sk]>=0)) {
            keystatus[PCkey[sk]]=0;
        }
        if (sk<SDLKEYS)
            newkeystatus[sk]=0;
        break;
        default:
        break;
    }
    }
    if (quitgame) quit();
    return 0;
}

/* Poll for keyboard input or quit command. */

void PollInputs() {
    SDL_Event event;
    int sk;
    static int i=0;
    //printf ("poll %d\n",i++);
    while(SDL_PollEvent(&event))
    {
    //printf ("event type=%d\n",event.type);
    switch(event.type)
    {
        case SDL_QUIT:
        quitgame=1;
        break;
        case SDL_JOYBUTTONDOWN:
        case SDL_JOYBUTTONUP:
        sk=event.jbutton.button;
        if (sk<numjoybuttons) {
            buttonstatus[sk]=(event.jbutton.state==SDL_PRESSED?1:0);
        }

        break;
        case SDL_JOYAXISMOTION:
        sk=event.jaxis.axis;
        if (sk<numjoyaxes) {
            axispos[sk]=(event.jaxis.value);
        }
        break;
        case SDL_KEYDOWN:
        sk=event.key.keysym.sym;
        // TO_DEBUG_LOG("Pushed Key is %d, code: %d.\n", sk, event.key.keysym.scancode );
#ifdef USE_SDL2
        sk=getOldAsciiKeyCode(sk);
#ifdef ANDROID_NDK
        sk=patchAndroidKeysDpadSDL2(sk);
#endif // ANDROID_NDK
#endif // USE_SDL2
        if ((sk<SDLKEYS)&&(PCkey[sk]>=0)) {
            keystatus[PCkey[sk]]=1;
        }
        if (sk<SDLKEYS)
            newkeystatus[sk]=1;
        break;
        case SDL_KEYUP:
        sk=event.key.keysym.sym;
#ifdef USE_SDL2
        sk=getOldAsciiKeyCode(sk);
#ifdef ANDROID_NDK
        sk=patchAndroidKeysDpadSDL2(sk);
#endif // ANDROID_NDK
#endif // USE_SDL2
        if ((sk<SDLKEYS)&&(PCkey[sk]>=0)) {
            keystatus[PCkey[sk]]=0;
        }
        if (sk<SDLKEYS)
            newkeystatus[sk]=0;
        break;
        default:
        break;
    }
    }
    if (quitgame) quit();
}

/* Read mouse position (into x and y; NULL to ignore) and return buttons
   (1=left, 2=right, 4=middle). */

unsigned char readmouse(int *x, int *y) {
    int tx,ty;
    unsigned char bstatus=SDL_GetRelativeMouseState(&tx, &ty);

    /* Swap middle and right mouse buttons to match Microsoft style. */

    if (x!=NULL) *x+=5*tx;
    if (y!=NULL) *y+=5*ty;

    return ((bstatus&4)>>1)|(bstatus&1)|(bstatus&2)<<1;
}

/* Read joystick position (into x and y; NULL to ignore) and return buttons
   (1=button 0, 2=button 1). */

unsigned char readjoystick(int *x, int *y) {
    SDL_JoystickUpdate();
#if 0
    if (x!=NULL) *x=SDL_JoystickGetAxis(joystick, 0);
    if (y!=NULL) *y=SDL_JoystickGetAxis(joystick, 1);

    return (SDL_JoystickGetButton(joystick, 0))|
    (SDL_JoystickGetButton(joystick, 1)<<1);
#else
    if (x!=NULL) *x=getaxispos(1,0);
    if (y!=NULL) *y=getaxispos(0,0);
    int ret=0;
    if (buttondefs[8]!=-1 && buttonstatus[buttondefs[8]]) ret |= 1;
    if (buttondefs[12]!=-1 && buttonstatus[buttondefs[12]]) ret |= 2;

    return ret;
#endif
}

void quit() {

    savesettings();

    /* Start by demolishing all other threads... */

    SDL_UnlockMutex(timermutex); /* Just in case we have it... */
    SDL_UnlockMutex(soundmutex); /* Just in case we have it... */

    musicoff();

    if (speechstatus >= 2) {
    /* SDL is very careful to allow the sound thread to stop. Good for
       us. */

    SDL_CloseAudio();
    free(SoundBuffer);
    }

    free(screenbuffer);

    if (quitgame == 2)
    TO_DEBUG_LOG("Error #3:  Invalid saved game.");
    if (numboards < 30)
    {

    /* Print shareware message:
       Dump 16K of segment walseg[endtext-1] to text mode screen,
       move cursor to start of line 0x16 (0 is top).

       Removed to avoid confusing people (and to save me the trouble
       of parsing a CGA colour text screen). */

    }
    free((void *)note);
    free(lzwbuf);
    free(lzwbuf2);
    if (convwalls > 0) free(pic);
    if (joystat==0)
    SDL_JoystickClose(joystick);

#ifdef USE_SDL2
#ifndef ANDROID_NDK
    TO_DEBUG_LOG("Delete gamma Ramp.\n");
    SDL_free(gammaRamp);
    gammaRamp = NULL;
#endif // !ANDROID_NDK
    TO_DEBUG_LOG("Delete GL contex.\n");
    //SDL_GL_DeleteContext(glContext);
    //glContext = NULL;
    eglDestroySurface(glDisplay, glSurface);
    eglDestroyContext(glDisplay, glContext);
    eglTerminate(glDisplay);
    TO_DEBUG_LOG("Destroy SDL Window.\n");
    SDL_DestroyWindow(globalWindow);
    globalWindow = NULL;
#endif // USE_SDL2

    SDL_Quit();

    exit(0);
}

#define log2(a) (log(a)/log(2))

void randoinsts()
{
    long i, j, k;
    float f;

    if (channels == 2)
    {
    j = (rand()&2)-1; k = 0;
    for(i=0;i<9;i++)
    {
        if ((i == 0) || (chantrack[i] != chantrack[i-1]))
        {
        f = (float)rand()/(float)RAND_MAX;
        if (j > 0)
        {
            //lvol[i] < rvol[i]
            lvol[i] = log2(f+1);
            rvol[i] = log2(3-f);
            lplc[i] = rand()&255;
            rplc[i] = 0;
        }
        else
        {
            //lvol[i] > rvol[i]
            lvol[i] = log2(3-f);
            rvol[i] = log2(f+1);
            lplc[i] = 0;
            rplc[i] = rand()&255;
        }
        j = -j;
        if (((drumstat&32) == 0) || (i < 6)) k++;
        }
        else
        {
        lvol[i] = lvol[i-1]; rvol[i] = rvol[i-1];
        lplc[i] = lplc[i-1]; rplc[i] = rplc[i-1];
        }
    }
    if (k < 2)  //If only 1 source, force it to be in center
    {
        if (drumstat&32) i = 5; else i = 8;
        for(;i>=0;i--)
        {
        lvol[i] = rvol[i] = 1;
        lplc[i] = rplc[i] = 0;
        }
    }
    }
}

void drawinputbox() {
    int k;

    menuing=1; menuleft=84; menutop=127;
    menuwidth=192; menuheight=64;
    statusbardraw(0, 0, 57, 64, 84-20, 127, lab3dversion?76:scorebox);
    statusbardraw(7, 0, 39, 64, 141-20, 127, lab3dversion?76:scorebox);
    statusbardraw(7, 0, 39, 64, 180-20, 127, lab3dversion?76:scorebox);
    statusbardraw(7, 0, 57, 64, 219-20, 127, lab3dversion?76:scorebox);
    for(k=0;k<22;k++)
    textbuf[k] = 8;
    textbuf[22] = 0;
    textprint(91,145+1,(char)0);
    textbuf[1] = 0;
    textprint(261,145+1,(char)0);
}
