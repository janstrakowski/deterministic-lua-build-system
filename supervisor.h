typedef struct cli_args cli_args;

cli_args *cli;

void cli_defaults(cli_args *a, const char *thisexecutable);

int startassupervisor();

int newsupervisorfd(void);

int cache_get(const char *path, const char hash[32]);

int cache_put(const char *path, const char hash[32]);

int cache_query(const char hash[32]);

int openthisexe();

void cache_mkpath(const char hash[32], char path[PATH_MAX]);

