/* ISC license. */

#include "sgetopt.h"
#include "bytestr.h"
#include "fmtscan.h"
#include "strerr2.h"
#include "allreadwrite.h"
#include "skamisc.h"

#define USAGE "s6-unquote [ -n ] [ -d delim ] string"

int main (int argc, char const *const *argv)
{
  char const *delim = "\"" ;
  unsigned int len, delimlen ;
  int nl = 1 ;
  char const *string ;
  PROG = "s6-unquote" ;
  {
    subgetopt_t l = SUBGETOPT_ZERO ;
    for (;;)
    {
      register int opt = subgetopt_r(argc, argv, "nd:", &l) ;
      if (opt == -1) break ;
      switch (opt)
      {
        case 'n' : nl = 0 ; break ;
        case 'd': delim = l.arg ; break ;
        default : strerr_dieusage(100, USAGE) ;
      }
    }
    argc -= l.ind ; argv += l.ind ;
  }
  if (!argc) strerr_dieusage(100, USAGE) ;
  string = *argv ;
  len = str_len(string) ;
  delimlen = str_len(delim) ;
  if (delimlen)
  {
    if (!len--) strerr_dief1x(100, "the empty string isn't a quoted string") ;
    if (byte_chr(delim, delimlen, *string++) >= delimlen)
      strerr_dief1x(100, "invalid starting quote character") ;
  }
  {
    unsigned int r = 0, w = 0 ;
    char buf[len+1] ;
    if (!string_unquote_withdelim(buf, &w, string, len, &r, delim, str_len(delim)))
    {
      char fmt[UINT_FMT] ;
      fmt[uint_fmt(fmt, r + !!delimlen)] = 0 ;
      strerr_diefu2sys(100, "unquote at character ", fmt) ;
    }
    if (delimlen)
    {
      if (r == len) strerr_dief1x(100, "no ending quote character") ;
      else if (r < len - 1)
      {
        char fmtnum[UINT_FMT] ;
        char fmtden[UINT_FMT] ;
        fmtnum[uint_fmt(fmtnum, r+1)] = 0 ;
        fmtden[uint_fmt(fmtden, len)] = 0 ;
        strerr_warnw5x("found ending quote character at position ", fmtnum, "/", fmtden, "; ignoring remainder") ;
      }
    }
    if (nl) buf[w++] = '\n' ;
    if (allwrite(1, buf, w) < w)
      strerr_diefu1sys(111, "write to stdout") ;
  }
  return 0 ;
}
