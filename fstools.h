#define CREATTOOL_FILE 0x1
#define CREATTOOL_DIR 0x2
#define CREATTOOL_UPDIRS 0x4
#define CREATTOOL_UPDIRMODE 0x8
#define CREATTOOL_EXCL 0x10

void creattool(int dir, const char *path, uint64_t flags, mode_t mode, ...);

void copytool(int srcdir, const char *src, int destdir, const char *dest, int flags);
