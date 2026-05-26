char fd_luareader_buf[2048];

const char * fd_luareader(lua_State *L, void *data, size_t *size) {
  ssize_t bytesread = read((size_t) data, fd_luareader_buf, 2048);
  errif(bytesread == -1);
  *size = bytesread;
  return fd_luareader_buf;
}

void lualoadfd(int fd, char *chunkname, lua_State *L) {
  int status = lua_load(L, &fd_luareader, (void *) (size_t) fd, chunkname, NULL);
  if (status != LUA_OK) {
    fprintf(stderr, "%s\n", lua_tostring(L, -1));
    exit(-1);
  }
}

int fd_luawriter(lua_State *L, const void* p, size_t sz, void *ud) {
  const void *pos = p;
  ssize_t bytes_written;

  if (sz == 0) return 0;

  while (1) {
    bytes_written = write((size_t) ud, pos, sz - (size_t) (pos - p));
    errif(bytes_written == -1);
    pos += bytes_written;
    if (sz - (size_t) (pos - p) == 0) break;
  }
  return 0;
}

void luadumpfd(int fd, lua_State *L) {
  lua_dump(L, fd_luawriter, (void *) (size_t) fd, 0);
}
