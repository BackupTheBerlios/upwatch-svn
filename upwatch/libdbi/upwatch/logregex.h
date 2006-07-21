#ifndef __LOGREGEX_H
#define __LOGREGEX_H

#include <pcreposix.h>

int logregex_matchline(char *type, char *buffer, int *color);
int logregex_rmatchline(char *type, char *line);
// int logregex_refresh(char *path);
void logregex_refresh_type(char *path, char *type);
void logregex_print_stats(char *type);
void logregex_expand_macros(char *type, char *in, char *out);

#endif /* __LOGREGEX_H */

