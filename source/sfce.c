// https://code.visualstudio.com/blogs/2018/03/23/text-buffer-reimplementation
// https://github.com/microsoft/vscode/tree/3cf67889583203811c81ca34bea2ad02d7c902db/src/vs/editor/common/model/pieceTreeTextBuffer
// https://github.com/microsoft/vscode-textbuffer
// gcc main.c -O3 -Werror -Wfatal-errors -Wall -o bin/sfce

#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
#   define SFCE_PLATFORM_WINDOWS
#elif defined(__linux__) || defined(__linux) || defined(linux)
#   define SFCE_PLATFORM_LINUX
#elif defined(__APPLE__) || defined(__MACH__)
#   define SFCE_PLATFORM_APPLE
#elif defined(__FreeBSD__)
#   define SFCE_PLATFORM_FREE_BSD
#endif

#include <stdint.h>
#include <stdio.h>
#include <assert.h>

#if defined(SFCE_PLATFORM_WINDOWS)
#  include <windows.h>
#  include <wincon.h>
#  include <conio.h>
#else
#  include <unistd.h>
#  include <sys/uio.h>
#endif

#define PRINT_LINE_NUMBER() fprintf(stderr, "LINE: %u\n", __LINE__)
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) < (b) ? (b) : (a))

enum {
    SFCE_FILE_READ_CHUNK_SIZE = 1024,
    SFCE_STRING_BUFFER_SIZE_THRESHOLD = 0xFFFF,
    
    //
    // ALL ALLOCATION SIZES MUST BE
    // A POWER OF TWO.
    //
    SFCE_LINE_STARTS_ALLOCATION_SIZE = 16,
    SFCE_STRING_BUFFER_ALLOCATION_SIZE = 16,
    SFCE_SNAPSHOT_ALLOCATION_SIZE = 16,
    SFCE_STRING_ALLOCATION_SIZE = 256,
};

enum sfce_boolean {
    SFCE_FALSE = 0,
    SFCE_TRUE  = 1,
};

enum sfce_error_code {
    SFCE_ERROR_OK,
    SFCE_ERROR_NEGATIVE_BUFFER_SIZE,
    SFCE_ERROR_BAD_INSERTION,
    SFCE_ERROR_MEMORY_ALLOCATION_FAILURE,
    SFCE_ERROR_UNABLE_TO_OPEN_FILE,
    SFCE_ERROR_UNIMPLEMENTED,
    SFCE_ERROR_INVALID_OFFSETS,
    SFCE_ERROR_OUTSIDE_CONSOLE_BOUNDS,
    SFCE_ERROR_FAILED_CONSOLE_READ,
    SFCE_ERROR_FAILED_CONSOLE_WRITE,
    SFCE_ERROR_FAILED_CONSOLE_STATE_RESTORE,
    SFCE_ERROR_FAILED_CONSOLE_STATE_SAVE,
    SFCE_ERROR_WIN32_API_FAILED,
    SFCE_ERROR_UNIX_API_FAILED,
    SFCE_ERROR_FORMATED_STRING_TOO_LARGE,
};

enum sfce_red_black_color {
    SFCE_COLOR_BLACK = 0,
    SFCE_COLOR_RED   = 1,
};

enum sfce_newline_type {
    SFCE_NEWLINE_TYPE_NONE,
    SFCE_NEWLINE_TYPE_CRLF,
    SFCE_NEWLINE_TYPE_CR,
    SFCE_NEWLINE_TYPE_LF,
};

struct sfce_window_size {
    int32_t width;
    int32_t height;
};

struct sfce_point {
    int32_t x;
    int32_t y;
};

struct sfce_string {
    char   *data;
    int32_t size;
    int32_t capacity;
};

struct sfce_string_view {
    const char *data;
    int32_t     size;
};

struct sfce_line_starts {
    int32_t *offsets;
    int32_t  count;
    int32_t  capacity;
};

struct sfce_string_buffer {
    struct sfce_string      content;
    struct sfce_line_starts line_starts;
};

struct sfce_string_buffer_position {
    int32_t line_start_index;
    int32_t column;
};

struct sfce_piece {
    struct sfce_string_buffer_position start;
    struct sfce_string_buffer_position end;
    uint32_t                           buffer_index;
    int32_t                            line_count;
    int32_t                            length;
};

struct sfce_piece_pair {
    struct sfce_piece left;
    struct sfce_piece right;
};

struct sfce_piece_node {
    struct sfce_piece_node    *left;
    struct sfce_piece_node    *right;
    struct sfce_piece_node    *parent;
    struct sfce_piece          piece;
    int32_t                    left_subtree_length;
    int32_t                    left_subtree_line_count;
    enum sfce_red_black_color  color;
};

struct sfce_node_position {
    struct sfce_piece_node *node;
    int32_t                 node_start_offset;
    // int32_t                 node_start_line_number;
    int32_t                 offset_within_piece;
};

struct sfce_piece_tree {
    struct sfce_piece_node    *root;
    struct sfce_string_buffer *buffers;
    int32_t                    buffer_count;
    int32_t                    buffer_capacity;
    int32_t                    line_count;
    int32_t                    length;
    int32_t                    change_buffer_index;
    enum sfce_newline_type     newline_type;
};

struct sfce_piece_tree_snapshot {
    struct sfce_piece *pieces;
    int32_t            piece_count;
    int32_t            piece_capacity;
};

struct sfce_console_state {
#if defined(SFCE_PLATFORM_WINDOWS)
    HANDLE input_handle;
    HANDLE output_handle;
    DWORD  output_mode;
    DWORD  input_mode;
#elif defined(SFCE_PLATFORM_LINUX)
#elif defined(SFCE_PLATFORM_APPLE)
#elif defined(SFCE_PLATFORM_FREE_BSD)
#endif
};

enum sfce_console_color_mode {
    SFCE_CONSOLE_COLOR_MODE_COLOR16,
    SFCE_CONSOLE_COLOR_MODE_COLOR256,
    SFCE_CONSOLE_COLOR_MODE_TRUE_COLOR,
};

struct sfce_console_cell_indexed {
    int32_t character;
    int16_t character_style;
    int16_t foreground_color;
    int16_t background_color;
};

struct sfce_console_cell_rgb {
    int32_t character;
    int16_t character_style;
    int8_t  foreground_red;
    int8_t  foreground_green;
    int8_t  foreground_blue;
    int8_t  background_red;
    int8_t  background_green;
    int8_t  background_blue;
};

union sfce_console_cell {
    struct {
        int32_t character;
        int16_t character_style;
    };

    struct sfce_console_cell_rgb rgb;
    struct sfce_console_cell_indexed index;
};

struct sfce_console_buffer {
    union sfce_console_cell     *cells;
    int32_t                      width;
    int32_t                      height;
    struct sfce_string           temp_command_sequence;
    enum sfce_console_color_mode color_mode;
};

int32_t round_multiple_of_two(int32_t value, int32_t multiple);
int32_t newline_sequence_size(const char *buffer, int32_t buffer_size);
int32_t buffer_newline_count(const char *buffer, int32_t buffer_size);
enum sfce_error_code sfce_get_console_screen_size(struct sfce_window_size *window_size);
enum sfce_error_code sfce_write(const void *buffer, int32_t buffer_size);
enum sfce_error_code sfce_write_string(const char *buffer);
enum sfce_error_code sfce_enable_console_temp_buffer();
enum sfce_error_code sfce_disable_console_temp_buffer();
enum sfce_error_code sfce_save_console_state(struct sfce_console_state *state);
enum sfce_error_code sfce_restore_console_state(const struct sfce_console_state *state);
enum sfce_error_code sfce_enable_virtual_terminal(const struct sfce_console_state *state);
enum sfce_error_code sfce_setup_console(struct sfce_console_state *state);
enum sfce_error_code sfce_restore_console(const struct sfce_console_state *state);

void sfce_string_destroy(struct sfce_string *string);
enum sfce_error_code sfce_string_reserve(struct sfce_string *result, int32_t capacity);
enum sfce_error_code sfce_string_resize(struct sfce_string *result, int32_t size);
enum sfce_error_code sfce_string_write(struct sfce_string *string, int32_t index, const void *buffer, int32_t buffer_size);
enum sfce_error_code sfce_string_insert(struct sfce_string *string, int32_t index, const void *buffer, int32_t buffer_size);
enum sfce_error_code sfce_string_push_back(struct sfce_string *string, int32_t character);
enum sfce_error_code sfce_string_push_back_buffer(struct sfce_string *string, const void *buffer, int32_t buffer_size);
enum sfce_error_code sfce_string_append_substring(struct sfce_string *string, struct sfce_string source_string, int32_t start_index, int32_t end_index);
enum sfce_error_code sfce_string_load_file(struct sfce_string *string, const char *filepath);
enum sfce_error_code sfce_string_append_formated_string(struct sfce_string *string, const void *format, ...);
enum sfce_error_code sfce_string_flush(struct sfce_string *string);
void sfce_string_clear(struct sfce_string *string);
int16_t sfce_string_compare(struct sfce_string string0, struct sfce_string string1);

void sfce_line_starts_destroy(struct sfce_line_starts *lines);
enum sfce_error_code sfce_line_starts_reserve(struct sfce_line_starts *lines, int32_t capacity);
enum sfce_error_code sfce_line_starts_resize(struct sfce_line_starts *lines, int32_t size);
enum sfce_error_code sfce_line_starts_push_line_offset(struct sfce_line_starts *lines, int32_t offset);

void sfce_string_buffer_destroy(struct sfce_string_buffer *buffer);
enum sfce_error_code sfce_string_buffer_append_content(struct sfce_string_buffer *buffer, const char *data, int32_t size);
struct sfce_string_buffer_position sfce_string_buffer_get_end_position(struct sfce_string_buffer *buffer);
struct sfce_string_buffer_position sfce_string_buffer_position_from_offset(struct sfce_string_buffer *buffer, int32_t offset);
struct sfce_string_buffer_position sfce_string_buffer_piece_position_in_buffer(struct sfce_string_buffer *buffer, struct sfce_piece piece, int32_t offset_within_piece);
struct sfce_string_buffer_position sfce_string_buffer_move_position_by_offset(struct sfce_string_buffer *buffer, struct sfce_string_buffer_position position, int32_t offset);
int32_t sfce_string_buffer_offset_from_position(struct sfce_string_buffer *string_buffer, struct sfce_string_buffer_position position);

struct sfce_piece_pair sfce_piece_split(struct sfce_piece_tree *tree, struct sfce_piece piece, int32_t offset, int32_t gap_size);
struct sfce_piece sfce_piece_erase_head(struct sfce_piece_tree *tree, struct sfce_piece piece, int32_t amount);
struct sfce_piece sfce_piece_erase_tail(struct sfce_piece_tree *tree, struct sfce_piece piece, int32_t amount);

struct sfce_piece_node *sfce_piece_node_create(struct sfce_piece piece);
void sfce_piece_node_destroy(struct sfce_piece_node *tree);
void sfce_piece_node_destroy_nonrecursive(struct sfce_piece_node *tree);
int32_t sfce_piece_node_calculate_length(struct sfce_piece_node *root);
int32_t sfce_piece_node_calculate_line_count(struct sfce_piece_node *root);
struct sfce_piece_node *sfce_piece_node_leftmost(struct sfce_piece_node *node);
struct sfce_piece_node *sfce_piece_node_rightmost(struct sfce_piece_node *node);
struct sfce_piece_node *sfce_piece_node_next(struct sfce_piece_node *node);
struct sfce_piece_node *sfce_piece_node_prev(struct sfce_piece_node *node);
struct sfce_piece_node *sfce_piece_node_rotate_left(struct sfce_piece_node **root, struct sfce_piece_node *node);
struct sfce_piece_node *sfce_piece_node_rotate_right(struct sfce_piece_node **root, struct sfce_piece_node *node);
struct sfce_piece_node *sfce_piece_node_insert_left(struct sfce_piece_node **root, struct sfce_piece_node *where, struct sfce_piece_node *node_to_insert);
struct sfce_piece_node *sfce_piece_node_insert_right(struct sfce_piece_node **root, struct sfce_piece_node *where, struct sfce_piece_node *node_to_insert);
void sfce_piece_node_remove_node(struct sfce_piece_node **root, struct sfce_piece_node *where);
void sfce_piece_node_transplant(struct sfce_piece_node **root, struct sfce_piece_node *where, struct sfce_piece_node *node_to_transplant);
void sfce_piece_node_update_metadata(struct sfce_piece_node **root, struct sfce_piece_node *node, int32_t delta_length, int32_t delta_line_count);
void sfce_piece_node_recompute_metadata(struct sfce_piece_node **root, struct sfce_piece_node *node);
void sfce_piece_node_fix_insert_violation(struct sfce_piece_node **root, struct sfce_piece_node *node);
void sfce_piece_node_fix_remove_violation(struct sfce_piece_node **root, struct sfce_piece_node *node);
void sfce_piece_node_print(struct sfce_piece_tree *tree, struct sfce_piece_node *root, uint32_t space);
void sfce_piece_node_inorder_print(struct sfce_piece_tree *tree, struct sfce_piece_node *root);
void sfce_piece_node_reset_sentinel();

struct sfce_piece_tree *sfce_piece_tree_create(enum sfce_newline_type newline_type);
void sfce_piece_tree_destroy(struct sfce_piece_tree *tree);
int32_t sfce_piece_tree_line_number_offset_within_piece(struct sfce_piece_tree *tree, struct sfce_piece piece, int32_t line_number);
struct sfce_string_view sfce_piece_tree_get_piece_content(struct sfce_piece_tree *tree, struct sfce_piece piece);
struct sfce_node_position sfce_piece_tree_node_at_offset(struct sfce_piece_tree *tree, int32_t offset);
struct sfce_node_position sfce_piece_tree_get_node_position_next_line(struct sfce_piece_tree *tree, const struct sfce_node_position position);
struct sfce_node_position sfce_piece_tree_node_at_row_and_col(struct sfce_piece_tree *tree, int32_t row, int32_t col);
enum sfce_error_code sfce_piece_tree_get_content_between_node_positions(struct sfce_piece_tree *tree, struct sfce_node_position position0, struct sfce_node_position position1, struct sfce_string *string);
enum sfce_error_code sfce_piece_tree_ensure_change_buffer_size(struct sfce_piece_tree *tree, int32_t required_size);
enum sfce_error_code sfce_piece_tree_set_buffer_count(struct sfce_piece_tree *tree, int32_t buffer_count);
enum sfce_error_code sfce_piece_tree_add_new_string_buffer(struct sfce_piece_tree *tree);
enum sfce_error_code sfce_piece_tree_create_node_subtree(struct sfce_piece_tree *tree, const char *buffer, int32_t buffer_size, struct sfce_piece_node **result);
enum sfce_error_code sfce_piece_tree_create_piece(struct sfce_piece_tree *tree, const char *data, int32_t byte_count, struct sfce_piece *result_piece);
enum sfce_error_code sfce_piece_tree_insert(struct sfce_piece_tree *tree, int32_t offset, const char *data, int32_t byte_count);
enum sfce_error_code sfce_piece_tree_erase(struct sfce_piece_tree *tree, int32_t offset, int32_t byte_count);
enum sfce_error_code sfce_piece_tree_insert_with_position(struct sfce_piece_tree *tree, struct sfce_node_position position, const char *data, int32_t byte_count);
enum sfce_error_code sfce_piece_tree_erase_with_position(struct sfce_piece_tree *tree, struct sfce_node_position start, struct sfce_node_position end);
enum sfce_error_code sfce_piece_tree_load_file(struct sfce_piece_tree *tree, const char *filepath);
enum sfce_error_code sfce_piece_tree_get_line_content(struct sfce_piece_tree *tree, int32_t line_number, struct sfce_string *string);
enum sfce_error_code sfce_piece_tree_create_snapshot(struct sfce_piece_tree *tree, struct sfce_piece_tree_snapshot *snapshot);
void sfce_piece_tree_recompute_metadata(struct sfce_piece_tree *tree);

enum sfce_error_code sfce_piece_tree_snapshot_set_piece_count(struct sfce_piece_tree_snapshot *snapshot, int32_t count);
enum sfce_error_code sfce_piece_tree_snapshot_add_piece(struct sfce_piece_tree_snapshot *snapshot, struct sfce_piece piece);

enum sfce_boolean sfce_console_cell_attributes_equal(enum sfce_console_color_mode color_mode, union sfce_console_cell *cell0, union sfce_console_cell *cell1);

void sfce_console_buffer_flush(struct sfce_console_buffer *buffer);
void sfce_console_buffer_destroy(struct sfce_console_buffer *buffer);

static struct sfce_piece_node sentinel = {
    .parent = &sentinel,
    .left   = &sentinel,
    .right  = &sentinel,
    .color  = SFCE_COLOR_BLACK,
};

static struct sfce_piece_node *sentinel_ptr = &sentinel;

int main2(int argc, const char *argv[])
{
    enum sfce_error_code error_code;

    if (argc < 2) {
        fprintf(stderr, "USAGE: sfce path/to/file\n");
        return -1;
    }

    const char *filepath = argv[1];
    struct sfce_string line_contents = {};
    struct sfce_piece_tree *tree = sfce_piece_tree_create(SFCE_NEWLINE_TYPE_CRLF);

    error_code = sfce_piece_tree_load_file(tree, filepath);
    if (error_code != SFCE_ERROR_OK) {
        fprintf(stderr, "ERROR: unable to load file: '%s'\n", filepath);
        goto error;
    }

    for (int is_running = 1; is_running;) {
        if (_kbhit()) {
            int32_t keycode = _getch();

            switch (keycode)
            {
            case '\x1b':
                is_running = 0;
                break;
            case 'B':
                is_running = 0;
        
                fprintf(stderr, "tree->line_count: %d", (int)tree->line_count);
                break;
            default:
                if (isprint(keycode) || isspace(keycode) || keycode == '\n' || keycode == '\r') {
                    sfce_piece_tree_insert(tree, 0, (char[]){ keycode }, 1);
                }
                break;
            }
        }

        if (is_running == 0) {
            break;
        }

        for (int32_t row = 0; row < tree->line_count; ++row) {
            error_code = sfce_piece_tree_get_line_content(tree, row, &line_contents);
            if (error_code != SFCE_ERROR_OK) {
                continue;
            }

            fprintf(stderr, "line_content at (%d): '%.*s'\n", row + 1, (int)line_contents.size, line_contents.data);
        }
    }

    sfce_string_destroy(&line_contents);
    sfce_piece_tree_destroy(tree);
    return 0;
error:
    sfce_string_destroy(&line_contents);
    sfce_piece_tree_destroy(tree);
    return -1;
}

int main(int argc, const char *argv[])
{
    enum sfce_error_code error_code;

    if (argc < 2) {
        fprintf(stderr, "USAGE: sfce path/to/file\n");
        return -1;
    }

    struct sfce_console_state console_state = {};
    error_code = sfce_setup_console(&console_state);
    if (error_code != SFCE_ERROR_OK) {
        fprintf(stderr, "error code: %d\n", error_code);
        goto error;
    }

    const char *filepath = argv[1];
    struct sfce_string command_sequence = {};
    struct sfce_string line_contents = {};
    struct sfce_piece_tree *tree = sfce_piece_tree_create(SFCE_NEWLINE_TYPE_CRLF);
    struct sfce_window_size window_size;

    error_code = sfce_piece_tree_load_file(tree, filepath);
    if (error_code != SFCE_ERROR_OK) {
        fprintf(stderr, "ERROR: unable to load file: '%s'\n", filepath);
        goto error;
    }

    // sfce_piece_node_print(tree, tree->root, 0);

    // error_code = sfce_piece_tree_insert(tree, 0, "__fewer__", 9);
    if (error_code != SFCE_ERROR_OK) {
        fprintf(stderr, "ERROR: unable to insert!\n");
        goto error;
    }

    // sfce_piece_node_print(tree, tree->root, 0);
    // sfce_piece_node_inorder_print(tree, tree->root);
    // sfce_string_resize(&line_contents, 0);
    // sfce_piece_tree_get_line_content(tree, 0, &line_contents);
    // fprintf(stderr, "line_contents: '%.*s'\n", line_contents.size, line_contents.data);

    // sfce_string_resize(&line_contents, 0);
    // sfce_piece_tree_get_line_content(tree, 1, &line_contents);
    // fprintf(stderr, "line_contents: '%.*s'\n", line_contents.size, line_contents.data);

    // sfce_string_resize(&line_contents, 0);
    // sfce_piece_tree_get_line_content(tree, 2, &line_contents);
    // fprintf(stderr, "line_contents: '%.*s'\n", line_contents.size, line_contents.data);

    // sfce_string_resize(&line_contents, 0);
    // sfce_piece_tree_get_line_content(tree, 3, &line_contents);
    // fprintf(stderr, "line_contents: '%.*s'\n", line_contents.size, line_contents.data);

    // fprintf(stderr, "================================ TREE OUTPUT ================================\n");
    // sfce_piece_node_print(tree, tree->root, 0);
    // fprintf(stderr, "================================ TREE OUTPUT ================================\n");

    for (int is_running = 1; is_running;) {
        if (_kbhit()) {
            int32_t keycode = _getch();

            switch (keycode)
            {
            case '\x1b':
                is_running = 0;
                break;
            case 'B':
                is_running = 0;
                sfce_restore_console(&console_state);
        
                fprintf(stderr, "tree->line_count: %d", (int)tree->line_count);
                fprintf(stderr, "%.*s", (int)command_sequence.size, command_sequence.data);
                break;
            default:
                if (isprint(keycode) || isspace(keycode) || keycode == '\n' || keycode == '\r') {
                    sfce_piece_tree_insert(tree, 0, (char[]){ keycode }, 1);
                }
                break;
            }
        }

        if (is_running == 0) {
            break;
        }

        error_code = sfce_get_console_screen_size(&window_size);
        if (error_code != SFCE_ERROR_OK) {
            continue;
        }

        sfce_string_clear(&command_sequence);
        sfce_string_append_formated_string(&command_sequence, "\x1b[0;0H");

        for (int32_t row = 0; row < window_size.height; ++row) {
            if (row >= tree->line_count) {
                sfce_string_append_formated_string(&command_sequence, "\x1b[%d;0H\x1b[2K", (int)row + 1);
                continue;
            }

            error_code = sfce_piece_tree_get_line_content(tree, row, &line_contents);
            if (error_code != SFCE_ERROR_OK) {
                continue;
            }

            int32_t new_size = 0;
            for (; new_size < line_contents.size; ++new_size) {
                int32_t newline_size = newline_sequence_size(&line_contents.data[new_size], line_contents.size - new_size);

                if (newline_size != 0) {
                    break;
                }
            }

            sfce_string_resize(&line_contents, new_size);

            sfce_string_append_formated_string(&command_sequence, "\x1b[%d;0H%-8d\x1b[K", (int)row + 1, (int)row + 1);
            sfce_string_append_formated_string(&command_sequence, "%.*s", (int)MIN(line_contents.size, window_size.width), line_contents.data);
        }

        sfce_string_flush(&command_sequence);
    }

    // sfce_restore_console(&console_state);

    // sfce_piece_node_inorder_print(tree, tree->root);
    sfce_restore_console(&console_state);
    sfce_string_destroy(&command_sequence);
    sfce_string_destroy(&line_contents);
    sfce_piece_tree_destroy(tree);
    return 0;

error:
    sfce_restore_console(&console_state);
    sfce_piece_tree_destroy(tree);
    fprintf(stderr, "ERROR_CODE: %d\n", error_code);
    return -1;
}

int32_t round_multiple_of_two(int32_t value, int32_t multiple)
{
    return (value + multiple - 1) & -multiple;
}

int32_t newline_sequence_size(const char *buffer, int32_t buffer_size)
{
    if (buffer_size > 0) {
        if (buffer[0] == '\r') return (buffer_size > 1 && buffer[1] == '\n') ? 2 : 1;
        if (buffer[0] == '\n') return 1;
    }

    return 0;
}

enum sfce_error_code sfce_write(const void *buffer, int32_t buffer_size)
{
#if defined(SFCE_PLATFORM_WINDOWS)
    static DWORD dummy = 0;
    if (!WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), buffer, buffer_size, &dummy, NULL)) {
        return SFCE_ERROR_FAILED_CONSOLE_WRITE;
    }
#elif defined(SFCE_PLATFORM_LINUX) || defined(SFCE_PLATFORM_MAC)
    if (write(STDOUT_FILENO, buffer, buffer_size) == -1) {
        return SFCE_ERROR_FAILED_CONSOLE_WRITE;
    }
#endif
    return SFCE_ERROR_OK;
}

enum sfce_error_code sfce_write_string(const char *buffer)
{
    return sfce_write(buffer, strlen(buffer));
}

int32_t buffer_newline_count(const char *buffer, int32_t buffer_size)
{
    int32_t newline_count = 0;

    const char *buffer_end = buffer + buffer_size;
    while (buffer < buffer_end) {
        int32_t newline_size = newline_sequence_size(buffer, buffer_end - buffer);

        if (newline_size > 0) {
            buffer += newline_size;
            ++newline_count;
            continue;
        }

        buffer += 1;
    }

    return newline_count;
}

enum sfce_error_code sfce_save_console_state(struct sfce_console_state *state)
{
    state->input_handle = GetStdHandle(STD_INPUT_HANDLE);
    if (state->input_handle == INVALID_HANDLE_VALUE || state->input_handle == NULL) {
        return SFCE_ERROR_WIN32_API_FAILED;
    }

    state->output_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (state->output_handle == INVALID_HANDLE_VALUE || state->output_handle == NULL) {
        return SFCE_ERROR_WIN32_API_FAILED;
    }

    if (!GetConsoleMode(state->input_handle, &state->input_mode)) {
        return SFCE_ERROR_WIN32_API_FAILED;
    }

    if (!GetConsoleMode(state->output_handle, &state->output_mode)) {
        return SFCE_ERROR_WIN32_API_FAILED;
    }

    return SFCE_ERROR_OK;
}

enum sfce_error_code sfce_restore_console_state(const struct sfce_console_state *state)
{
    if (!SetConsoleMode(state->output_handle, state->output_mode)) {
        return SFCE_ERROR_WIN32_API_FAILED;
    }

    if (!SetConsoleMode(state->input_handle, state->input_mode)) {
        return SFCE_ERROR_WIN32_API_FAILED;
    }

    return SFCE_ERROR_OK;
}

enum sfce_error_code sfce_enable_virtual_terminal(const struct sfce_console_state *state)
{
    DWORD new_output_mode = state->output_mode;
    new_output_mode |= ENABLE_PROCESSED_OUTPUT;
    new_output_mode &= ~ENABLE_WRAP_AT_EOL_OUTPUT;
    new_output_mode |= DISABLE_NEWLINE_AUTO_RETURN;
    new_output_mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    if (!SetConsoleMode(state->output_handle, new_output_mode)) {
        return SFCE_ERROR_WIN32_API_FAILED;
    }

    DWORD new_input_mode = state->input_mode;
    new_input_mode &= ~ENABLE_ECHO_INPUT;
    new_input_mode &= ~ENABLE_LINE_INPUT;
    new_input_mode &= ~ENABLE_PROCESSED_INPUT;
    new_input_mode |= ENABLE_VIRTUAL_TERMINAL_INPUT;
    if (!SetConsoleMode(state->input_handle, new_input_mode)) {
        return SFCE_ERROR_WIN32_API_FAILED;
    }

    return SFCE_ERROR_OK;
}

enum sfce_error_code sfce_get_console_screen_size(struct sfce_window_size *window_size)
{
    enum sfce_error_code error_code = sfce_write_string("\x1b[s\x1b[32767;32767H");
    if (error_code != SFCE_ERROR_OK) {
        return error_code;
    }

#if defined(SFCE_PLATFORM_WINDOWS)
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO cbsi = {};
    if (!GetConsoleScreenBufferInfo(handle, &cbsi)) {
        return SFCE_ERROR_FAILED_CONSOLE_READ;
    }

    window_size->width = cbsi.dwCursorPosition.X + 1;
    window_size->height = cbsi.dwCursorPosition.Y + 1;
#else
    window_size.width = 0;
    window_size.height = 0;
#endif

    error_code = sfce_write_string("\x1b[u");
    if (error_code != SFCE_ERROR_OK) {
        return error_code;
    }

    return SFCE_ERROR_OK;
}

enum sfce_error_code sfce_enable_console_temp_buffer()
{
    const char buffer[] = "\x1b[?47h\x1b[?25l\x1b[?1049h";
    return sfce_write(buffer, sizeof(buffer) - 1);
}

enum sfce_error_code sfce_disable_console_temp_buffer()
{
    const char buffer[] = "\x1b[?47l\x1b[?25h\x1b[?1049l";
    return sfce_write(buffer, sizeof(buffer) - 1);
}

enum sfce_error_code sfce_setup_console(struct sfce_console_state *state)
{
    enum sfce_error_code error_code = sfce_save_console_state(state);
    if (error_code != SFCE_ERROR_OK) {
        return error_code;
    }

    error_code = sfce_enable_virtual_terminal(state);
    if (error_code != SFCE_ERROR_OK) {
        return error_code;
    }

    error_code = sfce_enable_console_temp_buffer();
    if (error_code != SFCE_ERROR_OK) {
        return error_code;
    }

    return SFCE_ERROR_OK;
}

enum sfce_error_code sfce_restore_console(const struct sfce_console_state *state)
{
    enum sfce_error_code error_code = sfce_disable_console_temp_buffer();
    if (error_code != SFCE_ERROR_OK) {
        return error_code;
    }

    error_code = sfce_restore_console_state(state);
    if (error_code != SFCE_ERROR_OK) {
        return error_code;
    }

    return SFCE_ERROR_OK;
}

void sfce_string_destroy(struct sfce_string *string)
{
    if (string->data) free(string->data);
    *string = (struct sfce_string){};
}

enum sfce_error_code sfce_string_reserve(struct sfce_string *result, int32_t capacity)
{
    if (result->capacity >= capacity) {
        return SFCE_ERROR_OK;
    }

    void *temp = result->data;
    result->data = realloc(result->data, capacity * sizeof *result->data);
    
    if (result->data == NULL) {
        *result = (struct sfce_string){};
        free(temp);
        return SFCE_ERROR_MEMORY_ALLOCATION_FAILURE;
    }

    return SFCE_ERROR_OK;
}

enum sfce_error_code sfce_string_resize(struct sfce_string *result, int32_t size)
{
    if (size >= result->capacity) {
        int32_t new_capacity = round_multiple_of_two(size, SFCE_STRING_ALLOCATION_SIZE);

        enum sfce_error_code error_code = sfce_string_reserve(result, new_capacity);
        if (error_code != SFCE_ERROR_OK) {
            return error_code;
        }
    }

    result->size = size;
    return SFCE_ERROR_OK;
}

enum sfce_error_code sfce_string_write(struct sfce_string *string, int32_t index, const void *buffer, int32_t buffer_size)
{
    int32_t final_index = index + buffer_size;

    if (final_index > string->size) {
        enum sfce_error_code error_code = sfce_string_resize(string, final_index);
        if (error_code != SFCE_ERROR_OK) {
            return error_code;
        }
    }

    memcpy(&string->data[index], buffer, buffer_size * sizeof *buffer);
    return SFCE_ERROR_OK;
}

enum sfce_error_code sfce_string_insert(struct sfce_string *string, int32_t index, const void *buffer, int32_t buffer_size)
{
    enum sfce_error_code error_code = sfce_string_resize(string, string->size + buffer_size);
    if (error_code != SFCE_ERROR_OK) {
        return error_code;
    }

    memmove(&string->data[index + buffer_size], &string->data[index], buffer_size * sizeof *string->data);
    memcpy(&string->data[index], buffer, buffer_size * sizeof *buffer);
    return SFCE_ERROR_OK;
}

enum sfce_error_code sfce_string_push_back(struct sfce_string *string, int32_t character)
{
    const int32_t size = string->size;
    enum sfce_error_code error_code = sfce_string_resize(string, string->size + 1);
    if (error_code != SFCE_ERROR_OK) {
        return error_code;
    }

    string->data[size] = (uint8_t)character;
    return SFCE_ERROR_OK;
}

enum sfce_error_code sfce_string_push_back_buffer(struct sfce_string *string, const void *buffer_data, int32_t buffer_size)
{
    if (buffer_size < 0) {
        return SFCE_ERROR_NEGATIVE_BUFFER_SIZE;
    }
    
    const int32_t size = string->size;
    enum sfce_error_code error_code = sfce_string_resize(string, string->size + buffer_size);
    if (error_code != SFCE_ERROR_OK) {
        return error_code;
    }
    
    memcpy(&string->data[size], buffer_data, buffer_size * sizeof *buffer_data);
    return SFCE_ERROR_OK;
}

enum sfce_error_code sfce_string_append_substring(struct sfce_string *string, struct sfce_string source_string, int32_t start_index, int32_t end_index)
{
    return sfce_string_push_back_buffer(string, &source_string.data[start_index], end_index - start_index);
}

enum sfce_error_code sfce_string_load_file(struct sfce_string *string, const char *filepath)
{
    FILE *fp = fopen(filepath, "rb");
    if (fp == NULL) {
        return SFCE_ERROR_UNABLE_TO_OPEN_FILE;
    }

    struct sfce_string result = {};
    int32_t total_file_size = 0;
    for (int32_t current_chunk_size = SFCE_FILE_READ_CHUNK_SIZE; current_chunk_size >= SFCE_FILE_READ_CHUNK_SIZE;) {
        int32_t read_offset = result.size;

        enum sfce_error_code error_code = sfce_string_resize(&result, result.size + SFCE_FILE_READ_CHUNK_SIZE);
        if (error_code != SFCE_ERROR_OK) {
            *string = (struct sfce_string){};
            return error_code;
        }

        current_chunk_size = fread(result.data + read_offset, 1, SFCE_FILE_READ_CHUNK_SIZE, fp);
        total_file_size += current_chunk_size;
    }

    enum sfce_error_code error_code = sfce_string_resize(&result, total_file_size);
    if (error_code != SFCE_ERROR_OK) {
        *string = (struct sfce_string){};
        return error_code;
    }

    *string = result;
    fclose(fp);
    return SFCE_ERROR_OK;
}

enum sfce_error_code sfce_string_append_formated_string(struct sfce_string *string, const void *format, ...)
{
    enum { TEMP_BUFFER_SIZE = 4096 };

    va_list va_args;
    va_start(va_args, format);

    char temp_buffer[TEMP_BUFFER_SIZE] = {0};
    int temp_buffer_size = vsnprintf(temp_buffer, TEMP_BUFFER_SIZE, format, va_args);
    if (temp_buffer_size < 0) {
        return SFCE_ERROR_FORMATED_STRING_TOO_LARGE;
    }

    va_end(va_args);

    enum sfce_error_code error_code = sfce_string_push_back_buffer(string, temp_buffer, temp_buffer_size);
    if (error_code != SFCE_ERROR_OK) {
        return error_code;
    }

    return SFCE_ERROR_OK;
}

enum sfce_error_code sfce_string_flush(struct sfce_string *string)
{
    return sfce_write(string->data, string->size);
}

void sfce_string_clear(struct sfce_string *string)
{
    if (string != NULL) {
        string->size = 0;
    }
}

int16_t sfce_string_compare(struct sfce_string string0, struct sfce_string string1)
{
    if (string0.size > string1.size) return  1;
    if (string0.size < string1.size) return -1;

    for (int32_t idx = 0; idx < string0.size; ++idx) {
        int16_t c0 = string0.data[idx];
        int16_t c1 = string1.data[idx];
        if (c0 != c1) return c0 - c1;
    }

    return 0;
}

void sfce_line_starts_destroy(struct sfce_line_starts *lines)
{
    if (lines->offsets != NULL) {
        free(lines->offsets);
    }

    *lines = (struct sfce_line_starts) {};
}

enum sfce_error_code sfce_line_starts_reserve(struct sfce_line_starts *lines, int32_t capacity)
{
    if (lines->capacity < capacity) {
        void *temp = lines->offsets;

        lines->capacity = capacity;
        lines->offsets = realloc(lines->offsets, capacity * sizeof *lines->offsets);

        if (lines->offsets == NULL) {
            *lines = (struct sfce_line_starts) {};
            free(temp);
            return SFCE_ERROR_MEMORY_ALLOCATION_FAILURE;
        }
    }

    return SFCE_ERROR_OK;
}

enum sfce_error_code sfce_line_starts_resize(struct sfce_line_starts *lines, int32_t count)
{
    lines->count = count;
    
    if (lines->count >= lines->capacity) {
        int32_t new_capacity = round_multiple_of_two(lines->count, SFCE_LINE_STARTS_ALLOCATION_SIZE);
        return sfce_line_starts_reserve(lines, new_capacity);
    }

    return SFCE_ERROR_OK;
}

enum sfce_error_code sfce_line_starts_push_line_offset(struct sfce_line_starts *lines, int32_t offset)
{
    const int32_t index = lines->count;
    enum sfce_error_code error_code = sfce_line_starts_resize(lines, lines->count + 1);
    if (error_code != SFCE_ERROR_OK) {
        return error_code;
    }

    lines->offsets[index] = offset;
    return SFCE_ERROR_OK;
}

void sfce_string_buffer_destroy(struct sfce_string_buffer *buffer)
{
    sfce_line_starts_destroy(&buffer->line_starts);
    sfce_string_destroy(&buffer->content);
    *buffer = (struct sfce_string_buffer){};
}

enum sfce_error_code sfce_string_buffer_append_content(struct sfce_string_buffer *buffer, const char *data, int32_t size)
{
    int32_t begin_offset = buffer->content.size;
    enum sfce_error_code error_code = sfce_string_push_back_buffer(&buffer->content, data, size);
    if (error_code != SFCE_ERROR_OK) {
        return error_code;
    }

    for (int32_t offset = begin_offset; offset < buffer->content.size;) {
        int32_t newline_size = newline_sequence_size(&buffer->content.data[offset], buffer->content.size - offset);

        if (newline_size != 0) {
            offset += newline_size;

            error_code = sfce_line_starts_push_line_offset(&buffer->line_starts, offset);
            if (error_code != SFCE_ERROR_OK) {
                return error_code;
            }
        }
        else {
            offset += 1;
        }
    }

    return SFCE_ERROR_OK;
}

struct sfce_string_buffer_position sfce_string_buffer_get_end_position(struct sfce_string_buffer *buffer)
{
    struct sfce_string_buffer_position position = {};
    position.line_start_index = buffer->line_starts.count - 1;
    position.column = buffer->content.size - buffer->line_starts.offsets[position.line_start_index];
    return position;
}

struct sfce_string_buffer_position sfce_string_buffer_position_from_offset(struct sfce_string_buffer *buffer, int32_t offset)
{
    int32_t line_low_index    = 0;
    int32_t line_high_index   = buffer->line_starts.count - 1;
    int32_t line_middle_index = 0;

    while (line_low_index <= line_high_index) {
        line_middle_index = line_low_index + (line_high_index - line_low_index) / 2;

        if (line_middle_index == line_high_index) {
            break;
        }

        int32_t line_middle_offset = buffer->line_starts.offsets[line_middle_index];
        int32_t line_middle_end_offset = buffer->line_starts.offsets[line_middle_index + 1];

        if (offset < line_middle_offset) {
            line_high_index = line_middle_index - 1;
        }
        else if (offset >= line_middle_end_offset) {
            line_low_index = line_middle_index + 1;
        }
        else {
            break;
        }
    }

    int32_t line_start_offset = buffer->line_starts.offsets[line_middle_index];
    return (struct sfce_string_buffer_position) {
        .line_start_index = line_middle_index,
        .column = offset - line_start_offset,
    };
}

struct sfce_string_buffer_position sfce_string_buffer_piece_position_in_buffer(struct sfce_string_buffer *buffer, struct sfce_piece piece, int32_t offset_within_piece)
{
    int32_t line_low_index = piece.start.line_start_index;
    int32_t line_high_index = piece.end.line_start_index;
    int32_t line_middle_index = 0;

    int32_t offset = buffer->line_starts.offsets[piece.start.line_start_index] + piece.start.column + offset_within_piece;

    while (line_low_index <= line_high_index) {
        line_middle_index = line_low_index + (line_high_index - line_low_index) / 2;

        if (line_middle_index == line_high_index) {
            break;
        }

        int32_t line_middle_offset = buffer->line_starts.offsets[line_middle_index];
        int32_t line_middle_end_offset = buffer->line_starts.offsets[line_middle_index + 1];

        if (offset < line_middle_offset) {
            line_high_index = line_middle_index - 1;
        }
        else if (offset >= line_middle_end_offset) {
            line_low_index = line_middle_index + 1;
        }
        else {
            break;
        }
    }

    int32_t line_start_offset = buffer->line_starts.offsets[line_middle_index];
    return (struct sfce_string_buffer_position) {
        .line_start_index = line_middle_index,
        .column = offset - line_start_offset,
    };
}

struct sfce_string_buffer_position sfce_string_buffer_move_position_by_offset(struct sfce_string_buffer *buffer, struct sfce_string_buffer_position position, int32_t offset)
{
    int32_t line_begin, line_end;
    int32_t position_offset = sfce_string_buffer_offset_from_position(buffer, position) + offset;

    if (position_offset < 0) {
        return (struct sfce_string_buffer_position) {};
    }

    if (position_offset >= buffer->content.size) {
        return (struct sfce_string_buffer_position) {};
    }

    while (1) {
        line_begin = buffer->line_starts.offsets[position.line_start_index];

        if (position.line_start_index + 1 < buffer->line_starts.count) {
            line_end = buffer->line_starts.offsets[position.line_start_index + 1];
        }
        else {
            line_end = buffer->content.size;
        }

        position.column = position_offset - line_begin;

        if (position_offset < line_begin) {
            position.line_start_index -= 1;
            continue;
        }

        if (position_offset > line_end) {
            position.line_start_index += 1;
            continue;
        }

        break;
    }

    return position;
}

int32_t sfce_string_buffer_offset_from_position(struct sfce_string_buffer *string_buffer, struct sfce_string_buffer_position position)
{
    return string_buffer->line_starts.offsets[position.line_start_index] + position.column;
}

struct sfce_piece_pair sfce_piece_split(struct sfce_piece_tree *tree, struct sfce_piece piece, int32_t offset, int32_t gap_size)
{
    struct sfce_string_buffer *string_buffer = &tree->buffers[piece.buffer_index];
    struct sfce_string_buffer_position middle_postion0 = sfce_string_buffer_move_position_by_offset(string_buffer, piece.start, offset);
    struct sfce_string_buffer_position middle_postion1 = sfce_string_buffer_move_position_by_offset(string_buffer, middle_postion0, gap_size);
    int32_t start_offset = sfce_string_buffer_offset_from_position(string_buffer, piece.start);
    int32_t middle_offset = sfce_string_buffer_offset_from_position(string_buffer, middle_postion1);
    int32_t end_offset = sfce_string_buffer_offset_from_position(string_buffer, piece.end);

    int32_t remaining = end_offset - middle_offset;

    int32_t left_line_count = buffer_newline_count(&string_buffer->content.data[start_offset], offset);
    int32_t right_line_count = buffer_newline_count(&string_buffer->content.data[middle_offset], remaining);

    struct sfce_piece left = {
        .start = piece.start,
        .end = middle_postion0,
        .buffer_index = piece.buffer_index,
        .line_count = left_line_count,
        .length = offset,
    };

    struct sfce_piece right = {
        .start = middle_postion1,
        .end = piece.end,
        .buffer_index = piece.buffer_index,
        .line_count = right_line_count,
        .length = remaining,
    };

    if (right.length <= 0) {
        return (struct sfce_piece_pair) { .left = left };
    }

    if (left.length <= 0) {
        return (struct sfce_piece_pair) { .right = right };
    }

    return (struct sfce_piece_pair) {
        .left = left,
        .right = right,
    };
}

struct sfce_piece sfce_piece_erase_head(struct sfce_piece_tree *tree, struct sfce_piece piece, int32_t amount)
{
    struct sfce_string_buffer *string_buffer = &tree->buffers[piece.buffer_index];
    struct sfce_string_buffer_position new_start_position = sfce_string_buffer_move_position_by_offset(string_buffer, piece.start, amount);
    int32_t start_offset = sfce_string_buffer_offset_from_position(string_buffer, new_start_position);
    int32_t remaining = piece.length - amount;
    
    return (struct sfce_piece) {
        .buffer_index = piece.buffer_index,
        .start = new_start_position,
        .end = piece.end,
        .length = remaining,
        .line_count = buffer_newline_count(&string_buffer->content.data[start_offset], remaining),
    };
}

struct sfce_piece sfce_piece_erase_tail(struct sfce_piece_tree *tree, struct sfce_piece piece, int32_t amount)
{
    struct sfce_string_buffer *string_buffer = &tree->buffers[piece.buffer_index];
    struct sfce_string_buffer_position new_end_position = sfce_string_buffer_move_position_by_offset(string_buffer, piece.end, -amount);
    int32_t start_offset = sfce_string_buffer_offset_from_position(string_buffer, piece.start);
    int32_t remaining = piece.length - amount;
    
    return (struct sfce_piece) {
        .buffer_index = piece.buffer_index,
        .start = piece.start,
        .end = new_end_position,
        .length = remaining,
        .line_count = buffer_newline_count(&string_buffer->content.data[start_offset], remaining),
    };
}

struct sfce_piece_node *sfce_piece_node_create(struct sfce_piece piece)
{
    struct sfce_piece_node *tree = malloc(sizeof *tree);
    
    if (tree == NULL) {
        return NULL;
    }
    
    *tree = (struct sfce_piece_node) {
        .left = sentinel_ptr,
        .right = sentinel_ptr,
        .parent = sentinel_ptr,
        .piece = piece,
        .color = SFCE_COLOR_BLACK,
    };
    
    return tree;
}

void sfce_piece_node_destroy(struct sfce_piece_node *node)
{
    if (node != sentinel_ptr && node != NULL) {
        sfce_piece_node_destroy(node->left);
        sfce_piece_node_destroy(node->right);
        sfce_piece_node_destroy_nonrecursive(node);
    }
}

void sfce_piece_node_destroy_nonrecursive(struct sfce_piece_node *node)
{
    if (node != sentinel_ptr && node != NULL) {
        free(node);
    }
}

int32_t sfce_piece_node_calculate_length(struct sfce_piece_node *node)
{
    if (node == sentinel_ptr) {
        return 0;
    }

    int32_t right_node_length = sfce_piece_node_calculate_length(node->right);
    return node->left_subtree_length + node->piece.length + right_node_length;
}

int32_t sfce_piece_node_calculate_line_count(struct sfce_piece_node *node)
{
    if (node == sentinel_ptr) {
        return 0;
    }

    int32_t right_node_line_count = sfce_piece_node_calculate_line_count(node->right);
    return node->left_subtree_line_count + node->piece.line_count + right_node_line_count;
}

struct sfce_piece_node *sfce_piece_node_leftmost(struct sfce_piece_node *node)
{
    while (node->left != sentinel_ptr) {
        node = node->left;
    }

    return node;
}

struct sfce_piece_node *sfce_piece_node_rightmost(struct sfce_piece_node *node)
{
    while (node->right != sentinel_ptr) {
        node = node->right;
    }
    
    return node;
}

struct sfce_piece_node *sfce_piece_node_next(struct sfce_piece_node *node)
{
    if (node->right != sentinel_ptr) {
        return sfce_piece_node_leftmost(node->right);
    }

    while (node->parent != sentinel_ptr && node->parent->left != node) {
        node = node->parent;
    }

    return node->parent;
}

struct sfce_piece_node *sfce_piece_node_prev(struct sfce_piece_node *node)
{
    if (node->left != sentinel_ptr) {
        return sfce_piece_node_rightmost(node->left);
    }

    while (node->parent != sentinel_ptr && node->parent->right != node) {
        node = node->parent;
    }

    return node->parent;
}

struct sfce_piece_node *sfce_piece_node_rotate_left(struct sfce_piece_node **root, struct sfce_piece_node *x)
{
	struct sfce_piece_node *y = x->right;

    y->left_subtree_length += x->left_subtree_length + x->piece.length;
    y->left_subtree_line_count += x->left_subtree_line_count + x->piece.line_count;

	x->right = y->left;

	if (y->left != sentinel_ptr) {
		y->left->parent = x;
	}

	y->parent = x->parent;
    if (x->parent == sentinel_ptr) {
		*root = y;
	}
    else if (x->parent->left == x) {
		x->parent->left = y;
	}
    else {
		x->parent->right = y;
	}

	y->left = x;
	x->parent = y;
    return y;
}

struct sfce_piece_node *sfce_piece_node_rotate_right(struct sfce_piece_node **root, struct sfce_piece_node *y)
{
    struct sfce_piece_node *x = y->left;
    y->left = x->right;
    if (x->right != sentinel_ptr) {
        x->right->parent = y;
    }
    x->parent = y->parent;

    y->left_subtree_length -= x->left_subtree_length + x->piece.length;
    y->left_subtree_line_count -= x->left_subtree_line_count + x->piece.line_count;

    if (y->parent == sentinel_ptr) {
        *root = x;
    }
    else if (y == y->parent->right) {
        y->parent->right = x;
    }
    else {
        y->parent->left = x;
    }

    x->right = y;
    y->parent = x;
    return x;
}

struct sfce_piece_node *sfce_piece_node_insert_left(struct sfce_piece_node **root, struct sfce_piece_node *where, struct sfce_piece_node *node_to_insert)
{
    if (*root == sentinel_ptr) {
        *root = node_to_insert;
        node_to_insert->color = SFCE_COLOR_BLACK;
    }
    else if (where->left == sentinel_ptr) {
        where->left = node_to_insert;
        node_to_insert->parent = where;
    }
    else {
        struct sfce_piece_node *prev_node = sfce_piece_node_rightmost(where->left);
        prev_node->right = node_to_insert;
        node_to_insert->parent = prev_node;
    }

    sfce_piece_node_fix_insert_violation(root, node_to_insert);
    return node_to_insert;
}

struct sfce_piece_node *sfce_piece_node_insert_right(struct sfce_piece_node **root, struct sfce_piece_node *where, struct sfce_piece_node *node_to_insert)
{
    if (*root == sentinel_ptr) {
        *root = node_to_insert;
        node_to_insert->color = SFCE_COLOR_BLACK;
    }
    else if (where->right == sentinel_ptr) {
        where->right = node_to_insert;
        node_to_insert->parent = where;
    }
    else {
        struct sfce_piece_node *next_node = sfce_piece_node_leftmost(where->right);
        next_node->left = node_to_insert;
        node_to_insert->parent = next_node;
    }

    sfce_piece_node_fix_insert_violation(root, node_to_insert);
    return node_to_insert;
}

void sfce_piece_node_remove_node(struct sfce_piece_node **root, struct sfce_piece_node *node_to_remove)
{
    if (node_to_remove == sentinel_ptr) {
        return;
    }

    enum sfce_red_black_color original_color = node_to_remove->color;
    struct sfce_piece_node *y = sentinel_ptr;
    struct sfce_piece_node *x = sentinel_ptr;

    if (node_to_remove->left == sentinel_ptr) {
        x = node_to_remove->right;
        sfce_piece_node_transplant(root, node_to_remove, x);
        sfce_piece_node_recompute_metadata(root, x);
    }
    else if (node_to_remove->right == sentinel_ptr) {
        x = node_to_remove->left;
        sfce_piece_node_transplant(root, node_to_remove, x);
        sfce_piece_node_recompute_metadata(root, x);
    }
    else {
        y = sfce_piece_node_leftmost(node_to_remove->right);
        original_color = y->color;

        x = y->right;

        if (y->parent == node_to_remove) {
            x->parent = y;
            sfce_piece_node_recompute_metadata(root, x);
        }
        else {
            sfce_piece_node_transplant(root, y, y->right);
            y->right = node_to_remove->right;
            y->right->parent = y;

            sfce_piece_node_recompute_metadata(root, y);
        }

        sfce_piece_node_transplant(root, node_to_remove, y);
        y->left = node_to_remove->left;
        y->left->parent = y;
        y->color = node_to_remove->color;

        sfce_piece_node_recompute_metadata(root, y);
    }

    sfce_piece_node_reset_sentinel();
    if (original_color == SFCE_COLOR_BLACK) {
        sfce_piece_node_fix_remove_violation(root, x);
    }
    
    sfce_piece_node_destroy_nonrecursive(node_to_remove);
}

void sfce_piece_node_transplant(struct sfce_piece_node **root, struct sfce_piece_node *where, struct sfce_piece_node *node_to_transplant)
{
    if (where == *root) {
        *root = node_to_transplant;
    }
    else if (where == where->parent->left) {
        where->parent->left = node_to_transplant;
    }
    else if (where == where->parent->right) {
        where->parent->right = node_to_transplant;
    }

    if (node_to_transplant != sentinel_ptr) {
        node_to_transplant->parent = where->parent;
    }
}

void sfce_piece_node_update_metadata(struct sfce_piece_node **root, struct sfce_piece_node *node, int32_t delta_length, int32_t delta_line_count)
{
    if (delta_length == 0 && delta_line_count == 0) {
        return;
    }

    node->left_subtree_length += delta_length;
    node->left_subtree_line_count += delta_line_count;

    while (node != *root) {
        if (node->parent->left == node) {
            node->parent->left_subtree_length += delta_length;
            node->parent->left_subtree_line_count += delta_line_count;
        }

        node = node->parent;
    }
}

void sfce_piece_node_recompute_metadata(struct sfce_piece_node **root, struct sfce_piece_node *node)
{
    if (node == *root || node == sentinel_ptr) {
        return;
    }

    while (node != sentinel_ptr && node == node->parent->right) {
        node = node->parent;
    }

    if (node == *root || node == sentinel_ptr) {
        return;
    }

    node = node->parent;

    int32_t left_length = 0;
    int32_t left_line_count = 0;
    for (struct sfce_piece_node *current = node->left; current != sentinel_ptr; current = current->right) {
        left_length += current->left_subtree_length + current->piece.length;
        left_line_count += current->left_subtree_line_count + current->piece.line_count;
    }

    int32_t delta_length = left_length - node->left_subtree_length;
    int32_t delta_line_count = left_line_count - node->left_subtree_line_count;
    sfce_piece_node_update_metadata(root, node, delta_length, delta_line_count);
}

void sfce_piece_node_fix_insert_violation(struct sfce_piece_node **root, struct sfce_piece_node *node)
{
    sfce_piece_node_recompute_metadata(root, node);

    node->color = SFCE_COLOR_RED;
    while (node != *root && node->parent->color == SFCE_COLOR_RED) {
        if (node->parent->parent->left == node->parent) {
            struct sfce_piece_node *uncle = node->parent->parent->right;

            if (uncle->color == SFCE_COLOR_RED) {
                uncle->color = SFCE_COLOR_BLACK;
                node->parent->color = SFCE_COLOR_BLACK;
                node->parent->parent->color = SFCE_COLOR_RED;
                node = node->parent->parent;
            }
            else {
                if (node->parent->right == node) {
                    node = node->parent;
                    sfce_piece_node_rotate_left(root, node);
                }

                node->parent->color = SFCE_COLOR_BLACK;
                node->parent->parent->color = SFCE_COLOR_RED;
                sfce_piece_node_rotate_right(root, node->parent->parent);
            }
        }
        else {
            struct sfce_piece_node *uncle = node->parent->parent->left;

            if (uncle->color == SFCE_COLOR_RED) {
                uncle->color = SFCE_COLOR_BLACK;
                node->parent->color = SFCE_COLOR_BLACK;
                node->parent->parent->color = SFCE_COLOR_RED;
                node = node->parent->parent;
            }
            else {
                if (node->parent->left == node) {
                    node = node->parent;
                    sfce_piece_node_rotate_right(root, node);
                }

                node->parent->color = SFCE_COLOR_BLACK;
                node->parent->parent->color = SFCE_COLOR_RED;
                sfce_piece_node_rotate_left(root, node->parent->parent);
            }
        }
	}

    (*root)->color = SFCE_COLOR_BLACK;
    sfce_piece_node_reset_sentinel();
}

void sfce_piece_node_fix_remove_violation(struct sfce_piece_node **root, struct sfce_piece_node *x)
{
    struct sfce_piece_node *s;
    x->color = SFCE_COLOR_BLACK;
    while (x != *root && x->color == SFCE_COLOR_BLACK) {
        if (x == x->parent->left) {
            s = x->parent->right;
            if (s->color == SFCE_COLOR_RED) {
                s->color = SFCE_COLOR_BLACK;
                x->parent->color = SFCE_COLOR_RED;
                sfce_piece_node_rotate_left(root, x->parent);
                s = x->parent->right;
            }

            if (s->left->color == SFCE_COLOR_BLACK && s->right->color == SFCE_COLOR_BLACK) {
                s->color = SFCE_COLOR_RED;
                x = x->parent;
            }
            else {
                if (s->right->color == SFCE_COLOR_BLACK) {
                    s->left->color = SFCE_COLOR_BLACK;
                    s->color = SFCE_COLOR_RED;
                    sfce_piece_node_rotate_right(root, s);
                    s = x->parent->right;
                }

                s->color = x->parent->color;
                x->parent->color = SFCE_COLOR_BLACK;
                s->right->color = SFCE_COLOR_BLACK;
                sfce_piece_node_rotate_left(root, x->parent);
                x = *root;
            }
        }
        else {
            s = x->parent->left;
            if (s->color == SFCE_COLOR_RED) {
                s->color = SFCE_COLOR_BLACK;
                x->parent->color = SFCE_COLOR_RED;
                sfce_piece_node_rotate_right(root, x->parent);
                s = x->parent->left;
            }

            if (s->right->color == SFCE_COLOR_BLACK && s->right->color == SFCE_COLOR_BLACK) {
                s->color = SFCE_COLOR_RED;
                x = x->parent;
            }
            else {
                if (s->left->color == SFCE_COLOR_BLACK) {
                    s->right->color = SFCE_COLOR_BLACK;
                    s->color = SFCE_COLOR_RED;
                    sfce_piece_node_rotate_left(root, s);
                    s = x->parent->left;
                }

                s->color = x->parent->color;
                x->parent->color = SFCE_COLOR_BLACK;
                s->left->color = SFCE_COLOR_BLACK;
                sfce_piece_node_rotate_right(root, x->parent);
                x = *root;
            }
        }
    }

    x->color = SFCE_COLOR_BLACK;
    sfce_piece_node_reset_sentinel();
}

void sfce_piece_node_inorder_print(struct sfce_piece_tree *tree, struct sfce_piece_node *root)
{
    if (root == sentinel_ptr) {
        return;
    }

    sfce_piece_node_inorder_print(tree, root->left);

    struct sfce_piece piece = root->piece;
    struct sfce_string_buffer *buffer = &tree->buffers[piece.buffer_index];
    int32_t start_offset = sfce_string_buffer_offset_from_position(buffer, piece.start);
    int32_t end_offset = sfce_string_buffer_offset_from_position(buffer, piece.end);

    printf("%.*s", end_offset - start_offset, &buffer->content.data[start_offset]);

    sfce_piece_node_inorder_print(tree, root->right);
}

void sfce_piece_node_print(struct sfce_piece_tree *tree, struct sfce_piece_node *root, uint32_t space)
{
    static const char *const node_color_list[] = {
        [SFCE_COLOR_BLACK] = "BLACK",
        [SFCE_COLOR_RED] = "RED",
    };

    enum { COUNT = 4 };

    if (root == sentinel_ptr) {
        return;
    }

    sfce_piece_node_print(tree, root->right, space + COUNT);

    struct sfce_string_view piece_content = sfce_piece_tree_get_piece_content(tree, root->piece);
    fprintf(stderr, "%*snode(%s): '%.*s' \n", space, "", node_color_list[root->color], (int)piece_content.size, piece_content.data);

    sfce_piece_node_print(tree, root->left, space + COUNT);
}

void sfce_piece_node_reset_sentinel()
{
    sentinel = (struct sfce_piece_node){
        .parent = &sentinel,
        .left   = &sentinel,
        .right  = &sentinel,
        .color  = SFCE_COLOR_BLACK,
    };
}

struct sfce_piece_tree *sfce_piece_tree_create(enum sfce_newline_type newline_type)
{
    struct sfce_piece_tree *tree = malloc(sizeof *tree);

    if (tree != NULL) {
        *tree = (struct sfce_piece_tree) {
            .root = sentinel_ptr,
            .newline_type = newline_type,
        };

        enum sfce_error_code error_code = sfce_piece_tree_add_new_string_buffer(tree);

        if (error_code != SFCE_ERROR_OK) {
            sfce_piece_tree_destroy(tree);
            return NULL;
        }
    }

    return tree;
}

void sfce_piece_tree_destroy(struct sfce_piece_tree *tree)
{
    if (tree->root != NULL) {
        sfce_piece_node_destroy(tree->root);
    }

    if (tree->buffers != NULL) {
        for (int32_t idx = 0; idx < tree->buffer_count; ++idx) {
            sfce_string_buffer_destroy(tree->buffers);
        }

        free(tree->buffers);
    }

    free(tree);
}

int32_t sfce_piece_tree_line_number_offset_within_piece(struct sfce_piece_tree *tree, struct sfce_piece piece, int32_t lines_within_piece)
{
    struct sfce_line_starts *line_starts = &tree->buffers[piece.buffer_index].line_starts;
    int32_t line_number_within_buffer = piece.start.line_start_index + lines_within_piece;

    if (line_number_within_buffer <= piece.start.line_start_index) {
        return 0;
    }

    int32_t start_offset = line_starts->offsets[piece.start.line_start_index] + piece.start.column;
    if (line_number_within_buffer > piece.end.line_start_index) {
        int32_t end_offset = line_starts->offsets[piece.end.line_start_index] + piece.end.column;
        return end_offset - start_offset;
    }

    return line_starts->offsets[line_number_within_buffer] - start_offset;
}

struct sfce_string_view sfce_piece_tree_get_piece_content(struct sfce_piece_tree *tree, struct sfce_piece piece)
{
    struct sfce_string_buffer *string_buffer = &tree->buffers[piece.buffer_index];
    int32_t offset0 = sfce_string_buffer_offset_from_position(string_buffer, piece.start);
    int32_t offset1 = sfce_string_buffer_offset_from_position(string_buffer, piece.end);

    return (struct sfce_string_view) {
        .data = &string_buffer->content.data[offset0],
        .size = offset1 - offset0,
    };
}

struct sfce_node_position sfce_piece_tree_node_at_offset(struct sfce_piece_tree *tree, int32_t offset)
{
    struct sfce_piece_node *node = tree->root;
    int32_t node_start_offset = 0;
    int32_t node_start_line_count = 0;
    int32_t subtree_offset = offset;

    while (node != sentinel_ptr) {
        if (subtree_offset < node->left_subtree_length) {
            node = node->left;
        }
        else if (subtree_offset > node->left_subtree_length + node->piece.length) {
            node_start_offset += node->left_subtree_length + node->piece.length;
            node_start_line_count += node->left_subtree_line_count + node->piece.line_count;
            subtree_offset -= node->left_subtree_length + node->piece.length;
            node = node->right;
        }
        else {
            node_start_offset += node->left_subtree_length;
            node_start_line_count += node->left_subtree_line_count;
            return (struct sfce_node_position) {
                .node = node,
                .node_start_offset = node_start_offset,
                // .node_start_line_number = node_start_line_count,
                .offset_within_piece = offset - node_start_offset,
            };
        }
    }

    return (struct sfce_node_position) { .node = sentinel_ptr };
}

struct sfce_node_position sfce_piece_tree_get_node_position_next_line(struct sfce_piece_tree *tree, struct sfce_node_position position)
{
    while (position.node != sentinel_ptr) {
        if (position.node->piece.line_count > 0) {
            struct sfce_string_buffer *string_buffer = &tree->buffers[position.node->piece.buffer_index];
            int32_t piece_start_offset = sfce_string_buffer_offset_from_position(string_buffer, position.node->piece.start);
            int32_t line_start_index = sfce_string_buffer_piece_position_in_buffer(string_buffer, position.node->piece, position.offset_within_piece).line_start_index + 1;

            if (line_start_index <= position.node->piece.end.line_start_index) {
                position.offset_within_piece = string_buffer->line_starts.offsets[line_start_index] - piece_start_offset;
                return position;
            }
        }

        position.node_start_offset += position.node->piece.length;
        position.node = sfce_piece_node_next(position.node);
    }

    return (struct sfce_node_position) { .node = sentinel_ptr };
}

struct sfce_node_position sfce_piece_tree_node_at_row_and_col(struct sfce_piece_tree *tree, int32_t row, int32_t col)
{
    struct sfce_piece_node *node = tree->root;
    int32_t node_start_offset = 0;
    int32_t subtree_line_count = row;

    while (node != sentinel_ptr) {
        if (node->left != sentinel_ptr && subtree_line_count <= node->left_subtree_line_count) {
            node = node->left;
        }
        else if (subtree_line_count > node->left_subtree_line_count + node->piece.line_count) {
            node_start_offset += node->left_subtree_length + node->piece.length;
            subtree_line_count -= node->left_subtree_line_count + node->piece.line_count;
            node = node->right;
        }
        else {
            // if (subtree_line_count != node->left_subtree_line_count + node->piece.line_count) {
            // }
            node_start_offset += node->left_subtree_length;

            int32_t lines_within_piece = subtree_line_count - node->left_subtree_line_count;
            int32_t line_offset0 = sfce_piece_tree_line_number_offset_within_piece(tree, node->piece, lines_within_piece);
            int32_t line_offset1 = sfce_piece_tree_line_number_offset_within_piece(tree, node->piece, lines_within_piece + 1);
            int32_t line_length = line_offset1 - line_offset0;

            return (struct sfce_node_position) {
                .node = node,
                .node_start_offset = node_start_offset,
                .offset_within_piece = line_offset0 + MIN(col, line_length),
            };
        }
    }

    return (struct sfce_node_position) { .node = sentinel_ptr };
}

enum sfce_error_code sfce_piece_tree_get_content_between_node_positions(struct sfce_piece_tree *tree, struct sfce_node_position start, struct sfce_node_position end, struct sfce_string *string)
{
    enum sfce_error_code error_code;
    sfce_string_clear(string);

    if (start.node == end.node) {
        int32_t byte_count = end.offset_within_piece - start.offset_within_piece;
        struct sfce_string_view piece_content = sfce_piece_tree_get_piece_content(tree, start.node->piece);
        return sfce_string_push_back_buffer(string, &piece_content.data[start.offset_within_piece], byte_count);
    }

    struct sfce_string_view start_piece_content = sfce_piece_tree_get_piece_content(tree, start.node->piece);
    error_code = sfce_string_push_back_buffer(string, &start_piece_content.data[start.offset_within_piece], start.node->piece.length - start.offset_within_piece);
    if (error_code != SFCE_ERROR_OK) {
        return error_code;
    }

    for (struct sfce_piece_node *node = sfce_piece_node_next(start.node); node != end.node && node != sentinel_ptr;) {
        struct sfce_string_view piece_content = sfce_piece_tree_get_piece_content(tree, node->piece);

        error_code = sfce_string_push_back_buffer(string, piece_content.data, piece_content.size);
        if (error_code != SFCE_ERROR_OK) {
            return error_code;
        }

        node = sfce_piece_node_next(node);
    }

    struct sfce_string_view end_piece_content = sfce_piece_tree_get_piece_content(tree, end.node->piece);
    return sfce_string_push_back_buffer(string, &end_piece_content.data[0], end.offset_within_piece);
}

enum sfce_error_code sfce_piece_tree_ensure_change_buffer_size(struct sfce_piece_tree *tree, int32_t required_size)
{
    struct sfce_string_buffer *string_buffer = &tree->buffers[tree->change_buffer_index];
    int32_t remaining_size = SFCE_STRING_BUFFER_SIZE_THRESHOLD - string_buffer->content.size;

    if (remaining_size < required_size) {
        tree->change_buffer_index = tree->buffer_count;
        enum sfce_error_code error_code = sfce_piece_tree_add_new_string_buffer(tree);

        if (error_code != SFCE_ERROR_OK) {
            return error_code;
        }
    }

    return SFCE_ERROR_OK;
}

enum sfce_error_code sfce_piece_tree_set_buffer_count(struct sfce_piece_tree *tree, int32_t buffer_count)
{
    if (buffer_count >= tree->buffer_capacity) {
        void *temp_ptr = tree->buffers;
        tree->buffer_capacity = round_multiple_of_two(buffer_count, SFCE_STRING_BUFFER_ALLOCATION_SIZE);
        tree->buffers = realloc(tree->buffers, tree->buffer_capacity * sizeof *tree->buffers);

        if (tree->buffers == NULL) {
            sfce_piece_tree_destroy(tree);
            free(temp_ptr);
            return SFCE_ERROR_MEMORY_ALLOCATION_FAILURE;
        }
    }

    tree->buffer_count = buffer_count;
    return SFCE_ERROR_OK;
}

enum sfce_error_code sfce_piece_tree_add_new_string_buffer(struct sfce_piece_tree *tree)
{
    enum sfce_error_code error_code = sfce_piece_tree_set_buffer_count(tree, tree->buffer_count + 1);
    if (error_code != SFCE_ERROR_OK) {
        return error_code;
    }

    struct sfce_string_buffer new_buffer = {};
    error_code = sfce_line_starts_push_line_offset(&new_buffer.line_starts, 0);
    if (error_code != SFCE_ERROR_OK) {
        return error_code;
    }

    tree->buffers[tree->buffer_count - 1] = new_buffer;
    return SFCE_ERROR_OK;
}

enum sfce_error_code sfce_piece_tree_create_node_subtree(struct sfce_piece_tree *tree, const char *buffer, int32_t buffer_size, struct sfce_piece_node **result)
{
    enum sfce_error_code error_code;
    struct sfce_piece_node *root = sentinel_ptr;
    struct sfce_piece_node *rightmost_node = sentinel_ptr;

    const char *buffer_end = buffer + buffer_size;

    while (buffer < buffer_end) {
        int32_t remaining = buffer_end - buffer;
        int32_t chunk_size = MIN(remaining, SFCE_STRING_BUFFER_SIZE_THRESHOLD);

        struct sfce_piece piece;
        error_code = sfce_piece_tree_create_piece(tree, buffer, chunk_size, &piece);
        if (error_code != SFCE_ERROR_OK) {
            sfce_piece_node_destroy(root);
            return error_code;
        }

        struct sfce_piece_node *new_node = sfce_piece_node_create(piece);
        
        if (new_node == NULL) {
            sfce_piece_node_destroy(root);
            return SFCE_ERROR_MEMORY_ALLOCATION_FAILURE;
        }

        sfce_piece_node_insert_right(&root, rightmost_node, new_node);
        rightmost_node = new_node;
        buffer += chunk_size;
    }

    *result = root;
    return SFCE_ERROR_OK;
}

enum sfce_error_code sfce_piece_tree_create_piece(struct sfce_piece_tree *tree, const char *data, int32_t byte_count, struct sfce_piece *result_piece)
{
    enum sfce_error_code error_code = sfce_piece_tree_ensure_change_buffer_size(tree, byte_count);
    if (error_code != SFCE_ERROR_OK) {
        return error_code;
    }

    struct sfce_string_buffer *string_buffer = &tree->buffers[tree->change_buffer_index];
    struct sfce_string_buffer_position start_position = sfce_string_buffer_get_end_position(string_buffer);

    error_code = sfce_string_buffer_append_content(string_buffer, data, byte_count);
    if (error_code != SFCE_ERROR_OK) {
        return error_code;
    }

    struct sfce_string_buffer_position end_position = sfce_string_buffer_get_end_position(string_buffer);
    int32_t line_count = buffer_newline_count(data, byte_count);

    *result_piece = (struct sfce_piece) {
        .buffer_index = tree->change_buffer_index,
        .start = start_position,
        .end = end_position,
        .line_count = line_count,
        .length = byte_count,
    };

    return SFCE_ERROR_OK;
}

enum sfce_error_code sfce_piece_tree_insert(struct sfce_piece_tree *tree, int32_t offset, const char *data, int32_t byte_count)
{
    struct sfce_node_position where = sfce_piece_tree_node_at_offset(tree, offset);
    return sfce_piece_tree_insert_with_position(tree, where, data, byte_count);
}

enum sfce_error_code sfce_piece_tree_erase(struct sfce_piece_tree *tree, int32_t offset, int32_t byte_count)
{
    if (byte_count == 0) {
        return SFCE_ERROR_OK;
    }

    struct sfce_node_position start = sfce_piece_tree_node_at_offset(tree, offset);
    struct sfce_node_position end = sfce_piece_tree_node_at_offset(tree, offset + byte_count);
    return sfce_piece_tree_erase_with_position(tree, start, end);
}

enum sfce_error_code sfce_piece_tree_insert_with_position(struct sfce_piece_tree *tree, struct sfce_node_position where, const char *data, int32_t byte_count)
{
    if (where.node == sentinel_ptr && tree->root != sentinel_ptr) {
        return SFCE_ERROR_BAD_INSERTION;
    }

    struct sfce_piece_node *subtree = sentinel_ptr;
    enum sfce_error_code error_code = sfce_piece_tree_create_node_subtree(tree, data, byte_count, &subtree);
    if (error_code != SFCE_ERROR_OK) {
        return error_code;
    }

    if (tree->root == sentinel_ptr) {
        tree->root = subtree;
        tree->root->color = SFCE_COLOR_BLACK;
        return SFCE_ERROR_OK;
    }

    if (where.offset_within_piece == 0) {
        sfce_piece_node_insert_left(&tree->root, where.node, subtree);
    }
    else if (where.offset_within_piece >= where.node->piece.length) {
        sfce_piece_node_insert_right(&tree->root, where.node, subtree);
    }
    else {
        struct sfce_piece_pair split_pieces = sfce_piece_split(tree, where.node->piece, where.offset_within_piece, 0);
        struct sfce_piece_node *right_node = sfce_piece_node_create(split_pieces.right);

        if (right_node == NULL) {
            sfce_piece_node_destroy(subtree);
            return SFCE_ERROR_MEMORY_ALLOCATION_FAILURE;
        }

        where.node->piece = split_pieces.left;
        sfce_piece_node_recompute_metadata(&tree->root, where.node);

        sfce_piece_node_insert_right(&tree->root, where.node, right_node);
        sfce_piece_node_insert_right(&tree->root, where.node, subtree);
    }

    sfce_piece_tree_recompute_metadata(tree);
    return SFCE_ERROR_OK;
}

enum sfce_error_code sfce_piece_tree_erase_with_position(struct sfce_piece_tree *tree, struct sfce_node_position start, struct sfce_node_position end)
{
    if (start.node == end.node) {
        int32_t byte_count = end.offset_within_piece - start.offset_within_piece;

        if (byte_count == 0) {
            return SFCE_ERROR_OK;
        }

        struct sfce_piece_node *node = start.node;
        struct sfce_piece_pair split_pieces = sfce_piece_split(tree, node->piece, start.offset_within_piece, byte_count);

        if (split_pieces.left.length == 0) {
            node->piece = split_pieces.right;
            sfce_piece_node_recompute_metadata(&tree->root, node);
        }
        else if (split_pieces.right.length == 0) {
            node->piece = split_pieces.left;
            sfce_piece_node_recompute_metadata(&tree->root, node);
        }
        else {
            node->piece = split_pieces.left;
            sfce_piece_node_recompute_metadata(&tree->root, node);
            sfce_piece_node_insert_right(&tree->root, node, sfce_piece_node_create(split_pieces.right));
        }
    }
    else {
        for (struct sfce_piece_node *node = sfce_piece_node_next(start.node); node != end.node && node != sentinel_ptr;) {
            struct sfce_piece_node *temp = sfce_piece_node_next(node);
            sfce_piece_node_remove_node(&tree->root, node);
            node = temp;
        }

        start.node->piece = sfce_piece_erase_tail(tree, start.node->piece, start.node->piece.length - start.offset_within_piece);
        if (start.node->piece.length == 0) {
            sfce_piece_node_remove_node(&tree->root, start.node);
        }
        else {
            sfce_piece_node_recompute_metadata(&tree->root, start.node);
        }

        end.node->piece = sfce_piece_erase_head(tree, end.node->piece, end.offset_within_piece);
        if (end.node->piece.length == 0) {
            sfce_piece_node_remove_node(&tree->root, end.node);
        }
        else {
            sfce_piece_node_recompute_metadata(&tree->root, end.node);
        }
    }

    sfce_piece_tree_recompute_metadata(tree);
    return SFCE_ERROR_OK;
}

enum sfce_error_code sfce_piece_tree_load_file(struct sfce_piece_tree *tree, const char *filepath)
{
    FILE *fp = fopen(filepath, "rb+");

    if (fp == NULL) {
        return SFCE_ERROR_UNABLE_TO_OPEN_FILE;
    }

    char buffer[SFCE_STRING_BUFFER_SIZE_THRESHOLD] = {};
    int32_t chunk_size = 0;

    enum sfce_error_code error_code;
    struct sfce_piece_node *rightmost_node = sfce_piece_node_rightmost(tree->root);
    while (chunk_size = fread(buffer, 1, SFCE_STRING_BUFFER_SIZE_THRESHOLD, fp), chunk_size != 0) {
        struct sfce_piece piece = {};
        error_code = sfce_piece_tree_create_piece(tree, buffer, chunk_size, &piece);
        if (error_code != SFCE_ERROR_OK) {
            return error_code;
        }

        struct sfce_piece_node *node = sfce_piece_node_create(piece);
        
        if (node == NULL) {
            return SFCE_ERROR_MEMORY_ALLOCATION_FAILURE;
        }

        sfce_piece_node_insert_right(&tree->root, rightmost_node, node);
        rightmost_node = node;
    }

    sfce_piece_tree_recompute_metadata(tree);
    fclose(fp);
    return SFCE_ERROR_OK;
}

enum sfce_error_code sfce_piece_tree_get_line_content(struct sfce_piece_tree *tree, int32_t row, struct sfce_string *string)
{
    struct sfce_node_position position0 = sfce_piece_tree_node_at_row_and_col(tree, row, 0);
    struct sfce_node_position position1 = sfce_piece_tree_get_node_position_next_line(tree, position0);
    return sfce_piece_tree_get_content_between_node_positions(tree, position0, position1, string);
}

enum sfce_error_code sfce_piece_tree_create_snapshot(struct sfce_piece_tree *tree, struct sfce_piece_tree_snapshot *snapshot)
{
    enum sfce_error_code error_code;

    struct sfce_piece_node *node = sfce_piece_node_leftmost(tree->root);
    while (node != sentinel_ptr) {
        error_code = sfce_piece_tree_snapshot_add_piece(snapshot, node->piece);
        if (error_code != SFCE_ERROR_OK) {
            return error_code;
        }

        node = sfce_piece_node_next(node);
    }

    return SFCE_ERROR_OK;
}

void sfce_piece_tree_recompute_metadata(struct sfce_piece_tree *tree)
{
    tree->length = 0;
    tree->line_count = 1;
    
    for (struct sfce_piece_node *node = tree->root; node != sentinel_ptr; node = node->right) {
        tree->length += node->left_subtree_length + node->piece.length;
        tree->line_count += node->left_subtree_line_count + node->piece.line_count;
    }
}

enum sfce_error_code sfce_piece_tree_snapshot_set_piece_count(struct sfce_piece_tree_snapshot *snapshot, int32_t count)
{
    snapshot->piece_count = count;

    if (snapshot->piece_count >= snapshot->piece_capacity) {
        void *temp = snapshot->pieces;
        snapshot->piece_capacity = round_multiple_of_two(snapshot->piece_count, SFCE_SNAPSHOT_ALLOCATION_SIZE);
        snapshot->pieces = realloc(snapshot->pieces, snapshot->piece_capacity * sizeof *snapshot->pieces);

        if (snapshot->pieces == NULL) {
            free(temp);
            return SFCE_ERROR_MEMORY_ALLOCATION_FAILURE;
        }
    }

    return SFCE_ERROR_OK;
}

enum sfce_error_code sfce_piece_tree_snapshot_add_piece(struct sfce_piece_tree_snapshot *snapshot, struct sfce_piece piece)
{
    enum sfce_error_code error_code = sfce_piece_tree_snapshot_set_piece_count(snapshot, snapshot->piece_count + 1);
    if (error_code != SFCE_ERROR_OK) {
        return error_code;
    }

    snapshot->pieces[snapshot->piece_count - 1] = piece;
    return SFCE_ERROR_OK;
}

enum sfce_boolean sfce_console_cell_attributes_equal(enum sfce_console_color_mode color_mode, union sfce_console_cell *cell0, union sfce_console_cell *cell1)
{
    if (cell0->character_style != cell1->character_style) {
        return SFCE_FALSE;
    }

    switch (color_mode)
    {
    case SFCE_CONSOLE_COLOR_MODE_COLOR16:
    case SFCE_CONSOLE_COLOR_MODE_COLOR256:
        if (cell0->index.foreground_color   != cell1->index.foreground_color) return SFCE_FALSE;
        if (cell0->index.background_color   != cell1->index.background_color) return SFCE_FALSE;
        return SFCE_TRUE;
    case SFCE_CONSOLE_COLOR_MODE_TRUE_COLOR:
        if (cell0->rgb.foreground_red   != cell1->rgb.foreground_red) return SFCE_FALSE;
        if (cell0->rgb.foreground_green != cell1->rgb.foreground_green) return SFCE_FALSE;
        if (cell0->rgb.foreground_blue  != cell1->rgb.foreground_blue) return SFCE_FALSE;
        if (cell0->rgb.background_red   != cell1->rgb.background_red) return SFCE_FALSE;
        if (cell0->rgb.background_green != cell1->rgb.background_green) return SFCE_FALSE;
        if (cell0->rgb.background_blue  != cell1->rgb.background_blue) return SFCE_FALSE;
        return SFCE_TRUE;
    }

    return SFCE_TRUE;
}

void sfce_console_buffer_set_attributes(struct sfce_console_buffer *buffer, union sfce_console_cell *current_cell)
{
    struct sfce_string *command_sequence = &buffer->temp_command_sequence;
    switch (buffer->color_mode)
    {
    case SFCE_CONSOLE_COLOR_MODE_COLOR16:
    case SFCE_CONSOLE_COLOR_MODE_COLOR256:
        sfce_string_append_formated_string(
            command_sequence,
            "\x1b[%d;%d;%dm",
            current_cell->index.character_style,
            current_cell->index.foreground_color,
            current_cell->index.background_color
        );
        break;
    case SFCE_CONSOLE_COLOR_MODE_TRUE_COLOR:
        sfce_string_append_formated_string(
            command_sequence,
            "\x1b[%dm\x1b[38;2;%d;%d;%dm\x1b[48;2;%d;%d;%dm",
            current_cell->index.character_style,
            current_cell->rgb.foreground_red,
            current_cell->rgb.foreground_green,
            current_cell->rgb.foreground_blue,
            current_cell->rgb.background_red,
            current_cell->rgb.background_green,
            current_cell->rgb.background_blue
        );

        break;
    }
}

void sfce_console_buffer_flush(struct sfce_console_buffer *buffer)
{
    if (buffer->width == 0 && buffer->height == 0) {
        return;
    }

    struct sfce_string *command_sequence = &buffer->temp_command_sequence;

    sfce_string_resize(command_sequence, 0);
    sfce_string_append_formated_string(command_sequence, "\x1b[H");

    int32_t cell_index = 0;
    union sfce_console_cell prev_cell = buffer->cells[0];
    for (int32_t row = 0; row < buffer->height; ++row) {
        sfce_string_append_formated_string(command_sequence, "\x1b[%d;0H", (int)row + 1);

        for (int32_t col = 0; col < buffer->width; ++col) {
            union sfce_console_cell current_cell = buffer->cells[cell_index++];

            if (!sfce_console_cell_attributes_equal(buffer->color_mode, &prev_cell, &current_cell)) {
                sfce_console_buffer_set_attributes(buffer, &current_cell);
            }

            sfce_string_push_back(command_sequence, current_cell.character);
            prev_cell = current_cell;
        }
    }

    sfce_string_flush(command_sequence);
}

void sfce_console_buffer_destroy(struct sfce_console_buffer *buffer)
{
    if (buffer->cells != NULL) {
        free(buffer->cells);
    }

    sfce_string_destroy(&buffer->temp_command_sequence);
    *buffer = (struct sfce_console_buffer) {};
}
