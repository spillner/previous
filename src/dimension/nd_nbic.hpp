#pragma once

#ifndef __ND_NBIC_H__
#define __ND_NBIC_H__

#define ND_NBIC_ID        0xC0000001

#ifdef __cplusplus

class NBIC {
    int    slot;
  //  Uint32 control; // unused
    Uint32 id;
    Uint8  intstatus;
    Uint8  intmask;
public:
    static volatile Uint32 remInter;
    static volatile Uint32 remInterMask;

    NBIC(int slot, int id);
    
    Uint8 read(int addr);
    void  write(int addr, Uint8 val);
    
    Uint32 lget(Uint32 addr);
    Uint16 wget(Uint32 addr);
    Uint8  bget(Uint32 addr);
    void   lput(Uint32 addr, Uint32 l);
    void   wput(Uint32 addr, Uint16 w);
    void   bput(Uint32 addr, Uint8 b);
    
    void   init(void);
    void   set_intstatus(bool set);
};

extern "C" {
#endif /* __cplusplus */
    void   nd_nbic_interrupt(void);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __ND_NBIC_H__ */
