/*
  Hatari - dialog.h

  This file is distributed under the GNU Public License, version 2 or at
  your option any later version. Read the file gpl.txt for details.
*/

#ifndef HATARI_DIALOG_H
#define HATARI_DIALOG_H

#include "configuration.h"

/* prototypes for gui-sdl/dlg*.c functions: */
int Dialog_MainDlg(bool *bReset, bool *bLoadedSnapshot);
void Dialog_AboutDlg(void);
int DlgAlert_Notice(const char *text);
int DlgAlert_Query(const char *text);
void Dialog_DeviceDlg(void);
void DlgFloppy_Main(void);
void DlgSCSI_Main(void);
void DlgOptical_Main(void);
void DlgEthernet_Main(void);
bool DlgEthernetAdvanced_ConfigurePCAP(void);
void DlgEthernetAdvanced_ConfigureMAC(Uint8 *mac);
bool DlgEthernetAdvanced_GetRomMAC(Uint8 *mac);
void DlgEthernetAdvanced_GetMAC(Uint8 *mac);
void DlgSound_Main(void);
void DlgPrinter_Main(void);
void Dialog_JoyDlg(void);
void Dialog_KeyboardDlg(void);
void Dialog_MouseDlg(void);
bool Dialog_MemDlg(void);
void Dialog_MemAdvancedDlg(int *membanks);
char* DlgNewDisk_Main(void);
void Dialog_SlotSelect(int *slot);
void Dialog_MonitorDlg(void);
void Dialog_WindowDlg(void);
void Dialog_SoundDlg(void);
void Dialog_SystemDlg(void);
void Dialog_AdvancedDlg(void);
void Dialog_GraphicsCheckConsole(int board);
void Dialog_GraphicsDlg(void);
void Dialog_DimensionDlg(int board);
void DlgRom_Main(void);
void DlgBoot_Main(void);
void DlgMissing_Rom(const char* type, char *imgname, const char *defname, bool *enabled);
void DlgMissing_Disk(const char* type, int num, char *imgname, bool *ins, bool *wp);
/* and dialog.c */
bool Dialog_DoProperty(void);
void Dialog_CheckFiles(void);

#endif
