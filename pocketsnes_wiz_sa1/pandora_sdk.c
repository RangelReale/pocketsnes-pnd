#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include <linux/fb.h>
#include <linux/input.h>
#include <pthread.h>
#include "menu.h"
#include "pandora_sdk.h"
/*#include "squidgehack.h"*/
#include <time.h>

#ifndef FBIO_WAITFORVSYNC
#define FBIO_WAITFORVSYNC _IOW('F', 0x20, __u32)
#endif

#define SYS_CLK_FREQ 7372800

volatile unsigned short  gp2x_palette[512][2];

static int fb_size = 0;
//static int mmuHackStatus=0;

//unsigned long gp2x_ticks_per_second=7372800/1000;
unsigned long   gp2x_dev[5]={0,0,0,0,0};
//unsigned long gp2x_physvram[4]={0,0,0,0};

unsigned long fb_dev = -1;
struct fb_var_screeninfo vinfo;
struct fb_fix_screeninfo finfo;
int snd_dev = 0;
int mix_dev = 0;
int key_dev[2];
int key_devs = 0;

unsigned short *framebuffer16[4]={0,0,0,0};
static void *framebuffer_mmap;
unsigned short *gp2x_logvram15[2], gp2x_sound_buffer[4+((44100*2)*8)]; //*2=stereo, *4=max buffers
volatile unsigned short *gp2x_memregs;
volatile unsigned long *gp2x_memregl;
volatile unsigned long *gp2x_blitter = NULL;
unsigned int *gp2x_intVectors;
unsigned char  *framebuffer8[4], *gp2x_screen8prev, *gp2x_logvram8[2];

volatile short *pOutput[8];
int InitFramebuffer=0;
int Timer=0;
volatile int SoundThreadFlag=0;
volatile int CurrentSoundBank=0;
int CurrentFrameBuffer=0;
int CurrentFrag=0;
unsigned int ExistingIntHandler;

// 1024x8   8x8 font, i love it :)
const unsigned int font8x8[]= {0x0,0x0,0xc3663c18,0x3c2424e7,0xe724243c,0x183c66c3,0xc16f3818,0x18386fc1,0x83f61c18,0x181cf683,0xe7c3993c,0x3c99c3,0x3f7fffff,0xe7cf9f,0x3c99c3e7,0xe7c399,0x3160c080,0x40e1b,0xcbcbc37e,0x7ec3c3db,0x3c3c3c18,0x81c087e,0x8683818,0x60f0e08,0x81422418,0x18244281,0xbd5a2418,0x18245abd,0x818181ff,0xff8181,0xa1c181ff,0xff8995,0x63633e,0x3e6363,0x606060,0x606060,0x3e60603e,0x3e0303,0x3e60603e,0x3e6060,0x3e636363,0x606060,0x3e03033e,0x3e6060,0x3e03033e,0x3e6363,0x60603e,0x606060,0x3e63633e,0x3e6363,0x3e63633e,0x3e6060,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x18181818,0x180018,0x666666,0x0,0x367f3600,0x367f36,0x3c067c18,0x183e60,0x18366600,0x62660c,0xe1c361c,0x6e337b,0x181818,0x0,0x18183870,0x703818,0x18181c0e,0xe1c18,0xff3c6600,0x663c,0x7e181800,0x1818,0x0,0x60c0c00,0x7e000000,0x0,0x0,0x181800,0x18306040,0x2060c,0x6e76663c,0x3c6666,0x18181c18,0x7e1818,0x3060663c,0x7e0c18,0x3018307e,0x3c6660,0x363c3830,0x30307e,0x603e067e,0x3c6660,0x3e06063c,0x3c6666,0x1830607e,0xc0c0c,0x3c66663c,0x3c6666,0x7c66663c,0x1c3060,0x181800,0x1818,0x181800,0xc1818,0xc183060,0x603018,0x7e0000,0x7e00,0x30180c06,0x60c18,0x3060663c,0x180018,0x5676663c,0x7c0676,0x66663c18,0x66667e,0x3e66663e,0x3e6666,0x606663c,0x3c6606,0x6666361e,0x1e3666,0x3e06067e,0x7e0606,0x3e06067e,0x60606,0x7606067c,0x7c6666,0x7e666666,0x666666,0x1818183c,0x3c1818,0x60606060,0x3c6660,0xe1e3666,0x66361e,0x6060606,0x7e0606,0x6b7f7763,0x636363,0x7e7e6e66,0x666676,0x6666663c,0x3c6666,0x3e66663e,0x60606,0x6666663c,0x6c366e,0x3e66663e,0x666636,0x3c06663c,0x3c6660,0x1818187e,0x181818,0x66666666,0x7c6666,0x66666666,0x183c66,0x6b636363,0x63777f,0x183c6666,0x66663c,0x3c666666,0x181818,0x1830607e,0x7e060c,0x18181878,0x781818,0x180c0602,0x406030,0x1818181e,0x1e1818,0x63361c08,0x0,0x0,0x7f0000,0xc060300,0x0,0x603c0000,0x7c667c,0x663e0606,0x3e6666,0x63c0000,0x3c0606,0x667c6060,0x7c6666,0x663c0000,0x3c067e,0xc3e0c38,0xc0c0c,0x667c0000,0x3e607c66,0x663e0606,0x666666,0x181c0018,0x3c1818,0x18180018,0xe181818,0x36660606,0x66361e,0x1818181c,0x3c1818,0x7f370000,0x63636b,0x663e0000,0x666666,0x663c0000,0x3c6666,0x663e0000,0x63e6666,0x667c0000,0x607c6666,0x663e0000,0x60606,0x67c0000,0x3e603c,0x187e1800,0x701818,0x66660000,0x7c6666,0x66660000,0x183c66,0x63630000,0x363e6b,0x3c660000,0x663c18,0x66660000,0x3e607c66,0x307e0000,0x7e0c18,0xc181870,0x701818,0x18181818,0x18181818,0x3018180e,0xe1818,0x794f0600,0x30};

pthread_t       gp2x_sound_thread=0, gp2x_sound_thread_exit=0;

int altVolumeCtrl = 0;

/* 
########################
Graphics functions
########################
 */
unsigned short gpi_getbppmode()
{
	return vinfo.bits_per_pixel;
}



static void debug(char *text, int pause)
{
	unsigned short bppmode = gpi_getbppmode();
	//bppmode=gp2x_memregs[0x28DA>>1];
	//bppmode>>=9;
	//bppmode<<=3;
	
	if(bppmode==8)
	{
		gp_clearFramebuffer8(framebuffer8[currFB],0);
		gp_drawString(0,0,strlen(text),text,0x51,framebuffer16[currFB]);
	}
	else
	{
		gp_clearFramebuffer16(framebuffer16[currFB],0);
		gp_drawString(0,0,strlen(text),text,(unsigned short)MENU_RGB(31,31,31),framebuffer16[currFB]);
	}
	MenuFlip();
	if(pause)	MenuPause();

}
static __inline__
void gp_drawPixel8 ( int x, int y, unsigned char c, unsigned char *framebuffer ) 
{
	*(framebuffer +(320*y)+x ) = c;
}

static __inline__
void gp_drawPixel16 ( int x, int y, unsigned short c, unsigned short *framebuffer ) 
{
	*(framebuffer +(320*y)+x ) = c;
}

static
void set_char8x8_16bpp (int xx,int yy,int offset,unsigned short mode,unsigned short *framebuffer) 
{
	unsigned int y, pixel;
	offset *= 2;
	pixel = font8x8[0 + offset];
	for (y = 0; y < 4; y++) 
	{
		if (pixel&(1<<(0+(y<<3)))) gp_drawPixel16(xx+0, yy+y, mode, framebuffer);
		if (pixel&(1<<(1+(y<<3)))) gp_drawPixel16(xx+1, yy+y, mode, framebuffer);
		if (pixel&(1<<(2+(y<<3)))) gp_drawPixel16(xx+2, yy+y, mode, framebuffer);
		if (pixel&(1<<(3+(y<<3)))) gp_drawPixel16(xx+3, yy+y, mode, framebuffer);
		if (pixel&(1<<(4+(y<<3)))) gp_drawPixel16(xx+4, yy+y, mode, framebuffer);
		if (pixel&(1<<(5+(y<<3)))) gp_drawPixel16(xx+5, yy+y, mode, framebuffer);
		if (pixel&(1<<(6+(y<<3)))) gp_drawPixel16(xx+6, yy+y, mode, framebuffer);
		if (pixel&(1<<(7+(y<<3)))) gp_drawPixel16(xx+7, yy+y, mode, framebuffer);
	}
	pixel = font8x8[1 + offset];
	for (y = 0; y < 4; y++) 
	{
		if (pixel&(1<<(0+(y<<3)))) gp_drawPixel16(xx+0, yy+y+4, mode, framebuffer);
		if (pixel&(1<<(1+(y<<3)))) gp_drawPixel16(xx+1, yy+y+4, mode, framebuffer);
		if (pixel&(1<<(2+(y<<3)))) gp_drawPixel16(xx+2, yy+y+4, mode, framebuffer);
		if (pixel&(1<<(3+(y<<3)))) gp_drawPixel16(xx+3, yy+y+4, mode, framebuffer);
		if (pixel&(1<<(4+(y<<3)))) gp_drawPixel16(xx+4, yy+y+4, mode, framebuffer);
		if (pixel&(1<<(5+(y<<3)))) gp_drawPixel16(xx+5, yy+y+4, mode, framebuffer);
		if (pixel&(1<<(6+(y<<3)))) gp_drawPixel16(xx+6, yy+y+4, mode, framebuffer);
		if (pixel&(1<<(7+(y<<3)))) gp_drawPixel16(xx+7, yy+y+4, mode, framebuffer);

	}
}

static
void set_char8x8_8bpp (int xx,int yy,int offset,unsigned char mode,unsigned char *framebuffer) 
{
	unsigned int y, pixel;
	offset *= 2;
	pixel = font8x8[0 + offset];
	for (y = 0; y < 4; y++) 
	{
		if (pixel&(1<<(0+(y<<3)))) gp_drawPixel8(xx+0, yy+y, mode, framebuffer);
		if (pixel&(1<<(1+(y<<3)))) gp_drawPixel8(xx+1, yy+y, mode, framebuffer);
		if (pixel&(1<<(2+(y<<3)))) gp_drawPixel8(xx+2, yy+y, mode, framebuffer);
		if (pixel&(1<<(3+(y<<3)))) gp_drawPixel8(xx+3, yy+y, mode, framebuffer);
		if (pixel&(1<<(4+(y<<3)))) gp_drawPixel8(xx+4, yy+y, mode, framebuffer);
		if (pixel&(1<<(5+(y<<3)))) gp_drawPixel8(xx+5, yy+y, mode, framebuffer);
		if (pixel&(1<<(6+(y<<3)))) gp_drawPixel8(xx+6, yy+y, mode, framebuffer);
		if (pixel&(1<<(7+(y<<3)))) gp_drawPixel8(xx+7, yy+y, mode, framebuffer);
	}
	pixel = font8x8[1 + offset];
	for (y = 0; y < 4; y++) 
	{
		if (pixel&(1<<(0+(y<<3)))) gp_drawPixel8(xx+0, yy+y+4, mode, framebuffer);
		if (pixel&(1<<(1+(y<<3)))) gp_drawPixel8(xx+1, yy+y+4, mode, framebuffer);
		if (pixel&(1<<(2+(y<<3)))) gp_drawPixel8(xx+2, yy+y+4, mode, framebuffer);
		if (pixel&(1<<(3+(y<<3)))) gp_drawPixel8(xx+3, yy+y+4, mode, framebuffer);
		if (pixel&(1<<(4+(y<<3)))) gp_drawPixel8(xx+4, yy+y+4, mode, framebuffer);
		if (pixel&(1<<(5+(y<<3)))) gp_drawPixel8(xx+5, yy+y+4, mode, framebuffer);
		if (pixel&(1<<(6+(y<<3)))) gp_drawPixel8(xx+6, yy+y+4, mode, framebuffer);
		if (pixel&(1<<(7+(y<<3)))) gp_drawPixel8(xx+7, yy+y+4, mode, framebuffer);

	}
}

void gp_drawString (int x,int y,int len,char *buffer,unsigned short color,void *framebuffer) 
{
	int l,base=0;
	unsigned short bppmode = gpi_getbppmode();
	//bppmode=gp2x_memregs[0x28DA>>1];


	//bppmode>>=9;
	//bppmode<<=3;

		for (l=0;l<len;l++) 
		{
			if (bppmode==8)
			{
				set_char8x8_8bpp (x+base,y,buffer[l],color,(unsigned char*)framebuffer);
			}
			else 
			{
				set_char8x8_16bpp (x+base,y,buffer[l],color,(short unsigned int*)framebuffer);
			}
			base+=8;
		}
}

void gp_clearFramebuffer16(unsigned short *framebuffer, unsigned short pal)
{
	int x,y;
	for (y=0;y<240;y++)
	{
		for (x=0;x<320;x++)
		{
			*framebuffer++ = pal;
		}
	}
}

void gp_clearFramebuffer8(unsigned char *framebuffer, unsigned char pal)
{
	int x,y;
	for (y=0;y<240;y++)
	{
		for (x=0;x<320;x++)
		{
			*framebuffer++ = pal;
		}
	}
}

void gp_clearFramebuffer(void *framebuffer, unsigned int pal)
{
	unsigned short bppmode;
	bppmode=gp2x_memregs[0x28DA>>1];
	bppmode>>=9;
	bppmode<<=3;
	if(bppmode==8) 	gp_clearFramebuffer8((unsigned char*)framebuffer,(unsigned char)pal);
	else 					gp_clearFramebuffer16((unsigned short*)framebuffer,(unsigned short)pal);
}




unsigned int gp_getButton(unsigned char enable_diagnals)
{
#define NUM_INPUT_EVENTS 64
	static unsigned int ret = 0;
	struct input_event ev[NUM_INPUT_EVENTS];
	int nread = 0;
	int i = 0;
	int dev = 0;

	// Read events from the key devices
	for( dev = 0 ; dev < key_devs ; dev++ ) {
		nread = read( key_dev[dev], ev, sizeof(struct input_event) * NUM_INPUT_EVENTS );
		if( nread >= (int)sizeof(struct input_event) ) {
			for( i = 0 ; i < nread / sizeof(struct input_event) ; i++ ) {
				if ( ev[i].type == EV_KEY ) {
					if( ev[i].value == 0 ) {
						// Key up
						switch( ev[i].code ) {
							case KEY_UP:			ret &= ~(1<<INP_BUTTON_UP); break;
							case KEY_DOWN:			ret &= ~(1<<INP_BUTTON_DOWN); break;
							case KEY_LEFT:			ret &= ~(1<<INP_BUTTON_LEFT); break;
							case KEY_RIGHT:			ret &= ~(1<<INP_BUTTON_RIGHT); break;
							case KEY_LEFTALT:		ret &= ~(1<<INP_BUTTON_START); break;
							case KEY_LEFTCTRL:		ret &= ~(1<<INP_BUTTON_SELECT); break;
							case KEY_RIGHTSHIFT:	ret &= ~(1<<INP_BUTTON_L); break;
							case KEY_RIGHTCTRL: 	ret &= ~(1<<INP_BUTTON_R); break;
							case KEY_END: 			ret &= ~(1<<INP_BUTTON_A); break;
							case KEY_PAGEDOWN:		ret &= ~(1<<INP_BUTTON_B); break;
							case KEY_PAGEUP:		ret &= ~(1<<INP_BUTTON_X); break;
							case KEY_HOME:			ret &= ~(1<<INP_BUTTON_Y); break;

							case KEY_MENU:			ret &= ~(1<<INP_BUTTON_MENU); break;
							case KEY_SPACE:			ret &= ~(1<<INP_BUTTON_MENU); break;							

							case KEY_1: ret &= ~(1<<INP_BUTTON_VOL_UP); break;
							case KEY_2: ret &= ~(1<<INP_BUTTON_VOL_DOWN); break;
							default: break;
						}
					}
					else if( ev[i].value == 1 ) {
						// Key down
						switch( ev[i].code ) {
							case KEY_UP:			ret |= 1<<INP_BUTTON_UP; break;
							case KEY_DOWN:			ret |= 1<<INP_BUTTON_DOWN; break;
							case KEY_LEFT:			ret |= 1<<INP_BUTTON_LEFT; break;
							case KEY_RIGHT:			ret |= 1<<INP_BUTTON_RIGHT; break;
							case KEY_LEFTALT:		ret |= 1<<INP_BUTTON_START; break;
							case KEY_LEFTCTRL:		ret |= 1<<INP_BUTTON_SELECT; break;
							case KEY_RIGHTSHIFT:	ret |= 1<<INP_BUTTON_L; break;
							case KEY_RIGHTCTRL: 	ret |= 1<<INP_BUTTON_R; break;
							case KEY_END: 			ret |= 1<<INP_BUTTON_A; break;
							case KEY_PAGEDOWN:		ret |= 1<<INP_BUTTON_B; break;
							case KEY_PAGEUP:		ret |= 1<<INP_BUTTON_X; break;
							case KEY_HOME:			ret |= 1<<INP_BUTTON_Y; break;

							case KEY_MENU:			ret |= 1<<INP_BUTTON_MENU; break;
							case KEY_SPACE:			ret |= 1<<INP_BUTTON_MENU; break;
							
							case KEY_1: ret |= 1<<INP_BUTTON_VOL_UP; break;
							case KEY_2: ret |= 1<<INP_BUTTON_VOL_DOWN; break;
							default: break;
						}
					}
				}
			}
		}
	}

	return ret;
}

void gp_initGraphics(unsigned short bpp, int flip, int applyMmuHack)
{
	int x = 0;
	unsigned int key = 0;
	unsigned int offset = 0;
	char buf[256];
	int i = 0;
	int fd = -1;

#ifdef DEBUG
    printf("Entering gp_initGraphics....\r\n");
#endif    
	/*
	First check that frame buffer memory has not already been setup
	*/
	if (!InitFramebuffer)
	{
#ifdef DEBUG
		sprintf(buf, "Initing buffer\r\n");
		printf(buf);
#endif 
		fb_dev = open("/dev/fb1", O_RDWR);
		if(fb_dev < 0) {
			printf("Could not open frame buffer device\n");
			return;
		}
		
#ifdef DEBUG
		sprintf(buf, "Devices opened\r\n");
		printf(buf);
		sprintf(buf, "/dev/fb1: %x \r\n", fb_dev);
		printf(buf);
#endif 		

		// Get fixed screen information
		if (ioctl(fb_dev, FBIOGET_FSCREENINFO, &finfo)) {
			printf("Error reading fixed information.\n");
			return;
		}

		// Get variable screen information
		if (ioctl(fb_dev, FBIOGET_VSCREENINFO, &vinfo)) {
			printf("Error reading variable information.\n");
			return;
		}

		// Figure out the size of one buffer in bytes
		fb_size = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;
#ifdef DEBUG
		sprintf(buf, "FB: %dx%d @ %dbpp = %d bytes (%d for 4 screens)\n", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel, fb_size, fb_size*4);
		printf(buf);
#endif

		if (!framebuffer_mmap) framebuffer_mmap = mmap(0, fb_size*4, PROT_READ|PROT_WRITE, MAP_SHARED, fb_dev, 0 );
	
		// Clear the frame buffers
		memset(framebuffer_mmap,0,fb_size);

		framebuffer16[0]=(unsigned short*)(framebuffer_mmap);
		framebuffer16[1]=(unsigned short*)(framebuffer_mmap + (fb_size) );
		framebuffer16[2]=(unsigned short*)(framebuffer_mmap + (fb_size*2) );
		framebuffer16[3]=(unsigned short*)(framebuffer_mmap + (fb_size*3) );

#ifdef DEBUG
		{
			int i = 0;
			for( i = 0 ; i < 4 ; i++ ) {
				sprintf(buf, "FB%d @ 0x%x ", i, (void*)framebuffer16[i] );
				printf(buf);
				sprintf(buf, "yoffset = %d\n", vinfo.yres * i );
				printf(buf);
			}
		}
#endif		
	
		InitFramebuffer=1;
	}
	
	gp_setFramebuffer(flip,1);

	// Open devices to capture keyboard events
	if( key_devs == 0 ) {
		for( i = 0; 1 ; i++ ) {
			sprintf( buf, "/dev/input/event%i", i );

			fd = open( buf, O_RDONLY|O_NONBLOCK );
			if( fd < 0 ) {
				// No more devices
				break;
			}

			ioctl( fd, EVIOCGNAME(sizeof(buf)), buf );
			if( strcmp( buf, "keypad" ) == 0 || strcmp( buf, "gpio-keys" ) == 0 ) {
#ifdef DEBUG
				printf("Key device: /dev/input/event%i\n", i);
#endif
				key_dev[key_devs++] = fd;
			}
			else {
#ifdef DEBUG
				printf("Skipping: /dev/input/event%i (%s)\n", i, buf);
#endif
				close( fd );
			}
		}
	}

#ifdef DEBUG
    printf("Leaving gp_initGraphics....\r\n");
#endif
}

void gp_setFramebuffer(int flip, int sync)
{
	int fbarg = 0;

	ioctl(fb_dev, FBIOGET_VSCREENINFO, &vinfo);
	vinfo.yoffset = vinfo.yres * flip;

	if( sync )
		ioctl(fb_dev, FBIO_WAITFORVSYNC, &fbarg);

	ioctl(fb_dev, FBIOPAN_DISPLAY, &vinfo);

	CurrentFrameBuffer = flip;
}

void gp2x_video_setpalette(void) 
{ 
  //unsigned short *g=(unsigned short *)gp2x_palette; int i=512; 
  //gp2x_memregs[0x2958>>1]=0;                                                      
  //while(i--) gp2x_memregs[0x295A>>1]=*g++; 
} 

/* 
########################
Sound functions
########################
 */
static
void *gp2x_sound_play(void)
{
	struct timespec ts; ts.tv_sec=0, ts.tv_nsec=1000;
	while(! gp2x_sound_thread_exit)
	{
		Timer++;
		CurrentSoundBank++;

		if (CurrentSoundBank >= 8) CurrentSoundBank = 0;
		
		if (SoundThreadFlag==SOUND_THREAD_SOUND_ON)
		{
			write(snd_dev, (void *)pOutput[CurrentSoundBank], gp2x_sound_buffer[1]);
			//ioctl(snd_dev, SOUND_PCM_SYNC, 0); 
			ts.tv_sec=0, ts.tv_nsec=(gp2x_sound_buffer[3]<<16)|gp2x_sound_buffer[2];
			ts.tv_nsec-=800000; // pandora hack fix
			nanosleep(&ts, NULL);
		}
		else
		{
			write(snd_dev, (void *)&gp2x_sound_buffer[4], gp2x_sound_buffer[1]);
			//ioctl(snd_dev, SOUND_PCM_SYNC, 0); 
			ts.tv_sec=0, ts.tv_nsec=(gp2x_sound_buffer[3]<<16)|gp2x_sound_buffer[2];
			ts.tv_nsec-=800000; // pandora hack fix
			nanosleep(&ts, NULL);
		}
	}
 
	return NULL;
}

void gp_sound_volume(int l, int r) 
{ 
	if(!mix_dev)
	{
		mix_dev = open("/dev/mixer", O_WRONLY); 
	}

	l=((l<<8)|r);
	ioctl(mix_dev, SOUND_MIXER_WRITE_PCM, &l);
} 

unsigned long gp_timer_read(void)
{
  // Once again another peice of direct hardware access bites the dust
  // the code below is broken in firmware 2.1.1 so I've replaced it with a
  // to a linux function which seems to work
  //return gp2x_memregl[0x0A00>>2]/gp2x_ticks_per_second;
  struct timeval tval; // timing
  
  gettimeofday(&tval, 0);
  //tval.tv_usec
  //tval.tv_sec
  return (tval.tv_sec*1000000)+tval.tv_usec;
}

int gp_initSound(int rate, int bits, int stereo, int Hz, int frag)
{
	int status;
	int i=0;
	int nonblocking=1;
	void *bufferStart=NULL;
	int result;
	char text[256];
	
	printf("Init sound: %d %d %d %d %x\n",rate, bits, stereo, Hz, frag);

	//int frag=0x00020010;   // double buffer - frag size = 1<<0xf = 32768
	//frag=0x00020010;

	//8 = 256				= 2 fps loss			= good sound
	//9 = 512				= 1 fps loss			= good sound
	//A = 1024				= 
	//f = 32768				= 0 fps loss			= bad sound
	if ((frag!= CurrentFrag)&&(snd_dev!=0))
	{
		// Different frag config required
		// close device in order to re-adjust
		close(snd_dev);
		snd_dev=0;
	}

	if (snd_dev==0)
	{
		snd_dev = open("/dev/dsp", O_WRONLY);
		printf("Opening sound device: %x\r\n",snd_dev);
		ioctl(snd_dev, SNDCTL_DSP_SETFRAGMENT, &frag);
		CurrentFrag=frag; // save frag config
	}

	result=ioctl(snd_dev, SNDCTL_DSP_SPEED,  &rate);
	if(result==-1)
	{
		debug("Error setting DSP Speed",1);
		return(-1);
	}
	
	result=ioctl(snd_dev, SNDCTL_DSP_SETFMT,  &bits);
	if(result==-1)
	{
		debug("Error setting DSP format",1);
		return(-1);
	}

	result=ioctl(snd_dev, SNDCTL_DSP_STEREO,  &stereo);
	if(result==-1)
	{
		debug("Error setting DSP format",1);
		return(-1);
	}
	//printf("Disable Blocking: %x\r\n",ioctl(wiz_dev[3], 0x5421, &nonblocking));
	
	gp2x_sound_buffer[1]=(gp2x_sound_buffer[0]=(rate/Hz)) << (stereo + (bits==16));
	//gp2x_sound_buffer[1]=gp2x_sound_buffer[1] / 2;
	gp2x_sound_buffer[2]=(1000000000/Hz)&0xFFFF;
	gp2x_sound_buffer[3]=(1000000000/Hz)>>16;
 
	bufferStart= &gp2x_sound_buffer[4];
	pOutput[0] = (short*)bufferStart+(0*gp2x_sound_buffer[1]);
	pOutput[1] = (short*)bufferStart+(1*gp2x_sound_buffer[1]);
	pOutput[2] = (short*)bufferStart+(2*gp2x_sound_buffer[1]);
	pOutput[3] = (short*)bufferStart+(3*gp2x_sound_buffer[1]);
	pOutput[4] = (short*)bufferStart+(4*gp2x_sound_buffer[1]);
	pOutput[5] = (short*)bufferStart+(5*gp2x_sound_buffer[1]);
	pOutput[6] = (short*)bufferStart+(6*gp2x_sound_buffer[1]);
	pOutput[7] = (short*)bufferStart+(7*gp2x_sound_buffer[1]);

	printf("gp2x_sound_buffer: %d\n", sizeof(gp2x_sound_buffer));
	printf("gp2x_sound_buffer[0]: %d\n", gp2x_sound_buffer[0]);
	printf("gp2x_sound_buffer[1]: %d\n", gp2x_sound_buffer[1]);
	printf("gp2x_sound_buffer[2]: %d\n", gp2x_sound_buffer[2]);
	printf("gp2x_sound_buffer[3]: %d\n", gp2x_sound_buffer[3]);

	if(!gp2x_sound_thread) 
	{ 
		pthread_create( &gp2x_sound_thread, NULL, &gp2x_sound_play, NULL);
		//atexit(gp_Reset); 
	}
	
	for(i=0;i<(gp2x_sound_buffer[1]*8);i++)
	{
		gp2x_sound_buffer[4+i] = 0;
	}
	
	return(0);
}

void gp_stopSound(void)
{
	unsigned int i=0;
	gp2x_sound_thread_exit=1;
	printf("Killing Thread\r\n");
	for(i=0;i<(gp2x_sound_buffer[1]*8);i++)
	{
		gp2x_sound_buffer[4+i] = 0;
	}
	usleep(100000); 
	printf("Thread is dead\r\n");
	gp2x_sound_thread=0;
	gp2x_sound_thread_exit=0;
	CurrentSoundBank=0;
}


/* 
########################
System functions
########################
 */ 
void gp_Reset(void)
{
	int i;

	if( framebuffer_mmap )
		munmap( framebuffer_mmap, fb_size );
		
	if( fb_dev >= 0 )
		close( fb_dev );

	for( i = 0 ; i < key_devs ; i++) {
		close( key_dev[i] );
	}
	
	fcloseall();
}

void gp_video_RGB_setscaling(int W, int H)
{

}

void gp_setCpuspeed(unsigned int MHZ)
{

}

// craigix: --trc 6 --tras 4 --twr 1 --tmrd 1 --trfc 1 --trp 2 --trcd 2
// set_RAM_Timings(6, 4, 1, 1, 1, 2, 2);
void set_RAM_Timings(int tRC, int tRAS, int tWR, int tMRD, int tRFC, int tRP, int tRCD)
{
}

void set_gamma(int g100)
{
}





