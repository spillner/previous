#include "main.h"
#include "configuration.h"
#include "m68000.h"
#include "sysdeps.h"
#include "dimension.hpp"
#include "nd_mem.hpp"
#include "log.h"

/* --------- NEXTDIMENSION MEMORY ---------- */

/* NeXTdimension board memory */
#define ND_RAM_START	0xF8000000
#define ND_RAM_SIZE		0x04000000
#define ND_RAM_MASK		0x03FFFFFF

#define ND_VRAM_START	0xFE000000
#define ND_VRAM_SIZE	0x00400000
#define ND_VRAM_MASK	0x003FFFFF

#define ND_EEPROM_START	0xFFF00000
#define ND_EEPROM_SIZE	0x00020000
#define ND_EEPROM_MASK	0x0001FFFF

/* RAM banks */
#define ND_RAM_BANKSIZE 0x01000000
#define ND_RAM_BANKMASK 0x03000000

/* NeXTdimension dither memory */
#define ND_DMEM_START   0xFF000000
#define ND_DMEM_SIZE    0x00000200
#define ND_DMEM_MASK    0x000001FF

/* NeXTdimension board devices */
#define ND_IO_START		0xFF800000
#define ND_IO_SIZE		0x00004000
#define ND_IO_MASK		0x00003FFF

/* NeXTdimension board RAMDAC */
#define ND_RAMDAC_START	0xFF200000
#define ND_RAMDAC_SIZE  0x00001000
#define ND_RAMDAC_MASK  0x00000FFF

/* NeXTdimension board data path */
#define ND_DP_START	0xF0000000
#define ND_DP_SIZE  0x00001000
#define ND_DP_MASK  0x00000FFF

/* NeXTdimension unknown registers */
#define ND_CSR_START 0xFF400000
#define ND_CSR_SIZE  0x00000200
#define ND_CSR_MASK  0x000001FF

/* Function to fix address for ROM access */
static uaecptr nd_rom_addr_fix(uaecptr addr)
{
#if ND_STEP
    addr >>= 2;
#else
    addr &= ~3;
    addr |= (addr>>17)&3;
#endif
    addr &= ND_EEPROM_MASK;
    return addr;
}

/* Memory banks */

/* NeXTdimension RAM */

class ND_RAM : public ND_Addrbank {
    Uint8* base;
    Uint32 mask;
public:
    ND_RAM(NextDimension* nd, int bank) : ND_Addrbank(nd), base(nd->ram), mask(nd->bankmask[bank]) {}
    
    Uint32 lget(Uint32 addr) const {
        return do_get_mem_long(base + (addr & mask));
    }

    Uint32 wget(Uint32 addr) const {
        return do_get_mem_word(base + (addr & mask));
    }

    Uint32 bget(Uint32 addr) const {
        return base[addr & mask];
    }

     void lput(Uint32 addr, Uint32 l) const {
        do_put_mem_long(base + (addr & mask), l);
    }

     void wput(Uint32 addr, Uint32 w) const {
        do_put_mem_word(base + (addr & mask), w);
    }

     void bput(Uint32 addr, Uint32 b) const {
        base[addr & mask] = b;
    }
};

class ND_Empty : public ND_Addrbank {
public:
    ND_Empty(NextDimension* nd) : ND_Addrbank(nd) {}
    
    Uint32 lget(Uint32 addr) const {
        Log_Printf(LOG_ND_MEM, "[ND] Slot %i: empty memory bank lget at %08X\n", nd->slot,addr);
        return 0;
    }

     Uint32 wget(Uint32 addr) const {
        Log_Printf(LOG_ND_MEM, "[ND] Slot %i: empty memory bank wget at %08X\n", nd->slot,addr);
        return 0;
    }

     Uint32 bget(Uint32 addr) const {
        Log_Printf(LOG_ND_MEM, "[ND] Slot %i: empty memory bank bget at %08X\n", nd->slot,addr);
        return 0;
    }

     void lput(Uint32 addr, Uint32 l) const {
        Log_Printf(LOG_ND_MEM, "[ND] Slot %i: empty memory bank lput at %08X\n", nd->slot,addr);
    }

     void wput(Uint32 addr, Uint32 w) const {
        Log_Printf(LOG_ND_MEM, "[ND] Slot %i: empty memory bank wput at %08X\n", nd->slot,addr);
    }

     void bput(Uint32 addr, Uint32 b) const {
        Log_Printf(LOG_ND_MEM, "[ND] Slot %i: empty memory bank bput at %08X\n",nd->slot,addr);
    }
};

/* NeXTdimension VRAM */
/* stored as ARGB for faster blitting, assuming aligned access for 32 bit */

class ND_VRAM : public ND_Addrbank {
    Uint8* base;
public:
    ND_VRAM(NextDimension* nd) : ND_Addrbank(nd), base(nd->vram) {
        // sanity checks for ARGB mem access
        lput(0, 0x12345678);
        if(lget(0) != 0x12345678) {fprintf(stderr, "ND_VRAM: 32 bit access check failed\n");  goto error;}

        if(bget(0) != 0x12)       {fprintf(stderr, "ND_VRAM: 8 bit access 0 check failed\n"); goto error;}
        if(bget(1) != 0x34)       {fprintf(stderr, "ND_VRAM: 8 bit access 1 check failed\n"); goto error;}
        if(bget(2) != 0x56)       {fprintf(stderr, "ND_VRAM: 8 bit access 2 check failed\n"); goto error;}
        if(bget(3) != 0x78)       {fprintf(stderr, "ND_VRAM: 8 bit access 3 check failed\n"); goto error;}

        if(wget(0) != 0x1234)     {fprintf(stderr, "ND_VRAM: 16 bit access 0 check failed\n"); goto error;}
        if(wget(2) != 0x5678)     {fprintf(stderr, "ND_VRAM: 16 bit access 2 check failed\n"); goto error;}

        wput(0, 0x7654);
        wput(2, 0x3210);
        if(lget(0) != 0x76543210) {fprintf(stderr, "ND_VRAM: 32 bit access check failed (wput)\n");  goto error;}

        bput(0, 0x12);
        bput(1, 0x34);
        bput(2, 0x56);
        bput(3, 0x78);
        if(lget(0) != 0x12345678) {fprintf(stderr, "ND_VRAM: 32 bit access check failed (bput)\n");  goto error;}

        return;
    error:
        exit(1);
    }

    Uint32 lget(Uint32 addr) const {
        addr &= ND_VRAM_MASK;
        switch (addr&3) {
            case 0: return (base[addr+2] << 24) | (base[addr+1] << 16) | (base[addr+0] << 8) | base[addr+3];
            case 1: return (base[addr+0] << 24) | (base[addr-1] << 16) | (base[addr+2] << 8) | base[addr+5];
            case 2: return (base[addr-2] << 24) | (base[addr+1] << 16) | (base[addr+4] << 8) | base[addr+3];
            case 3: return (base[addr+0] << 24) | (base[addr+3] << 16) | (base[addr+2] << 8) | base[addr+1];
        }
        return 0;
    }

    void lput(Uint32 addr, Uint32 l) const {
        addr &= ND_VRAM_MASK;
        switch (addr&3) {
            case 0: base[addr+2] = l >> 24; base[addr+1] = l >> 16; base[addr+0] = l >> 8; base[addr+3] = l; break;
            case 1: base[addr+0] = l >> 24; base[addr-1] = l >> 16; base[addr+2] = l >> 8; base[addr+5] = l; break;
            case 2: base[addr-2] = l >> 24; base[addr+1] = l >> 16; base[addr+4] = l >> 8; base[addr+3] = l; break;
            case 3: base[addr+0] = l >> 24; base[addr+3] = l >> 16; base[addr+2] = l >> 8; base[addr+1] = l; break;
        }
    }

    Uint32 wget(Uint32 addr) const {
        addr &= ND_VRAM_MASK;
        switch (addr&3) {
            case 0: return (base[addr+2] << 8) | base[addr+1];
            case 1: return (base[addr+0] << 8) | base[addr-1];
            case 2: return (base[addr-2] << 8) | base[addr+1];
            case 3: return (base[addr+0] << 8) | base[addr+3];
        }
        return 0;
    }

    void wput(Uint32 addr, Uint32 w) const {
        addr &= ND_VRAM_MASK;
        switch (addr&3) {
            case 0: base[addr+2] = w >> 8; base[addr+1] = w; break;
            case 1: base[addr+0] = w >> 8; base[addr-1] = w; break;
            case 2: base[addr-2] = w >> 8; base[addr+1] = w; break;
            case 3: base[addr+0] = w >> 8; base[addr+3] = w; break;
        }
    }

    Uint32 bget(Uint32 addr) const {
        addr &= ND_VRAM_MASK;
        switch(addr&3) {
            case 0: return base[addr+2];
            case 1: return base[addr+0];
            case 2: return base[addr-2];
            case 3: return base[addr+0];
        }
        return 0;
    }

    void bput(Uint32 addr, Uint32 b) const {
        addr &= ND_VRAM_MASK;
        switch(addr&3) {
            case 0: base[addr+2] = b; break;
            case 1: base[addr+0] = b; break;
            case 2: base[addr-2] = b; break;
            case 3: base[addr+0] = b; break;
        }
    }
};

/* NeXTdimension ROM */

class ND_ROM : public ND_Addrbank {
public:
    ND_ROM(NextDimension* nd) : ND_Addrbank(nd) {}

    Uint32 lget(Uint32 addr) const {
        addr = nd_rom_addr_fix(addr);
        return (nd->rom_read(addr) << 24); /* byte lane at msb */
    }

    Uint32 wget(Uint32 addr) const {
        addr = nd_rom_addr_fix(addr);
        return (nd->rom_read(addr) << 8); /* byte lane at msb */
    }

    Uint32 bget(Uint32 addr) const {
        addr = nd_rom_addr_fix(addr);
        return nd->rom_read(addr);
    }

    Uint32 cs8get(Uint32 addr) const {
        addr &= ND_EEPROM_MASK;
        return nd->rom[addr];
    }

    void lput(Uint32 addr, Uint32 l) const {
        addr = nd_rom_addr_fix(addr);
        nd->rom_write(addr, l >> 24);
    }

    void wput(Uint32 addr, Uint32 w) const {
        addr = nd_rom_addr_fix(addr);
        nd->rom_write(addr, w >> 8);
    }

    void bput(Uint32 addr, Uint32 b) const {
        addr = nd_rom_addr_fix(addr);
        nd->rom_write(addr, b);
    }
};

/* Unknown register access functions (memory controller step 1) */
#if ND_STEP

class ND_CSR : public ND_Addrbank {
public:
    ND_CSR(NextDimension* nd) : ND_Addrbank(nd) {}

     Uint32 lget(Uint32 addr) const {
        Log_Printf(LOG_ND_MEM, "[ND] Slot %i: Board CSR lget at %08X\n", nd->slot,addr);
        return 0;
    }

     Uint32 wget(Uint32 addr) const {
        Log_Printf(LOG_ND_MEM, "[ND] Slot %i: Board CSR wget at %08X\n",nd->slot,addr);
        return 0;
    }

     Uint32 bget(Uint32 addr) const {
        Log_Printf(LOG_ND_MEM, "[ND] Slot %i: Board CSR bget at %08X\n",nd->slot,addr);
        return 0;
    }

     void lput(Uint32 addr, Uint32 l) const {
        Log_Printf(LOG_ND_MEM, "[ND] Slot %i: Board CSR lput at %08X: %08X\n",nd->slot,addr,l);
    }

     void wput(Uint32 addr, Uint32 w) const {
        Log_Printf(LOG_ND_MEM, "[ND] Slot %i: Board CSR wput at %08X: %04X\n",nd->slot,addr,w);
    }

     void bput(Uint32 addr, Uint32 b) const {
        Log_Printf(LOG_ND_MEM, "[ND] Slot %i: Board CSR bput at %08X: %02X\n",nd->slot,addr,b);
    }
};
#endif

/* NeXTdimension dither memory & datapath */

class ND_DMEM : public ND_Addrbank {
public:
    ND_DMEM(NextDimension* nd) : ND_Addrbank(nd) {}

     Uint32 lget(Uint32 addr) const {
        addr &= ND_DP_MASK;
        return addr < ND_DMEM_SIZE ? do_get_mem_long(nd->dmem + addr) : nd->dp.lget(addr);
    }

     Uint32 wget(Uint32 addr) const {
        addr &= ND_DP_MASK;
        return addr < ND_DMEM_SIZE ? do_get_mem_word(nd->dmem + addr) : nd->dp.lget(addr);
    }

     Uint32 bget(Uint32 addr) const {
        addr &= ND_DP_MASK;
        return addr < ND_DMEM_SIZE ? do_get_mem_byte(nd->dmem + addr) : nd->dp.lget(addr);
    }

     void lput(Uint32 addr, Uint32 l) const {
        addr &= ND_DP_MASK;
        if(addr < ND_DMEM_SIZE)
            do_put_mem_long(nd->dmem + addr, l);
        else
            nd->dp.lput(addr, l);
    }

     void wput(Uint32 addr, Uint32 w) const {
        addr &= ND_DP_MASK;
        if(addr < ND_DMEM_SIZE)
            do_put_mem_word(nd->dmem + addr, w);
        else
            nd->dp.lput(addr, w);
    }

     void bput(Uint32 addr, Uint32 b) const {
        addr &= ND_DP_MASK;
        if(addr < ND_DMEM_SIZE)
            do_put_mem_byte(nd->dmem + addr, b);
        else
            nd->dp.lput(addr, b);
    }
};

/* Illegal access functions */

ND_Addrbank::ND_Addrbank(NextDimension* nd) : nd(nd) {}

Uint32 ND_Addrbank::lget(Uint32 addr) const {
    Log_Printf(LOG_ND_MEM, "[ND] Slot %i: Illegal lget at %08X\n",nd->slot,addr);
    return 0;
}

Uint32 ND_Addrbank::wget(Uint32 addr) const {
    Log_Printf(LOG_ND_MEM, "[ND] Slot %i: Illegal wget at %08X\n",nd->slot,addr);
    return 0;
}

Uint32 ND_Addrbank::bget(Uint32 addr) const {
    Log_Printf(LOG_ND_MEM, "[ND] Slot %i: Illegal bget at %08X\n",nd->slot,addr);
    return 0;
}

Uint32 ND_Addrbank::cs8get(Uint32 addr) const {
    Log_Printf(LOG_ND_MEM, "[ND] Slot %i: Illegal cs8get at %08X\n",nd->slot,addr);
    return 0;
}

void ND_Addrbank::lput(Uint32 addr, Uint32 l) const {
    Log_Printf(LOG_ND_MEM, "[ND] Slot %i: Illegal lput at %08X\n",nd->slot,addr);
}

void ND_Addrbank::wput(Uint32 addr, Uint32 w) const {
    Log_Printf(LOG_ND_MEM, "[ND] Slot %i: Illegal wput at %08X\n",nd->slot,addr);
}

void ND_Addrbank::bput(Uint32 addr, Uint32 b) const {
    Log_Printf(LOG_ND_MEM, "[ND] Slot %i: Illegal bput at %08X\n",nd->slot,addr);
}

/* NeXTdimension device space */

class ND_IO : public ND_Addrbank {
public:
    ND_IO(NextDimension* nd) : ND_Addrbank(nd) {}
    
    Uint32 lget(Uint32 addr) const {
        return nd->mc.read(addr);
    }
    
    Uint32 wget(Uint32 addr) const {return 0;}
    
    Uint32 bget(Uint32 addr) const {return 0;}
    
    void lput(Uint32 addr, Uint32 l) const {
        nd->mc.write(addr, l);
    }
    
    void wput(Uint32 addr, Uint32 w) const {}
    void bput(Uint32 addr, Uint32 b) const {}
};

/* NeXTdimension RAMDAC */

class ND_RAMDAC : public ND_Addrbank {
public:
    ND_RAMDAC(NextDimension* nd) : ND_Addrbank(nd) {}
    
    Uint32 lget(Uint32 addr) const {
        return bt463_bget(&nd->ramdac, addr) << 24;
    }
    
    Uint32 wget(Uint32 addr) const {
        return bt463_bget(&nd->ramdac, addr) << 8;
    }
    
    Uint32 bget(Uint32 addr) const {
        return bt463_bget(&nd->ramdac, addr);
    }
    
    void lput(Uint32 addr, Uint32 l) const {
        bt463_bput(&nd->ramdac, addr, l >> 24);
    }
    
    void wput(Uint32 addr, Uint32 w) const {
        bt463_bput(&nd->ramdac, addr, w >> 8);
    }
    
    void bput(Uint32 addr, Uint32 b) const {
        bt463_bput(&nd->ramdac, addr, b);
    }
};

void NextDimension::map_banks (ND_Addrbank *bank, int start, int size) {
    for (int bnr = start; bnr < start + size; bnr++)
        nd_put_mem_bank (bnr << 16, bank);
    return;
}

void NextDimension::init_mem_banks(void) {
    ND_Addrbank* nd_illegal_bank = new ND_Addrbank(this);
    for (int i = 0; i < 65536; i++)
        nd_put_mem_bank(i<<16, nd_illegal_bank);
}

#define write_log printf

void NextDimension::mem_init(void) {
    write_log("[ND] Slot %i: Memory init: Memory size: %iMB\n", slot,
              Configuration_CheckDimensionMemory(ConfigureParams.Dimension.board[ND_NUM(slot)].nMemoryBankSize));

    /* Initialize banks with error memory */
    init_mem_banks();
    
    /* Clear first 4k of memory for m68k ROM polling code */
    memset(ram, 0, 4096 * sizeof(Uint8));
    
    /* Map main memory */
    
    for(int bank = 0; bank < 4; bank++) {
        if (ConfigureParams.Dimension.board[ND_NUM(slot)].nMemoryBankSize[bank]) {
            bankmask[bank] = ND_RAM_BANKMASK|((ConfigureParams.Dimension.board[ND_NUM(slot)].nMemoryBankSize[bank]<<20)-1);
            map_banks(new ND_RAM(this, bank), (ND_RAM_START+(bank*ND_RAM_BANKSIZE))>>16, ND_RAM_BANKSIZE >> 16);
            write_log("[ND] Slot %i: Mapping main memory bank%d at $%08x: %iMB\n", slot, bank,
                      (ND_RAM_START+(bank*ND_RAM_BANKSIZE)), ConfigureParams.Dimension.board[ND_NUM(slot)].nMemoryBankSize[0]);
        } else {
            bankmask[bank] = 0;
            map_banks(new ND_Empty(this), (ND_RAM_START+(bank*ND_RAM_BANKSIZE))>>16, ND_RAM_BANKSIZE >> 16);
            write_log("[ND] Slot %i: Mapping main memory bank%d at $%08x: empty\n", slot, bank,
                      (ND_RAM_START+(bank*ND_RAM_BANKSIZE)));
        }
    }
    
    write_log("[ND] Slot %i: Mapping video memory at $%08x: %iMB\n", slot,
              ND_VRAM_START, ND_VRAM_SIZE/(1024*1024));
    map_banks(new ND_VRAM(this), ND_VRAM_START>>16, (4*ND_VRAM_SIZE)>>16);
    
	write_log("[ND] Slot %i: Mapping ROM at $%08x: %ikB\n", slot,
              ND_EEPROM_START, ND_EEPROM_SIZE/1024);
	map_banks(new ND_ROM(this), ND_EEPROM_START>>16, (ND_EEPROM_SIZE*8)>>16);
    rom_load();
	
    write_log("[ND] Slot %i: Mapping dither memory and data path at $%08x: %ibyte\n", slot,
              ND_DMEM_START, ND_DMEM_SIZE);
    map_banks(new ND_DMEM(this), ND_DMEM_START>>16, 1);

	write_log("[ND] Slot %i: Mapping IO memory at $%08x\n", slot, ND_IO_START);
	map_banks(new ND_IO(this), ND_IO_START>>16, 1);
    
    write_log("[ND] Slot %i: Mapping RAMDAC registers at $%08x\n", slot, ND_RAMDAC_START);
    map_banks(new ND_RAMDAC(this), ND_RAMDAC_START>>16, 1);
#if ND_STEP
    write_log("[ND] Slot %i: Mapping board CSR at $%08x\n", slot, ND_CSR_START);
    map_banks(new ND_CSR(this), ND_CSR_START>>16, 1);
#endif
}
