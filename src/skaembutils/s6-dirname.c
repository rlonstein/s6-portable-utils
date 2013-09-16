/* ISC license. */

#include "sgetopt.h"
#include "allreadwrite.h"
#include "bytestr.h"
#include "strerr2.h"
#include "stralloc.h"
#include "djbunix.h"

#define USAGE "s6-dirname [ -n ] file"

int main (int argc, char const *const *argv)
{
  stralloc sa = STRALLOC_ZERO ;
  int nl = 1 ;
  PROG = "s6-dirname" ;
  {
    subgetopt_t l = SUBGETOPT_ZERO ;
    for (;;)
    {
      register int opt = subgetopt_r(argc, argv, "n", &l) ;
      if (opt == -1) break ;
      switch (opt)
      {
        case 'n' : nl = 0 ; break ;
        default : strerr_dieusage(100, USAGE) ;
      }
    }
    argc -= l.ind ; argv += l.ind ;
  }

  if (!argc) strerr_dieusage(100, USAGE) ;
  if (!sadirname(&sa, argv[0], str_len(argv[0])))
    strerr_diefu2sys(111, "get dirname of ", argv[0]) ;
  if (nl && !stralloc_catb(&sa, "\n", 1))
    strerr_diefu2sys(111, "get dirname of ", argv[0]) ;
  if (allwrite(1, sa.s, sa.len) < sa.len)
    strerr_diefu1sys(111, "write to stdout") ;
  return 0 ;
}
