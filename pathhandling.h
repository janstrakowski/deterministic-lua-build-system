#define checkpathlen(x) assert(strlen(x) < PATH_MAX);

void buildpath(char *buf, const char *first, ...);
