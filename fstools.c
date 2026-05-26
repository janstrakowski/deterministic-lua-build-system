void creattool(int dir, const char *path, uint64_t flags, mode_t mode, ...) {
  struct stat sb;
  const char *lastslash;
  char pathbuf[PATH_MAX];
  mode_t updirmode;
  va_list ap;
  int fd;

  errif(!(flags & CREATTOOL_FILE && !(flags & CREATTOOL_DIR) ||
      !(flags & CREATTOOL_FILE) && flags & CREATTOOL_DIR));
  errif(sizeof(pathbuf) <= strlen(path));

  updirmode = mode;
  if (updirmode & S_IRUSR) updirmode |= S_IXUSR;
  if (updirmode & S_IRGRP) updirmode |= S_IXGRP;
  if (updirmode & S_IROTH) updirmode |= S_IXOTH;
  if (flags & CREATTOOL_UPDIRMODE) {
    va_start(ap, mode);
    updirmode = va_arg(ap, mode_t);
    va_end(ap);
  }

  if (flags & CREATTOOL_UPDIRS) {
    lastslash = path;
    while (1) {
      lastslash = strchr(lastslash, '/') + 1;
      if (lastslash == NULL) break;
      if (lastslash == path + 1) continue;
      memcpy(pathbuf, path, (size_t) lastslash - (size_t) path - 2);
      pathbuf[(size_t) lastslash - (size_t) path - 1] = '\0';

      if (fstatat(dir, pathbuf, &sb, 0) == -1) {
        errif(errno != ENOENT);
        errif(mkdirat(dir, pathbuf, updirmode) != 0);
      }
    }
  }

  if (flags & CREATTOOL_FILE) {
    fd = openat(dir, path, O_WRONLY | O_CREAT | (flags & CREATTOOL_EXCL)? O_EXCL : 0, mode);
    errif(fd == -1);
    errif(close(fd) != 0);
  }

  if (flags & CREATTOOL_DIR) {
    if (mkdirat(dir, path, mode) == -1) {
      errif(errno != EEXIST);
      errif(flags & CREATTOOL_EXCL);
    }
  }
}

void copytool(int srcdir, const char *src, int destdir, const char *dest, int flags) {
  struct stat statbuf;
  int srcfd;
  int destfd;
  int dirfd;

  errif(fstatat(srcdir, src, &statbuf, AT_SYMLINK_NOFOLLOW | AT_EMPTY_PATH));
  if (S_ISREG(statbuf.st_mode)) {
    if (strcmp(src, "")) {
      srcfd = openat(srcdir, src, O_RDONLY);
    } else {
      srcfd = srcdir;
    }
    errif(srcfd == -1);
    if (strcmp(dest, "")) {
      destfd = openat(destdir, dest, O_WRONLY | O_CREAT, 0666);
    } else {
      destfd = destdir;
    }
    errif(destfd == -1);
    off_t bytesleft = statbuf.st_size;
    while (1) {
      ssize_t bytestransferred = sendfile(destfd, srcfd, NULL, bytesleft);
      errif(bytestransferred == -1);
      if (bytesleft == bytestransferred) {
        break;
      }
      bytesleft -= bytestransferred;
    }
    if (strcmp(src, "")) {
      errif(close(srcfd));
    }
    if (strcmp(dest, "")) {
      errif(close(destfd));
    }
  } else if (S_ISDIR(statbuf.st_mode)) {
    if (!strcmp(src, "")) {
      dirfd = srcdir;
    } else {
      dirfd = openat(srcdir, src, O_RDONLY);
      errif(dirfd == -1);
    }
    DIR *dirstream = fdopendir(dirfd);
    errif(dirstream == NULL);
    if (mkdirat(destdir, dest, 0777) == -1) {
      errif(errno != EEXIST);
    }

    while (1) {
      errno = 0;
      struct dirent *dent = readdir(dirstream);
      if (dent == NULL) {
        if (errno == 0) {
          break;
        }
        errif(1);
      }
      if (strcmp(dent->d_name, ".") == 0) {
        continue;
      }
      if (strcmp(dent->d_name, "..") == 0) {
        continue;
      }
      char newsrc[PATH_MAX];
      char newdest[PATH_MAX];
      if (strlen(src) + 1 + strlen(dent->d_name) > PATH_MAX) {
        errno = ENAMETOOLONG;
        if (closedir(dirstream) == -1) {
          close(dirfd);
          errif(1);
        }
        close(dirfd);
        errif(1);
      }
      if (strlen(dest) + 1 + strlen(dent->d_name) > PATH_MAX) {
        errno = ENAMETOOLONG;
        if (closedir(dirstream) == -1) {
          close(dirfd);
          errif(1);
        }
        close(dirfd);
        errif(1);
      }
      strcpy(newsrc, src);
      if (strcmp(src, "")) {
        strcat(newsrc, "/");
      }
      strcat(newsrc, dent->d_name);
      strcpy(newdest, dest);
      if (strcmp(dest, "")) {
        strcat(newdest, "/");
      }
      strcat(newdest, dent->d_name);
      copytool(srcdir, newsrc, destdir, newdest, flags);
    }
    if (closedir(dirstream) == -1) {
      close(dirfd);
      errif(1);
    }
  } else if (S_ISLNK(statbuf.st_mode)) {
    char tgtpath[PATH_MAX + 1];
    ssize_t bytesplaced = readlinkat(srcdir, src, tgtpath, PATH_MAX + 1);
    if (bytesplaced == -1) {
      errif(1);
    }
    if (bytesplaced == PATH_MAX + 1) {
      errno = ENAMETOOLONG;
      errif(1);
    }
    errif(!strcmp(dest, ""));
    if (symlinkat(tgtpath, destdir, dest) == -1) {
      errif(1);
    }
  } else {
    errno = EINVAL;
    errif(1);
  }
}
