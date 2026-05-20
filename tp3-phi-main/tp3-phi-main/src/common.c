#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "common.h"

// Signale une erreure fatale. Arguments comme `printf`
void error(const char *fmt, ...)
{
  va_list valist;
  va_start (valist, fmt);
  vfprintf (stderr, fmt, valist);
  exit (EXIT_FAILURE);
  va_end (valist);
}
