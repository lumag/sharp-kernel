/*
 * Glue audio driver for the Compaq iPAQ H3600 & Philips UDA1341 codec.
 * 
 * Copyright (c) 2000 Nicolas Pitre <nico@cam.org>
 *
 * This program is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License.
 *
 * This is the machine specific part of the Compaq iPAQ (aka Bitsy) support.
 * This driver makes use of the UDA1341 and the sa1100-audio modules.
 *
 * History:
 *
 * 2000-05-21	Nicolas Pitre	Initial UDA1341 driver release.
 * 
 * 2000-07-??	George France	Bitsy support.
 * 
 * 2000-12-13	Deborah Wallach	Fixed power handling for iPAQ/bitsy
 * 
 * 2001-06-03	Nicolas Pitre	Made this file a separate module, based on
 * 				the former sa1100-uda1341.c driver.
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
#include "uda1341.h"


#undef DEBUG
#ifdef DEBUG
#define DPRINTK( x... )  printk( ##x )
#else
#define DPRINTK( x... )
#endif


#define AUDIO_NAME		"Bitsy_UDA1341"

#define AUDIO_FMT_MASK		(AFMT_S16_LE)
#define AUDIO_FMT_DEFAULT	(AFMT_S16_LE)
#define AUDIO_CHANNELS_DEFAULT	2
#define AUDIO_RATE_DEFAULT	44100


/*
 * GPIO based L3 bus support.
 *
 * This provides control of Philips L3 type devices. 
 * GPIO lines are used for clock, data and mode pins.
 * It appears the read data is generated on the falling edge of the clock
 * and should be held stable during the clock high time.
 */

/* 
 * L3 bus GPIO pin definitions
 */

#define L3_DataPin	GPIO_BITSY_L3_DATA
#define L3_ClockPin	GPIO_BITSY_L3_CLOCK
#define L3_ModePin	GPIO_BITSY_L3_MODE

/* 
 * L3 setup and hold times (expressed in us)
 */
#define L3_DataSetupTime 1	/* 190 ns */
#define L3_DataHoldTime  1	/*  30 ns */
#define L3_ModeSetupTime 1	/* 190 ns */
#define L3_ModeHoldTime  1	/* 190 ns */
#define L3_ClockHighTime 1	/* 250 ns (min is 64*fs, 35us @ 44.1 Khz) */
#define L3_ClockLowTime  1	/* 250 ns (min is 64*fs, 35us @ 44.1 Khz) */
#define L3_HaltTime      1	/* 190 ns */

/*
 * Grab control of the IIC/L3 shared pins
 */
static inline void L3_acquirepins(void)
{
	GPSR = (L3_ModePin | L3_ClockPin | L3_DataPin);
	GPDR |= (L3_ModePin | L3_ClockPin | L3_DataPin);
}

/*
 * Release control of the IIC/L3 shared pins
 */
static inline void L3_releasepins(void)
{
	GPDR &= ~(L3_ModePin | L3_ClockPin | L3_DataPin);
}

/*
 * Initialize the interface
 */
static void L3_init(void)
{
	GAFR &= ~(L3_DataPin | L3_ClockPin | L3_ModePin);
	L3_releasepins();
}

/*
 * Get a bit. The clock is high on entry and on exit. Data is read after
 * the clock low time has expired.
 */
static inline int L3_getbit(void)
{
	int data;

	GPCR = L3_ClockPin;
	udelay(L3_ClockLowTime);

	data = (GPLR & L3_DataPin) ? 1 : 0;

	GPSR = L3_ClockPin;
	udelay(L3_ClockHighTime);

	return data;
}

/*
 * Send a bit. The clock is high on entry and on exit. Data is sent only
 * when the clock is low (I2C compatibility).
 */
static inline void L3_sendbit(int bit)
{
	GPCR = L3_ClockPin;

	if (bit & 1)
		GPSR = L3_DataPin;
	else
		GPCR = L3_DataPin;

	/* Assumes L3_DataSetupTime < L3_ClockLowTime */
	udelay(L3_ClockLowTime);

	GPSR = L3_ClockPin;
	udelay(L3_ClockHighTime);
}

/*
 * Send a byte. The mode line is set or pulsed based on the mode sequence
 * count. The mode line is high on entry and exit. The mod line is pulsed
 * before the second data byte and before ech byte thereafter.
 */
static void L3_sendbyte(char data, int mode)
{
	int i;

	switch (mode) {
	case 0:		/* Address mode */
		GPCR = L3_ModePin;
		break;
	case 1:		/* First data byte */
		break;
	default:		/* Subsequent bytes */
		GPCR = L3_ModePin;
		udelay(L3_HaltTime);
		GPSR = L3_ModePin;
		break;
	}

	udelay(L3_ModeSetupTime);

	for (i = 0; i < 8; i++)
		L3_sendbit(data >> i);

	if (mode == 0)		/* Address mode */
		GPSR = L3_ModePin;

	udelay(L3_ModeHoldTime);
}

/*
 * Get a byte. The mode line is set or pulsed based on the mode sequence
 * count. The mode line is high on entry and exit. The mod line is pulsed
 * before the second data byte and before each byte thereafter. This
 * function is never valid with mode == 0 (address cycle) as the address
 * is always sent on the bus, not read.
 * L3_DataPin must be set to an input before calling this function!
 */
static char L3_getbyte(int mode)
{
	char data = 0;
	int i;

	switch (mode) {
	case 0:		/* Address mode - never valid */
		break;
	case 1:		/* First data byte */
		break;
	default:		/* Subsequent bytes */
		GPCR = L3_ModePin;
		udelay(L3_HaltTime);
		GPSR = L3_ModePin;
		break;
	}

	udelay(L3_ModeSetupTime);

	for (i = 0; i < 8; i++)
		data |= (L3_getbit() << i);

	udelay(L3_ModeHoldTime);

	return data;
}

/*
 * Write data to a device on the L3 bus. The address is passed as well as
 * the data and length. The length written is returned. The register space
 * is encoded in the address (low two bits are set and device address is
 * in the upper 6 bits).
 */
static int L3_write(char addr, char *data, int len)
{
	int bytes = len;
	int mode = 0;

	DPRINTK("%s(0x%x, %d)\n", __FUNCTION__, addr, len);

	L3_acquirepins();
	L3_sendbyte(addr, mode++);
	while(len--)
		L3_sendbyte(*data++, mode++);
	L3_releasepins();

	return bytes;
}

/*
 * Read data from a device on the L3 bus. The address is passed as well as
 * the data and length. The length read is returned. The register space
 * is encoded in the address (low two bits are set and device address is
 * in the upper 6 bits).
 */
static int L3_read(char addr, char *data, int len)
{
	int bytes = len;
	int mode = 0;

	DPRINTK("%s(0x%x, %d)\n", __FUNCTION__, addr, len);

	L3_acquirepins();
	L3_sendbyte(addr, mode++);
	GPDR &= ~(L3_DataPin);
	while(len--)
		*data++ = L3_getbyte(mode++);
	L3_releasepins();

	return bytes;
}


/*
 * Mixer (UDA1341) interface
 */

static UDA1341_regs_t UDA1341_regs = UDA1341_REGS_DFLT;

static UDA1341_state_t uda1341_state = {
	regs:		&UDA1341_regs,
	L3_write:	L3_write,
	L3_read:	L3_read,
};

static int mixer_ioctl (struct inode *inode, struct file *file,
			uint cmd, ulong arg)
{
	return uda1341_mixer_ioctl(&uda1341_state, cmd, arg);
}

static struct file_operations bitsy_mixer_fops = {
	ioctl:		mixer_ioctl,
	owner:		THIS_MODULE
};


/*
 * Audio interface
 */

static long audio_samplerate = AUDIO_RATE_DEFAULT;

static void bitsy_set_samplerate(long val)
{
	if (val >= 48000)
		val = 48000;
	else if (val >= 44100)
		val = 44100;
	else if (val >= 32000)
		val = 32000;
	else if (val >= 22050)
		val = 22050;
	else if (val >= 16000)
		val = 16000;
	else if (val >= 11025)
		val = 11025;
	else
		val = 8000;
	
	/* set the external clock generator */
	switch (val) {
	case 32000:
	case 48000:
		/* 00: 12.288 MHz */
		GPCR = GPIO_BITSY_CLK_SET0 | GPIO_BITSY_CLK_SET1;
		break;
	case 44100:
	case 22050:
		/* 01: 11.2896 MHz */
		GPSR = GPIO_BITSY_CLK_SET0;
		GPCR = GPIO_BITSY_CLK_SET1;
		break;
	case 8000:
	case 16000:
		/* 10: 4.096 MHz */
		GPCR = GPIO_BITSY_CLK_SET0;
		GPSR = GPIO_BITSY_CLK_SET1;
		break;
	case 11025:
		/* 11: 5.6245 MHz */
		GPSR = GPIO_BITSY_CLK_SET0 | GPIO_BITSY_CLK_SET1;
		break;
	}
	switch (val) {
	case 8000:
	case 11025:
	case 22050:
		UDA1341_regs.status_0.system_clk = UDA_STATUS0_SC_512FS;
		break;
	case 16000:
	case 44100:
	case 48000:
		UDA1341_regs.status_0.system_clk = UDA_STATUS0_SC_256FS;
		break;
	case 32000:
		UDA1341_regs.status_0.system_clk = UDA_STATUS0_SC_384FS;
		break;
	}
	L3_write((UDA1341_L3Addr << 2) | UDA1341_STATUS,
		 (char *)&UDA1341_regs.status_0, 1);
	audio_samplerate = val;
}

static void bitsy_audio_init(void)
{
	/* Setup the uarts */
	GAFR &= ~(GPIO_SSP_TXD | GPIO_SSP_RXD | GPIO_SSP_SCLK | GPIO_SSP_SFRM);
	GAFR |= (GPIO_SSP_CLK);
	GPDR &= ~(GPIO_SSP_CLK);
	Ser4SSCR0 = 0;
	Ser4SSCR0 = SSCR0_DataSize(16) + SSCR0_TI;
	Ser4SSCR0 |= SSCR0_SerClkDiv(8);
	Ser4SSCR1 = SSCR1_SClkIactL + SSCR1_SClk1P + SSCR1_ExtClk;
	Ser4SSCR0 |= SSCR0_SSE;
	
	/* Setup L3 bus */
	L3_init();

	/* Enable the audio power */
	set_bitsy_egpio(EGPIO_BITSY_CODEC_NRESET | EGPIO_BITSY_AUD_AMP_ON |
			EGPIO_BITSY_AUD_PWR_ON);
	clr_bitsy_egpio(EGPIO_BITSY_QMUTE);

	/* external clock configuration */
	GPDR |= (GPIO_BITSY_CLK_SET0 | GPIO_BITSY_CLK_SET1);
	bitsy_set_samplerate(audio_samplerate);
	
	/* Wait for the UDA1341 to wake up */
	mdelay(1);
	
	/* Initialize the UDA1341 internal state */
	uda1341_state.active = 1;
	uda1341_reset(&uda1341_state);
}

static void bitsy_audio_shutdown(void)
{
	uda1341_state.active = 0;
	/* disable the audio power and all signals leading to the audio chip */
	clr_bitsy_egpio(EGPIO_BITSY_CODEC_NRESET | EGPIO_BITSY_AUD_AMP_ON |
			EGPIO_BITSY_AUD_PWR_ON | EGPIO_BITSY_QMUTE);
}

static int bitsy_audio_ioctl(struct inode *inode, struct file *file,
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
		/* the UDA1341 is stereo only */
		if (val != 2)
			return -EINVAL;
		break;

	case SOUND_PCM_READ_CHANNELS:
		return put_user(2, (long *) arg);

	case SNDCTL_DSP_SPEED:
		ret = get_user(val, (long *) arg);
		if (ret) break;
		bitsy_set_samplerate(val);
		/* fall through */
		
	case SOUND_PCM_READ_RATE:
		return put_user(audio_samplerate, (long *) arg);

	case SNDCTL_DSP_GETFMTS:
		return put_user(AUDIO_FMT_MASK, (long *) arg);

	default:
		/* Maybe this is meant for the mixer (As per OSS Docs) */
		return uda1341_mixer_ioctl(&uda1341_state, cmd, arg);
	}

	return ret;
}

static audio_stream_t output_stream, input_stream;

static audio_state_t audio_state = {
	output_stream:	&output_stream,
	input_stream:	&input_stream,
	need_tx_for_rx:	1,
	hw_init:	bitsy_audio_init,
	hw_shutdown:	bitsy_audio_shutdown,
	client_ioctl:	bitsy_audio_ioctl,
};

static int bitsy_audio_open(struct inode *inode, struct file *file)
{
	return sa1100_audio_instance(inode, file, &audio_state);
}

/* 
 * Missing fields of this structure will be patched with the call 
 * to sa1100_audio_instance() 
 */
static struct file_operations bitsy_audio_fops = {
	open:		bitsy_audio_open,
	owner:		THIS_MODULE
};


static int audio_dev_id, mixer_dev_id;

static int __init bitsy_uda1341_init(void)
{
	if (!machine_is_bitsy())
		return -ENODEV;

	/* Acquire and initialize DMA */
	if (sa1100_request_dma(&output_stream.dma_ch, "UDA1341 out") < 0 ||
	    sa1100_request_dma(&input_stream.dma_ch, "UDA1341 in") < 0) {
		sa1100_free_dma(output_stream.dma_ch);
		printk( KERN_ERR AUDIO_NAME ": unable to get DMA channels\n" );
		return -EBUSY;
	}
	sa1100_dma_set_device(output_stream.dma_ch, DMA_Ser4SSPWr);
	sa1100_dma_set_device(input_stream.dma_ch, DMA_Ser4SSPRd);

	/* register devices */
	audio_dev_id = register_sound_dsp(&bitsy_audio_fops, -1);
	mixer_dev_id = register_sound_mixer(&bitsy_mixer_fops, -1);

	printk( KERN_INFO AUDIO_NAME " initialized\n" );
	return 0;
}

static void __exit bitsy_uda1341_exit(void)
{
	unregister_sound_dsp(audio_dev_id);
	unregister_sound_mixer(mixer_dev_id);
	sa1100_free_dma(output_stream.dma_ch);
	sa1100_free_dma(input_stream.dma_ch);
}

module_init(bitsy_uda1341_init);
module_exit(bitsy_uda1341_exit);

MODULE_AUTHOR("Nicolas Pitre, George France");
MODULE_DESCRIPTION("Glue audio driver for the Compaq iPAQ H3600 & Philips UDA1341 codec.");

EXPORT_NO_SYMBOLS;
