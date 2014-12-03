/*
 * linux/include/asm-arm/arch-pxa/pcmcia.h
 *
 * Author:	George Davis
 * Created:	Jan 10, 2002
 * Copyright:	MontaVista Software Inc.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *
 * Originally based upon linux/include/asm-arm/arch-sa1100/pcmcia.h
 *
 * ChangLog:
 *	12-Dec-2002 Lineo Japan, Inc.
 *	26-Feb-2004 Lineo Solutions, Inc.  for Tosa
 */

#ifndef _ASM_ARCH_PCMCIA
#define _ASM_ARCH_PCMCIA


/* Ideally, we'd support up to MAX_SOCK sockets, but PXA250 only
 * provides support for a maximum of two.
 */
#define PXA_PCMCIA_MAX_SOCK   (2)


#ifndef __ASSEMBLY__

struct pcmcia_init {
  void (*handler)(int irq, void *dev, struct pt_regs *regs);
};

struct pcmcia_state {
  unsigned detect: 1,
            ready: 1,
             bvd1: 1,
             bvd2: 1,
           wrprot: 1,
            vs_3v: 1,
            vs_Xv: 1;
};

struct pcmcia_state_array {
  unsigned int size;
  struct pcmcia_state *state;
};

struct pcmcia_configure {
  unsigned sock: 8,
            vcc: 8,
            vpp: 8,
         output: 1,
        speaker: 1,
          reset: 1;
#ifdef CONFIG_ARCH_SHARP_SL
	u_int	flags;
	u_int	masks;
#endif
};

struct pcmcia_irq_info {
  unsigned int sock;
  unsigned int irq;
};

struct pcmcia_low_level {
  int (*init)(struct pcmcia_init *);
  int (*shutdown)(void);
  int (*socket_state)(struct pcmcia_state_array *);
  int (*get_irq_info)(struct pcmcia_irq_info *);
  int (*configure_socket)(const struct pcmcia_configure *);
#ifdef CONFIG_ARCH_SHARP_SL
  int (*socket_init)(int sock);
  int (*socket_suspend)(int sock);
#endif
};

extern struct pcmcia_low_level *pcmcia_low_level;

#endif  /* __ASSEMBLY__ */

#endif
