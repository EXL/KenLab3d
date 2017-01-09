#include "lab3d.h"
#ifdef USE_OSS
#define MUSIC_SOURCES 3
#else
#ifdef WIN32
#define MUSIC_SOURCES 3
#include "objbase.h"
#include "shlobj.h"
#else
#define MUSIC_SOURCES 2
#endif
#endif

#ifdef WIN32
HRESULT CreateLink(LPCSTR lpszPathObj,
           LPSTR lpszPathLink, LPSTR lpszDesc,
           LPSTR lpszArgs) {
    HRESULT hres;
    IShellLink* psl;
    char p[MAX_PATH];

    CoInitialize(NULL);
    hres = CoCreateInstance(&CLSID_ShellLink, NULL,
                CLSCTX_INPROC_SERVER, &IID_IShellLink,
                (void *)&psl);
    if (SUCCEEDED(hres)) {
        IPersistFile* ppf;

    GetCurrentDirectory(MAX_PATH, p);
        psl->lpVtbl->SetWorkingDirectory(psl, p);
        hres=psl->lpVtbl->SetPath(psl, lpszPathObj);

        psl->lpVtbl->SetArguments(psl, lpszArgs);

        psl->lpVtbl->SetDescription(psl, lpszDesc);

        hres = psl->lpVtbl->QueryInterface(psl, &IID_IPersistFile,
                       (void *)&ppf);

        if (SUCCEEDED(hres)) {
            WORD wsz[MAX_PATH];

        TO_DEBUG_LOG("Trying to save shortcut...\n");
            MultiByteToWideChar(CP_ACP, 0, lpszPathLink, -1,
                wsz, MAX_PATH);

            hres = ppf->lpVtbl->Save(ppf, wsz, TRUE);
            ppf->lpVtbl->Release(ppf);
        if (SUCCEEDED(hres))
        TO_DEBUG_LOG("Done.\n");
        }
        psl->lpVtbl->Release(psl);
    }
    CoUninitialize();
    return hres;
}

void createshortcut(void) {
    ITEMIDLIST *l;
    char p[MAX_PATH];
    char p2[MAX_PATH];
    char p3[MAX_PATH];
    int i;

    TO_DEBUG_LOG("Getting desktop location.\n");
    if (SHGetSpecialFolderLocation(GetDesktopWindow(),
                   CSIDL_DESKTOPDIRECTORY,&l)!=NOERROR)
    return;
    TO_DEBUG_LOG("Converting...\n");
    if (SHGetPathFromIDList(l,p)!=TRUE) return;
    TO_DEBUG_LOG("Desktop location is %s.\n",p);

    i=strlen(p);
    if (i>MAX_PATH-20) return;
    if (p[i-1]=='\\') p[i-1]=0;
    strcpy(p3,p);
    strcat(p3,"\\Ken's Labyrinth.lnk");

    TO_DEBUG_LOG("Creating link as %s.\n",p3);

    GetCurrentDirectory(MAX_PATH,p2);

    i=strlen(p2);
    if (i>MAX_PATH-10) return;
    if (p2[i-1]=='\\') p2[i-1]=0;
    strcat(p2,"\\ken.exe");

    CreateLink(p2, p3,"Ken's Labyrinth","");

    strcpy(p3,p);
    strcat(p3,"\\Ken's Labyrinth Setup.lnk");

    CreateLink(p2, p3,"Ken's Labyrinth Setup","-setup");
}
#endif

int inputdevice=0,resolutionnumber=3,nearest=0;
int music=1,sound=1,fullscr=1,cheat=0,channel=1,musicchannel=1;
int soundblock=0,timing=0,texturedepth=1,scaling=2;

char keynames[numkeys][30]={
    "Move FORWARD",
    "Move BACKWARD",
    "Turn LEFT",
    "Turn RIGHT",
    "STRAFE (walk sideways)",
    "STAND HIGH",
    "STAND LOW",
    "RUN",
    "FIRE",
    "FIREBALLS (red)",
    "BOUNCY-BULLETS (green)",
    "HEAT-SEEKING MISSILES",
    "UNLOCK / OPEN / CLOSE / USE",
    "CHEAT for more life",
    "RAISE / LOWER STATUS BAR",
    "PAUSE GAME",
    "MUTE KEY",
    "SHOW MENU"
};

char axisnames[numaxes][30]={
    "Move Forward/Back",
    "Turn Left/Right",
    "Strafe",
    "Stand High/Low"
};
char axisinst[numaxes][30]={
    "FORWARD",
    "LEFT",
    "LEFT",
    "UP"
};
char jaxisnames[numjoyaxes][30]={
    "X Axis",
    "Y Axis",
    "Axis 3",
    "Axis 4",
    "Axis 5",
    "Axis 6"
};

#ifdef USE_SDL2
#include "sdl2keyhelper.h"
#ifndef ANDROID_NDK
static int newdefaultkey[numkeys]={
    OLDK_UP,
    OLDK_DOWN,
    OLDK_LEFT,
    OLDK_RIGHT,
    OLDK_RCTRL,
    OLDK_a,
    OLDK_z,
    OLDK_LSHIFT,
    OLDK_LCTRL,
    OLDK_F1,
    OLDK_F2,
    OLDK_F3,
    OLDK_SPACE,
    OLDK_BACKSPACE,
    OLDK_RETURN,
    OLDK_p,
    OLDK_m,
    OLDK_ESCAPE
};
#else
static int newdefaultkey[numkeys]={
    OLDK_UP,
    OLDK_DOWN,
    OLDK_LEFT,
    OLDK_RIGHT,
    OLDK_q,
    OLDK_i,
    OLDK_k,
    OLDK_LSHIFT,
    OLDK_SPACE,
    OLDK_z,
    OLDK_x,
    OLDK_c,
    OLDK_e,
    OLDK_BACKSPACE,
    OLDK_RETURN,
    OLDK_p,
    OLDK_m,
    OLDK_ESCAPE
};
#endif // !ANDROID_NDK
#else
static int newdefaultkey[numkeys]={
    SDLK_UP,
    SDLK_DOWN,
    SDLK_LEFT,
    SDLK_RIGHT,
    SDLK_RCTRL,
    SDLK_a,
    SDLK_z,
    SDLK_LSHIFT,
    SDLK_LCTRL,
    SDLK_F1,
    SDLK_F2,
    SDLK_F3,
    SDLK_SPACE,
    SDLK_BACKSPACE,
    SDLK_RETURN,
    SDLK_p,
    SDLK_m,
    SDLK_ESCAPE
};
#endif // !USE_SDL2

static int newdefaultbutton[numkeys]={
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    0,
    -1,
    -1,
    -1,
    1,
    -1,
    -1,
    -1,
    -1,
    -1
};
static int newdefaultaxis[numaxes]={
    2,
    1,
    0,
    0
};

char inputdevicemenu[4][30]={
    "Keyboard only",
    "Keyboard + mouse",
    "Keyboard + joystick",
    "Keyboard + mouse + joystick"
};

/* Predefined resolutions for backward compatibility with early betas. */

static int resx[19]={360,512,640,800,1024,1152,1280,1600,
             320,320,400,480,640,720,720,960,1920,1920,1280};
static int resy[19]={240,384,480,600,768,864,1024,1200,
             200,240,300,360,400,480,576,720,1080,1200,960};

static char resolutiontypemenu[3][30]={
    "Fullscreen resolutions",
    /*    "Standard resolutions",
      "Non-standard resolutions",*/
    "Custom resolution",
    "Return to setup menu"
};

char resolutionstandardmenu[8][30]={
    "360x240",
    "512x384",
    "640x480",
    "800x600",
    "1024x768",
    "1152x864",
    "1280x1024",
    "1600x1200"
};

char resolutionspecialmenu[11][30]={
    "320x200",
    "320x240",
    "400x300",
    "480x360",
    "640x400",
    "720x480",
    "720x576",
    "960x720",
    "1920x1080",
    "1920x1200",
    "1280x960"
};

char configureinputmenu[4][30]={
    "Set Keys",
    "Set Joystick Buttons",
    "Set Axes",
    "Return to setup menu"
};

char fullscreenmenu[2][30]={
    "Windowed",
#ifndef ANDROID_NDK
    "Fullscreen"
#else
    "Fullscreen (Android)"
#endif // !ANDROID_NDK
};

char filtermenu[3][30]={
    "Anisotropic filtering",
    "Trilinear filtering",
    "No filtering"
};

char musicmenu[3][30]={
    "No music",
#ifndef ANDROID_NDK
    "Adlib emulation",
#else
    "See Android GUI Settings",
#endif // !ANDROID_NDK
    "General MIDI",
};

char soundmenu[2][30]={
    "No sound",
#ifndef ANDROID_NDK
    "Digital sound effects",
#else
    "See Android GUI Settings",
#endif // !ANDROID_NDK
};

char channelmenu[2][30]={
    "Mono",
    "Stereo"
};

char cheatmenu[3][30]={
    "No cheats",
    "LSHIFT-RSHIFT",
    "LSHIFT-LCTRL"
};

char soundblockmenu[10][30]={
    "Default (11.6 ms)",
#ifndef ANDROID_NDK
    "1.5 ms",
#else
    "Android (1.5 ms)",
#endif // !ANDROID_NDK
    "2.9 ms",
    "5.8 ms",
    "11.6 ms",
    "23.2 ms",
    "46.4 ms",
    "92.9 ms",
    "185.8 ms",
    "371.5 ms"
};

static char timingmenu[2][30]={
    "System timer",
    "Sound output"
};

char texturedepthmenu[3][30]={
    "Driver default",
#ifndef ANDROID_NDK
    "32 bit",
#else
    "GL ES GL_RGBA",
#endif // !ANDROID_NDK
    "16 bit"
};

char scalingtypemenu[4][30]={
    "Fill screen (4:3 view)",
    "Integer scale (4:3 view)",
    "Fill screen (square pixels)",
    "Integer scale (square pixels)"
};

void selectionmenu(int alts,char titles[][30],int *value) {
    int i;
    int j=12*alts+24;

#ifndef OPENGLES
    drawmenu(304,j,menu);

    for(i=0;i<alts;i++) {
    strcpy(textbuf,titles[i]);
    textprint(71,120-6*alts+12*i,lab3dversion?32:34);
    }

    finalisemenu();
#endif // OPENGLES

    if (j>240)
    i=getselection(28,97-6*alts,*value,alts);
    else
    i=getselection(28,99-6*alts,*value,alts);

    if (i>=0) *value=i;

#ifdef OPENGLES
    switch (currentMenuState) {
    case eInputDevicesMenu:
    case eSetupFiltering:
    case eSetupMusic:
    case eSetupSound:
    case eSetupSoundCh:
    case eSetupMusicCh:
    case eSetupCheats:
    case eSetupSoundBlock:
    case eSetupScaling:
        currentMenuState = eSettingsMenu;
        break;
    default:
        break;
    }
#endif // OPENGLES
}

int resolutionmenu(int alts,int start,char titles[][30],int def) {
    int i;
    int j=12*alts+24;
    char t[12];

    if (start<0) {
    for(i=0;i<alts;i++) {
        if (def>=10000)
        sprintf(t,"%dx%d",def/10000,def%10000);
        else
        sprintf(t,"%dx%d",resx[def],resy[def]);
        if (!strcmp(t,titles[i]))
        break;
    }
    if (i<alts)
        def=i;
    else
        def=0;
    } else {
    def-=start;
    if (def<0) def=0;
    if (def>=alts) def=0;
    }
#ifndef OPENGLES
    drawmenu(304,j,menu);

    for(i=0;i<alts;i++) {
    strcpy(textbuf,titles[i]);
    textprint(71,120-6*alts+12*i,lab3dversion?32:34);
    }

    finalisemenu();
#endif
    i=getselection(28,99-6*alts,def,alts);

#ifdef OPENGLES
    switch (currentMenuState) {
    case eSetupConfigureMenu:
        currentMenuState = eSettingsMenu;
        break;
    default:
        break;
    }
#endif // OPENGLES

    if (i>=0) return i; else return -1;
}

int getnumber(void) {
    unsigned char ch;
    char buf[10];
    K_INT16 i,j;

    for(j=0;j<12;j++)
    textbuf[j] = 8;
    textbuf[12] = 0;
    textprint(94,145+1,(char)0);
    j = 0;
    buf[0]=0;
    ch = 0;
#ifndef USE_SDL2
    SDL_EnableUNICODE(1);
#endif // !USE_SDL2
    while ((ch != 13) && (ch != 27))
    {
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
        buf[j] = ch;
        for(j=0;j<10;j++)
        buf[j] = 0;
        for(j=0;j<12;j++)
        textbuf[j] = 8;
        textbuf[12] = 0;
        textprint(94,145+1,(char)0);
        j = 0;
        ch = 0;
    }
    if ((ch == 8) && (j > 0))
    {
        j--, buf[j] = 0;
        textbuf[0] = ch;
        textbuf[1] = 0;
        textprint(94+(j<<3),145+1,(char)0);
    }
    if ((ch >= 48) && (ch <= 57) && (j < 4))
    {
        textbuf[0] = ch;
        textbuf[1] = 0;
        textprint(94+(j<<3),145+1,(char)97);
        buf[j] = ch;
        if ((ch != 32) || (j > 0))
        j++;
    }
    }
#ifndef USE_SDL2
    SDL_EnableUNICODE(0);
#endif // !USE_SDL2
    for(i=0;i<256;i++)
    keystatus[i] = 0;
    if (ch==27) return -1;
    return strtol(buf,NULL,10);
}

void customresolution(void) {
    int x,y;

    drawinputbox();
    finalisemenu();
    sprintf(&textbuf[0],"Enter screen width:");
    textprint(180-(strlen(textbuf)<<2),135+1,(char)161);
    x=getnumber();
    if (x>0) {
    drawinputbox();
    finalisemenu();
    sprintf(&textbuf[0],"Enter screen height:");
    textprint(180-(strlen(textbuf)<<2),135+1,(char)161);
    y=getnumber();
    if (y>0)
        resolutionnumber=x*10000+y;
    }
}

void setupinputdevices(void) {
#ifdef OPENGLES
    currentMenuState = eInputDevicesMenu;
#endif // OPENGLES
    selectionmenu(4,inputdevicemenu,&inputdevice);
}

int modecompare(const void *a, const void *b) {
    SDL_Rect *c=*(SDL_Rect **)a;
    SDL_Rect *d=*(SDL_Rect **)b;

    return ((K_INT32)d->w*((K_INT32)d->h)-((K_INT32)c->w)*((K_INT32)c->h));
}

void setupsetresolution(void) {
#ifndef USE_SDL2
    int a,i,m;
    int resolutionmenusize;
    int detectedresolution[11];
    char resolutiondetectmenu[11][30];

    SDL_Rect **modes,**umodes;
#else
    int a;
#endif // !USE_SDL2
    a=resolutionmenu(3,0,resolutiontypemenu,0);

    switch(a) {
        case 0:
#ifndef USE_SDL2
        modes=umodes=SDL_ListModes(NULL,SDL_FULLSCREEN);

        if ((modes==NULL)||(modes==(SDL_Rect **)-1))
        return;
        m=0;

        while(*modes) {
        m++;
        modes++;
        }

        modes=malloc(sizeof(SDL_Rect *)*(m+1));
        for(i=0;i<=m;i++)
        modes[i]=umodes[i];

        qsort(modes, m, sizeof(SDL_Rect *), modecompare);

        i=0;
        do {
        resolutionmenusize=0;
        while(modes[i]&&(resolutionmenusize<10)) {
            if (modes[i]->w<10000&&
            modes[i]->h<10000) {
            detectedresolution[resolutionmenusize]=
                modes[i]->w*10000+modes[i]->h;
            sprintf(resolutiondetectmenu[resolutionmenusize],
                "%dx%d",
                detectedresolution[resolutionmenusize]/10000,
                detectedresolution[resolutionmenusize]%10000);
            resolutionmenusize++;
            }
            i++;
        }
        if (modes[i]) {
            strcpy(resolutiondetectmenu[resolutionmenusize],"More...");
        }
        a=resolutionmenu(resolutionmenusize+(modes[i]!=0),
                 -100,resolutiondetectmenu,
                 resolutionnumber);
        if (a<0) {free(modes); return;}
        } while(a==resolutionmenusize);
        resolutionnumber=detectedresolution[a];
        free(modes);
#endif // !USE_SDL2
        break;
        /*
    case 1:
        a=resolutionmenu(8,0,resolutionstandardmenu,resolutionnumber);
        if (a>=0) resolutionnumber=a;
        break;
    case 2:
        a=resolutionmenu(11,8,resolutionspecialmenu,resolutionnumber);
        if (a>=0) resolutionnumber=a+8;
        break;
        */
    case 1:
        customresolution();
        break;
    }
}

void setupsetfullscreen(void) {
    selectionmenu(2,fullscreenmenu,&fullscr);
}

void setupsetfiltering(void) {
#ifdef OPENGLES
    currentMenuState = eSetupFiltering;
#endif // OPENGLES
    selectionmenu(3,filtermenu,&nearest);
}

void setupsetmusic(void) {
#ifdef OPENGLES
    currentMenuState = eSetupMusic;
#endif // OPENGLES
    selectionmenu(MUSIC_SOURCES,musicmenu,&music);
}

void setupsetsound(void) {
#ifdef OPENGLES
    currentMenuState = eSetupSound;
#endif // OPENGLES
    selectionmenu(2,soundmenu,&sound);
}

void setupcheatmenu(void) {
#ifdef OPENGLES
    currentMenuState = eSetupCheats;
#endif // OPENGLES
    selectionmenu(3,cheatmenu,&cheat);
}

void setupsetsoundchannels(void) {
#ifdef OPENGLES
    currentMenuState = eSetupSoundCh;
#endif // OPENGLES
    selectionmenu(2,channelmenu,&channel);
}

void setupsetmusicchannels(void) {
#ifdef OPENGLES
    currentMenuState = eSetupMusicCh;
#endif // OPENGLES
    selectionmenu(2,channelmenu,&musicchannel);
}

void setupsoundblockmenu(void) {
#ifdef OPENGLES
    currentMenuState = eSetupSoundBlock;
#endif // OPENGLES
    selectionmenu(10,soundblockmenu,&soundblock);
    TO_DEBUG_LOG("Sound Block Size: %d", soundblock);
}

void setuptimingmenu(void) {
    selectionmenu(2,timingmenu,&timing);
}

void setuptexturedepthmenu(void) {
    selectionmenu(3,texturedepthmenu,&texturedepth);
}

void setupscalingmodemenu(void) {
#ifdef OPENGLES
    currentMenuState = eSetupScaling;
#endif // OPENGLES
    selectionmenu(4,scalingtypemenu,&scaling);
}

void setupsetkeys(void) {
    int i=0,j,quit=0,sk;
    SDL_Event event;

    i=0;
    while(!quit) {
#ifndef OPENGLES
    drawmenu(360,240,menu);

    for(j=0;j<numkeys;j++) {
        strcpy(textbuf,keynames[j]);
        textprint(31,13+12*j,lab3dversion?32:34);
        strncpy(textbuf,SDL_GetKeyName(newkeydefs[j]),11);
        textbuf[11]=0;
        textprint(261,13+12*j,lab3dversion?32:34);
    }

    finalisemenu();
#endif
    i=getselection(-12,-9,i,numkeys);
    if (i<0) quit=1;
    else if (i>=numkeys) quit=1;
    else {
        j=-1;
        while(j<0) {
        while(SDL_PollEvent(&event))
        {
            switch(event.type)
            {
            case SDL_KEYDOWN:
                sk=event.key.keysym.sym;
#ifdef USE_SDL2
                sk=getOldAsciiKeyCode(sk);
#ifdef ANDROID_NDK
                sk=patchAndroidKeysDpadSDL2(sk);
#endif // ANDROID_NDK
#endif // USE_SDL2
                if (sk<SDLKEYS) {
                j=sk;
                }
                break;
            default:
                break;
            }
        }
        SDL_Delay(10);
        }
        newkeydefs[i]=j;
    }
    }
}
void setupsetbuttons(void) {
    int i=0,j,quit=0,sk,cb;
    SDL_Event event;

    i=0;
    while(!quit) {
#ifndef OPENGLES
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
#endif
    i=getselection(-12,-9,i,numkeys);
    if (i<0) quit=1;
    else if (i>=numkeys) quit=1;
    else {
        j=-2;
        while(j<-1) {
        while(SDL_PollEvent(&event))
        {
            switch(event.type)
            {
            case SDL_JOYBUTTONDOWN:
            sk=event.jbutton.button;
            if (sk<numjoybuttons) {
                j=sk;
            }
            break;
            case SDL_KEYDOWN:
                // TO_DEBUG_LOG("s.c2: Scancode: %d.\n", sk);
            j=-1;
            break;
            default:
            break;
            }
        }
        SDL_Delay(10);
        }
        buttondefs[i]=j;
    }
    }
}
#define AXIS_NOVALUE (~0)
void setupsetaxes(void) {
    int i=0,j,quit=0,jdone=0,sk,cb;
    int axisvalues[numjoyaxes];
    SDL_Event event;

    for (i=0;i<numjoyaxes;i++) {
    axisvalues[i]=AXIS_NOVALUE;
    }

    i=0;
    while(!quit) {
#ifndef OPENGLES
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
#else
        for(j=0;j<numaxes;j++);
#endif
    i=getselection(-12,-9,i,numaxes);
    if (i<0) quit=1;
    else if (i>=numaxes) quit=1;
    else {
#ifndef OPENGLES
        drawmenu(304,72,menu);
        strcpy(textbuf,"Move joystick in");
        textprint((360-(8*strlen(textbuf)))/2,((240-72)/2)+12+0*12,lab3dversion?32:34);
        strcpy(textbuf,axisinst[i]);
        textprint((360-(8*strlen(textbuf)))/2,((240-72)/2)+12+1*12,0);
        strcpy(textbuf,"direction, or press");
        textprint((360-(8*strlen(textbuf)))/2,((240-72)/2)+12+2*12,lab3dversion?32:34);

        strcpy(textbuf,"any key to delete");
        textprint((360-(8*strlen(textbuf)))/2,((240-72)/2)+12+3*12,lab3dversion?32:34);
        finalisemenu();
        glFlush();
#endif
        jdone=0;
        while(!jdone) {
#ifdef OPENGLES
            drawmenu(304,72,menu);
            strcpy(textbuf,"Move joystick in");
            textprint((360-(8*strlen(textbuf)))/2,((240-72)/2)+12+0*12,lab3dversion?32:34);
            strcpy(textbuf,axisinst[i]);
            textprint((360-(8*strlen(textbuf)))/2,((240-72)/2)+12+1*12,0);
            strcpy(textbuf,"direction, or press");
            textprint((360-(8*strlen(textbuf)))/2,((240-72)/2)+12+2*12,lab3dversion?32:34);

            strcpy(textbuf,"any key to delete");
            textprint((360-(8*strlen(textbuf)))/2,((240-72)/2)+12+3*12,lab3dversion?32:34);
            finalisemenu();
            glFlush();
#endif
        while(SDL_PollEvent(&event))
        {
            switch(event.type)
            {
            case SDL_JOYAXISMOTION:
            sk=event.jaxis.axis;
            if (sk<numjoyaxes) {
                if (axisvalues[sk]!=AXIS_NOVALUE) {
                int diff=event.jaxis.value-axisvalues[sk];
                if (diff>8192) {
                    j=-sk-1;
                    jdone=1;
                }
                if (diff<-8192) {
                    j=sk+1;
                    jdone=1;
                }
                } else axisvalues[sk]=event.jaxis.value;
            }
            break;
            case SDL_KEYDOWN:
            j=0;
            jdone=1;
            break;
            default:
            break;
            }
        }
        SDL_Delay(10);
#ifdef OPENGLES
        drawOnScreen();
#endif
        }
        axisdefs[i]=j;
    }
    }
}
void setupconfigureinput(void) {
#ifdef OPENGLES
    currentMenuState = eSetupConfigureMenu;
#endif // OPENGLES
    int a,quit=0;
    while (!quit) {
    a=resolutionmenu(4,0,configureinputmenu,0);
    switch(a) {
    case 0:
#ifdef OPENGLES
    currentMenuState = eSetupKeys;
#endif // OPENGLES
        setupsetkeys();
        break;
    case 1:
#ifdef OPENGLES
    currentMenuState = eSetupJButtons;
#endif // OPENGLES
        setupsetbuttons();
        break;
    case 2:
#ifdef OPENGLES
    currentMenuState = eSetupJAxes;
#endif // OPENGLES
        setupsetaxes();
        break;
    default:
        quit=1;
        break;
    }
#ifdef OPENGLES
    switch (currentMenuState) {
    case eSetupKeys:
    case eSetupJButtons:
    case eSetupJAxes:
        currentMenuState = eSetupConfigureMenu;
        break;
    default:
        break;
    }
#endif // OPENGLES
    }
}

void setupmenu(void) {
    int quit=0,sel=0;

    while(!quit) {
#ifndef OPENGLES
    drawmenu(360,240,menu);

    int offs = 81;
#ifndef USE_SDL2
    strcpy(textbuf,"LAB3D/SDL setup menu");
#else
#ifndef ANDROID_NDK
    strcpy(textbuf,"LAB3D/SDL2 setup menu");
#else
    offs = 31;
    strcpy(textbuf,"Ken's Labyrinth (LAB3D/SDL2) setup menu");
#endif // !ANDROID_NDK
#endif // !USE_SDL2
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
#ifdef WIN32
        strcpy(textbuf,"Create desktop shortcuts");
    textprint(51,204,96);
#endif

    strcpy(textbuf,"Use cursor keys and Return to select.");
    textprint(31,220,lab3dversion?32:34);

    finalisemenu();
#else
        currentMenuState = eSettingsMenu;
#endif // !OPENGLES
#ifdef WIN32
    if ((sel = getselection(12,15,sel,15)) < 0)
#else
    if ((sel = getselection(12,15,sel,14)) < 0)
#endif
        quit=1;
    else {
        switch(sel) {
        case 0:
            setupinputdevices();
            break;
        case 1:
            setupconfigureinput();
            break;
        case 2:
#ifndef ANDROID_NDK
            setupsetresolution();
#endif // !ANDROID_NDK
            break;
        case 3:
#ifndef ANDROID_NDK
            setupsetfullscreen();
#endif // !ANDROID_NDK
            break;
        case 4:
            setupsetfiltering();
            break;
        case 5:
#ifndef ANDROID_NDK
            setupsetmusic();
#endif // !ANDROID_NDK
            break;
        case 6:
#ifndef ANDROID_NDK
            setupsetsound();
#endif // !ANDROID_NDK
            break;
        case 7:
            setupsetsoundchannels();
            break;
        case 8:
            setupsetmusicchannels();
            break;
        case 9:
            setupcheatmenu();
            break;
        case 10:
            setupsoundblockmenu();
            break;
        case 11:
#ifndef ANDROID_NDK
            setuptexturedepthmenu();
#endif // !ANDROID_NDK
            break;
        case 12:
            setupscalingmodemenu();
            break;
#ifdef WIN32
        case 14:
            createshortcut();
            break;
#endif
        case 13:
            quit=1;
            break;
        }
    }
    }
#ifdef OPENGLES
    currentMenuState = eNoMenu;
#endif // OPENGLES
}

void configure(void) {
    int div1,div2;

    if (resolutionnumber<19) {
    screenwidth=resx[resolutionnumber];
    screenheight=resy[resolutionnumber];
    } else {
    /* Encoding chosen to be human-readable. Backwards compatibility
       kludge. */
    screenwidth=resolutionnumber/10000;
    screenheight=resolutionnumber%10000;
    }
    fullscreen=fullscr;
    fullfilter=nearest==2?GL_NEAREST:GL_LINEAR_MIPMAP_LINEAR;
    partialfilter=nearest==2?GL_NEAREST:GL_LINEAR;

    anisotropic=(nearest==0);

    speechstatus = sound?2:0;
    switch(music) {
    case 2:
        musicsource=1;
        break;
    case 1:
        musicsource=2;
        break;
    default:
        musicsource=-1;
        break;
    }

#ifdef ANDROID_NDK
    // Patch Sound and Music Settings
    if (getSoundSettingsValue()) {
        speechstatus = 2;
    } else {
        speechstatus = 0;
    }

    if (getMusicSettingsValue()) {
        musicsource = 2;
    } else {
        musicsource = -1;
    }
#endif // ANDROID_NDK

    moustat = ((3-inputdevice)&1);
    joystat = ((3-inputdevice)>>1);
    cheatenable=cheat;
    if (channel||musicchannel)
    channels=2;
    else
    channels=1;

    soundpan=channel;
    musicpan=musicchannel;

    soundblocksize=/* channels* */
    ((musicsource==2)?SOUNDBLOCKSIZE44KHZ:SOUNDBLOCKSIZE11KHZ);
    if (soundblock>0) {
    soundblocksize>>=4;
    soundblocksize<<=soundblock;
    }
    soundtimer=0;

#ifndef OPENGLES
    switch(texturedepth) {
    case 1:
        colourformat=GL_RGBA8;
        break;
    case 2:
        colourformat=GL_RGBA4;
        break;
    default:
        colourformat=GL_RGBA;
    }
#else
    colourformat = GL_RGBA;
#endif // !OPENGLES
    aspw=1.0;
    asph=1.0;
    switch(scaling) {
    case 1:
    case 3:
        div1=screenwidth/320;
        div2=screenheight/200;

        if (div2<div1) div1=div2;
        if (div1<1) {
            TO_DEBUG_LOG(
                "Warning: resolution must be 320x200 or more"
                " for integer scaling.\n");
            virtualscreenwidth=360;
            virtualscreenheight=240;
        } else {
            virtualscreenwidth=screenwidth/div1;
            virtualscreenheight=screenheight/div1;
            }
            if (scaling==3) {
            if (screenwidth*3>screenheight*4)
            aspw=((((double)screenwidth*3.0))/(((double)screenheight)*4.0));
        else
            asph=((((double)screenheight)*4.0))/(((double)screenwidth*3.0));
            }
            break;
    case 0:
        virtualscreenwidth=360;
        virtualscreenheight=240;
            break;
    default:
        if (screenwidth*3>screenheight*4) {
        aspw=((((double)screenwidth*3.0))/(((double)screenheight)*4.0));
            virtualscreenwidth=360.0*aspw;
            virtualscreenheight=240;
            } else {
        asph=((((double)screenheight)*4.0))/(((double)screenwidth*3.0));
            virtualscreenwidth=360;
            virtualscreenheight=240.0*asph;
            }
    }
}

void loadsettings(void) {
    char path[256];
#ifndef ANDROID_NDK
    snprintf(path, sizeof(path), "%s/settings.ini", globalDataDir);
#else
    snprintf(path, sizeof(path), "%s/settings.ini", globalAndroidRWdir);
#endif
    FILE *file=fopen(path,"r");
    int i,versflag,version;

#ifndef USE_SDL2
    SDL_Rect **modes;
#endif // !USE_SDL2

    channels=2; musicvolume=64; soundvolume=64; gammalevel=1.0;
#ifndef USE_SDL2
        modes=SDL_ListModes(NULL,SDL_FULLSCREEN);

        if ((modes!=NULL)&&(modes!=(SDL_Rect **)-1)&&(modes[0]!=NULL))
        resolutionnumber=modes[0]->w*10000+modes[0]->h;
#endif // !USE_SDL2
    i=0;

    for(i=0;i<numkeys;i++)
    newkeydefs[i]=newdefaultkey[i];

    for(i=0;i<numkeys;i++)
    buttondefs[i]=newdefaultbutton[i];

    for(i=0;i<numaxes;i++)
    axisdefs[i]=newdefaultaxis[i];

    if (lab3dversion) {
    newkeydefs[16]=SDLK_l;
    newkeydefs[13]=SDLK_s;
    }

    if (file==NULL)
    setup();
    i=fscanf(file,"%d",&versflag);
    if (versflag==-1) {
    i=fscanf(file,"%d",&version);
    i=fscanf(file,"%d",&inputdevice);
    } else {
    version=0;
    inputdevice=versflag;
    }
    //printf("%d %d\n",versflag,version);
    i=fscanf(file,"%d %d %d %d %d",&resolutionnumber,&fullscr,
         &nearest,&music,&sound); /* Non-existent set to defaults. */
    if (i==5) {
    for(i=0;i<numkeys;i++)
        if (fscanf(file,"%d\n",newkeydefs+i)!=1) break;

    if (version>0) {
        for(i=0;i<numkeys;i++)
        if (fscanf(file,"%d\n",buttondefs+i)!=1) break;
        for(i=0;i<numaxes;i++)
        if (fscanf(file,"%d\n",axisdefs+i)!=1) break;
    }
    }
    if (i>0) {
    i=fscanf(file,"%d %d\n",&soundvolume,&musicvolume);
    } else i=0;


    if (i==2) {
    i=fscanf(file,"%d\n",&cheat);
    } else i=0;

    if (i) i=fscanf(file,"%d\n",&channel);

    if (i) i=fscanf(file,"%f\n",&gammalevel);

    if (i) i=fscanf(file,"%d\n",&soundblock);

    if (i) i=fscanf(file,"%d\n",&timing); /* Left in case I put it back. */

    if (i) i=fscanf(file,"%d\n",&texturedepth);

    if (i) i=fscanf(file,"%d\n",&musicchannel);

    if (i) i=fscanf(file,"%d\n",&scaling);

    fclose(file);
}

void savesettings(void) {
    char path[256];
#ifndef ANDROID_NDK
    snprintf(path, sizeof(path), "%s/settings.ini", globalDataDir);
#else
    snprintf(path, sizeof(path), "%s/settings.ini", globalAndroidRWdir);
#endif // ANDROID_NDK
    FILE *file=fopen(path,"w");
    int i;

    if (file==NULL) return;
    fprintf(file,"-1 1\n");
    fprintf(file,"%d %d %d %d %d %d\n",inputdevice,resolutionnumber,fullscr,
        nearest,music,sound);
    for(i=0;i<numkeys;i++)
    fprintf(file,"%d\n",newkeydefs[i]);
    for(i=0;i<numkeys;i++)
    fprintf(file,"%d\n",buttondefs[i]);
    for(i=0;i<numaxes;i++)
    fprintf(file,"%d\n",axisdefs[i]);
    fprintf(file,"%d %d\n",soundvolume,musicvolume);
    fprintf(file,"%d\n",cheat);
    fprintf(file,"%d\n",channel);
    fprintf(file,"%f\n",gammalevel);
    fprintf(file,"%d\n",soundblock);
    fprintf(file,"%d\n",timing);
    fprintf(file,"%d\n",texturedepth);
    fprintf(file,"%d\n",musicchannel);
    fprintf(file,"%d\n",scaling);

    fclose(file);
}

void setup(void) {
    K_INT16 i, j, k, walcounter;
    K_UINT16 l;
    char *v;
#ifndef USE_SDL2
    SDL_Surface *screen, *icon;
#else
    SDL_Surface *icon;
#endif // !USE_SDL2

    configure();
    statusbaryoffset=250;

    /* Initialise SDL; uncomment the NOPARACHUTE bit if the parachute
       routine (which catches stuff like segfaults) gets in the way of your
       debugging. */

    /* Display accuracy not important in setup... */

    SDL_Init(SDL_INIT_TIMER|SDL_INIT_VIDEO/*|SDL_INIT_NOPARACHUTE*/|
         SDL_INIT_JOYSTICK);
    SDL_JoystickOpen(0);
    SDL_JoystickEventState(1);

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE,8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,24);

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE,0);
    SDL_GL_SetAttribute(SDL_GL_ACCUM_RED_SIZE,0);
    SDL_GL_SetAttribute(SDL_GL_ACCUM_GREEN_SIZE,0);
    SDL_GL_SetAttribute(SDL_GL_ACCUM_BLUE_SIZE,0);
    SDL_GL_SetAttribute(SDL_GL_ACCUM_ALPHA_SIZE,0);

#ifdef ANDROID_NDK
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1); // TODO: Check this.
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#endif // ANDROID_NDK

    SDL_ShowCursor(0);

#ifndef USE_SDL2
    SDL_EnableKeyRepeat(200,30);
#endif // !USE_SDL2

    TO_DEBUG_LOG("Activating video...\n");

    screenwidth=360; screenheight=240;

#ifndef ANDROID_NDK
    icon=SDL_LoadBMP("ken.bmp");
    if (icon==NULL) {
    TO_DEBUG_LOG("Warning: ken.bmp (icon file) not found.\n");
    }
#endif // !ANDROID_NDK

#ifndef USE_SDL2
    SDL_WM_SetIcon(icon,NULL);


    if ((screen=SDL_SetVideoMode(screenwidth, screenheight, 32,
                 SDL_OPENGL))==
    NULL) {
    TO_DEBUG_LOG("Video mode set failed.\n");
    SDL_Quit();
    exit(-1);
    }

    SDL_SetGamma(1.0,1.0,1.0); /* Zap gamma correction. */

    screenwidth=screen->w;
    screenheight=screen->h;
#else
    if ((globalWindow=SDL_CreateWindow("Ken's Labyrinth", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                       screenwidth, screenheight, SDL_WINDOW_OPENGL))==
            NULL) {
        TO_DEBUG_LOG("Video mode set failed: %s.\n", SDL_GetError());
        SDL_DestroyWindow(globalWindow);
        SDL_Quit();
        exit(-1);
    }

    // Create GL Context
//    if ((glContext=SDL_GL_CreateContext(globalWindow))==NULL) {
//        TO_DEBUG_LOG("Can't create GL Context: %s.\n", SDL_GetError());
//        SDL_GL_DeleteContext(glContext);
//        SDL_DestroyWindow(globalWindow);
//        SDL_Quit();
//        exit(-1);
//    }

#ifndef ANDROID_NDK
    // Calculate gamma ramp
    /* Zap gamma correction. */
    if (!gammaRamp) {
        gammaRamp = (Uint16 *)SDL_malloc(256 * sizeof(Uint16));
        SDL_CalculateGammaRamp(1.0, gammaRamp);

        if ((SDL_SetWindowGammaRamp(globalWindow, gammaRamp, gammaRamp, gammaRamp))==-1) {
            TO_DEBUG_LOG("setup.c: Can't set gamma ramp.\n");
        }
    } else {
        TO_DEBUG_LOG("Warning: gammaRump is null!\n");
    }
#endif // !ANDROID_NDK
    int _screen_w, _screen_h;
    SDL_GetWindowSize(globalWindow, &_screen_w, &_screen_h);

    screenwidth = _screen_w;
    screenheight = _screen_h;
#endif // !USE_SDL2

    virtualscreenwidth=360;
    virtualscreenheight=240;

    largescreentexture=0;

    if (largescreentexture) {
    /* One large 512x512 texture. */

    screenbufferwidth=screenbufferheight=512;
    } else {
    /* 6*11 matrix of 64x64 tiles with 1 pixel wide borders on shared
       edges. */

    screenbufferwidth=374;
    screenbufferheight=746;
    }

    screenbuffer=malloc(screenbufferwidth*screenbufferheight);
    screenbuffer32=malloc(screenbufferwidth*screenbufferheight*4);

#ifndef USE_SDL2
    SDL_WM_SetCaption("Ken's Labyrinth", "Ken's Labyrinth");
#else
#ifndef ANDROID_NDK
    SDL_SetWindowIcon(globalWindow, icon);
#endif // !ANDROID_NDK
#endif // !USE_SDL2

    linecompare(479);

    if (screenbuffer==NULL) {
    TO_DEBUG_LOG("Insufficient memory.\n");
    SDL_Quit();
    exit(-1);
    }

    TO_DEBUG_LOG("Loading configuration file...\n");

    loadtables();
    fullfilter=partialfilter=GL_NEAREST;
    vidmode = 1;
    mute = 0;
    moustat = 1;
    joystat = 1;

    musicsource=-1;
    speechstatus=0;

    TO_DEBUG_LOG("Allocating memory...\n");
    if (((lzwbuf = malloc(12304-8200)) == NULL)||
    ((lzwbuf2=malloc(8200))==NULL))
    {
    TO_DEBUG_LOG("Error #3: Memory allocation failed.\n");
    SDL_Quit();
    exit(-1);
    }

    convwalls = numwalls;

    if ((pic = malloc((numwalls-initialwalls)<<12)) == NULL)
    {
    TO_DEBUG_LOG(
        "Error #4: This computer does not have enough memory.\n");
    SDL_Quit();
    exit(-1);
    }
    walcounter = initialwalls;
    if (convwalls > initialwalls)
    {
    v = (char *)pic;
    for(i=0;i<convwalls-initialwalls;i++)
    {
        walseg[walcounter] = (unsigned char *)v;
        walcounter++;
        v += 4096;
    }
    }
    l = 0;
    for(i=0;i<240;i++)
    {
    times90[i] = l;
    l += 90;
    }
    less64inc[0] = 16384;
    for(i=1;i<64;i++)
    less64inc[i] = 16384 / i;
    for(i=0;i<256;i++)
    keystatus[i] = 0;

    if (largescreentexture) {
    glGenTextures(1,&screenbuffertexture);
    } else {
    glGenTextures(72,screenbuffertextures);
    }

    saidwelcome = 0;
    TO_DEBUG_LOG("Loading intro pictures...\n");

    if (lab3dversion) {
    kgif(-1);
    k=0;
    for(i=0;i<16;i++)
        for(j=1;j<17;j++)
        {
        spritepalette[k++] = (opaldef[i][0]*j)/17;
        spritepalette[k++] = (opaldef[i][1]*j)/17;
        spritepalette[k++] = (opaldef[i][2]*j)/17;
        }
    TO_DEBUG_LOG("Loading old graphics...\n");
    loadwalls(0);
    fade(63);
    k=0;
    for(i=0;i<16;i++)
        for(j=1;j<17;j++)
        {
        palette[k++] = (opaldef[i][0]*j)/17;
        palette[k++] = (opaldef[i][1]*j)/17;
        palette[k++] = (opaldef[i][2]*j)/17;
        }
    settransferpalette();
    strcpy(keynames[16], "LOAD game");
    strcpy(keynames[13], "SAVE game");

    } else {
    /* The ingame palette is stored in this GIF! */
    kgif(1);
    memcpy(spritepalette,palette,768);

    kgif(0);
    settransferpalette();
    TO_DEBUG_LOG("Loading graphics...\n");
    loadwalls(0);

    kgif(1);
    fade(63);
    }
#ifndef OPENGLES
    glDrawBuffer(GL_FRONT);
#endif // !OPENGLES

#ifdef ANDROID_NDK
    resolutionnumber=_screen_w*10000+_screen_h;
    soundblock=1;
#endif // ANDROID_NDK

    setupmenu();

    savesettings();

#ifdef USE_SDL2
#ifndef ANDROID_NDK
    TO_DEBUG_LOG("Delete gamma Ramp.\n");
    SDL_free(gammaRamp);
    gammaRamp = NULL;
#endif // !ANDROID_NDK
    TO_DEBUG_LOG("Delete GL contex.\n");
    //SDL_GL_DeleteContext(glContext);
    //glContext = NULL;
    TO_DEBUG_LOG("Destroy SDL Window.\n");
    SDL_DestroyWindow(globalWindow);
    globalWindow = NULL;
#endif // USE_SDL2

    SDL_Quit();
    exit(0);
}
