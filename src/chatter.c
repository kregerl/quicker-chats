#include "chatter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/XKBlib.h>
#include <X11/Xutil.h>
#include <unistd.h>
#include <stdbool.h>
#include <ctype.h>

#define INVALID_EVENT_TYPE -1
static int key_press_type = INVALID_EVENT_TYPE;
static int key_release_type = INVALID_EVENT_TYPE;

int xi_opcode;

int register_events(Display* display, XDeviceInfo device_info) {
    unsigned long screen = DefaultScreen(display);
    Window root_window = RootWindow(display, screen);
    XDevice* device = XOpenDevice(display, device_info.id);
    if (!device) {
        fprintf(stderr, "Unable to open device '%lu'\n", device_info.id);
        exit(EXIT_FAILURE);
    }
    // Where the event types will be held
    // Only listening to KeyPress and KeyRelease so there is 2 events in the list.
    int num_events = 0;
    XEventClass event_list[2];

    if (device->num_classes > 0) {
        for (int i = 0; i < device_info.num_classes; i++) {
            XInputClassInfo class_info = device->classes[i];
            switch (class_info.input_class) {
                case KeyClass: {
                    DeviceKeyPress(device, key_press_type, event_list[num_events])
                    num_events++;
                    DeviceKeyRelease(device, key_release_type, event_list[num_events])
                    num_events++;
                    break;
                }
                default: {
                    printf("Unknown device class\n");
                    break;
                }
            }
        }
        // Validate events are registered
        if (XSelectExtensionEvent(display, root_window, event_list, num_events)) {
            fprintf(stderr, "error selecting extended events\n");
            exit(EXIT_FAILURE);
        }
    }
    return num_events;
}

XKeyEvent create_key_event(Display* display, Window focused_window, Window root_window, Bool press, KeyCode keycode,
                           int modifiers) {
    XKeyEvent event;

    event.display = display;
    event.window = focused_window;
    event.root = root_window;
    event.subwindow = None;
    event.time = CurrentTime;
    event.x = 1;
    event.y = 1;
    event.x_root = 1;
    event.y_root = 1;
    event.same_screen = True;
    event.keycode = keycode;
    event.state = modifiers;
    if (press)
        event.type = KeyPress;
    else
        event.type = KeyRelease;

    return event;
}

void send_event(Display* display, Window focused_window, Window root_window, Bool press, int modifiers, KeyCode code) {
    XKeyEvent key_event = create_key_event(display, focused_window, root_window, press, code, modifiers);
    int mask = press ? KeyPressMask : KeyReleaseMask;
    XSendEvent(key_event.display, key_event.window, True, mask, (XEvent*) &key_event);
}

KeyCode convert_char_to_keycode(Display* display, char c) {
    if ((int) c >= 0x20) {
        return XKeysymToKeycode(display, (KeySym) c);
    }
    fprintf(stderr, "Unknown keycode for char '%d'\n", (int) c);
    exit(EXIT_FAILURE);
}

void press_key(Display* display, Window focused_window, Window root_window, int modifiers, KeyCode code) {
    send_event(display, focused_window, root_window, True, modifiers, code);
    send_event(display, focused_window, root_window, False, modifiers, code);
}

// Converts string into keycodes and sends keycode events.
void write_string(Display* display, Window focused_window, Window root_window, const char* string) {
    for (int i = 0; i < strlen(string); i++) {
        char c = string[i];
        bool is_capital = false;

        // If the char is an ASCII capital letter, hold SHIFT.
        if (c == 33 || c == 63 || (c >= 65 && c <= 90)) {
            is_capital = true;
        }

        KeyCode code = convert_char_to_keycode(display, c);
        if (is_capital)
            send_event(display, focused_window, root_window, True, 0, XKeysymToKeycode(display, XK_Shift_L));
        press_key(display, focused_window, root_window, 0, code);
        if (is_capital)
            send_event(display, focused_window, root_window, False, 0, XKeysymToKeycode(display, XK_Shift_L));
    }
}

void write_rl_message(Display* display, Window focused_window, Window root_window, const char* string) {
    write_string(display, focused_window, root_window, "t");
    XFlush(display);
    usleep(10000);
    write_string(display, focused_window, root_window, string);
    XFlush(display);

    press_key(display, focused_window, root_window, 0, XKeysymToKeycode(display, XK_KP_Enter));
    XFlush(display);
}

char* trim_whitespace(char* str) {
    char* end;
    // Trim leading whitespace
    while (isspace(*str)) str++;

    if (*str == 0)
        return str;
    // Trim tailing whitespace
    end = str + strlen(str) - 1;
    while (end > str && isspace(*end)) end--;

    end[1] = '\0';
    return str;
}

#define DELIMITER ":"
void read_mappings(Display* display, char** mappings, const char* mappings_file) {
    FILE* file;
    file = fopen(mappings_file, "r");
    if (file == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    int line_num = 1;
    size_t length = 0;
    char* line = NULL;
    // Read all lines from mappings file
    while (getline(&line, &length, file) != -1) {
        char* key = trim_whitespace(strtok(line, DELIMITER));
        char* message = trim_whitespace(strtok(NULL, DELIMITER));
        char* remaining = strtok(NULL, DELIMITER);
        // If there is any remaining tokens then there are too many delimiters.
        if (remaining != NULL) {
            fprintf(stderr, "Error parsing %s on line #%d", mappings_file, line_num);
        }
        // Allocate space for the string inside the mappings table.
        KeyCode code = XKeysymToKeycode(display, XStringToKeysym(key));
        mappings[code] = malloc(sizeof(char) * strlen(message));
        // Copy string into mappings table.
        strcpy(mappings[code], message);
        line_num++;
    }

    fclose(file);
    if (line)
        free(line);
}

_Noreturn void start_event_listener(Display* display, const char* mappings_file) {
    Window root_window = DefaultRootWindow(display);

    char* mappings[255];
    memset(&mappings, 0, 255 * sizeof(char*));
    read_mappings(display, mappings, mappings_file);

    XEvent event;
    setvbuf(stdout, NULL, _IOLBF, 0);
    bool held_keys[255];
    memset(&held_keys, 0, 255);
    while (1) {
        XNextEvent(display, &event);
        if (event.type == key_press_type || event.type == key_release_type) {
            XDeviceKeyEvent* key = (XDeviceKeyEvent*) &event;
            printf("key %s %d\n", (event.type == key_release_type) ? "release" : "press  ",
                   key->keycode);
            if (event.type == key_press_type) {
                Window focused_window;
                int revert;
                XGetInputFocus(display, &focused_window, &revert);
                // Skip invalid indices
                if (key->keycode > 255 || key->keycode < 8)
                    continue;
                const char* message = mappings[key->keycode];

                // No message at keycodes
                if (message == NULL) {
                    held_keys[key->keycode] = true;
                    continue;
                }
                write_rl_message(display, focused_window, root_window, message);

                usleep(10000);
                // Fix keyboard state after sending message
                for (int i = 0; i < 256; i++) {
                    if (held_keys[i]) {
                        send_event(display, focused_window, root_window, True, 0, (KeyCode) i);
                    }
                    XFlush(display);
                }
            } else {
                held_keys[key->keycode] = false;
            }
        }
    }
}

XDeviceInfo get_device_info(Display* display, int id) {
    int num_devices;
    XDeviceInfo* devices = XListInputDevices(display, &num_devices);

    if (!id || id > num_devices || id < 1) {
        fprintf(stderr, "'%d' is not a valid device id.\n", id);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < num_devices; i++) {
        XDeviceInfo info = devices[i];
        if (info.id == id) {
            return devices[i];
        }
    }
    fprintf(stderr, "No device with id '%d'\n", id);
    exit(EXIT_FAILURE);
}

XDeviceInfo prompt_device_selection(Display* display) {
    XDeviceInfo* devices;
    int num_devices;
    devices = XListInputDevices(display, &num_devices);

    // Print list of devices for user to select from
    for (int i = 0; i < num_devices; i++) {
        XDeviceInfo info = devices[i];
        printf("%d: %s(%lu)\n", i + 1, info.name, info.id);
    }
    printf("Select which device to use by number: ");
    char id_buffer[4];
    scanf("%4s", id_buffer);

    // Convert user's input to int and index device array.
    int id = (int) strtol((const char*) &id_buffer, NULL, 10);
    if (!id || id > num_devices || id < 1) {
        fprintf(stderr, "'%d' is not a valid device id.\n", id);
        exit(EXIT_FAILURE);
    }
    // Return selected device
    return devices[id - 1];
}

void init(int id, const char* mappings_file) {
    Display* display = XOpenDisplay(NULL);

    if (!display) {
        fprintf(stderr, "Unable to connect to X server\n");
        exit(EXIT_FAILURE);
    }

    int event, error;
    if (!XQueryExtension(display, "XInputExtension", &xi_opcode, &event, &error)) {
        fprintf(stderr, "XInput extension not available.\n");
        exit(EXIT_FAILURE);
    }

    // Prompt user to select device
    XDeviceInfo info;
    if (id < 0) {
        info = prompt_device_selection(display);
    } else {
        info = get_device_info(display, id);
    }
    XkbSetDetectableAutoRepeat(display, True, NULL);
    if (register_events(display, info)) {
        start_event_listener(display, mappings_file);
    } else {
        fprintf(stderr, "No events registered.\n");
        exit(EXIT_FAILURE);
    }
}