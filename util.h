/*
 * BLINKERCOUGH: NSA Playset implant for IR bridging of airgap
 *
 * Copyright (C) 2015  Hacker, J.R. <r00tkillah@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef _UTIL_H
#define _UTIL_H

#include <avr/pgmspace.h>
#include <stdio.h>

void init_debug();

#define debug(fmt, ...) { printf_P(PSTR("DBG: ")); printf_P(PSTR(fmt), ##__VA_ARGS__); putchar('\n'); }

#define debugstart(fmt, ...) { printf_P(PSTR("DBG: ")); printf_P(PSTR(fmt), ##__VA_ARGS__); }

#define debugcont(fmt, ...) { printf_P(PSTR(fmt), ##__VA_ARGS__); }

#define debugend() putchar('\n');

#endif /* _UTIL_H */

/*
 * Editor modelines  -  https://www.wireshark.org/tools/modelines.html
 *
 * Local variables:
 * mode: c++
 * c-basic-offset: 4
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 *
 * vi: set shiftwidth=4 tabstop=8 expandtab:
 * :indentSize=4:tabSize=8:noTabs=true:
 */
