/*=============================================================================
Copyright (C) 2016 Kristina Brooks
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
VideoCoreIV first stage bootloader.

=============================================================================*/

#include <common.h>
#include <hardware.h>
#include <cachectrl.h>
#include <cpu.h>

uint32_t g_CPUID;

void uart_putc(unsigned int ch)
{
	while(1) {
		if (mmio_read32(AUX_MU_LSR_REG) & 0x20)
			break;
	}
	mmio_write32(AUX_MU_IO_REG, ch);
}

static int uart_data_available(void)
{
  int havedata = mmio_read32(AUX_MU_LSR_REG);
  return havedata & 1;
}

static unsigned char uart_getc(void)
{
  while (!uart_data_available ())
    /* empty */;

  return mmio_read32(AUX_MU_IO_REG);
}

void uart_init(void) {
	unsigned int ra = GP_FSEL1;
	ra &= ~(7 << 12);
	ra |= 2 << 12;
        ra &= ~(7 << 15);
        ra |= 2 << 15;
	GP_FSEL1 = ra;

	GP_PUD = 0;

	udelay(150);
	GP_PUDCLK0 = (1 << 14) | (1 << 15);
	udelay(150);

	GP_PUDCLK0 = 0;

	mmio_write32(AUX_ENABLES, 1);
	mmio_write32(AUX_MU_IER_REG, 0);
	mmio_write32(AUX_MU_CNTL_REG, 0);
	mmio_write32(AUX_MU_LCR_REG, 3);
	mmio_write32(AUX_MU_MCR_REG, 0);
	mmio_write32(AUX_MU_IER_REG, 0);
	mmio_write32(AUX_MU_IIR_REG, 0xC6);

	mmio_write32(AUX_MU_BAUD_REG, 270);

	mmio_write32(AUX_MU_LCR_REG, 3);
	mmio_write32(AUX_MU_CNTL_REG, 3);
}

void led_init(void) {
	unsigned int ra;

	ra = GP_FSEL1;
	ra &= ~(7 << 18);
	ra |= 1 << 18;

	GP_FSEL1 = ra;
}

/*
  #define CM_PLLC_DIGRST_BITS                                9:9
  #define CM_PLLC_DIGRST_SET                                 0x00000200
  #define CM_PLLC_ANARST_BITS                                8:8
  #define CM_PLLC_ANARST_SET                                 0x00000100
  #define CM_PLLC_HOLDPER_BITS                               7:7
  #define CM_PLLC_HOLDPER_SET                                0x00000080
  #define CM_PLLC_LOADPER_BITS                               6:6
  #define CM_PLLC_LOADPER_SET                                0x00000040
  #define CM_PLLC_HOLDCORE2_BITS                             5:5
  #define CM_PLLC_HOLDCORE2_SET                              0x00000020
  #define CM_PLLC_LOADCORE2_BITS                             4:4
  #define CM_PLLC_LOADCORE2_SET                              0x00000010
  #define CM_PLLC_HOLDCORE1_BITS                             3:3
  #define CM_PLLC_HOLDCORE1_SET                              0x00000008
  #define CM_PLLC_LOADCORE1_BITS                             2:2
  #define CM_PLLC_LOADCORE1_SET                              0x00000004
  #define CM_PLLC_HOLDCORE0_BITS                             1:1
  #define CM_PLLC_HOLDCORE0_SET                              0x00000002
  #define CM_PLLC_LOADCORE0_BITS                             0:0
  #define CM_PLLC_LOADCORE0_SET                              0x00000001
*/

void switch_vpu_to_pllc() {
	A2W_XOSC_CTRL |= A2W_PASSWORD | A2W_XOSC_CTRL_PLLCEN_SET;

	A2W_PLLC_FRAC = A2W_PASSWORD | 87380;
	A2W_PLLC_CTRL = A2W_PASSWORD | 52 | 0x1000;
	
	A2W_PLLC_ANA3 = A2W_PASSWORD | 0x100;
	A2W_PLLC_ANA2 = A2W_PASSWORD | 0x0;
	A2W_PLLC_ANA1 = A2W_PASSWORD | 0x144000;
	A2W_PLLC_ANA0 = A2W_PASSWORD | 0x0;

	CM_PLLC = CM_PASSWORD | CM_PLLC_DIGRST_SET;

	/* hold all */
	CM_PLLC = CM_PASSWORD | CM_PLLC_DIGRST_SET |
		CM_PLLC_HOLDPER_SET | CM_PLLC_HOLDCORE2_SET |
		CM_PLLC_HOLDCORE1_SET | CM_PLLC_HOLDCORE0_SET;

	A2W_PLLC_DIG3 = A2W_PASSWORD | 0x0;
	A2W_PLLC_DIG2 = A2W_PASSWORD | 0x400000;
	A2W_PLLC_DIG1 = A2W_PASSWORD | 0x5;
	A2W_PLLC_DIG0 = A2W_PASSWORD | 52 | 0x555000;

	A2W_PLLC_CTRL = A2W_PASSWORD | 52 | 0x1000 | A2W_PLLC_CTRL_PRSTN_SET;

	A2W_PLLC_DIG3 = A2W_PASSWORD | 0x42;
	A2W_PLLC_DIG2 = A2W_PASSWORD | 0x500401;
	A2W_PLLC_DIG1 = A2W_PASSWORD | 0x4005;
	A2W_PLLC_DIG0 = A2W_PASSWORD | 52 | 0x555000;

	A2W_PLLC_CORE0 = A2W_PASSWORD | 2;

	CM_PLLC = CM_PASSWORD | CM_PLLC_DIGRST_SET |
		CM_PLLC_HOLDPER_SET | CM_PLLC_HOLDCORE2_SET |
		CM_PLLC_HOLDCORE1_SET | CM_PLLC_HOLDCORE0_SET | CM_PLLC_LOADCORE0_SET;

	CM_PLLC = CM_PASSWORD | CM_PLLC_DIGRST_SET |
		CM_PLLC_HOLDPER_SET | CM_PLLC_HOLDCORE2_SET |
		CM_PLLC_HOLDCORE1_SET | CM_PLLC_HOLDCORE0_SET;

	CM_PLLC = CM_PASSWORD | CM_PLLC_DIGRST_SET |
		CM_PLLC_HOLDPER_SET | CM_PLLC_HOLDCORE2_SET |
		CM_PLLC_HOLDCORE1_SET;

	CM_VPUCTL = CM_PASSWORD | CM_VPUCTL_FRAC_SET | CM_SRC_OSC | CM_VPUCTL_GATE_SET;
	CM_VPUDIV = CM_PASSWORD | (4 << 12);
	CM_VPUCTL = CM_PASSWORD | CM_SRC_PLLC_CORE0 | CM_VPUCTL_GATE_SET;
	CM_VPUCTL = CM_PASSWORD | CM_SRC_PLLC_CORE0 | CM_VPUCTL_GATE_SET | 0x10; /* ENAB */

	CM_TIMERDIV = CM_PASSWORD | (19 << 12) | 819;
	CM_TIMERCTL = CM_PASSWORD | CM_SRC_OSC | 0x10;
}

extern void sdram_init();
extern void arm_init();
extern void monitor_start();

void print_crap() {
	printf("TB_BOOT_OPT = 0x%X\n", TB_BOOT_OPT);
}

static unsigned crc16(unsigned crc, unsigned ch) {
  crc ^= ch << 8;
  for (unsigned j = 0; j < 8; j++)
    if (crc & 0x8000)
      crc = (crc << 1) ^ 0x1021;
    else
      crc <<= 1;
  return crc;
}

static void eat_bad_data(void)
{
  const int max_repeats = 10000000;
  while (1) {
    int repeat;
    for (repeat = 0; repeat < max_repeats; repeat++)
      if (uart_data_available())
        break;
    if (repeat == max_repeats)
      break;
    uart_getc();
  }
}

#ifdef TESTHARNESS_ONLY
static int xmodem_receive(unsigned int startaddr) {
  unsigned char *out = (unsigned char *) startaddr;
  unsigned char *packet_start = out;
  unsigned int expect_packet = 1;
  unsigned int crc = 0;
  unsigned int crc_mismatches = 0;
  unsigned int block_mismatches = 0;
  unsigned int last_seq, last_invseq;
  unsigned int last_sender_crc;
  unsigned int bad_commands = 0;
  unsigned int acks_sent = 0;
  int have_first_packet = false;

  eat_bad_data();

  while (1) {

    if (!have_first_packet) {
      // Send initial NAK
      uart_putc('C');
      while (!uart_data_available()) {
        const int max_repeats = 10000000;
        int repeat;
        for (repeat = 0; repeat < max_repeats; repeat++)
          if (uart_data_available()) 
            break;
        if (repeat == max_repeats)
          uart_putc('C');
      }
    }

    // Get a packet
    unsigned int firstbyte = uart_getc(), i;
    if (firstbyte == 1 || firstbyte == 2) {
      int blocklen = firstbyte == 1 ? 128 : 1024;
      crc = 0;
      unsigned int seqno = uart_getc();
      unsigned int inv_seqno = uart_getc();
      last_seq = seqno;
      last_invseq = inv_seqno;
      out = packet_start;
      for (i = 0; i < blocklen; i++) {
        int ch = uart_getc();
        *out++ = ch;
        crc = crc16(crc, ch);
      }
      // skip these bytes.
      unsigned int sender_crc_hi = uart_getc();
      unsigned int sender_crc_lo = uart_getc();
      unsigned int sender_crc = (sender_crc_hi << 8) | sender_crc_lo;
      last_sender_crc = sender_crc;
      if (sender_crc == (crc & 0xffff)
          && expect_packet == seqno
          && expect_packet == (inv_seqno ^ 0xff)) {
        uart_putc(6); // ack
        expect_packet = (expect_packet + 1) & 0xff;
        packet_start += blocklen;
        acks_sent++;
      } else {
        if (sender_crc != (crc & 0xffff))
          crc_mismatches++;
        if (expect_packet != seqno
            || expect_packet != (inv_seqno ^ 0xff))
          block_mismatches++;
        uart_putc(21); // nak
      }
      // Either a good or a bad one...
      have_first_packet = true;
    } else if (firstbyte == 4) {
      // Send ACK on EOT.
      uart_putc(6);
      break;
    } else {
      bad_commands++;
      eat_bad_data();
      if (have_first_packet)
        uart_putc(21);
    }
  }

  if (crc_mismatches || block_mismatches) {
    printf("*** crc mismatches: %d\n", crc_mismatches);
    printf("*** block mismatches: %d\n", block_mismatches);
    printf("*** last seq: %d\n", last_seq);
    printf("*** last inv seq: %d\n", last_invseq);
    printf("*** expected packet: %d\n", expect_packet);
    printf("*** last crc: %x\n", crc & 0xffff);
    printf("*** last sender crc: %x\n", (int)last_sender_crc);
    printf("*** bad commands: %d\n", bad_commands);
    printf("*** acks sent: %d\n", acks_sent);
  }

  return 1;
}

bool have_restart_point = false;
jmp_buf restart_shell;
jmp_buf postcopy_catcher;

static void return_from_code(void) {
  longjmp(restart_shell, 2);
}

typedef void (*fn_returner)(void);

static int run_code(unsigned int startaddr) {
  int (*fn)(fn_returner) = (int (*)(fn_returner)) startaddr;
  return fn(&return_from_code);
}

__attribute__((noinline)) static void *where_are_we(void) {
  return __builtin_return_address(0);
}

void exception_recover(void) {
  if (have_restart_point)
    longjmp(restart_shell, 1);

  printf("No recovery point! Hanging now.\n");

  hang_cpu();
}

#define LOAD_BASE 0x20000

#endif

int _main(unsigned int cpuid, unsigned int load_address) {
	switch_vpu_to_pllc();

	led_init();
	uart_init();

	printf(
		"=========================================================\n"
		"::\n"
		":: kFW for bcm2708, Copyright 2016, Kristina Brooks. \n"
		"::\n"
		":: BUILDATE  : %s %s \n"
		":: BUILDSTYLE: %s \n"
		"::\n"
		"=========================================================\n",
		__DATE__, __TIME__,
		"OPENSOURCE"
	);

	printf("CPUID    = 0x%X\n", cpuid);
	printf("LoadAddr = 0x%X\n", load_address);

	print_crap();

	g_CPUID = cpuid;

	/* bring up SDRAM */
	sdram_init();
	printf("SDRAM initialization completed successfully!\n");

#ifndef TESTHARNESS_ONLY
	/* bring up ARM */
	arm_init();

	/* start vpu monitor */
	monitor_start();

	panic("main exiting!");
#else
        printf("Attempting relocation...\n");
        switch (setjmp(postcopy_catcher)) {
          case 0:
            /* Copy boot code to start at 0xc0000000 + 128k.  */
            postcopy_catcher[26] += 0x40000000 + 128 * 1024;
            memcpy((void *) (0xc0000000 + 128 * 1024), (void *) 0x80000000,
                   128 * 1024);
            cachectrl_flush(CACHECTRL_L1_DATA);
            cachectrl_invalidate_range(CACHECTRL_L1_INSN,
                                       (void*) (0xc0000000 + 128 * 1024),
                                       128 * 1024);
            longjmp(postcopy_catcher, 1);
          case 1:
            printf("Middle state from uncached SDRAM: continue from 0x%x\n",
                   (int) where_are_we());
            /* Now we're running from the uncached alias, we can turn on L2
               cache safely.  */
            cachectrl_enable(CACHECTRL_IC0 | CACHECTRL_DC0 | CACHECTRL_L2);
            postcopy_catcher[26] -= 0xc0000000 + 128 * 1024;
            /* ...and copy the boot code back to where it's supposed to be.  */
            memcpy((void *) 0, (void *) (0xc0000000 + 128 * 1024), 128 * 1024);
            cachectrl_flush(CACHECTRL_L1_DATA);
            cachectrl_invalidate_range(CACHECTRL_L1_INSN, (void*) 0,
                                       128 * 1024);
            longjmp(postcopy_catcher, 2);
          case 2:
            printf("Successfully relocated boot code! (Continuing from 0x%x)\n",
                   (int) where_are_we());
        }

        switch (setjmp(restart_shell)) {
          case 0:
            printf(">>> Test harness starting\n");
            have_restart_point = true;
            break;
          case 1:
            printf(">>> Test harness restarting after exception\n");
            break;
          case 2:
            printf(">>> Test harness restarting after program return\n");
            break;
        }

        while (1) {
          int ch = uart_getc();
          switch (ch) {
            case 'L':
              {
                printf("Receive file by XMODEM.\n");
                int success = xmodem_receive(LOAD_BASE);
                if (success)
                  printf("Success!\n");
                else
                  printf("Tranfer failed.\n");
              }
              break;
            case 'G':
              {
                int rc;
                char *targ = (char *) LOAD_BASE;
                cachectrl_flush(CACHECTRL_L1_DATA);
                cachectrl_invalidate_range(CACHECTRL_L1_INSN, targ,
                                           256 * 1024 * 1024 - LOAD_BASE);
                if (targ[0] == 0x19
                    && targ[1] == 0xe8
                    && targ[2] == 0xfc
                    && targ[3] == 0xff
                    && targ[4] == 0xff
                    && targ[5] == 0x0f) {
                  printf("Signature looks OK, starting program!\n");
                  rc = run_code(LOAD_BASE);
                  printf("Program returned directly (%d)\n", rc);
                  /* Probably register state is all screwed up, so do a
                     longjmp.  */
                  longjmp(restart_shell, 2);
                } else {
                  printf("Signature looks bad, doing nothing.\n");
                }
              }
              break;
            case '?':
              printf ("OK.\n");
              break;
            default:
              printf ("Unknown command: '%c'\n", ch);
          }
        }
#endif
}

