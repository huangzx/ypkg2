#ifndef DOWNLOAD_H
#define DOWNLOAD_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

typedef struct _TARGET_CONTENT {
    char *text;
    size_t size;
} TARGET_CONTENT;


typedef struct _TARGET_FILE {
    char *file;
    FILE *stream;
} TARGET_FILE;

int get_content(char *url, TARGET_CONTENT *content);
int download_file(char *url, TARGET_FILE *file);

size_t memory_callback(void *data, size_t size, size_t nmemb, void *user);
size_t file_callback(void *data, size_t size, size_t nmemb, void *user);

#endif /* !DOWNLOAD_H */
