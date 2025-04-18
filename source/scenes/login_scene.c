#include "scenes/login_scene.h"
#include <stdio.h>
#include "clay/clay.h"
#include "defines.h"
#include "scenes/scene.h"
#include "scenes/main_scene.h"
#include "components/button.h"
#include "components/textedit.h"
#include "components/popup.h"

#include <stdlib.h>
#include <sys/select.h>
#include <curl/curl.h>
#include <string.h>
#include <3ds.h>

#include <jansson.h>

#include "bluesky/bluesky.h"

static Scene login_scene;

bool logging_in = false;
bool login_successful = false;

static char username[80];
static char password[80];

TextEditData handleData = { 
    .hintText = CLAY_STRING("Enter your username handle"), 
    .textToEdit = username, 
    .isPassword = false,
    .maxLength = 80,
    .disable = false
};

TextEditData passwordData = { 
    .hintText = CLAY_STRING("Enter your password"), 
    .textToEdit = password, 
    .isPassword = true,
    .maxLength = 80,
    .disable = false
};

Thread threadHandle;
Handle threadRequest;
bool runThread = true;

void on_login_button_pressed() {
    if (strlen(username) == 0 || strlen(password) == 0) {
        show_popup_message("Error: Username or password is empty", NULL);
        return;
    }

    if (logging_in || is_popup_visible() || login_successful) {
        return;
    }

    logging_in = true;
    
    svcSignalEvent(threadRequest);
}

/*
size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp)
{
    if (buffer == NULL || size == 0 || nmemb == 0) {
        return 0;
    }
    
    char *local_buffer = malloc(size * nmemb + 1);
    if (local_buffer == NULL) {
        return 0;
    }
    
    memcpy(local_buffer, buffer, size * nmemb);
    local_buffer[size * nmemb] = '\0';
    
    json_error_t error;
    json_t *root = json_loads(local_buffer, 0, &error);
    
    if (root == NULL) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "JSON error: %s", error.text);
        
        show_popup_message(error_msg, NULL);
        logging_in = false;
        
        free(local_buffer);
        return size * nmemb;
    }
    
    json_t *error_obj = json_object_get(root, "error");
    if (error_obj != NULL && !json_is_null(error_obj)) {
        json_t *message_obj = json_object_get(root, "message");
        if (message_obj != NULL && json_is_string(message_obj)) {
            const char* error_message = json_string_value(message_obj);
            
            show_popup_message(error_message, NULL);
            logging_in = false;
        }
        
        json_decref(root);
        free(local_buffer);
        return size * nmemb;
    }
    
    login_successful = true;

    json_t *accessJwt = json_object_get(root, "accessJwt");
    json_t *refreshJwt = json_object_get(root, "refreshJwt");

    if (accessJwt != NULL && json_is_string(accessJwt) && refreshJwt != NULL && json_is_string(refreshJwt)) {
        const char* access_token = json_string_value(accessJwt);
        const char* refresh_token = json_string_value(refreshJwt);

        set_main_scene_tokens(access_token, refresh_token);
    }

    json_decref(root);
    free(local_buffer);
    return size * nmemb;
}
*/

void threadMain(void *arg) {
	while(runThread) {
		svcWaitSynchronization(threadRequest, U64_MAX);
		svcClearEvent(threadRequest);

        char error_msg[256];
        int code = bs_client_init(username, password, error_msg);
        if (code != 0) {
            show_popup_message(error_msg, NULL);
            logging_in = false;
        } else {
            login_successful = true;
        }


        /*
        size_t buffer_size = strlen("{ \"identifier\":\"") + strlen(username) + 
                           strlen("\", \"password\":\"") + strlen(password) + 
                           strlen("\" }") + 1;
        
        char* urlstuff = malloc(buffer_size);
        if (urlstuff == NULL) {
            show_popup_message("Error: Failed to allocate memory", NULL);
            return;
        }

        snprintf(urlstuff, buffer_size, "{ \"identifier\":\"%s\", \"password\":\"%s\" }", 
                username, password);

        CURL *hnd;
        struct curl_slist *headers;

        headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");

        hnd = curl_easy_init();
        if (hnd == NULL) {
            show_popup_message("Error: Failed to initialize CURL", NULL);
            free(urlstuff);
            return;
        }
        
        const char *url = "https://bsky.social/xrpc/com.atproto.server.createSession";
        
        curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(hnd, CURLOPT_URL, url);
        curl_easy_setopt(hnd, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(hnd, CURLOPT_POSTFIELDS, urlstuff);
        
        curl_easy_setopt(hnd, CURLOPT_TIMEOUT, 30L);
        curl_easy_setopt(hnd, CURLOPT_CONNECTTIMEOUT, 10L);
        curl_easy_setopt(hnd, CURLOPT_BUFFERSIZE, 32768L);

        CURLcode ret = curl_easy_perform(hnd);

        curl_easy_cleanup(hnd);
        hnd = NULL;
        curl_slist_free_all(headers);
        headers = NULL;
        free(urlstuff);

        if (ret != CURLE_OK) {
            size_t buffer_size = strlen("Error when trying to login: ") + strlen(curl_easy_strerror(ret)) + 1;
            
            char* errorBuffer = malloc(buffer_size);
            if (errorBuffer == NULL) {
                show_popup_message("Error: Failed to allocate memory", NULL);
                logging_in = false;
                return;
            }

            snprintf(errorBuffer, buffer_size, "Error when trying to login: %s", curl_easy_strerror(ret));
            show_popup_message(errorBuffer, NULL);
            logging_in = false;
                CLAY({
            .layout = {
                .sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)},
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                .childAlignment = {
                    .x = CLAY_ALIGN_X_CENTER,
                    .y = CLAY_ALIGN_Y_CENTER
                },
                .childGap = BOTTOM_HEIGHT / 6
            },
        }) {
            textedit_component(CLAY_STRING("handle"), &handleData);
            textedit_component(CLAY_STRING("password"), &passwordData);
            if (!logging_in) {
                button_component(CLAY_STRING("login"), CLAY_STRING("Login"), logging_in || is_popup_visible() || login_successful, on_login_button_pressed);
            } else {
                button_component(CLAY_STRING("login"), CLAY_STRING("Logging in..."), logging_in, NULL);
            }

            if (is_popup_visible()) {
                render_current_popup();
            }
        }
            free(errorBuffer);
        }
        */
    }
}

static void login_init(void) {
    svcCreateEvent(&threadRequest,0);
	threadHandle = threadCreate(threadMain, 0, (16 * 1024), 0x3f, -2, true);
}

static void login_layout(void) {
    CLAY({
        .layout = {
            .sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)},
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
            .padding = { .top = TOP_HEIGHT }
        },
    }) {
        CLAY({
            .layout = {
                .sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)},
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                .childAlignment = {
                    .x = CLAY_ALIGN_X_CENTER,
                    .y = CLAY_ALIGN_Y_CENTER
                },
                .childGap = BOTTOM_HEIGHT / 6
            },
        }) {
            textedit_component(CLAY_STRING("handle"), &handleData);
            textedit_component(CLAY_STRING("password"), &passwordData);
            if (!logging_in) {
                button_component(CLAY_STRING("login"), CLAY_STRING("Login"), logging_in || is_popup_visible() || login_successful, on_login_button_pressed);
            } else {
                button_component(CLAY_STRING("login"), CLAY_STRING("Logging in..."), logging_in, NULL);
            }
        }
    }

    render_current_popup(true);
}

static void login_update(void) {
    handleData.disable = logging_in || is_popup_visible() || login_successful;
    passwordData.disable = logging_in || is_popup_visible() || login_successful;

    if (login_successful) {
        login_successful = false;
        change_scene(get_main_scene());
        return;
    }

    check_popup_close_button();
}

static void login_unload(void) {
    if (login_successful) {
        runThread = false;
        svcSignalEvent(threadRequest);
        return;
    }

    runThread = false;

    svcSignalEvent(threadRequest);
    threadJoin(threadHandle, U64_MAX);

    svcCloseHandle(threadRequest);
}

Scene* get_login_scene(void) {
    login_scene.init = login_init;
    login_scene.update = login_update;
    login_scene.layout = login_layout;
    login_scene.unload = login_unload;
    
    return &login_scene;
}