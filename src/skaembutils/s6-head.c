/* ISC license. */

#include <errno.h>
#include "allreadwrite.h"
#include "sgetopt.h"
#include "bytestr.h"
#include "fmtscan.h"
#include "buffer.h"
#include "strerr2.h"
#include "djbunix.h"

#define USAGE "s6-head [ -S ] [ -1..9 | -n lines | -c chars ] [ file... ]"
#define dieusage() strerr_dieusage(100, USAGE)

typedef int headfunc_t (int, unsigned int) ;
typedef headfunc_t *headfunc_t_ref ;

static int dolines (int fd, unsigned int lines)
{
 /* We can't use iobuffer here because we have to watch the data. */
 /* XXX: Buffers sharing the same storage area for optimization... */
 /* XXX: Violently breaks buffer encapsulation. Don't try this at home. */

  char buf[BUFFER_INSIZE] ;
  buffer in = BUFFER_INIT(&fd_read, fd, buf, BUFFER_INSIZE) ;
  buffer out = BUFFER_INIT((iofunc_t_ref)&fd_write, 1, buf, BUFFER_INSIZE) ;
  while (lines)
  {
    unsigned int n ;
    char *p ;
    int r = buffer_fill(&in) ; out.n = in.n ; out.p = in.p ;
    if (r <= 0) return !r ;
    p = buffer_PEEK(&in) ;
    n = buffer_len(&in) ;
    while (n)
    {
      register unsigned int i = byte_chr(p, n, '\n') ;
      if (i < n)
      {
        p += i+1 ; n -= i+1 ;
        if (!--lines)
        {
          out.n = p - buffer_PEEK(&in) ;
          break ;
        }
      }
      else
      {
        p += i ; n -= i ;
      }
    }
    r = buffer_flush(&out) ; in.n = out.n ; in.p = out.p ;
    if (r < 0) return 0 ;
  }
  return 1 ;
}

static int safedolines (int fd, unsigned int lines)
{
  char tmp[lines] ;
  while (lines)
  {
    unsigned int r = allread(fd, tmp, lines) ;
    if ((r < lines) && (errno != EPIPE)) return 0 ;
    lines -= byte_count(tmp, r, '\n') ;
    if (buffer_putalign(buffer_1, tmp, r) < (int)r) return 0 ;
  }
  if (buffer_flush(buffer_1) < 0) return 0 ;
  return 1 ;
}

static int safedochars (int fd, unsigned int chars)
{
  return (fd_catn(fd, 1, chars) >= chars) ;
}


int main (int argc, char const *const *argv)
{
  headfunc_t_ref f ;
  unsigned int lines = 10 ;
  int islines = 1, safe = 0 ;
  PROG = "s6-head" ;
  {
    subgetopt_t l = SUBGETOPT_ZERO ;
    int done = 0 ;
    for (;;)
    {
      register int opt = subgetopt_r(argc, argv, "S123456789n:c:", &l) ;
      if (opt == -1) break ;
      switch (opt)
      {
        case 'S' : safe = 1 ; break ;
        case '1' :
        case '2' :
        case '3' :
        case '4' :
        case '5' :
        case '6' :
        case '7' :
        case '8' :
        case '9' :
        {
          if (done) dieusage() ;
          islines = 1 ;
          lines = opt - '0' ;
          done = 1 ;
          break ;
        }
        case 'n' :
        {
          if (done || !uint0_scan(l.arg, &lines))
            strerr_dieusage(100, USAGE) ;
          islines = 1 ;
          done = 1 ;
          break ;
        }
        case 'c' :
        {
          if (done || !uint0_scan(l.arg, &lines))
            strerr_dieusage(100, USAGE) ;
          islines = 0 ;
          done = 1 ;
          break ;
        }
        default : strerr_dieusage(100, USAGE) ;
      }
    }
    argc -= l.ind ; argv += l.ind ;
  }
  if (argc) safe = 0 ;
  f = islines ? safe ? &safedolines : &dolines : &safedochars ;
  if (!argc)
  {
    if (!(*f)(0, lines))
      strerr_diefu1sys(111, "head stdin") ;
  }
  else
  {
    unsigned int i = 0 ;
    for (; argv[i] ; i++)
    {
      int fd ;
      if (i) buffer_putnoflush(buffer_1, "\n", 1) ;
      buffer_putnoflush(buffer_1, "==> ", 4) ;
      if ((buffer_puts(buffer_1, argv[i]) <= 0)
       || (buffer_putflush(buffer_1, " <==\n", 5) < 0))
        strerr_diefu1sys(111, "write to stdout") ;
      if ((argv[i][0] == '-') && !argv[i][1]) fd = 0 ;
      else fd = open_readb(argv[i]) ;
      if (fd == -1)
        strerr_diefu3sys(111, "open ", argv[i], " for reading") ;
      if (!(*f)(fd, lines))
        strerr_diefu2sys(111, "head ", argv[i]) ;
      fd_close(fd) ;
    }
  }
  return 0 ;
}
