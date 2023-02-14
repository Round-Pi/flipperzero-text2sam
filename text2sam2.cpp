#include <furi.h>
#include <furi_hal_speaker.h>
#include <gui/gui.h>
#include <input/input.h>
#include <dialogs/dialogs.h>
#include <flipper_format/flipper_format.h>
#include "stm32_sam.h"
#define SAM_DIR EXT_PATH("sam")
#define TEXT_BUFFER_SIZE 32
#define TAG "SAM"

STM32SAM voice;

// typedef enum {
//     EventTypeTick,
//     EventTypeKey,
// } EventType;

// typedef struct {
//     EventType type;
//     InputEvent input;
// } PluginEvent;

ViewPort* view_port;
char* words;

static void say_something(char* something) {
    if(furi_hal_speaker_is_mine() || furi_hal_speaker_acquire(1000)) {
        voice.begin();
        voice.say(something);
        furi_hal_speaker_release();
    }
}
// static void say_something(FuriString* something) {
//     char* something_char;
//     if(furi_string_get_char(something, 0)) {
//         for(int i = 0; furi_string_get_char(something, i); i++) {
//             something_char[i] = furi_string_get_char(something, i);
//         }
//         say_something(something_char);
//     }
// }
// static void sing_something(char* something) {
//     if(furi_hal_speaker_is_mine() || furi_hal_speaker_acquire(1000)) {
//         voice.begin();
//         voice.sing(something);
//         furi_hal_speaker_release();
//     }
// }
// static void sing_something(FuriString* something) {
//     char* something_char;
//     if(furi_string_get_char(something, 0)) {
//         for(int i = 0; furi_string_get_char(something, i); i++) {
//             something_char[i] = furi_string_get_char(something, i);
//         }
//         sing_something(something_char);
//     }
// }

static void app_input_callback(InputEvent* input_event, void* ctx) {
    furi_assert(ctx);
    FuriMessageQueue* event_queue = (FuriMessageQueue*)ctx;
    furi_message_queue_put(event_queue, input_event, FuriWaitForever);
}

extern "C" int32_t text2sam2(void* p) {
    UNUSED(p);
    FuriMessageQueue* event_queue = furi_message_queue_alloc(8, sizeof(InputEvent));

    view_port = view_port_alloc();

    // Configure view port
    view_port_input_callback_set(view_port, app_input_callback, event_queue);

    // Register view port in GUI
    Gui* gui = (Gui*)furi_record_open(RECORD_GUI);
    gui_add_view_port(gui, view_port, GuiLayerFullscreen);

    FURI_LOG_D(TAG, "Storage");
    Storage* storage = (Storage*)furi_record_open(RECORD_STORAGE);
    FlipperFormat* ff = flipper_format_file_alloc(storage);
    FURI_LOG_D(TAG, "DialogApp");
    DialogsApp* dialogs = (DialogsApp*)furi_record_open(RECORD_DIALOGS);
    FURI_LOG_D(TAG, "DialogsFileBrowserOptions");
    DialogsFileBrowserOptions browser_options;
    dialog_file_browser_set_basic_options(&browser_options, ".txt", NULL);
    // FuriString* map_file = furi_string_alloc();
    // furi_string_set(map_file, SAM_DIR);
    FuriString* map_file = (FuriString*)SAM_DIR;
    FURI_LOG_D(TAG, "storage_file_exists");
    if(!storage_file_exists(storage, SAM_DIR)) {
        storage_common_mkdir(storage, SAM_DIR); //Make Folder If dir not exist
    }

    bool res = dialog_file_browser_show(dialogs, map_file, map_file, &browser_options);

    furi_record_close(RECORD_DIALOGS);

    // if user didn't choose anything, free everything and exit
    if(res) {
        FURI_LOG_D(TAG, "opening file: %s", furi_string_get_cstr(map_file));
        if(!flipper_format_file_open_existing(ff, furi_string_get_cstr(map_file))) {
            FURI_LOG_E(TAG, "Could not open file %s", furi_string_get_cstr(map_file));
        }
        File* file = storage_file_alloc(storage);
        uint16_t bytes_read = 0;
        if(storage_file_open(file, furi_string_get_cstr(map_file), FSAM_READ, FSOM_OPEN_EXISTING)) {
            bytes_read = storage_file_read(file, words, TEXT_BUFFER_SIZE);
        }
        if(bytes_read != TEXT_BUFFER_SIZE) FURI_LOG_E(TAG, "Size mismatch");
        say_something(words);
    }
    FURI_LOG_I(TAG, "exit");

    // Free all things
    flipper_format_free(ff);
    furi_record_close(RECORD_STORAGE);
    view_port_enabled_set(view_port, false);
    gui_remove_view_port(gui, view_port);
    view_port_free(view_port);
    furi_message_queue_free(event_queue);
    furi_record_close(RECORD_GUI);
    return 0;
}
