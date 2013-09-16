/* ISC license. */

#include "bytestr.h"
#include "buffer.h"
#include "sgetopt.h"
#include "strerr2.h"
#include "stralloc.h"
#include "djbunix.h"

#define USAGE "s6-linkname [ -n ] [ -f ] link"
#define dieusage() strerr_dieusage(100, USAGE)

int main (int argc, char const *const *argv)
{
  stralloc sa = STRALLOC_ZERO ;
  int path = 0, nl = 1 ;
  PROG = "s6-linkname" ;
  {
    subgetopt_t l = SUBGETOPT_ZERO ;
    for (;;)
    {
      register int opt = subgetopt_r(argc, argv, "nf", &l) ;
      if (opt == -1) break ;
      switch(opt)
      {
        case 'n' : nl = 0 ; break ;
        case 'f' : path = 1 ; break ;
        default :  dieusage() ;
      }
    }
    argv += l.ind ; argc -= l.ind ;
  }
  if (!argc) dieusage() ;

  if ((path ? sarealpath(&sa, *argv) : sareadlink(&sa, *argv)) == -1)
    strerr_diefu2sys(111, "resolve ", *argv) ;

  if ((buffer_putalign(buffer_1small, sa.s, sa.len) == -1)
   || (nl && (buffer_putalign(buffer_1small, "\n", 1)) == -1)
   || (buffer_flush(buffer_1small) == -1))
    strerr_diefu1sys(111, "write to stdout") ;

 /* stralloc_free(&sa) ; */
  return 0 ;
}
