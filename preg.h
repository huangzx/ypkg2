#ifndef PREG_H
#define PREG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pcre.h>

#define OVECCOUNT 30    /* should be a multiple of 3 */
    
typedef struct {
    pcre *re;
    char *subject;
    int offset;
    int ovector[OVECCOUNT];
    int stringcount;
} PREGInfo;

int preg_match(PREGInfo *piptr, char *pattern, char *subject, int options, int first);

int preg_result(PREGInfo *piptr, int number, char *buf, int buf_size);

int preg_result2(PREGInfo *piptr, char *name, char *buf, int buf_size);

int preg_free(PREGInfo *piptr);

char *preg_replace(char *pattern, char *replace, char *subject, int options);

#endif /* !PREG_H */
