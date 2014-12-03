/*
 * Glue audio driver for a simple DAC on the SA1100's SSP port
 * 
 * Copyright (c) 2001 Nicolas Pitre <nico@cam.org>
 *
 * This program is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License.
 *
 * History:
 *
 * 2001-06-04	Nicolas Pitre	Initial release.
 * 
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/errno.h>
#include <linux/sound.h>
#include <linux/soundcard.h>

#include <asm/uaccess.h>
#include <asm/hardware.h>
#include <asm/dma.h>

#include "sa1100-audio.h"


#undef DEBUG
#ifdef DEBUG
#define DPRINTK( x... )  printk( ##x )
#else
#define DPRINTK( x... )
#endif


#define AUDIO_NAME		"SA1100 SSP audio"

#define AUDIO_FMT_MASK		AFMT_S16_LE
#define AUDIO_CHANNELS		2

static int sample_rate = 44100;


static void ssp_audio_init(void)
{
	if (machine_is_lart()) {
		/* LART has the SSP port rewired to GPIO 10-13, 19 */
		/* alternate functions for the GPIOs */
		GAFR |= ( GPIO_SSP_TXD | GPIO_SSP_RXD | GPIO_SSP_SCLK |
			  GPIO_SSP_SFRM | GPIO_SSP_CLK );

		/* Set the direction: 10, 12, 13 output; 11, 19 input */
		GPDR |= ( GPIO_SSP_TXD | GPIO_SSP_SCLK | GPIO_SSP_SFRM );
		GPDR &= ~( GPIO_SSP_RXD | GPIO_SSP_CLK );

		/* enable SSP pin swap */
		PPAR |= PPAR_SPR;
	}
	
	/* turn on the SSP */
	Ser4SSCR0 = (SSCR0_DataSize(16) | SSCR0_TI | SSCR0_SerClkDiv(2) |
		     SSCR0_SSE);
	Ser4SSCR1 = (SSCR1_SClkIactL | SSCR1_SClk1P | SSCR1_ExtClk);
}

static int ssp_audio_ioctl( struct inode *inode, struct file *file,
			    uint cmd, ulong arg)
{
	long val;
	int ret = 0;

	/*
	 * These are platform dependent ioctls which are not handled by the 
	 * generic sa1100-audio module.
	 */
	switch (cmd) {
	case SNDCTL_DSP_SETFMT:
		ret = get_user(val, (long *) arg);
		if (ret) break;
		if (val & AUDIO_FMT_MASK) {
			break;
		} else
			return -EINVAL;

	case SNDCTL_DSP_CHANNELS:
	case SNDCTL_DSP_STEREO:
		ret = get_user(val, (long *) arg);
		if (ret) break;
		if (cmd == SNDCTL_DSP_STEREO)
			val = val ? 2 : 1;
		/* Simple standard DACs are stereo only */
		if (val != AUDIO_CHANNELS)
			return -EINVAL;
		break;

	case SOUND_PCM_READ_CHANNELS:
		return put_user(AUDIO_CHANNELS, (long *) arg);

	case SNDCTL_DSP_SPEED:
	case SOUND_PCM_READ_RATE:
		/* We assume the clock doesn't change */
		return put_user(sample_rate, (long *) arg);

	case SNDCTL_DSP_GETFMTS:
		return put_user(AUDIO_FMT_MASK, (long *) arg);

	default:
		return -EINVAL;
	}

	return ret;
}

static audio_stream_t output_stream;

static audio_state_t audio_state = {
	output_stream:	&output_stream,
	hw_init:	ssp_audio_init,
	client_ioctl:	ssp_audio_ioctl,
};

static int ssp_audio_open(struct inode *inode, struct file *file)
{
	return sa1100_audio_instance(inode, file, &audio_state);
}

/* 
 * Missing fields of this structure will be patched with the call 
 * to sa1100_audio_instance() 
 */
static struct file_operations ssp_audio_fops = {
	open:		ssp_audio_open,
	owner:		THIS_MODULE
};

static int audio_dev_id;

static int __init sa1100ssp_audio_init(void)
{
	if (!machine_is_lart()) {
		printk(KERN_ERR AUDIO_NAME ": no support for this SA-1100 design!\n");
		/* look at ssp_audio_init() for specific initialisations */
		return -ENODEV;
	}

	/* Acquire and initialize DMA */
	if (sa1100_request_dma(&output_stream.dma_ch, "Generic SSP sound") < 0) {
		printk( KERN_ERR AUDIO_NAME ": unable to get DMA channel\n" );
		return -EBUSY;
	}
	sa1100_dma_set_device(output_stream.dma_ch, DMA_Ser4SSPWr);

	/* register devices */
	audio_dev_id = register_sound_dsp(&ssp_audio_fops, -1);

	printk( KERN_INFO AUDIO_NAME " initialized\n" );
	return 0;
}

static void __exit sa1100ssp_audio_exit(void)
{
	unregister_sound_dsp(audio_dev_id);
	sa1100_free_dma(output_stream.dma_ch);
}

module_init(sa1100ssp_audio_init);
module_exit(sa1100ssp_audio_exit);

MODULE_AUTHOR("Nicolas Pitre");
MODULE_DESCRIPTION("Glue audio driver for a simple DAC on the SA1100's SSP port");

MODULE_PARM(sample_rate, "i");
MODULE_PARM_DESC(sample_rate, "Sample rate of the audio DAC, default is 44100");

EXPORT_NO_SYMBOLS;
