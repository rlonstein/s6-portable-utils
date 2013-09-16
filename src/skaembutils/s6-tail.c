/* ISC license. */

#include <errno.h>
#include "sgetopt.h"
#include "allreadwrite.h"
#include "bytestr.h"
#include "fmtscan.h"
#include "buffer.h"
#include "strerr2.h"
#include "djbunix.h"
#include "skamisc.h"

#define USAGE "s6-tail [ -c chars | -n lines | -1..9 ] [ file ]"

typedef int tailfunc_t (int, unsigned int) ;
typedef tailfunc_t *tailfunc_t_ref ;

static int pluslines (int fd, unsigned int n)
{
  {
    char buf[BUFFER_INSIZE] ;
    buffer b = BUFFER_INIT(&fd_read, fd, buf, BUFFER_INSIZE) ;
    unsigned int count = 0 ;
    while (count < n)
    {
      int r = buffer_fill(&b) ;
      if (r <= 0) return !r ;
      while (buffer_len(&b) && (count < n))
      {
        char *p = buffer_RPEEK(&b) ;
        unsigned int i = byte_chr(p, buffer_len(&b), '\n') ;
        if (i < buffer_len(&b))
        {
          count++ ; i++ ;
        }
        buffer_RSEEK(&b, i) ;
      }
    }
    b.op = (iofunc_t_ref)&fd_write ; /* XXX: hotwiring rbuf into wbuf */
    b.fd = 1 ;
    if (buffer_flush(&b) < 0) return 0 ;
  }
  return (fd_cat(fd, 1) >= 0) ;
}

static int pluschars (int fd, unsigned int n)
{
  if (n > 1)
  {
    int nil = open_write("/dev/null") ;
    if (nil < 0) return 0 ;
    if (!fd_catn(fd, nil, n-1))
    {
      register int e = errno ;
      fd_close(nil) ;
      errno = e ;
      return 0 ;
    }
    fd_close(nil) ;
  }
  return (fd_cat(fd, 1) >= 0) ;
}

static int minuslines (int fd, unsigned int n)
{
  register unsigned int i ;
  if (!slurp(&satmp, fd)) return 0 ;
  if (satmp.s[satmp.len-1] == '\n') n++ ;
  for (i = satmp.len-1 ; i ; i--)
    if ((satmp.s[i] == '\n') && !--n) { i++ ; break ; }
  return (buffer_putflush(buffer_1, satmp.s + i, satmp.len - i) == (int)(satmp.len - i)) ;
}

static int minuschars (int fd, unsigned int n)
{
  if (!slurp(&satmp, fd)) return 0 ;
  if (n > satmp.len) n = satmp.len ;
  return (buffer_putflush(buffer_1, satmp.s + satmp.len - n, n) == (int)n) ;
}

int main (int argc, char const *const *argv)
{
  tailfunc_t_ref f = &minuslines ;
  unsigned int n = 10 ;
  int gotit = 0 ;
  PROG = "s6-tail" ;
  {
    subgetopt_t l = SUBGETOPT_ZERO ;
    for (;;)
    {
      register int opt = subgetopt_r(argc, argv, "123456789n:c:", &l) ;
      if (opt == -1) break ;
      switch (opt)
      {
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
          if (gotit) strerr_dieusage(100, USAGE) ;
          gotit = 1 ;
          f = &minuslines ;
          n = opt - '0' ;
          break ;
        }
        case 'n':
        {
          if (gotit) strerr_dieusage(100, USAGE) ;
          gotit = 1 ;
          f = &minuslines ;
          if (*l.arg == '-') l.arg++ ;
          else if (*l.arg == '+')
          {
            f = &pluslines ;
            l.arg++ ;
          }
          if (!uint0_scan(l.arg, &n)) strerr_dieusage(100, USAGE) ;
          break ;
        }
        case 'c':
        {
          if (gotit) strerr_dieusage(100, USAGE) ;
          gotit = 1 ;
          f = &minuschars ;
          if (*l.arg == '-') l.arg++ ;
          else if (*l.arg == '+')
          {
            f = &pluschars ;
            l.arg++ ;
          }
          if (!uint0_scan(l.arg, &n)) strerr_dieusage(100, USAGE) ;
          break ;
        }
        default : strerr_dieusage(100, USAGE) ;
      }
    }
    argc -= l.ind ; argv += l.ind ;
  }
  if (!n) strerr_dieusage(100, USAGE) ;
  if (!argc)
  {
    if (!(*f)(0, n))
      strerr_diefu1sys(111, "tail stdin") ;
  }
  else
  {
    int fd = open_readb(argv[0]) ;
    if (fd == -1) strerr_diefu3sys(111, "open ", argv[0], " for reading") ;
    if (!(*f)(fd, n))
      strerr_diefu2sys(111, "tail ", argv[0]) ;
    fd_close(fd) ;
  }
  return 0 ;
}
