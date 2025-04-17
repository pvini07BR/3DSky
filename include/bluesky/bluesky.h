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

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __BLUESKY_H
#define __BLUESKY_H

#include <stdlib.h>

#define BS_CLIENT_INIT_ERR_MEM  1
#define BS_CLIENT_INIT_ERR_AUTH 2
#define BS_CLIENT_INIT_ERR_JSON 3

/**
 * Default response structure returned for each call to the API. Contains the
 * API response, the response code, response size, any error code, and message
 * if applicable.
 */
typedef struct {
    char *resp;
    char *err_msg;
    size_t size;
    long resp_code;
    int err_code;
} bs_client_response_t;

/**
 * Contains the errors returned for pagination validation.
 */
enum bs_client_pagination_errs {
    BS_CLIENT_PAGINATION_ERR_OVER_LIMIT = 3
};

/**
 * Structure used to pass pagination settings. The limit field will be ignored
 * if the value is less than 50. The max value supported by the API is 100.
 */
typedef struct {
    unsigned int limit;
    char *cursor;
} bs_client_pagination_opts;

/**
 * Initialize the library.
 */
int
bs_client_init(const char *handle, const char *app_password, char *error);

/**
 * Free the memory used in the client response.
 */
void
bs_client_response_free(bs_client_response_t *res);

/** 
 * Creates a new post to the authenticated user's feed. The response
 * memory needs to be freed by the caller.
 * 
 * data argument must be JSON in the following format:
 * 
 * {"$type": "app.bsky.feed.post",
 *  "text": "Hello World!",
 *  "createdAt": "2023-08-07T05:31:12.156888Z"}
 */
bs_client_response_t*
bs_client_post(const char *msg);

/**
 * Retrieve the profile information for the given handle.
 */
bs_client_response_t*
bs_client_profile_get(const char *handle);

/**
 * Retrieve the DID for the given handle.
 */
bs_client_response_t*
bs_client_resolve_handle(const char *handle);

/**
 * Retrieve the follows for the given handle.
 */
bs_client_response_t*
bs_client_follows_get(const char *handle,
                      const bs_client_pagination_opts *opts);

/**
 * Retrieve the followers for the given handle.
 */
bs_client_response_t*
bs_client_followers_get(const char *handle,
                        const bs_client_pagination_opts *opts);

/**
 * Retrieve the profile preferences for the authenticated user.
 */
bs_client_response_t*
bs_client_profile_preferences();

/**
 * Get the authenticated user's timeline. The JSON response has a field called
 * "cursor". If this field is populated, there are more results to retrieve. 
 * The response memory needs to be freed by the caller.
 */
bs_client_response_t*
bs_client_timeline_get(const bs_client_pagination_opts *opts);

/**
 * Retrieve posts from the given user DID.
 */
bs_client_response_t*
bs_client_author_feed_get(const char *did,
                          const bs_client_pagination_opts *opts);

/**
 * Retrieve likes from the given user handle.
 */
bs_client_response_t*
bs_client_handle_likes_get(const char *handle,
                           const bs_client_pagination_opts *opts);

/**
 * Retrieve likes for the given user at-uri. The at-uri needs to be in the
 * following format: at://<did>
 */
bs_client_response_t*
bs_client_likes_get(const char *handle, const bs_client_pagination_opts *opts);

/**
 * Free the memory used by the client.
 */
void
bs_client_free();

#endif /* __BLUESKY_H */
#ifdef __cplusplus
}
#endif
