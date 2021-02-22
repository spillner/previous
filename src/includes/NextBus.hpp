#pragma once

#ifndef __NEXT_BUS_H__
#define __NEXT_BUS_H__

#include <SDL_stdinc.h>
#include "log.h"
#include "cycInt.h"

#define LOG_NEXTBUS_LEVEL   LOG_NONE

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
    void nextbus_init(void);

    Uint32 nextbus_slot_lget(Uint32 addr);
    Uint32 nextbus_slot_wget(Uint32 addr);
    Uint32 nextbus_slot_bget(Uint32 addr);
    void nextbus_slot_lput(Uint32 addr, Uint32 val);
    void nextbus_slot_wput(Uint32 addr, Uint32 val);
    void nextbus_slot_bput(Uint32 addr, Uint32 val);

    Uint32 nextbus_board_lget(Uint32 addr);
    Uint32 nextbus_board_wget(Uint32 addr);
    Uint32 nextbus_board_bget(Uint32 addr);
    void nextbus_board_lput(Uint32 addr, Uint32 val);
    void nextbus_board_wput(Uint32 addr, Uint32 val);
    void nextbus_board_bput(Uint32 addr, Uint32 val);
    
    void NextBus_Reset(void);
    void NextBus_Pause(bool pause);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#ifdef __cplusplus

#define FOR_EACH_SLOT(slot) for(int slot = 2; slot < 8; slot += 2)
#define IF_NEXT_DIMENSION(slot, nd) if(NextDimension* nd = dynamic_cast<NextDimension*>(nextbus[(slot)]))

class NextBusSlot {
public:
    int slot;
    
    NextBusSlot(int slot);
    
    virtual ~NextBusSlot();
    
    virtual Uint32 slot_lget(Uint32 addr);
    virtual Uint16 slot_wget(Uint32 addr);
    virtual Uint8  slot_bget(Uint32 addr);
    virtual void   slot_lput(Uint32 addr, Uint32 val);
    virtual void   slot_wput(Uint32 addr, Uint16 val);
    virtual void   slot_bput(Uint32 addr, Uint8 val);
    
    virtual Uint32 board_lget(Uint32 addr);
    virtual Uint16 board_wget(Uint32 addr);
    virtual Uint8  board_bget(Uint32 addr);
    virtual void   board_lput(Uint32 addr, Uint32 val);
    virtual void   board_wput(Uint32 addr, Uint16 val);
    virtual void   board_bput(Uint32 addr, Uint8 val);
    
    virtual void   reset(void);
    virtual void   pause(bool pause);
};

class NextBusBoard : public NextBusSlot {
public:
    NextBusBoard(int slot);
};

extern NextBusSlot* nextbus[];

#endif /* __cplusplus */

#endif /* __NEXT_BUS_H__ */
