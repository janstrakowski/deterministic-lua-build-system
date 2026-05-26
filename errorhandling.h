#define STR_HELPER(x) #x
#define TOSTRING(x) STR_HELPER(x)
#define AT __FILE__ ":" TOSTRING(__LINE__)
#define errif(x) if(x) { \
  perror(AT); \
  exit(errno); \
}
