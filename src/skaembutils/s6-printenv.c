/* ISC license. */

#include "bytestr.h"
#include "sgetopt.h"
#include "fmtscan.h"
#include "buffer.h"
#include "strerr2.h"
#include "djbunix.h"
#include "netstring.h"

#define USAGE "s6-printenv [ -n ] [ -0 | -d delimchar ]"

int main (int argc, char const *const *argv, char const *const *envp)
{
  char delim = '\n' ;
  int zero = 0, nl = 1 ;
  PROG = "s6-printenv" ;
  {
    subgetopt_t l = SUBGETOPT_ZERO ;
    for (;;)
    {
      register int opt = subgetopt_r(argc, argv, "nd:0", &l) ;
      if (opt == -1) break ;
      switch (opt)
      {
        case 'n' : nl = 0 ; break ;
        case 'd' : delim = *l.arg ; break ;
        case '0' : zero = 1 ; break ;
        default : strerr_dieusage(100, USAGE) ;
      }
    }
    argc -= l.ind ; argv += l.ind ;
  }
  if (zero) delim = 0 ;
  for (; *envp ; envp++)
  {
    if (delim || zero)
    {
      if ((buffer_putsalign(buffer_1, *envp) < 0)
       || ((nl || envp[1]) && (buffer_PUTC(buffer_1, delim) < 0)))
        strerr_diefu1sys(111, "write to stdout") ;
    }
    else
    {
      unsigned int written = 0 ;
      if (netstring_put(buffer_1, *envp, str_len(*envp), &written) == -1)
        strerr_diefu1sys(111, "write a netstring to stdout") ;
    }
  }
  if (buffer_flush(buffer_1) == -1)
    strerr_diefu1sys(111, "write to stdout") ;
  return 0 ;
}
