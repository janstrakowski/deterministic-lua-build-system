void buildpath(char buf[PATH_MAX], const char *first, ...) {
  va_list ap;
  const char *currentarg;

  checkpathlen(first);
  strcpy(buf, first);

  va_start(ap, first);
  while (1) {
    currentarg = va_arg(ap, char *);
    if (currentarg == NULL) break;
    errif(strlen(buf) + strlen(currentarg) >= PATH_MAX);
    strcat(buf, currentarg);
  }
  va_end(ap);
}
