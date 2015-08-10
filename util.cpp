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

#include "util.h"

#include <Arduino.h>

static int uart_putchar(char c, FILE *unused)
{
    if (c == '\n') {
        Serial.println();
    } else {
        Serial.write(c);
    }
    return 1;
}

static int uart_getchar(FILE *unused)
{
    return Serial.read();
}

void init_debug() {
    fdevopen(&uart_putchar, &uart_getchar);
}


// void debug(char *fmt, ...)
// {
//     printf_P(PSTR("DBG: "));

//     putchar('\n');
// }


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
