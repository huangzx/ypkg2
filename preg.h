#ifndef PREG_H
#define PREG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pcre.h>

#define OVECCOUNT 30    /* should be a multiple of 3 */
    
typedef struct _PREGINFO {
    pcre *re;
    char *subject;
    int offset;
    int ovector[OVECCOUNT];
    int stringcount;
} PREGINFO;

int preg_match(PREGINFO *piptr, char *pattern, char *subject, int options, int first);

int preg_result(PREGINFO *piptr, int number, char *buf, int buf_size);

int preg_result2(PREGINFO *piptr, char *name, char *buf, int buf_size);

int preg_free(PREGINFO *piptr);

char *preg_replace(char *pattern, char *replace, char *subject, int options);

#endif /* !PREG_H */
