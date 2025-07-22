#include "c2d/base.h"
#include "string_utils.h"
#include "sys/unistd.h"

#include <citro2d.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "thirdparty/bluesky/bluesky.h"

#include "defines.h"
#include "scenes/main_scene.h"
#include "components/button.h"
#include "components/textedit.h"
#include "components/popup.h"

#include "sys/select.h"
#include "curl/curl.h"

static Scene login_scene;

bool downloading_cert_file = false;

bool logging_in = false;
bool login_successful = false;

static char username[80];
static char password[80];
static char obfuscated_password[80];

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
    .obfuscatedText = obfuscated_password,
    .isPassword = true,
    .maxLength = 80,
    .disable = false
};

bool disableLogin = false;

C2D_SpriteSheet logoSpriteSheet;
C2D_Image logoImage;

Thread loginThreadHnd;
Thread fileDownloadThreadHnd;

void on_download_certificate_file_confirm();

int progress_callback(void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
    if (dltotal > 0) {
        popup_set_progress_bar((float)dlnow / (float)dltotal);
    }
    return 0;
}

size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
    size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
    return written;
}

void download_cert_file_thread() {
    CURL* hnd = curl_easy_init();
    curl_easy_setopt(hnd, CURLOPT_URL, "https://curl.se/ca/cacert.pem");
    curl_easy_setopt(hnd, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(hnd, CURLOPT_XFERINFOFUNCTION, progress_callback);
    curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, write_data);
    // Ironically, you have to disable SSL certificate checking in order to download the SSL certificate lol
    curl_easy_setopt(hnd, CURLOPT_SSL_VERIFYPEER, 0L);

    bool errorOcurred = false;

    FILE* pagefile = fopen(CERT_FILE_PATH, "wb");
    if(pagefile) {
        curl_easy_setopt(hnd, CURLOPT_WRITEDATA, pagefile);
        CURLcode ret = curl_easy_perform(hnd);
        fclose(pagefile);

        if (ret != CURLE_OK) {
            char* errorBuffer = formatted_string("Error when downloading cert file: %s", curl_easy_strerror(ret));
            show_popup_message(errorBuffer, POPUP_TYPE_ERROR, on_download_certificate_file_confirm);
            free(errorBuffer);

            remove(CERT_FILE_PATH);
            errorOcurred = true;
        }
    } else {
        char* error_msg = formatted_string("Error when opening file to download: %s", strerror(errno));
        show_popup_message(error_msg, POPUP_TYPE_ERROR, on_download_certificate_file_confirm);
        free(error_msg);

        errorOcurred = true;
    }
    
    curl_easy_cleanup(hnd);
    if (!errorOcurred)
        close_popup(NULL);
    downloading_cert_file = false;
}

void on_download_certificate_file_confirm() {
    if (!downloading_cert_file) {
        show_popup_message("Downloading certificate file...", POPUP_TYPE_PROGRESS, NULL);
        fileDownloadThreadHnd = threadCreate(download_cert_file_thread, 0, (16 * 1024), 0x3f, -2, true);
        downloading_cert_file = true;
    }
}

void loginThread(void *arg) {
    char bs_error_msg[256];
    int code = bs_client_init(username, password, bs_error_msg);
    if (code != 0) {
        char* errorBuffer = formatted_string("Error when trying to log in: %s\n\nPlease make sure your handle and password are correct.", bs_error_msg);
        show_popup_message(errorBuffer, POPUP_TYPE_MESSAGE, NULL);
        free(errorBuffer);
    } else {
        login_successful = true;
    }
    logging_in = false;
}

void on_login_button_pressed() {
    if (strlen(username) == 0 || strlen(password) == 0) {
        show_popup_message("Username or password cannot be empty.", POPUP_TYPE_MESSAGE, NULL);
        return;
    }
    
    if (disableLogin) {
        return;
    }

    logging_in = true;
    
    loginThreadHnd = threadCreate(loginThread, 0, (16 * 1024), 0x3f, -2, true);
}

static void login_init(void) {
    logoSpriteSheet = C2D_SpriteSheetLoad("romfs:/3dsky-icon-alt.t3x");
    if (logoSpriteSheet) {
        logoImage = C2D_SpriteSheetGetImage(logoSpriteSheet, 0);
    }
    
    if (access(CERT_FILE_PATH, F_OK) != 0) {
        show_popup_message("It seems the certificate file (cacert.pem) needed for this app was not found. Would you like to download it?", POPUP_TYPE_CONFIRM, on_download_certificate_file_confirm);
    }
    #if defined(LOGIN_HANDLE) && defined(LOGIN_PASSWORD)
    else {
        strcpy(username, LOGIN_HANDLE);
        strcpy(password, LOGIN_PASSWORD);
        on_login_button_pressed();
    }
    #endif
}

static void login_layout(float deltaTime) {
    CLAY({
        .layout = {
            .sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)},
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
            .childAlignment = {.x = CLAY_ALIGN_X_CENTER}
        },
    }) {
        CLAY({
            .layout = {
                .sizing = {CLAY_SIZING_FIXED(TOP_WIDTH), CLAY_SIZING_FIXED(TOP_HEIGHT)},
                .childAlignment = {.x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER},
            },
        }) {
            CLAY({
                .layout = { .sizing = { .width = CLAY_SIZING_FIXED(256), .height = CLAY_SIZING_FIXED(128) }},
                .image = {
                    .imageData = &logoImage,
                 },
                 .aspectRatio = { 256.0f / 128.0f }
            });
        }
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
            textedit_component(&handleData);
            textedit_component(&passwordData);
            if (!logging_in) {
                button_component(CLAY_STRING("Login"), disableLogin, on_login_button_pressed);
            } else {
                button_component(CLAY_STRING("Logging in..."), logging_in, NULL);
            }
        }
    }

    popup_layout(true);
}

static void login_update(float deltaTime) {
    disableLogin = downloading_cert_file || logging_in || is_popup_visible() || login_successful;

    handleData.disable = disableLogin;
    passwordData.disable = disableLogin;

    if (login_successful) {
        change_scene(get_main_scene());
    }
}

static void login_unload(void) {
    if (fileDownloadThreadHnd && downloading_cert_file) {
        threadJoin(fileDownloadThreadHnd, U64_MAX);
    }

    if (loginThreadHnd && logging_in) {
        threadJoin(loginThreadHnd, U64_MAX);
    }

    if (logoSpriteSheet) {
        C2D_SpriteSheetFree(logoSpriteSheet);
    }
}

Scene* get_login_scene(void) {
    login_scene.init = login_init;
    login_scene.update = login_update;
    login_scene.layout = login_layout;
    login_scene.unload = login_unload;
    
    return &login_scene;
}
