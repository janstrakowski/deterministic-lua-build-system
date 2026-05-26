void *luanull = "luanull";

uint32_t mkdirs(const char *path, mode_t mode) {
  const char *lastslashpos = path;
  char currentpath[PATH_MAX];
  while (1) {
    lastslashpos = strchr(lastslashpos, '/');
    if (lastslashpos == NULL) {
      strcpy(currentpath, path);
    } else {
      strncpy(currentpath, path, (int32_t) (lastslashpos - path));
      currentpath[(int32_t) (lastslashpos - path)] = '\0';
    }

    if (strcmp(currentpath, "") != 0) {
      struct stat finfo;
      if (stat(currentpath, &finfo) == -1) {
        if (errno != ENOENT) {
          return -1;
        } else {
          if (mkdir(currentpath, mode) == -1) {
            return -1;
          }
        }
      } else {
        if (!S_ISDIR(finfo.st_mode)) {
          errno = ENOTDIR;
          return -1;
        }
      }
    }
    
    if (lastslashpos == NULL) {
      break;
    }
    lastslashpos++;
  }
  return 0;
}

int rmr(const char *path, int flags) {
  struct stat statbuf;
  if (lstat(path, &statbuf) == -1) {
    return -1;
  }
  if (S_ISREG(statbuf.st_mode) || S_ISLNK(statbuf.st_mode)) {
    if (unlink(path) == -1) {
      return -1;
    }
  } else if (S_ISDIR(statbuf.st_mode)) {
    DIR *dstream = opendir(path);
    if (dstream == NULL) {
      return -1;
    }
    while (1) {
      errno = 0;
      struct dirent *dent = readdir(dstream);
      if (dent == NULL) {
        if (errno == 0) {
          break;
        }
        if (closedir(dstream) == -1) {
          return -1;
        }
        return -1;
      }

      if (strcmp(dent->d_name, ".") == 0 ||
          strcmp(dent->d_name, "..") == 0) {
        continue;
      }

      char newpath[PATH_MAX];
      if (strlen(path) + 1 + strlen(dent->d_name) >= PATH_MAX) {
        errno = ENAMETOOLONG;
        if (closedir(dstream) == -1) {
          return -1;
        }
        return -1;
      }
      strcpy(newpath, path);
      strcat(newpath, "/");
      strcat(newpath, dent->d_name);

      if (rmr(newpath, flags & ~RMR_KEEPCONTAINER) == -1) {
        if (closedir(dstream) == -1) {
          return -1;
        }
        return -1;
      }
    }
    if (!(flags & RMR_KEEPCONTAINER) && rmdir(path) == -1) {
      if (closedir(dstream) == -1) {
        return -1;
      }
      return -1;
    }
    if (closedir(dstream) == -1) {
      return -1;
    }
  } else {
    errno = ENOTSUP;
    return -1;
  }
  return 0;
}

void *lnullunwrp(void *value) {
  if (value == luanull) {
    return NULL;
  }
  return value;
}

const void *lnullwrp(void *value) {
  if (value == NULL) {
    return luanull;
  }
  return value;
}

int stdtbltostring(lua_State *ls) {
  luaL_checktype(ls, 1, LUA_TTABLE);
  luaL_Buffer b;
  luaL_buffinit(ls, &b);
  luaL_addstring(&b, "{");
  int index = 1;
  while (1) {
    int valtype = lua_geti(ls, 1, index);
    if (valtype == LUA_TNIL) {
      break;
    }
    const char *hmnrdbl = lua_tolstring(ls, -1, NULL);
    if (index > 1) {
      luaL_addstring(&b, ", ");
    }
    luaL_addstring(&b, hmnrdbl);
    lua_pop(ls, 1);
    index++;
  }
  luaL_addstring(&b, "}");
  luaL_pushresult(&b);
  return 1;
}

int errnotostring(lua_State *ls) {
  int givenerrno = luaL_checkinteger(ls, 1);
  lua_pushstring(ls, strerror(givenerrno));
  return 1;
}

int pusherrno(lua_State *ls, int givenerrno) {
  lua_pushinteger(ls, givenerrno);
  luaL_setmetatable(ls, "errno");
}

