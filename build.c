struct build_args {
  int srcfd;
  const char *workspace;
  int code;
  const char *chunkname;
  uint64_t flags;
  char hash[32];
  const char *destpath;
  char destpathbuf[PATH_MAX];
};

char * const emptyenvp[1] = {
  NULL
};

void calchash(build_args *ba, char hash[32]) {
  SHA256_CTX ctx;
  off_t foffset;
  ssize_t bytesread;
  char rdbuf[1024];

  sha256_init(&ctx);
  sha256_update(&ctx, ba->chunkname, strlen(ba->chunkname));
  
  foffset = lseek(ba->code, 0, SEEK_CUR);
  while (1) {
    bytesread = read(ba->code, rdbuf, 1024);
    assert(bytesread != -1);
    if (bytesread == 0) break;
    sha256_update(&ctx, rdbuf, bytesread);
  }
  lseek(ba->code, foffset, SEEK_SET);

  sha256_final(&ctx, hash);
}

int buildn = 0;

struct timespec calculate_duration(struct timespec start, struct timespec stop) {
    struct timespec result;

    if ((stop.tv_nsec - start.tv_nsec) < 0) {
        // Borrow 1 second from the seconds field
        result.tv_sec = stop.tv_sec - start.tv_sec - 1;
        result.tv_nsec = stop.tv_nsec - start.tv_nsec + 1000000000L;
    } else {
        result.tv_sec = stop.tv_sec - start.tv_sec;
        result.tv_nsec = stop.tv_nsec - start.tv_nsec;
    }

    return result;
}

void build(build_args *ba) {
  pid_t pid;
  int newsupfd;
  int returned_destpathfd;
  char *returned_destpath;
  char dest_internalpath[PATH_MAX];
  int thisexefd;
  char chunknamebuf[256];
  char *argv[4];
  int status;
  struct timespec starttime, finishtime, duration;

  buildn++;
  clock_gettime(CLOCK_MONOTONIC, &starttime);
  fprintf(stderr, "Begun the build #%d. Time: %ds %dms %dys %dns\n", buildn, starttime.tv_sec, starttime.tv_nsec / 1000000, (starttime.tv_nsec / 1000) % 1000, starttime.tv_nsec % 1000);
  if (ba->destpath != NULL) checkpathlen(ba->destpath);
  checkpathlen(ba->workspace);

  calchash(ba, ba->hash);
  if (cache_query(ba->hash)) {
    if (ba->destpath != NULL) {
      errif(cache_get(ba->destpath, ba->hash));
    }
    clock_gettime(CLOCK_MONOTONIC, &finishtime);
    duration = calculate_duration(starttime, finishtime);
    fprintf(stderr, "Finished the build #%d (cached). Time: %ds %dms %dys %dns. Duration: %ds %dms %dys %dns.\n", buildn, finishtime.tv_sec, finishtime.tv_nsec / 1000000, (finishtime.tv_nsec / 1000) % 1000, finishtime.tv_nsec % 1000, duration.tv_sec, duration.tv_nsec / 1000000, (duration.tv_nsec / 1000) % 1000, duration.tv_nsec % 1000);
    return;
  }

  newsupfd = newsupervisorfd();
  thisexefd = openthisexe();

  returned_destpathfd = memfd_create("returned_destpath", MFD_ALLOW_SEALING);
  errif(returned_destpathfd == -1);
  errif(ftruncate(returned_destpathfd, PATH_MAX));
  errif(fcntl(returned_destpathfd, F_ADD_SEALS, F_SEAL_SHRINK | F_SEAL_GROW));
  returned_destpath = mmap(NULL, PATH_MAX, PROT_READ, MAP_SHARED, returned_destpathfd, 0);
  errif(returned_destpath == MAP_FAILED);

  pid = fork();
  errif(pid == -1);
  if (pid != 0) {
    errif(waitpid(pid, &status, 0) == -1);
    errif(!WIFEXITED(status));
    errif(WEXITSTATUS(status) != 0);

    buildpath(dest_internalpath, ba->workspace, "/", returned_destpath, NULL);
    cache_put(dest_internalpath, ba->hash);
    if (ba->destpath != NULL) {
      errif(cache_get(ba->destpath, ba->hash));
    } else {
      strcpy(ba->destpathbuf, dest_internalpath);
      ba->destpath = ba->destpathbuf;
    }

    close(newsupfd);
    close(thisexefd);
    close(returned_destpathfd);
    clock_gettime(CLOCK_MONOTONIC, &finishtime);
    duration = calculate_duration(starttime, finishtime);
    fprintf(stderr, "Finished the build #%d (uncached). Time: %ds %dms %dys %dns. Duration: %ds %dms %dys %dns.\n", buildn, finishtime.tv_sec, finishtime.tv_nsec / 1000000, (finishtime.tv_nsec / 1000) % 1000, finishtime.tv_nsec % 1000, duration.tv_sec, duration.tv_nsec / 1000000, (duration.tv_nsec / 1000) % 1000, duration.tv_nsec % 1000);
    return;
  }

  

  unshare(CLONE_NEWUSER | CLONE_NEWNS);

  creattool(AT_FDCWD, ba->workspace, CREATTOOL_DIR, 0777);
  rmr(ba->workspace, RMR_KEEPCONTAINER);

  chdir(ba->workspace);
  chroot(".");

  errif(dup2(newsupfd, FDI_SUPERVISOR) == -1);
  if (ba->srcfd != -2) errif(dup2(ba->srcfd, FDI_SRC) == -1);
  errif(dup2(returned_destpathfd, FDI_DESTPATH) == -1);
  errif(dup2(ba->code, FDI_CODE) == -1);
  
  destpath = mmap(NULL, PATH_MAX, PROT_READ | PROT_WRITE, MAP_SHARED, FDI_DESTPATH, 0);
  strcpy(destpath, "/dest");
  syscall(SYS_close_range, FDI_LASTI + 1, ~0U, 0);
  
  lua_State *L = luaL_newstate();
  luaL_openlibs(L);
  luaapi_fslib_register(L);
  xiolib_init(L);
  luaapi_init(L);

  lualoadfd(FDI_CODE, chunkname, L);
  lua_call(L, 0, 0);
  exit(0);
}
