#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

Uint32 nbic_reg_lget(Uint32 addr);
Uint32 nbic_reg_wget(Uint32 addr);
Uint32 nbic_reg_bget(Uint32 addr);
void nbic_reg_lput(Uint32 addr, Uint32 val);
void nbic_reg_wput(Uint32 addr, Uint32 val);
void nbic_reg_bput(Uint32 addr, Uint32 val);

Uint32 nb_cpu_slot_lget(Uint32 addr);
Uint16 nb_cpu_slot_wget(Uint32 addr);
Uint8 nb_cpu_slot_bget(Uint32 addr);
void nb_cpu_slot_lput(Uint32 addr, Uint32 l);
void nb_cpu_slot_wput(Uint32 addr, Uint16 w);
void nb_cpu_slot_bput(Uint32 addr, Uint8 b);

#ifdef __cplusplus
}
#endif /* __cplusplus */
