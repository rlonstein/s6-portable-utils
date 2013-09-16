/* ISC license. */

#include "sgetopt.h"
#include "buffer.h"
#include "strerr2.h"

#define USAGE "s6-echo [ -n ] [ -s sep ] args..."

int main (int argc, char const *const *argv)
{
  char sep = ' ' ;
  char donl = 1 ;
  PROG = "s6-echo" ;
  {
    subgetopt_t l = SUBGETOPT_ZERO ;
    for (;;)
    {
      register int opt = subgetopt_r(argc, argv, "ns:", &l) ;
      if (opt == -1) break ;
      switch (opt)
      {
        case 'n': donl = 0 ; break ;
        case 's': sep = *l.arg ; break ;
        default : strerr_dieusage(100, USAGE) ;
      }
    }
    argc -= l.ind ; argv += l.ind ;
  }
  for ( ; *argv ; argv++)
    if ((buffer_putsalign(buffer_1small, *argv) == -1)
     || (argv[1] && (buffer_PUTC(buffer_1small, sep) == -1)))
      goto err ;
  if (donl && (buffer_PUTC(buffer_1small, '\n') == -1)) goto err ;
  if (buffer_flush(buffer_1small) == -1) goto err ;
  return 0 ;
err:
  strerr_diefu1sys(111, "write to stdout") ;
}
