#ifndef CACHECTRL_H
#define CACHECTRL_H 1

#define CACHECTRL_IC0           1
#define CACHECTRL_IC1           2
#define CACHECTRL_L1_INSN       (CACHECTRL_IC0 | CACHECTRL_IC1)
#define CACHECTRL_DC0           4
#define CACHECTRL_DC1           8
#define CACHECTRL_L1_DATA       (CACHECTRL_DC0 | CACHECTRL_DC1)
#define CACHECTRL_L2            16
#define CACHECTRL_ALL           (CACHECTRL_L1_INSN | CACHECTRL_L1_DATA | \
                                 CACHECTRL_L2)

/* These are for the two-bit L2_CONT_OFF_l2_flush_mode field: we don't know
   what the other values are for.  */
#define L2_FLUSH_MODE_FLUSH             0
#define L2_FLUSH_MODE_INVALIDATE        2

/* These are for L1 sandbox region control.  */
#define L1_SANDBOX_CTRL_EXECUTE         0x01
#define L1_SANDBOX_CTRL_WRITE           0x02
#define L1_SANDBOX_CTRL_READ            0x04
#define L1_SANDBOX_CTRL_PRIV_USER       0x00
#define L1_SANDBOX_CTRL_PRIV_SUPER      0x08
#define L1_SANDBOX_CTRL_PRIV_SECURE     0x18

struct cachestate {
  uint32_t l1_ic0_control;
  uint32_t l1_ic1_control;
  uint32_t l1_d_control;
  uint32_t l2_cont;
  uint32_t l1_ic0_priority;
  uint32_t l1_ic1_priority;
  uint32_t l1_d_priority;
  uint32_t sandbox_start[8];
  uint32_t sandbox_end[8];
};

extern void cachectrl_enable(unsigned flags);
extern void cachectrl_disable(unsigned flags);
extern void cachectrl_flush_range(unsigned flags, void *lo, size_t len);
extern void cachectrl_invalidate_range(unsigned flags, void *lo, size_t len);
extern void cachectrl_flush(unsigned flags);
extern void cachectrl_invalidate(unsigned flags);
extern void cachectrl_savestate(struct cachestate *cs);
extern void cachectrl_restorestate(struct cachestate *cs);

#endif
