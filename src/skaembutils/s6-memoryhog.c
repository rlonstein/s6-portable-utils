/* ISC license. */

#include <errno.h>
#include "sgetopt.h"
#include "fmtscan.h"
#include "buffer.h"
#include "strerr2.h"
#include "stralloc.h"

#define USAGE "s6-memoryhog [ -s ] [ -h ] bytes"
#define dieusage() strerr_dieusage(100, USAGE)

int main (int argc, char const *const *argv)
{
  unsigned int n ;
  int dostack = 0, doheap = 0 ;
  PROG = "s6-memoryhog" ;
  {
    subgetopt_t l = SUBGETOPT_ZERO ;
    for (;;)
    {
      register int opt = subgetopt_r(argc, argv, "sh", &l) ;
      if (opt == -1) break ;
      switch (opt)
      {
        case 's' : dostack = 1 ; break ;
        case 'h' : doheap = 1 ; break ;
        default : dieusage() ;
      }
    }
    argc -= l.ind ; argv += l.ind ;
  }
  if (!argc) dieusage() ;
  if (!uint0_scan(argv[0], &n)) dieusage() ;

  if (dostack)
  {
    buffer_puts(buffer_1, "Allocating ") ;
    buffer_puts(buffer_1, argv[0]) ;
    buffer_putsflush(buffer_1, " bytes in the stack\n") ;
    {
      char buf[n] ;
      byte_zero(buf, n) ;
    }
  }
  
  if (doheap)
  {
    stralloc sa = STRALLOC_ZERO ;
    buffer_puts(buffer_1, "Allocating ") ;
    buffer_puts(buffer_1, argv[0]) ;
    buffer_putsflush(buffer_1, " bytes in the heap\n") ;
    if (!stralloc_ready_tuned(&sa, n, 0, 0, 1))
      strerr_diefu1sys(111, "stralloc_ready") ;
    byte_zero(sa.s, n) ;
    /* stralloc_free(&sa) ; */
  }

  return 0 ;
}
