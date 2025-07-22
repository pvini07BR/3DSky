#include "string_utils.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Implementing strdup because C99 doesn't have it lol
char* strdup(const char* str) {
    if (str == NULL) {
        return NULL;
    }
    size_t len = strlen(str) + 1;
    char* copy = (char*)malloc(len);
    if (copy) {
        memcpy(copy, str, len);
    }
    return copy;
}

char* replace_substring(const char* original, const char* target, const char* replacement) {
    if (!original || !target || !replacement) {
        return NULL;
    }

    const char* pos = strstr(original, target);
    if (!pos) {
        char* result = strdup(original);
        return result;
    }

    size_t prefixLength = pos - original;
    size_t targetLength = strlen(target);
    size_t replacementLength = strlen(replacement);
    size_t originalLength = strlen(original);

    size_t newLength = originalLength - targetLength + replacementLength;
    char* result = (char*)malloc(newLength + 1);
    if (!result) {
        perror("Error allocating memory");
        return NULL;
    }

    strncpy(result, original, prefixLength);
    strcpy(result + prefixLength, replacement);
    strcpy(result + prefixLength + replacementLength, pos + targetLength);

    return result;
}

char* extract_filename(const char* url) {
    if (url == NULL) return NULL;

    const char* lastSlash = strrchr(url, '/');
    if (!lastSlash) return NULL;

    lastSlash++;

    const char* suffix = strstr(lastSlash, "@jpeg");
    if (!suffix) return NULL;

    size_t filenameLength = suffix - lastSlash;

    char* filename = (char*)malloc(filenameLength + 1);
    if (!filename) {
        perror("Error allocating memory");
        return NULL;
    }

    strncpy(filename, lastSlash, filenameLength);
    filename[filenameLength] = '\0';

    return filename;
}

