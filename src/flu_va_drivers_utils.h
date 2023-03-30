/* flu-va-drivers
 * Copyright 2022-2023 Fluendo S.A. <support@fluendo.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef __FLU_VA_DRIVERS_UTILS_H__
#define __FLU_VA_DRIVERS_UTILS_H__

#include <va/va_backend.h>
#include <stdio.h>

#define FLU_VA_DRIVERS_ALIGN(n, alignment)                                    \
  (((n) + ((alignment) -1)) & ~((alignment) -1))

#define FLU_VA_DRIVERS_INVALID_ID -1
typedef int FluVaDriversID;

void flu_va_drivers_get_vendor (char *str_vendor);

#endif /* __FLU_VA_DRIVERS_UTILS_H__ */
