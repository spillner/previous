/*
  Previous - dlgAbout.c

  This file is distributed under the GNU Public License, version 2 or at
  your option any later version. Read the file gpl.txt for details.

  Show information about the program and its license.
*/
const char DlgAbout_fileid[] = "Previous dlgAbout.c : " __DATE__ " " __TIME__;

#include "main.h"
#include "dialog.h"
#include "sdlgui.h"


/* The "About"-dialog: */
static SGOBJ aboutdlg[] =
{
	{ SGBOX, 0, 0, 0,0, 56,22, NULL },
	{ SGTEXT, 0, 0, 21,1, 12,1, PROG_NAME },
	{ SGTEXT, 0, 0, 21,2, 12,1, "==============" },
	{ SGTEXT, 0, 0, 2,4,  52,1, "Previous is a NeXT computer system (black hardware) " },
	{ SGTEXT, 0, 0, 2,5,  52,1, "emulator. It was written by Andreas Grabher,        " },
	{ SGTEXT, 0, 0, 2,6,  52,1, "Simon Schubiger and Gilles Fetis with the help of   " },
	{ SGTEXT, 0, 0, 2,7,  52,1, "the community at the NeXT International Forums      " },
	{ SGTEXT, 0, 0, 2,8,  52,1, "(http://www.nextcomputers.org/forums). Previous is  " },
	{ SGTEXT, 0, 0, 2,9,  52,1, "the work of many.                                   " },
	{ SGTEXT, 0, 0, 2,11, 52,1, "This program is free software. You can redistribute " },
	{ SGTEXT, 0, 0, 2,12, 52,1, "it and/or modify it. Please check the License and   " },
	{ SGTEXT, 0, 0, 2,13, 52,1, "Copyright in the individual source files.           " },
	{ SGTEXT, 0, 0, 2,15, 52,1, "This program is distributed in the hope that it will" },
	{ SGTEXT, 0, 0, 2,16, 52,1, "be useful, but WITHOUT ANY WARRANTY.                " },
	{ SGBUTTON, SG_DEFAULT, 0, 24,19, 8,1, "OK" },
	{ -1, 0, 0, 0,0, 0,0, NULL }
};


/*-----------------------------------------------------------------------*/
/**
 * Show the "about" dialog:
 */
void Dialog_AboutDlg(void)
{
	/* Center PROG_NAME title string */
	aboutdlg[1].x = (aboutdlg[0].w - strlen(PROG_NAME)) / 2;

	SDLGui_CenterDlg(aboutdlg);
	SDLGui_DoDialog(aboutdlg, NULL);
}
