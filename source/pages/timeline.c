#include <3ds.h>
#include "components/feed.h"
#include "components/post_view.h"
#include "defines.h"
#include "theming.h"
#include "thirdparty/bluesky/bluesky.h"
#include "pages/timeline.h"

/*
void post_open_callback(void* data, Post* post) {
    if (data == NULL || post == NULL) return;
    TimelinePage* dat = (TimelinePage*)data;
    if (dat == NULL) return;

    if (post != NULL) {
        dat->feed.setScroll = true;
        dat->postView.post = post;
        dat->postView.opened = true;
    } 
}
*/

void timeline_init(TimelinePage* data) {
    if (data == NULL) return;

    data->initialized = false,
    post_view_init(&data->postView);
    feed_init(&data->feed, FEED_TYPE_TIMELINE, &data->postView);

    feed_load(&data->feed);
    data->initialized = true;
}

void timeline_update(TimelinePage* data, float deltaTime) {
    if (data == NULL) return;
}

void timeline_page_layout(TimelinePage *data, float deltaTime) {
    if (data == NULL) return;

    CLAY({
        .layout = {
            .sizing = {
                data->postView.opened ? CLAY_SIZING_GROW() : CLAY_SIZING_FIXED(TOP_BOTTOM_DIFF + BOTTOM_WIDTH + 1),
                CLAY_SIZING_GROW()
            },
            .layoutDirection = CLAY_LEFT_TO_RIGHT
        },
        .border = {
            .width = {
                .right = data->postView.opened ? 0 : 1
            },
            .color = get_current_theme()->accentColor
        }
    }) {
        if (!data->postView.opened) {
            // Empty space to push the posts to fit in the bottom screen
            CLAY({
                .layout = {
                    .sizing = { CLAY_SIZING_FIXED(TOP_BOTTOM_DIFF), CLAY_SIZING_GROW() }
                },
                .border = {
                    .width = {
                        .right = 1
                    },
                    .color = get_current_theme()->accentColor
                }
            });
    
            // Where the posts will be
            feed_layout(&data->feed, TOP_HEIGHT);
        } else {
            post_view_layout(&data->postView);
        }
    }

    post_view_input(&data->postView, deltaTime);
}

void timeline_free(TimelinePage* data) {
    feed_free(&data->feed);
}