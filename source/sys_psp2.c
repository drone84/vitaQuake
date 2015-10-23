/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "quakedef.h"
#include "errno.h"
#include "splash.h"
#include <psp2/ctrl.h>
#include <psp2/types.h>
#include <psp2/rtc.h>
#include "draw_psp2.h"
#include "console_psp2.h"
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#define u64 uint64_t

extern uint8_t* decodeJpg(unsigned char* in,u64 size);

qboolean		isDedicated;

uint64_t initialTime = 0;
int hostInitialized = 0;
SceCtrlData pad, oldpad;
/*
===============================================================================

FILE IO

===============================================================================
*/

#define MAX_HANDLES             10
FILE    *sys_handles[MAX_HANDLES];

int             findhandle (void)
{
	int             i;

	for (i=1 ; i<MAX_HANDLES ; i++)
		if (!sys_handles[i])
			return i;
	Sys_Error ("out of handles");
	return -1;
}

/*
================
filelength
================
*/
int filelength (FILE *f)
{
	int             pos;
	int             end;

	pos = ftell (f);
	fseek (f, 0, SEEK_END);
	end = ftell (f);
	fseek (f, pos, SEEK_SET);

	return end;
}

int Sys_FileOpenRead (char *path, int *hndl)
{
	FILE    *f;
	int             i;

	i = findhandle ();

	f = fopen(path, "rb");
	if (!f)
	{
		*hndl = -1;
		return -1;
	}
	sys_handles[i] = f;
	*hndl = i;

	return filelength(f);
}

int Sys_FileOpenWrite (char *path)
{
	FILE    *f;
	int             i;

	i = findhandle ();

	f = fopen(path, "wb");
	if (!f)
		Sys_Error ("Error opening %s: %s", path,strerror(errno));
	sys_handles[i] = f;

	return i;
}

void Sys_FileClose (int handle)
{
	fclose (sys_handles[handle]);
	sys_handles[handle] = NULL;
}

void Sys_FileSeek (int handle, int position)
{
	fseek (sys_handles[handle], position, SEEK_SET);
}

int Sys_FileRead (int handle, void *dest, int count)
{
	return fread (dest, 1, count, sys_handles[handle]);
}

int Sys_FileWrite (int handle, void *data, int count)
{
	return fwrite (data, 1, count, sys_handles[handle]);
}

int     Sys_FileTime (char *path)
{
	FILE    *f;

	f = fopen(path, "rb");
	if (f)
	{
		fclose(f);
		return 1;
	}

	return -1;
}

void Sys_mkdir (char *path)
{
	sceIoMkdir(path, 0777);
}


/*
===============================================================================

SYSTEM IO

===============================================================================
*/

void Sys_MakeCodeWriteable (unsigned long startaddr, unsigned long length)
{
}

void Sys_Quit (void)
{
	Host_Shutdown();
	console_fini();
	end_video();
	sceKernelExitProcess(0);
}

void Sys_Error (char *error, ...)
{

	va_list         argptr;
	
	INFO("Sys_Error: ");
	
	char buf[256];
	va_start (argptr, error);
	vsnprintf (buf, sizeof(buf), error,argptr);
	va_end (argptr);
	INFO("%s\n", buf);
	INFO("Press START to exit");
	while(1){
		SceCtrlData pad;
		sceCtrlPeekBufferPositive(0, &pad, 1);
		int kDown = pad.buttons;
		if (kDown & PSP2_CTRL_START)
			break;
	}
	Sys_Quit();
}

void Sys_Printf (char *fmt, ...)
{
	if(hostInitialized)
		return;

	va_list argptr;
	char buf[256];
	va_start (argptr,fmt);
	vsnprintf (buf, sizeof(buf), fmt,argptr);
	va_end (argptr);
	INFO(buf);
	
}

char *Sys_ConsoleInput (void)
{
	return NULL;
}

void Sys_Sleep (void)
{
}

double Sys_FloatTime (void)
{
}

void PSP2_KeyDown(int keys){
	if( keys & PSP2_CTRL_SELECT)
		Key_Event(K_ESCAPE, true);
	if( keys & PSP2_CTRL_START)
		Key_Event(K_ENTER, true);
	if( keys & PSP2_CTRL_UP)
		Key_Event(K_UPARROW, true);
	if( keys & PSP2_CTRL_DOWN)
		Key_Event(K_DOWNARROW, true);
	if( keys & PSP2_CTRL_LEFT)
		Key_Event(K_LEFTARROW, true);
	if( keys & PSP2_CTRL_RIGHT)
		Key_Event(K_RIGHTARROW, true);
	if( keys & PSP2_CTRL_SQUARE)
		Key_Event(K_AUX2, true);
	if( keys & PSP2_CTRL_TRIANGLE)
		Key_Event(K_AUX3, true);
	if( keys & PSP2_CTRL_CROSS)
		Key_Event(K_AUX1, true);
	if( keys & PSP2_CTRL_CIRCLE)
		Key_Event(K_AUX4, true);
	if( keys & PSP2_CTRL_LTRIGGER)
		Key_Event(K_AUX5, true);
	if( keys & PSP2_CTRL_RTRIGGER)
		Key_Event(K_AUX6, true);
}

void PSP2_KeyUp(int keys, int oldkeys){
	if ((!(keys & PSP2_CTRL_SELECT)) && (oldkeys & PSP2_CTRL_SELECT))
		Key_Event(K_ESCAPE, false);
	if ((!(keys & PSP2_CTRL_START)) && (oldkeys & PSP2_CTRL_START))
		Key_Event(K_ENTER, false);
	if ((!(keys & PSP2_CTRL_UP)) && (oldkeys & PSP2_CTRL_UP))
		Key_Event(K_UPARROW, false);
	if ((!(keys & PSP2_CTRL_DOWN)) && (oldkeys & PSP2_CTRL_DOWN))
		Key_Event(K_DOWNARROW, false);
	if ((!(keys & PSP2_CTRL_LEFT)) && (oldkeys & PSP2_CTRL_LEFT))
		Key_Event(K_LEFTARROW, false);
	if ((!(keys & PSP2_CTRL_RIGHT)) && (oldkeys & PSP2_CTRL_RIGHT))
		Key_Event(K_RIGHTARROW, false);
	if ((!(keys & PSP2_CTRL_SQUARE)) && (oldkeys & PSP2_CTRL_SQUARE))
		Key_Event(K_AUX2, false);
	if ((!(keys & PSP2_CTRL_TRIANGLE)) && (oldkeys & PSP2_CTRL_TRIANGLE))
		Key_Event(K_AUX3, false);
	if ((!(keys & PSP2_CTRL_CROSS)) && (oldkeys & PSP2_CTRL_CROSS))
		Key_Event(K_AUX1, false);
	if ((!(keys & PSP2_CTRL_CIRCLE)) && (oldkeys & PSP2_CTRL_CIRCLE))
		Key_Event(K_AUX4, false);
	if ((!(keys & PSP2_CTRL_LTRIGGER)) && (oldkeys & PSP2_CTRL_LTRIGGER))
		Key_Event(K_AUX5, false);
	if ((!(keys & PSP2_CTRL_RTRIGGER)) && (oldkeys & PSP2_CTRL_RTRIGGER))
		Key_Event(K_AUX6, false);
}

void Sys_SendKeyEvents (void)
{
	sceCtrlPeekBufferPositive(0, &pad, 1);
	int kDown = pad.buttons;
	int kUp = oldpad.buttons;
	if(kDown)
		PSP2_KeyDown(kDown);
	if(kUp != kDown)
		PSP2_KeyUp(kDown, kUp);
		
	oldpad = pad;
}

void Sys_HighFPPrecision (void)
{
}

void Sys_LowFPPrecision (void)
{
}

//=============================================================================

int main (int argc, char **argv)
{
	scePowerSetArmClockFrequency(444);
	const float tickRate = 1.0f / sceRtcGetTickResolution();
	init_video();
	
	// Revitalize splashscreen
	uint8_t* buffer = decodeJpg(splash, size_splash);
	int x;
	int y;
	for(x=0; x<960; x++){
		for(y=0; y<544;y++){
			int idx = (y*960 + x) * 3;
			uint32_t color = (buffer[idx]) | (buffer[idx + 1] << 8) | (buffer[idx + 2] << 16);
			draw_pixel(x, y, color);
		}
	}
	sceKernelDelayThread(4000000);
	free(buffer);
	clear_screen();
	
	console_init();
	console_set_color(WHITE);
	static quakeparms_t    parms;

	parms.memsize = 16*1024*1024;
	parms.membase = malloc (parms.memsize);
	parms.basedir = ".";

	COM_InitArgv (argc, argv);

	parms.argc = com_argc;
	parms.argv = com_argv;	
	Host_Init (&parms);
	hostInitialized = 1;
	//Sys_Init();
	
	// Set default PSVITA controls
	Cbuf_AddText ("unbindall\n");
	Cbuf_AddText ("bind CROSS +jump\n"); // Cross
	Cbuf_AddText ("bind SQUARE +attack\n"); // Square
	Cbuf_AddText ("bind CIRCLE +jump\n"); // Circle
	Cbuf_AddText ("bind TRIANGLE \"impulse 10\"\n"); // Triangle
	Cbuf_AddText ("bind LTRIGGER +speed\n"); // Left Trigger
	Cbuf_AddText ("bind RTRIGGER +attack\n"); // Right Trigger
	Cbuf_AddText ("bind UPARROW +moveup\n"); // Up
	Cbuf_AddText ("bind DOWNARROW +movedown\n"); // Down
	Cbuf_AddText ("bind LEFTARROW +moveleft\n"); // Left
	Cbuf_AddText ("bind RIGHTARROW +moveright\n"); // Right
	Cbuf_AddText ("sensitivity 5\n"); // Right Analog Sensitivity
	
	u64 lastTick;
	sceRtcGetCurrentTick(&lastTick);
	
	while (1)
	{
		sceKernelPowerTick(0);
		u64 tick;
		sceRtcGetCurrentTick(&tick);
		const unsigned int deltaTick  = tick - lastTick;
		const float   deltaSecond = deltaTick * tickRate;
		
		Host_Frame(deltaSecond);
		
		lastTick = tick;
	}
	console_fini();
	end_video();
	sceKernelExitProcess(0);
	return 0;
}
