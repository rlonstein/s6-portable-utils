/* ISC license. */

#include "sgetopt.h"
#include "fmtscan.h"
#include "strerr2.h"
#include "djbunix.h"

#define USAGE "s6-mkfifo [ -m mode ] fifo..."

int main (int argc, char const *const *argv)
{
  unsigned int mode = 0666 ;
  PROG = "s6-mkfifo" ;
  {
    subgetopt_t l = SUBGETOPT_ZERO ;
    for (;;)
    {
      register int opt = subgetopt_r(argc, argv, "m:", &l) ;
      if (opt == -1) break ;
      switch (opt)
      {
        case 'm': if (uint_oscan(l.arg, &mode)) break ;
        default : strerr_dieusage(100, USAGE) ;
      }
    }
    argc -= l.ind ; argv += l.ind ;
  }
  if (!argc) strerr_dieusage(100, USAGE) ;
  for (; *argv ; argv++)
    if (fifo_make(*argv, mode) == -1)
      strerr_diefu2sys(111, "fifo_make ", *argv) ;
  return 0 ;
}
