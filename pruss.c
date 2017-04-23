/*
 * avrdude - A Downloader/Uploader for AVR XMEGA device programmers
 * Copyright (C) 2016 Enric Balletbo i Serra <enric.balletbo@collabora.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ac_cfg.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include <unistd.h>
#include <prussdrv.h>
#include <pruss_intc_mapping.h>

#include "avrdude.h"
#include "libavrdude.h"
#include "pruss.h"

#define PATH_MAX	4096
#define PRU_NUM		0

/*
 * TODO : get the START_ADDR at build time
 */
#define START_ADDR 0x00000c20

#ifndef START_ADDR
#error "START_ADDR must be defined"
#endif

volatile unsigned int* shared_ram = NULL;

/*
 * Private data for this programmer.
 */
struct pdata
{
    char data[PATH_MAX]; // The data.bin file location
    char text[PATH_MAX]; // The text.bin file location
};

#define PDATA(pgm) ((struct pdata *)(pgm->cookie))

/*
 * Wait for interrupt from the PRU
 */
static inline void pruss_wait_event_from_pru(void)
{
  /* Wait for interrupt from PRU */
  if (verbose >= 2)
    fprintf(stderr, "%s: Waiting for interrupt from PRU\n", progname);

  prussdrv_pru_wait_event(PRU_EVTOUT_0);
  prussdrv_pru_clear_event(PRU_EVTOUT_0, PRU0_ARM_INTERRUPT);
}

/*
 * Transmit an AVR device command and return the results; 'cmd' and
 * 'res' must point to at least a 4 byte data buffer
 */
static int pruss_cmd(PROGRAMMER * pgm, const unsigned char *cmd,
                      unsigned char *res)
{
  /* dummy function */
  return 0;
}

/*
 * Issue the 'chip erase' command to the AVR device
 */
static int pruss_chip_erase(PROGRAMMER * pgm, AVRPART * p)
{
  if (verbose >= 2)
    fprintf(stderr, "%s: Chip erase\n", progname);

  shared_ram[0] = CMD_CHIP_ERASE;
  pruss_wait_event_from_pru();

  return 0;
}

/*
 * Issue the 'program enable' command to the AVR device
 */
static int pruss_program_enable(PROGRAMMER * pgm, AVRPART * p)
{
  /* dummy function */
  return 0;
}

/*
 * Initialize the AVR device and prepare it to accept commands
 */
static int pruss_initialize(PROGRAMMER * pgm, AVRPART * p)
{
  return pgm->program_enable(pgm, p);
}

static void pruss_disable(PROGRAMMER * pgm)
{
  /* dummy function */
  return;
}

static void pruss_enable(PROGRAMMER * pgm)
{
  /* dummy function */
  return;
}

static void pruss_setup(PROGRAMMER * pgm)
{
   if ((pgm->cookie = malloc(sizeof(struct pdata))) == 0)
   {
     fprintf(stderr,
             "%s: pruss_setup(): Out of memory allocating private data\n",
             progname);
     exit(1);
   }
   memset(pgm->cookie, 0, sizeof(struct pdata));

   strcpy(PDATA(pgm)->data, "data.bin");
   strcpy(PDATA(pgm)->text, "text.bin");
}

static void pruss_teardown(PROGRAMMER * pgm)
{
    free(pgm->cookie);
}

static int pruss_open(PROGRAMMER * pgm, char * port)
{
  int ret;
  void *map;
  tpruss_intc_initdata pruss_intc_initdata = PRUSS_INTC_INITDATA;

  if (verbose >= 2)
    fprintf(stderr, "%s: Open programmer\n", progname);

  ret = prussdrv_init();
  if (ret) {
    fprintf(stderr, "Error: PRU initialization failed!\n");
    return ret;
  }

  ret = prussdrv_open(PRU_EVTOUT_0);
  if (ret) {
    fprintf(stderr, "Error: unable to open PRU device!\n");
    return ret;
  }

  ret = prussdrv_pruintc_init(&pruss_intc_initdata);
  if (ret) {
    fprintf(stderr, "Error: PRU interrupts initialization failed!\n");
    return ret;
  }

  /* Load and run the PRU program */
  /* TODO: ret = prussdrv_load_datafile(PRU_NUM, PDATA(pgm)->data);*/
  ret = prussdrv_load_datafile(PRU_NUM, "/usr/share/avrdude/pruss/data.bin");
  if (ret) {
    fprintf(stderr, "Error: failed to load PRU program (%s)!\n",
            "/usr/share/avrdude/pruss/data.bin");
    return ret;
  }

  /* TODO: ret = prussdrv_exec_program_at(PRU_NUM, PDATA(pgm)->text, START_ADDR); */
  ret = prussdrv_exec_program_at(PRU_NUM, "/usr/share/avrdude/pruss/text.bin", START_ADDR);
  if (ret) {
    fprintf(stderr, "Error: failed to run PRU program (%s)!\n",
            "/usr/share/avrdude/pruss/text.bin");
    return ret;
  }

  /* Get pointer to shared ram */
  prussdrv_map_prumem(PRUSS0_SHARED_DATARAM, &map);
  shared_ram = (unsigned int*)map;

  return 0;
}

static void pruss_close(PROGRAMMER * pgm)
{
  if (verbose >= 2)
    fprintf(stderr, "%s: Close programmer\n", progname);

  /* Disable PRU and exit */
  prussdrv_pru_disable(PRU_NUM);
  prussdrv_exit ();

  serial_close(&pgm->fd);
  pgm->fd.ifd = -1;
}

static void pruss_display(PROGRAMMER * pgm, const char * p)
{
  /* dummy function */
  return;
}

static int pruss_read_byte(PROGRAMMER * pgm, AVRPART * p, AVRMEM * mem,
			       unsigned long addr, unsigned char * value)
{
  if (verbose >= 2)
    fprintf(stderr, "%s: pruss_read_byte(.., %s, 0x%lx, ...)\n",
            progname, mem->desc, addr);

  if (strcmp(mem->desc, "signature") == 0) {
    shared_ram[1] = (unsigned int)addr;
    shared_ram[0] = CMD_READ_SIGNATURE;
  } else {
    fprintf(stderr, "%s: Read byte from memory '%s' is not supported.",
            progname, mem->desc);
    return -1;
  }

  pruss_wait_event_from_pru();

  *value = shared_ram[2];

  return 0;
}

static int pruss_write_byte(PROGRAMMER * pgm, AVRPART * p, AVRMEM * mem,
                                unsigned long addr, unsigned char value)
{
  fprintf(stderr, "%s: Write byte from memory '%s' is not supported.",
          progname, mem->desc);

  return -1;
}

static int pruss_paged_load(PROGRAMMER * pgm, AVRPART * p, AVRMEM * mem,
                               unsigned int page_size,
                               unsigned int addr, unsigned int n_bytes)
{
  int i;

  if (verbose >= 2)
    fprintf(stderr, "%s: pruss_paged_load(.., %s, 0x%x, %d, %d...)\n",
	    progname, mem->desc, addr, n_bytes, page_size);

  memset((void *)shared_ram, 0, 1024);

  /* determine which command is to be used */
  if (strcmp(mem->desc, "flash") == 0) {
    shared_ram[1] = addr;
    shared_ram[2] = n_bytes;
    shared_ram[0] = CMD_READ_FLASH;
  } else {
    fprintf(stderr, "Error: Read from memory '%s' is not supported.", mem->desc);
    return -1;
  }

  pruss_wait_event_from_pru();

  if (verbose >= 2)
    fprintf(stderr, "\n\nReading at '%s' memory at offset %x:\n",
            mem->desc, addr);
  for (i = 0; i < 256; i++) {
        mem->buf[addr + i] = (uint8_t)shared_ram[i + 5];
        if (verbose >= 2)
	  fprintf(stderr, "0x%x ", shared_ram[i + 5]);
  }
  if (verbose >= 2)
    fprintf(stderr, "\n\n");

  return 0;
}

static int pruss_paged_write(PROGRAMMER * pgm, AVRPART * p, AVRMEM * mem,
                               unsigned int page_size,
                               unsigned int addr, unsigned int n_bytes)
{
  int i;

  if (verbose >= 2)
    fprintf(stderr, "%s: pruss_paged_write(.., %s, 0x%x, %d, %d...)\n",
            progname, mem->desc, addr, n_bytes, page_size);

  /* clean shared_ram */
  memset((void *)shared_ram, 0, 1024);

  /* copy data to shared ram */
  if (verbose >= 2)
    fprintf(stderr, "\n\nWritting at memory '%s' offset %x:\n",
            mem->desc, addr);
  for (i = 0; i < n_bytes; i++) {
	shared_ram[i + 5] = *(mem->buf + addr + i);
        if (verbose >= 2)
	  fprintf(stderr, "0x%x ", *(mem->buf + addr + i));
  }
  if (verbose >= 2)
    fprintf(stderr, "\n\n");

  /* determine which command is to be used */
  if (strcmp(mem->desc, "flash") == 0) {
    shared_ram[1] = addr;
    shared_ram[2] = page_size;
    shared_ram[3] = n_bytes;
    shared_ram[0] = CMD_PROGRAM_FLASH;
  } else {
    fprintf(stderr, "Error: Write to memory '%s' is not supported.", mem->desc);
    return -1;
  }

  pruss_wait_event_from_pru();

  return 0;
}

static int pruss_parseextparams(struct programmer_t * pgm, LISTID extparms)
{
    LNODEID ln;
    const char *extended_param;
    int rv = 0;

    for (ln = lfirst(extparms); ln; ln = lnext(ln)) {
        extended_param = ldata(ln);

        if (strncmp(extended_param, "data=", strlen("data=")) == 0) {
            if (sscanf(extended_param, "data=%s", PDATA(pgm)->data) != 1) {
                fprintf(stderr, "%s: pruss_parseextparms(): invalid '%s'\n",
                        progname, extended_param);
                rv = -1;
                continue;
            }

            if (verbose >= 2) {
                fprintf(stderr,
			"%s: pruss_parseextparms(): fwpath set to %s\n",
                        progname, PDATA(pgm)->data);
            }

            continue;
        }

        if (strncmp(extended_param, "text=", strlen("text=")) == 0) {
            if (sscanf(extended_param, "text=%s", PDATA(pgm)->text) != 1) {
                fprintf(stderr, "%s: pruss_parseextparms(): invalid '%s'\n",
                        progname, extended_param);
                rv = -1;
                continue;
            }

            if (verbose >= 2) {
                fprintf(stderr,
			"%s: pruss_parseextparms(): fwpath set to %s\n",
                        progname, PDATA(pgm)->text);
            }

            continue;
        }

        fprintf(stderr,
                "%s: pruss_parseextparms(): invalid extended parameter '%s'\n",
                progname, extended_param);
        rv = -1;
    }

    return rv;
}

const char pruss_desc[] = "TI AM335x PRUSS Programmer";

void pruss_initpgm(PROGRAMMER * pgm)
{
  strcpy(pgm->type, "PRUSS");

  /*
   * mandatory functions
   */
  pgm->initialize     = pruss_initialize;
  pgm->display        = pruss_display;
  pgm->enable         = pruss_enable;
  pgm->disable        = pruss_disable;
  pgm->program_enable = pruss_program_enable;
  pgm->chip_erase     = pruss_chip_erase;
  pgm->cmd            = pruss_cmd;
  pgm->open           = pruss_open;
  pgm->close          = pruss_close;
  pgm->read_byte      = pruss_read_byte;
  pgm->write_byte     = pruss_write_byte;

  /*
   * optional functions
   */
  pgm->paged_write    = pruss_paged_write;
  pgm->paged_load     = pruss_paged_load;

  pgm->parseextparams = pruss_parseextparams;

  pgm->setup          = pruss_setup;
  pgm->teardown       = pruss_teardown;
}
