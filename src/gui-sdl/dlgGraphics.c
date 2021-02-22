/*
  Previous - dlgDimension.c

  This file is distributed under the GNU Public License, version 2 or at
  your option any later version. Read the file gpl.txt for details.

*/

const char DlgGraphics_fileid[] = "Previous dlgGraphics.c : " __DATE__ " " __TIME__;

#include "main.h"
#include "configuration.h"
#include "dialog.h"
#include "sdlgui.h"
#include "file.h"


#define GDLG_BOARD0         5
#define GDLG_BOARD1         7
#define GDLG_BOARD2         9
#define GDLG_MONOCHROME     12
#define GDLG_COLOR          13
#define GDLG_ALL            14
#define GDLG_DISPLAY		17
#define GDLG_EXIT           19

static SGOBJ graphicsdlg[] =
{
    { SGBOX,      0, 0, 0,  0, 56,24, NULL },
    { SGTEXT,     0, 0, 20, 1, 16, 1, "Graphics options" },
    
    { SGBOX,      0, 0, 2,  3, 32, 9, NULL },
    { SGTEXT,     0, 0, 3,  4, 21, 1, "NeXTdimension boards:" },
    { SGTEXT,     0, 0, 4,  6, 16, 1, "Board at slot 2:" },
    { SGBUTTON,   0, 0, 22, 6, 10, 1, "Add" },
    { SGTEXT,     0, 0, 4,  8, 16, 1, "Board at slot 4:" },
    { SGBUTTON,   0, 0, 22, 8, 10, 1, "Add" },
    { SGTEXT,     0, 0, 4, 10, 16, 1, "Board at slot 6:" },
    { SGBUTTON,   0, 0, 22,10, 10, 1, "Add" },
	
	{ SGBOX,      0, 0, 35, 3, 19, 9, NULL },
	{ SGTEXT,     0, 0, 36, 4, 13, 1, "Show display:" },
    { SGRADIOBUT, 0, 0, 37, 6, 12, 1, "Monochrome" },
	{ SGRADIOBUT, 0, 0, 37, 8,  7, 1, "Color" },
	{ SGRADIOBUT, 0, 0, 37,10,  5, 1, "All" },

    { SGBOX,      0, 0, 2, 13, 52, 3, NULL },
    { SGTEXT,     0, 0, 3, 14, 13, 1, "Main display:" },
    { SGCHECKBOX, 0, 0, 18,14, 26, 1, "Console on NeXTdimension" },

    { SGTEXT,     0, 0, 2, 18, 52, 1, "Note: NeXTdimension does not work with NeXTstations." },
    
    { SGBUTTON, SG_DEFAULT, 0, 17,21, 21,1, "Back to main menu" },

    { -1, 0, 0, 0,0, 0,0, NULL }
};


#define SDLG_SLOT0  2
#define SDLG_SLOT1  3
#define SDLG_SLOT2  4
#define SDLG_SELECT 5

static SGOBJ slotdlg[] =
{
    { SGBOX,      0, 0, 0,  0, 20,12, NULL },
    { SGTEXT,     0, 0, 4,  1, 12, 1, "Select slot:" },
    
    { SGRADIOBUT, 0, 0, 6,  3,  8, 1, "Slot 2" },
    { SGRADIOBUT, 0, 0, 6,  5,  8, 1, "Slot 4" },
    { SGRADIOBUT, 0, 0, 6,  7,  8, 1, "Slot 6" },
    
    { SGBUTTON, SG_DEFAULT, 0, 5,10, 10,1, "Select" },
    
    { -1, 0, 0, 0,0, 0,0, NULL }
};



/*-----------------------------------------------------------------------*/
/**
 * Show and process the "Slot select" dialog.
 */
void Dialog_SlotSelect(int *slot)
{
    int but;
    
    SDLGui_CenterDlg(slotdlg);
    
    /* Set up dialog from actual values: */
    slotdlg[SDLG_SLOT0].state &= ~SG_SELECTED;
    slotdlg[SDLG_SLOT1].state &= ~SG_SELECTED;
    slotdlg[SDLG_SLOT2].state &= ~SG_SELECTED;
    
    switch (*slot) {
        case 0:
            slotdlg[SDLG_SLOT0].state |= SG_SELECTED;
            break;
        case 1:
            slotdlg[SDLG_SLOT1].state |= SG_SELECTED;
            break;
        case 2:
            slotdlg[SDLG_SLOT2].state |= SG_SELECTED;
            break;
            
        default:
            break;
    }
    
    /* Draw and process the dialog: */
    
    do {
        but = SDLGui_DoDialog(slotdlg, NULL);
    }
    while (but != SDLG_SELECT && but != SDLGUI_QUIT
           && but != SDLGUI_ERROR && !bQuitProgram);
    
    if (slotdlg[SDLG_SLOT2].state&SG_SELECTED) {
        *slot = 2;
    } else if (slotdlg[SDLG_SLOT1].state&SG_SELECTED) {
        *slot = 1;
    } else {
        *slot = 0;
    }
}


/*-----------------------------------------------------------------------*/
/**
  * Show and process the "Graphics" dialog.
  */

#define GDLG_SIZE_NOTE "NOTE: Console on NeXTdimension may not show up if NeXTdimension has more than 32 MB of memory."

void Dialog_GraphicsCheckConsole(int board)
{
    int *membank = ConfigureParams.Dimension.board[board].nMemoryBankSize;
    
    if (ConfigureParams.Dimension.bMainDisplay) {
        if (ConfigureParams.Dimension.nMainDisplay == board) {
            if (ConfigureParams.Dimension.board[board].bEnabled) {
                if (Configuration_CheckDimensionMemory(membank) > 32) {
                    DlgAlert_Notice(GDLG_SIZE_NOTE);
                }
            }
        }
    }
}

void Dialog_GraphicsDlg(void)
{
    int but;
    char colordisplay[16];
    char maindisplay[64];
 
 	SDLGui_CenterDlg(graphicsdlg);
 
 	/* Set up dialog from actual values: */
    
    if (ConfigureParams.Dimension.bMainDisplay) {
        graphicsdlg[GDLG_DISPLAY].state |= SG_SELECTED;
    } else {
        graphicsdlg[GDLG_DISPLAY].state &= ~SG_SELECTED;
    }
	
    graphicsdlg[GDLG_COLOR].state      &= ~SG_SELECTED;
    graphicsdlg[GDLG_MONOCHROME].state &= ~SG_SELECTED;
    graphicsdlg[GDLG_ALL].state        &= ~SG_SELECTED;
    
    switch (ConfigureParams.Screen.nMonitorType) {
        case MONITOR_TYPE_DUAL:
            graphicsdlg[GDLG_ALL].state |= SG_SELECTED;
            break;
        case MONITOR_TYPE_CPU:
            graphicsdlg[GDLG_MONOCHROME].state |= SG_SELECTED;
            break;
        case MONITOR_TYPE_DIMENSION:
            graphicsdlg[GDLG_COLOR].state |= SG_SELECTED;
            break;
            
        default:
            break;
    }
    
 	/* Draw and process the dialog: */

    do {
        if (ConfigureParams.Dimension.board[0].bEnabled) {
            graphicsdlg[GDLG_BOARD0].txt = "Remove";
        } else {
            graphicsdlg[GDLG_BOARD0].txt = "Add";
        }
        
        if (ConfigureParams.Dimension.board[1].bEnabled) {
            graphicsdlg[GDLG_BOARD1].txt = "Remove";
        } else {
            graphicsdlg[GDLG_BOARD1].txt = "Add";
        }
        
        if (ConfigureParams.Dimension.board[2].bEnabled) {
            graphicsdlg[GDLG_BOARD2].txt = "Remove";
        } else {
            graphicsdlg[GDLG_BOARD2].txt = "Add";
        }
        
        if (ConfigureParams.Dimension.bMainDisplay) {
            sprintf(maindisplay, "Console on NeXTdimension (Slot %i)", ConfigureParams.Dimension.nMainDisplay*2+2);
            graphicsdlg[GDLG_DISPLAY].txt = maindisplay;
        } else {
            graphicsdlg[GDLG_DISPLAY].txt = "Console on NeXTdimension";
        }
        
        if (ConfigureParams.Screen.nMonitorType == MONITOR_TYPE_DIMENSION) {
            sprintf(colordisplay, "Color (Slot %i)", ConfigureParams.Screen.nMonitorNum*2+2);
            graphicsdlg[GDLG_COLOR].txt = colordisplay;
        } else {
            graphicsdlg[GDLG_COLOR].txt = "Color";
        }

        
        but = SDLGui_DoDialog(graphicsdlg, NULL);
        
        switch (but) {
            case GDLG_BOARD0:
                Dialog_DimensionDlg(0);
                Dialog_GraphicsCheckConsole(0);
                break;
                
            case GDLG_BOARD1:
                Dialog_DimensionDlg(1);
                Dialog_GraphicsCheckConsole(1);
                break;
                
            case GDLG_BOARD2:
                Dialog_DimensionDlg(2);
                Dialog_GraphicsCheckConsole(2);
                break;
                
            case GDLG_ALL:
                ConfigureParams.Screen.nMonitorType = MONITOR_TYPE_DUAL;
                break;
                
            case GDLG_MONOCHROME:
                ConfigureParams.Screen.nMonitorType = MONITOR_TYPE_CPU;
                break;

            case GDLG_COLOR:
                ConfigureParams.Screen.nMonitorType = MONITOR_TYPE_DIMENSION;
                Dialog_SlotSelect(&ConfigureParams.Screen.nMonitorNum);
                break;

            case GDLG_DISPLAY:
                if (ConfigureParams.Dimension.bMainDisplay) {
                    ConfigureParams.Dimension.bMainDisplay = false;
                } else {
                    ConfigureParams.Dimension.bMainDisplay = true;
                    Dialog_SlotSelect(&ConfigureParams.Dimension.nMainDisplay);
                    Dialog_GraphicsCheckConsole(ConfigureParams.Dimension.nMainDisplay);
                }
                break;

            default:
                break;
        }
    }
    while (but != GDLG_EXIT && but != SDLGUI_QUIT
           && but != SDLGUI_ERROR && !bQuitProgram);
}
