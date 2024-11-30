// https://code.visualstudio.com/blogs/2018/03/23/text-buffer-reimplementation
// https://github.com/microsoft/vscode/tree/3cf67889583203811c81ca34bea2ad02d7c902db/src/vs/editor/common/model/pieceTreeTextBuffer
// gcc main.c -O3 -Werror -Wfatal-errors -Wall -o bin/sfce

#include <stdint.h>
#include <stdio.h>
#include <assert.h>

#include <windows.h>

#define PRINT_LINE_NUMBER() fprintf(stderr, "LINE: %u\n", __LINE__)

enum {
    SFCE_FILE_READ_CHUNK_SIZE = 1024,
    SFCE_LINE_STARTS_ALLOCATION_SIZE = 16, // MUST BE A POWER OF TWO
    SFCE_STRING_BUFFER_ALLOCATION_SIZE = 16, // MUST BE A POWER OF TWO
    SFCE_STRING_ALLOCATION_SIZE = 256, // MUST BE A POWER OF TWO
    SFCE_STRING_BUFFER_SIZE_THRESHOLD = 0xFFFF,
    SFCE_LEFT  = 0,
    SFCE_RIGHT = 1,
    SFCE_CHANGE_BUFFER_INDEX = 0,
};

enum sfce_error_code {
    SFCE_ERROR_OK,
    SFCE_ERROR_BAD_INSERTION,
    SFCE_ERROR_MEMORY_ALLOCATION_FAILURE,
    SFCE_ERROR_UNABLE_TO_OPEN_FILE,
    SFCE_ERROR_UNIMPLEMENTED,
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

struct sfce_piece_collection {
    int32_t           capacity;
    int32_t           count;
    struct sfce_piece pieces[];
};

struct sfce_piece_tree {
    // struct sfce_piece_tree    *children[2];
    struct sfce_piece_tree    *left;
    struct sfce_piece_tree    *right;
    struct sfce_piece_tree    *parent;
    struct sfce_piece          piece;
    int32_t                    left_subtree_length;
    int32_t                    left_subtree_line_count;
    enum sfce_red_black_color  color;
};

struct sfce_piece_table {
    struct sfce_piece_tree    *root;
    struct sfce_string_buffer *buffers;
    int32_t                    buffer_count;
    int32_t                    buffer_capacity;
    int32_t                    line_count;
    int32_t                    character_count;
    enum sfce_newline_type     newline_type;
};

int32_t round_multiple_of_two(int32_t value, int32_t multiple);
int32_t newline_sequence_size(const char * string);
int32_t buffer_newline_sequence_size(const char *buffer, int32_t buffer_size);
int32_t buffer_newline_count(const char *buffer, int32_t buffer_size);

void sfce_string_destroy(struct sfce_string *string);
enum sfce_error_code sfce_string_reserve(struct sfce_string *result, int32_t capacity);
enum sfce_error_code sfce_string_resize(struct sfce_string *result, int32_t size);
enum sfce_error_code sfce_string_write(struct sfce_string *string, int32_t index, const void *buffer, int32_t buffer_size);
enum sfce_error_code sfce_string_insert(struct sfce_string *string, int32_t index, const void *buffer, int32_t buffer_size);
enum sfce_error_code sfce_string_push_back(struct sfce_string *string, int32_t character);
enum sfce_error_code sfce_string_push_back_buffer(struct sfce_string *string, const void *buffer, int32_t buffer_size);
enum sfce_error_code sfce_string_load_file(struct sfce_string *string, const char *filepath);
int16_t sfce_string_compare(const struct sfce_string string0, const struct sfce_string string1);

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
int32_t sfce_string_buffer_get_offset_from_position(struct sfce_string_buffer *string_buffer, struct sfce_string_buffer_position position);
enum sfce_error_code sfce_string_buffer_append_piece_content_to_string(struct sfce_string_buffer *string_buffer, struct sfce_piece piece, struct sfce_string *string);

struct sfce_piece_pair sfce_piece_split(struct sfce_piece_table *table, struct sfce_piece piece, int32_t offset);

struct sfce_piece_tree *sfce_piece_tree_create(struct sfce_piece piece);
void sfce_piece_tree_destroy(struct sfce_piece_tree *tree);
int32_t sfce_piece_tree_calculate_length(struct sfce_piece_tree *root);
int32_t sfce_piece_tree_calculate_line_count(struct sfce_piece_tree *root);
struct sfce_piece_tree *sfce_piece_tree_leftmost(struct sfce_piece_tree *node);
struct sfce_piece_tree *sfce_piece_tree_rightmost(struct sfce_piece_tree *node);
struct sfce_piece_tree *sfce_piece_tree_next(struct sfce_piece_tree *node);
struct sfce_piece_tree *sfce_piece_tree_prev(struct sfce_piece_tree *node);
struct sfce_piece_tree *sfce_piece_tree_rotate_left(struct sfce_piece_tree **root, struct sfce_piece_tree *node);
struct sfce_piece_tree *sfce_piece_tree_rotate_right(struct sfce_piece_tree **root, struct sfce_piece_tree *node);
struct sfce_piece_tree *sfce_piece_tree_insert_left(struct sfce_piece_tree **root, struct sfce_piece_tree *where, struct sfce_piece_tree *node_to_insert);
struct sfce_piece_tree *sfce_piece_tree_insert_right(struct sfce_piece_tree **root, struct sfce_piece_tree *where, struct sfce_piece_tree *node_to_insert);
struct sfce_piece_tree *sfce_piece_tree_remove_node(struct sfce_piece_tree *where);
struct sfce_piece_tree *sfce_piece_tree_find_node_at_offset(struct sfce_piece_tree *root, int32_t offset);
void sfce_piece_tree_update_metadata(struct sfce_piece_tree **root, struct sfce_piece_tree *node, int32_t delta_length, int32_t delta_line_count);
void sfce_piece_tree_recompute_metadata(struct sfce_piece_tree **root, struct sfce_piece_tree *node);
void sfce_piece_tree_fix_insert_violation(struct sfce_piece_tree **root, struct sfce_piece_tree *node);
void sfce_piece_tree_fix_remove_violation(struct sfce_piece_tree **root, struct sfce_piece_tree *node);
void sfce_piece_tree_print(struct sfce_piece_table *table, struct sfce_piece_tree *root, uint32_t space);
void sfce_piece_tree_inorder_print(struct sfce_piece_table *table, struct sfce_piece_tree *root);
void sfce_piece_tree_reset_sentinel();

struct sfce_piece_table sfce_piece_table_create(enum sfce_newline_type newline_type);
void sfce_piece_table_destroy(struct sfce_piece_table *table);
enum sfce_error_code sfce_piece_table_append_node_content_to_string(struct sfce_piece_table *table, struct sfce_piece_tree *node, struct sfce_string *string);
enum sfce_error_code sfce_piece_table_set_buffer_count(struct sfce_piece_table *table, int32_t buffer_count);
enum sfce_error_code sfce_piece_table_add_buffer(struct sfce_piece_table *table, struct sfce_string_buffer buffer);
struct sfce_piece sfce_piece_table_create_piece(struct sfce_piece_table *table, const char *data, int32_t byte_count);
enum sfce_error_code sfce_piece_table_insert(struct sfce_piece_table *table, int32_t offset, const char *data, int32_t byte_count);
enum sfce_error_code sfce_piece_table_erase(struct sfce_piece_table *table, int32_t offset, int32_t byte_count);

static struct sfce_piece_tree sentinel = {
    .parent = &sentinel,
    .left   = &sentinel,
    .right  = &sentinel,
    .color  = SFCE_COLOR_BLACK,
};

static struct sfce_piece_tree *sentinel_ptr = &sentinel;

void run_sfce_piece_table_test()
{
    struct sfce_piece_table table = sfce_piece_table_create(SFCE_NEWLINE_TYPE_CRLF);
    sfce_piece_table_insert(&table, 0, "123", 3);
    sfce_piece_table_insert(&table, 0, "abc", 3);

    struct sfce_string table_result_string = {};

    sfce_string_resize(&table_result_string, 0);
    sfce_piece_table_append_node_content_to_string(&table, table.root, &table_result_string);
    assert(strncmp(table_result_string.data, "abc123", 6) == 0);

    // sfce_piece_table_erase(&table, 2, 2);

    // sfce_string_resize(&table_result_string, 0);
    // sfce_piece_table_append_node_content_to_string(&table, table.root, &table_result_string);
    // assert(strncmp(table_result_string.data, "abc23", 6) == 0);

    sfce_piece_table_destroy(&table);
}

int main(int argc, const char *argv[])
{
    (void)(argc);
    (void)(argv);

    run_sfce_piece_table_test();

    struct sfce_piece_table table = sfce_piece_table_create(SFCE_NEWLINE_TYPE_CRLF);

    sfce_piece_table_insert(&table, 0, "asjdklasjdlk", 12);
    sfce_piece_table_insert(&table, 0, "------------", 12);
    sfce_piece_table_insert(&table, 0, "qwertyuiopas", 12);
    sfce_piece_table_insert(&table, 6, "0123456789", 10);

    sfce_piece_tree_print(&table, table.root, 0);
    // printf("---------------------------------------------\n");

    // table.root->left = sfce_piece_tree_remove_node(table.root->left);

    // sfce_piece_tree_print(&table, table.root, 0);
    sfce_piece_tree_inorder_print(&table, table.root);
    sfce_piece_table_destroy(&table);

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
    const int32_t size = string->size;
    enum sfce_error_code error_code = sfce_string_resize(string, string->size + buffer_size);
    
    if (error_code != SFCE_ERROR_OK) {
        return error_code;
    }
    
    memcpy(&string->data[size], buffer_data, buffer_size * sizeof *buffer_data);
    return SFCE_ERROR_OK;
}

enum sfce_error_code sfce_string_load_file(struct sfce_string *string, const char *filepath)
{
    FILE * fp = fopen(filepath, "rb");

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

int16_t sfce_string_compare(const struct sfce_string string0, const struct sfce_string string1)
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
    struct sfce_line_starts lines = {};
    lines.offsets = malloc(count * sizeof *lines.offsets);
    lines.capacity = count;
    lines.count = 0;

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
    int32_t line_index = buffer->line_starts.count - 1;
    int32_t column     = buffer->content.size - buffer->line_starts.offsets[line_index];
    return (struct sfce_string_buffer_position){
        .line_start_index = line_index,
        .column = column,
    };
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
    int32_t position_offset = sfce_string_buffer_get_offset_from_position(buffer, position) + offset;

    if (position_offset < 0) {
        return (struct sfce_string_buffer_position) {};
    }

    for (;;) {
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

int32_t sfce_string_buffer_get_offset_from_position(struct sfce_string_buffer *string_buffer, struct sfce_string_buffer_position position)
{
    return string_buffer->line_starts.offsets[position.line_start_index] + position.column;
}

enum sfce_error_code sfce_string_buffer_append_piece_content_to_string(struct sfce_string_buffer *string_buffer, struct sfce_piece piece, struct sfce_string *string)
{
    int32_t offset0 = sfce_string_buffer_get_offset_from_position(string_buffer, piece.start_position);
    int32_t offset1 = sfce_string_buffer_get_offset_from_position(string_buffer, piece.end_position);

    enum sfce_error_code error_code = sfce_string_push_back_buffer(string, &string_buffer->content.data[offset0], offset1 - offset0);
    if (error_code != SFCE_ERROR_OK) {
        return error_code;
    }

    return SFCE_ERROR_OK;
}

struct sfce_piece_pair sfce_piece_split(struct sfce_piece_table *table, struct sfce_piece piece, int32_t offset)
{
    struct sfce_string_buffer_position middle_position;
    struct sfce_string_buffer *buffer = &table->buffers[piece.buffer_index];
    int32_t start_offset = sfce_string_buffer_get_offset_from_position(buffer, piece.start_position);
    int32_t middle_offset = start_offset + offset;

    middle_position = sfce_string_buffer_position_from_offset(buffer, middle_offset);
    struct sfce_piece left = {
        .start_position = piece.start_position,
        .end_position = middle_position,
        .buffer_index = piece.buffer_index,
        .line_count = buffer_newline_count(&buffer->content.data[start_offset], offset),
        .length = offset,
    };

    int32_t remaining = piece.length - offset;
    middle_position = sfce_string_buffer_move_position_by_offset(buffer, middle_position, 1);
    struct sfce_piece right = {
        .start_position = middle_position,
        .end_position = piece.end_position,
        .buffer_index = piece.buffer_index,
        .line_count = buffer_newline_count(&buffer->content.data[middle_offset + 1], remaining),
        .length = remaining,
    };

    return (struct sfce_piece_pair) { left, right };
}

struct sfce_piece_tree *sfce_piece_tree_create(struct sfce_piece piece)
{
    struct sfce_piece_tree *tree = malloc(sizeof *tree);
    
    if (tree == NULL) {
        return NULL;
    }
    
    *tree = (struct sfce_piece_tree) {
        .left = sentinel_ptr,
        .right = sentinel_ptr,
        .parent = sentinel_ptr,
        .piece = piece,
        .color = SFCE_COLOR_BLACK,
    };
    
    return tree;
}

void sfce_piece_tree_destroy(struct sfce_piece_tree *tree)
{
    if (tree == sentinel_ptr || tree == NULL) {
        return;
    }
    
    sfce_piece_tree_destroy(tree->left);
    sfce_piece_tree_destroy(tree->right);
    free(tree);
}

int32_t sfce_piece_tree_calculate_length(struct sfce_piece_tree *node)
{
    if (node == sentinel_ptr) {
        return 0;
    }

    int32_t right_node_length = sfce_piece_tree_calculate_length(node->right);
    return node->left_subtree_length + node->piece.length + right_node_length;
}

int32_t sfce_piece_tree_calculate_line_count(struct sfce_piece_tree *node)
{
    if (node == sentinel_ptr) {
        return 0;
    }

    int32_t right_node_line_count = sfce_piece_tree_calculate_line_count(node->right);
    return node->left_subtree_line_count + node->piece.line_count + right_node_line_count;
}

struct sfce_piece_tree *sfce_piece_tree_leftmost(struct sfce_piece_tree *node)
{
    while (node->left != sentinel_ptr) { node = node->left; }
    return node;
}

struct sfce_piece_tree *sfce_piece_tree_rightmost(struct sfce_piece_tree *node)
{
    while (node->right != sentinel_ptr) { node = node->right; }
    return node;
}

struct sfce_piece_tree *sfce_piece_tree_next(struct sfce_piece_tree *node)
{
    if (node->right != sentinel_ptr) {
        return sfce_piece_tree_leftmost(node->right);
    }

    while (node->parent != sentinel_ptr && node->parent->left != node) {
        node = node->parent;
    }

    return node->parent;
}

struct sfce_piece_tree *sfce_piece_tree_prev(struct sfce_piece_tree *node)
{
    if (node->left != sentinel_ptr) {
        return sfce_piece_tree_rightmost(node->left);
    }

    while (node->parent != sentinel_ptr && node->parent->right != node) {
        node = node->parent;
    }

    return node->parent;
}

struct sfce_piece_tree *sfce_piece_tree_rotate_left(struct sfce_piece_tree **root, struct sfce_piece_tree *node)
{
    struct sfce_piece_tree *grandparent = node->parent;
    struct sfce_piece_tree *new_parent = node->right;
    struct sfce_piece_tree *new_right_node = new_parent->left;

    if (new_parent == sentinel_ptr) {
        return sentinel_ptr;
    }

    node->left_subtree_length += new_parent->left_subtree_length + new_parent->piece.length;
    node->left_subtree_line_count += new_parent->left_subtree_line_count + new_parent->piece.line_count;

    node->right = new_right_node;
    if (new_right_node != sentinel_ptr) {
        new_right_node->parent = node;
    }

    new_parent->parent = grandparent;
    new_parent->left = node;
    node->parent = new_parent;

    if (grandparent == sentinel_ptr) {
        return *root = new_parent;
    }

    if (node == grandparent->right) {
        grandparent->right = new_parent;
    }
    else {
        grandparent->left = new_parent;
    }

    return new_parent;
}

struct sfce_piece_tree *sfce_piece_tree_rotate_right(struct sfce_piece_tree **root, struct sfce_piece_tree *node)
{
    struct sfce_piece_tree *grandparent = node->parent;
    struct sfce_piece_tree *new_parent = node->left;
    struct sfce_piece_tree *new_left_node = new_parent->right;

    if (new_parent == sentinel_ptr) {
        return sentinel_ptr;
    }

    node->left_subtree_length -= new_parent->left_subtree_length + new_parent->piece.length;
    node->left_subtree_line_count -= new_parent->left_subtree_line_count + new_parent->piece.line_count;

    node->left = new_left_node;
    if (new_left_node != sentinel_ptr) {
        new_left_node->parent = node;
    }

    new_parent->parent = grandparent;
    new_parent->right = node;
    node->parent = new_parent;

    if (grandparent == sentinel_ptr) {
        return *root = new_parent;
    }

    if (node == grandparent->right) {
        grandparent->right = new_parent;
    }
    else {
        grandparent->left = new_parent;
    }

    return new_parent;
}

struct sfce_piece_tree *sfce_piece_tree_insert_left(struct sfce_piece_tree **root, struct sfce_piece_tree *where, struct sfce_piece_tree *node_to_insert)
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
        struct sfce_piece_tree *prev_node = sfce_piece_tree_rightmost(where->left);
        prev_node->right = node_to_insert;
        node_to_insert->parent = prev_node;
    }

    sfce_piece_tree_fix_insert_violation(root, node_to_insert);
    return node_to_insert;
}

struct sfce_piece_tree *sfce_piece_tree_insert_right(struct sfce_piece_tree **root, struct sfce_piece_tree *where, struct sfce_piece_tree *node_to_insert)
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
        struct sfce_piece_tree *next_node = sfce_piece_tree_leftmost(where->right);
        next_node->left = node_to_insert;
        node_to_insert->parent = next_node;
    }

    sfce_piece_tree_fix_insert_violation(root, node_to_insert);
    return node_to_insert;
}

struct sfce_piece_tree *sfce_piece_tree_remove_node(struct sfce_piece_tree *node_to_remove)
{
    struct sfce_piece_tree *left_node = node_to_remove->left;
    struct sfce_piece_tree *right_node = node_to_remove->right;

    if (node_to_remove == sentinel_ptr) {
        return sentinel_ptr;
    }

    if (left_node == sentinel_ptr && right_node == sentinel_ptr) {
        return sentinel_ptr;
    }

    if (right_node != sentinel_ptr) {
        return right_node;
    }

    if (left_node != sentinel_ptr) {
        return left_node;
    }

    struct sfce_piece_tree *successor_node = sfce_piece_tree_leftmost(right_node);
    node_to_remove->piece = successor_node->piece;

    if (successor_node->parent->right == successor_node) {
        successor_node->parent->right = sfce_piece_tree_remove_node(successor_node);
    }
    else {
        successor_node->parent->left = sfce_piece_tree_remove_node(successor_node);
    }

    return node_to_remove;
}

struct sfce_piece_tree *sfce_piece_tree_find_node_at_offset(struct sfce_piece_tree *root, int32_t offset)
{
    if (root == sentinel_ptr) {
        return sentinel_ptr;
    }

    if (offset < root->left_subtree_length) {
        return sfce_piece_tree_find_node_at_offset(root->left, offset);
    }

    int32_t right_subtree_offset = root->left_subtree_length + root->piece.length;
    if (offset > right_subtree_offset) {
        return sfce_piece_tree_find_node_at_offset(root->right, offset - right_subtree_offset);
    }

    return root;
}

void sfce_piece_tree_update_metadata(struct sfce_piece_tree **root, struct sfce_piece_tree *node, int32_t delta_length, int32_t delta_line_count)
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

void sfce_piece_tree_recompute_metadata(struct sfce_piece_tree **root, struct sfce_piece_tree *node)
{
    if (node == *root) {
        return;
    }

    while (node != sentinel_ptr && node == node->parent->right) {
        node = node->parent;
    }

    if (node == *root || node == sentinel_ptr) {
        return;
    }

    node = node->parent;

    int32_t delta_length = sfce_piece_tree_calculate_length(node->left) - node->left_subtree_length;
    int32_t delta_line_count = sfce_piece_tree_calculate_line_count(node->left) - node->left_subtree_line_count;
    sfce_piece_tree_update_metadata(root, node, delta_length, delta_line_count);
}

void sfce_piece_tree_fix_insert_violation(struct sfce_piece_tree **root, struct sfce_piece_tree *node)
{
    sfce_piece_tree_recompute_metadata(root, node);

    node->color = SFCE_COLOR_RED;
    while (node != *root && node->parent->color == SFCE_COLOR_RED) {
        if (node->parent->parent->left == node->parent) {
            struct sfce_piece_tree *uncle = node->parent->parent->right;

            if (uncle->color == SFCE_COLOR_RED) {
                uncle->color = SFCE_COLOR_BLACK;
                node->parent->color = SFCE_COLOR_BLACK;
                node->parent->parent->color = SFCE_COLOR_RED;
                node = node->parent->parent;
            }
            else {
                if (node->parent->right == node) {
                    node = node->parent;
                    sfce_piece_tree_rotate_left(root, node);
                }

                node->parent->color = SFCE_COLOR_BLACK;
                node->parent->parent->color = SFCE_COLOR_RED;
                sfce_piece_tree_rotate_right(root, node->parent->parent);
            }
        }
        else {
            struct sfce_piece_tree *uncle = node->parent->parent->left;

            if (uncle->color == SFCE_COLOR_RED) {
                uncle->color = SFCE_COLOR_BLACK;
                node->parent->color = SFCE_COLOR_BLACK;
                node->parent->parent->color = SFCE_COLOR_RED;
                node = node->parent->parent;
            }
            else {
                if (node->parent->left == node) {
                    node = node->parent;
                    sfce_piece_tree_rotate_right(root, node);
                }

                node->parent->color = SFCE_COLOR_BLACK;
                node->parent->parent->color = SFCE_COLOR_RED;
                sfce_piece_tree_rotate_left(root, node->parent->parent);
            }
        }
	}

    (*root)->color = SFCE_COLOR_BLACK;
    sfce_piece_tree_reset_sentinel();
}

void sfce_piece_tree_fix_remove_violation(struct sfce_piece_tree **root, struct sfce_piece_tree *node)
{
    assert(0 && "sfce_piece_tree_fix_remove_violation is unimplemented!");

    node->color = SFCE_COLOR_BLACK;
    (*root)->color = SFCE_COLOR_BLACK;
    sfce_piece_tree_reset_sentinel();
}

static enum sfce_error_code _sfce_piece_table_append_node_content_to_string(struct sfce_piece_table *table, struct sfce_piece_tree *node, struct sfce_string *string)
{
    struct sfce_string_buffer *string_buffer = &table->buffers[node->piece.buffer_index];
    int32_t offset0 = sfce_string_buffer_get_offset_from_position(string_buffer, node->piece.start_position);
    int32_t offset1 = sfce_string_buffer_get_offset_from_position(string_buffer, node->piece.end_position);

    enum sfce_error_code error_code = sfce_string_push_back_buffer(string, &string_buffer->content.data[offset0], offset1 - offset0);
    if (error_code != SFCE_ERROR_OK) {
        return error_code;
    }

    return SFCE_ERROR_OK;
}

void sfce_piece_tree_inorder_print(struct sfce_piece_table *table, struct sfce_piece_tree *root)
{
    if (root == sentinel_ptr) {
        return;
    }

    sfce_piece_tree_inorder_print(table, root->left);

    struct sfce_piece piece = root->piece;
    struct sfce_string_buffer *buffer = &table->buffers[piece.buffer_index];
    int32_t start_offset = sfce_string_buffer_get_offset_from_position(buffer, piece.start_position);
    int32_t end_offset = sfce_string_buffer_get_offset_from_position(buffer, piece.end_position);

    printf("%.*s", end_offset - start_offset, &buffer->content.data[start_offset]);

    sfce_piece_tree_inorder_print(table, root->right);
}

void sfce_piece_tree_print(struct sfce_piece_table *table, struct sfce_piece_tree *root, uint32_t space)
{
    enum { COUNT = 4 };

    if (root == sentinel_ptr) {
        return;
    }

    space += COUNT;

    sfce_piece_tree_print(table, root->right, space);

    for (uint32_t i = COUNT; i < space; i++) {
        printf(" ");
    }

    const char *node_color_name = root->color == SFCE_COLOR_BLACK ? "BLACK" : "RED";
    struct sfce_string content = {};
    _sfce_piece_table_append_node_content_to_string(table, root, &content);
    printf("node: '%.*s' (%s)\n", (int)content.size, content.data, node_color_name);
    sfce_string_destroy(&content);

    sfce_piece_tree_print(table, root->left, space);
}

void sfce_piece_tree_reset_sentinel()
{
    sentinel = (struct sfce_piece_tree){
        .parent = sentinel_ptr,
        .left = sentinel_ptr,
        .right = sentinel_ptr,
        .color = SFCE_COLOR_BLACK,
    };
}

struct sfce_piece_table sfce_piece_table_create(enum sfce_newline_type newline_type)
{
    struct sfce_piece_table table = {
        .root = sentinel_ptr,
        .newline_type = newline_type,
    };
    
    struct sfce_string_buffer change_buffer = sfce_string_buffer_create();
    enum sfce_error_code error_code = sfce_piece_table_add_buffer(&table, change_buffer);

    if (error_code != SFCE_ERROR_OK) {
        return (struct sfce_piece_table) {};
    }

    return table;
}

void sfce_piece_table_destroy(struct sfce_piece_table *table)
{
    if (table->root != NULL) {
        sfce_piece_tree_destroy(table->root);
    }

    if (table->buffers != NULL) {
        for (int32_t idx = 0; idx < table->buffer_count; ++idx) {
            sfce_string_buffer_destroy(table->buffers);
        }

        free(table->buffers);
    }

    *table = (struct sfce_piece_table) {};
}

enum sfce_error_code sfce_piece_table_append_node_content_to_string(struct sfce_piece_table *table, struct sfce_piece_tree *node, struct sfce_string *string)
{
    enum sfce_error_code error_code = SFCE_ERROR_OK;

    if (node == sentinel_ptr) {
        return error_code;
    }

    error_code = sfce_piece_table_append_node_content_to_string(table, node->left, string);
    if (error_code != SFCE_ERROR_OK) {
        return error_code;
    }

    struct sfce_string_buffer *string_buffer = &table->buffers[node->piece.buffer_index];
    error_code = sfce_string_buffer_append_piece_content_to_string(string_buffer, node->piece, string);
    if (error_code != SFCE_ERROR_OK) {
        return error_code;
    }

    error_code = sfce_piece_table_append_node_content_to_string(table, node->right, string);
    if (error_code != SFCE_ERROR_OK) {
        return error_code;
    }

    return error_code;
}

enum sfce_error_code sfce_piece_table_set_buffer_count(struct sfce_piece_table *table, int32_t buffer_count)
{
    if (buffer_count >= table->buffer_capacity) {
        void *temp_ptr = table->buffers;
        table->buffer_capacity = round_multiple_of_two(buffer_count, SFCE_STRING_BUFFER_ALLOCATION_SIZE);
        table->buffers = realloc(table->buffers, table->buffer_capacity * sizeof *table->buffers);

        if (table->buffers == NULL) {
            sfce_piece_table_destroy(table);
            free(temp_ptr);
            return SFCE_ERROR_MEMORY_ALLOCATION_FAILURE;
        }
    }

    table->buffer_count = buffer_count;
    return SFCE_ERROR_OK;
}

enum sfce_error_code sfce_piece_table_add_buffer(struct sfce_piece_table *table, struct sfce_string_buffer buffer)
{
    enum sfce_error_code error_code = sfce_piece_table_set_buffer_count(table, table->buffer_count + 1);
    if (error_code != SFCE_ERROR_OK) {
        return error_code;
    }

    table->buffers[table->buffer_count - 1] = buffer;
    return SFCE_ERROR_OK;
}

struct sfce_piece sfce_piece_table_create_piece(struct sfce_piece_table *table, const char *data, int32_t byte_count)
{
    const uint32_t buffer_index = SFCE_CHANGE_BUFFER_INDEX;
    struct sfce_string_buffer *change_buffer = &table->buffers[buffer_index];

    struct sfce_string_buffer_position start_position = sfce_string_buffer_get_end_position(change_buffer);
    enum sfce_error_code error = sfce_string_buffer_append_content(change_buffer, data, byte_count);
    if (error != SFCE_ERROR_OK) {
        return (struct sfce_piece) {};
    }

    struct sfce_string_buffer_position end_position = sfce_string_buffer_get_end_position(change_buffer);
    
    int32_t line_count = buffer_newline_count(data, byte_count);
    return (struct sfce_piece) {
        .start_position = start_position,
        .end_position = end_position,
        .buffer_index = buffer_index,
        .length = byte_count,
        .line_count = line_count,
    };
}

enum sfce_error_code sfce_piece_table_insert(struct sfce_piece_table *table, int32_t offset, const char *data, int32_t byte_count)
{
    struct sfce_piece piece_to_insert = sfce_piece_table_create_piece(table, data, byte_count);
    struct sfce_piece_tree *node_to_insert = sfce_piece_tree_create(piece_to_insert);

    if (node_to_insert == NULL) {
        return SFCE_ERROR_MEMORY_ALLOCATION_FAILURE;
    }

    if (table->root == sentinel_ptr) {
        table->root = node_to_insert;
        return SFCE_ERROR_OK;
    }

    struct sfce_piece_tree *where_to_insert = sfce_piece_tree_find_node_at_offset(table->root, offset);
    int32_t insert_offset = offset - where_to_insert->left_subtree_length;

    if (insert_offset == 0) {
        sfce_piece_tree_insert_left(&table->root, where_to_insert, node_to_insert);
    }
    else if (insert_offset >= where_to_insert->piece.length) {
        sfce_piece_tree_insert_right(&table->root, where_to_insert, node_to_insert);
    }
    else {
        struct sfce_piece_pair split_pieces = sfce_piece_split(table, where_to_insert->piece, insert_offset);
        struct sfce_piece_tree *right_node = sfce_piece_tree_create(split_pieces.right);

        if (right_node == NULL) {
            return SFCE_ERROR_MEMORY_ALLOCATION_FAILURE;
        }

        where_to_insert->piece = split_pieces.left;
        sfce_piece_tree_insert_right(&table->root, where_to_insert, right_node);
        sfce_piece_tree_insert_right(&table->root, where_to_insert, node_to_insert);
    }

    table->character_count += piece_to_insert.length;
    table->line_count      += piece_to_insert.line_count - 1;

    sfce_piece_tree_reset_sentinel();
    return SFCE_ERROR_OK;
}

enum sfce_error_code sfce_piece_table_erase(struct sfce_piece_table *table, int32_t offset, int32_t byte_count)
{
    assert(0 && "sfce_piece_table_erase is unimplemented!");

    struct sfce_piece_tree *node_remove_start = sfce_piece_tree_find_node_at_offset(table->root, offset);
    struct sfce_piece_tree *node_remove_end = sfce_piece_tree_find_node_at_offset(table->root, offset + byte_count);

    if (node_remove_start == node_remove_end) {
        // 
        // Remove range from a single node piece.
        // 
    }
    else {
        // 
        // Remove all nodes between the start and end node
        // and the parts overlapping the pieces from the
        // start and end nodes.
        // 
    }

    return SFCE_ERROR_OK;
}
