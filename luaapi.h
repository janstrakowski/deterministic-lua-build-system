void *luanull;

int luaapi_init(lua_State *ls);

void *lnullunwrp(void *value);

const void *lnullwrp(void *value);

// int errnotostring(lua_State *ls);

// int pusherrno(lua_State *ls, int givenerrno);

# define RMR_KEEPCONTAINER 0x1

int rmr(const char *path, int flags);
