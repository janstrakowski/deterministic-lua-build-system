int lapi_bindsrc(lua_State *L) {
  const char *target;
  struct stat st;
  mode_t file_type;
  mode_t permissions;
  int opentgt;

  target = luaL_checkstring(L, 1);

  errif(fstat(FDI_SRC, &st) == -1);
  file_type = st.st_mode & S_IFMT;
  permissions = 0644;
  if (S_ISDIR(st.st_mode)) {
    errif(mkdir(target, 0755) == -1);
  } else {
    errif(mknod(target, file_type | permissions, st.st_rdev) == -1);
  }

  opentgt = open(target, O_RDONLY);

  errif(move_mount(FDI_SRC, "", opentgt, "", MOVE_MOUNT_F_EMPTY_PATH | MOVE_MOUNT_T_EMPTY_PATH) == -1);
  return 0;
}

int lapi_setdest(lua_State *L) {
  const char *dest;

  dest = luaL_checkstring(L, 1);
  strcpy(destpath, dest);
  return 0;
}

int lapi_build(lua_State *L) {
  build_args bargs;
  lua_Debug ar;

  if (lua_getfield(L, 1, "src") == LUA_TNIL) {
    bargs.srcfd = -2;
  } else {
    fprintf(stderr, "Type: %s", lua_type(L, -1));
    errif(!lua_isstring(L, -1));
    bargs.srcfd = open_tree(AT_FDCWD, luaL_checkstring(L, -1), OPEN_TREE_CLONE);
  }
  lua_getfield(L, 1, "dest");
  errif(!lua_isstring(L, -1));
  bargs.destpath = luaL_checkstring(L, -1);
  lua_getfield(L, 1, "ws");
  errif(!lua_isstring(L, -1));
  bargs.workspace = luaL_checkstring(L, -1);
  luaL_checktype(L, 2, LUA_TFUNCTION);
  lua_pushvalue(L, 2);
  errif(lua_getinfo(L, ">S", &ar) == 0);
  bargs.chunkname = ar.short_src;

  lua_pushvalue(L, 2);
  bargs.code = memfd_create("bargscode", 0);
  errif(bargs.code == -1);
  luadumpfd(bargs.code, L);
  errif(lseek(bargs.code, 0, SEEK_SET) == -1);

  build(&bargs);

  if (bargs.srcfd != -2) errif(close(bargs.srcfd) != 0);
  errif(close(bargs.code) != 0);
  return 0;
}

int luaapi_init(lua_State *ls) {
  luaL_newmetatable(ls, "stdtable");
  lua_pushcfunction(ls, &stdtbltostring);
  lua_setfield(ls, -2, "__tostring");
  lua_pop(ls, 1);

  luaL_newmetatable(ls, "errno");
  lua_pushcfunction(ls, &errnotostring);
  lua_setfield(ls, -2, "__tostring");
  lua_pop(ls, 1);

  lua_pushcfunction(ls, &lapi_build);
  lua_setglobal(ls, "build");

  lua_pushcfunction(ls, &lapi_bindsrc);
  lua_setglobal(ls, "bindsrc");

  lua_pushcfunction(ls, &lapi_setdest);
  lua_setglobal(ls, "setdest");
}
