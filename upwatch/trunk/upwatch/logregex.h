#ifndef __LOGREGEX_H
#define __LOGREGEX_H

int logregex_matchline(char *type, char *buffer, int *color);
int logregex_rmatchline(char *type, char *line);
int logregex_refresh(char *path);

#endif /* __LOGREGEX_H */

