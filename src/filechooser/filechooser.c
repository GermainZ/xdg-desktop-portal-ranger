#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include "xdpw.h"
#include "filechooser.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>

#define PATH_PREFIX "file://"

static const char object_path[] = "/org/freedesktop/portal/desktop";
static const char interface_name[] = "org.freedesktop.impl.portal.FileChooser";

static char *term_cmd;
static char *filechooser_script;

static int exec_ranger(bool open, bool multiple, bool directory, char *path, char ***selected_files, size_t *num_selected_files) {
    size_t str_size = snprintf(NULL, 0, "%s %s %d %d %d \"%s\"", term_cmd, filechooser_script, open, multiple, directory, path) + 1;
    char *cmd = malloc(str_size);
    snprintf(cmd, str_size, "%s %s %d %d %d \"%s\"", term_cmd, filechooser_script, open, multiple, directory, path);

    FILE *fp = popen(cmd, "r");
    if (fp == NULL) {
        logprint(ERROR, "failed to run command");
        return -1;
    }

    size_t num_lines = 0;
    char cr = getc(fp);
    while (cr != EOF) {
        if (cr == '\n') {
            num_lines++;
        }
        cr = getc(fp);
    }
    rewind(fp);

    if (num_lines == 0) {
        num_lines = 1;
    }

    *num_selected_files = num_lines;
    *selected_files = malloc((num_lines + 1) * sizeof(char*));

    for (size_t i = 0; i < num_lines; i++) {
        size_t n = 0;
        char *line = NULL;
        ssize_t nread = getline(&line, &n, fp);
        if (ferror(fp)) {
            // TODO free line, selected_files[*], selected_files
            return 1;
        }
        size_t str_size = nread + strlen(PATH_PREFIX) + 1;
        if (line[nread - 1] == '\n') {
            str_size -= 1;
        }
        (*selected_files)[i] = malloc(str_size);
        snprintf((*selected_files)[i], str_size, "%s%s", PATH_PREFIX, line);
    }
    (*selected_files)[num_lines] = NULL;

    pclose(fp);
    return 0;
}

static int method_open_file(sd_bus_message *msg, void *data, sd_bus_error *ret_error) {
    int ret = 0;

    char *handle, *app_id, *parent_window, *title;
    ret = sd_bus_message_read(msg, "osss", &handle, &app_id, &parent_window, &title);
    if (ret < 0) {
        return ret;
    }

    ret = sd_bus_message_enter_container(msg, 'a', "{sv}");
    if (ret < 0) {
        return ret;
    }
    char *key;
    int inner_ret = 0;
    bool multiple, directory;
    while ((ret = sd_bus_message_enter_container(msg, 'e', "sv")) > 0) {
        inner_ret = sd_bus_message_read(msg, "s", &key);
        if (inner_ret < 0) {
            return inner_ret;
        }

        logprint(DEBUG, "dbus: option %s", key);
        if (strcmp(key, "multiple") == 0) {
            sd_bus_message_read(msg, "v", "b", &multiple);
            logprint(DEBUG, "dbus: option multiple: %x", multiple);
        } else if (strcmp(key, "modal") == 0) {
            bool modal;
            sd_bus_message_read(msg, "v", "b", &modal);
            logprint(DEBUG, "dbus: option modal: %x", modal);
        } else if (strcmp(key, "directory") == 0) {
            sd_bus_message_read(msg, "v", "b", &directory);
            logprint(DEBUG, "dbus: option directory: %x", directory);
        } else {
            logprint(WARN, "dbus: unknown option %s", key);
            sd_bus_message_skip(msg, "v");
        }

        inner_ret = sd_bus_message_exit_container(msg);
        if (inner_ret < 0) {
            return inner_ret;
        }
    }
    if (ret < 0) {
        return ret;
    }
    ret = sd_bus_message_exit_container(msg);
    if (ret < 0) {
        return ret;
    }

    // TODO: cleanup this
    struct xdpw_request *req =
        xdpw_request_create(sd_bus_message_get_bus(msg), handle);
    if (req == NULL) {
        return -ENOMEM;
    }

    char **selected_files = NULL;
    size_t num_selected_files = 0;
    ret = exec_ranger(true, multiple, directory, NULL, &selected_files, &num_selected_files);
    if (ret) {
        return ret;
    }

    logprint(TRACE, "Selected files: %d", num_selected_files);
    for (size_t i = 0; i < num_selected_files; i++) {
        logprint(TRACE, "%d. %s", i, selected_files[i]);
    }

    sd_bus_message *reply = NULL;
    ret = sd_bus_message_new_method_return(msg, &reply);
    if (ret < 0) {
        return ret;
    }

    ret = sd_bus_message_append(reply, "u", PORTAL_RESPONSE_SUCCESS, 1);
    if (ret < 0) {
        return ret;
    }

    ret = sd_bus_message_open_container(reply, 'a', "{sv}");
    if (ret < 0) {
        return ret;
    }

    ret = sd_bus_message_open_container(reply, 'e', "sv");
    if (ret < 0) {
        return ret;
    }

    ret = sd_bus_message_append_basic(reply, 's', "uris");
    if (ret < 0) {
        return ret;
    }

    ret = sd_bus_message_open_container(reply, 'v', "as");
    if (ret < 0) {
        return ret;
    }

    ret = sd_bus_message_append_strv(reply, selected_files);
    if (ret < 0) {
        return ret;
    }

    ret = sd_bus_message_close_container(reply);
    if (ret < 0) {
        return ret;
    }

    ret = sd_bus_message_close_container(reply);
    if (ret < 0) {
        return ret;
    }

    ret = sd_bus_message_close_container(reply);
    if (ret < 0) {
        return ret;
    }

    ret = sd_bus_send(NULL, reply, NULL);
    if (ret < 0) {
        return ret;
    }

    sd_bus_message_unref(reply);
    return 0;
}

static int method_save_file(sd_bus_message *msg, void *data, sd_bus_error *ret_error) {
    int ret = 0;

    char *handle, *app_id, *parent_window, *title;
    ret = sd_bus_message_read(msg, "osss", &handle, &app_id, &parent_window, &title);
    if (ret < 0) {
        return ret;
    }

    ret = sd_bus_message_enter_container(msg, 'a', "{sv}");
    if (ret < 0) {
        return ret;
    }
    char *key;
    int inner_ret = 0;
    char *current_name;
    char *current_folder;
    while ((ret = sd_bus_message_enter_container(msg, 'e', "sv")) > 0) {
        inner_ret = sd_bus_message_read(msg, "s", &key);
        if (inner_ret < 0) {
            return inner_ret;
        }

        logprint(DEBUG, "dbus: option %s", key);
        if (strcmp(key, "current_name") == 0) {
            sd_bus_message_read(msg, "v", "s", &current_name);
            logprint(DEBUG, "dbus: option current_name: %s", current_name);
        } else if (strcmp(key, "current_folder") == 0) {
            const void *p = NULL;
            size_t sz = 0;
            inner_ret = sd_bus_message_enter_container(msg, 'v', "ay");
            if (inner_ret < 0) {
                return inner_ret;
            }
            inner_ret = sd_bus_message_read_array(msg, 'y', &p, &sz);
            if (inner_ret < 0) {
                return inner_ret;
            }
            current_folder = (char *) p;
            logprint(DEBUG, "dbus: option current_folder: %s", current_folder);
        } else {
            logprint(WARN, "dbus: unknown option %s", key);
            sd_bus_message_skip(msg, "v");
        }

        inner_ret = sd_bus_message_exit_container(msg);
        if (inner_ret < 0) {
            return inner_ret;
        }
    }

    // TODO: cleanup this
    struct xdpw_request *req =
        xdpw_request_create(sd_bus_message_get_bus(msg), handle);
    if (req == NULL) {
        return -ENOMEM;
    }

    char **selected_files = NULL;
    size_t num_selected_files = 0;

    size_t path_size = snprintf(NULL, 0, "%s/%s", current_folder, current_name);
    char *path = malloc(path_size);
    snprintf(path, path_size, "%s/%s", current_folder, current_name);

    bool file_already_exists = true;
    while (file_already_exists) {
        if (access(path, F_OK) == 0) {
            char *path_tmp = malloc(path_size);
            snprintf(path_tmp, path_size, "%s", path);
            path_size += 1;
            path = realloc(path, path_size);
            snprintf(path, path_size, "%s.", path_tmp);
            free(path_tmp);
        } else {
            file_already_exists = false;
        }
    }

    ret = exec_ranger(false, false, false, path, &selected_files, &num_selected_files);
    if (num_selected_files == 0) {
        return -1;
    }

    logprint(TRACE, "Selected files: %d", num_selected_files);
    for (size_t i = 0; i < num_selected_files; i++) {
        logprint(TRACE, "%d. %s", i, selected_files[i]);
    }

    sd_bus_message *reply = NULL;
    ret = sd_bus_message_new_method_return(msg, &reply);
    if (ret < 0) {
        return ret;
    }

    ret = sd_bus_message_append(reply, "u", PORTAL_RESPONSE_SUCCESS, 1);
    if (ret < 0) {
        return ret;
    }

    ret = sd_bus_message_open_container(reply, 'a', "{sv}");
    if (ret < 0) {
        return ret;
    }

    ret = sd_bus_message_open_container(reply, 'e', "sv");
    if (ret < 0) {
        return ret;
    }

    ret = sd_bus_message_append_basic(reply, 's', "uris");
    if (ret < 0) {
        return ret;
    }

    ret = sd_bus_message_open_container(reply, 'v', "as");
    if (ret < 0) {
        return ret;
    }

    ret = sd_bus_message_append_strv(reply, selected_files);
    if (ret < 0) {
        return ret;
    }

    ret = sd_bus_message_close_container(reply);
    if (ret < 0) {
        return ret;
    }

    ret = sd_bus_message_close_container(reply);
    if (ret < 0) {
        return ret;
    }

    ret = sd_bus_message_close_container(reply);
    if (ret < 0) {
        return ret;
    }

    ret = sd_bus_send(NULL, reply, NULL);
    if (ret < 0) {
        return ret;
    }

    sd_bus_message_unref(reply);
    return 0;
}

static const sd_bus_vtable filechooser_vtable[] = {
    SD_BUS_VTABLE_START(0),
    SD_BUS_METHOD("OpenFile", "osssa{sv}", "ua{sv}", method_open_file, SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD("SaveFile", "osssa{sv}", "ua{sv}", method_save_file, SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_VTABLE_END
};

int xdpw_filechooser_init(struct xdpw_state *state) {
    // TODO: cleanup
    term_cmd = state->config->filechooser_conf.term_cmd;
    filechooser_script = state->config->filechooser_conf.filechooser_script;
    sd_bus_slot *slot = NULL;
    logprint(DEBUG, "dbus: init %s", interface_name);
    return sd_bus_add_object_vtable(state->bus, &slot, object_path, interface_name,
            filechooser_vtable, NULL);
}
