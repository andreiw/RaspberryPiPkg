/** @file
 *
 *  Copyright (c), 2018, Andrei Warkentin <andrey.warkentin@gmail.com>
 *
 *  This program and the accompanying materials
 *  are licensed and made available under the terms and conditions of the BSD License
 *  which accompanies this distribution.  The full text of the license may be found at
 *  http://opensource.org/licenses/bsd-license.php
 *
 *  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 *
 **/

#ifndef UTILS_H
#define UTILS_H

#define _IX_BITS(sm, bg) (bg - sm + 1)
#define _IX_MASK(sm, bg) ((1 << _IX_BITS(sm, bg)) - 1)
#define _X(val, sm, bg) (val >> sm) & _IX_MASK(sm, bg)
#define X(val, ix1, ix2) ((ix1 < ix2) ? _X(val, ix1, ix2) :     \
                          _X(val, ix2, ix1))

#define _I(val, sm, bg)  ((val & _IX_MASK(sm, bg)) << sm)
#define I(val, ix1, ix2) ((ix1 < ix2) ? _I(val, ix1, ix2) :     \
                          _I(val, ix2, ix1))

#endif /* UTILS_H */
