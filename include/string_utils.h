#ifndef STRING_UTILS_H
#define STRING_UTILS_H

char* strdup(const char* str);
char* formatted_string(const char* fmt, ...);
char* replace_substring(const char* original, const char* target, const char* replacement);
char* extract_filename(const char* url);

#endif
