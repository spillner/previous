#include <stdlib.h>

#include "configuration.h"
#include "m68000.h"
#include "dimension.hpp"
#include "nd_nbic.hpp"
#include "nd_mem.hpp"
#include "nd_sdl.hpp"

#define nd_get_mem_bank(addr)    (nd->mem_banks[nd_bankindex((addr)|ND_BOARD_BITS)])
#define nd68k_get_mem_bank(addr) (mem_banks[nd_bankindex(addr)])

#define nd_longget(addr)   (nd_get_mem_bank(addr)->lget(addr))
#define nd_wordget(addr)   (nd_get_mem_bank(addr)->wget(addr))
#define nd_byteget(addr)   (nd_get_mem_bank(addr)->bget(addr))
#define nd_longput(addr,l) (nd_get_mem_bank(addr)->lput(addr, l))
#define nd_wordput(addr,w) (nd_get_mem_bank(addr)->wput(addr, w))
#define nd_byteput(addr,b) (nd_get_mem_bank(addr)->bput(addr, b))
#define nd_cs8get(addr)    (nd_get_mem_bank(addr)->cs8get(addr))

#define nd68k_longget(addr)   (nd68k_get_mem_bank(addr)->lget(addr))
#define nd68k_wordget(addr)   (nd68k_get_mem_bank(addr)->wget(addr))
#define nd68k_byteget(addr)   (nd68k_get_mem_bank(addr)->bget(addr))
#define nd68k_longput(addr,l) (nd68k_get_mem_bank(addr)->lput(addr, l))
#define nd68k_wordput(addr,w) (nd68k_get_mem_bank(addr)->wput(addr, w))
#define nd68k_byteput(addr,b) (nd68k_get_mem_bank(addr)->bput(addr, b))
#define nd68k_cs8get(addr)    (nd68k_get_mem_bank(addr)->cs8geti(addr))

NextDimension::NextDimension(int slot) :
    NextBusBoard(slot),
    mem_banks(new ND_Addrbank*[65536]),
    ram(host_malloc_aligned(64*1024*1024)),
    vram(host_malloc_aligned(4*1024*1024)),
    rom(host_malloc_aligned(128*1024)),
    rom_last_addr(0),
    sdl(slot, (Uint32*)vram),
    i860(this),
    nbic(slot, ND_NBIC_ID),
    mc(this),
    dp(this),
    dmcd(this),
    dcsc0(this, 0),
    dcsc1(this, 1),
    rom_command(0)
{
    host_atomic_set(&m_port, 0);
    i860.uninit();
    nbic.init();
    mc.init();
    dp.init();
    mem_init();
    i860.init();
    sdl.init();
}

NextDimension::~NextDimension() {
    i860.uninit();
    sdl.destroy();
    
    delete[] mem_banks;
    free(ram);
    free(vram);
    free(rom);

}

void NextDimension::reset(void) {
    i860.set_run_func();
    sdl.start_interrupts();
}

void NextDimension::pause(bool pause) {
    i860.pause(pause);
    sdl.pause(pause);
}

/* NeXTdimension board memory access (m68k) */

 Uint32 NextDimension::board_lget(Uint32 addr) {
    addr |= ND_BOARD_BITS;
    return nd68k_longget(addr);
}

 Uint16 NextDimension::board_wget(Uint32 addr) {
    addr |= ND_BOARD_BITS;
    return nd68k_wordget(addr);
}

 Uint8 NextDimension::board_bget(Uint32 addr) {
    addr |= ND_BOARD_BITS;
    return nd68k_byteget(addr);
}

 void NextDimension::board_lput(Uint32 addr, Uint32 l) {
     addr |= ND_BOARD_BITS;
     nd68k_longput(addr, l);
}

 void NextDimension::board_wput(Uint32 addr, Uint16 w) {
    addr |= ND_BOARD_BITS;
    nd68k_wordput(addr, w);
}

 void NextDimension::board_bput(Uint32 addr, Uint8 b) {
    addr |= ND_BOARD_BITS;
    nd68k_byteput(addr, b);
}

/* NeXTdimension slot memory access */
Uint32 NextDimension::slot_lget(Uint32 addr) {
    addr |= ND_SLOT_BITS;
    
    if (addr<ND_NBIC_SPACE) {
        return nd68k_longget(addr);
    } else {
        return nbic.lget(addr);
    }
}

Uint16 NextDimension::slot_wget(Uint32 addr) {
    addr |= ND_SLOT_BITS;
    
    if (addr<ND_NBIC_SPACE) {
        return nd68k_wordget(addr);
    } else {
        return nbic.wget(addr);
    }
}

Uint8 NextDimension::slot_bget(Uint32 addr) {
    addr |= ND_SLOT_BITS;
    
    if (addr<ND_NBIC_SPACE) {
        return nd68k_byteget(addr);
    } else {
        return nbic.bget(addr);
    }
}

void NextDimension::slot_lput(Uint32 addr, Uint32 l) {
    addr |= ND_SLOT_BITS;
    
    if (addr<ND_NBIC_SPACE) {
        nd68k_longput(addr, l);
    } else {
        nbic.lput(addr, l);
    }
}

void NextDimension::slot_wput(Uint32 addr, Uint16 w) {
    addr |= ND_SLOT_BITS;
    
    if (addr<ND_NBIC_SPACE) {
        nd68k_wordput(addr, w);
    } else {
        nbic.wput(addr, w);
    }
}

void NextDimension::slot_bput(Uint32 addr, Uint8 b) {
    addr |= ND_SLOT_BITS;
    
    if (addr<ND_NBIC_SPACE) {
        nd68k_byteput(addr, b);
    } else {
        nbic.bput(addr, b);
    }
}

void NextDimension::send_msg(int msg) {
    int value;
    do {
        value = m_port.value;
    } while (!host_atomic_cas(&m_port, value, (value | msg)));
}

/* NeXTdimension board memory access (i860) */

Uint8  NextDimension::i860_cs8get(const NextDimension* nd, Uint32 addr) {
    return nd_cs8get(addr);
}

void   NextDimension::i860_rd8_be(const NextDimension* nd, Uint32 addr, Uint32* val) {
    *((Uint8*)val) = nd_byteget(addr);
}

void   NextDimension::i860_rd16_be(const NextDimension* nd, Uint32 addr, Uint32* val) {
    *((Uint16*)val) = nd_wordget(addr);
}

void   NextDimension::i860_rd32_be(const NextDimension* nd, Uint32 addr, Uint32* val) {
    val[0] = nd_longget(addr);
}

void   NextDimension::i860_rd64_be(const NextDimension* nd, Uint32 addr, Uint32* val) {
    const ND_Addrbank* ab = nd_get_mem_bank(addr);
    val[0] = ab->lget(addr+4);
    val[1] = ab->lget(addr+0);
}

void   NextDimension::i860_rd128_be(const NextDimension* nd, Uint32 addr, Uint32* val) {
    const ND_Addrbank* ab = nd_get_mem_bank(addr);
    val[0]  = ab->lget(addr+4);
    val[1]  = ab->lget(addr+0);
    val[2]  = ab->lget(addr+12);
    val[3]  = ab->lget(addr+8);
}

void   NextDimension::i860_wr8_be(const NextDimension* nd, Uint32 addr, const Uint32* val) {
    nd_byteput(addr, *((const Uint8*)val));
}

void   NextDimension::i860_wr16_be(const NextDimension* nd, Uint32 addr, const Uint32* val) {
    nd_wordput(addr, *((const Uint16*)val));
}

void   NextDimension::i860_wr32_be(const NextDimension* nd, Uint32 addr, const Uint32* val) {
    nd_longput(addr, val[0]);
}

void   NextDimension::i860_wr64_be(const NextDimension* nd, Uint32 addr, const Uint32* val) {
    const ND_Addrbank* ab = nd_get_mem_bank(addr);
    ab->lput(addr+4, val[0]);
    ab->lput(addr+0, val[1]);
}

void   NextDimension::i860_wr128_be(const NextDimension* nd, Uint32 addr, const Uint32* val) {
    const ND_Addrbank* ab = nd_get_mem_bank(addr);
    ab->lput(addr+4,  val[0]);
    ab->lput(addr+0,  val[1]);
    ab->lput(addr+12, val[2]);
    ab->lput(addr+8,  val[3]);
}

void   NextDimension::i860_rd8_le(const NextDimension* nd, Uint32 addr, Uint32* val) {
    *((Uint8*)val) = nd_byteget(addr^7);
}

void   NextDimension::i860_rd16_le(const NextDimension* nd, Uint32 addr, Uint32* val) {
    *((Uint16*)val) = nd_wordget(addr^6);
}

void   NextDimension::i860_rd32_le(const NextDimension* nd, Uint32 addr, Uint32* val) {
    val[0] = nd_longget(addr^4);
}

void   NextDimension::i860_rd64_le(const NextDimension* nd, Uint32 addr, Uint32* val) {
    const ND_Addrbank* ab = nd_get_mem_bank(addr);
    val[0] = ab->lget(addr+0);
    val[1] = ab->lget(addr+4);
}

void   NextDimension::i860_rd128_le(const NextDimension* nd, Uint32 addr, Uint32* val) {
    const ND_Addrbank* ab = nd_get_mem_bank(addr);
    val[0]  = ab->lget(addr+0);
    val[1]  = ab->lget(addr+4);
    val[2]  = ab->lget(addr+8);
    val[3]  = ab->lget(addr+12);
}

void   NextDimension::i860_wr8_le(const NextDimension* nd, Uint32 addr, const Uint32* val) {
    nd_byteput(addr^7, *((const Uint8*)val));
}

void   NextDimension::i860_wr16_le(const NextDimension* nd, Uint32 addr, const Uint32* val) {
    nd_wordput(addr^6, *((const Uint16*)val));
}

void   NextDimension::i860_wr32_le(const NextDimension* nd, Uint32 addr, const Uint32* val) {
    nd_longput(addr^4, val[0]);
}

void   NextDimension::i860_wr64_le(const NextDimension* nd, Uint32 addr, const Uint32* val) {
    const ND_Addrbank* ab = nd_get_mem_bank(addr);
    ab->lput(addr+0, val[0]);
    ab->lput(addr+4, val[1]);
}

void   NextDimension::i860_wr128_le(const NextDimension* nd, Uint32 addr, const Uint32* val) {
    const ND_Addrbank* ab = nd_get_mem_bank(addr);
    ab->lput(addr+0,  val[0]);
    ab->lput(addr+4,  val[1]);
    ab->lput(addr+8,  val[2]);
    ab->lput(addr+12, val[3]);
}

/* Message disaptcher - executed on i860 thread, safe to call i860 methods */
bool NextDimension::handle_msgs(void) {
    int msg = host_atomic_set(&m_port, 0);
    
    if(msg & MSG_DISPLAY_BLANK)
        set_blank_state(ND_DISPLAY, host_blank_state(slot, ND_DISPLAY));
    if(msg & MSG_VIDEO_BLANK)
        set_blank_state(ND_VIDEO, host_blank_state(slot, ND_VIDEO));

    return i860.handle_msgs(msg);
}

void nd_start_debugger(void) {
    FOR_EACH_SLOT(slot) {
        IF_NEXT_DIMENSION(slot, nd) {
            nd->send_msg(MSG_DBG_BREAK);
        }
    }
}

extern "C" {
    void nd_display_blank(int slot) {
        ((NextDimension*)nextbus[slot])->send_msg(MSG_DISPLAY_BLANK);
    }

    void nd_video_blank(int slot) {
        ((NextDimension*)nextbus[slot])->send_msg(MSG_VIDEO_BLANK);
    }

    const char* nd_reports(double realTime, double hostTime) {
        FOR_EACH_SLOT(slot) {
            IF_NEXT_DIMENSION(slot, nd) {
                return nd->i860.reports(realTime, hostTime);
            }
        }
        return "";
    }
    
    Uint32* nd_vram_for_slot(int slot) {
        IF_NEXT_DIMENSION(slot, nd)
            return (Uint32*)nd->vram;
        else
            return NULL;
    }
}



