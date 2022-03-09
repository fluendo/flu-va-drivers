#include "flu_va_drivers_utils.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

void
flu_va_drivers_get_vendor (char *str_vendor)
{
  sprintf (str_vendor, "%s (%s) - %s", FLU_VA_DRIVERS_COMMERCIAL_NAME,
      FLU_VA_DRIVERS_VENDOR, FLU_VA_DRIVERS_PROJECT_VERSION);
}
