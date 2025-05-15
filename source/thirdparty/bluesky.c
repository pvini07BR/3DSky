/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2024 Brian J. Downs
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#define _DEFAULT_SOURCE
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>
#include <jansson.h>

#include "thirdparty/bluesky/bluesky.h"

#define BS_RES_JSON_HEADER "Content-Type: application/json"
#define BS_REQ_JSON_HEADER "Accept: application/json"

#define TOKEN_HEADER_SIZE 1024
#define DID_MAX_SIZE      2048
#define DEFAULT_URL_SIZE  2048

#define API_BASE                     "https://bsky.social/xrpc"
#define AUTH_URL API_BASE            "/com.atproto.server.createSession"
#define RESOLVE_HANDLE_URL API_BASE  "/com.atproto.identity.resolveHandle"
#define POST_FEED_URL API_BASE       "/com.atproto.repo.createRecord"
#define GET_PROFILE_URL API_BASE     "/app.bsky.actor.getProfile"
#define GET_TIMELINE_URL API_BASE    "/app.bsky.feed.getTimeline"
#define PROFILE_PREF_URL API_BASE    "/app.bsky.actor.getPreferences"
#define GET_FOLLOWS_URL API_BASE     "/app.bsky.graph.getFollows"
#define GET_FOLLOWERS_URL API_BASE   "/app.bsky.graph.getFollowers"
#define GET_AUTHOR_FEED API_BASE     "/app.bsky.feed.getAuthorFeed"
#define GET_ACTOR_LIKES_URL API_BASE "/app.bsky.feed.getActorLikes"
#define GET_LIKES_URL API_BASE       "/app.bsky.feed.getLikes"

#define SET_BASIC_CURL_CONFIG \
    curl_easy_setopt(curl, CURLOPT_URL, url); \
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); \
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk); \
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, cb); \
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)response); 

#define CURL_CALL_ERROR_CHECK \
    if (res != CURLE_OK) { \
        char *err_msg = (char *)curl_easy_strerror(res); \
        response->err_msg = calloc(strlen(err_msg)+1, sizeof(char)); \
        strcpy(response->err_msg, err_msg); \
        CALL_CLEANUP; \
        return response; \
    }

#define CALL_CLEANUP \
    curl_slist_free_all(chunk); \
    free(url);

static CURL *curl = NULL;
static char did[256];
static char token[400];
static char cur_handle[256];
static char refresh_token[400];
static char token_header[TOKEN_HEADER_SIZE];

/**
 *
 */
static int
bs_client_pagination_opts_validate(const bs_client_pagination_opts *opts)
{
    if (opts->limit > 100) {
        return BS_CLIENT_PAGINATION_ERR_OVER_LIMIT;
    }
    
    return 0;
}

/**
 * Write a received response into a response type.
 */
static size_t
cb(char *data, size_t size, size_t nmemb, void *clientp)
{
    size_t real_size = size * nmemb;
    bs_client_response_t *mem = (bs_client_response_t*)clientp;

    char *ptr = realloc(mem->resp, mem->size + real_size+1);

    mem->resp = ptr;
    memcpy(&(mem->resp[mem->size]), data, real_size);
    mem->size += real_size;
    mem->resp[mem->size] = 0;

    return real_size;
}

/**
 * Create and return a new pointer for response data.
 */
static bs_client_response_t*
bs_client_response_new()
{
    bs_client_response_t *resp = calloc(1, sizeof(bs_client_response_t));
    return resp;
}

void
bs_client_response_free(bs_client_response_t *res)
{
    if (res != NULL) {
        if (res->resp != NULL) free(res->resp);
        if (res->err_msg != NULL) free(res->err_msg);

        free(res);
        res = NULL;
    }
}

bs_client_response_t*
bs_client_authenticate(const char *handle, const char *app_password)
{
    bs_client_response_t *response = bs_client_response_new();
    if (!response) {
        return NULL;
    }
    response->resp = NULL;
    response->err_msg = NULL;
    response->size = 0;
    response->resp_code = 0;

    struct curl_slist *chunk = NULL;

    chunk = curl_slist_append(chunk, BS_REQ_JSON_HEADER);
    chunk = curl_slist_append(chunk, BS_RES_JSON_HEADER);
    chunk = curl_slist_append(chunk, token_header);

    char *url = calloc(DEFAULT_URL_SIZE, sizeof(char));
    if (!url) {
        bs_client_response_free(response);
        return NULL;
    }
    strcpy(url, AUTH_URL);

    json_t *root = json_object();
    json_t *enc_handle = json_string(handle);
    json_t *enc_app_password = json_string(app_password);
    if (!root || !enc_handle || !enc_app_password) {
        if (root) json_decref(root);
        if (enc_handle) json_decref(enc_handle);
        if (enc_app_password) json_decref(enc_app_password);
        free(url);
        bs_client_response_free(response);
        return NULL;
    }

    json_object_set(root, "identifier", enc_handle);
    json_object_set(root, "password", enc_app_password);
    char *data = json_dumps(root, 0);
    if (!data) {
        json_decref(enc_handle);
        json_decref(enc_app_password);
        json_decref(root);
        free(url);
        bs_client_response_free(response);
        return NULL;
    }

    SET_BASIC_CURL_CONFIG;
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response->resp_code);
    
    if (res != CURLE_OK) {
        const char *err_msg = curl_easy_strerror(res);
        response->err_msg = calloc(strlen(err_msg) + 1, sizeof(char));
        if (response->err_msg) {
            strcpy(response->err_msg, err_msg);
        }
    } else if (response->resp_code != 200) {
        const char *err_msg = "Authentication failed";
        response->err_msg = calloc(strlen(err_msg) + 1, sizeof(char));
        if (response->err_msg) {
            strcpy(response->err_msg, err_msg);
        }
    }

    json_decref(enc_handle);
    json_decref(enc_app_password);
    json_decref(root);
    free(data);
    free(url);
    curl_slist_free_all(chunk);

    return response;
}

bs_client_response_t*
bs_client_resolve_handle(const char *handle)
{
    bs_client_response_t *response = bs_client_response_new();
    struct curl_slist *chunk = NULL;

    chunk = curl_slist_append(chunk, BS_REQ_JSON_HEADER);
    chunk = curl_slist_append(chunk, token_header);

    char *url = calloc(DEFAULT_URL_SIZE, sizeof(char));
    strcpy(url, RESOLVE_HANDLE_URL);
    strcat(url, "?handle=");
    strcat(url, handle);

    SET_BASIC_CURL_CONFIG;
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response->resp_code);
    CURL_CALL_ERROR_CHECK;

    CALL_CLEANUP;
    
    return response;  
}

int
bs_client_init(const char *handle, const char *app_password, char *error)
{
    if (!handle || !app_password) {
        if (error) {
            strcpy(error, "Handle and password cannot be null");
        }
        return BS_CLIENT_INIT_ERR_AUTH;
    }

    strcpy(cur_handle, handle);

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (!curl) {
        if (error) {
            strcpy(error, "Failed to initialize CURL");
        }
        return BS_CLIENT_INIT_ERR_MEM;
    }

    bs_client_response_t *res = bs_client_authenticate(handle, app_password);
    if (!res) {
        if (error) {
            strcpy(error, "Failed to allocate memory for authentication response");
        }
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        return BS_CLIENT_INIT_ERR_MEM;
    }

    if (res->err_msg != NULL) {
        if (error) {
            strncpy(error, res->err_msg, strlen(res->err_msg));
            error[strlen(res->err_msg)] = '\0';
        }
        bs_client_response_free(res);
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        return BS_CLIENT_INIT_ERR_AUTH;
    }

    if (!res->resp) {
        if (error) {
            strcpy(error, "No response received from server");
        }
        bs_client_response_free(res);
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        return BS_CLIENT_INIT_ERR_AUTH;
    }

    json_t *root;
    json_error_t j_error;

    root = json_loads(res->resp, 0, &j_error);
    if (root == NULL) {
        if (error) {
            strncpy(error, j_error.text, strlen(j_error.text));
            error[strlen(j_error.text)] = '\0';
        }
        bs_client_response_free(res);
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        return BS_CLIENT_INIT_ERR_JSON;
    }

    const char *t = NULL;
    const char *rt = NULL;
    if (json_unpack(root, "{s:s, s:s}", "accessJwt", &t, "refreshJwt", &rt) == -1) {
        if (error) {
            strcpy(error, "Failed to parse authentication tokens");
        }
        json_decref(root);
        bs_client_response_free(res);
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        return BS_CLIENT_INIT_ERR_JSON;
    }

    if (t && rt) {
        strcpy(token, t);
        strcpy(refresh_token, rt);
    } else {
        if (error) {
            strcpy(error, "Invalid authentication tokens received");
        }
        json_decref(root);
        bs_client_response_free(res);
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        return BS_CLIENT_INIT_ERR_AUTH;
    }

    json_decref(root);
    bs_client_response_free(res);

    strcpy(token_header, "Authorization: Bearer ");
    strcat(token_header, token);

    res = bs_client_resolve_handle(cur_handle);
    if (!res || res->err_msg) {
        if (error) {
            strcpy(error, res ? (res->err_msg ? res->err_msg : "Failed to resolve handle") : "Failed to allocate memory");
        }
        if (res) bs_client_response_free(res);
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        return BS_CLIENT_INIT_ERR_AUTH;
    }

    root = json_loads(res->resp, 0, &j_error);
    if (root == NULL) {
        if (error) {
            strncpy(error, j_error.text, strlen(j_error.text));
            error[strlen(j_error.text)] = '\0';
        }
        bs_client_response_free(res);
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        return BS_CLIENT_INIT_ERR_JSON;
    }

    const char *d = NULL;
    if (json_unpack(root, "{s:s}", "did", &d) == -1 || !d) {
        if (error) {
            strcpy(error, "Failed to parse DID from response");
        }
        json_decref(root);
        bs_client_response_free(res);
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        return BS_CLIENT_INIT_ERR_JSON;
    }

    strcpy(did, d);

    json_decref(root);
    bs_client_response_free(res);

    return 0;
}

bs_client_response_t*
bs_client_post(const char *msg)
{
    bs_client_response_t *response = bs_client_response_new();
    struct curl_slist *chunk = NULL;

    chunk = curl_slist_append(chunk, BS_REQ_JSON_HEADER);
    chunk = curl_slist_append(chunk, BS_RES_JSON_HEADER);
    chunk = curl_slist_append(chunk, token_header);

    char *url = calloc(DEFAULT_URL_SIZE, sizeof(char));
    strcpy(url, POST_FEED_URL);

    json_error_t error;
    json_t *root = json_object();
    json_t *repo = json_string(did);
    json_t *type = json_string("app.bsky.feed.post");
    json_t *collection = json_string("app.bsky.feed.post");
    json_t *record = json_loads(msg, 0, &error);
    if (root == NULL) {
        size_t err_len = strlen(error.text);
        response->err_msg = calloc(err_len+1, sizeof(char));
        strcpy(response->err_msg, error.text);
        return response;
    }

    json_object_set(root, "repo", repo);
    json_object_set(root, "type", type);
    json_object_set(root, "collection", collection);
    json_object_set(root, "record", record);
    char *data = json_dumps(root, 0);

    SET_BASIC_CURL_CONFIG;
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response->resp_code);
    CURL_CALL_ERROR_CHECK;

    json_decref(repo);
    json_decref(collection);
    json_decref(record);
    json_decref(root);
    free(data);

    CALL_CLEANUP;
    
    return response;     
}

bs_client_response_t*
bs_client_profile_get(const char *handle)
{
    bs_client_response_t *response = bs_client_response_new();
    struct curl_slist *chunk = NULL;

    chunk = curl_slist_append(chunk, BS_REQ_JSON_HEADER);
    chunk = curl_slist_append(chunk, token_header);

    char *escaped_handle = curl_easy_escape(curl, handle, strlen(handle));

    char *url = calloc(DEFAULT_URL_SIZE, sizeof(char));
    strcpy(url, GET_PROFILE_URL);
    strcat(url, "?actor=");
    strcat(url, handle);

    SET_BASIC_CURL_CONFIG;
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response->resp_code);
    if (res != CURLE_OK) {
        char *err_msg = (char *)curl_easy_strerror(res);
        response->err_msg = calloc(strlen(err_msg)+1, sizeof(char));
        strcpy(response->err_msg, err_msg);
        curl_free(escaped_handle);
        CALL_CLEANUP;
        return response;
    }

    curl_free(escaped_handle);
    CALL_CLEANUP;
    
    return response;   
}

bs_client_response_t*
bs_client_follows_get(const char *handle,
                      const bs_client_pagination_opts *opts)
{
    bs_client_response_t *response = bs_client_response_new();
    struct curl_slist *chunk = NULL;

    chunk = curl_slist_append(chunk, BS_REQ_JSON_HEADER);
    chunk = curl_slist_append(chunk, token_header);

    char *url = calloc(DEFAULT_URL_SIZE, sizeof(char));
    strcpy(url, GET_FOLLOWS_URL);
    strcat(url, "?actor=");
    strcat(url, handle);

    if (opts != NULL) {
        if (opts->limit != 0 && opts->limit >= 50) {
            if (opts->limit > 100) {
                char *err_msg = "limit max value is 100";
                response->err_msg = calloc(strlen(err_msg)+1, sizeof(char));
                strcpy(response->err_msg, err_msg);
                CALL_CLEANUP;
                return response;
            }

            strcat(url, "?limit=");
            char lim_val[11] = {0};
            sprintf(lim_val, "%d", opts->limit);
            strcat(url, lim_val);
        } else {
            strcat(url, "?limit=100");
        }

        if (opts->cursor != NULL) {
            strcat(url, "&cursor=");
            strcat(url, opts->cursor);
        }
    }

    SET_BASIC_CURL_CONFIG;
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response->resp_code);
    CURL_CALL_ERROR_CHECK;

    CALL_CLEANUP;
    
    return response; 
}

/**
 * Retrieve the followers for the given handle.
 */
bs_client_response_t*
bs_client_followers_get(const char *handle,
                        const bs_client_pagination_opts *opts)
{
    bs_client_response_t *response = bs_client_response_new();
    struct curl_slist *chunk = NULL;

    chunk = curl_slist_append(chunk, BS_REQ_JSON_HEADER);
    chunk = curl_slist_append(chunk, token_header);

    char *url = calloc(DEFAULT_URL_SIZE, sizeof(char));
    strcpy(url, GET_FOLLOWERS_URL);
    strcat(url, "?actor=");
    strcat(url, handle);

    if (opts != NULL) {
        if (opts->limit != 0 && opts->limit >= 50) {
            if (opts->limit > 100) {
                char *err_msg = "limit max value is 100";
                response->err_msg = calloc(strlen(err_msg)+1, sizeof(char));
                strcpy(response->err_msg, err_msg);
                CALL_CLEANUP;
                return response;
            }

            strcat(url, "&limit=");
            char lim_val[11] = {0};
            sprintf(lim_val, "%d", opts->limit);
            strcat(url, lim_val);
        } else {
            strcat(url, "&limit=100");
        }

        if (opts->cursor != NULL) {
            strcat(url, "&cursor=");
            strcat(url, opts->cursor);
        }
    }

    SET_BASIC_CURL_CONFIG;
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response->resp_code);
    CURL_CALL_ERROR_CHECK;

    CALL_CLEANUP;
    
    return response; 
}

bs_client_response_t*
bs_client_profile_preferences()
{
    bs_client_response_t *response = bs_client_response_new();
    struct curl_slist *chunk = NULL;

    chunk = curl_slist_append(chunk, BS_REQ_JSON_HEADER);
    chunk = curl_slist_append(chunk, token_header);

    char *url = calloc(DEFAULT_URL_SIZE, sizeof(char));
    strcpy(url, PROFILE_PREF_URL);

    SET_BASIC_CURL_CONFIG;
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response->resp_code);
    CURL_CALL_ERROR_CHECK;

    CALL_CLEANUP;
    
    return response;    
}

bs_client_response_t*
bs_client_timeline_get(const bs_client_pagination_opts *opts)
{
    bs_client_response_t *response = bs_client_response_new();
    struct curl_slist *chunk = NULL;

    chunk = curl_slist_append(chunk, BS_REQ_JSON_HEADER);
    chunk = curl_slist_append(chunk, token_header);

    char *url = calloc(DEFAULT_URL_SIZE, sizeof(char));
    strcpy(url, GET_TIMELINE_URL);

    if (opts != NULL) {
        if (opts->limit != 0 && opts->limit >= 50) {
            if (opts->limit > 100) {
                char *err_msg = "limit max value is 100";
                response->err_msg = calloc(strlen(err_msg)+1, sizeof(char));
                strcpy(response->err_msg, err_msg);
                CALL_CLEANUP;
                return response;
            }

            strcat(url, "?limit=");
            char lim_val[11] = {0};
            sprintf(lim_val, "%d", opts->limit);
            strcat(url, lim_val);
        } else {
            strcat(url, "?limit=100");
        }

        if (opts->cursor != NULL) {
            strcat(url, "&cursor=");
            strcat(url, opts->cursor);
        }
    }

    SET_BASIC_CURL_CONFIG;
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response->resp_code);
    CURL_CALL_ERROR_CHECK;

    CALL_CLEANUP;
    
    return response;
}

bs_client_response_t*
bs_client_author_feed_get(const char *did,
                          const bs_client_pagination_opts *opts)
{
    bs_client_response_t *response = bs_client_response_new();
    struct curl_slist *chunk = NULL;

    chunk = curl_slist_append(chunk, BS_REQ_JSON_HEADER);
    chunk = curl_slist_append(chunk, token_header);

    char *url = calloc(DEFAULT_URL_SIZE, sizeof(char));
    strcpy(url, GET_AUTHOR_FEED);
    strcat(url, "?actor=");
    strcat(url, did);

    if (opts != NULL) {
        if (opts->limit != 0 && opts->limit >= 50) {
            if (opts->limit > 100) {
                char *err_msg = "limit max value is 100";
                response->err_msg = calloc(strlen(err_msg)+1, sizeof(char));
                strcpy(response->err_msg, err_msg);
                CALL_CLEANUP;
                return response;
            }

            strcat(url, "&limit=");
            char lim_val[11] = {0};
            sprintf(lim_val, "%d", opts->limit);
            strcat(url, lim_val);
        } else {
            strcat(url, "&limit=100");
        }

        if (opts->cursor != NULL) {
            strcat(url, "&cursor=");
            strcat(url, opts->cursor);
        }
    }

    SET_BASIC_CURL_CONFIG;
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response->resp_code);
    CURL_CALL_ERROR_CHECK;

    CALL_CLEANUP;
    
    return response;
}

bs_client_response_t*
bs_client_handle_likes_get(const char *handle,
                           const bs_client_pagination_opts *opts)
{
    bs_client_response_t *response = bs_client_response_new();
    struct curl_slist *chunk = NULL;

    chunk = curl_slist_append(chunk, BS_REQ_JSON_HEADER);
    chunk = curl_slist_append(chunk, token_header);

    char *url = calloc(DEFAULT_URL_SIZE, sizeof(char));
    strcpy(url, GET_ACTOR_LIKES_URL);
    strcat(url, "?actor=");
    strcat(url, handle);

    if (opts != NULL) {
        if (opts->limit != 0 && opts->limit >= 50) {
            if (opts->limit > 100) {
                char *err_msg = "limit max value is 100";
                response->err_msg = calloc(strlen(err_msg)+1, sizeof(char));
                strcpy(response->err_msg, err_msg);
                CALL_CLEANUP;
                return response;
            }

            strcat(url, "&limit=");
            char lim_val[11] = {0};
            sprintf(lim_val, "%d", opts->limit);
            strcat(url, lim_val);
        } else {
            strcat(url, "&limit=100");
        }

        if (opts->cursor != NULL) {
            strcat(url, "&cursor=");
            strcat(url, opts->cursor);
        }
    }

    SET_BASIC_CURL_CONFIG;
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response->resp_code);
    CURL_CALL_ERROR_CHECK;

    CALL_CLEANUP;
    
    return response;
}

bs_client_response_t*
bs_client_likes_get(const char *handle, const bs_client_pagination_opts *opts)
{
    bs_client_response_t *response = bs_client_response_new();
    struct curl_slist *chunk = NULL;

    chunk = curl_slist_append(chunk, BS_REQ_JSON_HEADER);
    chunk = curl_slist_append(chunk, token_header);

    char *url = calloc(DEFAULT_URL_SIZE, sizeof(char));
    strcpy(url, GET_LIKES_URL);
    strcat(url, "?uri=");
    strcat(url, handle);

    if (opts != NULL) {
        if (opts->limit != 0 && opts->limit >= 50) {
            if (opts->limit > 100) {
                char *err_msg = "limit max value is 100";
                response->err_msg = calloc(strlen(err_msg)+1, sizeof(char));
                strcpy(response->err_msg, err_msg);
                CALL_CLEANUP;
                return response;
            }

            strcat(url, "&limit=");
            char lim_val[11] = {0};
            sprintf(lim_val, "%d", opts->limit);
            strcat(url, lim_val);
        } else {
            strcat(url, "&limit=100");
        }

        if (opts->cursor != NULL) {
            strcat(url, "&cursor=");
            strcat(url, opts->cursor);
        }
    }

    SET_BASIC_CURL_CONFIG;
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response->resp_code);
    CURL_CALL_ERROR_CHECK;

    CALL_CLEANUP;
    
    return response;
}

void
bs_client_free()
{
    curl_easy_cleanup(curl);
    curl_global_cleanup();
}

const char*
bs_client_get_current_handle()
{
    return cur_handle;
}
