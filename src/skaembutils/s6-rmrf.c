/* ISC license. */

#include "strerr2.h"
#include "djbunix.h"

#define USAGE "s6-rmrf file ..."

int main (int argc, char const *const *argv)
{
  char const *const *p = argv + 1 ;
  PROG = "s6-rmrf" ;
  if (argc < 2) strerr_dieusage(100, USAGE) ;
  for (; *p ; p++)
    if (rm_rf(*p) == -1)
      strerr_diefu2sys(111, "remove ", argv[1]) ;
  return 0 ;
}
