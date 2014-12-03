/*
 * Sharp PDA Driver Header File
 * Copyright (c) 2001 SHARP
 *
 * based on
 * Collie APM Driver Header File
 * Copyright (c) 2001 Lineo Inc.
 *
 * Change Log
 *	26-Jun-2002 SHARP	Add `APM_IOC_GET_BACKPACK_STATE`
 */

#ifndef _SHARP_APM_H
#define _SHARP_APM_H

/*
 *   /proc/apm battery status
 */
#define APM_BATTERY_STATUS_VERY_LOW 0x7f



#define APM_IOC_MAGIC 'A'

#define APM_IOC_GET_AC_STATUS		_IO(APM_IOC_MAGIC, 10)
#define APM_IOC_AUTO_POWER_CANCEL	_IO(APM_IOC_MAGIC, 11)
#define APM_IOC_GET_AUTO_POWER_CANCEL	_IO(APM_IOC_MAGIC, 12)
#define APM_IOC_AUTO_LIGHT_CANCEL	_IO(APM_IOC_MAGIC, 13)
#define APM_IOC_GET_AUTO_LIGHT_CANCEL	_IO(APM_IOC_MAGIC, 14)
#define APM_IOC_GET_AUTO_POWER_TIME	_IO(APM_IOC_MAGIC, 15)
#define APM_IOC_SET_AUTO_POWER_TIME	_IO(APM_IOC_MAGIC, 16)
#define APM_IOC_GET_AUTO_LIGHT_TIME	_IO(APM_IOC_MAGIC, 17)
#define APM_IOC_SET_AUTO_LIGHT_TIME	_IO(APM_IOC_MAGIC, 18)


#define APM_IOC_SET_DAEMON_MODE		_IO(APM_IOC_MAGIC, 20)
#define APM_IOC_RESET_DAEMON_MODE	_IO(APM_IOC_MAGIC, 21)

#define APM_IOC_BATTERY_MAIN_MODE	_IO(APM_IOC_MAGIC, 30)
#define APM_IOC_BATTERY_BACK_MODE	_IO(APM_IOC_MAGIC, 31)
#define APM_IOC_BATTERY_BACK_CHK	_IO(APM_IOC_MAGIC, 32)
#define APM_IOC_BATTERY_MAIN_CHK	_IO(APM_IOC_MAGIC, 33)
#define APM_IOC_GET_BACKPACK_STATE	_IO(APM_IOC_MAGIC, 34)

#define APM_IOC_GET_ON_MODE		_IO(APM_IOC_MAGIC, 40)
#define APM_IOC_GET_ON_MODE_CLEAR	_IO(APM_IOC_MAGIC, 41)
#define APM_IOC_GET_REGISTER		_IO(APM_IOC_MAGIC, 45)

#define APM_IOC_RESET			_IO(APM_IOC_MAGIC, 50)


#define APM_IOCGWUPSRC  _IOR (APM_IOC_MAGIC, 200, int) /* 0x800441c8 */
#define APM_IOCSWUPSRC  _IOWR(APM_IOC_MAGIC, 201, int) /* 0xc00441c9 */
#define APM_IOCGWUPFACT _IOR (APM_IOC_MAGIC, 202, int) /* 0x800441ca */
#define APM_IOCGEVTSRC  _IOR (APM_IOC_MAGIC, 203, int) /* 0x800441cb */
#define APM_IOCSEVTSRC  _IOWR(APM_IOC_MAGIC, 204, int) /* 0xc00441cc */

/* COLLIE_APM_IOCGEVSRC, COLLIE_APM_IOCEEVSRC */
#define APM_EVT_POWER_BUTTON	(1 << 0)
#define APM_EVT_BATTERY_FAULT	(1 << 1)
#define APM_EVT_BATTERY_STATUS	(1 << 2)
#define APM_EVT_LIGHT_CHANGE	(1 << 3)

#define WAKEUP_RTC		0x80000000
#define WAKEUP_HOME_KEY		0x40000000
#define WAKEUP_OK_KEY		0x20000000
#define WAKEUP_MENU_KEY		0x10000000
#define WAKEUP_SYNC_KEY		0x01000000
#define WAKEUP_POWER_KEY	0x02000000
#define WAKEUP_PHS			0x04000000

#ifdef CONFIG_COLLIE
#include <asm/arch/collie_apm.h>
#endif

#ifdef CONFIG_IRIS
#include <asm/arch/iris_apm.h>
#endif

#endif /* _SHARP_APM_H */
