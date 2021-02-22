#include "configuration.h"
#include "NextBus.hpp"
#include "nbic.h"
#include "dimension.hpp"

extern "C" void M68000_BusError(Uint32 addr, bool bReadWrite);

static Uint8 bus_error(Uint32 addr, const char* acc) {
    Log_Printf(LOG_WARN, "[NextBus] Bus error %s at %08X", acc, addr);
    M68000_BusError(addr, 1);
    return 0;
}

NextBusSlot::NextBusSlot(int slot) : slot(slot) {}
NextBusSlot::~NextBusSlot() {}

Uint32 NextBusSlot::slot_lget(Uint32 addr) {return bus_error(addr, "lget");}
Uint16 NextBusSlot::slot_wget(Uint32 addr) {return bus_error(addr, "wget");}
Uint8  NextBusSlot::slot_bget(Uint32 addr) {return bus_error(addr, "bget");}

Uint32 NextBusSlot::board_lget(Uint32 addr) {return bus_error(addr, "lget");}
Uint16 NextBusSlot::board_wget(Uint32 addr) {return bus_error(addr, "wget");}
Uint8  NextBusSlot::board_bget(Uint32 addr) {return bus_error(addr, "bget");}

void NextBusSlot::slot_lput(Uint32 addr, Uint32 val) {bus_error(addr, "lput");}
void NextBusSlot::slot_wput(Uint32 addr, Uint16 val) {bus_error(addr, "wput");}
void NextBusSlot::slot_bput(Uint32 addr, Uint8 val)  {bus_error(addr, "bput");}

void NextBusSlot::board_lput(Uint32 addr, Uint32 val) {bus_error(addr, "lput");}
void NextBusSlot::board_wput(Uint32 addr, Uint16 val) {bus_error(addr, "wput");}
void NextBusSlot::board_bput(Uint32 addr, Uint8 val)  {bus_error(addr, "bput");}

void NextBusSlot::reset(void) {}
void NextBusSlot::pause(bool pause) {}

NextBusBoard::NextBusBoard(int slot) : NextBusSlot(slot) {}

class M68KBoard : public NextBusBoard {
public:
    M68KBoard(int slot) : NextBusBoard(slot) {}
        
    virtual Uint32 slot_lget(Uint32 addr) {return nb_cpu_slot_lget(addr);}
    virtual Uint16 slot_wget(Uint32 addr) {return nb_cpu_slot_wget(addr);}
    virtual Uint8  slot_bget(Uint32 addr) {return nb_cpu_slot_bget(addr);}
    virtual void   slot_lput(Uint32 addr, Uint32 val) {nb_cpu_slot_lput(addr, val);}
    virtual void   slot_wput(Uint32 addr, Uint16 val) {nb_cpu_slot_wput(addr, val);}
    virtual void   slot_bput(Uint32 addr, Uint8 val)  {nb_cpu_slot_bput(addr, val);}
};

NextBusSlot* nextbus[16] = {
    new NextBusSlot(0),
    new NextBusSlot(1),
    new NextBusSlot(2),
    new NextBusSlot(3),
    new NextBusSlot(4),
    new NextBusSlot(5),
    new NextBusSlot(6),
    new NextBusSlot(7),
    new NextBusSlot(8),
    new NextBusSlot(9),
    new NextBusSlot(10),
    new NextBusSlot(11),
    new NextBusSlot(12),
    new NextBusSlot(13),
    new NextBusSlot(14),
    new NextBusSlot(15),
};

#define SLOT(addr) (((addr)  & 0x0F000000)>>24)
#define BOARD(addr) (((addr) & 0xF0000000)>>28)

extern "C" {
    /* Slot memory */
    Uint32 nextbus_slot_lget(Uint32 addr) {
        int slot = SLOT(addr);
        Log_Printf(LOG_NEXTBUS_LEVEL, "[NextBus] Slot %i: lget at %08X",slot,addr);
        
        return nextbus[slot]->slot_lget(addr);
    }
    
    Uint32 nextbus_slot_wget(Uint32 addr) {
        int slot = SLOT(addr);
        Log_Printf(LOG_NEXTBUS_LEVEL, "[NextBus] Slot %i: wget at %08X",slot,addr);
        
        return nextbus[slot]->slot_wget(addr);
    }
    
    Uint32 nextbus_slot_bget(Uint32 addr) {
        int slot = SLOT(addr);
        Log_Printf(LOG_NEXTBUS_LEVEL, "[NextBus] Slot %i: bget at %08X",slot,addr);
        
        return nextbus[slot]->slot_bget(addr);
    }
    
    void nextbus_slot_lput(Uint32 addr, Uint32 val) {
        int slot = SLOT(addr);
        Log_Printf(LOG_NEXTBUS_LEVEL, "[NextBus] Slot %i: lput at %08X, val %08X",slot,addr,val);
        
        nextbus[slot]->slot_lput(addr, val);
    }
    
    void nextbus_slot_wput(Uint32 addr, Uint32 val) {
        int slot = SLOT(addr);
        Log_Printf(LOG_NEXTBUS_LEVEL, "[NextBus] Slot %i: wput at %08X, val %04X",slot,addr,val);
        
        nextbus[slot]->slot_wput(addr, val);
    }
    
    void nextbus_slot_bput(Uint32 addr, Uint32 val) {
        int slot = SLOT(addr);
        Log_Printf(LOG_NEXTBUS_LEVEL, "[NextBus] Slot %i: bput at %08X, val %02X",slot,addr,val);
        
        nextbus[slot]->slot_bput(addr, val);
    }
    
    /* Board memory */

    Uint32 nextbus_board_lget(Uint32 addr) {
        int board = BOARD(addr);
        Log_Printf(LOG_NEXTBUS_LEVEL, "[NextBus] Board %i: lget at %08X",board,addr);
        
        return nextbus[board]->board_lget(addr);
    }
    
    Uint32 nextbus_board_wget(Uint32 addr) {
        int board = BOARD(addr);
        Log_Printf(LOG_NEXTBUS_LEVEL, "[NextBus] Board %i: wget at %08X",board,addr);
        
        return nextbus[board]->board_wget(addr);
    }
    
    Uint32 nextbus_board_bget(Uint32 addr) {
        int board = BOARD(addr);
        Log_Printf(LOG_NEXTBUS_LEVEL, "[NextBus] Board %i: bget at %08X",board,addr);
        
        return nextbus[board]->board_bget(addr);
    }
    
    void nextbus_board_lput(Uint32 addr, Uint32 val) {
        int board = BOARD(addr);
        Log_Printf(LOG_NEXTBUS_LEVEL, "[NextBus] Board %i: lput at %08X, val %08X",board,addr,val);
        
        nextbus[board]->board_lput(addr, val);
    }
    
    void nextbus_board_wput(Uint32 addr, Uint32 val) {
        int board = BOARD(addr);
        Log_Printf(LOG_NEXTBUS_LEVEL, "[NextBus] Board %i: wput at %08X, val %04X",board,addr,val);
        
        nextbus[board]->board_wput(addr, val);
    }
    
    void nextbus_board_bput(Uint32 addr, Uint32 val) {
        int board = BOARD(addr);
        Log_Printf(LOG_NEXTBUS_LEVEL, "[NextBus] Board %i: bput at %08X, val %02X",board,addr,val);
        
        nextbus[board]->board_bput(addr, val);
    }
    
    static void remove_board(int slot) {
        delete nextbus[slot];
        nextbus[slot] = new NextBusSlot(slot);
    }

    static void insert_board(NextBusBoard* board) {
        delete nextbus[board->slot];
        nextbus[board->slot] = board;
    }
    
    /* Init function for NextBus */
    void nextbus_init(void) {
        insert_board(new M68KBoard(0));
        
        for (int i = 0; i < ND_MAX_BOARDS; i++) {
            remove_board(ND_SLOT(i));
            if (ConfigureParams.Dimension.board[i].bEnabled && ConfigureParams.System.nMachineType != NEXT_STATION) {
                Log_Printf(LOG_WARN, "[NextBus] NeXTdimension board at slot %i", ND_SLOT(i));
                insert_board(new NextDimension(ND_SLOT(i)));
            }
        }
    }
    
    void NextBus_Reset(void) {
        for(int slot = 0; slot < 16; slot++)
            nextbus[slot]->reset();
    }
    
    void NextBus_Pause(bool pause) {
        for(int slot = 0; slot < 16; slot++)
            nextbus[slot]->pause(pause);
    }
}
