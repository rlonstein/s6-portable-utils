/* ISC license. */

#include <errno.h>
#include "sgetopt.h"
#include "bytestr.h"
#include "fmtscan.h"
#include "strerr2.h"
#include "buffer.h"
#include "stralloc.h"
#include "skamisc.h"

#define USAGE "s6-unquote-filter [ -q | -Q | -v | -w ] [ -d delim ]"

static unsigned int strictness = 1 ;
static char const *delim = "\"" ;
static unsigned int delimlen = 1 ;

static int doit (char const *s, unsigned int len)
{
  if (delimlen)
  {
    if (!len--)
    {
      switch (strictness)
      {
        case 1 :
        case 2 :
          strerr_warnw1x("empty line") ;
          break ;
        case 3 :
          buffer_flush(buffer_1) ;
          strerr_dief1x(100, "empty line") ;
        default : break ;
      }
      return 1 ;
    }
    if (byte_chr(delim, delimlen, *s) >= delimlen)
    {
      switch (strictness)
      {
        case 0 : return 0 ;
        case 1 :
        {
          strerr_warnw1x("invalid starting quote character") ;
          return 0 ;
        }
        case 2 :
        {
          char fmt[40] ;
          unsigned int n = len < 39 ? len+1 : 36 ;
          byte_copy(fmt, n, s) ;
          if (len >= 39)
          {
            byte_copy(fmt+n, 3, "...") ;
            n += 3 ;
          }
          fmt[n] = 0 ;
          strerr_warnw3x("invalid starting quote character", " in line: ", fmt) ;
          return 0 ;
        }
        case 3 :
        {
          buffer_flush(buffer_1) ;
          strerr_dief1x(100, "invalid starting quote character") ;
        }
        default : strerr_dief1x(101, "can't happen: unknown strictness") ;
      }
    }
  }
  {
    unsigned int r, w ;
    char d[len] ;
    if (!string_unquote_withdelim(d, &w, s + !!delimlen, len, &r, delim, delimlen))
    {
      switch (strictness)
      {
        case 0 : return 0 ;
        case 1 :
        {
          strerr_warnwu1sys("unquote") ;
          return 0 ;
        }
        case 2 :
        {
          char fmt[40] ;
          unsigned int n = (len + !!delimlen) < 40 ? (len + !!delimlen) : 36 ;
          byte_copy(fmt, n, s) ;
          if ((len + !!delimlen) >= 40)
          {
            byte_copy(fmt+n, 3, "...") ;
            n += 3 ;
          }
          fmt[n] = 0 ;
          strerr_warnwu3sys("unquote", " line: ", fmt) ;
          return 0 ;
        }
        case 3 :
        {
          int e = errno ;
          buffer_flush(buffer_1) ;
          errno = e ;
          strerr_diefu1sys(100, "unquote") ;
        }
        default : strerr_dief1x(101, "can't happen: unknown strictness") ;
      }
    }
    if (delimlen)
    {
      if (r == len)
      {
        switch (strictness)
        {
          case 0 : return 0 ;
          case 1 :
          {
            strerr_warnwu2x("unquote", ": no ending quote character") ;
            return 0 ;
          }
          case 2 :
          {
            char fmt[40] ;
            unsigned int n = len < 40 ? len : 36 ;
            byte_copy(fmt, n, s) ;
            if (len >= 40)
            {
              byte_copy(fmt+n, 3, "...") ;
              n += 3 ;
            }
            fmt[n] = 0 ;
            strerr_warnwu5x("unquote", ": no ending quote character", " in ", "line: ", fmt) ;
            return 0 ;
          }
          case 3 :
          {
            int e = errno ;
            buffer_flush(buffer_1) ;
            errno = e ;
            strerr_diefu2x(100, "unquote", ": no ending quote character") ;
          }
          default : strerr_dief1x(101, "can't happen: unknown strictness") ;
        }
      }
      else if ((r < len-1) && (strictness >= 2))
      {
        char fmtnum[UINT_FMT] ;
        char fmtden[UINT_FMT] ;
        char fmt[40] ;
        unsigned int n = len < 39 ? len+1 : 36 ;
        byte_copy(fmt, n, s) ;
        if (len >= 39)
        {
          byte_copy(fmt+n, 3, "...") ;
          n += 3 ;
        }
        fmt[n] = 0 ;
        fmtnum[uint_fmt(fmtnum, r+1)] = 0 ;
        fmtden[uint_fmt(fmtden, len)] = 0 ;
        strerr_warnw7x("found ending quote character at position ", fmtnum, "/", fmtden, ", ignoring remainder of ", "line: ", fmt) ;
      }
    }
    if (buffer_putalign(buffer_1, d, w) < (int)w)
      strerr_diefu1sys(111, "write to stdout") ;
  }
  return 1 ;
}


int main (int argc, char const *const *argv)
{
  stralloc src = STRALLOC_ZERO ;
  PROG = "s6-unquote-filter" ;
  {
    subgetopt_t l = SUBGETOPT_ZERO ;
    for (;;)
    {
      register int opt = subgetopt_r(argc, argv, "qQvwd:", &l) ;
      if (opt == -1) break ;
      switch (opt)
      {
        case 'q': strictness = 0 ; break ;
        case 'Q': strictness = 1 ; break ;
        case 'v': strictness = 2 ; break ;
        case 'w': strictness = 3 ; break ;
        case 'd': delim = l.arg ; break ;
        default : strerr_dieusage(100, USAGE) ;
      }
    }
    argc -= l.ind ; argv += l.ind ;
  }
  delimlen = str_len(delim) ;
  for (;;)
  {
    int r ;
    src.len = 0 ;
    r = skagetln(buffer_0f1, &src, '\n') ;
    if (!r) break ;
    if (r < 0)
    {
      if (errno != EPIPE) strerr_diefu1sys(111, "read from stdin") ;
    }
    else src.len-- ;
    if (!doit(src.s, src.len))
    {
      if (buffer_putalign(buffer_1, src.s, src.len) < (int)src.len)
        strerr_diefu1sys(111, "write to stdout") ;
    }
    if (r > 0)
    {
      if (buffer_putalign(buffer_1, "\n", 1) < 1)
        strerr_diefu1sys(111, "write to stdout") ;
    }
  }
  return 0 ;
}
