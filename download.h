#ifndef DOWNLOAD_H
#define DOWNLOAD_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

typedef struct _DownloadContent {
    char *text;
    size_t size;
} DownloadContent;


typedef struct _DownloadFile {
    char *file;
    FILE *stream;
} DownloadFile;

int get_content(char *url, DownloadContent *content);
int download_file(char *url, DownloadFile *file);

size_t memory_callback(void *data, size_t size, size_t nmemb, void *user);
size_t file_callback(void *data, size_t size, size_t nmemb, void *user);

#endif /* !DOWNLOAD_H */
