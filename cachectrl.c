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
