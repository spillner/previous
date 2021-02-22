#pragma once

#ifndef __RAMDAC_H__
#define __RAMDAC_H__

typedef struct {
    int   addr;
    int   idx;
    Uint32 wtt_tmp;
    Uint32 wtt[0x10];
    Uint8 ccr[0xC];
    Uint8 reg[0x30];
    Uint8 ram[0x630];
} bt463;



#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
    uae_u32 bt463_bget(bt463* ramdac, uaecptr addr);
    void bt463_bput(bt463* ramdac, uaecptr addr, uae_u32 b);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __RAMDAC_H__ */
