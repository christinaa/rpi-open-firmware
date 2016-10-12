/*=============================================================================
Copyright (C) 2016 Julian Brown
All rights reserved.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

FILE DESCRIPTION
Cache control (enable, disable, flushing, invalidation).

=============================================================================*/

#include <common.h>
#include <hardware.h>
#include <cachectrl.h>

static int int_disable(void) {
        int enabled_previously;
        __asm__ __volatile__ ("mov %0,sr\n\tdi" : "=r" (enabled_previously));
        return (enabled_previously & (1 << 30)) != 0;
}

static void int_enable(int enabled_previously) {
        if (enabled_previously)
                __asm__ __volatile__ ("ei");
}

void cachectrl_enable(unsigned flags) {
        int was_enabled = int_disable();
        if (flags & CACHECTRL_IC0) {
                L1_IC0_CONTROL &= L1_IC0_CONTROL_DISABLE_CLR;
        }
        if (flags & CACHECTRL_IC1) {
                L1_IC1_CONTROL &= L1_IC1_CONTROL_DISABLE_CLR;
        }
        if (flags & CACHECTRL_L2) {
                L2_CONT_OFF &= L2_CONT_OFF_l2_disable_CLR;
        }
        if (flags & CACHECTRL_DC0) {
                L1_D_CONTROL &= L1_D_CONTROL_DC_DISABLE_CLR;
        }
        if (flags & CACHECTRL_DC1) {
                L1_D_CONTROL &= L1_D_CONTROL_DC_DISABLE_CLR;
        }
        int_enable(was_enabled);
}

void cachectrl_disable(unsigned flags) {
        int was_enabled = int_disable();
        if (flags & CACHECTRL_IC0) {
                L1_IC0_CONTROL |= L1_IC0_CONTROL_DISABLE_SET;
        }
        if (flags & CACHECTRL_IC1) {
                L1_IC1_CONTROL |= L1_IC1_CONTROL_DISABLE_SET;
        }
        if (flags & CACHECTRL_L2) {
                L2_CONT_OFF |= L2_CONT_OFF_l2_disable_SET;
        }
        if (flags & CACHECTRL_DC0) {
                L1_D_CONTROL |= L1_D_CONTROL_DC_DISABLE_SET;
        }
        if (flags & CACHECTRL_DC1) {
                L1_D_CONTROL |= L1_D_CONTROL_DC_DISABLE_SET;
        }
        int_enable(was_enabled);
}

static void cachectrl_range_op(unsigned flags, void *lo, size_t len,
                               unsigned op) {
        void *hi = (void*) &((char *)lo)[len - 1];
        int was_enabled = int_disable();
        L1_D_FLUSH_S = (intptr_t) lo;
        L1_D_FLUSH_E = (intptr_t) hi;
        /*L1_IC0_FLUSH_S = (intptr_t) lo;
        L1_IC0_FLUSH_E = (intptr_t) hi;
        L1_IC1_FLUSH_S = (intptr_t) lo;
        L1_IC1_FLUSH_E = (intptr_t) hi;*/
        L2_FLUSH_STA = (intptr_t) lo;
        L2_FLUSH_END = (intptr_t) hi;
        if (flags & CACHECTRL_L1_INSN) {
                uint32_t tmp1, tmp2;
                if (flags & CACHECTRL_IC0)
                        L1_IC0_CONTROL |= L1_IC0_CONTROL_START_FLUSH_SET;
                if (flags & CACHECTRL_IC1)
                        L1_IC1_CONTROL |= L1_IC1_CONTROL_START_FLUSH_SET;
                tmp1 = L1_IC0_CONTROL;
                tmp2 = L1_IC1_CONTROL;
                tmp1 += tmp2;
                __asm__ __volatile__ ("nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop"
                                      : : "r" (tmp1));
        }
        if (flags & CACHECTRL_L1_DATA) {
                uint32_t tmp = L1_D_CONTROL;
                if (flags & CACHECTRL_DC0)
                        tmp |= L1_D_CONTROL_DC0_FLUSH_SET;
                if (flags & CACHECTRL_DC1)
                        tmp |= L1_D_CONTROL_DC1_FLUSH_SET;
                L1_D_CONTROL = tmp;
                while (L1_D_CONTROL & (L1_D_CONTROL_DC0_FLUSH_SET |
                                       L1_D_CONTROL_DC1_FLUSH_SET))
                        /* empty */;
        }
        if (flags & CACHECTRL_L2) {
                uint32_t tmp = L2_CONT_OFF;
                tmp &= L2_CONT_OFF_l2_flush_mode_CLR;
                tmp |= op << L2_CONT_OFF_l2_flush_mode_LSB;
                tmp |= L2_CONT_OFF_l2_flush_SET;
                L2_CONT_OFF = tmp;
                while (L2_CONT_OFF & L2_CONT_OFF_l2_flush_SET)
                        /* empty */;
        }
        int_enable(was_enabled);
}

void cachectrl_flush_range(unsigned flags, void *lo, size_t len) {
        cachectrl_range_op(flags, lo, len, L2_FLUSH_MODE_FLUSH);
}

void cachectrl_invalidate_range(unsigned flags, void *lo, size_t len) {
        cachectrl_range_op(flags, lo, len, L2_FLUSH_MODE_INVALIDATE);
}

void cachectrl_flush(unsigned flags) {
        cachectrl_flush_range(flags, (void*) 0, 0xffffffffu);
}

void cachectrl_invalidate(unsigned flags) {
        cachectrl_invalidate_range(flags, (void*) 0, 0xffffffffu);
}

void cachectrl_savestate(struct cachestate *cs) {
        cs->l1_ic0_control = L1_IC0_CONTROL & L1_IC0_CONTROL_START_FLUSH_CLR;
        cs->l1_ic1_control = L1_IC1_CONTROL & L1_IC1_CONTROL_START_FLUSH_CLR;
        cs->l1_d_control = L1_D_CONTROL & L1_D_CONTROL_DC0_FLUSH_CLR
                                        & L1_D_CONTROL_DC1_FLUSH_CLR;
        cs->l2_cont = L2_CONT_OFF & L2_CONT_OFF_l2_flush_CLR;
        cs->l1_ic0_priority = L1_IC0_PRIORITY;
        cs->l1_ic1_priority = L1_IC1_PRIORITY;
        cs->l1_d_priority = L1_D_PRIORITY;
        cs->sandbox_start[0] = L1_L1_SANDBOX_START0;
        cs->sandbox_start[1] = L1_L1_SANDBOX_START1;
        cs->sandbox_start[2] = L1_L1_SANDBOX_START2;
        cs->sandbox_start[3] = L1_L1_SANDBOX_START3;
        cs->sandbox_start[4] = L1_L1_SANDBOX_START4;
        cs->sandbox_start[5] = L1_L1_SANDBOX_START5;
        cs->sandbox_start[6] = L1_L1_SANDBOX_START6;
        cs->sandbox_start[7] = L1_L1_SANDBOX_START7;
        cs->sandbox_end[0] = L1_L1_SANDBOX_END0;
        cs->sandbox_end[1] = L1_L1_SANDBOX_END1;
        cs->sandbox_end[2] = L1_L1_SANDBOX_END2;
        cs->sandbox_end[3] = L1_L1_SANDBOX_END3;
        cs->sandbox_end[4] = L1_L1_SANDBOX_END4;
        cs->sandbox_end[5] = L1_L1_SANDBOX_END5;
        cs->sandbox_end[6] = L1_L1_SANDBOX_END6;
        cs->sandbox_end[7] = L1_L1_SANDBOX_END7;
}

static void enable_or_disable(uint32_t current, uint32_t disable_bits,
                              uint32_t desired, unsigned cachebits) {
        if (current & disable_bits) {
                if (!(desired & disable_bits))
                        cachectrl_enable(cachebits);
        } else {
                if (desired & disable_bits)
                        cachectrl_disable(cachebits);
        }
}

void cachectrl_restorestate(struct cachestate *cs) {
        enable_or_disable(L1_IC0_CONTROL, L1_IC0_CONTROL_DISABLE_SET,
                          cs->l1_ic0_control, CACHECTRL_IC0);
        enable_or_disable(L1_IC1_CONTROL, L1_IC1_CONTROL_DISABLE_SET,
                          cs->l1_ic1_control, CACHECTRL_IC1);
        enable_or_disable(L1_D_CONTROL, L1_D_CONTROL_DC_DISABLE_SET,
                          cs->l1_d_control, CACHECTRL_L1_DATA);
        enable_or_disable(L2_CONT_OFF, L2_CONT_OFF_l2_disable_SET,
                          cs->l2_cont, CACHECTRL_L2);
        L1_IC0_CONTROL = cs->l1_ic0_control;
        L1_IC1_CONTROL = cs->l1_ic1_control;
        L1_D_CONTROL = cs->l1_d_control;
        L1_IC0_PRIORITY = cs->l1_ic0_priority;
        L1_IC1_PRIORITY = cs->l1_ic1_priority;
        L1_D_PRIORITY = cs->l1_d_priority;
        L2_CONT_OFF = cs->l2_cont;
        L1_L1_SANDBOX_START0 = cs->sandbox_start[0];
        L1_L1_SANDBOX_START1 = cs->sandbox_start[1];
        L1_L1_SANDBOX_START2 = cs->sandbox_start[2];
        L1_L1_SANDBOX_START3 = cs->sandbox_start[3];
        L1_L1_SANDBOX_START4 = cs->sandbox_start[4];
        L1_L1_SANDBOX_START5 = cs->sandbox_start[5];
        L1_L1_SANDBOX_START6 = cs->sandbox_start[6];
        L1_L1_SANDBOX_START7 = cs->sandbox_start[7];
        L1_L1_SANDBOX_END0 = cs->sandbox_end[0];
        L1_L1_SANDBOX_END1 = cs->sandbox_end[1];
        L1_L1_SANDBOX_END2 = cs->sandbox_end[2];
        L1_L1_SANDBOX_END3 = cs->sandbox_end[3];
        L1_L1_SANDBOX_END4 = cs->sandbox_end[4];
        L1_L1_SANDBOX_END5 = cs->sandbox_end[5];
        L1_L1_SANDBOX_END6 = cs->sandbox_end[6];
        L1_L1_SANDBOX_END7 = cs->sandbox_end[7];
}
