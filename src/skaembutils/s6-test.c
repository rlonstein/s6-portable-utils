/* ISC license. */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "bytestr.h"
#include "fmtscan.h"
#include "strerr2.h"
#include "djbunix.h"

#define USAGE "s6-test expression  or  [ expression ]"

enum opnum
{
  T_NOT,
  T_AND,
  T_OR,
  T_LEFTP,
  T_RIGHTP,
  T_BLOCK,
  T_CHAR,
  T_DIR,
  T_EXIST,
  T_REGULAR,
  T_SGID,
  T_SYMLINK,
  T_STICKY,
  T_NONZERO,
  T_FIFO,
  T_READABLE,
  T_NONZEROFILE,
  T_TERM,
  T_SUID,
  T_WRITABLE,
  T_EXECUTABLE,
  T_ZERO,
  T_EUID,
  T_EGID,
  T_SOCKET,
  T_MODIFIED,
  T_NEWER,
  T_OLDER,
  T_DEVINO,
  T_STREQUAL,
  T_STRNEQUAL,
  T_STRLESSER,
  T_STRLESSERE,
  T_STRGREATER,
  T_STRGREATERE,
  T_NUMEQUAL,
  T_NUMNEQUAL,
  T_NUMGREATER,
  T_NUMGREATERE,
  T_NUMLESSER,
  T_NUMLESSERE
} ;

struct token
{
  char const *string ;
  enum opnum op ;
  unsigned int type ;
} ;

struct node
{
  enum opnum op ;
  unsigned int type ;
  unsigned int arg1 ;
  unsigned int arg2 ;
  char const *data ;
} ;

static unsigned int lex (struct node *tree, char const *const *argv)
{
  static struct token const tokens[44] =
  {
    { "-n", T_NONZERO, 2 },
    { "-z", T_ZERO, 2 },
    { "=", T_STREQUAL, 3 },
    { "!=", T_STRNEQUAL, 3 },
    { "-eq", T_NUMEQUAL, 3 },
    { "-ne", T_NUMNEQUAL, 3 },
    { "-gt", T_NUMGREATER, 3 },
    { "-ge", T_NUMGREATERE, 3 },
    { "-lt", T_NUMLESSER, 3 },
    { "-le", T_NUMLESSERE, 3 },
    { "-f", T_REGULAR, 2 },
    { "-h", T_SYMLINK, 2 },
    { "-L", T_SYMLINK, 2 },
    { "-e", T_EXIST, 2 },
    { "-k", T_STICKY, 2 },
    { "-a", T_AND, 7 },
    { "-o", T_OR, 8 },
    { "!", T_NOT, 6 },
    { "(", T_LEFTP, 4 },
    { ")", T_RIGHTP, 5 },
    { "-b", T_BLOCK, 2 },
    { "-c", T_CHAR, 2 },
    { "-d", T_DIR, 2 },
    { "-g", T_SGID, 2 },
    { "-p", T_FIFO, 2 },
    { "-r", T_READABLE, 2 },
    { "-s", T_NONZEROFILE, 2 },
    { "-t", T_TERM, 2 },
    { "-u", T_SUID, 2 },
    { "-w", T_WRITABLE, 2 },
    { "-x", T_EXECUTABLE, 2 },
    { "-O", T_EUID, 2 },
    { "-U", T_EUID, 2 },
    { "-G", T_EGID, 2 },
    { "-S", T_SOCKET, 2 },
    { "-N", T_MODIFIED, 2 },
    { "-nt", T_NEWER, 3 },
    { "-ot", T_OLDER, 3 },
    { "-ef", T_DEVINO, 3 },
    { "<", T_STRLESSER, 3 },
    { "<=", T_STRLESSERE, 3 },
    { ">", T_STRGREATER, 3 },
    { ">=", T_STRGREATERE, 3 },
    { 0, 0, 0 }
  } ;
  register unsigned int pos = 0 ;

  for (; argv[pos] ; pos++)
  {
    unsigned int i = 0 ;
    tree[pos].data = argv[pos] ;
    for (i = 0 ; tokens[i].string ; i++)
      if (!str_diff(argv[pos], tokens[i].string))
      {
        tree[pos].op = tokens[i].op ;
        tree[pos].type = tokens[i].type ;
        break ;
      }
    if (!tokens[i].string)
    {
      tree[pos].op = T_NONZERO ;
      tree[pos].type = 0 ;
      tree[pos].arg1 = pos ;
      if (*(argv[pos]) == '\\') tree[pos].data++ ; /* better than SUSv3 */
    }
  }
  return pos ;
}

static unsigned int parse (struct node *tree, unsigned int n)
{
  static char const table[9][13] =
  {
    "xssssxsssxxxx",
    "xxxxxaxxxxxxx",
    "xsxxsxsssxxxx",
    "sxxxxxxxxxxxx",
    "xsxxsxsssxxxx",
    "nxxxxNxxxAOEs",
    "xsxxsxsssxxxx",
    "nxxxxNxxxAsxx",
    "nxxxxNxxxAOsx"
  } ;

  unsigned int stack[n+2] ;
  unsigned int sp = 0, pos = 0 ;
  int cont = 1 ;

  stack[0] = n+1 ;
  tree[n].type = 5 ; /* add ) for the final reduce */
  tree[n+1].type = 1 ; /* add EOF */

  while (cont)
  {
    switch (table[tree[pos].type][tree[stack[sp]].type])
    {
      case 'x' : /* error */
      {
        char fmt[FMT_ULONG] ;
        fmt[fmt_uint(fmt, pos)] = 0 ;
        strerr_dief2x(100, "parse error at argument ", fmt) ;
        break ;
      }
      case 'a' : /* accept */
      {
        cont = 0 ;
        break ;
      }
      case 's' : /* shift */
      {
        stack[++sp] = pos++ ;
        break ;
      }
      case 'n' : /* reduce -> expr without nots, from atom */
      {
        switch (tree[stack[sp-1]].type)
        {
          case 2 :
          {
            tree[stack[sp-1]].arg1 = stack[sp] ;
            sp-- ;
            break ;
          }
          case 3 :
          {
            tree[stack[sp-1]].arg1 = stack[sp-2] ;
            tree[stack[sp-1]].arg2 = stack[sp] ;
            stack[sp-2] = stack[sp-1] ;
            sp -= 2 ;
            break ;
          }
          /* default : assert: its a zero */
        }
        tree[stack[sp]].type = 9 ;
        while (tree[stack[sp-1]].type == 6)
        {
          tree[stack[sp-1]].type = 9 ;
          tree[stack[sp-1]].arg1 = stack[sp] ;
          sp-- ;
        }
        break ;
      }
      case 'N' : /* reduce -> expr without nots, from expr */
      {
        if (tree[stack[sp-2]].type != 4)
        {
          char fmt[FMT_ULONG] ;
          fmt[fmt_uint(fmt, pos)] = 0 ;
          strerr_dief2x(100, "parse error: bad right parenthesis at argument ", fmt) ;
        }
        stack[sp-2] = stack[sp-1] ;
        sp -= 2 ;
        tree[stack[sp]].type = 9 ;
        while (tree[stack[sp-1]].type == 6)
        {
          tree[stack[sp-1]].type = 9 ;
          tree[stack[sp-1]].arg1 = stack[sp] ;
          sp-- ;
        }
        break ;
      }
      case 'A' : /* reduce -> exprs without ands */
      {
        if (tree[stack[sp-1]].type == 7)
        {
          tree[stack[sp-1]].arg1 = stack[sp-2] ;
          tree[stack[sp-1]].arg2 = stack[sp] ;
          stack[sp-2] = stack[sp-1] ;
          sp -= 2 ;
        }
        tree[stack[sp]].type = 10 ;
        break ;
      }
      case 'O' : /* reduce -> expr without ors */
      {
        if (tree[stack[sp-1]].type == 8)
        {
          tree[stack[sp-1]].arg1 = stack[sp-2] ;
          tree[stack[sp-1]].arg2 = stack[sp] ;
          stack[sp-2] = stack[sp-1] ;
          sp -= 2 ;
        }
        tree[stack[sp]].type = 11 ;
        break ;
      }
      case 'E' : /* reduce -> expr */
      {
        tree[stack[sp]].type = 12 ;
        break ;
      }
      default : /* can't happen */
        strerr_dief1x(101, "internal error, please submit a bug-report.") ;
    }
  }
  if (sp != 2) strerr_dief1x(100, "parse error: too many left parentheses") ;
  return stack[1] ;
}

static int run (struct node const *tree, unsigned int root)
{
  switch (tree[root].op)
  {
    case T_NOT :
      return !run(tree, tree[root].arg1) ;
    case T_AND :
      return run(tree, tree[root].arg1) && run(tree, tree[root].arg2) ;
    case T_OR :
      return run(tree, tree[root].arg1) || run(tree, tree[root].arg2) ;
    case T_EXIST :
    {
      struct stat st ;
      return !stat(tree[tree[root].arg1].data, &st) ;
    }
    case T_BLOCK :
    {
      struct stat st ;
      if (stat(tree[tree[root].arg1].data, &st) == -1) return 0 ;
      return S_ISBLK(st.st_mode) ;
    }
    case T_CHAR :
    {
      struct stat st ;
      if (stat(tree[tree[root].arg1].data, &st) == -1) return 0 ;
      return S_ISCHR(st.st_mode) ;
    }
    case T_DIR :
    {
      struct stat st ;
      if (stat(tree[tree[root].arg1].data, &st) == -1) return 0 ;
      return S_ISDIR(st.st_mode) ;
    }
    case T_REGULAR :
    {
      struct stat st ;
      if (stat(tree[tree[root].arg1].data, &st) == -1) return 0 ;
      return S_ISREG(st.st_mode) ;
    }
    case T_FIFO :
    {
      struct stat st ;
      if (stat(tree[tree[root].arg1].data, &st) == -1) return 0 ;
      return S_ISFIFO(st.st_mode) ;
    }
    case T_SOCKET :
    {
      struct stat st ;
      if (stat(tree[tree[root].arg1].data, &st) == -1) return 0 ;
      return S_ISSOCK(st.st_mode) ;
    }
    case T_SYMLINK :
    {
      struct stat st ;
      if (lstat(tree[tree[root].arg1].data, &st) == -1) return 0 ;
      return S_ISLNK(st.st_mode) ;
    }
    case T_SGID :
    {
      struct stat st ;
      if (stat(tree[tree[root].arg1].data, &st) == -1) return 0 ;
      return (st.st_mode & S_ISGID) ;
    }
    case T_SUID :
    {
      struct stat st ;
      if (stat(tree[tree[root].arg1].data, &st) == -1) return 0 ;
      return (st.st_mode & S_ISUID) ;
    }
    case T_STICKY :
    {
      struct stat st ;
      if (stat(tree[tree[root].arg1].data, &st) == -1) return 0 ;
      return (st.st_mode & S_ISVTX) ;
    }
    case T_NONZEROFILE :
    {
      struct stat st ;
      if (stat(tree[tree[root].arg1].data, &st) == -1) return 0 ;
      return (st.st_size > 0) ;
    }
    case T_MODIFIED :
    {
      struct stat st ;
      if (stat(tree[tree[root].arg1].data, &st) == -1) return 0 ;
      return (st.st_mtime > st.st_atime) ;
    }
    case T_EUID :
    {
      struct stat st ;
      if (stat(tree[tree[root].arg1].data, &st) == -1) return 0 ;
      return st.st_uid == geteuid() ;
    }
    case T_EGID :
    {
      struct stat st ;
      if (stat(tree[tree[root].arg1].data, &st) == -1) return 0 ;
      return st.st_gid == getegid() ;
    }
    case T_READABLE :
    {
      struct stat st ;
      if (stat(tree[tree[root].arg1].data, &st) == -1) return 0 ;
      if (st.st_uid == geteuid()) return st.st_mode & S_IRUSR ;
      else if (st.st_gid == getegid()) return st.st_mode & S_IRGRP ;
      else return st.st_mode & S_IROTH ;
    }
    case T_WRITABLE :
    {
      struct stat st ;
      if (stat(tree[tree[root].arg1].data, &st) == -1) return 0 ;
      if (st.st_uid == geteuid()) return st.st_mode & S_IWUSR ;
      else if (st.st_gid == getegid()) return st.st_mode & S_IWGRP ;
      else return st.st_mode & S_IWOTH ;
    }
    case T_EXECUTABLE :
    {
      struct stat st ;
      if (stat(tree[tree[root].arg1].data, &st) == -1) return 0 ;
      if (st.st_uid == geteuid()) return st.st_mode & S_IXUSR ;
      else if (st.st_gid == getegid()) return st.st_mode & S_IXGRP ;
      else return st.st_mode & S_IXOTH ;
    }
    case T_NEWER :
    {
      struct stat st1, st2 ;
      if (stat(tree[tree[root].arg1].data, &st1) == -1) return 0 ;
      if (stat(tree[tree[root].arg2].data, &st2) == -1) return 1 ;
      return st1.st_mtime > st2.st_mtime ;
    }
    case T_OLDER :
    {
      struct stat st1, st2 ;
      if (stat(tree[tree[root].arg1].data, &st1) == -1) return 1 ;
      if (stat(tree[tree[root].arg2].data, &st2) == -1) return 0 ;
      return st1.st_mtime < st2.st_mtime ;
    }
    case T_DEVINO :
    {
      struct stat st1, st2 ;
      if (stat(tree[tree[root].arg1].data, &st1) == -1) return 0 ;
      if (stat(tree[tree[root].arg2].data, &st2) == -1) return 1 ;
      return (st1.st_dev == st2.st_dev) && (st1.st_ino == st2.st_ino) ;
    }
    case T_TERM :
    {
      unsigned int fd ;
      if (!uint0_scan(tree[tree[root].arg1].data, &fd))
        strerr_dief2x(100, tree[root].data, " requires an integer argument") ;
      return isatty((int)fd) ;
    }
    case T_NONZERO :
      return tree[tree[root].arg1].data[0] ;
    case T_ZERO :
      return !tree[tree[root].arg1].data[0] ;
    case T_STREQUAL :
      return !str_diff(tree[tree[root].arg1].data, tree[tree[root].arg2].data) ;
    case T_STRNEQUAL :
      return !!str_diff(tree[tree[root].arg1].data, tree[tree[root].arg2].data) ;
    case T_STRLESSER :
      return str_diff(tree[tree[root].arg1].data, tree[tree[root].arg2].data) < 0 ;
    case T_STRLESSERE :
      return str_diff(tree[tree[root].arg1].data, tree[tree[root].arg2].data) <= 0 ;
    case T_STRGREATER :
      return str_diff(tree[tree[root].arg1].data, tree[tree[root].arg2].data) > 0 ;
    case T_STRGREATERE :
      return str_diff(tree[tree[root].arg1].data, tree[tree[root].arg2].data) >= 0 ;
    case T_NUMEQUAL :
    {
      int n1, n2 ;
      if (!int_scan(tree[tree[root].arg1].data, &n1)
       || !int_scan(tree[tree[root].arg2].data, &n2))
        goto errorint ;
      return n1 == n2 ;
    }
    case T_NUMNEQUAL :
    {
      int n1, n2 ;
      if (!int_scan(tree[tree[root].arg1].data, &n1)
       || !int_scan(tree[tree[root].arg2].data, &n2))
        goto errorint ;
      return n1 != n2 ;
    }
    case T_NUMGREATER :
    {
      int n1, n2 ;
      if (!int_scan(tree[tree[root].arg1].data, &n1)
       || !int_scan(tree[tree[root].arg2].data, &n2))
        goto errorint ;
      return n1 > n2 ;
    }
    case T_NUMGREATERE :
    {
      int n1, n2 ;
      if (!int_scan(tree[tree[root].arg1].data, &n1)
       || !int_scan(tree[tree[root].arg2].data, &n2))
        goto errorint ;
      return n1 >= n2 ;
    }
    case T_NUMLESSER :
    {
      int n1, n2 ;
      if (!int_scan(tree[tree[root].arg1].data, &n1)
       || !int_scan(tree[tree[root].arg2].data, &n2))
        goto errorint ;
      return n1 < n2 ;
    }
    case T_NUMLESSERE :
    {
      int n1, n2 ;
      if (!int_scan(tree[tree[root].arg1].data, &n1)
       || !int_scan(tree[tree[root].arg2].data, &n2))
        goto errorint ;
      return n1 <= n2 ;
    }
    default:
      strerr_dief1x(111, "operation not implemented") ;
  }

errorint:
  strerr_dief2x(100, tree[root].data, " requires integer arguments") ;
}

int main (int argc, char const *const *argv)
{
  PROG = "s6-test" ;
  if (argc <= 1) return 1 ;
  {
    struct node tree[argc + 2] ;    
    unsigned int n = lex(tree, argv+1) ;
    if ((argv[0][0] == '[') && !argv[0][1])
    {
      if (n && (!tree[n-1].type) && (tree[n-1].data[0] == ']') && !tree[n-1].data[1])
        n-- ;
      else strerr_dief1x(100, "parse error: missing closing bracket") ;
    }
    return !run(tree, parse(tree, n)) ;
  }
}
