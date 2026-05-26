#define SUPVSRFDI_EPOLL 0
#define SUPVSRFDI_LAST 0

typedef struct supvsr_server {
  int *fds;
  pthread_t thread;
} supvsr_server;

# define SUPVSRREQ_NEWSCK 0
# define SUPVSRREQ_PUT 1
# define SUPVSRREQ_QUERY 2
# define SUPVSRREQ_GET 3
# define SUPVSRREQ_OPENEXE 4

typedef struct {
  int type;
  char sha256[32];
} supvsr_msg;

struct cli_args {
  char src[PATH_MAX];
  char dest[PATH_MAX];
  char workspace[PATH_MAX];
  char cache[PATH_MAX];
  char thisexecutable[PATH_MAX];
  char script[PATH_MAX];
};

void cli_defaults(cli_args *a, const char *thisexecutable) {
  strcpy(a->src, "");
  strcpy(a->dest, "pldest");
  strcpy(a->workspace, "plworkspace");
  strcpy(a->cache, "plcache");
  strcpy(a->thisexecutable, thisexecutable);
  strcpy(a->script, "build.lua");
}

int supvsr_sendmsg(int sck, supvsr_msg *payload, int fd) {
  union {
    char buf[CMSG_SPACE(sizeof(int))];
    struct cmsghdr align_to_me;
  } chdr_holder;
  struct cmsghdr *chdr_p;
  struct msghdr hdr;
  struct iovec pl_iov;

  memset(&hdr, 0, sizeof(struct msghdr));

  pl_iov.iov_base = payload;
  pl_iov.iov_len = sizeof(supvsr_msg);
  hdr.msg_iov = &pl_iov;
  hdr.msg_iovlen = 1;

  if (fd >= 0) {
    hdr.msg_control = chdr_holder.buf;
    hdr.msg_controllen = sizeof(chdr_holder.buf);
    chdr_p = CMSG_FIRSTHDR(&hdr);
    chdr_p->cmsg_level = SOL_SOCKET;
    chdr_p->cmsg_type = SCM_RIGHTS;
    chdr_p->cmsg_len = CMSG_LEN(sizeof(int));
    memcpy(CMSG_DATA(chdr_p), &fd, sizeof(int));
  }

  int r = sendmsg(sck, &hdr, 0);
  errif(r != sizeof(supvsr_msg));
  return 0;
}

#define SUPVSRRECV_MAYSHUTDOWN 1
int supvsr_recvmsg(int sck, supvsr_msg *payload, int *fd, int flags) {
  struct cmsghdr *chdr_p;
  struct msghdr hdr;
  struct iovec pl_iov;
  union {
    struct cmsghdr align;
    char buf[CMSG_SPACE(sizeof(int))];
  } cmsg_holder;

  memset(&hdr, 0, sizeof(struct msghdr));
  pl_iov.iov_base = payload;
  pl_iov.iov_len = sizeof(supvsr_msg);
  hdr.msg_iov = &pl_iov;
  hdr.msg_iovlen = 1;
  hdr.msg_control = cmsg_holder.buf;
  hdr.msg_controllen = sizeof(cmsg_holder.buf);

  int r = recvmsg(sck, &hdr, 0);
  if (flags & SUPVSRRECV_MAYSHUTDOWN && r == 0) {
    return 1;
  }
  errif(r != sizeof(supvsr_msg));

  if (fd != NULL) {
    chdr_p = CMSG_FIRSTHDR(&hdr);
    if (chdr_p != NULL) {
      memcpy(fd, CMSG_DATA(chdr_p), sizeof(int));
    }
  }

  return 0;
}

int newsupervisorfd(void) {
  int newsck;
  supvsr_msg req;
  supvsr_msg res;

  req.type = SUPVSRREQ_NEWSCK;
  supvsr_sendmsg(FDI_SUPERVISOR, &req, -1);

  supvsr_recvmsg(FDI_SUPERVISOR, &res, &newsck, 0);

  return newsck;
}

int cache_get(const char *path, const char hash[32]) {
  int fd;
  supvsr_msg req;
  supvsr_msg res;

  creattool(AT_FDCWD, path, CREATTOOL_DIR, 0766);
  fd = open(path, O_RDONLY);
  if (fd == -1) {
    fprintf(stderr, "Could not access '%s': %s.\n", path, strerror(errno));
    exit(errno);
  }

  req.type = SUPVSRREQ_GET;
  memcpy(req.sha256, hash, 32);
  supvsr_sendmsg(FDI_SUPERVISOR, &req, fd);
  close(fd);

  supvsr_recvmsg(FDI_SUPERVISOR, &res, NULL, 0);
  errif(!(res.sha256[0] == 0 || res.sha256[0] == 1));
  return res.sha256[0];
}

int cache_put(const char *path, const char hash[32]) {
  int fd;
  supvsr_msg req;
  supvsr_msg res;

  fd = open(path, O_RDONLY);
  if (fd == -1) {
    fprintf(stderr, "Could not access '%s': %s.\n", path, strerror(errno));
    exit(errno);
  }

  req.type = SUPVSRREQ_PUT;
  memcpy(req.sha256, hash, 32);
  supvsr_sendmsg(FDI_SUPERVISOR, &req, fd);
  close(fd);

  supvsr_recvmsg(FDI_SUPERVISOR, &res, NULL, 0);
  return 0;
}

int cache_query(const char hash[32]) {
  supvsr_msg req;
  supvsr_msg res;

  req.type = SUPVSRREQ_QUERY;
  memcpy(req.sha256, hash, 32);
  supvsr_sendmsg(FDI_SUPERVISOR, &req, -1);
  
  supvsr_recvmsg(FDI_SUPERVISOR, &res, NULL, 0);
  return res.sha256[0];
}

int openthisexe() {
  int exefd;
  supvsr_msg req;
  supvsr_msg res;

  req.type = SUPVSRREQ_OPENEXE;
  supvsr_sendmsg(FDI_SUPERVISOR, &req, -1);

  supvsr_recvmsg(FDI_SUPERVISOR, &res, &exefd, 0);
  return exefd;
}

void cache_mkpath(const char hash[32], char path[PATH_MAX]) {
  char *pos = path;
  char *endpos;

  strcpy(path, cli->cache);
  pos += strlen(cli->cache);
  strcpy(pos, "/sha256_");
  pos += strlen("/sha256_");
  endpos = pos + base64_encode(hash, pos, 32, 0);
  *endpos = '\0';
  for (char *cpos = pos; cpos != endpos; cpos++) {
    if (*cpos == '/') {
      *cpos = '_';
    }
  }
}

int cache_handleput(supvsr_server *srv, const char hash[32], int obj) {
  char path[PATH_MAX];

  cache_mkpath(hash, path);
  copytool(obj, "", AT_FDCWD, path, 0);
  return 0;
}

int cache_handleget(supvsr_server *srv, const char hash[32], int obj) {
  char path[PATH_MAX];
  int treeopenedobj;

  cache_mkpath(hash, path);

  treeopenedobj = open_tree(AT_FDCWD, path, OPEN_TREE_CLONE);
  if (treeopenedobj == -1) {
    if (errno == ENOENT) {
      return 1;
    }
    fprintf(stderr, "Could not access '%s': %s.\n", path, strerror(errno));
    exit(errno);
  }
  errif(move_mount(treeopenedobj, "", obj, "", MOVE_MOUNT_F_EMPTY_PATH | MOVE_MOUNT_T_EMPTY_PATH) != 0);
  return 0;
}

int cache_handlequery(supvsr_server *srv, const char hash[32]) {
  struct stat statbuf;
  char path[PATH_MAX];

  cache_mkpath(hash, path);
  errif(stat(cli->cache, &statbuf));
  if (stat(path, &statbuf) == -1) {
    errif(errno != ENOENT);
    return 0;
  }
  return 1;
}

void* supervisor_handler(void *arg) {
  struct epoll_event event;
  struct epoll_event eventtoregister;
  int fdnum;
  supvsr_msg req;
  int reqfd = -1;
  supvsr_msg res;
  int thisexefd;
  int sckpair[2];
  supvsr_server *srv;

  srv = (supvsr_server *) arg;

  while (1) {
    fdnum = epoll_wait(srv->fds[SUPVSRFDI_EPOLL], &event, 1, -1);
    if (fdnum == -1 && errno == EINTR) continue;
    errif(fdnum == -1);
    errif(fdnum != 1);

    if (supvsr_recvmsg(srv->fds[event.data.u32], &req, &reqfd, SUPVSRRECV_MAYSHUTDOWN) == 1) {
      errif(epoll_ctl(srv->fds[SUPVSRFDI_EPOLL], EPOLL_CTL_DEL, srv->fds[event.data.u32], NULL) != 0);
      errif(close(srv->fds[event.data.u32]) != 0);
      arrdel(srv->fds, event.data.u32);
      continue;
    }

    if (req.type == SUPVSRREQ_NEWSCK) {
      errif(socketpair(AF_UNIX, SOCK_SEQPACKET, 0, sckpair) != 0);
      arrput(srv->fds, sckpair[0]);
      eventtoregister.data.u32 = arrlen(srv->fds) - 1;
      eventtoregister.events = EPOLLIN | EPOLLRDHUP;
      errif(epoll_ctl(srv->fds[SUPVSRFDI_EPOLL], EPOLL_CTL_ADD, sckpair[0], &eventtoregister) != 0);

      res.type = SUPVSRREQ_NEWSCK;
      supvsr_sendmsg(srv->fds[event.data.u32], &res, sckpair[1]);
      errif(close(sckpair[1]) != 0);
    } else if (req.type == SUPVSRREQ_PUT) {
      cache_handleput(srv, req.sha256, reqfd);

      res.type = SUPVSRREQ_PUT;
      supvsr_sendmsg(srv->fds[event.data.u32], &res, -1);
    } else if (req.type == SUPVSRREQ_GET) {
      int rslt = cache_handleget(srv, req.sha256, reqfd);

      res.type = SUPVSRREQ_GET;
      memset(res.sha256, (char) rslt, 32);
      supvsr_sendmsg(srv->fds[event.data.u32], &res, -1);
    } else if (req.type == SUPVSRREQ_OPENEXE) {
      thisexefd = open(cli->thisexecutable, O_RDONLY);
      errif(thisexefd == -1);

      res.type = SUPVSRREQ_OPENEXE;
      supvsr_sendmsg(srv->fds[event.data.u32], &res, thisexefd);
      errif(close(thisexefd) != 0);
    } else if (req.type == SUPVSRREQ_QUERY) {
      int rslt = cache_handlequery(srv, req.sha256);

      res.type = SUPVSRREQ_QUERY;
      memset(res.sha256, (char) rslt, 32);
      supvsr_sendmsg(srv->fds[event.data.u32], &res, -1);
    }
  }
}

int startsupervisorserver() {
  struct epoll_event event;
  int sckpair[2];
  supvsr_server *srv = malloc(sizeof(supvsr_server));

  creattool(AT_FDCWD, cli->cache, CREATTOOL_DIR, 0777);

  srv->fds = NULL;
  arraddnptr(srv->fds, SUPVSRFDI_LAST + 1);
  srv->fds[SUPVSRFDI_EPOLL] = epoll_create(1);
  errif(srv->fds[SUPVSRFDI_EPOLL] == -1);

  errif(socketpair(AF_UNIX, SOCK_SEQPACKET, 0, sckpair) != 0);
  arrput(srv->fds, sckpair[0]);
  errif(dup2(sckpair[1], FDI_SUPERVISOR) == -1);
  errif(close(sckpair[1]) != 0);
  event.data.u32 = arrlen(srv->fds) - 1;
  event.events = EPOLLIN | EPOLLRDHUP;
  errif(epoll_ctl(srv->fds[SUPVSRFDI_EPOLL], EPOLL_CTL_ADD, sckpair[0], &event) != 0);

  errno = pthread_create(&srv->thread, NULL, &supervisor_handler, srv);
  errif(errno != 0);
  return 0;
}

int startassupervisor() {
  char dpathbuf[PATH_MAX];
  build_args bargs;
  char cachepath[PATH_MAX];

  errif(unshare(CLONE_NEWUSER | CLONE_NEWNS) != 0);

  startsupervisorserver();

  if (strcmp(cli->src, "")) {
    bargs.srcfd = open_tree(AT_FDCWD, cli->src, OPEN_TREE_CLONE);
    errif(bargs.srcfd == -1);
  } else {
    bargs.srcfd = -2;
  }
  bargs.destpath = NULL;
  bargs.code = open(cli->script, O_RDONLY);
  errif(bargs.code == -1);
  bargs.workspace = cli->workspace;
  bargs.chunkname = cli->script;
  build(&bargs);

  cache_mkpath(bargs.hash, cachepath);
  copytool(AT_FDCWD, cachepath, AT_FDCWD, cli->dest, 0);
  exit(0);
}

