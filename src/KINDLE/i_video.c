/* Emacs style mode select   -*- C++ -*-
 *-----------------------------------------------------------------------------
 *
 *
 *  PrBoom: a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2006 by
 *  Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze
 *  Copyright 2005, 2006 by
 *  Florian Schulze, Colin Phipps, Neil Stevens, Andrey Budko
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 *  02111-1307, USA.
 *
 * DESCRIPTION:
 *  DOOM graphics stuff for SDL
 *
 *-----------------------------------------------------------------------------
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <math.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#endif


#include "m_argv.h"
#include "doomstat.h"
#include "doomdef.h"
#include "doomtype.h"
#include "v_video.h"
#include "r_draw.h"
#include "d_main.h"
#include "d_event.h"
#include "i_joy.h"
#include "i_video.h"
#include "z_zone.h"
#include "s_sound.h"
#include "sounds.h"
#include "w_wad.h"
#include "st_stuff.h"
#include "lprintf.h"

int gl_colorbuffer_bits=16;
int gl_depthbuffer_bits=16;

extern void M_QuitDOOM(int choice);
#ifdef DISABLE_DOUBLEBUFFER
int use_doublebuffer = 0;
#else
int use_doublebuffer = 1; // Included not to break m_misc, but not relevant to SDL
#endif
int use_fullscreen;
int desired_fullscreen;
static byte *screen;
static int fbFd;
static int kindleKeysFd;
static int kindleKeysFd2;

typedef struct {
  int truc1;
	int truc2;
	unsigned short truc3;
  unsigned short keyCode;
  int status;
} kindle_key_t;

static kindle_key_t kkey;

#define KINDLE_WIDTH 800
#define KINDLE_HEIGHT 600

/* Destination framebuffer geometry. Defaults are the Kindle 3 panel; they
 * are overwritten from FBIOGET_VSCREENINFO when the ioctl is available. */
static int fb_width = 600;
static int fb_height = 800;
static int fb_bpp = 4;
static int fb_line_length = 300;
static int fb_size = 240000;

/* 8-bit Doom palette index -> 4-bit grayscale value. */
static unsigned char palette_lut[256];
static int palette_lut_ready = 0;
static int invert_fb = 1;

/////////// FRAMEBUFFER STUFF
static void init_framebuffer(void) {
  struct fb_var_screeninfo vinfo;
  struct fb_fix_screeninfo finfo;

  fbFd = open("/dev/fb0", O_RDWR);
  if (fbFd == -1) {
    perror("open(/dev/fb0)");
    exit(1);
  }

  if (ioctl(fbFd, FBIOGET_VSCREENINFO, &vinfo) == 0) {
    fb_width = vinfo.xres;
    fb_height = vinfo.yres;
    fb_bpp = vinfo.bits_per_pixel;
  } else {
    lprintf(LO_INFO, "FBIOGET_VSCREENINFO failed, assuming Kindle 3 600x800x4\n");
  }

  if (ioctl(fbFd, FBIOGET_FSCREENINFO, &finfo) == 0 && finfo.line_length > 0) {
    fb_line_length = finfo.line_length;
  } else {
    fb_line_length = (fb_width * fb_bpp + 7) / 8;
  }
  fb_size = fb_line_length * fb_height;

  lprintf(LO_INFO, "framebuffer: %dx%d, %dbpp, stride %d, %d bytes\n",
          fb_width, fb_height, fb_bpp, fb_line_length, fb_size);
}

static void update_framebuffer(int full) {
  if (full) {
    ioctl(fbFd, 0x46db, 1);
  } else {
    ioctl(fbFd, 0x46db, 0);
  }
}

static void clear_framebuffer(void) {
  ioctl(fbFd, 0x46e1, 0);
}


////// KINDLE EVENTS

static void init_kindle_keys() {
  kindleKeysFd = open("/dev/input/event1", O_RDONLY|O_NONBLOCK);
  if (kindleKeysFd == -1) {
    perror("open(/dev/input/event1)");
    exit(1);
  }

  kindleKeysFd2 = open("/dev/input/event0", O_RDONLY|O_NONBLOCK);
  if (kindleKeysFd2 == -1) {
    perror("open(/dev/input/event0)");
    exit(1);
  }
}

#define KINDLE_READ_AGAIN 4

static int kindle_poll_keys(kindle_key_t *k) {
    int retval = read(kindleKeysFd, k, sizeof(kindle_key_t));
    if (retval == -1) {
      if (errno != EAGAIN) {
        perror("read()");
        exit(1);
      }
    } else if (retval > 0)  {
      return 1;
    }

  // This SHOULD NOT WORK: /dev/input/event0 doesn't have the same structure as event1
  // For some reason it matches for the HOME key, but it's a kludge and doesn't allow us to detect the other buttons
    retval = read(kindleKeysFd2, k, sizeof(kindle_key_t));
    if (retval == -1) {
      if (errno != EAGAIN) {
        perror("read()");
        exit(1);
      }
    } else if (k->keyCode == KINDLE_READ_AGAIN) {
      printf("Read again\n");
      read(kindleKeysFd2, k, sizeof(kindle_key_t));
      return 1;
    } else if (retval > 0)  {
      return 1;
    }

    return 0;
}

////////////////////////////////////////////////////////////////////////////
// Input code
int             leds_always_off = 0; // Expected by m_misc, not relevant

// Mouse handling
extern int     usemouse;        // config file var
static boolean mouse_enabled; // usemouse, but can be overriden by -nomouse
static boolean mouse_currently_grabbed;

/////////////////////////////////////////////////////////////////////////////////
// Keyboard handling

//
//  Translates the key currently in key
//

#define KINDLE_LEFT 105
#define KINDLE_RIGHT 106
#define KINDLE_UP 103
#define KINDLE_DOWN 108
#define KINDLE_OK 194
#define KINDLE_HOME 102
#define KINDLE_MENU 139
#define KINDLE_BACK 158

static int I_TranslateKey(unsigned short code)
{
  switch (code) {
    /* Fiveway. The game is rotated into landscape, so keep the original
     * rotated mapping used by this port. */
    case KINDLE_LEFT:  return KEYD_DOWNARROW;
    case KINDLE_RIGHT: return KEYD_UPARROW;
    case KINDLE_UP:    return KEYD_LEFTARROW;
    case KINDLE_DOWN:  return KEYD_RIGHTARROW;
    case KINDLE_OK:    return KEYD_RCTRL;
    case KINDLE_BACK:  return KEYD_ESCAPE;
    case KINDLE_MENU:  return KEYD_SPACEBAR;

    /* Kindle 3 physical keyboard (standard Linux key codes). */
    case 1:  return KEYD_ESCAPE;       /* Esc */
    case 14: return KEYD_BACKSPACE;    /* Del/backspace */
    case 28: return KEYD_ENTER;        /* Enter */
    case 29: return KEYD_RCTRL;        /* Ctrl: fire */
    case 42: return KEYD_RSHIFT;       /* Shift: run */
    case 56: return KEYD_RALT;         /* Alt: strafe modifier */
    case 57: return KEYD_SPACEBAR;     /* Space: use */
    case 17: return KEYD_UPARROW;      /* W: forward */
    case 31: return KEYD_DOWNARROW;    /* S: backward */
    case 30: return KEYD_LEFTARROW;    /* A: turn left */
    case 32: return KEYD_RIGHTARROW;   /* D: turn right */
    case 16: return ',';               /* Q: strafe left */
    case 18: return '.';               /* E: strafe right */
    case 2:  return '1';               /* weapon 1 */
    case 3:  return '2';
    case 4:  return '3';
    case 5:  return '4';
    case 6:  return '5';
    case 7:  return '6';
    case 8:  return '7';               /* weapon 7 */
    default: return 0;
  }
}

/////////////////////////////////////////////////////////////////////////////////
// Main input code

/* cph - pulled out common button code logic */
static int I_SDLtoDoomMouseState(int buttonstate)
{
  return 0;
}

static void I_GetEvent(kindle_key_t k)
{
  event_t event;

  if (k.keyCode == 102 && k.status == 1) {
    // Press HOME
    exit(0);
  }

  switch (k.status) {
  case 1: // SDL_KEYDOWN
    event.type = ev_keydown;
    event.data1 = I_TranslateKey(k.keyCode);
    D_PostEvent(&event);
    break;

  case 0: // SDL_KEYUP
  {
    event.type = ev_keyup;
    event.data1 = I_TranslateKey(k.keyCode);
    D_PostEvent(&event);
  }
  break;    

  default:
    break;
  }
}


//
// I_StartTic
//

void I_StartTic (void)
{
  while ( kindle_poll_keys(&kkey)) {
    /* truc3 is the input_event type. Only feed EV_KEY events (1). */
    if (kkey.truc3 == 1)
      I_GetEvent(kkey);
  }
}

//
// I_StartFrame
//
void I_StartFrame (void)
{
}

//
// I_InitInputs
//

static void I_InitInputs(void)
{
  init_kindle_keys();
}
/////////////////////////////////////////////////////////////////////////////

// I_SkipFrame
//
// Returns true if it thinks we can afford to skip this frame

inline static boolean I_SkipFrame(void)
{
  static int frameno;

  frameno++;
  switch (gamestate) {
  case GS_LEVEL:
    if (!paused)
      return false;
  default:
    // Skip odd frames
    return (frameno & 1) ? true : false;
  }
}

///////////////////////////////////////////////////////////
// Palette stuff.
//
static void I_UploadNewPalette(int pal)
{
}

//////////////////////////////////////////////////////////////////////////////
// Graphics API

void I_ShutdownGraphics(void)
{
}

//
// I_UpdateNoBlit
//
void I_UpdateNoBlit (void)
{
}

//
// I_FinishUpdate
//
static int newpal = 0;
#define NO_PALETTE_CHANGE 1000

void I_FinishUpdate (void)
{
  static int first_update = 1;
  const unsigned char *src;
  int x, y, col;
  int v0, v1;

  if (I_SkipFrame()) return;

  src = screens[0].data;

  if (fb_bpp == 4) {
    /* Kindle 3: 4bpp, two pixels per byte, high nibble first.
     * The game renders 800x600 landscape; the panel is 600x800 portrait. */
    for (x = 0; x < SCREENWIDTH && x < fb_height; x++) {
      unsigned char *row = screen + x * fb_line_length;
      for (col = 0; col < fb_width; col += 2) {
        int sy0 = fb_width - 1 - col;
        int sy1 = fb_width - 1 - (col + 1);
        if (sy0 >= 0 && sy0 < SCREENHEIGHT) {
          v0 = palette_lut_ready ? palette_lut[src[sy0 * SCREENWIDTH + x]] : (src[sy0 * SCREENWIDTH + x] >> 4);
          if (invert_fb) v0 = 15 - v0;
        } else {
          v0 = 0;
        }
        if (sy1 >= 0 && sy1 < SCREENHEIGHT) {
          v1 = palette_lut_ready ? palette_lut[src[sy1 * SCREENWIDTH + x]] : (src[sy1 * SCREENWIDTH + x] >> 4);
          if (invert_fb) v1 = 15 - v1;
        } else {
          v1 = 0;
        }
        row[col >> 1] = (unsigned char)((v0 << 4) | (v1 & 0x0f));
      }
    }
  } else {
    /* Kindle 4 and other 8bpp framebuffers: keep the original behaviour. */
    for (x = 0; x < SCREENWIDTH && x < fb_height; x++) {
      unsigned char *row = screen + x * fb_line_length;
      for (y = 0; y < SCREENHEIGHT && y < fb_width; y++) {
        col = fb_width - 1 - y;
        row[col] = (unsigned char)~src[y * SCREENWIDTH + x];
      }
    }
  }

  update_framebuffer(first_update);
  first_update = 0;
}

//
// I_ScreenShot - moved to i_sshot.c
//

//
// I_SetPalette
//
void I_SetPalette (int pal)
{
  const unsigned char *p;
  const char *g;
  float gamma = 1.0f;
  int i;

  newpal = pal;
  if (pal == NO_PALETTE_CHANGE)
    return;
  if (W_CheckNumForName("PLAYPAL") < 0)
    return;

  p = (const unsigned char *)W_CacheLumpName("PLAYPAL");
  if (!p)
    return;
  if (pal < 0 || pal > 13)
    pal = 0;
  p += pal * 768;

  g = getenv("PRBOOM_GAMMA");
  if (g && *g) {
    gamma = (float)atof(g);
    if (gamma < 0.5f) gamma = 0.5f;
    if (gamma > 3.0f) gamma = 3.0f;
  }

  for (i = 0; i < 256; i++) {
    int r = p[i*3];
    int gg = p[i*3+1];
    int b = p[i*3+2];
    int lum = (r*77 + gg*151 + b*28) >> 8;
    int v;

    if (gamma == 1.0f) {
      v = lum >> 4;
    } else {
      float f = (float)lum / 255.0f;
      f = (float)pow(f, gamma);
      v = (int)(f * 15.0f + 0.5f);
    }
    if (v < 0) v = 0;
    if (v > 15) v = 15;
    palette_lut[i] = (unsigned char)v;
  }

  W_UnlockLumpName("PLAYPAL");
  palette_lut_ready = 1;
}

// I_PreInitGraphics

static void I_ShutdownSDL(void)
{
  clear_framebuffer();
  close(fbFd);
  return;
}

void I_PreInitGraphics(void)
{
  int i;

  for (i = 0; i < 256; i++)
    palette_lut[i] = (unsigned char)(i >> 4);
  palette_lut_ready = 1;

  {
    const char *inv = getenv("PRBOOM_INVERT");
    if (inv)
      invert_fb = atoi(inv) != 0;
  }

  init_framebuffer();

 atexit(I_ShutdownSDL);
}

// e6y
// GLBoom use this function for trying to set the closest supported resolution if the requested mode can't be set correctly.
// For example glboom.exe -geom 1025x768 -nowindow will set 1024x768.
// It should be used only for fullscreen modes.
static void I_ClosestResolution (int *width, int *height, int flags)
{
  *width=KINDLE_WIDTH;
  *height=KINDLE_HEIGHT;
}  

// CPhipps -
// I_CalculateRes
// Calculates the screen resolution, possibly using the supplied guide
void I_CalculateRes(unsigned int width, unsigned int height)
{
  // e6y: how about 1680x1050?
  /*
  SCREENWIDTH = (width+3) & ~3;
  SCREENHEIGHT = (height+3) & ~3;
  */

// e6y
// GLBoom will try to set the closest supported resolution 
// if the requested mode can't be set correctly.
// For example glboom.exe -geom 1025x768 -nowindow will set 1024x768.
// It affects only fullscreen modes.
  if (V_GetMode() == VID_MODEGL) {
    if ( desired_fullscreen )
    {
      I_ClosestResolution(&width, &height, 0);
    }
    SCREENWIDTH = width;
    SCREENHEIGHT = height;
    SCREENPITCH = SCREENWIDTH;
  } else {
    SCREENWIDTH = KINDLE_WIDTH;
    SCREENHEIGHT = KINDLE_HEIGHT;
    if (!(SCREENWIDTH % 1024)) {
      SCREENPITCH = SCREENWIDTH*V_GetPixelDepth()+32;
    } else {
      SCREENPITCH = SCREENWIDTH*V_GetPixelDepth();
    }
  }
}

// CPhipps -
// I_SetRes
// Sets the screen resolution
void I_SetRes(void)
{
  int i;

  I_CalculateRes(SCREENWIDTH, SCREENHEIGHT);

  // set first three to standard values
  for (i=0; i<3; i++) {
    screens[i].width = SCREENWIDTH;
    screens[i].height = SCREENHEIGHT;
    screens[i].byte_pitch = SCREENPITCH;
    screens[i].short_pitch = SCREENPITCH / V_GetModePixelDepth(VID_MODE16);
    screens[i].int_pitch = SCREENPITCH / V_GetModePixelDepth(VID_MODE32);
  }

  // statusbar
  screens[4].width = SCREENWIDTH;
  screens[4].height = (ST_SCALED_HEIGHT+1);
  screens[4].byte_pitch = SCREENPITCH;
  screens[4].short_pitch = SCREENPITCH / V_GetModePixelDepth(VID_MODE16);
  screens[4].int_pitch = SCREENPITCH / V_GetModePixelDepth(VID_MODE32);

  lprintf(LO_INFO,"I_SetRes: Using resolution %dx%d\n", SCREENWIDTH, SCREENHEIGHT);
}

void I_InitGraphics(void)
{
  char titlebuffer[2048];
  static int    firsttime=1;

  if (firsttime)
  {
    firsttime = 0;

   atexit(I_ShutdownGraphics);
    lprintf(LO_INFO, "I_InitGraphics: %dx%d\n", SCREENWIDTH, SCREENHEIGHT);

    /* Set the video mode */
    I_UpdateVideoMode();

    /* Setup the window title */
    strcpy(titlebuffer,PACKAGE);
    strcat(titlebuffer," ");
    strcat(titlebuffer,VERSION);
    printf("Title: %s\n", titlebuffer);

    /* Initialize the input system */
    I_InitInputs();
  }
}

int I_GetModeFromString(const char *modestr)
{
  video_mode_t mode;


  mode = VID_MODE8;

  return mode;
}

void I_UpdateVideoMode(void)
{
  int init_flags;
  int i;
  video_mode_t mode;

  lprintf(LO_INFO, "I_UpdateVideoMode: %dx%d (%s)\n", SCREENWIDTH, SCREENHEIGHT, desired_fullscreen ? "fullscreen" : "nofullscreen");

  mode = I_GetModeFromString(default_videomode);
  if ((i=M_CheckParm("-vidmode")) && i<myargc-1) {
    mode = I_GetModeFromString(myargv[i+1]);
  }

  V_InitMode(mode);
  V_DestroyUnusedTrueColorPalettes();
  V_FreeScreens();

  I_SetRes();


  screen = mmap(NULL, fb_size, PROT_READ|PROT_WRITE, MAP_SHARED, fbFd, 0);

  if(screen == MAP_FAILED) {
    screen = NULL;
    perror("mmap(/dev/fb0)");
    I_Error("Couldn't set %dx%d video mode", SCREENWIDTH, SCREENHEIGHT);
  }

  lprintf(LO_INFO, "I_UpdateVideoMode: 0x%x, %s, %s\n", init_flags, "", "");

  mouse_currently_grabbed = false;

  // Get the info needed to render to the display

    byte* screen_data = (byte*)malloc(SCREENWIDTH * SCREENHEIGHT);
    screens[0].not_on_heap = true;
    screens[0].data = (unsigned char *) (screen_data);
    screens[0].byte_pitch = SCREENWIDTH;
    screens[0].short_pitch = SCREENWIDTH; // screen->pitch / V_GetModePixelDepth(VID_MODE16);
    screens[0].int_pitch = SCREENWIDTH; // screen->pitch / V_GetModePixelDepth(VID_MODE32);


  V_AllocScreens();

  // Hide pointer while over this window
  //SDL_ShowCursor(0);

  R_InitBuffer(SCREENWIDTH, SCREENHEIGHT);

}
