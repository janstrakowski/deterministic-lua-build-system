int fslib_direntries(lua_State *luast) {
  const char *path = luaL_checkstring(luast, 1);

  lua_newtable(luast);
  luaL_setmetatable(luast, "stdtable");
  DIR *dir = opendir(path);
  if (dir == NULL) {
    luaL_error(luast, "Could not opendir '%s': %s", path, strerror(errno));
  }

  int entord = 0;
  while(1) {
    errno = 0;
    struct dirent* dent = readdir(dir);
    if (dent == NULL) {
      if (errno != 0) {
        luaL_error(luast, "Could not readdir: %s", strerror(errno));
      }
      break;
    }
    entord++;
    lua_pushstring(luast, dent->d_name);
    lua_seti(luast, -2, entord);
  }

  return 1;
}
        
int fs_copy(lua_State *L) {
  const char *src = luaL_checkstring(L, 1);
  const char *dest = luaL_checkstring(L, 2);

  copytool(AT_FDCWD, src, AT_FDCWD, dest, 0);

  return 0;
}

int fs_rename(lua_State *L) {
  const char *oldpath = luaL_checkstring(L, 1);
  const char *newpath = luaL_checkstring(L, 2);

  if (rename(oldpath, newpath) == -1) {
    pusherrno(L, errno);
    lua_error(L);
  }

  return 0;
}

int fs_mkdir(lua_State *L) {
  const char *path = luaL_checkstring(L, 1);
  mode_t mode = 0777;
  if (lua_gettop(L) > 1) {
    mode = luaL_checkinteger(L, 2);
  }

  if (mkdir(path, mode) == -1) {
    pusherrno(L, errno);
    lua_error(L);
  }

  return 0;
}

const luaL_Reg luaapi_fslib[] = {
  {"direntries", &fslib_direntries},
  {"copy", &fs_copy},
  {"rename", &fs_rename},
  {"mkdir", &fs_mkdir},
  {NULL, NULL}
};

int luaapi_fslib_register(lua_State *luast) {
  luaL_newlib(luast, luaapi_fslib);
  lua_setglobal(luast, "fs");
}
