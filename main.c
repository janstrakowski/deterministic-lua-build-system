# define _GNU_SOURCE

# include "crypto-algorithms/base64.h"
# include "crypto-algorithms/sha256.h"
# include <dirent.h>
# include <errno.h>
# include <limits.h>
# include <linux/sched.h>
# include "lua/lua.h"
# include "lua/lauxlib.h"
# include "lua/lualib.h"
# include <pthread.h>
# include <sched.h>
# include "stb_ds.h"
# include <stdarg.h>
# include <stdlib.h>
# include <stdio.h>
# include <stdint.h>
# include <string.h>
# include <sys/epoll.h>
# include <sys/eventfd.h>
# include <sys/mman.h>
# include <sys/mount.h>
# include <sys/sendfile.h>
# include <sys/socket.h>
# include <sys/stat.h>
# include <sys/syscall.h>
# include <sys/wait.h>
# include <unistd.h>

#define STB_DS_IMPLEMENTATION
# include "stb_ds.h"
#include "crypto-algorithms/base64.h"
#include "crypto-algorithms/sha256.h"

# include "errorhandling.h"
# include "fstools.h"
# include "build.h"
# include "lualoaddump.h"
# include "pathhandling.h"
# include "specialfdindices.h"
# include "supervisor.h"
# include "lfslib.h"
# include "lxiolib.h"
# include "luaapi.h"

#include "crypto-algorithms/base64.c"
#include "crypto-algorithms/sha256.c"
#include "build.c"
#include "common.c"
#include "fstools.c"
#include "lfslib.c"
#include "luaapi.c"
#include "lualoaddump.c"
#include "lxiolib.c"
#include "pathhandling.c"
#include "supervisor.c"

#define intsrcpath "/src"
#define intdestpath "/dest"

uint32_t main(uint32_t argc, char* argv[]) {
  int32_t opt;
  int stderr_fd;

  stderr_fd = STDERR_FILENO;
  errif(fcntl(stderr_fd, F_GETFL) == -1);
  for (int fd = 3; fd <= FDI_LASTI; fd++) {
    if (fd == stderr_fd) continue;
    errif(dup2(stderr_fd, fd) == -1);
  }

  cli = malloc(sizeof(cli_args));
  cli_defaults(cli, argv[0]);

  while(1){
    opt = getopt(argc, argv, "s:d:w:c:x:");
    if (opt == 's') {
      checkpathlen(optarg);
      strcpy(cli->src, optarg);
      continue;
    }
    if (opt == 'd') {
      checkpathlen(optarg);
      strcpy(cli->dest, optarg);
      continue;
    }
    if (opt == 'w') {
      checkpathlen(optarg);
      strcpy(cli->workspace, optarg);
      continue;
    }
    if (opt == 'c') {
      checkpathlen(optarg);
      strcpy(cli->cache, optarg);
      continue;
    }
    if (opt == 'x') {
      checkpathlen(optarg);
      strcpy(cli->script, optarg);
      continue;
    }
    break;
  }

  startassupervisor();
  return 0;
}
