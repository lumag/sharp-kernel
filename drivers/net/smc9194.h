/*------------------------------------------------------------------------
 . smc9194.h
 . Copyright (C) 1996 by Erik Stahlman 
 .
 . This software may be used and distributed according to the terms
 . of the GNU General Public License, incorporated herein by reference.
 .
 . This file contains register information and access macros for 
 . the SMC91xxx chipset.   
 . 
 . Information contained in this file was obtained from the SMC91C94 
 . manual from SMC.  To get a copy, if you really want one, you can find 
 . information under www.smc.com in the components division.
 . ( this thanks to advice from Donald Becker ).
 . 
 . Authors
 . 	Erik Stahlman				( erik@vt.edu )
 .
 . History
 . 01/06/96		 Erik Stahlman   moved definitions here from main .c file
 . 01/19/96		 Erik Stahlman	  polished this up some, and added better
 .										  error handling
 .
 ---------------------------------------------------------------------------*/
#ifndef _SMC9194_H_
#define _SMC9194_H_

/* I want some simple types */

typedef unsigned char			byte;
typedef unsigned short			word;
typedef unsigned long int 		dword;


#ifdef CONFIG_ISA
#define SMC_USES_IO_PORT

/* Because of bank switching, the SMC91xxx uses only 16 I/O ports */
#define SMC_IO_EXTENT	16

#define SMC_inb( r )  inb(ioaddr+(r))
#define SMC_inw( r )  inw(ioaddr+(r))
#define SMC_inl( r )  inl(ioaddr+(r))
#define SMC_outb( d, r )  outb((d),ioaddr+(r))
#define SMC_outw( d, r )  outw((d),ioaddr+(r))
#define SMC_outl( d, r )  outl((d),ioaddr+(r))
#define SMC_insb( r, b, l )  insb(ioaddr+(r),(b),(l))
#define SMC_insw( r, b, l )  insw(ioaddr+(r),(b),(l))
#define SMC_insl( r, b, l )  insl(ioaddr+(r),(b),(l))
#define SMC_outsb( r, b, l )  outsb(ioaddr+(r),(b),(l))
#define SMC_outsw( r, b, l )  outsw(ioaddr+(r),(b),(l))
#define SMC_outsl( r, b, l )  outsl(ioaddr+(r),(b),(l))

#endif

#if defined(CONFIG_SA1100_GRAPHICSCLIENT) || defined(CONFIG_SA1100_PFS168) || defined(CONFIG_SA1100_FLEXANET)
#define SMC_USES_16BIT_MEM
/* 
 * We can only do 16-bit reads and writes in the static memory space.
 * Byte reads must read 16 bits and return the correct byte.
 * Byte writes must do a read/modify/write.
 */

/* The first two address lines aren't connected... */
#define SMC_IO_EXTENT	(16<<2)

#define SMC_inw( r )  (*((volatile word *)(ioaddr+((r)<<2))))
#define SMC_inb( r )  (((r)&1) ? SMC_inw((r)&~1)>>8 : SMC_inw(r)&0xFF)

#define SMC_outw( d, r )  (*((volatile word *)(ioaddr+((r)<<2))) = d)
#define SMC_outb( d, r )  \
    ({	word __d = (byte)(d);  \
	word __w = SMC_inw((r)&~1);  \
	__w &= ((r)&1) ? 0x00FF : 0xFF00;  \
	__w |= ((r)&1) ? __d<<8 : __d;  \
	SMC_outw(__w,(r)&~1);  })

/* We remove PCIO_BASE that is added in insw()/outsw() */
#define SMC_insw( r, b, l )  insw(ioaddr+((r)<<2)-PCIO_BASE,(b),(l))
#define SMC_outsw( r, b, l )  outsw(ioaddr+((r)<<2)-PCIO_BASE,(b),(l))

#endif

#if defined(CONFIG_ASSABET_NEPONSET)
#define SMC_USES_8BIT_MEM
/* 
 * We can only do 8-bit reads and writes in the static memory space.
 * word access must splice two byte access together
 */

/* The first two address lines aren't connected... */
#define SMC_IO_EXTENT	(16<<2)

#define SMC_inb( r )  (*((volatile byte *)(ioaddr+((r)<<2))))

#define SMC_outb( d, r )  (*((volatile byte *)(ioaddr+((r)<<2))) = (byte)(d))

/*#define IODELAY udelay(1)*/
#define IODELAY

static inline unsigned short __SMC_inw(unsigned ioaddr, int r)
{
  unsigned char b1, b2;
  IODELAY;
  b1 = SMC_inb(r);
  IODELAY;
  b2 = SMC_inb(r+1);
  return (b2 << 8) | b1;
}
#define SMC_inw(r) __SMC_inw(ioaddr, r)

#define SMC_outw( d, r )  \
		do { \
		     IODELAY;\
                     SMC_outb((d), r); \
		     IODELAY;\
		     SMC_outb((d)>>8, (r)+1);} while (0)

/* We remove PCIO_BASE that is added in insb()/outsb() */
#define SMC_insw( r, b, l )  insb(ioaddr+((r)<<2)-PCIO_BASE,(b),(l)<<1)
#define SMC_outsw( r, b, l )  outsb(ioaddr+((r)<<2)-PCIO_BASE,(b),(l)<<1)

#endif


/*---------------------------------------------------------------
 .  
 . A description of the SMC registers is probably in order here,
 . although for details, the SMC datasheet is invaluable.  
 . 
 . Basically, the chip has 4 banks of registers ( 0 to 3 ), which
 . are accessed by writing a number into the BANK_SELECT register
 . ( I also use a SMC_SELECT_BANK macro for this ).
 . 
 . The banks are configured so that for most purposes, bank 2 is all
 . that is needed for simple run time tasks.  
 -----------------------------------------------------------------------*/

/*
 . Bank Select Register: 
 .
 .		yyyy yyyy 0000 00xx  
 .		xx 		= bank number
 .		yyyy yyyy	= 0x33, for identification purposes.
*/
#define	BANK_SELECT		14

/* BANK 0  */

#define	TCR 		0    	/* transmit control register */
#define TCR_ENABLE	0x0001	/* if this is 1, we can transmit */ 
#define TCR_FDUPLX    	0x0800  /* receive packets sent out */
#define TCR_STP_SQET	0x1000	/* stop transmitting if Signal quality error */
#define	TCR_MON_CNS	0x0400	/* monitors the carrier status */
#define	TCR_PAD_ENABLE	0x0080	/* pads short packets to 64 bytes */

#define	TCR_CLEAR	0	/* do NOTHING */
/* the normal settings for the TCR register : */ 
/* QUESTION: do I want to enable padding of short packets ? */
#define	TCR_NORMAL  	TCR_ENABLE 


#define EPH_STATUS	2
#define ES_LINK_OK	0x4000	/* is the link integrity ok ? */

#define	RCR		4
#define RCR_SOFTRESET	0x8000 	/* resets the chip */	
#define	RCR_STRIP_CRC	0x200	/* strips CRC */
#define RCR_ENABLE	0x100	/* IFF this is set, we can receive packets */
#define RCR_ALMUL	0x4 	/* receive all multicast packets */
#define	RCR_PROMISC	0x2	/* enable promiscuous mode */

/* the normal settings for the RCR register : */
#define	RCR_NORMAL	(RCR_STRIP_CRC | RCR_ENABLE)
#define RCR_CLEAR	0x0		/* set it to a base state */

#define	COUNTER		6
#define	MIR		8
#define	MCR		10
/* 12 is reserved */

/* BANK 1 */
#define CONFIG			0
#define CFG_AUI_SELECT	 	0x100
#define	BASE			2
#define	ADDR0			4
#define	ADDR1			6
#define	ADDR2			8
#define	GENERAL			10
#define	CONTROL			12
#define	CTL_POWERDOWN		0x2000
#define	CTL_LE_ENABLE		0x80
#define	CTL_CR_ENABLE		0x40
#define	CTL_TE_ENABLE		0x0020
#define CTL_AUTO_RELEASE	0x0800
#define	CTL_EPROM_ACCESS	0x0003 /* high if Eprom is being read */

/* BANK 2 */
#define MMU_CMD		0
#define MC_BUSY		1	/* only readable bit in the register */
#define MC_NOP		0
#define	MC_ALLOC	0x20  	/* or with number of 256 byte packets */
#define	MC_RESET	0x40	
#define	MC_REMOVE	0x60  	/* remove the current rx packet */
#define MC_RELEASE  	0x80  	/* remove and release the current rx packet */
#define MC_FREEPKT  	0xA0  	/* Release packet in PNR register */
#define MC_ENQUEUE	0xC0 	/* Enqueue the packet for transmit */
 	
#define	PNR_ARR		2
#define FIFO_PORTS	4

#define FP_RXEMPTY  0x8000
#define FP_TXEMPTY  0x80

#define	POINTER		6
#define PTR_READ	0x2000
#define	PTR_RCV		0x8000
#define	PTR_AUTOINC 	0x4000
#define PTR_AUTO_INC	0x0040

#define	DATA_1		8
#define	DATA_2		10
#define	INTERRUPT	12

#define INT_MASK	13
#define IM_RCV_INT	0x1
#define	IM_TX_INT	0x2
#define	IM_TX_EMPTY_INT	0x4	
#define	IM_ALLOC_INT	0x8
#define	IM_RX_OVRN_INT	0x10
#define	IM_EPH_INT	0x20
#define	IM_ERCV_INT	0x40 /* not on SMC9192 */		

/* BANK 3 */
#define	MULTICAST1	0
#define	MULTICAST2	2
#define	MULTICAST3	4
#define	MULTICAST4	6
#define	MGMT		8
#define	REVISION	10 /* ( hi: chip id   low: rev # ) */


/* this is NOT on SMC9192 */
#define	ERCV		12

#define CHIP_9190	3
#define CHIP_9194	4
#define CHIP_9195	5
#define CHIP_9196	6
#define CHIP_91100	7

static const char * chip_ids[ 15 ] =  { 
	NULL, NULL, NULL, 
	/* 3 */ "SMC91C90/91C92",
	/* 4 */ "SMC91C94",
	/* 5 */ "SMC91C95",
	/* 6 */ "SMC91C96",	/* can't really happen, found via rev */
	/* 7 */ "SMC91C100", 
	/* 8 */ "SMC91C100FD", 
	/* 9 */ "SMC91C110", 
	NULL, NULL, 
	NULL, NULL, NULL};  

/* 
 . Transmit status bits 
*/
#define TS_SUCCESS 0x0001
#define TS_LOSTCAR 0x0400
#define TS_LATCOL  0x0200
#define TS_16COL   0x0010

/*
 . Receive status bits
*/
#define RS_ALGNERR	0x8000
#define RS_BADCRC	0x2000
#define RS_ODDFRAME	0x1000
#define RS_TOOLONG	0x0800
#define RS_TOOSHORT	0x0400
#define RS_MULTICAST	0x0001
#define RS_ERRORS	(RS_ALGNERR | RS_BADCRC | RS_TOOLONG | RS_TOOSHORT) 

static const char * interfaces[ 2 ] = { "TP", "AUI" };

/* SMC9196 ethernet config & status regs */
/* requires A25 be set with offset of 0x8000<<2 */
#define LAN91C96_ECOR	0x0
#define LAN91C96_ECSR	0x2

#define ECOR_RESET	0x80
#define ECOR_LEVEL_IRQ	0x40
#define ECOR_WR_ATTRIB	0x02
#define ECOR_ENABLE	0x01

#define ECSR_IOIS8	0x20
#define ECSR_PWRDWN	0x04
#define ECSR_INT	0x02

/*-------------------------------------------------------------------------
 .  I define some macros to make it easier to do somewhat common
 . or slightly complicated, repeated tasks. 
 --------------------------------------------------------------------------*/

/* select a register bank, 0 to 3  */

#define SMC_SELECT_BANK(x)  { SMC_outw( x, BANK_SELECT ); } 

/* define a small delay for the reset */
#define SMC_DELAY() { SMC_inw( RCR );\
			SMC_inw( RCR );\
			SMC_inw( RCR );  }

/* this enables an interrupt in the interrupt mask register */
#define SMC_ENABLE_INT(x) {\
		word mask;\
		SMC_SELECT_BANK(2);\
		mask = SMC_inw( INTERRUPT ) & 0xFF00;\
		mask |= ((x) << 8);\
		SMC_outw( mask, INTERRUPT );\
}

/* this disables an interrupt from the interrupt mask register */

#define SMC_DISABLE_INT(x) {\
		word mask;\
		SMC_SELECT_BANK(2);\
		mask = SMC_inw( INTERRUPT ) & 0xFF00;\
		mask &= ~((x) << 8);\
		SMC_outw( mask, INTERRUPT );\
}

/* this sets the absolute interrupt mask */

#define SMC_SET_INT(x) {\
		SMC_SELECT_BANK(2);\
		SMC_outw( ((x) << 8), INTERRUPT );\
}

/*----------------------------------------------------------------------
 . Define the interrupts that I want to receive from the card
 . 
 . I want: 
 .  IM_EPH_INT, for nasty errors
 .  IM_RCV_INT, for happy received packets
 .  IM_RX_OVRN_INT, because I have to kick the receiver
 --------------------------------------------------------------------------*/
#define SMC_INTERRUPT_MASK   (IM_EPH_INT | IM_RX_OVRN_INT | IM_RCV_INT) 

#endif  /* _SMC_9194_H_ */

