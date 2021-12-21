#ifndef __FLU_VA_DRIVERS_UTILS_H__
#define __FLU_VA_DRIVERS_UTILS_H__

#include <va/va_backend.h>
#include <stdio.h>

#define FLU_N_ELEMENTS(x) (sizeof (x) / sizeof (x[0]))

void flu_va_drivers_get_vendor (char *str_vendor);

#endif /* __FLU_VA_DRIVERS_UTILS_H__ */
