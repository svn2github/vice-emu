/*
 * cw_zorro.c - Zorro catweasel driver.
 *
 * Written by
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
 *
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */

#include "vice.h"

#if defined(HAVE_CATWEASELMKIII) && defined(AMIGA_M68K)

#include <stdlib.h>
#include <string.h>

#include "cw.h"
#include "loadlibs.h"
#include "log.h"
#include "types.h"

#define MAXSID 1

static unsigned char read_sid(unsigned char reg); // Read a SID register
static void write_sid(unsigned char reg, unsigned char data); // Write a SID register

static int sidfh = 0;

typedef void (*voidfunc_t)(void);

static BYTE *CWbase = NULL;

/* read value from SIDs */
int cw_zorro_read(WORD addr, int chipno)
{
    /* check if chipno and addr is valid */
    if (chipno < MAXSID && addr < 0x20) {
        /* if addr is from read-only register, perform a real read */
        if (addr >= 0x19 && addr <= 0x1C && sidfh >= 0) {
            return read_sid(addr);
        }
    }

    return 0;
}

/* write value into SID */
void cw_zorro_store(WORD addr, BYTE val, int chipno)
{
    /* check if chipno and addr is valid */
    if (chipno < MAXSID && addr <= 0x18) {
	  /* if the device is opened, write to device */
        if (sidfh >= 0) {
            write_sid(addr, val);
        }
    }
}

#define CW_SID_DAT 0xd8
#define CW_SID_CMD 0xdc

#undef BYTE
#undef WORD
#include <exec/types.h>
#include <exec/memory.h>
#include <libraries/dos.h>
#include <libraries/configvars.h>

#include <clib/exec_protos.h>
#include <clib/expansion_protos.h>

struct Library *ExpansionBase = NULL;

int cw_zorro_open(void)
{
    struct ConfigDev *myCD;

    if ((ExpansionBase = OpenLibrary("expansion.library", 0L)) == NULL) {
        return -1;
    }

    if (atexitinitialized) {
        cw_zorro_close();
    }

    myCD = FindConfigDev(myCD, 0x1212, 0x42);

    if (!myCD) {
        return -1;
    }

    CWbase = myCD->cd_BoardAddr;

    if (!CWbase) {
        return -1;
    }

    /* mute all sids */
    for (i = 0; i < 32; i++) {
        write_sid(i, 0);
    }

    log_message(LOG_DEFAULT, "CatWeasel MK3 PCI SID: opened");

    /* install exit handler, so device is closed on exit */
    if (!atexitinitialized) {
        atexitinitialized = 1;
        atexit((voidfunc_t)cw_zorro_close);
    }

    sidfh = 1; /* ok */

    return 1;
}

static unsigned char read_sid(unsigned char reg)
{
    unsigned char cmd;
    BYTE tmp;

    cmd = (reg & 0x1f) | 0x20;   // Read command & address

    if (catweaselmkiii_get_ntsc()) {
        cmd |= 0x40;  // Make sure its correct frequency
    }

    // Write command to the SID
    CWbase[CW_SID_CMD] = cmd;

    // Waste 1ms
    usleep(1);

    return CWbase[CW_SID_DAT];
}

static void write_sid(unsigned char reg, unsigned char data)
{
    unsigned char cmd;
    BYTE tmp;

    cmd = reg & 0x1f;            // Write command & address
    if (catweaselmkiii_get_ntsc()) {
        cmd |= 0x40;  // Make sure its correct frequency
    }

    // Write data to the SID
    CWbase[CW_SID_DAT] = data;
    CWbase[CW_SID_CMD] = cmd;

    // Waste 1ms
    usleep(1);
}

int cw_zorro_close(void)
{
    unsigned int i;

    /* mute all sids */
    for (i = 0; i < 32; i++) {
        write_sid(i, 0);
    }

    return 0;
}
#endif