// https://code.visualstudio.com/blogs/2018/03/23/text-buffer-reimplementation
// https://github.com/microsoft/vscode/tree/3cf67889583203811c81ca34bea2ad02d7c902db/src/vs/editor/common/model/pieceTreeTextBuffer
// https://github.com/microsoft/vscode-textbuffer
// gcc main.c -O3 -Werror -Wfatal-errors -Wall -o bin/sfce

#include <stdint.h>
#include <stdio.h>
#include <assert.h>

#include <windows.h>
#include <wincon.h>
#include <conio.h>

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

enum sfce_error_code {
    SFCE_ERROR_OK,
    SFCE_ERROR_BAD,
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
    SFCE_ERROR_FORMATED_STRING_TOO_LARGE,
};

enum sfce_red_black_color {
    SFCE_COLOR_BLACK = 0,
    SFCE_COLOR_RED   = 1,
};

enum sfce_newline_type {
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
    struct sfce_string_buffer_position start_position;
    struct sfce_string_buffer_position end_position;
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
    int32_t                 node_start_line_number;
    int32_t                 offset_within_piece;
};

struct sfce_piece_tree {
    struct sfce_piece_node    *root;
    struct sfce_string_buffer *buffers;
    int32_t                    change_buffer_index;
    int32_t                    buffer_count;
    int32_t                    buffer_capacity;
    int32_t                    line_count;
    int32_t                    length;
    enum sfce_newline_type     newline_type;
};

struct sfce_piece_tree_snapshot {
    struct sfce_piece *pieces;
    int32_t            piece_count;
    int32_t            piece_capacity;
};

int32_t round_multiple_of_two(int32_t value, int32_t multiple);
int32_t newline_sequence_size(const char * string);
int32_t buffer_newline_sequence_size(const char *buffer, int32_t buffer_size);
int32_t buffer_newline_count(const char *buffer, int32_t buffer_size);
int win32_set_console_cursor_visibilty(HANDLE console_handle, int visible);
COORD win32_get_rect_size(SMALL_RECT rect);

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
int16_t sfce_string_compare(struct sfce_string string0, struct sfce_string string1);

struct sfce_line_starts sfce_line_starts_alloc(size_t count);
void sfce_line_starts_destroy(struct sfce_line_starts *lines);
enum sfce_error_code sfce_line_starts_reserve(struct sfce_line_starts *lines, int32_t capacity);
enum sfce_error_code sfce_line_starts_resize(struct sfce_line_starts *lines, int32_t size);
enum sfce_error_code sfce_line_starts_push_line_offset(struct sfce_line_starts *lines, int32_t offset);

struct sfce_string_buffer sfce_string_buffer_create();
void sfce_string_buffer_destroy(struct sfce_string_buffer *buffer);
enum sfce_error_code sfce_string_buffer_append_content(struct sfce_string_buffer *buffer, const char *data, int32_t size);
struct sfce_string_buffer_position sfce_string_buffer_get_end_position(struct sfce_string_buffer *buffer);
struct sfce_string_buffer_position sfce_string_buffer_position_from_offset(struct sfce_string_buffer *buffer, int32_t offset);
struct sfce_string_buffer_position sfce_string_buffer_move_position_by_offset(struct sfce_string_buffer *buffer, struct sfce_string_buffer_position position, int32_t offset);
int32_t sfce_string_buffer_offset_from_position(struct sfce_string_buffer *string_buffer, struct sfce_string_buffer_position position);
enum sfce_error_code sfce_string_buffer_append_piece_content_to_string(struct sfce_string_buffer *string_buffer, struct sfce_piece piece, struct sfce_string *string);

struct sfce_piece_pair sfce_piece_split(struct sfce_piece_tree *tree, struct sfce_piece piece, int32_t offset, int32_t gap_size);
struct sfce_piece sfce_piece_erase_head(struct sfce_piece_tree *tree, struct sfce_piece piece, int32_t amount);
struct sfce_piece sfce_piece_erase_tail(struct sfce_piece_tree *tree, struct sfce_piece piece, int32_t amount);
enum sfce_error_code sfce_piece_append_content_to_string(struct sfce_piece piece, struct sfce_string_buffer *string_buffer, struct sfce_string *string);

struct sfce_piece_node *sfce_piece_node_create(struct sfce_piece piece);
void sfce_piece_node_destroy(struct sfce_piece_node *tree);
void sfce_piece_node_destroy_nonrecursive(struct sfce_piece_node *tree);
int32_t sfce_piece_node_calculate_length(struct sfce_piece_node *root);
int32_t sfce_piece_node_calculate_line_count(struct sfce_piece_node *root);
struct sfce_piece_node *sfce_piece_node_leftmost(struct sfce_piece_node *node);
struct sfce_piece_node *sfce_piece_node_rightmost(struct sfce_piece_node *node);
struct sfce_piece_node *sfce_piece_node_next(struct sfce_piece_node *node);
struct sfce_piece_node *sfce_piece_node_prev(struct sfce_piece_node *node);
struct sfce_piece_node *sfce_piece_node_rotate_left(struct sfce_piece_tree *tree, struct sfce_piece_node *node);
struct sfce_piece_node *sfce_piece_node_rotate_right(struct sfce_piece_tree *tree, struct sfce_piece_node *node);
struct sfce_piece_node *sfce_piece_node_insert_left(struct sfce_piece_tree *tree, struct sfce_piece_node *where, struct sfce_piece_node *node_to_insert);
struct sfce_piece_node *sfce_piece_node_insert_right(struct sfce_piece_tree *tree, struct sfce_piece_node *where, struct sfce_piece_node *node_to_insert);
void sfce_piece_node_remove_node(struct sfce_piece_tree *tree, struct sfce_piece_node *where);
void sfce_piece_node_transplant(struct sfce_piece_tree *tree, struct sfce_piece_node *where, struct sfce_piece_node *node_to_transplant);
void sfce_piece_node_update_metadata(struct sfce_piece_tree *tree, struct sfce_piece_node *node, int32_t delta_length, int32_t delta_line_count);
void sfce_piece_node_recompute_metadata(struct sfce_piece_tree *tree, struct sfce_piece_node *node);
void sfce_piece_node_fix_insert_violation(struct sfce_piece_tree *tree, struct sfce_piece_node *node);
void sfce_piece_node_fix_remove_violation(struct sfce_piece_tree *tree, struct sfce_piece_node *node);
void sfce_piece_node_print(struct sfce_piece_tree *tree, struct sfce_piece_node *root, uint32_t space);
void sfce_piece_node_inorder_print(struct sfce_piece_tree *tree, struct sfce_piece_node *root);
void sfce_piece_node_reset_sentinel();

struct sfce_piece_tree sfce_piece_tree_create(enum sfce_newline_type newline_type);
void sfce_piece_tree_destroy(struct sfce_piece_tree *tree);
int32_t sfce_piece_tree_get_change_buffer_index(struct sfce_piece_tree *tree, int32_t required_size);
struct sfce_node_position sfce_piece_tree_node_at_offset(struct sfce_piece_tree *tree, int32_t offset);
struct sfce_node_position sfce_piece_tree_node_at_row_and_col(struct sfce_piece_tree *tree, int32_t row, int32_t col);
struct sfce_string_buffer_position sfce_piece_tree_node_position_to_buffer_position(struct sfce_piece_tree *tree, struct sfce_node_position position);
enum sfce_error_code sfce_piece_tree_append_node_content_to_string(struct sfce_piece_tree *tree, struct sfce_piece_node *node, struct sfce_string *string);
enum sfce_error_code sfce_piece_tree_set_buffer_count(struct sfce_piece_tree *tree, int32_t buffer_count);
enum sfce_error_code sfce_piece_tree_add_buffer(struct sfce_piece_tree *tree, struct sfce_string_buffer buffer);
struct sfce_piece sfce_piece_tree_create_piece(struct sfce_piece_tree *tree, const char *data, int32_t byte_count);
enum sfce_error_code sfce_piece_tree_insert(struct sfce_piece_tree *tree, int32_t offset, const char *data, int32_t byte_count);
enum sfce_error_code sfce_piece_tree_erase(struct sfce_piece_tree *tree, int32_t offset, int32_t byte_count);
enum sfce_error_code sfce_piece_tree_load_file(struct sfce_piece_tree *tree, const char *filepath);
enum sfce_error_code sfce_piece_tree_get_line_content(struct sfce_piece_tree *tree, int32_t line_number, struct sfce_string *string);
enum sfce_error_code sfce_piece_tree_create_snapshot(struct sfce_piece_tree *tree, struct sfce_piece_tree_snapshot *snapshot);
void sfce_piece_tree_recompute_metadata(struct sfce_piece_tree *tree);

enum sfce_error_code sfce_piece_tree_snapshot_set_piece_count(struct sfce_piece_tree_snapshot *snapshot, int32_t count);
enum sfce_error_code sfce_piece_tree_snapshot_add_piece(struct sfce_piece_tree_snapshot *snapshot, struct sfce_piece piece);

static struct sfce_piece_node sentinel = {
    .parent = &sentinel,
    .left   = &sentinel,
    .right  = &sentinel,
    .color  = SFCE_COLOR_BLACK,
};

static struct sfce_piece_node *sentinel_ptr = &sentinel;

void run_sfce_piece_tree_test()
{
    struct sfce_piece_tree tree = sfce_piece_tree_create(SFCE_NEWLINE_TYPE_CRLF);
    sfce_piece_tree_insert(&tree, 0, "123", 3);
    sfce_piece_tree_insert(&tree, 0, "abc", 3);

    struct sfce_string tree_result_string = {};

    sfce_string_resize(&tree_result_string, 0);
    sfce_piece_tree_append_node_content_to_string(&tree, tree.root, &tree_result_string);

    sfce_piece_tree_erase(&tree, 0, 1);

    sfce_string_resize(&tree_result_string, 0);
    sfce_piece_tree_append_node_content_to_string(&tree, tree.root, &tree_result_string);
    assert(strncmp(tree_result_string.data, "bc123", 5) == 0);

    sfce_piece_tree_erase(&tree, 2, 2);
    sfce_string_resize(&tree_result_string, 0);
    sfce_piece_tree_append_node_content_to_string(&tree, tree.root, &tree_result_string);
    assert(strncmp(tree_result_string.data, "bc3", 3) == 0);

    sfce_piece_tree_destroy(&tree);
}

int savecursor() { return printf("\x1b[s") >= 0; }
int restorecursor() { return printf("\x1b[u") >= 0; }
int movecursor(int x, int y) { return printf("\x1b[%d;%dH", y, x) >= 0; }

#define ESC "\x1b"
#define CSI "\x1b["

void PrintVerticalBorder()
{
    printf(ESC "(0"); // Enter Line drawing mode
    printf(CSI "104;93m"); // bright yellow on bright blue
    printf("x"); // in line drawing mode, \x78 -> \u2502 "Vertical Bar"
    printf(CSI "0m"); // restore color
    printf(ESC "(B"); // exit line drawing mode
}

enum sfce_error_code sfce_enable_virtual_terminal()
{
    HANDLE console_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (console_handle == INVALID_HANDLE_VALUE) {
        return SFCE_ERROR_BAD;
    }

    DWORD console_mode = 0;
    if (!GetConsoleMode(console_handle, &console_mode)) {
        return SFCE_ERROR_BAD;
    }

    console_mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    if (!SetConsoleMode(console_handle, console_mode)) {
        return SFCE_ERROR_BAD;
    }

    return SFCE_ERROR_OK;
}

struct sfce_window_size sfce_get_console_screen_size()
{
    struct sfce_window_size window_size = {0};

    fprintf(stdout, "\x1b[s\x1b[32767;32767H");

#if defined(_WIN32) || defined(_WIN64)
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO cbsi = {};
    if (!GetConsoleScreenBufferInfo(handle, &cbsi)) {
        return (struct sfce_window_size) {};
    }

    window_size.width = cbsi.dwCursorPosition.X;
    window_size.height = cbsi.dwCursorPosition.Y;
#else
    window_size.width = 0;
    window_size.height = 0;
#endif

    fprintf(stdout, "\x1b[u");

    return window_size;
}

int main(void)
{
    enum sfce_error_code error_code = sfce_enable_virtual_terminal();
    if (error_code != SFCE_ERROR_OK) {
        return -1;
    }

    fprintf(stdout, "\x1b[?25l\x1b[?1049h");

    while (!_kbhit()) {
        Sleep(100);
    }

    struct sfce_window_size window_size = sfce_get_console_screen_size();

    fprintf(stdout, "\x1b[?25h\x1b[?1049l");
    fprintf(stderr, "window_size = { %d, %d }\n", window_size.width, window_size.height);

    return 0;
}

int main6(void)
{
    enum sfce_error_code error_code = sfce_enable_virtual_terminal();
    if (error_code != SFCE_ERROR_OK) {
        return -1;
    }

    savecursor();
    movecursor(10, 5);

    // Print a message at the new cursor position
    // printf("Cursor moved to (5,10)\n");
    // PrintVerticalBorder();

    _getch();

    restorecursor();
    return 0;
}

int main5(void)
{
    HANDLE console_handle = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
    assert(console_handle != INVALID_HANDLE_VALUE);

    DWORD console_mode = 0;
    assert(GetConsoleMode(console_handle, &console_mode));
    console_mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    assert(SetConsoleMode(console_handle, console_mode));

    assert(SetConsoleActiveScreenBuffer(console_handle));

    CONSOLE_SCREEN_BUFFER_INFO buffer_info = {};
    assert(GetConsoleScreenBufferInfo(console_handle, &buffer_info));

    // int16_t screen_width = buffer_info.srWindow.Right - buffer_info.srWindow.Left + 1;
    // int16_t screen_height = buffer_info.srWindow.Bottom - buffer_info.srWindow.Top + 1;
    COORD screen_size = { 120, 40 };
    CHAR_INFO *buffer = calloc(screen_size.X * screen_size.Y, sizeof *buffer);
    assert(buffer);

    for (int32_t idx = 0; idx < screen_size.X * screen_size.Y; ++idx) {
        buffer[idx].Char.UnicodeChar = L'A';
        buffer[idx].Attributes = FOREGROUND_BLUE | BACKGROUND_RED;
    }

    SMALL_RECT write_region = {
        0, 0,
        screen_size.X,
        screen_size.Y,
    };

    assert(WriteConsoleOutput(console_handle, buffer, screen_size, (COORD) { 0, 0 }, &write_region));

    _getch();

    return 0;
}

int main4(void)
{
    // enum sfce_error_code error_code;
    // HANDLE console_handle = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
    // // HANDLE console_handle = GetStdHandle(STD_OUTPUT_HANDLE);

    // if (console_handle == INVALID_HANDLE_VALUE) {
    //     return -1;
    // }
    
    // struct sfce_console_buffer console = sfce_console_buffer_create(console_handle);
    // if (console.buffer == NULL) {
    //     return -1;
    // }

    // while (!_kbhit()) {
    //     error_code = sfce_console_buffer_update(&console);
    //     if (error_code != SFCE_ERROR_OK) {
    //         fprintf(stderr, "from 'sfce_console_buffer_update' ERROR: %d\n", error_code);
    //         return -1;
    //     }
        
    //     for (int32_t idx = 0; idx < console.capacity; ++idx) {
    //         console.buffer[idx] = (CHAR_INFO) {
    //             .Char.UnicodeChar = 'A' + (idx) % ('z' - 'A' + 1),
    //             .Attributes = FOREGROUND_BLUE
    //         };
    //     }
        
    //     error_code = sfce_console_buffer_flush(&console);
    //     if (error_code != SFCE_ERROR_OK) {
    //         fprintf(stderr, "from 'sfce_console_buffer_flush' ERROR: %d\n", error_code);
    //         return -1;
    //     }
    // }

    // return 0;
}

int main3(void)
{
    HANDLE console_handle = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
    COORD buffer_size = {};
     
    if (console_handle == INVALID_HANDLE_VALUE) {
        return -1;
    }

    assert(SetConsoleActiveScreenBuffer(console_handle));

    CONSOLE_SCREEN_BUFFER_INFOEX info = { .cbSize = sizeof(info) };
    assert(GetConsoleScreenBufferInfoEx(console_handle, &info));

    CONSOLE_CURSOR_INFO cursor_info = {};
    assert(GetConsoleCursorInfo(console_handle, &cursor_info));
    cursor_info.bVisible = FALSE;
    assert(SetConsoleCursorInfo(console_handle, &cursor_info));

    buffer_size = info.dwSize;
    fprintf(stderr, "buffer_size = { %d, %d }\n", (int)buffer_size.X, (int)buffer_size.Y);
    
    CHAR_INFO *buffer = calloc(buffer_size.X * buffer_size.Y, sizeof *buffer);
    if (buffer == NULL) {
        return -1;
    }

    buffer[0].Char.UnicodeChar = 'A';
    buffer[0].Attributes = FOREGROUND_BLUE;

    while (!kbhit()) {
        COORD buffer_coord = { 0, 0 };
        SMALL_RECT write_region = {};

        assert(WriteConsoleOutput(console_handle, buffer, buffer_size, buffer_coord, &write_region));
    }

    assert(SetConsoleActiveScreenBuffer(GetStdHandle(STD_OUTPUT_HANDLE)));

    return 0;
}

int main2(int argc, const char *argv[])
{
    (void)(argc);
    (void)(argv);

    struct sfce_piece_tree tree = sfce_piece_tree_create(SFCE_NEWLINE_TYPE_LF);
    struct sfce_piece_tree_snapshot snapshot = {0};
    
    // sfce_piece_tree_load_file(&tree, "source/sfce.c");
    sfce_piece_tree_load_file(&tree, "ignore/index.js");
    sfce_piece_node_print(&tree, tree.root, 0);
    
    assert(sfce_piece_tree_create_snapshot(&tree, &snapshot) == SFCE_ERROR_OK);

    fprintf(stderr, "snapshot.pieces: %p\n", snapshot.pieces);
    fprintf(stderr, "snapshot.piece_count: %d\n", snapshot.piece_count);

    return 0;
}

int32_t round_multiple_of_two(int32_t value, int32_t multiple)
{
    return (value + multiple - 1) & -multiple;
}

int32_t newline_sequence_size(const char *string)
{
    if (string[0] == '\r') return (string[1] == '\n') ? 2 : 1;
    if (string[0] == '\n') return 1;
    return 0;
}

int32_t buffer_newline_sequence_size(const char *buffer, int32_t buffer_size)
{
    if (buffer_size != 0) {
        if (buffer[0] == '\r') return (buffer_size >= 2 && buffer[1] == '\n') ? 2 : 1;
        if (buffer[0] == '\n') return 1;
    }

    return 0;
}

int32_t buffer_newline_count(const char *buffer, int32_t buffer_size)
{
    int32_t newline_count = 0;
    for (int32_t idx = 0; idx < buffer_size;) {
        int32_t newline_size = buffer_newline_sequence_size(buffer, buffer_size - idx);

        if (newline_size != 0) {
            idx += newline_size;
            ++newline_count;
            continue;
        }

        ++idx;
    }

    return newline_count;
}

int win32_set_console_cursor_visibilty(HANDLE console_handle, int visible)
{
    CONSOLE_CURSOR_INFO cursor_info = {};
    
    if (!GetConsoleCursorInfo(console_handle, &cursor_info)) {
        return FALSE;
    }

    cursor_info.bVisible = visible;
    
    if (!SetConsoleCursorInfo(console_handle, &cursor_info)) {
        return FALSE;
    }

    return TRUE;
}

COORD win32_get_rect_size(SMALL_RECT rect)
{
    return (COORD) {
        .X = rect.Right - rect.Left + 1,
        .Y = rect.Bottom - rect.Top + 1,
    };
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
    result->size = size;
    
    if (result->size >= result->capacity) {
        int32_t new_capacity = round_multiple_of_two(result->size, SFCE_STRING_ALLOCATION_SIZE);
        return sfce_string_reserve(result, new_capacity);
    }

    return SFCE_ERROR_OK;
}

enum sfce_error_code sfce_string_copy(struct sfce_string *result, struct sfce_string string)
{
    enum sfce_error_code error_code = sfce_string_resize(result, string.size);

    if (error_code != SFCE_ERROR_OK) {
        return error_code;
    }

    memcpy(result->data, string.data, string.size);
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
    enum { TEMP_BUFFER_SIZE = 1024 };

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
    int size = fprintf(stdout, "%.*s", string->size, string->data);
    
    if (size < 0) {
        return SFCE_ERROR_FORMATED_STRING_TOO_LARGE;
    }

    return SFCE_ERROR_OK;
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

struct sfce_line_starts sfce_line_starts_alloc(size_t count)
{
    struct sfce_line_starts lines = {
        .offsets = malloc(count * sizeof *lines.offsets),
        .capacity = count,
        .count = 0,
    };

    if (lines.offsets == NULL) {
        return (struct sfce_line_starts) {};
    }

    return lines;
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

struct sfce_string_buffer sfce_string_buffer_create()
{
    struct sfce_line_starts line_starts = {};
    enum sfce_error_code error_code = sfce_line_starts_push_line_offset(&line_starts, 0);

    if (error_code != SFCE_ERROR_OK) {
        return (struct sfce_string_buffer) {};
    }

    return (struct sfce_string_buffer) {
        .content = (struct sfce_string) {},
        .line_starts = line_starts,
    };
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
        int32_t newline_size = buffer_newline_sequence_size(&buffer->content.data[offset], buffer->content.size - offset);

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
    int32_t line_low_index  = 0;
    int32_t line_high_index = buffer->line_starts.count - 1;

    while (line_low_index < line_high_index) {
        int32_t line_middle_index = line_low_index + (line_high_index - line_low_index) / 2;
        int32_t line_middle_start = buffer->line_starts.offsets[line_middle_index];

        if (line_middle_index == line_high_index) {
            break;
        }

        int32_t line_middle_end = buffer->line_starts.offsets[line_middle_index + 1];

        if (offset >= line_middle_end) {
            line_low_index = line_middle_index + 1;
            continue;
        }
        else if (offset < line_middle_start) {
            line_high_index = line_middle_index - 1;
            continue;
        }

        break;
    }

    int32_t line_start_offset = buffer->line_starts.offsets[line_low_index];
    return (struct sfce_string_buffer_position) {
        .line_start_index = line_low_index,
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

enum sfce_error_code sfce_string_buffer_append_piece_content_to_string(struct sfce_string_buffer *string_buffer, struct sfce_piece piece, struct sfce_string *string)
{
    int32_t offset0 = sfce_string_buffer_offset_from_position(string_buffer, piece.start_position);
    int32_t offset1 = sfce_string_buffer_offset_from_position(string_buffer, piece.end_position);
    
    enum sfce_error_code error_code = sfce_string_push_back_buffer(string, &string_buffer->content.data[offset0], offset1 - offset0);
    if (error_code != SFCE_ERROR_OK) {
        return error_code;
    }

    return SFCE_ERROR_OK;
}

struct sfce_piece_pair sfce_piece_split(struct sfce_piece_tree *tree, struct sfce_piece piece, int32_t offset, int32_t gap_size)
{
    struct sfce_string_buffer *string_buffer = &tree->buffers[piece.buffer_index];
    struct sfce_string_buffer_position middle_postion0 = sfce_string_buffer_move_position_by_offset(string_buffer, piece.start_position, offset);
    struct sfce_string_buffer_position middle_postion1 = sfce_string_buffer_move_position_by_offset(string_buffer, middle_postion0, gap_size);
    int32_t start_offset = sfce_string_buffer_offset_from_position(string_buffer, piece.start_position);
    int32_t middle_offset = sfce_string_buffer_offset_from_position(string_buffer, middle_postion1);
    int32_t end_offset = sfce_string_buffer_offset_from_position(string_buffer, piece.end_position);

    int32_t remaining = end_offset - middle_offset;

    int32_t left_line_count = buffer_newline_count(&string_buffer->content.data[start_offset], offset);
    int32_t right_line_count = buffer_newline_count(&string_buffer->content.data[middle_offset], remaining);

    struct sfce_piece left = {
        .start_position = piece.start_position,
        .end_position = middle_postion0,
        .buffer_index = piece.buffer_index,
        .line_count = left_line_count,
        .length = offset,
    };

    struct sfce_piece right = {
        .start_position = middle_postion1,
        .end_position = piece.end_position,
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
    struct sfce_string_buffer_position new_start_position = sfce_string_buffer_move_position_by_offset(string_buffer, piece.start_position, amount);
    int32_t start_offset = sfce_string_buffer_offset_from_position(string_buffer, new_start_position);
    int32_t remaining = piece.length - amount;
    
    return (struct sfce_piece) {
        .buffer_index = piece.buffer_index,
        .start_position = new_start_position,
        .end_position = piece.end_position,
        .length = remaining,
        .line_count = buffer_newline_count(&string_buffer->content.data[start_offset], remaining),
    };
}

struct sfce_piece sfce_piece_erase_tail(struct sfce_piece_tree *tree, struct sfce_piece piece, int32_t amount)
{
    struct sfce_string_buffer *string_buffer = &tree->buffers[piece.buffer_index];
    struct sfce_string_buffer_position new_end_position = sfce_string_buffer_move_position_by_offset(string_buffer, piece.end_position, -amount);
    int32_t start_offset = sfce_string_buffer_offset_from_position(string_buffer, piece.start_position);
    int32_t remaining = piece.length - amount;
    
    return (struct sfce_piece) {
        .buffer_index = piece.buffer_index,
        .start_position = piece.start_position,
        .end_position = new_end_position,
        .length = remaining,
        .line_count = buffer_newline_count(&string_buffer->content.data[start_offset], remaining),
    };
}

enum sfce_error_code sfce_piece_append_content_to_string(struct sfce_piece piece, struct sfce_string_buffer *string_buffer, struct sfce_string *string)
{
    int32_t offset0 = sfce_string_buffer_offset_from_position(string_buffer, piece.start_position);
    int32_t offset1 = sfce_string_buffer_offset_from_position(string_buffer, piece.end_position);

    enum sfce_error_code error_code = sfce_string_push_back_buffer(string, &string_buffer->content.data[offset0], offset1 - offset0);
    if (error_code != SFCE_ERROR_OK) {
        return error_code;
    }

    return SFCE_ERROR_OK;
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

void sfce_piece_node_destroy(struct sfce_piece_node *tree)
{
    if (tree != sentinel_ptr && tree != NULL) {
        sfce_piece_node_destroy(tree->left);
        sfce_piece_node_destroy(tree->right);
        sfce_piece_node_destroy_nonrecursive(tree);
    }
}

void sfce_piece_node_destroy_nonrecursive(struct sfce_piece_node *tree)
{
    if (tree != sentinel_ptr && tree != NULL) {
        free(tree);
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
    while (node->left != sentinel_ptr) { node = node->left; }
    return node;
}

struct sfce_piece_node *sfce_piece_node_rightmost(struct sfce_piece_node *node)
{
    while (node->right != sentinel_ptr) { node = node->right; }
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

struct sfce_piece_node *sfce_piece_node_rotate_left(struct sfce_piece_tree *tree, struct sfce_piece_node *x)
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
		tree->root = y;
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

struct sfce_piece_node *sfce_piece_node_rotate_right(struct sfce_piece_tree *tree, struct sfce_piece_node *y)
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
        tree->root = x;
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

struct sfce_piece_node *sfce_piece_node_insert_left(struct sfce_piece_tree *tree, struct sfce_piece_node *where, struct sfce_piece_node *node_to_insert)
{
    if (tree->root == sentinel_ptr) {
        tree->root = node_to_insert;
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

    sfce_piece_node_fix_insert_violation(tree, node_to_insert);
    return node_to_insert;
}

struct sfce_piece_node *sfce_piece_node_insert_right(struct sfce_piece_tree *tree, struct sfce_piece_node *where, struct sfce_piece_node *node_to_insert)
{
    if (tree->root == sentinel_ptr) {
        tree->root = node_to_insert;
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

    sfce_piece_node_fix_insert_violation(tree, node_to_insert);
    return node_to_insert;
}

void sfce_piece_node_remove_node(struct sfce_piece_tree *tree, struct sfce_piece_node *node_to_remove)
{
    if (node_to_remove == sentinel_ptr) {
        return;
    }

    enum sfce_red_black_color original_color = node_to_remove->color;
    struct sfce_piece_node *y = sentinel_ptr;
    struct sfce_piece_node *x = sentinel_ptr;

    if (node_to_remove->left == sentinel_ptr) {
        x = node_to_remove->right;
        sfce_piece_node_transplant(tree, node_to_remove, x);
        sfce_piece_node_recompute_metadata(tree, x);
    }
    else if (node_to_remove->right == sentinel_ptr) {
        x = node_to_remove->left;
        sfce_piece_node_transplant(tree, node_to_remove, x);
        sfce_piece_node_recompute_metadata(tree, x);
    }
    else {
        y = sfce_piece_node_leftmost(node_to_remove->right);
        original_color = y->color;

        x = y->right;

        if (y->parent == node_to_remove) {
            x->parent = y;
            sfce_piece_node_recompute_metadata(tree, x);
        }
        else {
            sfce_piece_node_transplant(tree, y, y->right);
            y->right = node_to_remove->right;
            y->right->parent = y;

            sfce_piece_node_recompute_metadata(tree, y);
        }

        sfce_piece_node_transplant(tree, node_to_remove, y);
        y->left = node_to_remove->left;
        y->left->parent = y;
        y->color = node_to_remove->color;

        sfce_piece_node_recompute_metadata(tree, y);
    }

    sfce_piece_node_reset_sentinel();
    if (original_color == SFCE_COLOR_BLACK) {
        sfce_piece_node_fix_remove_violation(tree, x);
    }
    
    sfce_piece_node_destroy_nonrecursive(node_to_remove);
}

void sfce_piece_node_transplant(struct sfce_piece_tree *tree, struct sfce_piece_node *where, struct sfce_piece_node *node_to_transplant)
{
    if (where == tree->root) {
        tree->root = node_to_transplant;
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

void sfce_piece_node_update_metadata(struct sfce_piece_tree *tree, struct sfce_piece_node *node, int32_t delta_length, int32_t delta_line_count)
{
    if (delta_length == 0 && delta_line_count == 0) {
        return;
    }

    node->left_subtree_length += delta_length;
    node->left_subtree_line_count += delta_line_count;

    while (node != tree->root) {
        if (node->parent->left == node) {
            node->parent->left_subtree_length += delta_length;
            node->parent->left_subtree_line_count += delta_line_count;
        }

        node = node->parent;
    }
}

void sfce_piece_node_recompute_metadata(struct sfce_piece_tree *tree, struct sfce_piece_node *node)
{
    if (node == tree->root || node == sentinel_ptr) {
        return;
    }

    while (node != sentinel_ptr && node == node->parent->right) {
        node = node->parent;
    }

    if (node == tree->root || node == sentinel_ptr) {
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
    sfce_piece_node_update_metadata(tree, node, delta_length, delta_line_count);
}

void sfce_piece_node_fix_insert_violation(struct sfce_piece_tree *tree, struct sfce_piece_node *node)
{
    sfce_piece_node_recompute_metadata(tree, node);

    node->color = SFCE_COLOR_RED;
    while (node != tree->root && node->parent->color == SFCE_COLOR_RED) {
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
                    sfce_piece_node_rotate_left(tree, node);
                }

                node->parent->color = SFCE_COLOR_BLACK;
                node->parent->parent->color = SFCE_COLOR_RED;
                sfce_piece_node_rotate_right(tree, node->parent->parent);
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
                    sfce_piece_node_rotate_right(tree, node);
                }

                node->parent->color = SFCE_COLOR_BLACK;
                node->parent->parent->color = SFCE_COLOR_RED;
                sfce_piece_node_rotate_left(tree, node->parent->parent);
            }
        }
	}

    tree->root->color = SFCE_COLOR_BLACK;
    sfce_piece_node_reset_sentinel();
}

void sfce_piece_node_fix_remove_violation(struct sfce_piece_tree *tree, struct sfce_piece_node *x)
{
    struct sfce_piece_node *s;
    x->color = SFCE_COLOR_BLACK;
    while (x != tree->root && x->color == SFCE_COLOR_BLACK) {
        if (x == x->parent->left) {
            s = x->parent->right;
            if (s->color == SFCE_COLOR_RED) {
                s->color = SFCE_COLOR_BLACK;
                x->parent->color = SFCE_COLOR_RED;
                sfce_piece_node_rotate_left(tree, x->parent);
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
                    sfce_piece_node_rotate_right(tree, s);
                    s = x->parent->right;
                }

                s->color = x->parent->color;
                x->parent->color = SFCE_COLOR_BLACK;
                s->right->color = SFCE_COLOR_BLACK;
                sfce_piece_node_rotate_left(tree, x->parent);
                x = tree->root;
            }
        }
        else {
            s = x->parent->left;
            if (s->color == SFCE_COLOR_RED) {
                s->color = SFCE_COLOR_BLACK;
                x->parent->color = SFCE_COLOR_RED;
                sfce_piece_node_rotate_right(tree, x->parent);
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
                    sfce_piece_node_rotate_left(tree, s);
                    s = x->parent->left;
                }

                s->color = x->parent->color;
                x->parent->color = SFCE_COLOR_BLACK;
                s->left->color = SFCE_COLOR_BLACK;
                sfce_piece_node_rotate_right(tree, x->parent);
                x = tree->root;
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
    int32_t start_offset = sfce_string_buffer_offset_from_position(buffer, piece.start_position);
    int32_t end_offset = sfce_string_buffer_offset_from_position(buffer, piece.end_position);

    printf("%.*s", end_offset - start_offset, &buffer->content.data[start_offset]);

    sfce_piece_node_inorder_print(tree, root->right);
}

void sfce_piece_node_print(struct sfce_piece_tree *tree, struct sfce_piece_node *root, uint32_t space)
{
    enum { COUNT = 4 };

    if (root == sentinel_ptr) {
        return;
    }

    space += COUNT;

    sfce_piece_node_print(tree, root->right, space);

    for (uint32_t i = COUNT; i < space; i++) {
        printf(" ");
    }

    const char *node_color_name = root->color == SFCE_COLOR_BLACK ? "BLACK" : "RED";
    struct sfce_string content = {};
    sfce_string_buffer_append_piece_content_to_string(&tree->buffers[root->piece.buffer_index], root->piece, &content);
    if (content.data != NULL) {
        printf("node(%s): '%.*s' \n", node_color_name, (int)content.size, content.data);
    }
    else {
        printf("node(%s): ''\n", node_color_name);
    }

    sfce_string_destroy(&content);

    sfce_piece_node_print(tree, root->left, space);
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

struct sfce_piece_tree sfce_piece_tree_create(enum sfce_newline_type newline_type)
{
    struct sfce_piece_tree tree = {
        .root = sentinel_ptr,
        .newline_type = newline_type,
    };
    
    struct sfce_string_buffer change_buffer = sfce_string_buffer_create();
    enum sfce_error_code error_code = sfce_piece_tree_add_buffer(&tree, change_buffer);

    if (error_code != SFCE_ERROR_OK) {
        return (struct sfce_piece_tree) {};
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

    *tree = (struct sfce_piece_tree) {};
}

int32_t sfce_piece_tree_get_change_buffer_index(struct sfce_piece_tree *tree, int32_t required_size)
{
    struct sfce_string_buffer *string_buffer = &tree->buffers[tree->change_buffer_index];
    int32_t remaining_size = SFCE_STRING_BUFFER_SIZE_THRESHOLD - string_buffer->content.size;

    if (remaining_size < required_size) {
        struct sfce_string_buffer new_string_buffer = sfce_string_buffer_create();
        tree->change_buffer_index = tree->buffer_count;
        sfce_piece_tree_add_buffer(tree, new_string_buffer);
    }

    return tree->change_buffer_index;
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
            return (struct sfce_node_position) {
                .node = node,
                .node_start_offset = node_start_offset,
                .node_start_line_number = node_start_line_count,
                .offset_within_piece = offset - node_start_offset,
            };
        }
    }

    return (struct sfce_node_position) { .node = sentinel_ptr };
}

struct sfce_node_position sfce_piece_tree_node_at_row_and_col(struct sfce_piece_tree *tree, int32_t row, int32_t col)
{
    struct sfce_node_position position = { .node = tree->root };
    
    for (int32_t subtree_line_count = row; position.node != sentinel_ptr;) {
        if (subtree_line_count < position.node->left_subtree_line_count) {
            position.node = position.node->left;
        }
        else if (subtree_line_count > position.node->left_subtree_line_count + position.node->piece.line_count) {
            position.node_start_offset += position.node->left_subtree_length + position.node->piece.length;
            position.node_start_line_number += position.node->left_subtree_line_count + position.node->piece.line_count;
            subtree_line_count -= position.node->left_subtree_line_count + position.node->piece.line_count;
            position.node = position.node->right;
        }
        else {

        }
    }

    return (struct sfce_node_position) { .node = sentinel_ptr };
}

// struct sfce_node_position sfce_piece_tree_node_at_row_and_col(struct sfce_piece_tree *tree, int32_t row, int32_t col)
// {
//     struct sfce_node_position position = { .node = tree->root };
//     int32_t subtree_line_number = row;

//     while (position.node != sentinel_ptr) {
//         if (subtree_line_number < position.node->left_subtree_line_count) {
//             position.node = position.node->left;
//         }
//         else if (subtree_line_number > position.node->left_subtree_line_count + position.node->piece.line_count) {
//             position.node_start_offset += position.node->left_subtree_length + position.node->piece.length;
//             position.node_start_line_number += position.node->left_subtree_line_count + position.node->piece.line_count;
//             subtree_line_number -= position.node->left_subtree_line_count + position.node->piece.line_count;
//             position.node = position.node->right;
//         }
//         else {
//             position.node_start_offset += position.node->left_subtree_length;
//             position.offset_within_piece = 0;
            
//             return position;
//         }
//     }

//     return (struct sfce_node_position) { .node = sentinel_ptr };
// }

struct sfce_string_buffer_position sfce_piece_tree_node_position_to_buffer_position(struct sfce_piece_tree *tree, struct sfce_node_position position)
{
    struct sfce_string_buffer *string_buffer = &tree->buffers[position.node->piece.buffer_index];
    int32_t piece_start_offset = sfce_string_buffer_offset_from_position(string_buffer, position.node->piece.start_position);
    return sfce_string_buffer_position_from_offset(string_buffer, piece_start_offset + position.offset_within_piece);
}

enum sfce_error_code sfce_piece_tree_append_node_content_to_string(struct sfce_piece_tree *tree, struct sfce_piece_node *node, struct sfce_string *string)
{
    enum sfce_error_code error_code = SFCE_ERROR_OK;

    if (node == sentinel_ptr) {
        return error_code;
    }

    error_code = sfce_piece_tree_append_node_content_to_string(tree, node->left, string);
    if (error_code != SFCE_ERROR_OK) {
        return error_code;
    }

    struct sfce_string_buffer *string_buffer = &tree->buffers[node->piece.buffer_index];
    error_code = sfce_string_buffer_append_piece_content_to_string(string_buffer, node->piece, string);
    if (error_code != SFCE_ERROR_OK) {
        return error_code;
    }

    error_code = sfce_piece_tree_append_node_content_to_string(tree, node->right, string);
    if (error_code != SFCE_ERROR_OK) {
        return error_code;
    }

    return error_code;
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

enum sfce_error_code sfce_piece_tree_add_buffer(struct sfce_piece_tree *tree, struct sfce_string_buffer buffer)
{
    enum sfce_error_code error_code = sfce_piece_tree_set_buffer_count(tree, tree->buffer_count + 1);
    if (error_code != SFCE_ERROR_OK) {
        return error_code;
    }

    tree->buffers[tree->buffer_count - 1] = buffer;
    return SFCE_ERROR_OK;
}

struct sfce_piece sfce_piece_tree_create_piece(struct sfce_piece_tree *tree, const char *data, int32_t byte_count)
{
    struct sfce_piece piece = {};
    piece.buffer_index = sfce_piece_tree_get_change_buffer_index(tree, byte_count);

    struct sfce_string_buffer *buffer = &tree->buffers[piece.buffer_index];
    piece.start_position = sfce_string_buffer_get_end_position(buffer);

    enum sfce_error_code error = sfce_string_buffer_append_content(buffer, data, byte_count);
    if (error != SFCE_ERROR_OK) {
        return (struct sfce_piece) {};
    }

    piece.end_position = sfce_string_buffer_get_end_position(buffer);
    piece.line_count = buffer_newline_count(data, byte_count);
    piece.length = byte_count;
    return piece;
}

enum sfce_error_code sfce_piece_tree_insert(struct sfce_piece_tree *tree, int32_t offset, const char *data, int32_t byte_count)
{
    struct sfce_node_position where = sfce_piece_tree_node_at_offset(tree, offset);

    if (where.node == sentinel_ptr) {
        return SFCE_ERROR_BAD_INSERTION;
    }

    struct sfce_piece piece_to_insert = sfce_piece_tree_create_piece(tree, data, byte_count);
    struct sfce_piece_node *node_to_insert = sfce_piece_node_create(piece_to_insert);

    if (node_to_insert == NULL) {
        return SFCE_ERROR_MEMORY_ALLOCATION_FAILURE;
    }

    if (tree->root == sentinel_ptr) {
        tree->root = node_to_insert;
        return SFCE_ERROR_OK;
    }

    if (where.offset_within_piece == 0) {
        sfce_piece_node_insert_left(tree, where.node, node_to_insert);
    }
    else if (where.offset_within_piece >= where.node->piece.length) {
        sfce_piece_node_insert_right(tree, where.node, node_to_insert);
    }
    else {
        // 
        // Within this else block we should checking for insertions
        // between newlines if the piece tree has a newline mode set.
        // 
        struct sfce_piece_pair split_pieces = sfce_piece_split(tree, where.node->piece, where.offset_within_piece, 0);
        struct sfce_piece_node *right_node = sfce_piece_node_create(split_pieces.right);

        if (right_node == NULL) {
            sfce_piece_node_destroy(node_to_insert);
            return SFCE_ERROR_MEMORY_ALLOCATION_FAILURE;
        }

        where.node->piece = split_pieces.left;
        sfce_piece_node_recompute_metadata(tree, where.node);

        sfce_piece_node_insert_right(tree, where.node, right_node);
        sfce_piece_node_insert_right(tree, where.node, node_to_insert);
    }

    sfce_piece_tree_recompute_metadata(tree);
    return SFCE_ERROR_OK;
}

enum sfce_error_code sfce_piece_tree_erase(struct sfce_piece_tree *tree, int32_t offset, int32_t byte_count)
{
    if (byte_count == 0) {
        return SFCE_ERROR_OK;
    }

    struct sfce_node_position start = sfce_piece_tree_node_at_offset(tree, offset);
    struct sfce_node_position end = sfce_piece_tree_node_at_offset(tree, offset + byte_count);

    if (start.node == end.node) {
        struct sfce_piece_node *node = start.node;
        struct sfce_piece_pair split_pieces = sfce_piece_split(tree, node->piece, start.offset_within_piece, byte_count);

        if (split_pieces.left.length == 0) {
            node->piece = split_pieces.right;
            sfce_piece_node_recompute_metadata(tree, node);
        }
        else if (split_pieces.right.length == 0) {
            node->piece = split_pieces.left;
            sfce_piece_node_recompute_metadata(tree, node);
        }
        else {
            node->piece = split_pieces.left;
            sfce_piece_node_recompute_metadata(tree, node);
            sfce_piece_node_insert_right(tree, node, sfce_piece_node_create(split_pieces.right));
        }
    }
    else {
        for (struct sfce_piece_node *node = sfce_piece_node_next(start.node); node != end.node && node != sentinel_ptr;) {
            struct sfce_piece_node *temp = sfce_piece_node_next(node);
            sfce_piece_node_remove_node(tree, node);
            node = temp;
        }

        start.node->piece = sfce_piece_erase_tail(tree, start.node->piece, start.node->piece.length - start.offset_within_piece);
        if (start.node->piece.length == 0) {
            sfce_piece_node_remove_node(tree, start.node);
        }
        else {
            sfce_piece_node_recompute_metadata(tree, start.node);
        }

        end.node->piece = sfce_piece_erase_head(tree, end.node->piece, end.offset_within_piece);
        if (end.node->piece.length == 0) {
            sfce_piece_node_remove_node(tree, end.node);
        }
        else {
            sfce_piece_node_recompute_metadata(tree, end.node);
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

    while (chunk_size = fread(buffer, 1, SFCE_STRING_BUFFER_SIZE_THRESHOLD, fp), chunk_size != 0) {
        struct sfce_piece piece = sfce_piece_tree_create_piece(tree, buffer, chunk_size);
        struct sfce_piece_node *node = sfce_piece_node_create(piece);
        struct sfce_piece_node *where = sfce_piece_node_rightmost(tree->root);
        sfce_piece_node_insert_right(tree, where, node);
    }

    sfce_piece_tree_recompute_metadata(tree);
    fclose(fp);
    return SFCE_ERROR_OK;
}

int32_t sfce_string_buffer_get_accumulated_value(struct sfce_string_buffer *string_buffer, struct sfce_piece_node *node, int32_t index) {
    if (index < 0) {
        return 0;
    }

    struct sfce_piece piece = node->piece;
    struct sfce_line_starts line_starts = string_buffer->line_starts;
    int32_t expectedLineStartIndex = piece.start_position.line_start_index + index + 1;
    if (expectedLineStartIndex > piece.end_position.line_start_index) {
        return line_starts.offsets[piece.end_position.line_start_index] + piece.end_position.column - line_starts.offsets[piece.start_position.line_start_index] - piece.start_position.column;
    }
    else {
        return line_starts.offsets[expectedLineStartIndex] - line_starts.offsets[piece.start_position.line_start_index] - piece.start_position.column;
    }
}

enum sfce_error_code _sfce_piece_tree_get_line_content(struct sfce_piece_tree *tree, int32_t line_number, struct sfce_string *string)
{
    // struct sfce_piece_node_position position = sfce_piece_tree_get_node_at_line_number(tree, line_number);

    return SFCE_ERROR_OK;
}

enum sfce_error_code sfce_piece_tree_get_line_content(struct sfce_piece_tree *tree, int32_t line_number, struct sfce_string *string)
{
    assert(0 && "sfce_piece_tree_get_line_content is unimplemented!");

    int32_t nodeStartOffset = 0;
    struct sfce_piece_node *x = tree->root;

    sfce_string_resize(string, 0);
    while (x != sentinel_ptr) {
        if (x->left != sentinel_ptr && x->left_subtree_line_count >= line_number - 1) {
            x = x->left;
        }
        else if (x->left_subtree_line_count + x->piece.line_count > line_number - 1) {
            struct sfce_string_buffer *string_buffer = &tree->buffers[x->piece.buffer_index];
            int32_t prevAccumulatedValue = sfce_string_buffer_get_accumulated_value(string_buffer, x, line_number - x->left_subtree_line_count - 2);
            int32_t accumulatedValue = sfce_string_buffer_get_accumulated_value(string_buffer, x, line_number - x->left_subtree_line_count - 1);
            int32_t startOffset = sfce_string_buffer_offset_from_position(string_buffer, x->piece.start_position);

            nodeStartOffset += x->left_subtree_length;
            
            enum sfce_error_code error_code = sfce_string_append_substring(string, string_buffer->content, startOffset + prevAccumulatedValue, startOffset + accumulatedValue);
            if (error_code != SFCE_ERROR_OK) {
                return error_code;
            }

            return SFCE_ERROR_OK;
        }
        else if (x->left_subtree_line_count + x->piece.line_count == line_number - 1) {
            struct sfce_string_buffer *string_buffer = &tree->buffers[x->piece.buffer_index];
            int32_t prevAccumulatedValue = sfce_string_buffer_get_accumulated_value(string_buffer, x, line_number - x->left_subtree_line_count - 2);
            int32_t startOffset = sfce_string_buffer_offset_from_position(string_buffer, x->piece.start_position);

            enum sfce_error_code error_code = sfce_string_append_substring(string, string_buffer->content, startOffset + prevAccumulatedValue, startOffset + x->piece.length);
            if (error_code != SFCE_ERROR_OK) {
                return error_code;
            }
            break;
        }
        else {
            line_number -= x->left_subtree_line_count + x->piece.line_count;
            nodeStartOffset += x->left_subtree_length + x->piece.length;
            x = x->right;
        }
    }

    // search in order, to find the node contains end column
    x = sfce_piece_node_next(x);
    while (x != sentinel_ptr) {
        struct sfce_string_buffer *buffer = &tree->buffers[x->piece.buffer_index];

        if (x->piece.line_count > 0) {
            int32_t accumulatedValue = sfce_string_buffer_get_accumulated_value(buffer, x, 0);
            int32_t startOffset = sfce_string_buffer_offset_from_position(buffer, x->piece.start_position);

            enum sfce_error_code error_code = sfce_string_append_substring(string, buffer->content, startOffset, startOffset + accumulatedValue);
            
            if (error_code != SFCE_ERROR_OK) {
                return error_code;
            }
            
            return SFCE_ERROR_OK;
        }
        else {
            int32_t startOffset = sfce_string_buffer_offset_from_position(buffer, x->piece.start_position);
            enum sfce_error_code error_code = sfce_string_append_substring(string, buffer->content, startOffset, x->piece.length);
            
            if (error_code != SFCE_ERROR_OK) {
                return error_code;
            }
        }

        x = sfce_piece_node_next(x);
    }

    return SFCE_ERROR_OK;
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
