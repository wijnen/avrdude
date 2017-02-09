/*
 * avrdude - A Downloader/Uploader for AVR XMEGA device programmers
 *
 * Copyright (C) 2016  Enric Balletbo i Serra <enric.balletbo@collabora.com>>
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

#ifndef pruss_h
#define pruss_h

#define CMD_ENTER_PROGMODE	0x10
#define CMD_LEAVE_PROGMODE	0x11
#define CMD_READ_SIGNATURE	0x12
#define CMD_CHIP_ERASE		0x13
#define CMD_PROGRAM_FLASH	0x14
#define CMD_READ_FLASH		0x15

#ifdef __cplusplus
extern "C" {
#endif

extern const char pruss_desc[];
void pruss_initpgm (PROGRAMMER * pgm);

#ifdef __cplusplus
}
#endif

#endif


