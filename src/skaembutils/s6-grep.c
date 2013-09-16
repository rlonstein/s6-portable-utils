/* ISC license. */

#include <errno.h>
#include "bytestr.h"
#include "sgetopt.h"
#include "buffer.h"
#include "fmtscan.h"
#include "strerr2.h"
#include "stralloc.h"
#include "skamisc.h"
#include "sredfa.h"

#define USAGE "s6-grep [ -F ] [ -c ] [ -n ] [ -q ] [ -v ] pattern"

typedef struct flags_s flags_t, *flags_t_ref ;
struct flags_s
{
  unsigned int fixed : 1 ;
  unsigned int count : 1 ;
  unsigned int num : 1 ;
  unsigned int quiet : 1 ;
  unsigned int not : 1 ;
} ;
#define FLAGS_ZERO { 0, 0, 0, 0, 0 }

int main (int argc, char const *const *argv)
{
  unsigned int count = 0 ;
  flags_t flags = FLAGS_ZERO ;
  PROG = "s6-grep" ;
  {
    subgetopt_t l = SUBGETOPT_ZERO ;
    for (;;)
    {
      register int opt = subgetopt_r(argc, argv, "Fcnqv", &l) ;
      if (opt == -1) break ;
      switch (opt)
      {
        case 'F': flags.fixed = 1 ; break ;
        case 'c': flags.count = 1 ; break ;
        case 'n': flags.num = 1 ; break ;
        case 'q': flags.quiet = 1 ; break ;
        case 'v': flags.not = 1 ; break ;
        default : strerr_dieusage(100, USAGE) ;
      }
    }
    argc -= l.ind ; argv += l.ind ;
  }
  if (!argc) strerr_dieusage(100, USAGE) ;
  {
    stralloc line = STRALLOC_ZERO ;
    struct sredfa *re = 0 ;
    unsigned int num = 0 ;
    unsigned int arglen = 0 ;
    if (flags.fixed) arglen = str_len(argv[0]) ;
    else
    {
      re = sredfa_new() ;
      if (!re) strerr_diefu1sys(111, "sredfa_new") ;
      if (!sredfa_from_regexp(re, argv[0]))
        strerr_diefu1sys(111, "compile regular expression") ;
    }

    for (;;)
    {
      register int r ;
      line.len = 0 ;
      r = skagetln(buffer_0f1, &line, '\n') ;
      if (!r) break ;
      if (r < 0)
      {
        if ((errno != EPIPE) || !stralloc_catb(&line, "\n", 1))
          strerr_diefu1sys(111, "read from stdin") ;
      }
      num++ ;
      r = flags.fixed ?
        (str_strn(line.s, line.len-1, argv[0], arglen) < line.len - 1) :
        sredfa_match(re, line.s, line.len-1) ;
      if (r < 0)
      {
        int e = errno ;
        buffer_flush(buffer_1) ;
        errno = e ;
        strerr_diefu1sys(111, "match line against pattern") ;
      }
      if (r ^ flags.not)
      {
        count++ ;
        if (!flags.quiet && !flags.count)
        {
          if (flags.num)
          {
            char fmt[UINT_FMT] ;
            register unsigned int n = uint_fmt(fmt, num) ;
            fmt[n++] = ':' ;
            if (buffer_putalign(buffer_1, fmt, n) < (int)n)
              strerr_diefu1sys(111, "write to stdout") ;
          }
          if (buffer_putalign(buffer_1, line.s, line.len) < (int)line.len)
            strerr_diefu1sys(111, "write to stdout") ;
        }
      }
    }
    if (flags.quiet) return !count ;
    stralloc_free(&line) ;
    if (!flags.fixed) sredfa_delete(re) ;
  }
  if (flags.count)
  {
    char fmt[UINT_FMT] ;
    register unsigned int n = uint_fmt(fmt, count) ;
    fmt[n++] = '\n' ;
    if (buffer_putalign(buffer_1, fmt, n) < (int)n)
      strerr_diefu1sys(111, "write to stdout") ;
  }
  return !count ;
}
