char *
repeat_str(const char *str, size_t n);

char *
join_path(const char *parts, ...);

bool
check_file_exist(const char *path);

int
mkdirp(const char *path);

int
mkdirp_for_file(const char *file_path);
