// https://code.visualstudio.com/blogs/2018/03/23/text-buffer-reimplementation
// https://github.com/microsoft/vscode/tree/3cf67889583203811c81ca34bea2ad02d7c902db/src/vs/editor/common/model/pieceTreeTextBuffer
// https://github.com/microsoft/vscode-textbuffer
// https://www.unicode.org/Public/16.0.0/ucd/
// https://www.unicode.org/reports/tr44/#Canonical_Combining_Class_Values
// https://www.compart.com/en/unicode
// https://gist.github.com/fnky/458719343aabd01cfb17a3a4f7296797
// https://en.wikipedia.org/wiki/ANSI_escape_code#Terminal_input_sequences
// https://vt100.net/docs/vt100-ug/contents.html
// https://vt100.net/emu/dec_ansi_parser
// https://en.wikipedia.org/wiki/ANSI_escape_code#Fe_Escape_sequences
// https://vt100.net/annarbor/aaa-ug/section13.html#:~:text=These%20Standards%20define%20a%20set,(ASCII)%20and%20ANSI%20X3.

#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
#   define SFCE_PLATFORM_WINDOWS
#elif defined(__linux__) || defined(__linux) || defined(linux)
#   define SFCE_PLATFORM_LINUX
#elif defined(__APPLE__) || defined(__MACH__)
#   define SFCE_PLATFORM_APPLE
#elif defined(__FreeBSD__)
#   define SFCE_PLATFORM_FREE_BSD
#endif

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
// #include <stdbool.h>

#include <assert.h>
#include <locale.h>
#include <math.h>

#if defined(SFCE_PLATFORM_WINDOWS)
// #  define _UNICODE
// #  define UNICODE
#  include <windows.h>
#  include <wincon.h>
#  include <conio.h>
#else
#  include <unistd.h>
#  include <sys/uio.h>
#endif

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) < (b) ? (b) : (a))
#define CLAMP(value, min, max) ((value) < (min) ? (min) : (value) > (max) ? (max) : (value))
#define CTRL(character) ((character) - 64)

enum {
    SFCE_TRUE = 1,
    SFCE_FALSE = 0,
};

enum {
    FNV_PRIME = 0x00000100000001b3,
    FNV_OFFSET_BASIS = 0xcbf29ce484222325,
};

enum { SFCE_DEFAULT_TAB_SIZE = 4 };
enum { SFCE_STRING_BUFFER_SIZE_THRESHOLD = 0xFFFF };
enum { SFCE_EDITOR_STYLE_BUCKET_COUNT = 0x100 };

//
// NOTE: All allocation sizes must be powers of two.
// Inorder for the "round_multiple_of_two" function
// to work correctly.
//
enum {
    SFCE_LINE_STARTS_ALLOCATION_SIZE = 16,
    SFCE_STRING_BUFFER_ALLOCATION_SIZE = 16,
    SFCE_SNAPSHOT_ALLOCATION_SIZE = 16,
    SFCE_STRING_ALLOCATION_SIZE = 256,
};

#define SFCE_ERROR_CODE(o)\
    o(SFCE_ERROR_OK)\
    o(SFCE_ERROR_NULL_POINTER)\
    o(SFCE_ERROR_BUFFER_OVERFLOW)\
    o(SFCE_ERROR_UNABLE_TO_OPEN_FILE)\
    o(SFCE_ERROR_NEGATIVE_BUFFER_SIZE)\
    o(SFCE_ERROR_OUT_OF_MEMORY)\
    o(SFCE_ERROR_OUT_OF_BOUNDS)\
    o(SFCE_ERROR_FAILED_INSERTION)\
    o(SFCE_ERROR_FAILED_ERASURE)\
    o(SFCE_ERROR_FAILED_CONSOLE_READ)\
    o(SFCE_ERROR_FAILED_CONSOLE_WRITE)\
    o(SFCE_ERROR_FAILED_FILE_READ)\
    o(SFCE_ERROR_FAILED_FILE_WRITE)\
    o(SFCE_ERROR_FAILED_WIN32_API_CALL)\
    o(SFCE_ERROR_FAILED_UNIX_API_CALL)\
    o(SFCE_ERROR_UNABLE_TO_CREATE_FILE)\
    o(SFCE_ERROR_UNIMPLEMENTED)

enum sfce_error_code {
    #define o(name) name,
    SFCE_ERROR_CODE(o)
    #undef o
};

enum sfce_modifier {
    SFCE_MODIFIER_NONE  = 0x00,
    SFCE_MODIFIER_SHIFT = 0x01,
    SFCE_MODIFIER_ALT   = 0x02,
    SFCE_MODIFIER_CTRL  = 0x04,
    SFCE_MODIFIER_META  = 0x08,
};

enum sfce_keycode {
    SFCE_KEYCODE_A = 'A',
    SFCE_KEYCODE_B = 'B',
    SFCE_KEYCODE_C = 'C',
    SFCE_KEYCODE_D = 'D',
    SFCE_KEYCODE_E = 'E',
    SFCE_KEYCODE_F = 'F',
    SFCE_KEYCODE_G = 'G',
    SFCE_KEYCODE_H = 'H',
    SFCE_KEYCODE_I = 'I',
    SFCE_KEYCODE_J = 'J',
    SFCE_KEYCODE_K = 'K',
    SFCE_KEYCODE_L = 'L',
    SFCE_KEYCODE_M = 'M',
    SFCE_KEYCODE_N = 'N',
    SFCE_KEYCODE_O = 'O',
    SFCE_KEYCODE_P = 'P',
    SFCE_KEYCODE_Q = 'Q',
    SFCE_KEYCODE_R = 'R',
    SFCE_KEYCODE_S = 'S',
    SFCE_KEYCODE_T = 'T',
    SFCE_KEYCODE_U = 'U',
    SFCE_KEYCODE_V = 'V',
    SFCE_KEYCODE_W = 'W',
    SFCE_KEYCODE_X = 'X',
    SFCE_KEYCODE_Y = 'Y',
    SFCE_KEYCODE_Z = 'Z',
    SFCE_KEYCODE_0 = '0',
    SFCE_KEYCODE_1 = '1',
    SFCE_KEYCODE_2 = '2',
    SFCE_KEYCODE_3 = '3',
    SFCE_KEYCODE_4 = '4',
    SFCE_KEYCODE_5 = '5',
    SFCE_KEYCODE_6 = '6',
    SFCE_KEYCODE_7 = '7',
    SFCE_KEYCODE_8 = '8',
    SFCE_KEYCODE_9 = '9',

    SFCE_KEYCODE_TAB = '\x09',
    SFCE_KEYCODE_ENTER = '\x0D',
    SFCE_KEYCODE_ESCAPE = '\x1B',
    SFCE_KEYCODE_BACKSPACE = '\x7F',

    SFCE_KEYCODE_LINE_FEED = '\x0A',
    SFCE_KEYCODE_FORM_FEED = '\x0C',
    SFCE_KEYCODE_CARRIAGE_RETURN = '\x0D',

    SFCE_KEYCODE_NO_KEY_PRESS = 0x110000,
    SFCE_KEYCODE_UNKNOWN,
    SFCE_KEYCODE_ARROW_LEFT,
    SFCE_KEYCODE_ARROW_RIGHT,
    SFCE_KEYCODE_ARROW_UP,
    SFCE_KEYCODE_ARROW_DOWN,
    SFCE_KEYCODE_END,
    SFCE_KEYCODE_HOME,
    SFCE_KEYCODE_PAGE_UP,
    SFCE_KEYCODE_PAGE_DOWN,
    SFCE_KEYCODE_DELETE,
    SFCE_KEYCODE_INSERT,
    SFCE_KEYCODE_NUMPAD_5,
    SFCE_KEYCODE_F1,
    SFCE_KEYCODE_F2,
    SFCE_KEYCODE_F3,
    SFCE_KEYCODE_F4,
    SFCE_KEYCODE_F5,
    SFCE_KEYCODE_F6,
    SFCE_KEYCODE_F7,
    SFCE_KEYCODE_F8,
    SFCE_KEYCODE_F9,
    SFCE_KEYCODE_F10,
    SFCE_KEYCODE_F11,
    SFCE_KEYCODE_F12,
    SFCE_KEYCODE_COUNT,
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

enum sfce_console_attribute {
    SFCE_CONSOLE_ATTRIBUTE_NONE = 0, // reset all modes (styles and colors)
    SFCE_CONSOLE_ATTRIBUTE_BOLD = 1, // set bold mode.
    SFCE_CONSOLE_ATTRIBUTE_DIM_FAINT = 2, // set dim/faint mode.
    SFCE_CONSOLE_ATTRIBUTE_ITALIC = 3, // set italic mode.
    SFCE_CONSOLE_ATTRIBUTE_UNDERLINE = 4, // set underline mode.
    SFCE_CONSOLE_ATTRIBUTE_BLINKING = 5, // set blinking mode
    SFCE_CONSOLE_ATTRIBUTE_INVERSE_REVERSE = 7, // set inverse/reverse mode
    SFCE_CONSOLE_ATTRIBUTE_HIDDEN_INVISIBLE = 8, // set hidden/invisible mode
    SFCE_CONSOLE_ATTRIBUTE_STRIKETHROUGH = 9, // set strikethrough mode.
};

// https://www.compart.com/en/unicode/category
enum sfce_unicode_category {
    SFCE_UNICODE_CATEGORY_CN = 0, // Other, not assigned
    SFCE_UNICODE_CATEGORY_CC = 1, // Control
    SFCE_UNICODE_CATEGORY_CF = 2, // Format
    SFCE_UNICODE_CATEGORY_CO = 3, // Private Use
    SFCE_UNICODE_CATEGORY_CS = 4, // Surrogate
    SFCE_UNICODE_CATEGORY_LL = 5, // Lowercase Letter 2,15
    SFCE_UNICODE_CATEGORY_LM = 6, // Modifier Letter
    SFCE_UNICODE_CATEGORY_LO = 7, // Other Letter 127,00
    SFCE_UNICODE_CATEGORY_LT = 8, // Titlecase Letter
    SFCE_UNICODE_CATEGORY_LU = 9, // Uppercase Letter 1,79
    SFCE_UNICODE_CATEGORY_MC = 10, // Spacing Mark
    SFCE_UNICODE_CATEGORY_ME = 11, // Enclosing Mark
    SFCE_UNICODE_CATEGORY_MN = 12, // Nonspacing Mark  1,83
    SFCE_UNICODE_CATEGORY_ND = 13, // Decimal Number
    SFCE_UNICODE_CATEGORY_NL = 14, // Letter Number
    SFCE_UNICODE_CATEGORY_NO = 15, // Other Number
    SFCE_UNICODE_CATEGORY_PC = 16, // Connector Punctuation
    SFCE_UNICODE_CATEGORY_PD = 17, // Dash Punctuation
    SFCE_UNICODE_CATEGORY_PE = 18, // Close Punctuation
    SFCE_UNICODE_CATEGORY_PF = 19, // Final Punctuation
    SFCE_UNICODE_CATEGORY_PI = 20, // Initial Punctuation
    SFCE_UNICODE_CATEGORY_PO = 21, // Other Punctuation
    SFCE_UNICODE_CATEGORY_PS = 22, // Open Punctuation
    SFCE_UNICODE_CATEGORY_SC = 23, // Currency Symbol
    SFCE_UNICODE_CATEGORY_SK = 24, // Modifier Symbol
    SFCE_UNICODE_CATEGORY_SM = 25, // Math Symbol
    SFCE_UNICODE_CATEGORY_SO = 26, // Other Symbol 6,43
    SFCE_UNICODE_CATEGORY_ZL = 27, // Line Separator
    SFCE_UNICODE_CATEGORY_ZP = 28, // Paragraph Separator
    SFCE_UNICODE_CATEGORY_ZS = 29, // Space Separator
};

// https://www.compart.com/en/unicode/bidiclass
enum sfce_unicode_bidi_class {
    SFCE_UNICODE_BIDI_CLASS_NONE = 0, // Represents a unicode codepoint without a bidirectional class
    SFCE_UNICODE_BIDI_CLASS_AL   = 1, // Arabic Letter
    SFCE_UNICODE_BIDI_CLASS_AN   = 2, // Arabic Number
    SFCE_UNICODE_BIDI_CLASS_B    = 3, // Paragraph Separator
    SFCE_UNICODE_BIDI_CLASS_BN   = 4, // Boundary Neutral
    SFCE_UNICODE_BIDI_CLASS_CS   = 5, // Common Separator
    SFCE_UNICODE_BIDI_CLASS_EN   = 6, // European Number
    SFCE_UNICODE_BIDI_CLASS_ES   = 7, // European Separator
    SFCE_UNICODE_BIDI_CLASS_ET   = 8, // European Terminator
    SFCE_UNICODE_BIDI_CLASS_FSI  = 9, // First Strong Isolate
    SFCE_UNICODE_BIDI_CLASS_L    = 10, // Left To Right
    SFCE_UNICODE_BIDI_CLASS_LRE  = 11, // Left To Right Embedding
    SFCE_UNICODE_BIDI_CLASS_LRI  = 12, // Left To Right Isolate
    SFCE_UNICODE_BIDI_CLASS_LRO  = 13, // Left To Right Override
    SFCE_UNICODE_BIDI_CLASS_NSM  = 14, // Nonspacing Mark
    SFCE_UNICODE_BIDI_CLASS_ON   = 15, // Other Neutral
    SFCE_UNICODE_BIDI_CLASS_PDF  = 16, // Pop Directional Format
    SFCE_UNICODE_BIDI_CLASS_PDI  = 17, // Pop Directional Isolate
    SFCE_UNICODE_BIDI_CLASS_R    = 18, // Right To Left
    SFCE_UNICODE_BIDI_CLASS_RLE  = 19, // Right To Left Embedding
    SFCE_UNICODE_BIDI_CLASS_RLI  = 20, // Right To Left Isolate
    SFCE_UNICODE_BIDI_CLASS_RLO  = 21, // Right To Left Override
    SFCE_UNICODE_BIDI_CLASS_S    = 22, // Segment Separator
    SFCE_UNICODE_BIDI_CLASS_WS   = 23, // White Space
};

// https://www.compart.com/en/unicode/combining
enum sfce_unicode_decomposition {
    SFCE_UNICODE_DECOMPOSITION_NONE     = 0, // Represents a unicode codepoint without a decomposition type
    SFCE_UNICODE_DECOMPOSITION_CIRCLE   = 1, // <circle>    Encircled form
    SFCE_UNICODE_DECOMPOSITION_COMPAT   = 2, // <compat>    Otherwise unspecified compatibility character
    SFCE_UNICODE_DECOMPOSITION_FINAL    = 3, // <final> Final presentation form (Arabic)
    SFCE_UNICODE_DECOMPOSITION_FONT     = 4, // <font>  Font variant
    SFCE_UNICODE_DECOMPOSITION_FRACTION = 5, // <fraction>  Vulgar fraction form
    SFCE_UNICODE_DECOMPOSITION_INITIAL  = 6, // <initial>   Initial presentation form (Arabic)
    SFCE_UNICODE_DECOMPOSITION_ISOLATED = 7, // <isolated>  Isolated presentation form (Arabic)
    SFCE_UNICODE_DECOMPOSITION_MEDIAL   = 8, // <medial>    Medial presentation form (Arabic)
    SFCE_UNICODE_DECOMPOSITION_NARROW   = 9, // <narrow>    Narrow (or hankaku) compatibility character
    SFCE_UNICODE_DECOMPOSITION_NOBREAK  = 10, // <noBreak>   No-break version of a space or hyphen
    SFCE_UNICODE_DECOMPOSITION_SMALL    = 11, // <small> Small variant form (CNS compatibility)
    SFCE_UNICODE_DECOMPOSITION_SQUARE   = 12, // <square>    CJK squared font variant
    SFCE_UNICODE_DECOMPOSITION_SUB      = 13, // <sub>   Subscript form
    SFCE_UNICODE_DECOMPOSITION_SUPER    = 14, // <super> Superscript form
    SFCE_UNICODE_DECOMPOSITION_VERTICAL = 15, // <vertical>  Vertical layout presentation form
    SFCE_UNICODE_DECOMPOSITION_WIDE     = 16, // <wide>  Wide (or zenkaku) compatibility character
};

enum sfce_split_kind {
    SFCE_SPLIT_NONE,
    SFCE_SPLIT_HORIZONTAL,
    SFCE_SPLIT_VERTICAL,
};

struct sfce_window_size {
    int32_t width;
    int32_t height;
};

struct sfce_string {
    uint8_t *data;
    int32_t  size;
    int32_t  capacity;
};

struct sfce_string_view {
    const uint8_t *data;
    int32_t        size;
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

struct sfce_buffer_position {
    int32_t line_start_index;
    int32_t column;
};

struct sfce_piece {
    struct sfce_buffer_position start;
    struct sfce_buffer_position end;
    uint32_t                    buffer_index;
    int32_t                     line_count;
    int32_t                     length;
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
    int32_t                 offset_within_piece;
};

struct sfce_position {
    int32_t col;
    int32_t row;
};

struct sfce_piece_tree {
    struct sfce_piece_node    *root;
    struct sfce_string_buffer *buffers;
    int32_t                    buffer_count;
    int32_t                    buffer_capacity;
    int32_t                    line_count;
    int32_t                    length;
    int32_t                    change_buffer_index;
};

struct sfce_piece_tree_snapshot {
    struct sfce_piece *pieces;
    int32_t            piece_count;
    int32_t            piece_capacity;
};

struct sfce_console_state {
#if defined(SFCE_PLATFORM_WINDOWS)
    HANDLE                       input_handle;
    HANDLE                       output_handle;
    DWORD                        output_mode;
    DWORD                        input_mode;
    UINT                         input_code_page;
    UINT                         output_code_page;
    CONSOLE_SCREEN_BUFFER_INFOEX console_screen_buffer_info;
    CONSOLE_FONT_INFOEX          console_font_info;
#elif defined(SFCE_PLATFORM_LINUX)
#elif defined(SFCE_PLATFORM_APPLE)
#elif defined(SFCE_PLATFORM_FREE_BSD)
#endif
};

struct sfce_console_style {
    uint32_t foreground;
    uint32_t background;
    uint32_t attributes;
};

struct sfce_console_cell {
    int32_t                   codepoint;
    struct sfce_console_style style;
};

struct sfce_console_buffer {
    struct sfce_console_state save_state;
    struct sfce_string        temp_print_string;
    struct sfce_string        command;
    struct sfce_console_cell *cells;
    struct sfce_window_size   window_size;
    int32_t                   tab_size;
    unsigned                  use_truecolor: 1;
};

struct sfce_utf8_property {
    enum sfce_unicode_category      category;
    int16_t                         combining_class;
    enum sfce_unicode_bidi_class    bidi_class;
    enum sfce_unicode_decomposition decomposition;
    // int32_t                         decomposition_mapping;
    int32_t                         uppercase_mapping;
    int32_t                         lowercase_mapping;
    int32_t                         titlecase_mapping;
    unsigned                        width: 2;
    unsigned                        byte_count: 3;
    unsigned                        bidi_mirrored: 1;
};

struct sfce_rectangle {
    int32_t left;
    int32_t top;
    int32_t right;
    int32_t bottom;
};

struct sfce_cursor {
    struct sfce_cursor        *prev;
    struct sfce_cursor        *next;
    struct sfce_editor_window *window;
    struct sfce_piece_tree    *tree;
    struct sfce_position       position;
    struct sfce_position       anchor;
    struct sfce_string         copy_string;
    int32_t                    target_render_col;
    unsigned                   is_selecting: 1;
};

struct sfce_editor_theme {
    int32_t dummy;
};

struct sfce_keypress {
    uint32_t keycode;
    int32_t  codepoint;
    uint8_t  modifiers;
};

struct sfce_editor_style_bucket {
    struct sfce_string name;
    struct sfce_console_style style;
    struct sfce_editor_style_bucket *next;
};

struct sfce_editor_style {
    struct sfce_editor_style_bucket *buckets[SFCE_EDITOR_STYLE_BUCKET_COUNT];
};

struct sfce_editor_window {
    struct sfce_cursor          *cursors;
    uint32_t                     cursor_count;
    uint32_t                     cursor_capacity;
    const char                  *filepath;
    struct sfce_piece_tree      *tree;
    struct sfce_rectangle        rectangle;
    struct sfce_editor_window   *window0;
    struct sfce_editor_window   *window1;
    enum sfce_split_kind         split_kind;
    unsigned                     should_close: 1;
    unsigned                     enable_line_numbering: 1;
    unsigned                     enable_relative_line_numbering: 1;
};

struct sfce_editor_buffer {
    struct sfce_editor_buffer *prev;
    struct sfce_editor_buffer *next;
    const char                *filepath;
    struct sfce_piece_tree    *tree;
};

struct sfce_editor {
    struct sfce_editor_window *active_window;
    struct sfce_editor_buffer *open_buffers;
};

// struct sfce_editor_cursor {
//     struct sfce_editor_cursor *next;
//     struct sfce_editor_cursor *prev;
//     struct sfce_piece_tree    *tree;
//     struct sfce_position       position;
//     struct sfce_position       anchor;
//     struct sfce_string         copy_buffer;
//     int32_t                    virtual_col;
// };

// struct sfce_editor {
//     struct sfce_piece_tree    *tree;
//     struct sfce_editor_cursor *cursors;
//     struct sfce_string         filepath;
//     struct sfce_string         status_message;

//     int8_t                     should_close: 1;
//     int8_t                     use_relative_line_numbering: 1;
//     int8_t                     disable_cursor_scroll: 1;
//     int8_t                     display_line_numbers: 1;
//     int8_t                     indent_using_spaces: 1;
//     int8_t                     auto_close_brace: 1;
//     int8_t                     auto_indent: 1;
//     int8_t                     display_status: 1;
// };

uint64_t fnv1a(uint64_t hash, uint8_t byte);
int32_t round_multiple_of_two(int32_t value, int32_t multiple);
int32_t newline_sequence_size(const uint8_t *buffer, int32_t buffer_size);
int32_t buffer_newline_count(const uint8_t *buffer, int32_t buffer_size);
const char *make_character_printable(int32_t character);

enum sfce_error_code sfce_write(const void *buffer, int32_t buffer_size);
enum sfce_error_code sfce_write_zero_terminated_string(const void *buffer);
enum sfce_error_code sfce_get_console_screen_size(struct sfce_window_size *window_size);
enum sfce_error_code sfce_enable_console_temp_buffer();
enum sfce_error_code sfce_disable_console_temp_buffer();
int32_t sfce_parse_csi_parameter(int32_t *character);
struct sfce_keypress sfce_get_keypress();

enum sfce_unicode_category sfce_codepoint_category(int32_t codepoint);
int32_t sfce_codepoint_to_upper(int32_t codepoint);
int32_t sfce_codepoint_to_lower(int32_t codepoint);
uint8_t sfce_codepoint_width(int32_t codepoint);
uint8_t sfce_codepoint_utf8_continuation(uint8_t byte);
uint8_t sfce_codepoint_encode_utf8(int32_t codepoint, uint8_t *buffer);
int32_t sfce_codepoint_decode_utf8(const void *buffer, int32_t byte_count);
uint8_t sfce_codepoint_utf8_byte_count(int32_t codepoint);
int8_t sfce_codepoint_is_print(int32_t codepoint);

enum sfce_error_code sfce_save_console_state(struct sfce_console_state *state);
enum sfce_error_code sfce_restore_console_state(struct sfce_console_state *state);
enum sfce_error_code sfce_enable_virtual_terminal(const struct sfce_console_state *state);
enum sfce_error_code sfce_setup_console(struct sfce_console_state *state);

void sfce_string_destroy(struct sfce_string *string);
void sfce_string_clear(struct sfce_string *string);
enum sfce_error_code sfce_string_reserve(struct sfce_string *result, int32_t capacity);
enum sfce_error_code sfce_string_resize(struct sfce_string *result, int32_t size);
enum sfce_error_code sfce_string_write(struct sfce_string *string, int32_t index, const void *buffer, int32_t buffer_size);
enum sfce_error_code sfce_string_insert(struct sfce_string *string, int32_t index, const void *buffer, int32_t buffer_size);
enum sfce_error_code sfce_string_push_back_byte(struct sfce_string *string, uint8_t byte);
enum sfce_error_code sfce_string_push_back_buffer(struct sfce_string *string, const void *buffer, int32_t buffer_size);
enum sfce_error_code sfce_string_push_back_codepoint(struct sfce_string *string, int32_t codepoint);
enum sfce_error_code sfce_string_nprintf(struct sfce_string *string, int32_t max_length, const void *format, ...);
enum sfce_error_code sfce_string_vnprintf(struct sfce_string *string, int32_t max_length, const void *format, va_list va_args);
enum sfce_error_code sfce_string_to_upper_case(const struct sfce_string *string, struct sfce_string *result_string);
enum sfce_error_code sfce_string_to_lower_case(const struct sfce_string *string, struct sfce_string *result_string);
// enum sfce_error_code sfce_string_to_snake_case(const struct sfce_string *string, struct sfce_string *result_string);
// enum sfce_error_code sfce_string_to_kebab_case(const struct sfce_string *string, struct sfce_string *result_string);
// enum sfce_error_code sfce_string_to_title_case(const struct sfce_string *string, struct sfce_string *result_string);
// enum sfce_error_code sfce_string_to_camel_case(const struct sfce_string *string, struct sfce_string *result_string);
// enum sfce_error_code sfce_string_to_pascal_case(const struct sfce_string *string, struct sfce_string *result_string);
int16_t sfce_string_compare(struct sfce_string string0, struct sfce_string string1);

void sfce_line_starts_destroy(struct sfce_line_starts *lines);
enum sfce_error_code sfce_line_starts_reserve(struct sfce_line_starts *lines, int32_t capacity);
enum sfce_error_code sfce_line_starts_resize(struct sfce_line_starts *lines, int32_t size);
enum sfce_error_code sfce_line_starts_push_line_offset(struct sfce_line_starts *lines, int32_t offset);
struct sfce_buffer_position sfce_line_starts_search_for_position(struct sfce_line_starts buffer, int32_t low_line_index, int32_t high_line_index, int32_t offset);

void sfce_string_buffer_destroy(struct sfce_string_buffer *buffer);
enum sfce_error_code sfce_string_buffer_recount_line_start_offsets(struct sfce_string_buffer *buffer, int32_t offset_begin, int32_t offset_end);
enum sfce_error_code sfce_string_buffer_append_content(struct sfce_string_buffer *buffer, const uint8_t *data, int32_t size);
struct sfce_buffer_position sfce_string_buffer_get_end_position(struct sfce_string_buffer *buffer);
struct sfce_buffer_position sfce_string_buffer_offset_to_position(struct sfce_string_buffer *buffer, int32_t offset);
struct sfce_buffer_position sfce_string_buffer_piece_position_in_buffer(struct sfce_string_buffer *buffer, struct sfce_piece piece, int32_t offset_within_piece);
struct sfce_buffer_position sfce_string_buffer_move_position_by_offset(struct sfce_string_buffer *buffer, struct sfce_buffer_position position, int32_t offset);
int32_t sfce_string_buffer_line_number_offset_within_piece(struct sfce_string_buffer *string_buffer, struct sfce_piece piece, int32_t lines_within_piece);
int32_t sfce_string_buffer_position_to_offset(struct sfce_string_buffer *string_buffer, struct sfce_buffer_position position);

struct sfce_piece_node *sfce_piece_node_create(struct sfce_piece piece);
void sfce_piece_node_destroy(struct sfce_piece_node *tree);
void sfce_piece_node_destroy_non_recursive(struct sfce_piece_node *tree);
int32_t sfce_piece_node_calculate_length(struct sfce_piece_node *root);
int32_t sfce_piece_node_calculate_line_count(struct sfce_piece_node *root);
int32_t sfce_piece_node_offset_from_start(struct sfce_piece_node *node);
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
void sfce_piece_node_recompute_piece_length(struct sfce_piece_tree *tree, struct sfce_piece_node *node);
void sfce_piece_node_print(struct sfce_piece_tree *tree, struct sfce_piece_node *root, int32_t space);
void sfce_piece_node_inorder_print(struct sfce_piece_tree *tree, struct sfce_piece_node *root);
void sfce_piece_node_to_string(struct sfce_piece_tree *tree, struct sfce_piece_node *node, int32_t space, struct sfce_string *out);
void sfce_piece_node_inorder_print_to_string(struct sfce_piece_tree *tree, struct sfce_piece_node *root, struct sfce_string *out);
void sfce_piece_node_reset_sentinel();

struct sfce_node_position sfce_node_position_move_by_offset(struct sfce_node_position position, int32_t offset);
// uint8_t sfce_node_position_get_byte(struct sfce_node_position position);

struct sfce_piece_tree *sfce_piece_tree_create();
void sfce_piece_tree_destroy(struct sfce_piece_tree *tree);
int32_t sfce_piece_tree_line_offset_in_piece(struct sfce_piece_tree *tree, struct sfce_piece piece, int32_t line_number);
int32_t sfce_piece_tree_count_lines_in_piece_until_offset(struct sfce_piece_tree *tree, struct sfce_piece piece, int32_t offset);
int32_t sfce_piece_tree_offset_at_position(struct sfce_piece_tree *tree, const struct sfce_position position);
int32_t sfce_piece_tree_get_codepoint_from_node_position(struct sfce_piece_tree *tree, struct sfce_node_position node_position);
int32_t sfce_piece_tree_get_character_length_from_node_position(struct sfce_piece_tree *tree, struct sfce_node_position start);
int32_t sfce_piece_tree_get_line_length(struct sfce_piece_tree *tree, int32_t row);
uint8_t sfce_piece_tree_get_byte_at_node_position(struct sfce_piece_tree *tree, struct sfce_node_position node_position);
int32_t sfce_piece_tree_read_into_buffer(struct sfce_piece_tree *tree, struct sfce_node_position start, struct sfce_node_position end, int32_t buffer_size, uint8_t *buffer);
int32_t sfce_piece_tree_get_column_from_render_column(struct sfce_piece_tree *tree, int32_t row, int32_t target_render_col);
int32_t sfce_piece_tree_get_render_column_from_column(struct sfce_piece_tree *tree, int32_t row, int32_t col);
// int32_t sfce_piece_tree_character_at_node_position(struct sfce_piece_tree *tree, struct sfce_node_position position);
// int32_t sfce_piece_tree_codepoint_at_node_position(struct sfce_piece_tree *tree, struct sfce_node_position position);
struct sfce_position sfce_piece_tree_position_at_offset(struct sfce_piece_tree *tree, int32_t offset);
struct sfce_position sfce_piece_tree_move_position_by_offset(struct sfce_piece_tree *tree, struct sfce_position position, int32_t offset);
struct sfce_node_position sfce_piece_tree_node_at_offset(struct sfce_piece_tree *tree, int32_t offset);
struct sfce_node_position sfce_piece_tree_node_at_position(struct sfce_piece_tree *tree, int32_t col, int32_t row);
struct sfce_string_view sfce_piece_tree_get_piece_content(const struct sfce_piece_tree *tree, struct sfce_piece piece);
enum sfce_error_code sfce_piece_tree_get_node_content(const struct sfce_piece_tree *tree, struct sfce_piece_node *node, struct sfce_string *string);
enum sfce_error_code sfce_piece_tree_get_substring(struct sfce_piece_tree *tree, int32_t offset, int32_t length, struct sfce_string *string);
enum sfce_error_code sfce_piece_tree_get_line_content(struct sfce_piece_tree *tree, int32_t line_number, struct sfce_string *string);
enum sfce_error_code sfce_piece_tree_get_content_between_node_positions(struct sfce_piece_tree *tree, struct sfce_node_position position0, struct sfce_node_position position1, struct sfce_string *string);
enum sfce_error_code sfce_piece_tree_ensure_change_buffer_size(struct sfce_piece_tree *tree, int32_t required_size);
enum sfce_error_code sfce_piece_tree_set_buffer_count(struct sfce_piece_tree *tree, int32_t buffer_count);
enum sfce_error_code sfce_piece_tree_add_string_buffer(struct sfce_piece_tree *tree, struct sfce_string_buffer string_buffer);
enum sfce_error_code sfce_piece_tree_add_new_string_buffer(struct sfce_piece_tree *tree);
enum sfce_error_code sfce_piece_tree_create_node_subtree(struct sfce_piece_tree *tree, const uint8_t *buffer, int32_t buffer_size, struct sfce_piece_node **result);
enum sfce_error_code sfce_piece_tree_create_piece(struct sfce_piece_tree *tree, const void *data, int32_t byte_count, struct sfce_piece *result_piece);
enum sfce_error_code sfce_piece_tree_insert_with_offset(struct sfce_piece_tree *tree, int32_t offset, const uint8_t *data, int32_t byte_count);
enum sfce_error_code sfce_piece_tree_erase_with_offset(struct sfce_piece_tree *tree, int32_t offset, int32_t byte_count);
enum sfce_error_code sfce_piece_tree_insert_with_position(struct sfce_piece_tree *tree, struct sfce_position position, const uint8_t *data, int32_t byte_count);
enum sfce_error_code sfce_piece_tree_erase_with_position(struct sfce_piece_tree *tree, struct sfce_position position, int32_t byte_count);
enum sfce_error_code sfce_piece_tree_insert_left_of_node(struct sfce_piece_tree *tree, struct sfce_piece_node *node, const uint8_t *data, int32_t byte_count);
enum sfce_error_code sfce_piece_tree_insert_right_of_node(struct sfce_piece_tree *tree, struct sfce_piece_node *node, const uint8_t *data, int32_t byte_count);
enum sfce_error_code sfce_piece_tree_insert_middle_of_node_position(struct sfce_piece_tree *tree, struct sfce_node_position where, const uint8_t *data, int32_t byte_count);
enum sfce_error_code sfce_piece_tree_insert_with_node_position(struct sfce_piece_tree *tree, struct sfce_node_position position, const uint8_t *data, int32_t byte_count);
enum sfce_error_code sfce_piece_tree_erase_with_node_position(struct sfce_piece_tree *tree, struct sfce_node_position start, struct sfce_node_position end);
enum sfce_error_code sfce_piece_tree_write_to_file(struct sfce_piece_tree *tree, const char *filepath);
enum sfce_error_code sfce_piece_tree_load_file(struct sfce_piece_tree *tree, const char *filepath);
enum sfce_error_code sfce_piece_tree_create_snapshot(struct sfce_piece_tree *tree, struct sfce_piece_tree_snapshot *snapshot);
enum sfce_error_code sfce_piece_tree_from_snapshot(struct sfce_piece_tree *tree, struct sfce_piece_tree_snapshot *snapshot);
void sfce_piece_tree_recompute_metadata(struct sfce_piece_tree *tree);

enum sfce_error_code sfce_piece_tree_snapshot_set_piece_count(struct sfce_piece_tree_snapshot *snapshot, int32_t count);
enum sfce_error_code sfce_piece_tree_snapshot_add_piece(struct sfce_piece_tree_snapshot *snapshot, struct sfce_piece piece);

void sfce_console_buffer_destroy(struct sfce_console_buffer *console);
void sfce_console_buffer_clear(struct sfce_console_buffer *console, struct sfce_console_style style);
enum sfce_error_code sfce_console_buffer_create(struct sfce_console_buffer *console);
enum sfce_error_code sfce_console_buffer_update(struct sfce_console_buffer *console);
enum sfce_error_code sfce_console_buffer_nprintf(struct sfce_console_buffer *console, int32_t col, int32_t row, struct sfce_console_style style, int32_t max_length, const char *format, ...);
enum sfce_error_code sfce_console_buffer_print_string(struct sfce_console_buffer *console, int32_t col, int32_t row, struct sfce_console_style style, const void *string, uint32_t length);
enum sfce_error_code sfce_console_buffer_set_style(struct sfce_console_buffer *console, int32_t col, int32_t row, struct sfce_console_style style);
enum sfce_error_code sfce_console_buffer_set_cell(struct sfce_console_buffer *console, int32_t col, int32_t row, struct sfce_console_cell cell);
enum sfce_error_code sfce_console_buffer_flush(struct sfce_console_buffer *console);

void sfce_editor_window_destroy(struct sfce_editor_window *window);
enum sfce_error_code sfce_editor_window_display(struct sfce_editor_window *window, struct sfce_console_buffer *console, struct sfce_string *line_temp);

struct sfce_cursor *sfce_cursor_create(struct sfce_editor_window *window);
void sfce_cursor_destroy(struct sfce_cursor *cursor);
// void sfce_cursor_move_left(struct sfce_cursor *cursor);
// void sfce_cursor_move_right(struct sfce_cursor *cursor);
// void sfce_cursor_move_up(struct sfce_cursor *cursor);
// void sfce_cursor_move_down(struct sfce_cursor *cursor);
// uint8_t sfce_cursor_move_word_left(struct sfce_cursor *cursor);
// uint8_t sfce_cursor_move_word_right(struct sfce_cursor *cursor);
// uint8_t sfce_cursor_move_offset(struct sfce_cursor *cursor, int32_t offset);
// enum sfce_error_code sfce_cursor_insert_character(struct sfce_cursor *cursor, int32_t character);
// enum sfce_error_code sfce_cursor_insert(struct sfce_cursor *cursor, size_t size, const uint8_t *data);
// enum sfce_error_code sfce_cursor_erase_character(struct sfce_cursor *cursor);
// enum sfce_error_code sfce_cursor_erase(struct sfce_cursor *cursor, size_t size);
// int32_t sfce_cursor_get_character(const struct sfce_cursor *cursor);
// int32_t sfce_cursor_get_prev_character(const struct sfce_cursor *cursor);

void sfce_log_error(const char *format, ...);

static struct sfce_piece_node sentinel = {
    .parent = &sentinel,
    .left   = &sentinel,
    .right  = &sentinel,
    .color  = SFCE_COLOR_BLACK,
};

static struct sfce_piece_node *sentinel_ptr = &sentinel;

static const struct sfce_node_position sentinel_node_position = {
    .node = &sentinel,
    .node_start_offset = 0,
    .offset_within_piece = 0,
};

static struct sfce_string g_logging_string = {};
static int g_should_log_to_error_string = 1;

#define o(name) #name,
const char *sfce_error_code_names[] = { SFCE_ERROR_CODE(o) };
#undef o

int main(int argc, const char *argv[])
{
    enum sfce_error_code error_code;

    struct sfce_console_buffer console = {};
    error_code = sfce_console_buffer_create(&console);
    if (error_code != SFCE_ERROR_OK) {
        goto error;
    }

    struct sfce_piece_tree *tree = sfce_piece_tree_create();
    if (argc > 1) {
        error_code = sfce_piece_tree_load_file(tree, argv[1]);
        if (error_code != SFCE_ERROR_OK && error_code != SFCE_ERROR_UNABLE_TO_OPEN_FILE) {
            goto error;
        }
    }

    struct sfce_string line_contents = {};
    struct sfce_editor_window window = {
        .rectangle.left = 0,
        .rectangle.top = 0,
        .rectangle.right = console.window_size.width - 1,
        .rectangle.bottom = console.window_size.height - 1,
        .tree = tree,
        .split_kind = SFCE_SPLIT_NONE,
        .enable_line_numbering = 1,
        .enable_relative_line_numbering = 0,
        // .enable_relative_line_numbering = 1,
        .filepath = argv[1],
    };

    window.cursors = sfce_cursor_create(&window);

    struct sfce_keypress keypress = {};
    int32_t running = SFCE_TRUE;
    int32_t should_render = SFCE_TRUE;
    while (running) {
        keypress = sfce_get_keypress();
        switch (keypress.keycode) {
        case SFCE_KEYCODE_NO_KEY_PRESS: {
            goto render_console;
        }

        case SFCE_KEYCODE_ESCAPE: {
            running = SFCE_FALSE;
            continue;
        }

        case SFCE_KEYCODE_DELETE: {
            should_render = SFCE_TRUE;
            error_code = sfce_piece_tree_erase_with_position(window.tree, window.cursors->position, 1);

            if (error_code != SFCE_ERROR_OK) {
                goto error;
            }
        } break;

        case SFCE_KEYCODE_F10: {
            should_render = SFCE_TRUE;
            error_code = sfce_piece_tree_write_to_file(window.tree, window.filepath);
            if (error_code != SFCE_ERROR_OK) {
                goto error;
            }

            break;
        }

        case SFCE_KEYCODE_BACKSPACE: {
            should_render = SFCE_TRUE;
            int32_t position_offset = sfce_piece_tree_offset_at_position(window.tree, window.cursors->position);
            window.cursors->position = sfce_piece_tree_position_at_offset(window.tree, position_offset - 1);
            error_code = sfce_piece_tree_erase_with_position(window.tree, window.cursors->position, 1);

            if (error_code != SFCE_ERROR_OK) {
                goto error;
            }

            break;
        }

        case SFCE_KEYCODE_ARROW_RIGHT: {
            should_render = SFCE_TRUE;
            int32_t position_offset = sfce_piece_tree_offset_at_position(window.tree, window.cursors->position);
            struct sfce_node_position node_position = sfce_piece_tree_node_at_position(window.tree, window.cursors->position.col, window.cursors->position.row);
            int32_t character_length = sfce_piece_tree_get_character_length_from_node_position(window.tree, node_position);
            window.cursors->position = sfce_piece_tree_position_at_offset(window.tree, position_offset + character_length);

            break;
        }

        case SFCE_KEYCODE_ARROW_LEFT: {
            should_render = SFCE_TRUE;

            struct sfce_node_position node_position = sfce_piece_tree_node_at_position(
                window.tree,
                window.cursors->position.col,
                window.cursors->position.row
            );

            sfce_node_position_move_by_offset(node_position, -1);

            uint8_t byte;
            do {
                sfce_node_position_move_by_offset(node_position, -1);
                byte = sfce_piece_tree_get_byte_at_node_position(window.tree, node_position);
            } while (sfce_codepoint_utf8_continuation(byte));

            window.cursors->position = sfce_piece_tree_position_at_offset(window.tree, node_position.node_start_offset + node_position.offset_within_piece);

            // int32_t position_offset = sfce_piece_tree_offset_at_position(window.tree, window.cursors->position);
            // window.cursors->position = sfce_piece_tree_position_at_offset(window.tree, position_offset - 1);

            break;
        }

        case SFCE_KEYCODE_ARROW_UP: {
            should_render = SFCE_TRUE;
            window.cursors->position.row = MAX(window.cursors->position.row - 1, 0);

            break;
        }

        case SFCE_KEYCODE_ARROW_DOWN: {
            should_render = SFCE_TRUE;
            window.cursors->position.row = MIN(window.cursors->position.row + 1, window.tree->line_count - 1);

            break;
        }

        default: {
            should_render = SFCE_TRUE;

            uint8_t buffer[4] = {};
            int32_t buffer_length = sfce_codepoint_encode_utf8(keypress.codepoint, buffer);
            error_code = sfce_piece_tree_insert_with_position(tree, window.cursors->position, buffer, buffer_length);
            int32_t codepoint_length = sfce_codepoint_utf8_byte_count(keypress.codepoint);
            int32_t position_offset = sfce_piece_tree_offset_at_position(window.tree, window.cursors->position);
            window.cursors->position = sfce_piece_tree_position_at_offset(window.tree, position_offset + codepoint_length);
            if (error_code != SFCE_ERROR_OK) {
                goto error;
            }

            break;
        }
        }

render_console:
        if (should_render) {
            should_render = SFCE_FALSE;

            error_code = sfce_console_buffer_update(&console);
            if (error_code != SFCE_ERROR_OK) {
                goto error;
            }

            g_should_log_to_error_string = 0;
            error_code = sfce_editor_window_display(&window, &console, &line_contents);
            g_should_log_to_error_string = 1;
            if (error_code != SFCE_ERROR_OK) {
                goto error;
            }

            error_code = sfce_console_buffer_flush(&console);
            if (error_code != SFCE_ERROR_OK) {
                goto error;
            }
        }
    }

    sfce_string_destroy(&line_contents);

    sfce_console_buffer_destroy(&console);
    // sfce_piece_node_print(window.tree, window.tree->root, 0);
    fprintf(stderr, "Log string: \"%.*s\"", g_logging_string.size, g_logging_string.data);

    sfce_string_destroy(&g_logging_string);

    return 0;

error:
    sfce_console_buffer_destroy(&console);
    fprintf(stderr, "ERROR CODE: %s\n", sfce_error_code_names[error_code]);
    return -1;
}

uint64_t fnv1a(uint64_t hash, uint8_t byte)
{
    return (hash ^ byte) * FNV_PRIME;
}

int32_t round_multiple_of_two(int32_t value, int32_t multiple)
{
    return (value + multiple - 1) & -multiple;
}

// 
// TODO: Fix newline scanning withing the piece tree where
// the piece tree accept multiple different newline types
// within a single file.
// 
int32_t newline_sequence_size(const uint8_t *buffer, int32_t buffer_size)
{
    if (buffer_size > 0) {
        if (buffer[0] == '\r') {
            return (buffer_size > 1 && buffer[1] == '\n') ? 2 : 1;
        }

        if (buffer[0] == '\n') {
            return 1;
        }
    }

    return 0;
}

int32_t buffer_newline_count(const uint8_t *buffer, int32_t buffer_size)
{
    int32_t newline_count = 0;

    const uint8_t *buffer_end = buffer + buffer_size;
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

const char *make_character_printable(int32_t character)
{
    static char buffer[16] = {0};

    switch (character)
    {
        case '\n': return "\\n";
        case '\r': return "\\r";
        case '\t': return "\\t";
        case '\b': return "\\b";
        case '\f': return "\\f";
        case '\v': return "\\v";
        case '\0': return "\\0";
    }

    if (character >= 32 && character <= 126) {
        buffer[0] = (char)character;
        buffer[1] = 0;
        return buffer;
    }

    if (character <= 0xFF) {
        snprintf(buffer, sizeof(buffer), "\\x%02X", character);
    }
    else if (character <= 0xFFFF) {
        snprintf(buffer, sizeof(buffer), "\\x%04X", character);
    }
    else if (character <= 0x7FFFFFFF) {
        snprintf(buffer, sizeof(buffer), "\\x%08X", character);
    }

    return buffer;
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

enum sfce_error_code sfce_write_zero_terminated_string(const void *buffer)
{
    return sfce_write(buffer, strlen((const char *)buffer));
}

enum sfce_error_code sfce_get_console_screen_size(struct sfce_window_size *window_size)
{
    // enum sfce_error_code error_code = sfce_write_zero_terminated_string("\x1b[s\x1b[32767;32767H");
    // enum sfce_error_code error_code = sfce_write_zero_terminated_string("\x1b[s\x1b[32767C\x1b[32767B");
    enum sfce_error_code error_code = sfce_write_zero_terminated_string("\x1b[7\x1b[32767C\x1b[32767B");
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
    window_size->width = 0;
    window_size->height = 0;
#endif

    error_code = sfce_write_zero_terminated_string("\x1b[8");
    // error_code = sfce_write_zero_terminated_string("\x1b[u");
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


int32_t sfce_parse_csi_parameter(int32_t *character)
{
    int32_t parameter = 0;

start:
    switch (*character) {
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': {
        int32_t digit_value = *character - '0';
        parameter = 10 * parameter + digit_value;
    } break;
    case '<':
    case '=':
    case '>':
    case '?':
    case ':':
        //
        // TODO: Implement the required logic for intermediate parameter bytes
        //
        break;
    case ';':
        goto done;
        // if (kbhit()) *character = getch();
        // goto done;
    }

    if (kbhit()) {
        *character = getch();
        goto start;
    }

done:
    return parameter;
}

//
// TODO: This function still doesn't handle SS3 and SS2
// input control sequences, for compatibility with older
// consoles this function has to be able to handle
// SS3 and SS2 sequences.
//
struct sfce_keypress sfce_get_keypress()
{
    static const struct sfce_keypress NO_KEYPRESS = { SFCE_KEYCODE_NO_KEY_PRESS, -1, 0 };

     if (!kbhit()) {
        return NO_KEYPRESS;
    }

    int32_t character = getch();
    switch (character) {
    csi_begin: case '\x9B': {
        int32_t parameter = 0, modifiers = 0;

        character = getch();
        if (isdigit(character)) {
            parameter = sfce_parse_csi_parameter(&character);

            if (character == ';') {
                character = getch();
                modifiers = sfce_parse_csi_parameter(&character) - 1;
            }

            if (character == '~') {
                switch (parameter) {
                case 5: return (struct sfce_keypress) { SFCE_KEYCODE_PAGE_UP, -1, modifiers };
                case 6: return (struct sfce_keypress) { SFCE_KEYCODE_PAGE_DOWN, -1, modifiers };
                case 2: return (struct sfce_keypress) { SFCE_KEYCODE_INSERT, -1, modifiers };
                case 3: return (struct sfce_keypress) { SFCE_KEYCODE_DELETE, -1, modifiers };

                case 15: return (struct sfce_keypress) { SFCE_KEYCODE_F5, -1, modifiers };
                case 17: return (struct sfce_keypress) { SFCE_KEYCODE_F6, -1, modifiers };
                case 18: return (struct sfce_keypress) { SFCE_KEYCODE_F7, -1, modifiers };
                case 19: return (struct sfce_keypress) { SFCE_KEYCODE_F8, -1, modifiers };
                case 20: return (struct sfce_keypress) { SFCE_KEYCODE_F9, -1, modifiers };
                case 21: return (struct sfce_keypress) { SFCE_KEYCODE_F10, -1, modifiers };
                case 24: return (struct sfce_keypress) { SFCE_KEYCODE_F12, -1, modifiers };
                }

                return NO_KEYPRESS;
            }
        }

        switch (character) {
        case 'A': return (struct sfce_keypress) { SFCE_KEYCODE_ARROW_UP, -1, modifiers };
        case 'B': return (struct sfce_keypress) { SFCE_KEYCODE_ARROW_DOWN, -1, modifiers };
        case 'C': return (struct sfce_keypress) { SFCE_KEYCODE_ARROW_RIGHT, -1, modifiers };
        case 'D': return (struct sfce_keypress) { SFCE_KEYCODE_ARROW_LEFT, -1, modifiers };
        case 'G':
        case 'E': return (struct sfce_keypress) { SFCE_KEYCODE_NUMPAD_5, -1, modifiers };
        case 'F': return (struct sfce_keypress) { SFCE_KEYCODE_END, -1, modifiers };
        case 'H': return (struct sfce_keypress) { SFCE_KEYCODE_HOME, -1, modifiers };
        case 'P': return (struct sfce_keypress) { SFCE_KEYCODE_F1, -1, modifiers };
        case 'Q': return (struct sfce_keypress) { SFCE_KEYCODE_F2, -1, modifiers };
        case 'R': return (struct sfce_keypress) { SFCE_KEYCODE_F3, -1, modifiers };
        case 'S': return (struct sfce_keypress) { SFCE_KEYCODE_F4, -1, modifiers };
        }
    } break;

    single_shift_two: case '\x8E': {
        while (kbhit()) getch();
    } break;

    single_shift_three: case '\x8f': {
        character = getch();
        if (isdigit(character)) {
            int32_t parameter = sfce_parse_csi_parameter(&character);
            if (parameter == 1 && character == ';') {
                int32_t modifiers = getch() - 1;
                int32_t codepoint = sfce_parse_csi_parameter(&character);
                return (struct sfce_keypress) { codepoint, codepoint, modifiers };
            }
        }

        switch (character) {
        case 'P': return (struct sfce_keypress) { SFCE_KEYCODE_F1, -1, SFCE_MODIFIER_NONE };
        case 'Q': return (struct sfce_keypress) { SFCE_KEYCODE_F2, -1, SFCE_MODIFIER_NONE };
        case 'R': return (struct sfce_keypress) { SFCE_KEYCODE_F3, -1, SFCE_MODIFIER_NONE };
        case 'S': return (struct sfce_keypress) { SFCE_KEYCODE_F4, -1, SFCE_MODIFIER_NONE };
        }
    } break;

    case '\x1b':
        if (!kbhit()) {
            return (struct sfce_keypress) { SFCE_KEYCODE_ESCAPE, '\x1b', 0 };
        }

        character = getch();
        switch (character) {
        case '\x1b': return (struct sfce_keypress) { SFCE_KEYCODE_ESCAPE, '\x1b', SFCE_MODIFIER_NONE };
        case '[':
            if (!kbhit()) {
                return (struct sfce_keypress) { '[', 0, SFCE_MODIFIER_ALT };
            }

            goto csi_begin;
        case 'N': goto single_shift_two;
        case 'O': goto single_shift_three;
        default: return (struct sfce_keypress) { character, character, SFCE_MODIFIER_ALT};
        }

        break;

    default: {
        if (character & 0x80) {
            uint8_t buffer[32] = { character };
            int32_t character_count = 1;

            while (kbhit()) {
                buffer[character_count] = (char)getch();
                character_count += 1;
            }

            int32_t codepoint = sfce_codepoint_decode_utf8(buffer, character_count);
            return (struct sfce_keypress) { codepoint, codepoint, 0 };
        }

        return (struct sfce_keypress) { character, character, 0 };
    }
    }

    return NO_KEYPRESS;
}

enum sfce_error_code sfce_save_console_state(struct sfce_console_state *state)
{
    state->input_handle = GetStdHandle(STD_INPUT_HANDLE);
    if (state->input_handle == INVALID_HANDLE_VALUE || state->input_handle == NULL) {
        return SFCE_ERROR_FAILED_WIN32_API_CALL;
    }

    state->output_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (state->output_handle == INVALID_HANDLE_VALUE || state->output_handle == NULL) {
        return SFCE_ERROR_FAILED_WIN32_API_CALL;
    }

    if (!GetConsoleMode(state->input_handle, &state->input_mode)) {
        return SFCE_ERROR_FAILED_WIN32_API_CALL;
    }

    if (!GetConsoleMode(state->output_handle, &state->output_mode)) {
        return SFCE_ERROR_FAILED_WIN32_API_CALL;
    }

    state->console_screen_buffer_info.cbSize = sizeof state->console_screen_buffer_info;
    if (!GetConsoleScreenBufferInfoEx(state->output_handle, &state->console_screen_buffer_info)) {
        return SFCE_ERROR_FAILED_WIN32_API_CALL;
    }

    state->console_font_info.cbSize = sizeof state->console_font_info;
    if (!GetCurrentConsoleFontEx(state->output_handle, FALSE, &state->console_font_info)) {
        return SFCE_ERROR_FAILED_WIN32_API_CALL;
    }

    state->input_code_page = GetConsoleCP();
    state->output_code_page = GetConsoleOutputCP();

    return SFCE_ERROR_OK;
}

enum sfce_error_code sfce_restore_console_state(struct sfce_console_state *state)
{
    enum sfce_error_code error_code = sfce_disable_console_temp_buffer();

    if (error_code != SFCE_ERROR_OK) {
        return error_code;
    }

    if (!SetConsoleMode(state->output_handle, state->output_mode)) {
        return SFCE_ERROR_FAILED_WIN32_API_CALL;
    }

    if (!SetConsoleMode(state->input_handle, state->input_mode)) {
        return SFCE_ERROR_FAILED_WIN32_API_CALL;
    }

    state->console_screen_buffer_info.srWindow.Bottom += 1;
    if (!SetConsoleScreenBufferInfoEx(state->output_handle, (void *)&state->console_screen_buffer_info)) {
        return SFCE_ERROR_FAILED_WIN32_API_CALL;
    }

    if (!SetCurrentConsoleFontEx(state->output_handle, FALSE, (void*)&state->console_font_info)) {
        return SFCE_ERROR_FAILED_WIN32_API_CALL;
    }

    if (!SetConsoleCP(state->input_code_page)) {
        return SFCE_ERROR_FAILED_WIN32_API_CALL;
    }

    if (!SetConsoleOutputCP(state->output_code_page)) {
        return SFCE_ERROR_FAILED_WIN32_API_CALL;
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
        return SFCE_ERROR_FAILED_WIN32_API_CALL;
    }

    DWORD new_input_mode = state->input_mode;
    new_input_mode &= ~ENABLE_ECHO_INPUT;
    new_input_mode &= ~ENABLE_LINE_INPUT;
    new_input_mode &= ~ENABLE_PROCESSED_INPUT;
    new_input_mode |= ENABLE_VIRTUAL_TERMINAL_INPUT;
    if (!SetConsoleMode(state->input_handle, new_input_mode)) {
        return SFCE_ERROR_FAILED_WIN32_API_CALL;
    }

    return SFCE_ERROR_OK;
}

//
// Auto generated by utf8codegen.mjs at 2025-09-06
// 
enum sfce_unicode_category sfce_codepoint_category(int32_t codepoint)
{
    switch (codepoint) {
    case 32:    case 160:   case 5760:  case 8239:  case 8287:  case 12288: 
        return SFCE_UNICODE_CATEGORY_ZS;
    case 36:     case 1423:   case 1547:   case 2555:   case 2801:
    case 3065:   case 3647:   case 6107:   case 43064:  case 65020:
    case 65129:  case 65284:  case 123647: case 126128: 
        return SFCE_UNICODE_CATEGORY_SC;
    case 40:    case 91:    case 123:   case 3898:  case 3900:  case 5787:
    case 8218:  case 8222:  case 8261:  case 8317:  case 8333:  case 8968:
    case 8970:  case 9001:  case 10088: case 10090: case 10092: case 10094:
    case 10096: case 10098: case 10100: case 10181: case 10214: case 10216:
    case 10218: case 10220: case 10222: case 10627: case 10629: case 10631:
    case 10633: case 10635: case 10637: case 10639: case 10641: case 10643:
    case 10645: case 10647: case 10712: case 10714: case 10748: case 11810:
    case 11812: case 11814: case 11816: case 11842: case 11861: case 11863:
    case 11865: case 11867: case 12296: case 12298: case 12300: case 12302:
    case 12304: case 12308: case 12310: case 12312: case 12314: case 12317:
    case 64831: case 65047: case 65077: case 65079: case 65081: case 65083:
    case 65085: case 65087: case 65089: case 65091: case 65095: case 65113:
    case 65115: case 65117: case 65288: case 65339: case 65371: case 65375:
    case 65378: 
        return SFCE_UNICODE_CATEGORY_PS;
    case 41:    case 93:    case 125:   case 3899:  case 3901:  case 5788:
    case 8262:  case 8318:  case 8334:  case 8969:  case 8971:  case 9002:
    case 10089: case 10091: case 10093: case 10095: case 10097: case 10099:
    case 10101: case 10182: case 10215: case 10217: case 10219: case 10221:
    case 10223: case 10628: case 10630: case 10632: case 10634: case 10636:
    case 10638: case 10640: case 10642: case 10644: case 10646: case 10648:
    case 10713: case 10715: case 10749: case 11811: case 11813: case 11815:
    case 11817: case 11862: case 11864: case 11866: case 11868: case 12297:
    case 12299: case 12301: case 12303: case 12305: case 12309: case 12311:
    case 12313: case 12315: case 64830: case 65048: case 65078: case 65080:
    case 65082: case 65084: case 65086: case 65088: case 65090: case 65092:
    case 65096: case 65114: case 65116: case 65118: case 65289: case 65341:
    case 65373: case 65376: case 65379: 
        return SFCE_UNICODE_CATEGORY_PE;
    case 42:     case 44:     case 92:     case 161:    case 167:
    case 191:    case 894:    case 903:    case 1417:   case 1472:
    case 1475:   case 1478:   case 1563:   case 1748:   case 2142:
    case 2416:   case 2557:   case 2678:   case 2800:   case 3191:
    case 3204:   case 3572:   case 3663:   case 3860:   case 3973:
    case 4347:   case 5742:   case 7379:   case 8275:   case 11632:
    case 11787:  case 11803:  case 11841:  case 12349:  case 12539:
    case 42611:  case 42622:  case 43260:  case 43359:  case 44011:
    case 65049:  case 65072:  case 65128:  case 65290:  case 65292:
    case 65340:  case 65377:  case 66463:  case 66512:  case 66927:
    case 67671:  case 67871:  case 67903:  case 68223:  case 70093:
    case 70107:  case 70313:  case 70749:  case 70854:  case 71353:
    case 71739:  case 72162:  case 72673:  case 73727:  case 92917:
    case 92996:  case 94178:  case 113823: case 124415: 
        return SFCE_UNICODE_CATEGORY_PO;
    case 43:     case 124:    case 126:    case 172:    case 177:
    case 215:    case 247:    case 1014:   case 8260:   case 8274:
    case 8472:   case 8523:   case 8608:   case 8611:   case 8614:
    case 8622:   case 8658:   case 8660:   case 9084:   case 9655:
    case 9665:   case 9839:   case 64297:  case 65122:  case 65291:
    case 65372:  case 65374:  case 65506:  case 120513: case 120539:
    case 120571: case 120597: case 120629: case 120655: case 120687:
    case 120713: case 120745: case 120771: 
        return SFCE_UNICODE_CATEGORY_SM;
    case 45:    case 1418:  case 1470:  case 5120:  case 6150:  case 11799:
    case 11802: case 11840: case 11869: case 12316: case 12336: case 12448:
    case 65112: case 65123: case 65293: case 68974: case 69293: 
        return SFCE_UNICODE_CATEGORY_PD;
    case 94:    case 96:    case 168:   case 175:   case 180:   case 184:
    case 749:   case 885:   case 2184:  case 8125:  case 43867: case 65342:
    case 65344: case 65507: 
        return SFCE_UNICODE_CATEGORY_SK;
    case 95:    case 8276:  case 65343: 
        return SFCE_UNICODE_CATEGORY_PC;
    case 166:    case 169:    case 174:    case 176:    case 1154:
    case 1758:   case 1769:   case 2038:   case 2554:   case 2928:
    case 3066:   case 3199:   case 3407:   case 3449:   case 3859:
    case 3892:   case 3894:   case 3896:   case 5741:   case 6464:
    case 8468:   case 8485:   case 8487:   case 8489:   case 8494:
    case 8522:   case 8527:   case 8659:   case 12292:  case 12320:
    case 12783:  case 12880:  case 43065:  case 64975:  case 65508:
    case 65512:  case 65952:  case 68296:  case 71487:  case 92997:
    case 113820: case 119365: case 123215: case 126124: case 126254:
    case 129008: 
        return SFCE_UNICODE_CATEGORY_SO;
    case 170:    case 186:    case 443:    case 660:    case 1749:
    case 1791:   case 1808:   case 1969:   case 2365:   case 2384:
    case 2482:   case 2493:   case 2510:   case 2556:   case 2654:
    case 2749:   case 2768:   case 2809:   case 2877:   case 2929:
    case 2947:   case 2972:   case 3024:   case 3133:   case 3165:
    case 3200:   case 3261:   case 3389:   case 3406:   case 3517:
    case 3716:   case 3749:   case 3773:   case 3840:   case 4159:
    case 4193:   case 4238:   case 4696:   case 4800:   case 6108:
    case 6314:   case 7418:   case 12294:  case 12348:  case 12447:
    case 12543:  case 13312:  case 19903:  case 19968:  case 42606:
    case 42895:  case 42999:  case 43259:  case 43642:  case 43697:
    case 43712:  case 43714:  case 43762:  case 44032:  case 55203:
    case 64285:  case 64318:  case 67592:  case 67644:  case 68096:
    case 68943:  case 69415:  case 69749:  case 69956:  case 69959:
    case 70006:  case 70106:  case 70108:  case 70280:  case 70461:
    case 70480:  case 70539:  case 70542:  case 70583:  case 70609:
    case 70611:  case 70855:  case 71236:  case 71352:  case 71945:
    case 71999:  case 72001:  case 72161:  case 72163:  case 72192:
    case 72250:  case 72272:  case 72349:  case 72768:  case 73030:
    case 73112:  case 73474:  case 73648:  case 94032:  case 94208:
    case 100343: case 101640: case 110898: case 110933: case 122634:
    case 123214: case 124400: case 126500: case 126503: case 126521:
    case 126523: case 126530: case 126535: case 126537: case 126539:
    case 126548: case 126551: case 126553: case 126555: case 126557:
    case 126559: case 126564: case 126590: case 131072: case 173791:
    case 173824: case 177977: case 177984: case 178205: case 178208:
    case 183969: case 183984: case 191456: case 191472: case 192093:
    case 196608: case 201546: case 201552: case 205743: 
        return SFCE_UNICODE_CATEGORY_LO;
    case 171:   case 8216:  case 8223:  case 8249:  case 11778: case 11780:
    case 11785: case 11788: case 11804: case 11808: 
        return SFCE_UNICODE_CATEGORY_PI;
    case 173:    case 1564:   case 1757:   case 1807:   case 2274:
    case 6158:   case 65279:  case 69821:  case 69837:  case 917505: 
        return SFCE_UNICODE_CATEGORY_CF;
    case 181:    case 257:    case 259:    case 261:    case 263:
    case 265:    case 267:    case 269:    case 271:    case 273:
    case 275:    case 277:    case 279:    case 281:    case 283:
    case 285:    case 287:    case 289:    case 291:    case 293:
    case 295:    case 297:    case 299:    case 301:    case 303:
    case 305:    case 307:    case 309:    case 314:    case 316:
    case 318:    case 320:    case 322:    case 324:    case 326:
    case 331:    case 333:    case 335:    case 337:    case 339:
    case 341:    case 343:    case 345:    case 347:    case 349:
    case 351:    case 353:    case 355:    case 357:    case 359:
    case 361:    case 363:    case 365:    case 367:    case 369:
    case 371:    case 373:    case 375:    case 378:    case 380:
    case 387:    case 389:    case 392:    case 402:    case 405:
    case 414:    case 417:    case 419:    case 421:    case 424:
    case 429:    case 432:    case 436:    case 438:    case 454:
    case 457:    case 460:    case 462:    case 464:    case 466:
    case 468:    case 470:    case 472:    case 474:    case 479:
    case 481:    case 483:    case 485:    case 487:    case 489:
    case 491:    case 493:    case 499:    case 501:    case 505:
    case 507:    case 509:    case 511:    case 513:    case 515:
    case 517:    case 519:    case 521:    case 523:    case 525:
    case 527:    case 529:    case 531:    case 533:    case 535:
    case 537:    case 539:    case 541:    case 543:    case 545:
    case 547:    case 549:    case 551:    case 553:    case 555:
    case 557:    case 559:    case 561:    case 572:    case 578:
    case 583:    case 585:    case 587:    case 589:    case 881:
    case 883:    case 887:    case 912:    case 985:    case 987:
    case 989:    case 991:    case 993:    case 995:    case 997:
    case 999:    case 1001:   case 1003:   case 1005:   case 1013:
    case 1016:   case 1121:   case 1123:   case 1125:   case 1127:
    case 1129:   case 1131:   case 1133:   case 1135:   case 1137:
    case 1139:   case 1141:   case 1143:   case 1145:   case 1147:
    case 1149:   case 1151:   case 1153:   case 1163:   case 1165:
    case 1167:   case 1169:   case 1171:   case 1173:   case 1175:
    case 1177:   case 1179:   case 1181:   case 1183:   case 1185:
    case 1187:   case 1189:   case 1191:   case 1193:   case 1195:
    case 1197:   case 1199:   case 1201:   case 1203:   case 1205:
    case 1207:   case 1209:   case 1211:   case 1213:   case 1215:
    case 1218:   case 1220:   case 1222:   case 1224:   case 1226:
    case 1228:   case 1233:   case 1235:   case 1237:   case 1239:
    case 1241:   case 1243:   case 1245:   case 1247:   case 1249:
    case 1251:   case 1253:   case 1255:   case 1257:   case 1259:
    case 1261:   case 1263:   case 1265:   case 1267:   case 1269:
    case 1271:   case 1273:   case 1275:   case 1277:   case 1279:
    case 1281:   case 1283:   case 1285:   case 1287:   case 1289:
    case 1291:   case 1293:   case 1295:   case 1297:   case 1299:
    case 1301:   case 1303:   case 1305:   case 1307:   case 1309:
    case 1311:   case 1313:   case 1315:   case 1317:   case 1319:
    case 1321:   case 1323:   case 1325:   case 1327:   case 7306:
    case 7681:   case 7683:   case 7685:   case 7687:   case 7689:
    case 7691:   case 7693:   case 7695:   case 7697:   case 7699:
    case 7701:   case 7703:   case 7705:   case 7707:   case 7709:
    case 7711:   case 7713:   case 7715:   case 7717:   case 7719:
    case 7721:   case 7723:   case 7725:   case 7727:   case 7729:
    case 7731:   case 7733:   case 7735:   case 7737:   case 7739:
    case 7741:   case 7743:   case 7745:   case 7747:   case 7749:
    case 7751:   case 7753:   case 7755:   case 7757:   case 7759:
    case 7761:   case 7763:   case 7765:   case 7767:   case 7769:
    case 7771:   case 7773:   case 7775:   case 7777:   case 7779:
    case 7781:   case 7783:   case 7785:   case 7787:   case 7789:
    case 7791:   case 7793:   case 7795:   case 7797:   case 7799:
    case 7801:   case 7803:   case 7805:   case 7807:   case 7809:
    case 7811:   case 7813:   case 7815:   case 7817:   case 7819:
    case 7821:   case 7823:   case 7825:   case 7827:   case 7839:
    case 7841:   case 7843:   case 7845:   case 7847:   case 7849:
    case 7851:   case 7853:   case 7855:   case 7857:   case 7859:
    case 7861:   case 7863:   case 7865:   case 7867:   case 7869:
    case 7871:   case 7873:   case 7875:   case 7877:   case 7879:
    case 7881:   case 7883:   case 7885:   case 7887:   case 7889:
    case 7891:   case 7893:   case 7895:   case 7897:   case 7899:
    case 7901:   case 7903:   case 7905:   case 7907:   case 7909:
    case 7911:   case 7913:   case 7915:   case 7917:   case 7919:
    case 7921:   case 7923:   case 7925:   case 7927:   case 7929:
    case 7931:   case 7933:   case 8126:   case 8458:   case 8467:
    case 8495:   case 8500:   case 8505:   case 8526:   case 8580:
    case 11361:  case 11368:  case 11370:  case 11372:  case 11377:
    case 11393:  case 11395:  case 11397:  case 11399:  case 11401:
    case 11403:  case 11405:  case 11407:  case 11409:  case 11411:
    case 11413:  case 11415:  case 11417:  case 11419:  case 11421:
    case 11423:  case 11425:  case 11427:  case 11429:  case 11431:
    case 11433:  case 11435:  case 11437:  case 11439:  case 11441:
    case 11443:  case 11445:  case 11447:  case 11449:  case 11451:
    case 11453:  case 11455:  case 11457:  case 11459:  case 11461:
    case 11463:  case 11465:  case 11467:  case 11469:  case 11471:
    case 11473:  case 11475:  case 11477:  case 11479:  case 11481:
    case 11483:  case 11485:  case 11487:  case 11489:  case 11500:
    case 11502:  case 11507:  case 11559:  case 11565:  case 42561:
    case 42563:  case 42565:  case 42567:  case 42569:  case 42571:
    case 42573:  case 42575:  case 42577:  case 42579:  case 42581:
    case 42583:  case 42585:  case 42587:  case 42589:  case 42591:
    case 42593:  case 42595:  case 42597:  case 42599:  case 42601:
    case 42603:  case 42605:  case 42625:  case 42627:  case 42629:
    case 42631:  case 42633:  case 42635:  case 42637:  case 42639:
    case 42641:  case 42643:  case 42645:  case 42647:  case 42649:
    case 42651:  case 42787:  case 42789:  case 42791:  case 42793:
    case 42795:  case 42797:  case 42803:  case 42805:  case 42807:
    case 42809:  case 42811:  case 42813:  case 42815:  case 42817:
    case 42819:  case 42821:  case 42823:  case 42825:  case 42827:
    case 42829:  case 42831:  case 42833:  case 42835:  case 42837:
    case 42839:  case 42841:  case 42843:  case 42845:  case 42847:
    case 42849:  case 42851:  case 42853:  case 42855:  case 42857:
    case 42859:  case 42861:  case 42863:  case 42874:  case 42876:
    case 42879:  case 42881:  case 42883:  case 42885:  case 42887:
    case 42892:  case 42894:  case 42897:  case 42903:  case 42905:
    case 42907:  case 42909:  case 42911:  case 42913:  case 42915:
    case 42917:  case 42919:  case 42921:  case 42927:  case 42933:
    case 42935:  case 42937:  case 42939:  case 42941:  case 42943:
    case 42945:  case 42947:  case 42952:  case 42954:  case 42957:
    case 42961:  case 42963:  case 42965:  case 42967:  case 42969:
    case 42971:  case 42998:  case 43002:  case 119995: case 120779: 
        return SFCE_UNICODE_CATEGORY_LL;
    case 185:   case 6618:  case 8304:  case 8585:  case 11517: 
        return SFCE_UNICODE_CATEGORY_NO;
    case 187:   case 8217:  case 8221:  case 8250:  case 11779: case 11781:
    case 11786: case 11789: case 11805: case 11809: 
        return SFCE_UNICODE_CATEGORY_PF;
    case 256:    case 258:    case 260:    case 262:    case 264:
    case 266:    case 268:    case 270:    case 272:    case 274:
    case 276:    case 278:    case 280:    case 282:    case 284:
    case 286:    case 288:    case 290:    case 292:    case 294:
    case 296:    case 298:    case 300:    case 302:    case 304:
    case 306:    case 308:    case 310:    case 313:    case 315:
    case 317:    case 319:    case 321:    case 323:    case 325:
    case 327:    case 330:    case 332:    case 334:    case 336:
    case 338:    case 340:    case 342:    case 344:    case 346:
    case 348:    case 350:    case 352:    case 354:    case 356:
    case 358:    case 360:    case 362:    case 364:    case 366:
    case 368:    case 370:    case 372:    case 374:    case 379:
    case 381:    case 388:    case 418:    case 420:    case 425:
    case 428:    case 437:    case 444:    case 452:    case 455:
    case 458:    case 461:    case 463:    case 465:    case 467:
    case 469:    case 471:    case 473:    case 475:    case 478:
    case 480:    case 482:    case 484:    case 486:    case 488:
    case 490:    case 492:    case 494:    case 497:    case 500:
    case 506:    case 508:    case 510:    case 512:    case 514:
    case 516:    case 518:    case 520:    case 522:    case 524:
    case 526:    case 528:    case 530:    case 532:    case 534:
    case 536:    case 538:    case 540:    case 542:    case 544:
    case 546:    case 548:    case 550:    case 552:    case 554:
    case 556:    case 558:    case 560:    case 562:    case 577:
    case 584:    case 586:    case 588:    case 590:    case 880:
    case 882:    case 886:    case 895:    case 902:    case 908:
    case 975:    case 984:    case 986:    case 988:    case 990:
    case 992:    case 994:    case 996:    case 998:    case 1000:
    case 1002:   case 1004:   case 1006:   case 1012:   case 1015:
    case 1120:   case 1122:   case 1124:   case 1126:   case 1128:
    case 1130:   case 1132:   case 1134:   case 1136:   case 1138:
    case 1140:   case 1142:   case 1144:   case 1146:   case 1148:
    case 1150:   case 1152:   case 1162:   case 1164:   case 1166:
    case 1168:   case 1170:   case 1172:   case 1174:   case 1176:
    case 1178:   case 1180:   case 1182:   case 1184:   case 1186:
    case 1188:   case 1190:   case 1192:   case 1194:   case 1196:
    case 1198:   case 1200:   case 1202:   case 1204:   case 1206:
    case 1208:   case 1210:   case 1212:   case 1214:   case 1219:
    case 1221:   case 1223:   case 1225:   case 1227:   case 1229:
    case 1232:   case 1234:   case 1236:   case 1238:   case 1240:
    case 1242:   case 1244:   case 1246:   case 1248:   case 1250:
    case 1252:   case 1254:   case 1256:   case 1258:   case 1260:
    case 1262:   case 1264:   case 1266:   case 1268:   case 1270:
    case 1272:   case 1274:   case 1276:   case 1278:   case 1280:
    case 1282:   case 1284:   case 1286:   case 1288:   case 1290:
    case 1292:   case 1294:   case 1296:   case 1298:   case 1300:
    case 1302:   case 1304:   case 1306:   case 1308:   case 1310:
    case 1312:   case 1314:   case 1316:   case 1318:   case 1320:
    case 1322:   case 1324:   case 1326:   case 4295:   case 4301:
    case 7305:   case 7680:   case 7682:   case 7684:   case 7686:
    case 7688:   case 7690:   case 7692:   case 7694:   case 7696:
    case 7698:   case 7700:   case 7702:   case 7704:   case 7706:
    case 7708:   case 7710:   case 7712:   case 7714:   case 7716:
    case 7718:   case 7720:   case 7722:   case 7724:   case 7726:
    case 7728:   case 7730:   case 7732:   case 7734:   case 7736:
    case 7738:   case 7740:   case 7742:   case 7744:   case 7746:
    case 7748:   case 7750:   case 7752:   case 7754:   case 7756:
    case 7758:   case 7760:   case 7762:   case 7764:   case 7766:
    case 7768:   case 7770:   case 7772:   case 7774:   case 7776:
    case 7778:   case 7780:   case 7782:   case 7784:   case 7786:
    case 7788:   case 7790:   case 7792:   case 7794:   case 7796:
    case 7798:   case 7800:   case 7802:   case 7804:   case 7806:
    case 7808:   case 7810:   case 7812:   case 7814:   case 7816:
    case 7818:   case 7820:   case 7822:   case 7824:   case 7826:
    case 7828:   case 7838:   case 7840:   case 7842:   case 7844:
    case 7846:   case 7848:   case 7850:   case 7852:   case 7854:
    case 7856:   case 7858:   case 7860:   case 7862:   case 7864:
    case 7866:   case 7868:   case 7870:   case 7872:   case 7874:
    case 7876:   case 7878:   case 7880:   case 7882:   case 7884:
    case 7886:   case 7888:   case 7890:   case 7892:   case 7894:
    case 7896:   case 7898:   case 7900:   case 7902:   case 7904:
    case 7906:   case 7908:   case 7910:   case 7912:   case 7914:
    case 7916:   case 7918:   case 7920:   case 7922:   case 7924:
    case 7926:   case 7928:   case 7930:   case 7932:   case 7934:
    case 8025:   case 8027:   case 8029:   case 8031:   case 8450:
    case 8455:   case 8469:   case 8484:   case 8486:   case 8488:
    case 8517:   case 8579:   case 11360:  case 11367:  case 11369:
    case 11371:  case 11378:  case 11381:  case 11394:  case 11396:
    case 11398:  case 11400:  case 11402:  case 11404:  case 11406:
    case 11408:  case 11410:  case 11412:  case 11414:  case 11416:
    case 11418:  case 11420:  case 11422:  case 11424:  case 11426:
    case 11428:  case 11430:  case 11432:  case 11434:  case 11436:
    case 11438:  case 11440:  case 11442:  case 11444:  case 11446:
    case 11448:  case 11450:  case 11452:  case 11454:  case 11456:
    case 11458:  case 11460:  case 11462:  case 11464:  case 11466:
    case 11468:  case 11470:  case 11472:  case 11474:  case 11476:
    case 11478:  case 11480:  case 11482:  case 11484:  case 11486:
    case 11488:  case 11490:  case 11499:  case 11501:  case 11506:
    case 42560:  case 42562:  case 42564:  case 42566:  case 42568:
    case 42570:  case 42572:  case 42574:  case 42576:  case 42578:
    case 42580:  case 42582:  case 42584:  case 42586:  case 42588:
    case 42590:  case 42592:  case 42594:  case 42596:  case 42598:
    case 42600:  case 42602:  case 42604:  case 42624:  case 42626:
    case 42628:  case 42630:  case 42632:  case 42634:  case 42636:
    case 42638:  case 42640:  case 42642:  case 42644:  case 42646:
    case 42648:  case 42650:  case 42786:  case 42788:  case 42790:
    case 42792:  case 42794:  case 42796:  case 42798:  case 42802:
    case 42804:  case 42806:  case 42808:  case 42810:  case 42812:
    case 42814:  case 42816:  case 42818:  case 42820:  case 42822:
    case 42824:  case 42826:  case 42828:  case 42830:  case 42832:
    case 42834:  case 42836:  case 42838:  case 42840:  case 42842:
    case 42844:  case 42846:  case 42848:  case 42850:  case 42852:
    case 42854:  case 42856:  case 42858:  case 42860:  case 42862:
    case 42873:  case 42875:  case 42880:  case 42882:  case 42884:
    case 42886:  case 42891:  case 42893:  case 42896:  case 42898:
    case 42902:  case 42904:  case 42906:  case 42908:  case 42910:
    case 42912:  case 42914:  case 42916:  case 42918:  case 42920:
    case 42934:  case 42936:  case 42938:  case 42940:  case 42942:
    case 42944:  case 42946:  case 42953:  case 42960:  case 42966:
    case 42968:  case 42970:  case 42972:  case 42997:  case 119964:
    case 119970: case 120134: case 120778: 
        return SFCE_UNICODE_CATEGORY_LU;
    case 453:  case 456:  case 459:  case 498:  case 8124: case 8140:
    case 8188: 
        return SFCE_UNICODE_CATEGORY_LT;
    case 748:    case 750:    case 884:    case 890:    case 1369:
    case 1600:   case 2042:   case 2074:   case 2084:   case 2088:
    case 2249:   case 2417:   case 3654:   case 3782:   case 4348:
    case 6103:   case 6211:   case 6823:   case 7544:   case 8305:
    case 8319:   case 11631:  case 11823:  case 12293:  case 12347:
    case 40981:  case 42508:  case 42623:  case 42864:  case 42888:
    case 43471:  case 43494:  case 43632:  case 43741:  case 43881:
    case 65392:  case 68942:  case 68975:  case 94179:  case 124139:
    case 125259: 
        return SFCE_UNICODE_CATEGORY_LM;
    case 1471:   case 1479:   case 1648:   case 1809:   case 2045:
    case 2362:   case 2364:   case 2381:   case 2433:   case 2492:
    case 2509:   case 2558:   case 2620:   case 2641:   case 2677:
    case 2748:   case 2765:   case 2817:   case 2876:   case 2879:
    case 2893:   case 2946:   case 3008:   case 3021:   case 3072:
    case 3076:   case 3132:   case 3201:   case 3260:   case 3263:
    case 3270:   case 3405:   case 3457:   case 3530:   case 3542:
    case 3633:   case 3761:   case 3893:   case 3895:   case 3897:
    case 4038:   case 4226:   case 4237:   case 4253:   case 6086:
    case 6109:   case 6159:   case 6313:   case 6450:   case 6683:
    case 6742:   case 6752:   case 6754:   case 6783:   case 6964:
    case 6972:   case 6978:   case 7142:   case 7149:   case 7405:
    case 7412:   case 8417:   case 11647:  case 42607:  case 43010:
    case 43014:  case 43019:  case 43052:  case 43263:  case 43443:
    case 43493:  case 43587:  case 43596:  case 43644:  case 43696:
    case 43713:  case 43766:  case 44005:  case 44008:  case 44013:
    case 64286:  case 66045:  case 66272:  case 68159:  case 69633:
    case 69744:  case 69826:  case 70003:  case 70095:  case 70196:
    case 70206:  case 70209:  case 70367:  case 70464:  case 70606:
    case 70608:  case 70610:  case 70726:  case 70750:  case 70842:
    case 71229:  case 71339:  case 71341:  case 71351:  case 71453:
    case 71455:  case 71998:  case 72003:  case 72160:  case 72263:
    case 72767:  case 73018:  case 73031:  case 73109:  case 73111:
    case 73536:  case 73538:  case 73562:  case 78912:  case 94031:
    case 94180:  case 121461: case 121476: case 123023: case 123566: 
        return SFCE_UNICODE_CATEGORY_MN;
    case 2307:  case 2363:  case 2519:  case 2563:  case 2691:  case 2761:
    case 2878:  case 2880:  case 2903:  case 3031:  case 3262:  case 3315:
    case 3415:  case 3967:  case 4145:  case 4152:  case 4239:  case 5909:
    case 5940:  case 6070:  case 6741:  case 6743:  case 6753:  case 6916:
    case 6965:  case 6971:  case 7042:  case 7073:  case 7082:  case 7143:
    case 7150:  case 7393:  case 7415:  case 43047: case 43395: case 43597:
    case 43643: case 43645: case 43755: case 43765: case 44012: case 69632:
    case 69634: case 69762: case 69932: case 70018: case 70094: case 70197:
    case 70487: case 70594: case 70597: case 70607: case 70725: case 70841:
    case 70849: case 71102: case 71230: case 71340: case 71350: case 71454:
    case 71462: case 71736: case 71997: case 72000: case 72002: case 72164:
    case 72249: case 72343: case 72751: case 72766: case 72873: case 72881:
    case 72884: case 73110: case 73475: case 73537: 
        return SFCE_UNICODE_CATEGORY_MC;
    case 6846: return SFCE_UNICODE_CATEGORY_ME;
    case 8232: return SFCE_UNICODE_CATEGORY_ZL;
    case 8233: return SFCE_UNICODE_CATEGORY_ZP;
    case 12295: case 66369: case 66378: 
        return SFCE_UNICODE_CATEGORY_NL;
    case 55296: case 57343: 
        return SFCE_UNICODE_CATEGORY_CS;
    case 57344:   case 63743:   case 983040:  case 1048573: case 1048576:
    case 1114109: 
        return SFCE_UNICODE_CATEGORY_CO;
    }

    if (codepoint > -1 && codepoint < 32) return SFCE_UNICODE_CATEGORY_CC;
    if (codepoint > 32 && codepoint < 36) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 36 && codepoint < 40) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 45 && codepoint < 48) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 47 && codepoint < 58) return SFCE_UNICODE_CATEGORY_ND;
    if (codepoint > 57 && codepoint < 60) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 59 && codepoint < 63) return SFCE_UNICODE_CATEGORY_SM;
    if (codepoint > 62 && codepoint < 65) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 64 && codepoint < 91) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 96 && codepoint < 123) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 126 && codepoint < 160) return SFCE_UNICODE_CATEGORY_CC;
    if (codepoint > 161 && codepoint < 166) return SFCE_UNICODE_CATEGORY_SC;
    if (codepoint > 177 && codepoint < 180) return SFCE_UNICODE_CATEGORY_NO;
    if (codepoint > 181 && codepoint < 184) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 187 && codepoint < 191) return SFCE_UNICODE_CATEGORY_NO;
    if (codepoint > 191 && codepoint < 215) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 215 && codepoint < 223) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 222 && codepoint < 247) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 247 && codepoint < 256) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 310 && codepoint < 313) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 327 && codepoint < 330) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 375 && codepoint < 378) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 381 && codepoint < 385) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 384 && codepoint < 387) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 389 && codepoint < 392) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 392 && codepoint < 396) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 395 && codepoint < 398) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 397 && codepoint < 402) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 402 && codepoint < 405) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 405 && codepoint < 409) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 408 && codepoint < 412) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 411 && codepoint < 414) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 414 && codepoint < 417) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 421 && codepoint < 424) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 425 && codepoint < 428) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 429 && codepoint < 432) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 432 && codepoint < 436) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 438 && codepoint < 441) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 440 && codepoint < 443) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 444 && codepoint < 448) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 447 && codepoint < 452) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 475 && codepoint < 478) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 494 && codepoint < 497) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 501 && codepoint < 505) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 562 && codepoint < 570) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 569 && codepoint < 572) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 572 && codepoint < 575) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 574 && codepoint < 577) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 578 && codepoint < 583) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 590 && codepoint < 660) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 660 && codepoint < 688) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 687 && codepoint < 706) return SFCE_UNICODE_CATEGORY_LM;
    if (codepoint > 705 && codepoint < 710) return SFCE_UNICODE_CATEGORY_SK;
    if (codepoint > 709 && codepoint < 722) return SFCE_UNICODE_CATEGORY_LM;
    if (codepoint > 721 && codepoint < 736) return SFCE_UNICODE_CATEGORY_SK;
    if (codepoint > 735 && codepoint < 741) return SFCE_UNICODE_CATEGORY_LM;
    if (codepoint > 740 && codepoint < 748) return SFCE_UNICODE_CATEGORY_SK;
    if (codepoint > 750 && codepoint < 768) return SFCE_UNICODE_CATEGORY_SK;
    if (codepoint > 767 && codepoint < 880) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 890 && codepoint < 894) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 899 && codepoint < 902) return SFCE_UNICODE_CATEGORY_SK;
    if (codepoint > 903 && codepoint < 907) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 909 && codepoint < 912) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 912 && codepoint < 930) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 930 && codepoint < 940) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 939 && codepoint < 975) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 975 && codepoint < 978) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 977 && codepoint < 981) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 980 && codepoint < 984) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 1006 && codepoint < 1012) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 1016 && codepoint < 1019) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 1018 && codepoint < 1021) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 1020 && codepoint < 1072) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 1071 && codepoint < 1120) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 1154 && codepoint < 1160) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 1159 && codepoint < 1162) return SFCE_UNICODE_CATEGORY_ME;
    if (codepoint > 1215 && codepoint < 1218) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 1229 && codepoint < 1232) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 1328 && codepoint < 1367) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 1369 && codepoint < 1376) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 1375 && codepoint < 1417) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 1420 && codepoint < 1423) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 1424 && codepoint < 1470) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 1472 && codepoint < 1475) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 1475 && codepoint < 1478) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 1487 && codepoint < 1515) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 1518 && codepoint < 1523) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 1522 && codepoint < 1525) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 1535 && codepoint < 1542) return SFCE_UNICODE_CATEGORY_CF;
    if (codepoint > 1541 && codepoint < 1545) return SFCE_UNICODE_CATEGORY_SM;
    if (codepoint > 1544 && codepoint < 1547) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 1547 && codepoint < 1550) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 1549 && codepoint < 1552) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 1551 && codepoint < 1563) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 1564 && codepoint < 1568) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 1567 && codepoint < 1600) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 1600 && codepoint < 1611) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 1610 && codepoint < 1632) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 1631 && codepoint < 1642) return SFCE_UNICODE_CATEGORY_ND;
    if (codepoint > 1641 && codepoint < 1646) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 1645 && codepoint < 1648) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 1648 && codepoint < 1748) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 1749 && codepoint < 1757) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 1758 && codepoint < 1765) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 1764 && codepoint < 1767) return SFCE_UNICODE_CATEGORY_LM;
    if (codepoint > 1766 && codepoint < 1769) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 1769 && codepoint < 1774) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 1773 && codepoint < 1776) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 1775 && codepoint < 1786) return SFCE_UNICODE_CATEGORY_ND;
    if (codepoint > 1785 && codepoint < 1789) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 1788 && codepoint < 1791) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 1791 && codepoint < 1806) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 1809 && codepoint < 1840) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 1839 && codepoint < 1867) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 1868 && codepoint < 1958) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 1957 && codepoint < 1969) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 1983 && codepoint < 1994) return SFCE_UNICODE_CATEGORY_ND;
    if (codepoint > 1993 && codepoint < 2027) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 2026 && codepoint < 2036) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 2035 && codepoint < 2038) return SFCE_UNICODE_CATEGORY_LM;
    if (codepoint > 2038 && codepoint < 2042) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 2045 && codepoint < 2048) return SFCE_UNICODE_CATEGORY_SC;
    if (codepoint > 2047 && codepoint < 2070) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 2069 && codepoint < 2074) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 2074 && codepoint < 2084) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 2084 && codepoint < 2088) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 2088 && codepoint < 2094) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 2095 && codepoint < 2111) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 2111 && codepoint < 2137) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 2136 && codepoint < 2140) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 2143 && codepoint < 2155) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 2159 && codepoint < 2184) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 2184 && codepoint < 2191) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 2191 && codepoint < 2194) return SFCE_UNICODE_CATEGORY_CF;
    if (codepoint > 2198 && codepoint < 2208) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 2207 && codepoint < 2249) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 2249 && codepoint < 2274) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 2274 && codepoint < 2307) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 2307 && codepoint < 2362) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 2365 && codepoint < 2369) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 2368 && codepoint < 2377) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 2376 && codepoint < 2381) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 2381 && codepoint < 2384) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 2384 && codepoint < 2392) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 2391 && codepoint < 2402) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 2401 && codepoint < 2404) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 2403 && codepoint < 2406) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 2405 && codepoint < 2416) return SFCE_UNICODE_CATEGORY_ND;
    if (codepoint > 2417 && codepoint < 2433) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 2433 && codepoint < 2436) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 2436 && codepoint < 2445) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 2446 && codepoint < 2449) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 2450 && codepoint < 2473) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 2473 && codepoint < 2481) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 2485 && codepoint < 2490) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 2493 && codepoint < 2497) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 2496 && codepoint < 2501) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 2502 && codepoint < 2505) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 2506 && codepoint < 2509) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 2523 && codepoint < 2526) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 2526 && codepoint < 2530) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 2529 && codepoint < 2532) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 2533 && codepoint < 2544) return SFCE_UNICODE_CATEGORY_ND;
    if (codepoint > 2543 && codepoint < 2546) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 2545 && codepoint < 2548) return SFCE_UNICODE_CATEGORY_SC;
    if (codepoint > 2547 && codepoint < 2554) return SFCE_UNICODE_CATEGORY_NO;
    if (codepoint > 2560 && codepoint < 2563) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 2564 && codepoint < 2571) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 2574 && codepoint < 2577) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 2578 && codepoint < 2601) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 2601 && codepoint < 2609) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 2609 && codepoint < 2612) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 2612 && codepoint < 2615) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 2615 && codepoint < 2618) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 2621 && codepoint < 2625) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 2624 && codepoint < 2627) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 2630 && codepoint < 2633) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 2634 && codepoint < 2638) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 2648 && codepoint < 2653) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 2661 && codepoint < 2672) return SFCE_UNICODE_CATEGORY_ND;
    if (codepoint > 2671 && codepoint < 2674) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 2673 && codepoint < 2677) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 2688 && codepoint < 2691) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 2692 && codepoint < 2702) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 2702 && codepoint < 2706) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 2706 && codepoint < 2729) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 2729 && codepoint < 2737) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 2737 && codepoint < 2740) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 2740 && codepoint < 2746) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 2749 && codepoint < 2753) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 2752 && codepoint < 2758) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 2758 && codepoint < 2761) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 2762 && codepoint < 2765) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 2783 && codepoint < 2786) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 2785 && codepoint < 2788) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 2789 && codepoint < 2800) return SFCE_UNICODE_CATEGORY_ND;
    if (codepoint > 2809 && codepoint < 2816) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 2817 && codepoint < 2820) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 2820 && codepoint < 2829) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 2830 && codepoint < 2833) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 2834 && codepoint < 2857) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 2857 && codepoint < 2865) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 2865 && codepoint < 2868) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 2868 && codepoint < 2874) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 2880 && codepoint < 2885) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 2886 && codepoint < 2889) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 2890 && codepoint < 2893) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 2900 && codepoint < 2903) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 2907 && codepoint < 2910) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 2910 && codepoint < 2914) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 2913 && codepoint < 2916) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 2917 && codepoint < 2928) return SFCE_UNICODE_CATEGORY_ND;
    if (codepoint > 2929 && codepoint < 2936) return SFCE_UNICODE_CATEGORY_NO;
    if (codepoint > 2948 && codepoint < 2955) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 2957 && codepoint < 2961) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 2961 && codepoint < 2966) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 2968 && codepoint < 2971) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 2973 && codepoint < 2976) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 2978 && codepoint < 2981) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 2983 && codepoint < 2987) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 2989 && codepoint < 3002) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 3005 && codepoint < 3008) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 3008 && codepoint < 3011) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 3013 && codepoint < 3017) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 3017 && codepoint < 3021) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 3045 && codepoint < 3056) return SFCE_UNICODE_CATEGORY_ND;
    if (codepoint > 3055 && codepoint < 3059) return SFCE_UNICODE_CATEGORY_NO;
    if (codepoint > 3058 && codepoint < 3065) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 3072 && codepoint < 3076) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 3076 && codepoint < 3085) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 3085 && codepoint < 3089) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 3089 && codepoint < 3113) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 3113 && codepoint < 3130) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 3133 && codepoint < 3137) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 3136 && codepoint < 3141) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 3141 && codepoint < 3145) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 3145 && codepoint < 3150) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 3156 && codepoint < 3159) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 3159 && codepoint < 3163) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 3167 && codepoint < 3170) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 3169 && codepoint < 3172) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 3173 && codepoint < 3184) return SFCE_UNICODE_CATEGORY_ND;
    if (codepoint > 3191 && codepoint < 3199) return SFCE_UNICODE_CATEGORY_NO;
    if (codepoint > 3201 && codepoint < 3204) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 3204 && codepoint < 3213) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 3213 && codepoint < 3217) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 3217 && codepoint < 3241) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 3241 && codepoint < 3252) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 3252 && codepoint < 3258) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 3263 && codepoint < 3269) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 3270 && codepoint < 3273) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 3273 && codepoint < 3276) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 3275 && codepoint < 3278) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 3284 && codepoint < 3287) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 3292 && codepoint < 3295) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 3295 && codepoint < 3298) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 3297 && codepoint < 3300) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 3301 && codepoint < 3312) return SFCE_UNICODE_CATEGORY_ND;
    if (codepoint > 3312 && codepoint < 3315) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 3327 && codepoint < 3330) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 3329 && codepoint < 3332) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 3331 && codepoint < 3341) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 3341 && codepoint < 3345) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 3345 && codepoint < 3387) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 3386 && codepoint < 3389) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 3389 && codepoint < 3393) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 3392 && codepoint < 3397) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 3397 && codepoint < 3401) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 3401 && codepoint < 3405) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 3411 && codepoint < 3415) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 3415 && codepoint < 3423) return SFCE_UNICODE_CATEGORY_NO;
    if (codepoint > 3422 && codepoint < 3426) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 3425 && codepoint < 3428) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 3429 && codepoint < 3440) return SFCE_UNICODE_CATEGORY_ND;
    if (codepoint > 3439 && codepoint < 3449) return SFCE_UNICODE_CATEGORY_NO;
    if (codepoint > 3449 && codepoint < 3456) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 3457 && codepoint < 3460) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 3460 && codepoint < 3479) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 3481 && codepoint < 3506) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 3506 && codepoint < 3516) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 3519 && codepoint < 3527) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 3534 && codepoint < 3538) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 3537 && codepoint < 3541) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 3543 && codepoint < 3552) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 3557 && codepoint < 3568) return SFCE_UNICODE_CATEGORY_ND;
    if (codepoint > 3569 && codepoint < 3572) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 3584 && codepoint < 3633) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 3633 && codepoint < 3636) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 3635 && codepoint < 3643) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 3647 && codepoint < 3654) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 3654 && codepoint < 3663) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 3663 && codepoint < 3674) return SFCE_UNICODE_CATEGORY_ND;
    if (codepoint > 3673 && codepoint < 3676) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 3712 && codepoint < 3715) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 3717 && codepoint < 3723) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 3723 && codepoint < 3748) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 3750 && codepoint < 3761) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 3761 && codepoint < 3764) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 3763 && codepoint < 3773) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 3775 && codepoint < 3781) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 3783 && codepoint < 3791) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 3791 && codepoint < 3802) return SFCE_UNICODE_CATEGORY_ND;
    if (codepoint > 3803 && codepoint < 3808) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 3840 && codepoint < 3844) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 3843 && codepoint < 3859) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 3860 && codepoint < 3864) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 3863 && codepoint < 3866) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 3865 && codepoint < 3872) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 3871 && codepoint < 3882) return SFCE_UNICODE_CATEGORY_ND;
    if (codepoint > 3881 && codepoint < 3892) return SFCE_UNICODE_CATEGORY_NO;
    if (codepoint > 3901 && codepoint < 3904) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 3903 && codepoint < 3912) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 3912 && codepoint < 3949) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 3952 && codepoint < 3967) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 3967 && codepoint < 3973) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 3973 && codepoint < 3976) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 3975 && codepoint < 3981) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 3980 && codepoint < 3992) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 3992 && codepoint < 4029) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 4029 && codepoint < 4038) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 4038 && codepoint < 4045) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 4045 && codepoint < 4048) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 4047 && codepoint < 4053) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 4052 && codepoint < 4057) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 4056 && codepoint < 4059) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 4095 && codepoint < 4139) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 4138 && codepoint < 4141) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 4140 && codepoint < 4145) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 4145 && codepoint < 4152) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 4152 && codepoint < 4155) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 4154 && codepoint < 4157) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 4156 && codepoint < 4159) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 4159 && codepoint < 4170) return SFCE_UNICODE_CATEGORY_ND;
    if (codepoint > 4169 && codepoint < 4176) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 4175 && codepoint < 4182) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 4181 && codepoint < 4184) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 4183 && codepoint < 4186) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 4185 && codepoint < 4190) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 4189 && codepoint < 4193) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 4193 && codepoint < 4197) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 4196 && codepoint < 4199) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 4198 && codepoint < 4206) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 4205 && codepoint < 4209) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 4208 && codepoint < 4213) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 4212 && codepoint < 4226) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 4226 && codepoint < 4229) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 4228 && codepoint < 4231) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 4230 && codepoint < 4237) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 4239 && codepoint < 4250) return SFCE_UNICODE_CATEGORY_ND;
    if (codepoint > 4249 && codepoint < 4253) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 4253 && codepoint < 4256) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 4255 && codepoint < 4294) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 4303 && codepoint < 4347) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 4348 && codepoint < 4352) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 4351 && codepoint < 4681) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 4681 && codepoint < 4686) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 4687 && codepoint < 4695) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 4697 && codepoint < 4702) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 4703 && codepoint < 4745) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 4745 && codepoint < 4750) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 4751 && codepoint < 4785) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 4785 && codepoint < 4790) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 4791 && codepoint < 4799) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 4801 && codepoint < 4806) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 4807 && codepoint < 4823) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 4823 && codepoint < 4881) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 4881 && codepoint < 4886) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 4887 && codepoint < 4955) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 4956 && codepoint < 4960) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 4959 && codepoint < 4969) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 4968 && codepoint < 4989) return SFCE_UNICODE_CATEGORY_NO;
    if (codepoint > 4991 && codepoint < 5008) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 5007 && codepoint < 5018) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 5023 && codepoint < 5110) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 5111 && codepoint < 5118) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 5120 && codepoint < 5741) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 5742 && codepoint < 5760) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 5760 && codepoint < 5787) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 5791 && codepoint < 5867) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 5866 && codepoint < 5870) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 5869 && codepoint < 5873) return SFCE_UNICODE_CATEGORY_NL;
    if (codepoint > 5872 && codepoint < 5881) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 5887 && codepoint < 5906) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 5905 && codepoint < 5909) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 5918 && codepoint < 5938) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 5937 && codepoint < 5940) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 5940 && codepoint < 5943) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 5951 && codepoint < 5970) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 5969 && codepoint < 5972) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 5983 && codepoint < 5997) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 5997 && codepoint < 6001) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 6001 && codepoint < 6004) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 6015 && codepoint < 6068) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 6067 && codepoint < 6070) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 6070 && codepoint < 6078) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 6077 && codepoint < 6086) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 6086 && codepoint < 6089) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 6088 && codepoint < 6100) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 6099 && codepoint < 6103) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 6103 && codepoint < 6107) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 6111 && codepoint < 6122) return SFCE_UNICODE_CATEGORY_ND;
    if (codepoint > 6127 && codepoint < 6138) return SFCE_UNICODE_CATEGORY_NO;
    if (codepoint > 6143 && codepoint < 6150) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 6150 && codepoint < 6155) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 6154 && codepoint < 6158) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 6159 && codepoint < 6170) return SFCE_UNICODE_CATEGORY_ND;
    if (codepoint > 6175 && codepoint < 6211) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 6211 && codepoint < 6265) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 6271 && codepoint < 6277) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 6276 && codepoint < 6279) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 6278 && codepoint < 6313) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 6319 && codepoint < 6390) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 6399 && codepoint < 6431) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 6431 && codepoint < 6435) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 6434 && codepoint < 6439) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 6438 && codepoint < 6441) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 6440 && codepoint < 6444) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 6447 && codepoint < 6450) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 6450 && codepoint < 6457) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 6456 && codepoint < 6460) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 6467 && codepoint < 6470) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 6469 && codepoint < 6480) return SFCE_UNICODE_CATEGORY_ND;
    if (codepoint > 6479 && codepoint < 6510) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 6511 && codepoint < 6517) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 6527 && codepoint < 6572) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 6575 && codepoint < 6602) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 6607 && codepoint < 6618) return SFCE_UNICODE_CATEGORY_ND;
    if (codepoint > 6621 && codepoint < 6656) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 6655 && codepoint < 6679) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 6678 && codepoint < 6681) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 6680 && codepoint < 6683) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 6685 && codepoint < 6688) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 6687 && codepoint < 6741) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 6743 && codepoint < 6751) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 6754 && codepoint < 6757) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 6756 && codepoint < 6765) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 6764 && codepoint < 6771) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 6770 && codepoint < 6781) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 6783 && codepoint < 6794) return SFCE_UNICODE_CATEGORY_ND;
    if (codepoint > 6799 && codepoint < 6810) return SFCE_UNICODE_CATEGORY_ND;
    if (codepoint > 6815 && codepoint < 6823) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 6823 && codepoint < 6830) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 6831 && codepoint < 6846) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 6846 && codepoint < 6863) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 6911 && codepoint < 6916) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 6916 && codepoint < 6964) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 6965 && codepoint < 6971) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 6972 && codepoint < 6978) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 6978 && codepoint < 6981) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 6980 && codepoint < 6989) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 6989 && codepoint < 6992) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 6991 && codepoint < 7002) return SFCE_UNICODE_CATEGORY_ND;
    if (codepoint > 7001 && codepoint < 7009) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 7008 && codepoint < 7019) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 7018 && codepoint < 7028) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 7027 && codepoint < 7037) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 7036 && codepoint < 7040) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 7039 && codepoint < 7042) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 7042 && codepoint < 7073) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 7073 && codepoint < 7078) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 7077 && codepoint < 7080) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 7079 && codepoint < 7082) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 7082 && codepoint < 7086) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 7085 && codepoint < 7088) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 7087 && codepoint < 7098) return SFCE_UNICODE_CATEGORY_ND;
    if (codepoint > 7097 && codepoint < 7142) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 7143 && codepoint < 7146) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 7145 && codepoint < 7149) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 7150 && codepoint < 7154) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 7153 && codepoint < 7156) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 7163 && codepoint < 7168) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 7167 && codepoint < 7204) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 7203 && codepoint < 7212) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 7211 && codepoint < 7220) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 7219 && codepoint < 7222) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 7221 && codepoint < 7224) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 7226 && codepoint < 7232) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 7231 && codepoint < 7242) return SFCE_UNICODE_CATEGORY_ND;
    if (codepoint > 7244 && codepoint < 7248) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 7247 && codepoint < 7258) return SFCE_UNICODE_CATEGORY_ND;
    if (codepoint > 7257 && codepoint < 7288) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 7287 && codepoint < 7294) return SFCE_UNICODE_CATEGORY_LM;
    if (codepoint > 7293 && codepoint < 7296) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 7295 && codepoint < 7305) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 7311 && codepoint < 7355) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 7356 && codepoint < 7360) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 7359 && codepoint < 7368) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 7375 && codepoint < 7379) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 7379 && codepoint < 7393) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 7393 && codepoint < 7401) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 7400 && codepoint < 7405) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 7405 && codepoint < 7412) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 7412 && codepoint < 7415) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 7415 && codepoint < 7418) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 7423 && codepoint < 7468) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 7467 && codepoint < 7531) return SFCE_UNICODE_CATEGORY_LM;
    if (codepoint > 7530 && codepoint < 7544) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 7544 && codepoint < 7579) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 7578 && codepoint < 7616) return SFCE_UNICODE_CATEGORY_LM;
    if (codepoint > 7615 && codepoint < 7680) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 7828 && codepoint < 7838) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 7934 && codepoint < 7944) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 7943 && codepoint < 7952) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 7951 && codepoint < 7958) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 7959 && codepoint < 7966) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 7967 && codepoint < 7976) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 7975 && codepoint < 7984) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 7983 && codepoint < 7992) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 7991 && codepoint < 8000) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 7999 && codepoint < 8006) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 8007 && codepoint < 8014) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 8015 && codepoint < 8024) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 8031 && codepoint < 8040) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 8039 && codepoint < 8048) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 8047 && codepoint < 8062) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 8063 && codepoint < 8072) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 8071 && codepoint < 8080) return SFCE_UNICODE_CATEGORY_LT;
    if (codepoint > 8079 && codepoint < 8088) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 8087 && codepoint < 8096) return SFCE_UNICODE_CATEGORY_LT;
    if (codepoint > 8095 && codepoint < 8104) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 8103 && codepoint < 8112) return SFCE_UNICODE_CATEGORY_LT;
    if (codepoint > 8111 && codepoint < 8117) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 8117 && codepoint < 8120) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 8119 && codepoint < 8124) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 8126 && codepoint < 8130) return SFCE_UNICODE_CATEGORY_SK;
    if (codepoint > 8129 && codepoint < 8133) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 8133 && codepoint < 8136) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 8135 && codepoint < 8140) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 8140 && codepoint < 8144) return SFCE_UNICODE_CATEGORY_SK;
    if (codepoint > 8143 && codepoint < 8148) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 8149 && codepoint < 8152) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 8151 && codepoint < 8156) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 8156 && codepoint < 8160) return SFCE_UNICODE_CATEGORY_SK;
    if (codepoint > 8159 && codepoint < 8168) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 8167 && codepoint < 8173) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 8172 && codepoint < 8176) return SFCE_UNICODE_CATEGORY_SK;
    if (codepoint > 8177 && codepoint < 8181) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 8181 && codepoint < 8184) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 8183 && codepoint < 8188) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 8188 && codepoint < 8191) return SFCE_UNICODE_CATEGORY_SK;
    if (codepoint > 8191 && codepoint < 8203) return SFCE_UNICODE_CATEGORY_ZS;
    if (codepoint > 8202 && codepoint < 8208) return SFCE_UNICODE_CATEGORY_CF;
    if (codepoint > 8207 && codepoint < 8214) return SFCE_UNICODE_CATEGORY_PD;
    if (codepoint > 8213 && codepoint < 8216) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 8218 && codepoint < 8221) return SFCE_UNICODE_CATEGORY_PI;
    if (codepoint > 8223 && codepoint < 8232) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 8233 && codepoint < 8239) return SFCE_UNICODE_CATEGORY_CF;
    if (codepoint > 8239 && codepoint < 8249) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 8250 && codepoint < 8255) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 8254 && codepoint < 8257) return SFCE_UNICODE_CATEGORY_PC;
    if (codepoint > 8256 && codepoint < 8260) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 8262 && codepoint < 8274) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 8276 && codepoint < 8287) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 8287 && codepoint < 8293) return SFCE_UNICODE_CATEGORY_CF;
    if (codepoint > 8293 && codepoint < 8304) return SFCE_UNICODE_CATEGORY_CF;
    if (codepoint > 8307 && codepoint < 8314) return SFCE_UNICODE_CATEGORY_NO;
    if (codepoint > 8313 && codepoint < 8317) return SFCE_UNICODE_CATEGORY_SM;
    if (codepoint > 8319 && codepoint < 8330) return SFCE_UNICODE_CATEGORY_NO;
    if (codepoint > 8329 && codepoint < 8333) return SFCE_UNICODE_CATEGORY_SM;
    if (codepoint > 8335 && codepoint < 8349) return SFCE_UNICODE_CATEGORY_LM;
    if (codepoint > 8351 && codepoint < 8385) return SFCE_UNICODE_CATEGORY_SC;
    if (codepoint > 8399 && codepoint < 8413) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 8412 && codepoint < 8417) return SFCE_UNICODE_CATEGORY_ME;
    if (codepoint > 8417 && codepoint < 8421) return SFCE_UNICODE_CATEGORY_ME;
    if (codepoint > 8420 && codepoint < 8433) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 8447 && codepoint < 8450) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 8450 && codepoint < 8455) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 8455 && codepoint < 8458) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 8458 && codepoint < 8462) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 8461 && codepoint < 8464) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 8463 && codepoint < 8467) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 8469 && codepoint < 8472) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 8472 && codepoint < 8478) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 8477 && codepoint < 8484) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 8489 && codepoint < 8494) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 8495 && codepoint < 8500) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 8500 && codepoint < 8505) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 8505 && codepoint < 8508) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 8507 && codepoint < 8510) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 8509 && codepoint < 8512) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 8511 && codepoint < 8517) return SFCE_UNICODE_CATEGORY_SM;
    if (codepoint > 8517 && codepoint < 8522) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 8523 && codepoint < 8526) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 8527 && codepoint < 8544) return SFCE_UNICODE_CATEGORY_NO;
    if (codepoint > 8543 && codepoint < 8579) return SFCE_UNICODE_CATEGORY_NL;
    if (codepoint > 8580 && codepoint < 8585) return SFCE_UNICODE_CATEGORY_NL;
    if (codepoint > 8585 && codepoint < 8588) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 8591 && codepoint < 8597) return SFCE_UNICODE_CATEGORY_SM;
    if (codepoint > 8596 && codepoint < 8602) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 8601 && codepoint < 8604) return SFCE_UNICODE_CATEGORY_SM;
    if (codepoint > 8603 && codepoint < 8608) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 8608 && codepoint < 8611) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 8611 && codepoint < 8614) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 8614 && codepoint < 8622) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 8622 && codepoint < 8654) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 8653 && codepoint < 8656) return SFCE_UNICODE_CATEGORY_SM;
    if (codepoint > 8655 && codepoint < 8658) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 8660 && codepoint < 8692) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 8691 && codepoint < 8960) return SFCE_UNICODE_CATEGORY_SM;
    if (codepoint > 8959 && codepoint < 8968) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 8971 && codepoint < 8992) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 8991 && codepoint < 8994) return SFCE_UNICODE_CATEGORY_SM;
    if (codepoint > 8993 && codepoint < 9001) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 9002 && codepoint < 9084) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 9084 && codepoint < 9115) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 9114 && codepoint < 9140) return SFCE_UNICODE_CATEGORY_SM;
    if (codepoint > 9139 && codepoint < 9180) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 9179 && codepoint < 9186) return SFCE_UNICODE_CATEGORY_SM;
    if (codepoint > 9185 && codepoint < 9258) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 9279 && codepoint < 9291) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 9311 && codepoint < 9372) return SFCE_UNICODE_CATEGORY_NO;
    if (codepoint > 9371 && codepoint < 9450) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 9449 && codepoint < 9472) return SFCE_UNICODE_CATEGORY_NO;
    if (codepoint > 9471 && codepoint < 9655) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 9655 && codepoint < 9665) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 9665 && codepoint < 9720) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 9719 && codepoint < 9728) return SFCE_UNICODE_CATEGORY_SM;
    if (codepoint > 9727 && codepoint < 9839) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 9839 && codepoint < 10088) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 10101 && codepoint < 10132) return SFCE_UNICODE_CATEGORY_NO;
    if (codepoint > 10131 && codepoint < 10176) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 10175 && codepoint < 10181) return SFCE_UNICODE_CATEGORY_SM;
    if (codepoint > 10182 && codepoint < 10214) return SFCE_UNICODE_CATEGORY_SM;
    if (codepoint > 10223 && codepoint < 10240) return SFCE_UNICODE_CATEGORY_SM;
    if (codepoint > 10239 && codepoint < 10496) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 10495 && codepoint < 10627) return SFCE_UNICODE_CATEGORY_SM;
    if (codepoint > 10648 && codepoint < 10712) return SFCE_UNICODE_CATEGORY_SM;
    if (codepoint > 10715 && codepoint < 10748) return SFCE_UNICODE_CATEGORY_SM;
    if (codepoint > 10749 && codepoint < 11008) return SFCE_UNICODE_CATEGORY_SM;
    if (codepoint > 11007 && codepoint < 11056) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 11055 && codepoint < 11077) return SFCE_UNICODE_CATEGORY_SM;
    if (codepoint > 11076 && codepoint < 11079) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 11078 && codepoint < 11085) return SFCE_UNICODE_CATEGORY_SM;
    if (codepoint > 11084 && codepoint < 11124) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 11125 && codepoint < 11158) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 11158 && codepoint < 11264) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 11263 && codepoint < 11312) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 11311 && codepoint < 11360) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 11361 && codepoint < 11365) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 11364 && codepoint < 11367) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 11372 && codepoint < 11377) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 11378 && codepoint < 11381) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 11381 && codepoint < 11388) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 11387 && codepoint < 11390) return SFCE_UNICODE_CATEGORY_LM;
    if (codepoint > 11389 && codepoint < 11393) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 11490 && codepoint < 11493) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 11492 && codepoint < 11499) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 11502 && codepoint < 11506) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 11512 && codepoint < 11517) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 11517 && codepoint < 11520) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 11519 && codepoint < 11558) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 11567 && codepoint < 11624) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 11647 && codepoint < 11671) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 11679 && codepoint < 11687) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 11687 && codepoint < 11695) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 11695 && codepoint < 11703) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 11703 && codepoint < 11711) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 11711 && codepoint < 11719) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 11719 && codepoint < 11727) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 11727 && codepoint < 11735) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 11735 && codepoint < 11743) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 11743 && codepoint < 11776) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 11775 && codepoint < 11778) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 11781 && codepoint < 11785) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 11789 && codepoint < 11799) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 11799 && codepoint < 11802) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 11805 && codepoint < 11808) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 11817 && codepoint < 11823) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 11823 && codepoint < 11834) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 11833 && codepoint < 11836) return SFCE_UNICODE_CATEGORY_PD;
    if (codepoint > 11835 && codepoint < 11840) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 11842 && codepoint < 11856) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 11855 && codepoint < 11858) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 11857 && codepoint < 11861) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 11903 && codepoint < 11930) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 11930 && codepoint < 12020) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 12031 && codepoint < 12246) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 12271 && codepoint < 12288) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 12288 && codepoint < 12292) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 12305 && codepoint < 12308) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 12317 && codepoint < 12320) return SFCE_UNICODE_CATEGORY_PE;
    if (codepoint > 12320 && codepoint < 12330) return SFCE_UNICODE_CATEGORY_NL;
    if (codepoint > 12329 && codepoint < 12334) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 12333 && codepoint < 12336) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 12336 && codepoint < 12342) return SFCE_UNICODE_CATEGORY_LM;
    if (codepoint > 12341 && codepoint < 12344) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 12343 && codepoint < 12347) return SFCE_UNICODE_CATEGORY_NL;
    if (codepoint > 12349 && codepoint < 12352) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 12352 && codepoint < 12439) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 12440 && codepoint < 12443) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 12442 && codepoint < 12445) return SFCE_UNICODE_CATEGORY_SK;
    if (codepoint > 12444 && codepoint < 12447) return SFCE_UNICODE_CATEGORY_LM;
    if (codepoint > 12448 && codepoint < 12539) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 12539 && codepoint < 12543) return SFCE_UNICODE_CATEGORY_LM;
    if (codepoint > 12548 && codepoint < 12592) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 12592 && codepoint < 12687) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 12687 && codepoint < 12690) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 12689 && codepoint < 12694) return SFCE_UNICODE_CATEGORY_NO;
    if (codepoint > 12693 && codepoint < 12704) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 12703 && codepoint < 12736) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 12735 && codepoint < 12774) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 12783 && codepoint < 12800) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 12799 && codepoint < 12831) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 12831 && codepoint < 12842) return SFCE_UNICODE_CATEGORY_NO;
    if (codepoint > 12841 && codepoint < 12872) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 12871 && codepoint < 12880) return SFCE_UNICODE_CATEGORY_NO;
    if (codepoint > 12880 && codepoint < 12896) return SFCE_UNICODE_CATEGORY_NO;
    if (codepoint > 12895 && codepoint < 12928) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 12927 && codepoint < 12938) return SFCE_UNICODE_CATEGORY_NO;
    if (codepoint > 12937 && codepoint < 12977) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 12976 && codepoint < 12992) return SFCE_UNICODE_CATEGORY_NO;
    if (codepoint > 12991 && codepoint < 13312) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 19903 && codepoint < 19968) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 40958 && codepoint < 40981) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 40981 && codepoint < 42125) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 42127 && codepoint < 42183) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 42191 && codepoint < 42232) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 42231 && codepoint < 42238) return SFCE_UNICODE_CATEGORY_LM;
    if (codepoint > 42237 && codepoint < 42240) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 42239 && codepoint < 42508) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 42508 && codepoint < 42512) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 42511 && codepoint < 42528) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 42527 && codepoint < 42538) return SFCE_UNICODE_CATEGORY_ND;
    if (codepoint > 42537 && codepoint < 42540) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 42607 && codepoint < 42611) return SFCE_UNICODE_CATEGORY_ME;
    if (codepoint > 42611 && codepoint < 42622) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 42651 && codepoint < 42654) return SFCE_UNICODE_CATEGORY_LM;
    if (codepoint > 42653 && codepoint < 42656) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 42655 && codepoint < 42726) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 42725 && codepoint < 42736) return SFCE_UNICODE_CATEGORY_NL;
    if (codepoint > 42735 && codepoint < 42738) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 42737 && codepoint < 42744) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 42751 && codepoint < 42775) return SFCE_UNICODE_CATEGORY_SK;
    if (codepoint > 42774 && codepoint < 42784) return SFCE_UNICODE_CATEGORY_LM;
    if (codepoint > 42783 && codepoint < 42786) return SFCE_UNICODE_CATEGORY_SK;
    if (codepoint > 42798 && codepoint < 42802) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 42864 && codepoint < 42873) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 42876 && codepoint < 42879) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 42888 && codepoint < 42891) return SFCE_UNICODE_CATEGORY_SK;
    if (codepoint > 42898 && codepoint < 42902) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 42921 && codepoint < 42927) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 42927 && codepoint < 42933) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 42947 && codepoint < 42952) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 42954 && codepoint < 42957) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 42993 && codepoint < 42997) return SFCE_UNICODE_CATEGORY_LM;
    if (codepoint > 42999 && codepoint < 43002) return SFCE_UNICODE_CATEGORY_LM;
    if (codepoint > 43002 && codepoint < 43010) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 43010 && codepoint < 43014) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 43014 && codepoint < 43019) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 43019 && codepoint < 43043) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 43042 && codepoint < 43045) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 43044 && codepoint < 43047) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 43047 && codepoint < 43052) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 43055 && codepoint < 43062) return SFCE_UNICODE_CATEGORY_NO;
    if (codepoint > 43061 && codepoint < 43064) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 43071 && codepoint < 43124) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 43123 && codepoint < 43128) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 43135 && codepoint < 43138) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 43137 && codepoint < 43188) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 43187 && codepoint < 43204) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 43203 && codepoint < 43206) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 43213 && codepoint < 43216) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 43215 && codepoint < 43226) return SFCE_UNICODE_CATEGORY_ND;
    if (codepoint > 43231 && codepoint < 43250) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 43249 && codepoint < 43256) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 43255 && codepoint < 43259) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 43260 && codepoint < 43263) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 43263 && codepoint < 43274) return SFCE_UNICODE_CATEGORY_ND;
    if (codepoint > 43273 && codepoint < 43302) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 43301 && codepoint < 43310) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 43309 && codepoint < 43312) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 43311 && codepoint < 43335) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 43334 && codepoint < 43346) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 43345 && codepoint < 43348) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 43359 && codepoint < 43389) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 43391 && codepoint < 43395) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 43395 && codepoint < 43443) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 43443 && codepoint < 43446) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 43445 && codepoint < 43450) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 43449 && codepoint < 43452) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 43451 && codepoint < 43454) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 43453 && codepoint < 43457) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 43456 && codepoint < 43470) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 43471 && codepoint < 43482) return SFCE_UNICODE_CATEGORY_ND;
    if (codepoint > 43485 && codepoint < 43488) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 43487 && codepoint < 43493) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 43494 && codepoint < 43504) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 43503 && codepoint < 43514) return SFCE_UNICODE_CATEGORY_ND;
    if (codepoint > 43513 && codepoint < 43519) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 43519 && codepoint < 43561) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 43560 && codepoint < 43567) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 43566 && codepoint < 43569) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 43568 && codepoint < 43571) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 43570 && codepoint < 43573) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 43572 && codepoint < 43575) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 43583 && codepoint < 43587) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 43587 && codepoint < 43596) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 43599 && codepoint < 43610) return SFCE_UNICODE_CATEGORY_ND;
    if (codepoint > 43611 && codepoint < 43616) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 43615 && codepoint < 43632) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 43632 && codepoint < 43639) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 43638 && codepoint < 43642) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 43645 && codepoint < 43696) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 43697 && codepoint < 43701) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 43700 && codepoint < 43703) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 43702 && codepoint < 43705) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 43704 && codepoint < 43710) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 43709 && codepoint < 43712) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 43738 && codepoint < 43741) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 43741 && codepoint < 43744) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 43743 && codepoint < 43755) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 43755 && codepoint < 43758) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 43757 && codepoint < 43760) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 43759 && codepoint < 43762) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 43762 && codepoint < 43765) return SFCE_UNICODE_CATEGORY_LM;
    if (codepoint > 43776 && codepoint < 43783) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 43784 && codepoint < 43791) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 43792 && codepoint < 43799) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 43807 && codepoint < 43815) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 43815 && codepoint < 43823) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 43823 && codepoint < 43867) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 43867 && codepoint < 43872) return SFCE_UNICODE_CATEGORY_LM;
    if (codepoint > 43871 && codepoint < 43881) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 43881 && codepoint < 43884) return SFCE_UNICODE_CATEGORY_SK;
    if (codepoint > 43887 && codepoint < 43968) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 43967 && codepoint < 44003) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 44002 && codepoint < 44005) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 44005 && codepoint < 44008) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 44008 && codepoint < 44011) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 44015 && codepoint < 44026) return SFCE_UNICODE_CATEGORY_ND;
    if (codepoint > 55215 && codepoint < 55239) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 55242 && codepoint < 55292) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 56190 && codepoint < 56193) return SFCE_UNICODE_CATEGORY_CS;
    if (codepoint > 56318 && codepoint < 56321) return SFCE_UNICODE_CATEGORY_CS;
    if (codepoint > 63743 && codepoint < 64110) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 64111 && codepoint < 64218) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 64255 && codepoint < 64263) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 64274 && codepoint < 64280) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 64286 && codepoint < 64297) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 64297 && codepoint < 64311) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 64311 && codepoint < 64317) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 64319 && codepoint < 64322) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 64322 && codepoint < 64325) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 64325 && codepoint < 64434) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 64433 && codepoint < 64451) return SFCE_UNICODE_CATEGORY_SK;
    if (codepoint > 64466 && codepoint < 64830) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 64831 && codepoint < 64848) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 64847 && codepoint < 64912) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 64913 && codepoint < 64968) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 65007 && codepoint < 65020) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 65020 && codepoint < 65024) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 65023 && codepoint < 65040) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 65039 && codepoint < 65047) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 65055 && codepoint < 65072) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 65072 && codepoint < 65075) return SFCE_UNICODE_CATEGORY_PD;
    if (codepoint > 65074 && codepoint < 65077) return SFCE_UNICODE_CATEGORY_PC;
    if (codepoint > 65092 && codepoint < 65095) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 65096 && codepoint < 65101) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 65100 && codepoint < 65104) return SFCE_UNICODE_CATEGORY_PC;
    if (codepoint > 65103 && codepoint < 65107) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 65107 && codepoint < 65112) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 65118 && codepoint < 65122) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 65123 && codepoint < 65127) return SFCE_UNICODE_CATEGORY_SM;
    if (codepoint > 65129 && codepoint < 65132) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 65135 && codepoint < 65141) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 65141 && codepoint < 65277) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 65280 && codepoint < 65284) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 65284 && codepoint < 65288) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 65293 && codepoint < 65296) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 65295 && codepoint < 65306) return SFCE_UNICODE_CATEGORY_ND;
    if (codepoint > 65305 && codepoint < 65308) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 65307 && codepoint < 65311) return SFCE_UNICODE_CATEGORY_SM;
    if (codepoint > 65310 && codepoint < 65313) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 65312 && codepoint < 65339) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 65344 && codepoint < 65371) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 65379 && codepoint < 65382) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 65381 && codepoint < 65392) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 65392 && codepoint < 65438) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 65437 && codepoint < 65440) return SFCE_UNICODE_CATEGORY_LM;
    if (codepoint > 65439 && codepoint < 65471) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 65473 && codepoint < 65480) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 65481 && codepoint < 65488) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 65489 && codepoint < 65496) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 65497 && codepoint < 65501) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 65503 && codepoint < 65506) return SFCE_UNICODE_CATEGORY_SC;
    if (codepoint > 65508 && codepoint < 65511) return SFCE_UNICODE_CATEGORY_SC;
    if (codepoint > 65512 && codepoint < 65517) return SFCE_UNICODE_CATEGORY_SM;
    if (codepoint > 65516 && codepoint < 65519) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 65528 && codepoint < 65532) return SFCE_UNICODE_CATEGORY_CF;
    if (codepoint > 65531 && codepoint < 65534) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 65535 && codepoint < 65548) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 65548 && codepoint < 65575) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 65575 && codepoint < 65595) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 65595 && codepoint < 65598) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 65598 && codepoint < 65614) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 65615 && codepoint < 65630) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 65663 && codepoint < 65787) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 65791 && codepoint < 65795) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 65798 && codepoint < 65844) return SFCE_UNICODE_CATEGORY_NO;
    if (codepoint > 65846 && codepoint < 65856) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 65855 && codepoint < 65909) return SFCE_UNICODE_CATEGORY_NL;
    if (codepoint > 65908 && codepoint < 65913) return SFCE_UNICODE_CATEGORY_NO;
    if (codepoint > 65912 && codepoint < 65930) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 65929 && codepoint < 65932) return SFCE_UNICODE_CATEGORY_NO;
    if (codepoint > 65931 && codepoint < 65935) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 65935 && codepoint < 65949) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 65999 && codepoint < 66045) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 66175 && codepoint < 66205) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 66207 && codepoint < 66257) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 66272 && codepoint < 66300) return SFCE_UNICODE_CATEGORY_NO;
    if (codepoint > 66303 && codepoint < 66336) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 66335 && codepoint < 66340) return SFCE_UNICODE_CATEGORY_NO;
    if (codepoint > 66348 && codepoint < 66369) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 66369 && codepoint < 66378) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 66383 && codepoint < 66422) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 66421 && codepoint < 66427) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 66431 && codepoint < 66462) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 66463 && codepoint < 66500) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 66503 && codepoint < 66512) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 66512 && codepoint < 66518) return SFCE_UNICODE_CATEGORY_NL;
    if (codepoint > 66559 && codepoint < 66600) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 66599 && codepoint < 66640) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 66639 && codepoint < 66718) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 66719 && codepoint < 66730) return SFCE_UNICODE_CATEGORY_ND;
    if (codepoint > 66735 && codepoint < 66772) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 66775 && codepoint < 66812) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 66815 && codepoint < 66856) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 66863 && codepoint < 66916) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 66927 && codepoint < 66939) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 66939 && codepoint < 66955) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 66955 && codepoint < 66963) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 66963 && codepoint < 66966) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 66966 && codepoint < 66978) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 66978 && codepoint < 66994) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 66994 && codepoint < 67002) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 67002 && codepoint < 67005) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 67007 && codepoint < 67060) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 67071 && codepoint < 67383) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 67391 && codepoint < 67414) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 67423 && codepoint < 67432) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 67455 && codepoint < 67462) return SFCE_UNICODE_CATEGORY_LM;
    if (codepoint > 67462 && codepoint < 67505) return SFCE_UNICODE_CATEGORY_LM;
    if (codepoint > 67505 && codepoint < 67515) return SFCE_UNICODE_CATEGORY_LM;
    if (codepoint > 67583 && codepoint < 67590) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 67593 && codepoint < 67638) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 67638 && codepoint < 67641) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 67646 && codepoint < 67670) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 67671 && codepoint < 67680) return SFCE_UNICODE_CATEGORY_NO;
    if (codepoint > 67679 && codepoint < 67703) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 67702 && codepoint < 67705) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 67704 && codepoint < 67712) return SFCE_UNICODE_CATEGORY_NO;
    if (codepoint > 67711 && codepoint < 67743) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 67750 && codepoint < 67760) return SFCE_UNICODE_CATEGORY_NO;
    if (codepoint > 67807 && codepoint < 67827) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 67827 && codepoint < 67830) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 67834 && codepoint < 67840) return SFCE_UNICODE_CATEGORY_NO;
    if (codepoint > 67839 && codepoint < 67862) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 67861 && codepoint < 67868) return SFCE_UNICODE_CATEGORY_NO;
    if (codepoint > 67871 && codepoint < 67898) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 67967 && codepoint < 68024) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 68027 && codepoint < 68030) return SFCE_UNICODE_CATEGORY_NO;
    if (codepoint > 68029 && codepoint < 68032) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 68031 && codepoint < 68048) return SFCE_UNICODE_CATEGORY_NO;
    if (codepoint > 68049 && codepoint < 68096) return SFCE_UNICODE_CATEGORY_NO;
    if (codepoint > 68096 && codepoint < 68100) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 68100 && codepoint < 68103) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 68107 && codepoint < 68112) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 68111 && codepoint < 68116) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 68116 && codepoint < 68120) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 68120 && codepoint < 68150) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 68151 && codepoint < 68155) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 68159 && codepoint < 68169) return SFCE_UNICODE_CATEGORY_NO;
    if (codepoint > 68175 && codepoint < 68185) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 68191 && codepoint < 68221) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 68220 && codepoint < 68223) return SFCE_UNICODE_CATEGORY_NO;
    if (codepoint > 68223 && codepoint < 68253) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 68252 && codepoint < 68256) return SFCE_UNICODE_CATEGORY_NO;
    if (codepoint > 68287 && codepoint < 68296) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 68296 && codepoint < 68325) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 68324 && codepoint < 68327) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 68330 && codepoint < 68336) return SFCE_UNICODE_CATEGORY_NO;
    if (codepoint > 68335 && codepoint < 68343) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 68351 && codepoint < 68406) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 68408 && codepoint < 68416) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 68415 && codepoint < 68438) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 68439 && codepoint < 68448) return SFCE_UNICODE_CATEGORY_NO;
    if (codepoint > 68447 && codepoint < 68467) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 68471 && codepoint < 68480) return SFCE_UNICODE_CATEGORY_NO;
    if (codepoint > 68479 && codepoint < 68498) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 68504 && codepoint < 68509) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 68520 && codepoint < 68528) return SFCE_UNICODE_CATEGORY_NO;
    if (codepoint > 68607 && codepoint < 68681) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 68735 && codepoint < 68787) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 68799 && codepoint < 68851) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 68857 && codepoint < 68864) return SFCE_UNICODE_CATEGORY_NO;
    if (codepoint > 68863 && codepoint < 68900) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 68899 && codepoint < 68904) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 68911 && codepoint < 68922) return SFCE_UNICODE_CATEGORY_ND;
    if (codepoint > 68927 && codepoint < 68938) return SFCE_UNICODE_CATEGORY_ND;
    if (codepoint > 68937 && codepoint < 68942) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 68943 && codepoint < 68966) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 68968 && codepoint < 68974) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 68975 && codepoint < 68998) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 69005 && codepoint < 69008) return SFCE_UNICODE_CATEGORY_SM;
    if (codepoint > 69215 && codepoint < 69247) return SFCE_UNICODE_CATEGORY_NO;
    if (codepoint > 69247 && codepoint < 69290) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 69290 && codepoint < 69293) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 69295 && codepoint < 69298) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 69313 && codepoint < 69317) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 69371 && codepoint < 69376) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 69375 && codepoint < 69405) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 69404 && codepoint < 69415) return SFCE_UNICODE_CATEGORY_NO;
    if (codepoint > 69423 && codepoint < 69446) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 69445 && codepoint < 69457) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 69456 && codepoint < 69461) return SFCE_UNICODE_CATEGORY_NO;
    if (codepoint > 69460 && codepoint < 69466) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 69487 && codepoint < 69506) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 69505 && codepoint < 69510) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 69509 && codepoint < 69514) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 69551 && codepoint < 69573) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 69572 && codepoint < 69580) return SFCE_UNICODE_CATEGORY_NO;
    if (codepoint > 69599 && codepoint < 69623) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 69634 && codepoint < 69688) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 69687 && codepoint < 69703) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 69702 && codepoint < 69710) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 69713 && codepoint < 69734) return SFCE_UNICODE_CATEGORY_NO;
    if (codepoint > 69733 && codepoint < 69744) return SFCE_UNICODE_CATEGORY_ND;
    if (codepoint > 69744 && codepoint < 69747) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 69746 && codepoint < 69749) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 69758 && codepoint < 69762) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 69762 && codepoint < 69808) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 69807 && codepoint < 69811) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 69810 && codepoint < 69815) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 69814 && codepoint < 69817) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 69816 && codepoint < 69819) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 69818 && codepoint < 69821) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 69821 && codepoint < 69826) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 69839 && codepoint < 69865) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 69871 && codepoint < 69882) return SFCE_UNICODE_CATEGORY_ND;
    if (codepoint > 69887 && codepoint < 69891) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 69890 && codepoint < 69927) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 69926 && codepoint < 69932) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 69932 && codepoint < 69941) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 69941 && codepoint < 69952) return SFCE_UNICODE_CATEGORY_ND;
    if (codepoint > 69951 && codepoint < 69956) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 69956 && codepoint < 69959) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 69967 && codepoint < 70003) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 70003 && codepoint < 70006) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 70015 && codepoint < 70018) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 70018 && codepoint < 70067) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 70066 && codepoint < 70070) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 70069 && codepoint < 70079) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 70078 && codepoint < 70081) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 70080 && codepoint < 70085) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 70084 && codepoint < 70089) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 70088 && codepoint < 70093) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 70095 && codepoint < 70106) return SFCE_UNICODE_CATEGORY_ND;
    if (codepoint > 70108 && codepoint < 70112) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 70112 && codepoint < 70133) return SFCE_UNICODE_CATEGORY_NO;
    if (codepoint > 70143 && codepoint < 70162) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 70162 && codepoint < 70188) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 70187 && codepoint < 70191) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 70190 && codepoint < 70194) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 70193 && codepoint < 70196) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 70197 && codepoint < 70200) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 70199 && codepoint < 70206) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 70206 && codepoint < 70209) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 70271 && codepoint < 70279) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 70281 && codepoint < 70286) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 70286 && codepoint < 70302) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 70302 && codepoint < 70313) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 70319 && codepoint < 70367) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 70367 && codepoint < 70371) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 70370 && codepoint < 70379) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 70383 && codepoint < 70394) return SFCE_UNICODE_CATEGORY_ND;
    if (codepoint > 70399 && codepoint < 70402) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 70401 && codepoint < 70404) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 70404 && codepoint < 70413) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 70414 && codepoint < 70417) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 70418 && codepoint < 70441) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 70441 && codepoint < 70449) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 70449 && codepoint < 70452) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 70452 && codepoint < 70458) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 70458 && codepoint < 70461) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 70461 && codepoint < 70464) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 70464 && codepoint < 70469) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 70470 && codepoint < 70473) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 70474 && codepoint < 70478) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 70492 && codepoint < 70498) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 70497 && codepoint < 70500) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 70501 && codepoint < 70509) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 70511 && codepoint < 70517) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 70527 && codepoint < 70538) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 70543 && codepoint < 70582) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 70583 && codepoint < 70587) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 70586 && codepoint < 70593) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 70598 && codepoint < 70603) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 70603 && codepoint < 70606) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 70611 && codepoint < 70614) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 70614 && codepoint < 70617) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 70624 && codepoint < 70627) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 70655 && codepoint < 70709) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 70708 && codepoint < 70712) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 70711 && codepoint < 70720) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 70719 && codepoint < 70722) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 70721 && codepoint < 70725) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 70726 && codepoint < 70731) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 70730 && codepoint < 70736) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 70735 && codepoint < 70746) return SFCE_UNICODE_CATEGORY_ND;
    if (codepoint > 70745 && codepoint < 70748) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 70750 && codepoint < 70754) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 70783 && codepoint < 70832) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 70831 && codepoint < 70835) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 70834 && codepoint < 70841) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 70842 && codepoint < 70847) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 70846 && codepoint < 70849) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 70849 && codepoint < 70852) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 70851 && codepoint < 70854) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 70863 && codepoint < 70874) return SFCE_UNICODE_CATEGORY_ND;
    if (codepoint > 71039 && codepoint < 71087) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 71086 && codepoint < 71090) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 71089 && codepoint < 71094) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 71095 && codepoint < 71100) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 71099 && codepoint < 71102) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 71102 && codepoint < 71105) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 71104 && codepoint < 71128) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 71127 && codepoint < 71132) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 71131 && codepoint < 71134) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 71167 && codepoint < 71216) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 71215 && codepoint < 71219) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 71218 && codepoint < 71227) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 71226 && codepoint < 71229) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 71230 && codepoint < 71233) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 71232 && codepoint < 71236) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 71247 && codepoint < 71258) return SFCE_UNICODE_CATEGORY_ND;
    if (codepoint > 71263 && codepoint < 71277) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 71295 && codepoint < 71339) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 71341 && codepoint < 71344) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 71343 && codepoint < 71350) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 71359 && codepoint < 71370) return SFCE_UNICODE_CATEGORY_ND;
    if (codepoint > 71375 && codepoint < 71396) return SFCE_UNICODE_CATEGORY_ND;
    if (codepoint > 71423 && codepoint < 71451) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 71455 && codepoint < 71458) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 71457 && codepoint < 71462) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 71462 && codepoint < 71468) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 71471 && codepoint < 71482) return SFCE_UNICODE_CATEGORY_ND;
    if (codepoint > 71481 && codepoint < 71484) return SFCE_UNICODE_CATEGORY_NO;
    if (codepoint > 71483 && codepoint < 71487) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 71487 && codepoint < 71495) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 71679 && codepoint < 71724) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 71723 && codepoint < 71727) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 71726 && codepoint < 71736) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 71736 && codepoint < 71739) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 71839 && codepoint < 71872) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 71871 && codepoint < 71904) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 71903 && codepoint < 71914) return SFCE_UNICODE_CATEGORY_ND;
    if (codepoint > 71913 && codepoint < 71923) return SFCE_UNICODE_CATEGORY_NO;
    if (codepoint > 71934 && codepoint < 71943) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 71947 && codepoint < 71956) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 71956 && codepoint < 71959) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 71959 && codepoint < 71984) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 71983 && codepoint < 71990) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 71990 && codepoint < 71993) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 71994 && codepoint < 71997) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 72003 && codepoint < 72007) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 72015 && codepoint < 72026) return SFCE_UNICODE_CATEGORY_ND;
    if (codepoint > 72095 && codepoint < 72104) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 72105 && codepoint < 72145) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 72144 && codepoint < 72148) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 72147 && codepoint < 72152) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 72153 && codepoint < 72156) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 72155 && codepoint < 72160) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 72192 && codepoint < 72203) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 72202 && codepoint < 72243) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 72242 && codepoint < 72249) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 72250 && codepoint < 72255) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 72254 && codepoint < 72263) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 72272 && codepoint < 72279) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 72278 && codepoint < 72281) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 72280 && codepoint < 72284) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 72283 && codepoint < 72330) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 72329 && codepoint < 72343) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 72343 && codepoint < 72346) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 72345 && codepoint < 72349) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 72349 && codepoint < 72355) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 72367 && codepoint < 72441) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 72447 && codepoint < 72458) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 72639 && codepoint < 72673) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 72687 && codepoint < 72698) return SFCE_UNICODE_CATEGORY_ND;
    if (codepoint > 72703 && codepoint < 72713) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 72713 && codepoint < 72751) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 72751 && codepoint < 72759) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 72759 && codepoint < 72766) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 72768 && codepoint < 72774) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 72783 && codepoint < 72794) return SFCE_UNICODE_CATEGORY_ND;
    if (codepoint > 72793 && codepoint < 72813) return SFCE_UNICODE_CATEGORY_NO;
    if (codepoint > 72815 && codepoint < 72818) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 72817 && codepoint < 72848) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 72849 && codepoint < 72872) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 72873 && codepoint < 72881) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 72881 && codepoint < 72884) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 72884 && codepoint < 72887) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 72959 && codepoint < 72967) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 72967 && codepoint < 72970) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 72970 && codepoint < 73009) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 73008 && codepoint < 73015) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 73019 && codepoint < 73022) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 73022 && codepoint < 73030) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 73039 && codepoint < 73050) return SFCE_UNICODE_CATEGORY_ND;
    if (codepoint > 73055 && codepoint < 73062) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 73062 && codepoint < 73065) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 73065 && codepoint < 73098) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 73097 && codepoint < 73103) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 73103 && codepoint < 73106) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 73106 && codepoint < 73109) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 73119 && codepoint < 73130) return SFCE_UNICODE_CATEGORY_ND;
    if (codepoint > 73439 && codepoint < 73459) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 73458 && codepoint < 73461) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 73460 && codepoint < 73463) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 73462 && codepoint < 73465) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 73471 && codepoint < 73474) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 73475 && codepoint < 73489) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 73489 && codepoint < 73524) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 73523 && codepoint < 73526) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 73525 && codepoint < 73531) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 73533 && codepoint < 73536) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 73538 && codepoint < 73552) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 73551 && codepoint < 73562) return SFCE_UNICODE_CATEGORY_ND;
    if (codepoint > 73663 && codepoint < 73685) return SFCE_UNICODE_CATEGORY_NO;
    if (codepoint > 73684 && codepoint < 73693) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 73692 && codepoint < 73697) return SFCE_UNICODE_CATEGORY_SC;
    if (codepoint > 73696 && codepoint < 73714) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 73727 && codepoint < 74650) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 74751 && codepoint < 74863) return SFCE_UNICODE_CATEGORY_NL;
    if (codepoint > 74863 && codepoint < 74869) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 74879 && codepoint < 75076) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 77711 && codepoint < 77809) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 77808 && codepoint < 77811) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 77823 && codepoint < 78896) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 78895 && codepoint < 78912) return SFCE_UNICODE_CATEGORY_CF;
    if (codepoint > 78912 && codepoint < 78919) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 78918 && codepoint < 78934) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 78943 && codepoint < 82939) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 82943 && codepoint < 83527) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 90367 && codepoint < 90398) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 90397 && codepoint < 90410) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 90409 && codepoint < 90413) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 90412 && codepoint < 90416) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 90415 && codepoint < 90426) return SFCE_UNICODE_CATEGORY_ND;
    if (codepoint > 92159 && codepoint < 92729) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 92735 && codepoint < 92767) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 92767 && codepoint < 92778) return SFCE_UNICODE_CATEGORY_ND;
    if (codepoint > 92781 && codepoint < 92784) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 92783 && codepoint < 92863) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 92863 && codepoint < 92874) return SFCE_UNICODE_CATEGORY_ND;
    if (codepoint > 92879 && codepoint < 92910) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 92911 && codepoint < 92917) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 92927 && codepoint < 92976) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 92975 && codepoint < 92983) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 92982 && codepoint < 92988) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 92987 && codepoint < 92992) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 92991 && codepoint < 92996) return SFCE_UNICODE_CATEGORY_LM;
    if (codepoint > 93007 && codepoint < 93018) return SFCE_UNICODE_CATEGORY_ND;
    if (codepoint > 93018 && codepoint < 93026) return SFCE_UNICODE_CATEGORY_NO;
    if (codepoint > 93026 && codepoint < 93048) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 93052 && codepoint < 93072) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 93503 && codepoint < 93507) return SFCE_UNICODE_CATEGORY_LM;
    if (codepoint > 93506 && codepoint < 93547) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 93546 && codepoint < 93549) return SFCE_UNICODE_CATEGORY_LM;
    if (codepoint > 93548 && codepoint < 93552) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 93551 && codepoint < 93562) return SFCE_UNICODE_CATEGORY_ND;
    if (codepoint > 93759 && codepoint < 93792) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 93791 && codepoint < 93824) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 93823 && codepoint < 93847) return SFCE_UNICODE_CATEGORY_NO;
    if (codepoint > 93846 && codepoint < 93851) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 93951 && codepoint < 94027) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 94032 && codepoint < 94088) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 94094 && codepoint < 94099) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 94098 && codepoint < 94112) return SFCE_UNICODE_CATEGORY_LM;
    if (codepoint > 94175 && codepoint < 94178) return SFCE_UNICODE_CATEGORY_LM;
    if (codepoint > 94191 && codepoint < 94194) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 100351 && codepoint < 101590) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 101630 && codepoint < 101633) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 110575 && codepoint < 110580) return SFCE_UNICODE_CATEGORY_LM;
    if (codepoint > 110580 && codepoint < 110588) return SFCE_UNICODE_CATEGORY_LM;
    if (codepoint > 110588 && codepoint < 110591) return SFCE_UNICODE_CATEGORY_LM;
    if (codepoint > 110591 && codepoint < 110883) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 110927 && codepoint < 110931) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 110947 && codepoint < 110952) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 110959 && codepoint < 111356) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 113663 && codepoint < 113771) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 113775 && codepoint < 113789) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 113791 && codepoint < 113801) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 113807 && codepoint < 113818) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 113820 && codepoint < 113823) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 113823 && codepoint < 113828) return SFCE_UNICODE_CATEGORY_CF;
    if (codepoint > 117759 && codepoint < 118000) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 117999 && codepoint < 118010) return SFCE_UNICODE_CATEGORY_ND;
    if (codepoint > 118015 && codepoint < 118452) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 118527 && codepoint < 118574) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 118575 && codepoint < 118599) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 118607 && codepoint < 118724) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 118783 && codepoint < 119030) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 119039 && codepoint < 119079) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 119080 && codepoint < 119141) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 119140 && codepoint < 119143) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 119142 && codepoint < 119146) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 119145 && codepoint < 119149) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 119148 && codepoint < 119155) return SFCE_UNICODE_CATEGORY_MC;
    if (codepoint > 119154 && codepoint < 119163) return SFCE_UNICODE_CATEGORY_CF;
    if (codepoint > 119162 && codepoint < 119171) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 119170 && codepoint < 119173) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 119172 && codepoint < 119180) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 119179 && codepoint < 119210) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 119209 && codepoint < 119214) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 119213 && codepoint < 119275) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 119295 && codepoint < 119362) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 119361 && codepoint < 119365) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 119487 && codepoint < 119508) return SFCE_UNICODE_CATEGORY_NO;
    if (codepoint > 119519 && codepoint < 119540) return SFCE_UNICODE_CATEGORY_NO;
    if (codepoint > 119551 && codepoint < 119639) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 119647 && codepoint < 119673) return SFCE_UNICODE_CATEGORY_NO;
    if (codepoint > 119807 && codepoint < 119834) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 119833 && codepoint < 119860) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 119859 && codepoint < 119886) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 119885 && codepoint < 119893) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 119893 && codepoint < 119912) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 119911 && codepoint < 119938) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 119937 && codepoint < 119964) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 119965 && codepoint < 119968) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 119972 && codepoint < 119975) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 119976 && codepoint < 119981) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 119981 && codepoint < 119990) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 119989 && codepoint < 119994) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 119996 && codepoint < 120004) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 120004 && codepoint < 120016) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 120015 && codepoint < 120042) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 120041 && codepoint < 120068) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 120067 && codepoint < 120070) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 120070 && codepoint < 120075) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 120076 && codepoint < 120085) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 120085 && codepoint < 120093) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 120093 && codepoint < 120120) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 120119 && codepoint < 120122) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 120122 && codepoint < 120127) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 120127 && codepoint < 120133) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 120137 && codepoint < 120145) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 120145 && codepoint < 120172) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 120171 && codepoint < 120198) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 120197 && codepoint < 120224) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 120223 && codepoint < 120250) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 120249 && codepoint < 120276) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 120275 && codepoint < 120302) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 120301 && codepoint < 120328) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 120327 && codepoint < 120354) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 120353 && codepoint < 120380) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 120379 && codepoint < 120406) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 120405 && codepoint < 120432) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 120431 && codepoint < 120458) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 120457 && codepoint < 120486) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 120487 && codepoint < 120513) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 120513 && codepoint < 120539) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 120539 && codepoint < 120546) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 120545 && codepoint < 120571) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 120571 && codepoint < 120597) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 120597 && codepoint < 120604) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 120603 && codepoint < 120629) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 120629 && codepoint < 120655) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 120655 && codepoint < 120662) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 120661 && codepoint < 120687) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 120687 && codepoint < 120713) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 120713 && codepoint < 120720) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 120719 && codepoint < 120745) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 120745 && codepoint < 120771) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 120771 && codepoint < 120778) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 120781 && codepoint < 120832) return SFCE_UNICODE_CATEGORY_ND;
    if (codepoint > 120831 && codepoint < 121344) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 121343 && codepoint < 121399) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 121398 && codepoint < 121403) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 121402 && codepoint < 121453) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 121452 && codepoint < 121461) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 121461 && codepoint < 121476) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 121476 && codepoint < 121479) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 121478 && codepoint < 121484) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 121498 && codepoint < 121504) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 121504 && codepoint < 121520) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 122623 && codepoint < 122634) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 122634 && codepoint < 122655) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 122660 && codepoint < 122667) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 122879 && codepoint < 122887) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 122887 && codepoint < 122905) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 122906 && codepoint < 122914) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 122914 && codepoint < 122917) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 122917 && codepoint < 122923) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 122927 && codepoint < 122990) return SFCE_UNICODE_CATEGORY_LM;
    if (codepoint > 123135 && codepoint < 123181) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 123183 && codepoint < 123191) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 123190 && codepoint < 123198) return SFCE_UNICODE_CATEGORY_LM;
    if (codepoint > 123199 && codepoint < 123210) return SFCE_UNICODE_CATEGORY_ND;
    if (codepoint > 123535 && codepoint < 123566) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 123583 && codepoint < 123628) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 123627 && codepoint < 123632) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 123631 && codepoint < 123642) return SFCE_UNICODE_CATEGORY_ND;
    if (codepoint > 124111 && codepoint < 124139) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 124139 && codepoint < 124144) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 124143 && codepoint < 124154) return SFCE_UNICODE_CATEGORY_ND;
    if (codepoint > 124367 && codepoint < 124398) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 124397 && codepoint < 124400) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 124400 && codepoint < 124411) return SFCE_UNICODE_CATEGORY_ND;
    if (codepoint > 124895 && codepoint < 124903) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 124903 && codepoint < 124908) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 124908 && codepoint < 124911) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 124911 && codepoint < 124927) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 124927 && codepoint < 125125) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 125126 && codepoint < 125136) return SFCE_UNICODE_CATEGORY_NO;
    if (codepoint > 125135 && codepoint < 125143) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 125183 && codepoint < 125218) return SFCE_UNICODE_CATEGORY_LU;
    if (codepoint > 125217 && codepoint < 125252) return SFCE_UNICODE_CATEGORY_LL;
    if (codepoint > 125251 && codepoint < 125259) return SFCE_UNICODE_CATEGORY_MN;
    if (codepoint > 125263 && codepoint < 125274) return SFCE_UNICODE_CATEGORY_ND;
    if (codepoint > 125277 && codepoint < 125280) return SFCE_UNICODE_CATEGORY_PO;
    if (codepoint > 126064 && codepoint < 126124) return SFCE_UNICODE_CATEGORY_NO;
    if (codepoint > 126124 && codepoint < 126128) return SFCE_UNICODE_CATEGORY_NO;
    if (codepoint > 126128 && codepoint < 126133) return SFCE_UNICODE_CATEGORY_NO;
    if (codepoint > 126208 && codepoint < 126254) return SFCE_UNICODE_CATEGORY_NO;
    if (codepoint > 126254 && codepoint < 126270) return SFCE_UNICODE_CATEGORY_NO;
    if (codepoint > 126463 && codepoint < 126468) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 126468 && codepoint < 126496) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 126496 && codepoint < 126499) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 126504 && codepoint < 126515) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 126515 && codepoint < 126520) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 126540 && codepoint < 126544) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 126544 && codepoint < 126547) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 126560 && codepoint < 126563) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 126566 && codepoint < 126571) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 126571 && codepoint < 126579) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 126579 && codepoint < 126584) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 126584 && codepoint < 126589) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 126591 && codepoint < 126602) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 126602 && codepoint < 126620) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 126624 && codepoint < 126628) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 126628 && codepoint < 126634) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 126634 && codepoint < 126652) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 126703 && codepoint < 126706) return SFCE_UNICODE_CATEGORY_SM;
    if (codepoint > 126975 && codepoint < 127020) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 127023 && codepoint < 127124) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 127135 && codepoint < 127151) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 127152 && codepoint < 127168) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 127168 && codepoint < 127184) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 127184 && codepoint < 127222) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 127231 && codepoint < 127245) return SFCE_UNICODE_CATEGORY_NO;
    if (codepoint > 127244 && codepoint < 127406) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 127461 && codepoint < 127491) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 127503 && codepoint < 127548) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 127551 && codepoint < 127561) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 127567 && codepoint < 127570) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 127583 && codepoint < 127590) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 127743 && codepoint < 127995) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 127994 && codepoint < 128000) return SFCE_UNICODE_CATEGORY_SK;
    if (codepoint > 127999 && codepoint < 128728) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 128731 && codepoint < 128749) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 128751 && codepoint < 128765) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 128767 && codepoint < 128887) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 128890 && codepoint < 128986) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 128991 && codepoint < 129004) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 129023 && codepoint < 129036) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 129039 && codepoint < 129096) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 129103 && codepoint < 129114) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 129119 && codepoint < 129160) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 129167 && codepoint < 129198) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 129199 && codepoint < 129212) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 129215 && codepoint < 129218) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 129279 && codepoint < 129620) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 129631 && codepoint < 129646) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 129647 && codepoint < 129661) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 129663 && codepoint < 129674) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 129678 && codepoint < 129735) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 129741 && codepoint < 129757) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 129758 && codepoint < 129770) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 129775 && codepoint < 129785) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 129791 && codepoint < 129939) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 129939 && codepoint < 130032) return SFCE_UNICODE_CATEGORY_SO;
    if (codepoint > 130031 && codepoint < 130042) return SFCE_UNICODE_CATEGORY_ND;
    if (codepoint > 194559 && codepoint < 195102) return SFCE_UNICODE_CATEGORY_LO;
    if (codepoint > 917535 && codepoint < 917632) return SFCE_UNICODE_CATEGORY_CF;
    if (codepoint > 917759 && codepoint < 918000) return SFCE_UNICODE_CATEGORY_MN;
    return SFCE_UNICODE_CATEGORY_CN;
}

int32_t sfce_codepoint_to_upper(int32_t codepoint)
{
    switch (codepoint) {
    case 181: return 924;
    case 255: return 376;
    case 257: return 256;
    case 259: return 258;
    case 261: return 260;
    case 263: return 262;
    case 265: return 264;
    case 267: return 266;
    case 269: return 268;
    case 271: return 270;
    case 273: return 272;
    case 275: return 274;
    case 277: return 276;
    case 279: return 278;
    case 281: return 280;
    case 283: return 282;
    case 285: return 284;
    case 287: return 286;
    case 289: return 288;
    case 291: return 290;
    case 293: return 292;
    case 295: return 294;
    case 297: return 296;
    case 299: return 298;
    case 301: return 300;
    case 303: return 302;
    case 305: return 73;
    case 307: return 306;
    case 309: return 308;
    case 311: return 310;
    case 314: return 313;
    case 316: return 315;
    case 318: return 317;
    case 320: return 319;
    case 322: return 321;
    case 324: return 323;
    case 326: return 325;
    case 328: return 327;
    case 331: return 330;
    case 333: return 332;
    case 335: return 334;
    case 337: return 336;
    case 339: return 338;
    case 341: return 340;
    case 343: return 342;
    case 345: return 344;
    case 347: return 346;
    case 349: return 348;
    case 351: return 350;
    case 353: return 352;
    case 355: return 354;
    case 357: return 356;
    case 359: return 358;
    case 361: return 360;
    case 363: return 362;
    case 365: return 364;
    case 367: return 366;
    case 369: return 368;
    case 371: return 370;
    case 373: return 372;
    case 375: return 374;
    case 378: return 377;
    case 380: return 379;
    case 382: return 381;
    case 383: return 83;
    case 384: return 579;
    case 387: return 386;
    case 389: return 388;
    case 392: return 391;
    case 396: return 395;
    case 402: return 401;
    case 405: return 502;
    case 409: return 408;
    case 410: return 573;
    case 411: return 42972;
    case 414: return 544;
    case 417: return 416;
    case 419: return 418;
    case 421: return 420;
    case 424: return 423;
    case 429: return 428;
    case 432: return 431;
    case 436: return 435;
    case 438: return 437;
    case 441: return 440;
    case 445: return 444;
    case 447: return 503;
    case 453: case 454: 
        return 452;
    case 456: case 457: 
        return 455;
    case 459: case 460: 
        return 458;
    case 462: return 461;
    case 464: return 463;
    case 466: return 465;
    case 468: return 467;
    case 470: return 469;
    case 472: return 471;
    case 474: return 473;
    case 476: return 475;
    case 477: return 398;
    case 479: return 478;
    case 481: return 480;
    case 483: return 482;
    case 485: return 484;
    case 487: return 486;
    case 489: return 488;
    case 491: return 490;
    case 493: return 492;
    case 495: return 494;
    case 498: case 499: 
        return 497;
    case 501: return 500;
    case 505: return 504;
    case 507: return 506;
    case 509: return 508;
    case 511: return 510;
    case 513: return 512;
    case 515: return 514;
    case 517: return 516;
    case 519: return 518;
    case 521: return 520;
    case 523: return 522;
    case 525: return 524;
    case 527: return 526;
    case 529: return 528;
    case 531: return 530;
    case 533: return 532;
    case 535: return 534;
    case 537: return 536;
    case 539: return 538;
    case 541: return 540;
    case 543: return 542;
    case 547: return 546;
    case 549: return 548;
    case 551: return 550;
    case 553: return 552;
    case 555: return 554;
    case 557: return 556;
    case 559: return 558;
    case 561: return 560;
    case 563: return 562;
    case 572: return 571;
    case 578: return 577;
    case 583: return 582;
    case 585: return 584;
    case 587: return 586;
    case 589: return 588;
    case 591: return 590;
    case 592: return 11375;
    case 593: return 11373;
    case 594: return 11376;
    case 595: return 385;
    case 596: return 390;
    case 601: return 399;
    case 603: return 400;
    case 604: return 42923;
    case 608: return 403;
    case 609: return 42924;
    case 611: return 404;
    case 612: return 42955;
    case 613: return 42893;
    case 614: return 42922;
    case 616: return 407;
    case 617: return 406;
    case 618: return 42926;
    case 619: return 11362;
    case 620: return 42925;
    case 623: return 412;
    case 625: return 11374;
    case 626: return 413;
    case 629: return 415;
    case 637: return 11364;
    case 640: return 422;
    case 642: return 42949;
    case 643: return 425;
    case 647: return 42929;
    case 648: return 430;
    case 649: return 580;
    case 652: return 581;
    case 658: return 439;
    case 669: return 42930;
    case 670: return 42928;
    case 837:  case 8126: 
        return 921;
    case 881: return 880;
    case 883: return 882;
    case 887: return 886;
    case 940: return 902;
    case 962: return 931;
    case 972: return 908;
    case 976: return 914;
    case 977: return 920;
    case 981: return 934;
    case 982: return 928;
    case 983: return 975;
    case 985: return 984;
    case 987: return 986;
    case 989: return 988;
    case 991: return 990;
    case 993: return 992;
    case 995: return 994;
    case 997: return 996;
    case 999: return 998;
    case 1001: return 1000;
    case 1003: return 1002;
    case 1005: return 1004;
    case 1007: return 1006;
    case 1008: return 922;
    case 1009: return 929;
    case 1010: return 1017;
    case 1011: return 895;
    case 1013: return 917;
    case 1016: return 1015;
    case 1019: return 1018;
    case 1121: return 1120;
    case 1123: case 7303: 
        return 1122;
    case 1125: return 1124;
    case 1127: return 1126;
    case 1129: return 1128;
    case 1131: return 1130;
    case 1133: return 1132;
    case 1135: return 1134;
    case 1137: return 1136;
    case 1139: return 1138;
    case 1141: return 1140;
    case 1143: return 1142;
    case 1145: return 1144;
    case 1147: return 1146;
    case 1149: return 1148;
    case 1151: return 1150;
    case 1153: return 1152;
    case 1163: return 1162;
    case 1165: return 1164;
    case 1167: return 1166;
    case 1169: return 1168;
    case 1171: return 1170;
    case 1173: return 1172;
    case 1175: return 1174;
    case 1177: return 1176;
    case 1179: return 1178;
    case 1181: return 1180;
    case 1183: return 1182;
    case 1185: return 1184;
    case 1187: return 1186;
    case 1189: return 1188;
    case 1191: return 1190;
    case 1193: return 1192;
    case 1195: return 1194;
    case 1197: return 1196;
    case 1199: return 1198;
    case 1201: return 1200;
    case 1203: return 1202;
    case 1205: return 1204;
    case 1207: return 1206;
    case 1209: return 1208;
    case 1211: return 1210;
    case 1213: return 1212;
    case 1215: return 1214;
    case 1218: return 1217;
    case 1220: return 1219;
    case 1222: return 1221;
    case 1224: return 1223;
    case 1226: return 1225;
    case 1228: return 1227;
    case 1230: return 1229;
    case 1231: return 1216;
    case 1233: return 1232;
    case 1235: return 1234;
    case 1237: return 1236;
    case 1239: return 1238;
    case 1241: return 1240;
    case 1243: return 1242;
    case 1245: return 1244;
    case 1247: return 1246;
    case 1249: return 1248;
    case 1251: return 1250;
    case 1253: return 1252;
    case 1255: return 1254;
    case 1257: return 1256;
    case 1259: return 1258;
    case 1261: return 1260;
    case 1263: return 1262;
    case 1265: return 1264;
    case 1267: return 1266;
    case 1269: return 1268;
    case 1271: return 1270;
    case 1273: return 1272;
    case 1275: return 1274;
    case 1277: return 1276;
    case 1279: return 1278;
    case 1281: return 1280;
    case 1283: return 1282;
    case 1285: return 1284;
    case 1287: return 1286;
    case 1289: return 1288;
    case 1291: return 1290;
    case 1293: return 1292;
    case 1295: return 1294;
    case 1297: return 1296;
    case 1299: return 1298;
    case 1301: return 1300;
    case 1303: return 1302;
    case 1305: return 1304;
    case 1307: return 1306;
    case 1309: return 1308;
    case 1311: return 1310;
    case 1313: return 1312;
    case 1315: return 1314;
    case 1317: return 1316;
    case 1319: return 1318;
    case 1321: return 1320;
    case 1323: return 1322;
    case 1325: return 1324;
    case 1327: return 1326;
    case 7296: return 1042;
    case 7297: return 1044;
    case 7298: return 1054;
    case 7301: return 1058;
    case 7302: return 1066;
    case 7304:  case 42571: 
        return 42570;
    case 7306: return 7305;
    case 7545: return 42877;
    case 7549: return 11363;
    case 7566: return 42950;
    case 7681: return 7680;
    case 7683: return 7682;
    case 7685: return 7684;
    case 7687: return 7686;
    case 7689: return 7688;
    case 7691: return 7690;
    case 7693: return 7692;
    case 7695: return 7694;
    case 7697: return 7696;
    case 7699: return 7698;
    case 7701: return 7700;
    case 7703: return 7702;
    case 7705: return 7704;
    case 7707: return 7706;
    case 7709: return 7708;
    case 7711: return 7710;
    case 7713: return 7712;
    case 7715: return 7714;
    case 7717: return 7716;
    case 7719: return 7718;
    case 7721: return 7720;
    case 7723: return 7722;
    case 7725: return 7724;
    case 7727: return 7726;
    case 7729: return 7728;
    case 7731: return 7730;
    case 7733: return 7732;
    case 7735: return 7734;
    case 7737: return 7736;
    case 7739: return 7738;
    case 7741: return 7740;
    case 7743: return 7742;
    case 7745: return 7744;
    case 7747: return 7746;
    case 7749: return 7748;
    case 7751: return 7750;
    case 7753: return 7752;
    case 7755: return 7754;
    case 7757: return 7756;
    case 7759: return 7758;
    case 7761: return 7760;
    case 7763: return 7762;
    case 7765: return 7764;
    case 7767: return 7766;
    case 7769: return 7768;
    case 7771: return 7770;
    case 7773: return 7772;
    case 7775: return 7774;
    case 7777: case 7835: 
        return 7776;
    case 7779: return 7778;
    case 7781: return 7780;
    case 7783: return 7782;
    case 7785: return 7784;
    case 7787: return 7786;
    case 7789: return 7788;
    case 7791: return 7790;
    case 7793: return 7792;
    case 7795: return 7794;
    case 7797: return 7796;
    case 7799: return 7798;
    case 7801: return 7800;
    case 7803: return 7802;
    case 7805: return 7804;
    case 7807: return 7806;
    case 7809: return 7808;
    case 7811: return 7810;
    case 7813: return 7812;
    case 7815: return 7814;
    case 7817: return 7816;
    case 7819: return 7818;
    case 7821: return 7820;
    case 7823: return 7822;
    case 7825: return 7824;
    case 7827: return 7826;
    case 7829: return 7828;
    case 7841: return 7840;
    case 7843: return 7842;
    case 7845: return 7844;
    case 7847: return 7846;
    case 7849: return 7848;
    case 7851: return 7850;
    case 7853: return 7852;
    case 7855: return 7854;
    case 7857: return 7856;
    case 7859: return 7858;
    case 7861: return 7860;
    case 7863: return 7862;
    case 7865: return 7864;
    case 7867: return 7866;
    case 7869: return 7868;
    case 7871: return 7870;
    case 7873: return 7872;
    case 7875: return 7874;
    case 7877: return 7876;
    case 7879: return 7878;
    case 7881: return 7880;
    case 7883: return 7882;
    case 7885: return 7884;
    case 7887: return 7886;
    case 7889: return 7888;
    case 7891: return 7890;
    case 7893: return 7892;
    case 7895: return 7894;
    case 7897: return 7896;
    case 7899: return 7898;
    case 7901: return 7900;
    case 7903: return 7902;
    case 7905: return 7904;
    case 7907: return 7906;
    case 7909: return 7908;
    case 7911: return 7910;
    case 7913: return 7912;
    case 7915: return 7914;
    case 7917: return 7916;
    case 7919: return 7918;
    case 7921: return 7920;
    case 7923: return 7922;
    case 7925: return 7924;
    case 7927: return 7926;
    case 7929: return 7928;
    case 7931: return 7930;
    case 7933: return 7932;
    case 7935: return 7934;
    case 8017: return 8025;
    case 8019: return 8027;
    case 8021: return 8029;
    case 8023: return 8031;
    case 8115: return 8124;
    case 8131: return 8140;
    case 8165: return 8172;
    case 8179: return 8188;
    case 8526: return 8498;
    case 8580: return 8579;
    case 11361: return 11360;
    case 11365: return 570;
    case 11366: return 574;
    case 11368: return 11367;
    case 11370: return 11369;
    case 11372: return 11371;
    case 11379: return 11378;
    case 11382: return 11381;
    case 11393: return 11392;
    case 11395: return 11394;
    case 11397: return 11396;
    case 11399: return 11398;
    case 11401: return 11400;
    case 11403: return 11402;
    case 11405: return 11404;
    case 11407: return 11406;
    case 11409: return 11408;
    case 11411: return 11410;
    case 11413: return 11412;
    case 11415: return 11414;
    case 11417: return 11416;
    case 11419: return 11418;
    case 11421: return 11420;
    case 11423: return 11422;
    case 11425: return 11424;
    case 11427: return 11426;
    case 11429: return 11428;
    case 11431: return 11430;
    case 11433: return 11432;
    case 11435: return 11434;
    case 11437: return 11436;
    case 11439: return 11438;
    case 11441: return 11440;
    case 11443: return 11442;
    case 11445: return 11444;
    case 11447: return 11446;
    case 11449: return 11448;
    case 11451: return 11450;
    case 11453: return 11452;
    case 11455: return 11454;
    case 11457: return 11456;
    case 11459: return 11458;
    case 11461: return 11460;
    case 11463: return 11462;
    case 11465: return 11464;
    case 11467: return 11466;
    case 11469: return 11468;
    case 11471: return 11470;
    case 11473: return 11472;
    case 11475: return 11474;
    case 11477: return 11476;
    case 11479: return 11478;
    case 11481: return 11480;
    case 11483: return 11482;
    case 11485: return 11484;
    case 11487: return 11486;
    case 11489: return 11488;
    case 11491: return 11490;
    case 11500: return 11499;
    case 11502: return 11501;
    case 11507: return 11506;
    case 11559: return 4295;
    case 11565: return 4301;
    case 42561: return 42560;
    case 42563: return 42562;
    case 42565: return 42564;
    case 42567: return 42566;
    case 42569: return 42568;
    case 42573: return 42572;
    case 42575: return 42574;
    case 42577: return 42576;
    case 42579: return 42578;
    case 42581: return 42580;
    case 42583: return 42582;
    case 42585: return 42584;
    case 42587: return 42586;
    case 42589: return 42588;
    case 42591: return 42590;
    case 42593: return 42592;
    case 42595: return 42594;
    case 42597: return 42596;
    case 42599: return 42598;
    case 42601: return 42600;
    case 42603: return 42602;
    case 42605: return 42604;
    case 42625: return 42624;
    case 42627: return 42626;
    case 42629: return 42628;
    case 42631: return 42630;
    case 42633: return 42632;
    case 42635: return 42634;
    case 42637: return 42636;
    case 42639: return 42638;
    case 42641: return 42640;
    case 42643: return 42642;
    case 42645: return 42644;
    case 42647: return 42646;
    case 42649: return 42648;
    case 42651: return 42650;
    case 42787: return 42786;
    case 42789: return 42788;
    case 42791: return 42790;
    case 42793: return 42792;
    case 42795: return 42794;
    case 42797: return 42796;
    case 42799: return 42798;
    case 42803: return 42802;
    case 42805: return 42804;
    case 42807: return 42806;
    case 42809: return 42808;
    case 42811: return 42810;
    case 42813: return 42812;
    case 42815: return 42814;
    case 42817: return 42816;
    case 42819: return 42818;
    case 42821: return 42820;
    case 42823: return 42822;
    case 42825: return 42824;
    case 42827: return 42826;
    case 42829: return 42828;
    case 42831: return 42830;
    case 42833: return 42832;
    case 42835: return 42834;
    case 42837: return 42836;
    case 42839: return 42838;
    case 42841: return 42840;
    case 42843: return 42842;
    case 42845: return 42844;
    case 42847: return 42846;
    case 42849: return 42848;
    case 42851: return 42850;
    case 42853: return 42852;
    case 42855: return 42854;
    case 42857: return 42856;
    case 42859: return 42858;
    case 42861: return 42860;
    case 42863: return 42862;
    case 42874: return 42873;
    case 42876: return 42875;
    case 42879: return 42878;
    case 42881: return 42880;
    case 42883: return 42882;
    case 42885: return 42884;
    case 42887: return 42886;
    case 42892: return 42891;
    case 42897: return 42896;
    case 42899: return 42898;
    case 42900: return 42948;
    case 42903: return 42902;
    case 42905: return 42904;
    case 42907: return 42906;
    case 42909: return 42908;
    case 42911: return 42910;
    case 42913: return 42912;
    case 42915: return 42914;
    case 42917: return 42916;
    case 42919: return 42918;
    case 42921: return 42920;
    case 42933: return 42932;
    case 42935: return 42934;
    case 42937: return 42936;
    case 42939: return 42938;
    case 42941: return 42940;
    case 42943: return 42942;
    case 42945: return 42944;
    case 42947: return 42946;
    case 42952: return 42951;
    case 42954: return 42953;
    case 42957: return 42956;
    case 42961: return 42960;
    case 42967: return 42966;
    case 42969: return 42968;
    case 42971: return 42970;
    case 42998: return 42997;
    case 43859: return 42931;
    }

    if (codepoint > 96 && codepoint < 123) return codepoint - 32;
    if (codepoint > 223 && codepoint < 247) return codepoint - 32;
    if (codepoint > 247 && codepoint < 255) return codepoint - 32;
    if (codepoint > 574 && codepoint < 577) return codepoint + 10815;
    if (codepoint > 597 && codepoint < 600) return codepoint - 205;
    if (codepoint > 649 && codepoint < 652) return codepoint - 217;
    if (codepoint > 890 && codepoint < 894) return codepoint + 130;
    if (codepoint > 940 && codepoint < 944) return codepoint - 37;
    if (codepoint > 944 && codepoint < 962) return codepoint - 32;
    if (codepoint > 962 && codepoint < 972) return codepoint - 32;
    if (codepoint > 972 && codepoint < 975) return codepoint - 63;
    if (codepoint > 1071 && codepoint < 1104) return codepoint - 32;
    if (codepoint > 1103 && codepoint < 1120) return codepoint - 80;
    if (codepoint > 1376 && codepoint < 1415) return codepoint - 48;
    if (codepoint > 4303 && codepoint < 4347) return codepoint + 3008;
    if (codepoint > 4348 && codepoint < 4352) return codepoint + 3008;
    if (codepoint > 5111 && codepoint < 5118) return codepoint - 8;
    if (codepoint > 7298 && codepoint < 7301) return codepoint - 6242;
    if (codepoint > 7935 && codepoint < 7944) return codepoint + 8;
    if (codepoint > 7951 && codepoint < 7958) return codepoint + 8;
    if (codepoint > 7967 && codepoint < 7976) return codepoint + 8;
    if (codepoint > 7983 && codepoint < 7992) return codepoint + 8;
    if (codepoint > 7999 && codepoint < 8006) return codepoint + 8;
    if (codepoint > 8031 && codepoint < 8040) return codepoint + 8;
    if (codepoint > 8047 && codepoint < 8050) return codepoint + 74;
    if (codepoint > 8049 && codepoint < 8054) return codepoint + 86;
    if (codepoint > 8053 && codepoint < 8056) return codepoint + 100;
    if (codepoint > 8055 && codepoint < 8058) return codepoint + 128;
    if (codepoint > 8057 && codepoint < 8060) return codepoint + 112;
    if (codepoint > 8059 && codepoint < 8062) return codepoint + 126;
    if (codepoint > 8063 && codepoint < 8072) return codepoint + 8;
    if (codepoint > 8079 && codepoint < 8088) return codepoint + 8;
    if (codepoint > 8095 && codepoint < 8104) return codepoint + 8;
    if (codepoint > 8111 && codepoint < 8114) return codepoint + 8;
    if (codepoint > 8143 && codepoint < 8146) return codepoint + 8;
    if (codepoint > 8159 && codepoint < 8162) return codepoint + 8;
    if (codepoint > 8559 && codepoint < 8576) return codepoint - 16;
    if (codepoint > 9423 && codepoint < 9450) return codepoint - 26;
    if (codepoint > 11311 && codepoint < 11360) return codepoint - 48;
    if (codepoint > 11519 && codepoint < 11558) return codepoint - 7264;
    if (codepoint > 43887 && codepoint < 43968) return codepoint - 38864;
    if (codepoint > 65344 && codepoint < 65371) return codepoint - 32;
    if (codepoint > 66599 && codepoint < 66640) return codepoint - 40;
    if (codepoint > 66775 && codepoint < 66812) return codepoint - 40;
    if (codepoint > 66966 && codepoint < 66978) return codepoint - 39;
    if (codepoint > 66978 && codepoint < 66994) return codepoint - 39;
    if (codepoint > 66994 && codepoint < 67002) return codepoint - 39;
    if (codepoint > 67002 && codepoint < 67005) return codepoint - 39;
    if (codepoint > 68799 && codepoint < 68851) return codepoint - 64;
    if (codepoint > 68975 && codepoint < 68998) return codepoint - 32;
    if (codepoint > 71871 && codepoint < 71904) return codepoint - 32;
    if (codepoint > 93791 && codepoint < 93824) return codepoint - 32;
    if (codepoint > 125217 && codepoint < 125252) return codepoint - 34;
    return codepoint;
}

int32_t sfce_codepoint_to_lower(int32_t codepoint)
{
    switch (codepoint) {
    case 256: return 257;
    case 258: return 259;
    case 260: return 261;
    case 262: return 263;
    case 264: return 265;
    case 266: return 267;
    case 268: return 269;
    case 270: return 271;
    case 272: return 273;
    case 274: return 275;
    case 276: return 277;
    case 278: return 279;
    case 280: return 281;
    case 282: return 283;
    case 284: return 285;
    case 286: return 287;
    case 288: return 289;
    case 290: return 291;
    case 292: return 293;
    case 294: return 295;
    case 296: return 297;
    case 298: return 299;
    case 300: return 301;
    case 302: return 303;
    case 304: return 105;
    case 306: return 307;
    case 308: return 309;
    case 310: return 311;
    case 313: return 314;
    case 315: return 316;
    case 317: return 318;
    case 319: return 320;
    case 321: return 322;
    case 323: return 324;
    case 325: return 326;
    case 327: return 328;
    case 330: return 331;
    case 332: return 333;
    case 334: return 335;
    case 336: return 337;
    case 338: return 339;
    case 340: return 341;
    case 342: return 343;
    case 344: return 345;
    case 346: return 347;
    case 348: return 349;
    case 350: return 351;
    case 352: return 353;
    case 354: return 355;
    case 356: return 357;
    case 358: return 359;
    case 360: return 361;
    case 362: return 363;
    case 364: return 365;
    case 366: return 367;
    case 368: return 369;
    case 370: return 371;
    case 372: return 373;
    case 374: return 375;
    case 376: return 255;
    case 377: return 378;
    case 379: return 380;
    case 381: return 382;
    case 385: return 595;
    case 386: return 387;
    case 388: return 389;
    case 390: return 596;
    case 391: return 392;
    case 395: return 396;
    case 398: return 477;
    case 399: return 601;
    case 400: return 603;
    case 401: return 402;
    case 403: return 608;
    case 404: return 611;
    case 406: return 617;
    case 407: return 616;
    case 408: return 409;
    case 412: return 623;
    case 413: return 626;
    case 415: return 629;
    case 416: return 417;
    case 418: return 419;
    case 420: return 421;
    case 422: return 640;
    case 423: return 424;
    case 425: return 643;
    case 428: return 429;
    case 430: return 648;
    case 431: return 432;
    case 435: return 436;
    case 437: return 438;
    case 439: return 658;
    case 440: return 441;
    case 444: return 445;
    case 452: case 453: 
        return 454;
    case 455: case 456: 
        return 457;
    case 458: case 459: 
        return 460;
    case 461: return 462;
    case 463: return 464;
    case 465: return 466;
    case 467: return 468;
    case 469: return 470;
    case 471: return 472;
    case 473: return 474;
    case 475: return 476;
    case 478: return 479;
    case 480: return 481;
    case 482: return 483;
    case 484: return 485;
    case 486: return 487;
    case 488: return 489;
    case 490: return 491;
    case 492: return 493;
    case 494: return 495;
    case 497: case 498: 
        return 499;
    case 500: return 501;
    case 502: return 405;
    case 503: return 447;
    case 504: return 505;
    case 506: return 507;
    case 508: return 509;
    case 510: return 511;
    case 512: return 513;
    case 514: return 515;
    case 516: return 517;
    case 518: return 519;
    case 520: return 521;
    case 522: return 523;
    case 524: return 525;
    case 526: return 527;
    case 528: return 529;
    case 530: return 531;
    case 532: return 533;
    case 534: return 535;
    case 536: return 537;
    case 538: return 539;
    case 540: return 541;
    case 542: return 543;
    case 544: return 414;
    case 546: return 547;
    case 548: return 549;
    case 550: return 551;
    case 552: return 553;
    case 554: return 555;
    case 556: return 557;
    case 558: return 559;
    case 560: return 561;
    case 562: return 563;
    case 570: return 11365;
    case 571: return 572;
    case 573: return 410;
    case 574: return 11366;
    case 577: return 578;
    case 579: return 384;
    case 580: return 649;
    case 581: return 652;
    case 582: return 583;
    case 584: return 585;
    case 586: return 587;
    case 588: return 589;
    case 590: return 591;
    case 880: return 881;
    case 882: return 883;
    case 886: return 887;
    case 895: return 1011;
    case 902: return 940;
    case 908: return 972;
    case 975: return 983;
    case 984: return 985;
    case 986: return 987;
    case 988: return 989;
    case 990: return 991;
    case 992: return 993;
    case 994: return 995;
    case 996: return 997;
    case 998: return 999;
    case 1000: return 1001;
    case 1002: return 1003;
    case 1004: return 1005;
    case 1006: return 1007;
    case 1012: return 952;
    case 1015: return 1016;
    case 1017: return 1010;
    case 1018: return 1019;
    case 1120: return 1121;
    case 1122: return 1123;
    case 1124: return 1125;
    case 1126: return 1127;
    case 1128: return 1129;
    case 1130: return 1131;
    case 1132: return 1133;
    case 1134: return 1135;
    case 1136: return 1137;
    case 1138: return 1139;
    case 1140: return 1141;
    case 1142: return 1143;
    case 1144: return 1145;
    case 1146: return 1147;
    case 1148: return 1149;
    case 1150: return 1151;
    case 1152: return 1153;
    case 1162: return 1163;
    case 1164: return 1165;
    case 1166: return 1167;
    case 1168: return 1169;
    case 1170: return 1171;
    case 1172: return 1173;
    case 1174: return 1175;
    case 1176: return 1177;
    case 1178: return 1179;
    case 1180: return 1181;
    case 1182: return 1183;
    case 1184: return 1185;
    case 1186: return 1187;
    case 1188: return 1189;
    case 1190: return 1191;
    case 1192: return 1193;
    case 1194: return 1195;
    case 1196: return 1197;
    case 1198: return 1199;
    case 1200: return 1201;
    case 1202: return 1203;
    case 1204: return 1205;
    case 1206: return 1207;
    case 1208: return 1209;
    case 1210: return 1211;
    case 1212: return 1213;
    case 1214: return 1215;
    case 1216: return 1231;
    case 1217: return 1218;
    case 1219: return 1220;
    case 1221: return 1222;
    case 1223: return 1224;
    case 1225: return 1226;
    case 1227: return 1228;
    case 1229: return 1230;
    case 1232: return 1233;
    case 1234: return 1235;
    case 1236: return 1237;
    case 1238: return 1239;
    case 1240: return 1241;
    case 1242: return 1243;
    case 1244: return 1245;
    case 1246: return 1247;
    case 1248: return 1249;
    case 1250: return 1251;
    case 1252: return 1253;
    case 1254: return 1255;
    case 1256: return 1257;
    case 1258: return 1259;
    case 1260: return 1261;
    case 1262: return 1263;
    case 1264: return 1265;
    case 1266: return 1267;
    case 1268: return 1269;
    case 1270: return 1271;
    case 1272: return 1273;
    case 1274: return 1275;
    case 1276: return 1277;
    case 1278: return 1279;
    case 1280: return 1281;
    case 1282: return 1283;
    case 1284: return 1285;
    case 1286: return 1287;
    case 1288: return 1289;
    case 1290: return 1291;
    case 1292: return 1293;
    case 1294: return 1295;
    case 1296: return 1297;
    case 1298: return 1299;
    case 1300: return 1301;
    case 1302: return 1303;
    case 1304: return 1305;
    case 1306: return 1307;
    case 1308: return 1309;
    case 1310: return 1311;
    case 1312: return 1313;
    case 1314: return 1315;
    case 1316: return 1317;
    case 1318: return 1319;
    case 1320: return 1321;
    case 1322: return 1323;
    case 1324: return 1325;
    case 1326: return 1327;
    case 4295: return 11559;
    case 4301: return 11565;
    case 7305: return 7306;
    case 7680: return 7681;
    case 7682: return 7683;
    case 7684: return 7685;
    case 7686: return 7687;
    case 7688: return 7689;
    case 7690: return 7691;
    case 7692: return 7693;
    case 7694: return 7695;
    case 7696: return 7697;
    case 7698: return 7699;
    case 7700: return 7701;
    case 7702: return 7703;
    case 7704: return 7705;
    case 7706: return 7707;
    case 7708: return 7709;
    case 7710: return 7711;
    case 7712: return 7713;
    case 7714: return 7715;
    case 7716: return 7717;
    case 7718: return 7719;
    case 7720: return 7721;
    case 7722: return 7723;
    case 7724: return 7725;
    case 7726: return 7727;
    case 7728: return 7729;
    case 7730: return 7731;
    case 7732: return 7733;
    case 7734: return 7735;
    case 7736: return 7737;
    case 7738: return 7739;
    case 7740: return 7741;
    case 7742: return 7743;
    case 7744: return 7745;
    case 7746: return 7747;
    case 7748: return 7749;
    case 7750: return 7751;
    case 7752: return 7753;
    case 7754: return 7755;
    case 7756: return 7757;
    case 7758: return 7759;
    case 7760: return 7761;
    case 7762: return 7763;
    case 7764: return 7765;
    case 7766: return 7767;
    case 7768: return 7769;
    case 7770: return 7771;
    case 7772: return 7773;
    case 7774: return 7775;
    case 7776: return 7777;
    case 7778: return 7779;
    case 7780: return 7781;
    case 7782: return 7783;
    case 7784: return 7785;
    case 7786: return 7787;
    case 7788: return 7789;
    case 7790: return 7791;
    case 7792: return 7793;
    case 7794: return 7795;
    case 7796: return 7797;
    case 7798: return 7799;
    case 7800: return 7801;
    case 7802: return 7803;
    case 7804: return 7805;
    case 7806: return 7807;
    case 7808: return 7809;
    case 7810: return 7811;
    case 7812: return 7813;
    case 7814: return 7815;
    case 7816: return 7817;
    case 7818: return 7819;
    case 7820: return 7821;
    case 7822: return 7823;
    case 7824: return 7825;
    case 7826: return 7827;
    case 7828: return 7829;
    case 7838: return 223;
    case 7840: return 7841;
    case 7842: return 7843;
    case 7844: return 7845;
    case 7846: return 7847;
    case 7848: return 7849;
    case 7850: return 7851;
    case 7852: return 7853;
    case 7854: return 7855;
    case 7856: return 7857;
    case 7858: return 7859;
    case 7860: return 7861;
    case 7862: return 7863;
    case 7864: return 7865;
    case 7866: return 7867;
    case 7868: return 7869;
    case 7870: return 7871;
    case 7872: return 7873;
    case 7874: return 7875;
    case 7876: return 7877;
    case 7878: return 7879;
    case 7880: return 7881;
    case 7882: return 7883;
    case 7884: return 7885;
    case 7886: return 7887;
    case 7888: return 7889;
    case 7890: return 7891;
    case 7892: return 7893;
    case 7894: return 7895;
    case 7896: return 7897;
    case 7898: return 7899;
    case 7900: return 7901;
    case 7902: return 7903;
    case 7904: return 7905;
    case 7906: return 7907;
    case 7908: return 7909;
    case 7910: return 7911;
    case 7912: return 7913;
    case 7914: return 7915;
    case 7916: return 7917;
    case 7918: return 7919;
    case 7920: return 7921;
    case 7922: return 7923;
    case 7924: return 7925;
    case 7926: return 7927;
    case 7928: return 7929;
    case 7930: return 7931;
    case 7932: return 7933;
    case 7934: return 7935;
    case 8025: return 8017;
    case 8027: return 8019;
    case 8029: return 8021;
    case 8031: return 8023;
    case 8124: return 8115;
    case 8140: return 8131;
    case 8172: return 8165;
    case 8188: return 8179;
    case 8486: return 969;
    case 8490: return 107;
    case 8491: return 229;
    case 8498: return 8526;
    case 8579: return 8580;
    case 11360: return 11361;
    case 11362: return 619;
    case 11363: return 7549;
    case 11364: return 637;
    case 11367: return 11368;
    case 11369: return 11370;
    case 11371: return 11372;
    case 11373: return 593;
    case 11374: return 625;
    case 11375: return 592;
    case 11376: return 594;
    case 11378: return 11379;
    case 11381: return 11382;
    case 11392: return 11393;
    case 11394: return 11395;
    case 11396: return 11397;
    case 11398: return 11399;
    case 11400: return 11401;
    case 11402: return 11403;
    case 11404: return 11405;
    case 11406: return 11407;
    case 11408: return 11409;
    case 11410: return 11411;
    case 11412: return 11413;
    case 11414: return 11415;
    case 11416: return 11417;
    case 11418: return 11419;
    case 11420: return 11421;
    case 11422: return 11423;
    case 11424: return 11425;
    case 11426: return 11427;
    case 11428: return 11429;
    case 11430: return 11431;
    case 11432: return 11433;
    case 11434: return 11435;
    case 11436: return 11437;
    case 11438: return 11439;
    case 11440: return 11441;
    case 11442: return 11443;
    case 11444: return 11445;
    case 11446: return 11447;
    case 11448: return 11449;
    case 11450: return 11451;
    case 11452: return 11453;
    case 11454: return 11455;
    case 11456: return 11457;
    case 11458: return 11459;
    case 11460: return 11461;
    case 11462: return 11463;
    case 11464: return 11465;
    case 11466: return 11467;
    case 11468: return 11469;
    case 11470: return 11471;
    case 11472: return 11473;
    case 11474: return 11475;
    case 11476: return 11477;
    case 11478: return 11479;
    case 11480: return 11481;
    case 11482: return 11483;
    case 11484: return 11485;
    case 11486: return 11487;
    case 11488: return 11489;
    case 11490: return 11491;
    case 11499: return 11500;
    case 11501: return 11502;
    case 11506: return 11507;
    case 42560: return 42561;
    case 42562: return 42563;
    case 42564: return 42565;
    case 42566: return 42567;
    case 42568: return 42569;
    case 42570: return 42571;
    case 42572: return 42573;
    case 42574: return 42575;
    case 42576: return 42577;
    case 42578: return 42579;
    case 42580: return 42581;
    case 42582: return 42583;
    case 42584: return 42585;
    case 42586: return 42587;
    case 42588: return 42589;
    case 42590: return 42591;
    case 42592: return 42593;
    case 42594: return 42595;
    case 42596: return 42597;
    case 42598: return 42599;
    case 42600: return 42601;
    case 42602: return 42603;
    case 42604: return 42605;
    case 42624: return 42625;
    case 42626: return 42627;
    case 42628: return 42629;
    case 42630: return 42631;
    case 42632: return 42633;
    case 42634: return 42635;
    case 42636: return 42637;
    case 42638: return 42639;
    case 42640: return 42641;
    case 42642: return 42643;
    case 42644: return 42645;
    case 42646: return 42647;
    case 42648: return 42649;
    case 42650: return 42651;
    case 42786: return 42787;
    case 42788: return 42789;
    case 42790: return 42791;
    case 42792: return 42793;
    case 42794: return 42795;
    case 42796: return 42797;
    case 42798: return 42799;
    case 42802: return 42803;
    case 42804: return 42805;
    case 42806: return 42807;
    case 42808: return 42809;
    case 42810: return 42811;
    case 42812: return 42813;
    case 42814: return 42815;
    case 42816: return 42817;
    case 42818: return 42819;
    case 42820: return 42821;
    case 42822: return 42823;
    case 42824: return 42825;
    case 42826: return 42827;
    case 42828: return 42829;
    case 42830: return 42831;
    case 42832: return 42833;
    case 42834: return 42835;
    case 42836: return 42837;
    case 42838: return 42839;
    case 42840: return 42841;
    case 42842: return 42843;
    case 42844: return 42845;
    case 42846: return 42847;
    case 42848: return 42849;
    case 42850: return 42851;
    case 42852: return 42853;
    case 42854: return 42855;
    case 42856: return 42857;
    case 42858: return 42859;
    case 42860: return 42861;
    case 42862: return 42863;
    case 42873: return 42874;
    case 42875: return 42876;
    case 42877: return 7545;
    case 42878: return 42879;
    case 42880: return 42881;
    case 42882: return 42883;
    case 42884: return 42885;
    case 42886: return 42887;
    case 42891: return 42892;
    case 42893: return 613;
    case 42896: return 42897;
    case 42898: return 42899;
    case 42902: return 42903;
    case 42904: return 42905;
    case 42906: return 42907;
    case 42908: return 42909;
    case 42910: return 42911;
    case 42912: return 42913;
    case 42914: return 42915;
    case 42916: return 42917;
    case 42918: return 42919;
    case 42920: return 42921;
    case 42922: return 614;
    case 42923: return 604;
    case 42924: return 609;
    case 42925: return 620;
    case 42926: return 618;
    case 42928: return 670;
    case 42929: return 647;
    case 42930: return 669;
    case 42931: return 43859;
    case 42932: return 42933;
    case 42934: return 42935;
    case 42936: return 42937;
    case 42938: return 42939;
    case 42940: return 42941;
    case 42942: return 42943;
    case 42944: return 42945;
    case 42946: return 42947;
    case 42948: return 42900;
    case 42949: return 642;
    case 42950: return 7566;
    case 42951: return 42952;
    case 42953: return 42954;
    case 42955: return 612;
    case 42956: return 42957;
    case 42960: return 42961;
    case 42966: return 42967;
    case 42968: return 42969;
    case 42970: return 42971;
    case 42972: return 411;
    case 42997: return 42998;
    }

    if (codepoint > 64 && codepoint < 91) return codepoint + 32;
    if (codepoint > 191 && codepoint < 215) return codepoint + 32;
    if (codepoint > 215 && codepoint < 223) return codepoint + 32;
    if (codepoint > 392 && codepoint < 395) return codepoint + 205;
    if (codepoint > 432 && codepoint < 435) return codepoint + 217;
    if (codepoint > 903 && codepoint < 907) return codepoint + 37;
    if (codepoint > 909 && codepoint < 912) return codepoint + 63;
    if (codepoint > 912 && codepoint < 930) return codepoint + 32;
    if (codepoint > 930 && codepoint < 940) return codepoint + 32;
    if (codepoint > 1020 && codepoint < 1024) return codepoint - 130;
    if (codepoint > 1023 && codepoint < 1040) return codepoint + 80;
    if (codepoint > 1039 && codepoint < 1072) return codepoint + 32;
    if (codepoint > 1328 && codepoint < 1367) return codepoint + 48;
    if (codepoint > 4255 && codepoint < 4294) return codepoint + 7264;
    if (codepoint > 5023 && codepoint < 5104) return codepoint + 38864;
    if (codepoint > 5103 && codepoint < 5110) return codepoint + 8;
    if (codepoint > 7311 && codepoint < 7355) return codepoint - 3008;
    if (codepoint > 7356 && codepoint < 7360) return codepoint - 3008;
    if (codepoint > 7943 && codepoint < 7952) return codepoint - 8;
    if (codepoint > 7959 && codepoint < 7966) return codepoint - 8;
    if (codepoint > 7975 && codepoint < 7984) return codepoint - 8;
    if (codepoint > 7991 && codepoint < 8000) return codepoint - 8;
    if (codepoint > 8007 && codepoint < 8014) return codepoint - 8;
    if (codepoint > 8039 && codepoint < 8048) return codepoint - 8;
    if (codepoint > 8071 && codepoint < 8080) return codepoint - 8;
    if (codepoint > 8087 && codepoint < 8096) return codepoint - 8;
    if (codepoint > 8103 && codepoint < 8112) return codepoint - 8;
    if (codepoint > 8119 && codepoint < 8122) return codepoint - 8;
    if (codepoint > 8121 && codepoint < 8124) return codepoint - 74;
    if (codepoint > 8135 && codepoint < 8140) return codepoint - 86;
    if (codepoint > 8151 && codepoint < 8154) return codepoint - 8;
    if (codepoint > 8153 && codepoint < 8156) return codepoint - 100;
    if (codepoint > 8167 && codepoint < 8170) return codepoint - 8;
    if (codepoint > 8169 && codepoint < 8172) return codepoint - 112;
    if (codepoint > 8183 && codepoint < 8186) return codepoint - 128;
    if (codepoint > 8185 && codepoint < 8188) return codepoint - 126;
    if (codepoint > 8543 && codepoint < 8560) return codepoint + 16;
    if (codepoint > 9397 && codepoint < 9424) return codepoint + 26;
    if (codepoint > 11263 && codepoint < 11312) return codepoint + 48;
    if (codepoint > 11389 && codepoint < 11392) return codepoint - 10815;
    if (codepoint > 65312 && codepoint < 65339) return codepoint + 32;
    if (codepoint > 66559 && codepoint < 66600) return codepoint + 40;
    if (codepoint > 66735 && codepoint < 66772) return codepoint + 40;
    if (codepoint > 66927 && codepoint < 66939) return codepoint + 39;
    if (codepoint > 66939 && codepoint < 66955) return codepoint + 39;
    if (codepoint > 66955 && codepoint < 66963) return codepoint + 39;
    if (codepoint > 66963 && codepoint < 66966) return codepoint + 39;
    if (codepoint > 68735 && codepoint < 68787) return codepoint + 64;
    if (codepoint > 68943 && codepoint < 68966) return codepoint + 32;
    if (codepoint > 71839 && codepoint < 71872) return codepoint + 32;
    if (codepoint > 93759 && codepoint < 93792) return codepoint + 32;
    if (codepoint > 125183 && codepoint < 125218) return codepoint + 34;
    return codepoint;
}

uint8_t sfce_codepoint_width(int32_t codepoint)
{
    switch (codepoint) {
    case 908:     case 2142:    case 2482:    case 2519:    case 2620:
    case 2641:    case 2654:    case 2768:    case 2972:    case 3024:
    case 3031:    case 3165:    case 3517:    case 3530:    case 3542:
    case 3716:    case 3749:    case 3782:    case 4295:    case 4301:
    case 4696:    case 4800:    case 6464:    case 8025:    case 8027:
    case 8029:    case 11559:   case 11565:   case 12351:   case 42963:
    case 55296:   case 64318:   case 64975:   case 65279:   case 65952:
    case 67592:   case 67644:   case 67903:   case 69837:   case 70280:
    case 70480:   case 70487:   case 70539:   case 70542:   case 70594:
    case 70597:   case 71945:   case 73018:   case 73648:   case 119970:
    case 119995:  case 120134:  case 123023:  case 123647:  case 124415:
    case 126500:  case 126503:  case 126521:  case 126523:  case 126530:
    case 126535:  case 126537:  case 126539:  case 126548:  case 126551:
    case 126553:  case 126555:  case 126557:  case 126559:  case 126564:
    case 126590:  case 917505:  case 983040:  case 1048573: case 1048576:
    case 1114109: 
        return 1;
    case 9200:   case 9203:   case 9855:   case 9875:   case 9889:
    case 9934:   case 9940:   case 9962:   case 9978:   case 9981:
    case 9989:   case 10024:  case 10060:  case 10160:  case 10175:
    case 11088:  case 11093:  case 44032:  case 55203:  case 94208:
    case 100343: case 101640: case 110898: case 110933: case 126980:
    case 127183: case 127374: case 127988: case 128378: case 128420:
    case 128716: case 129008: case 131072: case 173791: case 173824:
    case 177977: case 177984: case 178205: case 178208: case 183969:
    case 183984: case 191456: case 191472: case 192093: case 196608:
    case 201546: case 201552: case 205743: 
        return 2;
    }

    if (codepoint > -1 && codepoint < 888) return 1;
    if (codepoint > 889 && codepoint < 896) return 1;
    if (codepoint > 899 && codepoint < 907) return 1;
    if (codepoint > 909 && codepoint < 930) return 1;
    if (codepoint > 930 && codepoint < 1328) return 1;
    if (codepoint > 1328 && codepoint < 1367) return 1;
    if (codepoint > 1368 && codepoint < 1419) return 1;
    if (codepoint > 1420 && codepoint < 1424) return 1;
    if (codepoint > 1424 && codepoint < 1480) return 1;
    if (codepoint > 1487 && codepoint < 1515) return 1;
    if (codepoint > 1518 && codepoint < 1525) return 1;
    if (codepoint > 1535 && codepoint < 1806) return 1;
    if (codepoint > 1806 && codepoint < 1867) return 1;
    if (codepoint > 1868 && codepoint < 1970) return 1;
    if (codepoint > 1983 && codepoint < 2043) return 1;
    if (codepoint > 2044 && codepoint < 2094) return 1;
    if (codepoint > 2095 && codepoint < 2111) return 1;
    if (codepoint > 2111 && codepoint < 2140) return 1;
    if (codepoint > 2143 && codepoint < 2155) return 1;
    if (codepoint > 2159 && codepoint < 2191) return 1;
    if (codepoint > 2191 && codepoint < 2194) return 1;
    if (codepoint > 2198 && codepoint < 2436) return 1;
    if (codepoint > 2436 && codepoint < 2445) return 1;
    if (codepoint > 2446 && codepoint < 2449) return 1;
    if (codepoint > 2450 && codepoint < 2473) return 1;
    if (codepoint > 2473 && codepoint < 2481) return 1;
    if (codepoint > 2485 && codepoint < 2490) return 1;
    if (codepoint > 2491 && codepoint < 2501) return 1;
    if (codepoint > 2502 && codepoint < 2505) return 1;
    if (codepoint > 2506 && codepoint < 2511) return 1;
    if (codepoint > 2523 && codepoint < 2526) return 1;
    if (codepoint > 2526 && codepoint < 2532) return 1;
    if (codepoint > 2533 && codepoint < 2559) return 1;
    if (codepoint > 2560 && codepoint < 2564) return 1;
    if (codepoint > 2564 && codepoint < 2571) return 1;
    if (codepoint > 2574 && codepoint < 2577) return 1;
    if (codepoint > 2578 && codepoint < 2601) return 1;
    if (codepoint > 2601 && codepoint < 2609) return 1;
    if (codepoint > 2609 && codepoint < 2612) return 1;
    if (codepoint > 2612 && codepoint < 2615) return 1;
    if (codepoint > 2615 && codepoint < 2618) return 1;
    if (codepoint > 2621 && codepoint < 2627) return 1;
    if (codepoint > 2630 && codepoint < 2633) return 1;
    if (codepoint > 2634 && codepoint < 2638) return 1;
    if (codepoint > 2648 && codepoint < 2653) return 1;
    if (codepoint > 2661 && codepoint < 2679) return 1;
    if (codepoint > 2688 && codepoint < 2692) return 1;
    if (codepoint > 2692 && codepoint < 2702) return 1;
    if (codepoint > 2702 && codepoint < 2706) return 1;
    if (codepoint > 2706 && codepoint < 2729) return 1;
    if (codepoint > 2729 && codepoint < 2737) return 1;
    if (codepoint > 2737 && codepoint < 2740) return 1;
    if (codepoint > 2740 && codepoint < 2746) return 1;
    if (codepoint > 2747 && codepoint < 2758) return 1;
    if (codepoint > 2758 && codepoint < 2762) return 1;
    if (codepoint > 2762 && codepoint < 2766) return 1;
    if (codepoint > 2783 && codepoint < 2788) return 1;
    if (codepoint > 2789 && codepoint < 2802) return 1;
    if (codepoint > 2808 && codepoint < 2816) return 1;
    if (codepoint > 2816 && codepoint < 2820) return 1;
    if (codepoint > 2820 && codepoint < 2829) return 1;
    if (codepoint > 2830 && codepoint < 2833) return 1;
    if (codepoint > 2834 && codepoint < 2857) return 1;
    if (codepoint > 2857 && codepoint < 2865) return 1;
    if (codepoint > 2865 && codepoint < 2868) return 1;
    if (codepoint > 2868 && codepoint < 2874) return 1;
    if (codepoint > 2875 && codepoint < 2885) return 1;
    if (codepoint > 2886 && codepoint < 2889) return 1;
    if (codepoint > 2890 && codepoint < 2894) return 1;
    if (codepoint > 2900 && codepoint < 2904) return 1;
    if (codepoint > 2907 && codepoint < 2910) return 1;
    if (codepoint > 2910 && codepoint < 2916) return 1;
    if (codepoint > 2917 && codepoint < 2936) return 1;
    if (codepoint > 2945 && codepoint < 2948) return 1;
    if (codepoint > 2948 && codepoint < 2955) return 1;
    if (codepoint > 2957 && codepoint < 2961) return 1;
    if (codepoint > 2961 && codepoint < 2966) return 1;
    if (codepoint > 2968 && codepoint < 2971) return 1;
    if (codepoint > 2973 && codepoint < 2976) return 1;
    if (codepoint > 2978 && codepoint < 2981) return 1;
    if (codepoint > 2983 && codepoint < 2987) return 1;
    if (codepoint > 2989 && codepoint < 3002) return 1;
    if (codepoint > 3005 && codepoint < 3011) return 1;
    if (codepoint > 3013 && codepoint < 3017) return 1;
    if (codepoint > 3017 && codepoint < 3022) return 1;
    if (codepoint > 3045 && codepoint < 3067) return 1;
    if (codepoint > 3071 && codepoint < 3085) return 1;
    if (codepoint > 3085 && codepoint < 3089) return 1;
    if (codepoint > 3089 && codepoint < 3113) return 1;
    if (codepoint > 3113 && codepoint < 3130) return 1;
    if (codepoint > 3131 && codepoint < 3141) return 1;
    if (codepoint > 3141 && codepoint < 3145) return 1;
    if (codepoint > 3145 && codepoint < 3150) return 1;
    if (codepoint > 3156 && codepoint < 3159) return 1;
    if (codepoint > 3159 && codepoint < 3163) return 1;
    if (codepoint > 3167 && codepoint < 3172) return 1;
    if (codepoint > 3173 && codepoint < 3184) return 1;
    if (codepoint > 3190 && codepoint < 3213) return 1;
    if (codepoint > 3213 && codepoint < 3217) return 1;
    if (codepoint > 3217 && codepoint < 3241) return 1;
    if (codepoint > 3241 && codepoint < 3252) return 1;
    if (codepoint > 3252 && codepoint < 3258) return 1;
    if (codepoint > 3259 && codepoint < 3269) return 1;
    if (codepoint > 3269 && codepoint < 3273) return 1;
    if (codepoint > 3273 && codepoint < 3278) return 1;
    if (codepoint > 3284 && codepoint < 3287) return 1;
    if (codepoint > 3292 && codepoint < 3295) return 1;
    if (codepoint > 3295 && codepoint < 3300) return 1;
    if (codepoint > 3301 && codepoint < 3312) return 1;
    if (codepoint > 3312 && codepoint < 3316) return 1;
    if (codepoint > 3327 && codepoint < 3341) return 1;
    if (codepoint > 3341 && codepoint < 3345) return 1;
    if (codepoint > 3345 && codepoint < 3397) return 1;
    if (codepoint > 3397 && codepoint < 3401) return 1;
    if (codepoint > 3401 && codepoint < 3408) return 1;
    if (codepoint > 3411 && codepoint < 3428) return 1;
    if (codepoint > 3429 && codepoint < 3456) return 1;
    if (codepoint > 3456 && codepoint < 3460) return 1;
    if (codepoint > 3460 && codepoint < 3479) return 1;
    if (codepoint > 3481 && codepoint < 3506) return 1;
    if (codepoint > 3506 && codepoint < 3516) return 1;
    if (codepoint > 3519 && codepoint < 3527) return 1;
    if (codepoint > 3534 && codepoint < 3541) return 1;
    if (codepoint > 3543 && codepoint < 3552) return 1;
    if (codepoint > 3557 && codepoint < 3568) return 1;
    if (codepoint > 3569 && codepoint < 3573) return 1;
    if (codepoint > 3584 && codepoint < 3643) return 1;
    if (codepoint > 3646 && codepoint < 3676) return 1;
    if (codepoint > 3712 && codepoint < 3715) return 1;
    if (codepoint > 3717 && codepoint < 3723) return 1;
    if (codepoint > 3723 && codepoint < 3748) return 1;
    if (codepoint > 3750 && codepoint < 3774) return 1;
    if (codepoint > 3775 && codepoint < 3781) return 1;
    if (codepoint > 3783 && codepoint < 3791) return 1;
    if (codepoint > 3791 && codepoint < 3802) return 1;
    if (codepoint > 3803 && codepoint < 3808) return 1;
    if (codepoint > 3839 && codepoint < 3912) return 1;
    if (codepoint > 3912 && codepoint < 3949) return 1;
    if (codepoint > 3952 && codepoint < 3992) return 1;
    if (codepoint > 3992 && codepoint < 4029) return 1;
    if (codepoint > 4029 && codepoint < 4045) return 1;
    if (codepoint > 4045 && codepoint < 4059) return 1;
    if (codepoint > 4095 && codepoint < 4294) return 1;
    if (codepoint > 4303 && codepoint < 4352) return 1;
    if (codepoint > 4351 && codepoint < 4448) return 2;
    if (codepoint > 4447 && codepoint < 4681) return 1;
    if (codepoint > 4681 && codepoint < 4686) return 1;
    if (codepoint > 4687 && codepoint < 4695) return 1;
    if (codepoint > 4697 && codepoint < 4702) return 1;
    if (codepoint > 4703 && codepoint < 4745) return 1;
    if (codepoint > 4745 && codepoint < 4750) return 1;
    if (codepoint > 4751 && codepoint < 4785) return 1;
    if (codepoint > 4785 && codepoint < 4790) return 1;
    if (codepoint > 4791 && codepoint < 4799) return 1;
    if (codepoint > 4801 && codepoint < 4806) return 1;
    if (codepoint > 4807 && codepoint < 4823) return 1;
    if (codepoint > 4823 && codepoint < 4881) return 1;
    if (codepoint > 4881 && codepoint < 4886) return 1;
    if (codepoint > 4887 && codepoint < 4955) return 1;
    if (codepoint > 4956 && codepoint < 4989) return 1;
    if (codepoint > 4991 && codepoint < 5018) return 1;
    if (codepoint > 5023 && codepoint < 5110) return 1;
    if (codepoint > 5111 && codepoint < 5118) return 1;
    if (codepoint > 5119 && codepoint < 5789) return 1;
    if (codepoint > 5791 && codepoint < 5881) return 1;
    if (codepoint > 5887 && codepoint < 5910) return 1;
    if (codepoint > 5918 && codepoint < 5943) return 1;
    if (codepoint > 5951 && codepoint < 5972) return 1;
    if (codepoint > 5983 && codepoint < 5997) return 1;
    if (codepoint > 5997 && codepoint < 6001) return 1;
    if (codepoint > 6001 && codepoint < 6004) return 1;
    if (codepoint > 6015 && codepoint < 6110) return 1;
    if (codepoint > 6111 && codepoint < 6122) return 1;
    if (codepoint > 6127 && codepoint < 6138) return 1;
    if (codepoint > 6143 && codepoint < 6170) return 1;
    if (codepoint > 6175 && codepoint < 6265) return 1;
    if (codepoint > 6271 && codepoint < 6315) return 1;
    if (codepoint > 6319 && codepoint < 6390) return 1;
    if (codepoint > 6399 && codepoint < 6431) return 1;
    if (codepoint > 6431 && codepoint < 6444) return 1;
    if (codepoint > 6447 && codepoint < 6460) return 1;
    if (codepoint > 6467 && codepoint < 6510) return 1;
    if (codepoint > 6511 && codepoint < 6517) return 1;
    if (codepoint > 6527 && codepoint < 6572) return 1;
    if (codepoint > 6575 && codepoint < 6602) return 1;
    if (codepoint > 6607 && codepoint < 6619) return 1;
    if (codepoint > 6621 && codepoint < 6684) return 1;
    if (codepoint > 6685 && codepoint < 6751) return 1;
    if (codepoint > 6751 && codepoint < 6781) return 1;
    if (codepoint > 6782 && codepoint < 6794) return 1;
    if (codepoint > 6799 && codepoint < 6810) return 1;
    if (codepoint > 6815 && codepoint < 6830) return 1;
    if (codepoint > 6831 && codepoint < 6863) return 1;
    if (codepoint > 6911 && codepoint < 6989) return 1;
    if (codepoint > 6989 && codepoint < 7156) return 1;
    if (codepoint > 7163 && codepoint < 7224) return 1;
    if (codepoint > 7226 && codepoint < 7242) return 1;
    if (codepoint > 7244 && codepoint < 7307) return 1;
    if (codepoint > 7311 && codepoint < 7355) return 1;
    if (codepoint > 7356 && codepoint < 7368) return 1;
    if (codepoint > 7375 && codepoint < 7419) return 1;
    if (codepoint > 7423 && codepoint < 7958) return 1;
    if (codepoint > 7959 && codepoint < 7966) return 1;
    if (codepoint > 7967 && codepoint < 8006) return 1;
    if (codepoint > 8007 && codepoint < 8014) return 1;
    if (codepoint > 8015 && codepoint < 8024) return 1;
    if (codepoint > 8030 && codepoint < 8062) return 1;
    if (codepoint > 8063 && codepoint < 8117) return 1;
    if (codepoint > 8117 && codepoint < 8133) return 1;
    if (codepoint > 8133 && codepoint < 8148) return 1;
    if (codepoint > 8149 && codepoint < 8156) return 1;
    if (codepoint > 8156 && codepoint < 8176) return 1;
    if (codepoint > 8177 && codepoint < 8181) return 1;
    if (codepoint > 8181 && codepoint < 8191) return 1;
    if (codepoint > 8191 && codepoint < 8293) return 1;
    if (codepoint > 8293 && codepoint < 8306) return 1;
    if (codepoint > 8307 && codepoint < 8335) return 1;
    if (codepoint > 8335 && codepoint < 8349) return 1;
    if (codepoint > 8351 && codepoint < 8385) return 1;
    if (codepoint > 8399 && codepoint < 8433) return 1;
    if (codepoint > 8447 && codepoint < 8588) return 1;
    if (codepoint > 8591 && codepoint < 8986) return 1;
    if (codepoint > 8985 && codepoint < 8988) return 2;
    if (codepoint > 8987 && codepoint < 9001) return 1;
    if (codepoint > 9000 && codepoint < 9003) return 2;
    if (codepoint > 9002 && codepoint < 9193) return 1;
    if (codepoint > 9192 && codepoint < 9197) return 2;
    if (codepoint > 9196 && codepoint < 9200) return 1;
    if (codepoint > 9200 && codepoint < 9203) return 1;
    if (codepoint > 9203 && codepoint < 9258) return 1;
    if (codepoint > 9279 && codepoint < 9291) return 1;
    if (codepoint > 9311 && codepoint < 9725) return 1;
    if (codepoint > 9724 && codepoint < 9727) return 2;
    if (codepoint > 9726 && codepoint < 9748) return 1;
    if (codepoint > 9747 && codepoint < 9750) return 2;
    if (codepoint > 9749 && codepoint < 9776) return 1;
    if (codepoint > 9775 && codepoint < 9784) return 2;
    if (codepoint > 9783 && codepoint < 9800) return 1;
    if (codepoint > 9799 && codepoint < 9812) return 2;
    if (codepoint > 9811 && codepoint < 9855) return 1;
    if (codepoint > 9855 && codepoint < 9866) return 1;
    if (codepoint > 9865 && codepoint < 9872) return 2;
    if (codepoint > 9871 && codepoint < 9875) return 1;
    if (codepoint > 9875 && codepoint < 9889) return 1;
    if (codepoint > 9889 && codepoint < 9898) return 1;
    if (codepoint > 9897 && codepoint < 9900) return 2;
    if (codepoint > 9899 && codepoint < 9917) return 1;
    if (codepoint > 9916 && codepoint < 9919) return 2;
    if (codepoint > 9918 && codepoint < 9924) return 1;
    if (codepoint > 9923 && codepoint < 9926) return 2;
    if (codepoint > 9925 && codepoint < 9934) return 1;
    if (codepoint > 9934 && codepoint < 9940) return 1;
    if (codepoint > 9940 && codepoint < 9962) return 1;
    if (codepoint > 9962 && codepoint < 9970) return 1;
    if (codepoint > 9969 && codepoint < 9972) return 2;
    if (codepoint > 9971 && codepoint < 9978) return 1;
    if (codepoint > 9978 && codepoint < 9981) return 1;
    if (codepoint > 9981 && codepoint < 9989) return 1;
    if (codepoint > 9989 && codepoint < 9994) return 1;
    if (codepoint > 9993 && codepoint < 9996) return 2;
    if (codepoint > 9995 && codepoint < 10024) return 1;
    if (codepoint > 10024 && codepoint < 10060) return 1;
    if (codepoint > 10060 && codepoint < 10067) return 1;
    if (codepoint > 10066 && codepoint < 10070) return 2;
    if (codepoint > 10069 && codepoint < 10133) return 1;
    if (codepoint > 10132 && codepoint < 10136) return 2;
    if (codepoint > 10135 && codepoint < 10160) return 1;
    if (codepoint > 10160 && codepoint < 10175) return 1;
    if (codepoint > 10175 && codepoint < 11035) return 1;
    if (codepoint > 11034 && codepoint < 11037) return 2;
    if (codepoint > 11036 && codepoint < 11088) return 1;
    if (codepoint > 11088 && codepoint < 11093) return 1;
    if (codepoint > 11093 && codepoint < 11124) return 1;
    if (codepoint > 11125 && codepoint < 11158) return 1;
    if (codepoint > 11158 && codepoint < 11508) return 1;
    if (codepoint > 11512 && codepoint < 11558) return 1;
    if (codepoint > 11567 && codepoint < 11624) return 1;
    if (codepoint > 11630 && codepoint < 11633) return 1;
    if (codepoint > 11646 && codepoint < 11671) return 1;
    if (codepoint > 11679 && codepoint < 11687) return 1;
    if (codepoint > 11687 && codepoint < 11695) return 1;
    if (codepoint > 11695 && codepoint < 11703) return 1;
    if (codepoint > 11703 && codepoint < 11711) return 1;
    if (codepoint > 11711 && codepoint < 11719) return 1;
    if (codepoint > 11719 && codepoint < 11727) return 1;
    if (codepoint > 11727 && codepoint < 11735) return 1;
    if (codepoint > 11735 && codepoint < 11743) return 1;
    if (codepoint > 11743 && codepoint < 11870) return 1;
    if (codepoint > 11903 && codepoint < 11930) return 2;
    if (codepoint > 11930 && codepoint < 12020) return 2;
    if (codepoint > 12031 && codepoint < 12246) return 2;
    if (codepoint > 12271 && codepoint < 12351) return 2;
    if (codepoint > 12352 && codepoint < 12439) return 2;
    if (codepoint > 12440 && codepoint < 12544) return 2;
    if (codepoint > 12548 && codepoint < 12592) return 2;
    if (codepoint > 12592 && codepoint < 12687) return 2;
    if (codepoint > 12687 && codepoint < 12774) return 2;
    if (codepoint > 12782 && codepoint < 12831) return 2;
    if (codepoint > 12831 && codepoint < 12872) return 2;
    if (codepoint > 12871 && codepoint < 12880) return 1;
    if (codepoint > 12879 && codepoint < 13313) return 2;
    if (codepoint > 19902 && codepoint < 19969) return 2;
    if (codepoint > 40958 && codepoint < 42125) return 2;
    if (codepoint > 42127 && codepoint < 42183) return 2;
    if (codepoint > 42191 && codepoint < 42540) return 1;
    if (codepoint > 42559 && codepoint < 42744) return 1;
    if (codepoint > 42751 && codepoint < 42958) return 1;
    if (codepoint > 42959 && codepoint < 42962) return 1;
    if (codepoint > 42964 && codepoint < 42973) return 1;
    if (codepoint > 42993 && codepoint < 43053) return 1;
    if (codepoint > 43055 && codepoint < 43066) return 1;
    if (codepoint > 43071 && codepoint < 43128) return 1;
    if (codepoint > 43135 && codepoint < 43206) return 1;
    if (codepoint > 43213 && codepoint < 43226) return 1;
    if (codepoint > 43231 && codepoint < 43348) return 1;
    if (codepoint > 43358 && codepoint < 43361) return 1;
    if (codepoint > 43360 && codepoint < 43389) return 2;
    if (codepoint > 43391 && codepoint < 43470) return 1;
    if (codepoint > 43470 && codepoint < 43482) return 1;
    if (codepoint > 43485 && codepoint < 43519) return 1;
    if (codepoint > 43519 && codepoint < 43575) return 1;
    if (codepoint > 43583 && codepoint < 43598) return 1;
    if (codepoint > 43599 && codepoint < 43610) return 1;
    if (codepoint > 43611 && codepoint < 43715) return 1;
    if (codepoint > 43738 && codepoint < 43767) return 1;
    if (codepoint > 43776 && codepoint < 43783) return 1;
    if (codepoint > 43784 && codepoint < 43791) return 1;
    if (codepoint > 43792 && codepoint < 43799) return 1;
    if (codepoint > 43807 && codepoint < 43815) return 1;
    if (codepoint > 43815 && codepoint < 43823) return 1;
    if (codepoint > 43823 && codepoint < 43884) return 1;
    if (codepoint > 43887 && codepoint < 44014) return 1;
    if (codepoint > 44015 && codepoint < 44026) return 1;
    if (codepoint > 55215 && codepoint < 55239) return 1;
    if (codepoint > 55242 && codepoint < 55292) return 1;
    if (codepoint > 56190 && codepoint < 56193) return 1;
    if (codepoint > 56318 && codepoint < 56321) return 1;
    if (codepoint > 57342 && codepoint < 57345) return 1;
    if (codepoint > 63742 && codepoint < 63745) return 1;
    if (codepoint > 63744 && codepoint < 64110) return 2;
    if (codepoint > 64111 && codepoint < 64218) return 2;
    if (codepoint > 64255 && codepoint < 64263) return 1;
    if (codepoint > 64274 && codepoint < 64280) return 1;
    if (codepoint > 64284 && codepoint < 64311) return 1;
    if (codepoint > 64311 && codepoint < 64317) return 1;
    if (codepoint > 64319 && codepoint < 64322) return 1;
    if (codepoint > 64322 && codepoint < 64325) return 1;
    if (codepoint > 64325 && codepoint < 64451) return 1;
    if (codepoint > 64466 && codepoint < 64912) return 1;
    if (codepoint > 64913 && codepoint < 64968) return 1;
    if (codepoint > 65007 && codepoint < 65040) return 1;
    if (codepoint > 65039 && codepoint < 65050) return 2;
    if (codepoint > 65055 && codepoint < 65072) return 1;
    if (codepoint > 65071 && codepoint < 65107) return 2;
    if (codepoint > 65107 && codepoint < 65127) return 2;
    if (codepoint > 65127 && codepoint < 65132) return 2;
    if (codepoint > 65135 && codepoint < 65141) return 1;
    if (codepoint > 65141 && codepoint < 65277) return 1;
    if (codepoint > 65280 && codepoint < 65377) return 2;
    if (codepoint > 65376 && codepoint < 65471) return 1;
    if (codepoint > 65473 && codepoint < 65480) return 1;
    if (codepoint > 65481 && codepoint < 65488) return 1;
    if (codepoint > 65489 && codepoint < 65496) return 1;
    if (codepoint > 65497 && codepoint < 65501) return 1;
    if (codepoint > 65503 && codepoint < 65511) return 2;
    if (codepoint > 65511 && codepoint < 65519) return 1;
    if (codepoint > 65528 && codepoint < 65534) return 1;
    if (codepoint > 65535 && codepoint < 65548) return 1;
    if (codepoint > 65548 && codepoint < 65575) return 1;
    if (codepoint > 65575 && codepoint < 65595) return 1;
    if (codepoint > 65595 && codepoint < 65598) return 1;
    if (codepoint > 65598 && codepoint < 65614) return 1;
    if (codepoint > 65615 && codepoint < 65630) return 1;
    if (codepoint > 65663 && codepoint < 65787) return 1;
    if (codepoint > 65791 && codepoint < 65795) return 1;
    if (codepoint > 65798 && codepoint < 65844) return 1;
    if (codepoint > 65846 && codepoint < 65935) return 1;
    if (codepoint > 65935 && codepoint < 65949) return 1;
    if (codepoint > 65999 && codepoint < 66046) return 1;
    if (codepoint > 66175 && codepoint < 66205) return 1;
    if (codepoint > 66207 && codepoint < 66257) return 1;
    if (codepoint > 66271 && codepoint < 66300) return 1;
    if (codepoint > 66303 && codepoint < 66340) return 1;
    if (codepoint > 66348 && codepoint < 66379) return 1;
    if (codepoint > 66383 && codepoint < 66427) return 1;
    if (codepoint > 66431 && codepoint < 66462) return 1;
    if (codepoint > 66462 && codepoint < 66500) return 1;
    if (codepoint > 66503 && codepoint < 66518) return 1;
    if (codepoint > 66559 && codepoint < 66718) return 1;
    if (codepoint > 66719 && codepoint < 66730) return 1;
    if (codepoint > 66735 && codepoint < 66772) return 1;
    if (codepoint > 66775 && codepoint < 66812) return 1;
    if (codepoint > 66815 && codepoint < 66856) return 1;
    if (codepoint > 66863 && codepoint < 66916) return 1;
    if (codepoint > 66926 && codepoint < 66939) return 1;
    if (codepoint > 66939 && codepoint < 66955) return 1;
    if (codepoint > 66955 && codepoint < 66963) return 1;
    if (codepoint > 66963 && codepoint < 66966) return 1;
    if (codepoint > 66966 && codepoint < 66978) return 1;
    if (codepoint > 66978 && codepoint < 66994) return 1;
    if (codepoint > 66994 && codepoint < 67002) return 1;
    if (codepoint > 67002 && codepoint < 67005) return 1;
    if (codepoint > 67007 && codepoint < 67060) return 1;
    if (codepoint > 67071 && codepoint < 67383) return 1;
    if (codepoint > 67391 && codepoint < 67414) return 1;
    if (codepoint > 67423 && codepoint < 67432) return 1;
    if (codepoint > 67455 && codepoint < 67462) return 1;
    if (codepoint > 67462 && codepoint < 67505) return 1;
    if (codepoint > 67505 && codepoint < 67515) return 1;
    if (codepoint > 67583 && codepoint < 67590) return 1;
    if (codepoint > 67593 && codepoint < 67638) return 1;
    if (codepoint > 67638 && codepoint < 67641) return 1;
    if (codepoint > 67646 && codepoint < 67670) return 1;
    if (codepoint > 67670 && codepoint < 67743) return 1;
    if (codepoint > 67750 && codepoint < 67760) return 1;
    if (codepoint > 67807 && codepoint < 67827) return 1;
    if (codepoint > 67827 && codepoint < 67830) return 1;
    if (codepoint > 67834 && codepoint < 67868) return 1;
    if (codepoint > 67870 && codepoint < 67898) return 1;
    if (codepoint > 67967 && codepoint < 68024) return 1;
    if (codepoint > 68027 && codepoint < 68048) return 1;
    if (codepoint > 68049 && codepoint < 68100) return 1;
    if (codepoint > 68100 && codepoint < 68103) return 1;
    if (codepoint > 68107 && codepoint < 68116) return 1;
    if (codepoint > 68116 && codepoint < 68120) return 1;
    if (codepoint > 68120 && codepoint < 68150) return 1;
    if (codepoint > 68151 && codepoint < 68155) return 1;
    if (codepoint > 68158 && codepoint < 68169) return 1;
    if (codepoint > 68175 && codepoint < 68185) return 1;
    if (codepoint > 68191 && codepoint < 68256) return 1;
    if (codepoint > 68287 && codepoint < 68327) return 1;
    if (codepoint > 68330 && codepoint < 68343) return 1;
    if (codepoint > 68351 && codepoint < 68406) return 1;
    if (codepoint > 68408 && codepoint < 68438) return 1;
    if (codepoint > 68439 && codepoint < 68467) return 1;
    if (codepoint > 68471 && codepoint < 68498) return 1;
    if (codepoint > 68504 && codepoint < 68509) return 1;
    if (codepoint > 68520 && codepoint < 68528) return 1;
    if (codepoint > 68607 && codepoint < 68681) return 1;
    if (codepoint > 68735 && codepoint < 68787) return 1;
    if (codepoint > 68799 && codepoint < 68851) return 1;
    if (codepoint > 68857 && codepoint < 68904) return 1;
    if (codepoint > 68911 && codepoint < 68922) return 1;
    if (codepoint > 68927 && codepoint < 68966) return 1;
    if (codepoint > 68968 && codepoint < 68998) return 1;
    if (codepoint > 69005 && codepoint < 69008) return 1;
    if (codepoint > 69215 && codepoint < 69247) return 1;
    if (codepoint > 69247 && codepoint < 69290) return 1;
    if (codepoint > 69290 && codepoint < 69294) return 1;
    if (codepoint > 69295 && codepoint < 69298) return 1;
    if (codepoint > 69313 && codepoint < 69317) return 1;
    if (codepoint > 69371 && codepoint < 69416) return 1;
    if (codepoint > 69423 && codepoint < 69466) return 1;
    if (codepoint > 69487 && codepoint < 69514) return 1;
    if (codepoint > 69551 && codepoint < 69580) return 1;
    if (codepoint > 69599 && codepoint < 69623) return 1;
    if (codepoint > 69631 && codepoint < 69710) return 1;
    if (codepoint > 69713 && codepoint < 69750) return 1;
    if (codepoint > 69758 && codepoint < 69827) return 1;
    if (codepoint > 69839 && codepoint < 69865) return 1;
    if (codepoint > 69871 && codepoint < 69882) return 1;
    if (codepoint > 69887 && codepoint < 69941) return 1;
    if (codepoint > 69941 && codepoint < 69960) return 1;
    if (codepoint > 69967 && codepoint < 70007) return 1;
    if (codepoint > 70015 && codepoint < 70112) return 1;
    if (codepoint > 70112 && codepoint < 70133) return 1;
    if (codepoint > 70143 && codepoint < 70162) return 1;
    if (codepoint > 70162 && codepoint < 70210) return 1;
    if (codepoint > 70271 && codepoint < 70279) return 1;
    if (codepoint > 70281 && codepoint < 70286) return 1;
    if (codepoint > 70286 && codepoint < 70302) return 1;
    if (codepoint > 70302 && codepoint < 70314) return 1;
    if (codepoint > 70319 && codepoint < 70379) return 1;
    if (codepoint > 70383 && codepoint < 70394) return 1;
    if (codepoint > 70399 && codepoint < 70404) return 1;
    if (codepoint > 70404 && codepoint < 70413) return 1;
    if (codepoint > 70414 && codepoint < 70417) return 1;
    if (codepoint > 70418 && codepoint < 70441) return 1;
    if (codepoint > 70441 && codepoint < 70449) return 1;
    if (codepoint > 70449 && codepoint < 70452) return 1;
    if (codepoint > 70452 && codepoint < 70458) return 1;
    if (codepoint > 70458 && codepoint < 70469) return 1;
    if (codepoint > 70470 && codepoint < 70473) return 1;
    if (codepoint > 70474 && codepoint < 70478) return 1;
    if (codepoint > 70492 && codepoint < 70500) return 1;
    if (codepoint > 70501 && codepoint < 70509) return 1;
    if (codepoint > 70511 && codepoint < 70517) return 1;
    if (codepoint > 70527 && codepoint < 70538) return 1;
    if (codepoint > 70543 && codepoint < 70582) return 1;
    if (codepoint > 70582 && codepoint < 70593) return 1;
    if (codepoint > 70598 && codepoint < 70603) return 1;
    if (codepoint > 70603 && codepoint < 70614) return 1;
    if (codepoint > 70614 && codepoint < 70617) return 1;
    if (codepoint > 70624 && codepoint < 70627) return 1;
    if (codepoint > 70655 && codepoint < 70748) return 1;
    if (codepoint > 70748 && codepoint < 70754) return 1;
    if (codepoint > 70783 && codepoint < 70856) return 1;
    if (codepoint > 70863 && codepoint < 70874) return 1;
    if (codepoint > 71039 && codepoint < 71094) return 1;
    if (codepoint > 71095 && codepoint < 71134) return 1;
    if (codepoint > 71167 && codepoint < 71237) return 1;
    if (codepoint > 71247 && codepoint < 71258) return 1;
    if (codepoint > 71263 && codepoint < 71277) return 1;
    if (codepoint > 71295 && codepoint < 71354) return 1;
    if (codepoint > 71359 && codepoint < 71370) return 1;
    if (codepoint > 71375 && codepoint < 71396) return 1;
    if (codepoint > 71423 && codepoint < 71451) return 1;
    if (codepoint > 71452 && codepoint < 71468) return 1;
    if (codepoint > 71471 && codepoint < 71495) return 1;
    if (codepoint > 71679 && codepoint < 71740) return 1;
    if (codepoint > 71839 && codepoint < 71923) return 1;
    if (codepoint > 71934 && codepoint < 71943) return 1;
    if (codepoint > 71947 && codepoint < 71956) return 1;
    if (codepoint > 71956 && codepoint < 71959) return 1;
    if (codepoint > 71959 && codepoint < 71990) return 1;
    if (codepoint > 71990 && codepoint < 71993) return 1;
    if (codepoint > 71994 && codepoint < 72007) return 1;
    if (codepoint > 72015 && codepoint < 72026) return 1;
    if (codepoint > 72095 && codepoint < 72104) return 1;
    if (codepoint > 72105 && codepoint < 72152) return 1;
    if (codepoint > 72153 && codepoint < 72165) return 1;
    if (codepoint > 72191 && codepoint < 72264) return 1;
    if (codepoint > 72271 && codepoint < 72355) return 1;
    if (codepoint > 72367 && codepoint < 72441) return 1;
    if (codepoint > 72447 && codepoint < 72458) return 1;
    if (codepoint > 72639 && codepoint < 72674) return 1;
    if (codepoint > 72687 && codepoint < 72698) return 1;
    if (codepoint > 72703 && codepoint < 72713) return 1;
    if (codepoint > 72713 && codepoint < 72759) return 1;
    if (codepoint > 72759 && codepoint < 72774) return 1;
    if (codepoint > 72783 && codepoint < 72813) return 1;
    if (codepoint > 72815 && codepoint < 72848) return 1;
    if (codepoint > 72849 && codepoint < 72872) return 1;
    if (codepoint > 72872 && codepoint < 72887) return 1;
    if (codepoint > 72959 && codepoint < 72967) return 1;
    if (codepoint > 72967 && codepoint < 72970) return 1;
    if (codepoint > 72970 && codepoint < 73015) return 1;
    if (codepoint > 73019 && codepoint < 73022) return 1;
    if (codepoint > 73022 && codepoint < 73032) return 1;
    if (codepoint > 73039 && codepoint < 73050) return 1;
    if (codepoint > 73055 && codepoint < 73062) return 1;
    if (codepoint > 73062 && codepoint < 73065) return 1;
    if (codepoint > 73065 && codepoint < 73103) return 1;
    if (codepoint > 73103 && codepoint < 73106) return 1;
    if (codepoint > 73106 && codepoint < 73113) return 1;
    if (codepoint > 73119 && codepoint < 73130) return 1;
    if (codepoint > 73439 && codepoint < 73465) return 1;
    if (codepoint > 73471 && codepoint < 73489) return 1;
    if (codepoint > 73489 && codepoint < 73531) return 1;
    if (codepoint > 73533 && codepoint < 73563) return 1;
    if (codepoint > 73663 && codepoint < 73714) return 1;
    if (codepoint > 73726 && codepoint < 74650) return 1;
    if (codepoint > 74751 && codepoint < 74863) return 1;
    if (codepoint > 74863 && codepoint < 74869) return 1;
    if (codepoint > 74879 && codepoint < 75076) return 1;
    if (codepoint > 77711 && codepoint < 77811) return 1;
    if (codepoint > 77823 && codepoint < 78934) return 1;
    if (codepoint > 78943 && codepoint < 82939) return 1;
    if (codepoint > 82943 && codepoint < 83527) return 1;
    if (codepoint > 90367 && codepoint < 90426) return 1;
    if (codepoint > 92159 && codepoint < 92729) return 1;
    if (codepoint > 92735 && codepoint < 92767) return 1;
    if (codepoint > 92767 && codepoint < 92778) return 1;
    if (codepoint > 92781 && codepoint < 92863) return 1;
    if (codepoint > 92863 && codepoint < 92874) return 1;
    if (codepoint > 92879 && codepoint < 92910) return 1;
    if (codepoint > 92911 && codepoint < 92918) return 1;
    if (codepoint > 92927 && codepoint < 92998) return 1;
    if (codepoint > 93007 && codepoint < 93018) return 1;
    if (codepoint > 93018 && codepoint < 93026) return 1;
    if (codepoint > 93026 && codepoint < 93048) return 1;
    if (codepoint > 93052 && codepoint < 93072) return 1;
    if (codepoint > 93503 && codepoint < 93562) return 1;
    if (codepoint > 93759 && codepoint < 93851) return 1;
    if (codepoint > 93951 && codepoint < 94027) return 1;
    if (codepoint > 94030 && codepoint < 94088) return 1;
    if (codepoint > 94094 && codepoint < 94112) return 1;
    if (codepoint > 94175 && codepoint < 94181) return 2;
    if (codepoint > 94191 && codepoint < 94194) return 2;
    if (codepoint > 100351 && codepoint < 101590) return 2;
    if (codepoint > 101630 && codepoint < 101633) return 2;
    if (codepoint > 110575 && codepoint < 110580) return 2;
    if (codepoint > 110580 && codepoint < 110588) return 2;
    if (codepoint > 110588 && codepoint < 110591) return 2;
    if (codepoint > 110591 && codepoint < 110883) return 2;
    if (codepoint > 110927 && codepoint < 110931) return 2;
    if (codepoint > 110947 && codepoint < 110952) return 2;
    if (codepoint > 110959 && codepoint < 111356) return 2;
    if (codepoint > 113663 && codepoint < 113771) return 1;
    if (codepoint > 113775 && codepoint < 113789) return 1;
    if (codepoint > 113791 && codepoint < 113801) return 1;
    if (codepoint > 113807 && codepoint < 113818) return 1;
    if (codepoint > 113819 && codepoint < 113828) return 1;
    if (codepoint > 117759 && codepoint < 118010) return 1;
    if (codepoint > 118015 && codepoint < 118452) return 1;
    if (codepoint > 118527 && codepoint < 118574) return 1;
    if (codepoint > 118575 && codepoint < 118599) return 1;
    if (codepoint > 118607 && codepoint < 118724) return 1;
    if (codepoint > 118783 && codepoint < 119030) return 1;
    if (codepoint > 119039 && codepoint < 119079) return 1;
    if (codepoint > 119080 && codepoint < 119275) return 1;
    if (codepoint > 119295 && codepoint < 119366) return 1;
    if (codepoint > 119487 && codepoint < 119508) return 1;
    if (codepoint > 119519 && codepoint < 119540) return 1;
    if (codepoint > 119551 && codepoint < 119639) return 2;
    if (codepoint > 119647 && codepoint < 119671) return 2;
    if (codepoint > 119670 && codepoint < 119673) return 1;
    if (codepoint > 119807 && codepoint < 119893) return 1;
    if (codepoint > 119893 && codepoint < 119965) return 1;
    if (codepoint > 119965 && codepoint < 119968) return 1;
    if (codepoint > 119972 && codepoint < 119975) return 1;
    if (codepoint > 119976 && codepoint < 119981) return 1;
    if (codepoint > 119981 && codepoint < 119994) return 1;
    if (codepoint > 119996 && codepoint < 120004) return 1;
    if (codepoint > 120004 && codepoint < 120070) return 1;
    if (codepoint > 120070 && codepoint < 120075) return 1;
    if (codepoint > 120076 && codepoint < 120085) return 1;
    if (codepoint > 120085 && codepoint < 120093) return 1;
    if (codepoint > 120093 && codepoint < 120122) return 1;
    if (codepoint > 120122 && codepoint < 120127) return 1;
    if (codepoint > 120127 && codepoint < 120133) return 1;
    if (codepoint > 120137 && codepoint < 120145) return 1;
    if (codepoint > 120145 && codepoint < 120486) return 1;
    if (codepoint > 120487 && codepoint < 120780) return 1;
    if (codepoint > 120781 && codepoint < 121484) return 1;
    if (codepoint > 121498 && codepoint < 121504) return 1;
    if (codepoint > 121504 && codepoint < 121520) return 1;
    if (codepoint > 122623 && codepoint < 122655) return 1;
    if (codepoint > 122660 && codepoint < 122667) return 1;
    if (codepoint > 122879 && codepoint < 122887) return 1;
    if (codepoint > 122887 && codepoint < 122905) return 1;
    if (codepoint > 122906 && codepoint < 122914) return 1;
    if (codepoint > 122914 && codepoint < 122917) return 1;
    if (codepoint > 122917 && codepoint < 122923) return 1;
    if (codepoint > 122927 && codepoint < 122990) return 1;
    if (codepoint > 123135 && codepoint < 123181) return 1;
    if (codepoint > 123183 && codepoint < 123198) return 1;
    if (codepoint > 123199 && codepoint < 123210) return 1;
    if (codepoint > 123213 && codepoint < 123216) return 1;
    if (codepoint > 123535 && codepoint < 123567) return 1;
    if (codepoint > 123583 && codepoint < 123642) return 1;
    if (codepoint > 124111 && codepoint < 124154) return 1;
    if (codepoint > 124367 && codepoint < 124411) return 1;
    if (codepoint > 124895 && codepoint < 124903) return 1;
    if (codepoint > 124903 && codepoint < 124908) return 1;
    if (codepoint > 124908 && codepoint < 124911) return 1;
    if (codepoint > 124911 && codepoint < 124927) return 1;
    if (codepoint > 124927 && codepoint < 125125) return 1;
    if (codepoint > 125126 && codepoint < 125143) return 1;
    if (codepoint > 125183 && codepoint < 125260) return 1;
    if (codepoint > 125263 && codepoint < 125274) return 1;
    if (codepoint > 125277 && codepoint < 125280) return 1;
    if (codepoint > 126064 && codepoint < 126133) return 1;
    if (codepoint > 126208 && codepoint < 126270) return 1;
    if (codepoint > 126463 && codepoint < 126468) return 1;
    if (codepoint > 126468 && codepoint < 126496) return 1;
    if (codepoint > 126496 && codepoint < 126499) return 1;
    if (codepoint > 126504 && codepoint < 126515) return 1;
    if (codepoint > 126515 && codepoint < 126520) return 1;
    if (codepoint > 126540 && codepoint < 126544) return 1;
    if (codepoint > 126544 && codepoint < 126547) return 1;
    if (codepoint > 126560 && codepoint < 126563) return 1;
    if (codepoint > 126566 && codepoint < 126571) return 1;
    if (codepoint > 126571 && codepoint < 126579) return 1;
    if (codepoint > 126579 && codepoint < 126584) return 1;
    if (codepoint > 126584 && codepoint < 126589) return 1;
    if (codepoint > 126591 && codepoint < 126602) return 1;
    if (codepoint > 126602 && codepoint < 126620) return 1;
    if (codepoint > 126624 && codepoint < 126628) return 1;
    if (codepoint > 126628 && codepoint < 126634) return 1;
    if (codepoint > 126634 && codepoint < 126652) return 1;
    if (codepoint > 126703 && codepoint < 126706) return 1;
    if (codepoint > 126975 && codepoint < 126980) return 1;
    if (codepoint > 126980 && codepoint < 127020) return 1;
    if (codepoint > 127023 && codepoint < 127124) return 1;
    if (codepoint > 127135 && codepoint < 127151) return 1;
    if (codepoint > 127152 && codepoint < 127168) return 1;
    if (codepoint > 127168 && codepoint < 127183) return 1;
    if (codepoint > 127184 && codepoint < 127222) return 1;
    if (codepoint > 127231 && codepoint < 127374) return 1;
    if (codepoint > 127374 && codepoint < 127377) return 1;
    if (codepoint > 127376 && codepoint < 127387) return 2;
    if (codepoint > 127386 && codepoint < 127406) return 1;
    if (codepoint > 127461 && codepoint < 127488) return 1;
    if (codepoint > 127487 && codepoint < 127491) return 2;
    if (codepoint > 127503 && codepoint < 127548) return 2;
    if (codepoint > 127551 && codepoint < 127561) return 2;
    if (codepoint > 127567 && codepoint < 127570) return 2;
    if (codepoint > 127583 && codepoint < 127590) return 2;
    if (codepoint > 127743 && codepoint < 127777) return 2;
    if (codepoint > 127776 && codepoint < 127789) return 1;
    if (codepoint > 127788 && codepoint < 127798) return 2;
    if (codepoint > 127797 && codepoint < 127800) return 1;
    if (codepoint > 127799 && codepoint < 127869) return 2;
    if (codepoint > 127868 && codepoint < 127871) return 1;
    if (codepoint > 127870 && codepoint < 127892) return 2;
    if (codepoint > 127891 && codepoint < 127904) return 1;
    if (codepoint > 127903 && codepoint < 127947) return 2;
    if (codepoint > 127946 && codepoint < 127951) return 1;
    if (codepoint > 127950 && codepoint < 127956) return 2;
    if (codepoint > 127955 && codepoint < 127968) return 1;
    if (codepoint > 127967 && codepoint < 127985) return 2;
    if (codepoint > 127984 && codepoint < 127988) return 1;
    if (codepoint > 127988 && codepoint < 127992) return 1;
    if (codepoint > 127991 && codepoint < 128063) return 2;
    if (codepoint > 128062 && codepoint < 128066) return 1;
    if (codepoint > 128065 && codepoint < 128253) return 2;
    if (codepoint > 128252 && codepoint < 128255) return 1;
    if (codepoint > 128254 && codepoint < 128318) return 2;
    if (codepoint > 128317 && codepoint < 128331) return 1;
    if (codepoint > 128330 && codepoint < 128335) return 2;
    if (codepoint > 128334 && codepoint < 128337) return 1;
    if (codepoint > 128336 && codepoint < 128360) return 2;
    if (codepoint > 128359 && codepoint < 128378) return 1;
    if (codepoint > 128378 && codepoint < 128405) return 1;
    if (codepoint > 128404 && codepoint < 128407) return 2;
    if (codepoint > 128406 && codepoint < 128420) return 1;
    if (codepoint > 128420 && codepoint < 128507) return 1;
    if (codepoint > 128506 && codepoint < 128592) return 2;
    if (codepoint > 128591 && codepoint < 128640) return 1;
    if (codepoint > 128639 && codepoint < 128710) return 2;
    if (codepoint > 128709 && codepoint < 128716) return 1;
    if (codepoint > 128716 && codepoint < 128720) return 1;
    if (codepoint > 128719 && codepoint < 128723) return 2;
    if (codepoint > 128722 && codepoint < 128725) return 1;
    if (codepoint > 128724 && codepoint < 128728) return 2;
    if (codepoint > 128731 && codepoint < 128736) return 2;
    if (codepoint > 128735 && codepoint < 128747) return 1;
    if (codepoint > 128746 && codepoint < 128749) return 2;
    if (codepoint > 128751 && codepoint < 128756) return 1;
    if (codepoint > 128755 && codepoint < 128765) return 2;
    if (codepoint > 128767 && codepoint < 128887) return 1;
    if (codepoint > 128890 && codepoint < 128986) return 1;
    if (codepoint > 128991 && codepoint < 129004) return 2;
    if (codepoint > 129023 && codepoint < 129036) return 1;
    if (codepoint > 129039 && codepoint < 129096) return 1;
    if (codepoint > 129103 && codepoint < 129114) return 1;
    if (codepoint > 129119 && codepoint < 129160) return 1;
    if (codepoint > 129167 && codepoint < 129198) return 1;
    if (codepoint > 129199 && codepoint < 129212) return 1;
    if (codepoint > 129215 && codepoint < 129218) return 1;
    if (codepoint > 129279 && codepoint < 129292) return 1;
    if (codepoint > 129291 && codepoint < 129339) return 2;
    if (codepoint > 129338 && codepoint < 129341) return 1;
    if (codepoint > 129340 && codepoint < 129350) return 2;
    if (codepoint > 129349 && codepoint < 129352) return 1;
    if (codepoint > 129351 && codepoint < 129536) return 2;
    if (codepoint > 129535 && codepoint < 129620) return 1;
    if (codepoint > 129631 && codepoint < 129646) return 1;
    if (codepoint > 129647 && codepoint < 129661) return 2;
    if (codepoint > 129663 && codepoint < 129674) return 2;
    if (codepoint > 129678 && codepoint < 129735) return 2;
    if (codepoint > 129741 && codepoint < 129757) return 2;
    if (codepoint > 129758 && codepoint < 129770) return 2;
    if (codepoint > 129775 && codepoint < 129785) return 2;
    if (codepoint > 129791 && codepoint < 129939) return 1;
    if (codepoint > 129939 && codepoint < 130042) return 1;
    if (codepoint > 194559 && codepoint < 195102) return 2;
    if (codepoint > 917535 && codepoint < 917632) return 1;
    if (codepoint > 917759 && codepoint < 918000) return 1;
    return 1;
}

uint8_t sfce_codepoint_utf8_continuation(uint8_t byte)
{
    return (byte & 0xC0) == 0x80;
}

uint8_t sfce_codepoint_encode_utf8(int32_t codepoint, uint8_t *bytes)
{
    if (codepoint < 0x00) {
        return 0;
    }

    if ((codepoint & 0xFFFFFF80) == 0) {
        bytes[0] = (uint8_t)codepoint;
        return 1;
    }

    if ((codepoint & 0xFFFFF800) == 0) {
        bytes[0] = 0xC0 | (uint8_t)((codepoint >>  6) & 0x1F);
        bytes[1] = 0x80 | (uint8_t)((codepoint      ) & 0x3F);
        return 2;
    }

    if ((codepoint & 0xFFFF0000) == 0) {
        bytes[0] = 0xE0 | (uint8_t)((codepoint >> 12) & 0x0F);
        bytes[1] = 0x80 | (uint8_t)((codepoint >>  6) & 0x3F);
        bytes[2] = 0x80 | (uint8_t)((codepoint      ) & 0x3F);
        return 3;
    }

    if ((codepoint & 0xFFE00000) == 0) {
        bytes[0] = 0xF0 | (uint8_t)((codepoint >> 18) & 0x07);
        bytes[1] = 0x80 | (uint8_t)((codepoint >> 12) & 0x3F);
        bytes[2] = 0x80 | (uint8_t)((codepoint >>  6) & 0x3F);
        bytes[3] = 0x80 | (uint8_t)((codepoint      ) & 0x3F);
        return 4;
    }

    return 0;
}

int32_t sfce_codepoint_decode_utf8(const void *buffer, int32_t buffer_size)
{
    if (buffer_size <= 0) {
        return -1;
    }

    const uint8_t *bytes = buffer;
    if ((bytes[0] & 0xF8) == 0xF0) {
        if ( buffer_size < 4
        ||  !sfce_codepoint_utf8_continuation(bytes[1])
        ||  !sfce_codepoint_utf8_continuation(bytes[2])
        ||  !sfce_codepoint_utf8_continuation(bytes[3])) {
            return -1;
        }

        // four-byte sequence
        return ((bytes[0] & 0x07) << 18)
        |      ((bytes[1] & 0x3F) << 12)
        |      ((bytes[2] & 0x3F) <<  6)
        |      ((bytes[3] & 0x3F)      );
    }
    else if ((bytes[0] & 0xF0) == 0xE0) {
        if ( buffer_size < 3
        ||  !sfce_codepoint_utf8_continuation(bytes[1])
        ||  !sfce_codepoint_utf8_continuation(bytes[2])) {
            return -1;
        }

        // three-byte sequence
        return ((bytes[0] & 0x0F) << 12)
        |      ((bytes[1] & 0x3F) <<  6)
        |      ((bytes[2] & 0x3F)      );
    }
    else if ((bytes[0] & 0xE0) == 0xC0) {
        if (buffer_size < 2 || !sfce_codepoint_utf8_continuation(bytes[1])) {
            return -1;
        }

        // two-byte sequence
        return ((bytes[0] & 0x1F) << 6)
        |      ((bytes[1] & 0x3F)     );
    }
    else if ((bytes[0] & 0x80) == 0x00) {
        // one-byte sequence
        return bytes[0] & 0x7F;
    }

    return -1;
}

uint8_t sfce_codepoint_utf8_byte_count(int32_t codepoint)
{
    if ((codepoint & 0xFFFFFF80) == 0) return 1;
    if ((codepoint & 0xFFFFF800) == 0) return 2;
    if ((codepoint & 0xFFFF0000) == 0) return 3;
    if ((codepoint & 0xFFE00000) == 0) return 4;
    return 0;
}

int8_t sfce_codepoint_is_print(int32_t codepoint)
{
    enum sfce_unicode_category category = sfce_codepoint_category(codepoint);

    switch (category) {
    case SFCE_UNICODE_CATEGORY_LL:
    case SFCE_UNICODE_CATEGORY_LM:
    case SFCE_UNICODE_CATEGORY_LO:
    case SFCE_UNICODE_CATEGORY_LT:
    case SFCE_UNICODE_CATEGORY_LU:
        return 1;
    case SFCE_UNICODE_CATEGORY_ND:
    case SFCE_UNICODE_CATEGORY_NL:
    case SFCE_UNICODE_CATEGORY_NO:
        return 1;
    case SFCE_UNICODE_CATEGORY_MC:
    case SFCE_UNICODE_CATEGORY_ME:
    case SFCE_UNICODE_CATEGORY_MN:
        return 1;
    case SFCE_UNICODE_CATEGORY_PC:
    case SFCE_UNICODE_CATEGORY_PD:
    case SFCE_UNICODE_CATEGORY_PE:
    case SFCE_UNICODE_CATEGORY_PF:
    case SFCE_UNICODE_CATEGORY_PI:
    case SFCE_UNICODE_CATEGORY_PO:
    case SFCE_UNICODE_CATEGORY_PS:
        return 1;
    case SFCE_UNICODE_CATEGORY_SC:
    case SFCE_UNICODE_CATEGORY_SK:
    case SFCE_UNICODE_CATEGORY_SM:
    case SFCE_UNICODE_CATEGORY_SO:
        return 1;
    case SFCE_UNICODE_CATEGORY_ZS:
        return 1;
    default:
        return 0;
    }

    return 0;
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

    if (!SetConsoleCP(CP_UTF8)) {
        return SFCE_ERROR_FAILED_WIN32_API_CALL;
    }

    if (!SetConsoleOutputCP(CP_UTF8)) {
        return SFCE_ERROR_FAILED_WIN32_API_CALL;
    }

    return SFCE_ERROR_OK;
}

void sfce_string_destroy(struct sfce_string *string)
{
    if (string->data) free(string->data);
    *string = (struct sfce_string){};
}

void sfce_string_clear(struct sfce_string *string)
{
    if (string != NULL) {
        string->size = 0;
    }
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
        return SFCE_ERROR_OUT_OF_MEMORY;
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

enum sfce_error_code sfce_string_push_back_byte(struct sfce_string *string, uint8_t byte)
{
    const int32_t size = string->size;
    enum sfce_error_code error_code = sfce_string_resize(string, string->size + 1);
    if (error_code != SFCE_ERROR_OK) {
        return error_code;
    }

    string->data[size] = (uint8_t)byte;
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

enum sfce_error_code sfce_string_push_back_codepoint(struct sfce_string *string, int32_t codepoint)
{
    uint8_t buffer[4] = {};
    int32_t buffer_size = sfce_codepoint_encode_utf8(codepoint, buffer);
    return sfce_string_push_back_buffer(string, buffer, buffer_size);
}

enum sfce_error_code sfce_string_nprintf(struct sfce_string *string, int32_t max_length, const void *format, ...)
{
    va_list va_args;
    va_start(va_args, format);

    enum sfce_error_code error_code = sfce_string_vnprintf(string, max_length, format, va_args);

    va_end(va_args);
    return error_code;
}

enum sfce_error_code sfce_string_vnprintf(struct sfce_string *string, int32_t max_length, const void *format, va_list va_args)
{
    int formatted_string_size = vsnprintf(NULL, 0, format, va_args);
    if (formatted_string_size < 0) {
        return SFCE_ERROR_BUFFER_OVERFLOW;
    }

    int32_t write_location = string->size;
    int32_t size_to_allocate = formatted_string_size > max_length ? max_length : formatted_string_size;

    enum sfce_error_code error_code = sfce_string_resize(string, string->size + size_to_allocate + 1);
    if (error_code != SFCE_ERROR_OK) {
        return error_code;
    }

    formatted_string_size = vsnprintf((char *)&string->data[write_location], string->size - write_location, format, va_args);
    if (formatted_string_size < 0) {
        return SFCE_ERROR_BUFFER_OVERFLOW;
    }

    // Remove the null terminating character
    string->size = string->size - 1;
    return SFCE_ERROR_OK;
}

enum sfce_error_code sfce_string_to_upper_case(const struct sfce_string *string, struct sfce_string *result_string)
{
    sfce_string_clear(result_string);

    int32_t idx = 0;
    while (idx < string->size) {
        int32_t codepoint = sfce_codepoint_decode_utf8(&string->data[idx], string->size - idx);
        int32_t codepoint_size = sfce_codepoint_utf8_byte_count(codepoint);
        enum sfce_error_code error_code = sfce_string_push_back_codepoint(result_string, sfce_codepoint_to_upper(codepoint));

        if (error_code != SFCE_ERROR_OK) {
            return error_code;
        }

        idx += codepoint_size;
    }

    return SFCE_ERROR_OK;
}

enum sfce_error_code sfce_string_to_lower_case(const struct sfce_string *string, struct sfce_string *result_string)
{
    sfce_string_clear(result_string);

    int32_t idx = 0;
    while (idx < string->size) {
        int32_t codepoint = sfce_codepoint_decode_utf8(&string->data[idx], string->size - idx);
        int32_t codepoint_size = sfce_codepoint_utf8_byte_count(codepoint);
        enum sfce_error_code error_code = sfce_string_push_back_codepoint(result_string, sfce_codepoint_to_lower(codepoint));

        if (error_code != SFCE_ERROR_OK) {
            return error_code;
        }

        idx += codepoint_size;
    }

    return SFCE_ERROR_OK;
}

enum sfce_error_code sfce_string_to_snake_case(const struct sfce_string *string, struct sfce_string *result_string);
enum sfce_error_code sfce_string_to_kebab_case(const struct sfce_string *string, struct sfce_string *result_string);
enum sfce_error_code sfce_string_to_title_case(const struct sfce_string *string, struct sfce_string *result_string);
enum sfce_error_code sfce_string_to_camel_case(const struct sfce_string *string, struct sfce_string *result_string);
enum sfce_error_code sfce_string_to_pascal_case(const struct sfce_string *string, struct sfce_string *result_string);

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
            return SFCE_ERROR_OUT_OF_MEMORY;
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
    enum sfce_error_code error_code = sfce_line_starts_resize(lines, lines->count + 1);
    if (error_code != SFCE_ERROR_OK) {
        return error_code;
    }

    lines->offsets[lines->count - 1] = offset;
    return SFCE_ERROR_OK;
}

struct sfce_buffer_position sfce_line_starts_search_for_position(struct sfce_line_starts line_starts, int32_t line_low_index, int32_t line_high_index, int32_t offset)
{
    int32_t line_middle_index = 0;

    while (line_low_index <= line_high_index) {
        line_middle_index = line_low_index + (line_high_index - line_low_index) / 2;

        if (line_middle_index == line_high_index) {
            break;
        }

        int32_t line_middle_offset = line_starts.offsets[line_middle_index];
        int32_t line_middle_end_offset = line_starts.offsets[line_middle_index + 1];

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

    int32_t line_start_offset = line_starts.offsets[line_middle_index];
    return (struct sfce_buffer_position) {
        .line_start_index = line_middle_index,
        .column = offset - line_start_offset,
    };
}

void sfce_string_buffer_destroy(struct sfce_string_buffer *buffer)
{
    sfce_line_starts_destroy(&buffer->line_starts);
    sfce_string_destroy(&buffer->content);
    *buffer = (struct sfce_string_buffer){};
}

enum sfce_error_code sfce_string_buffer_append_content(struct sfce_string_buffer *buffer, const uint8_t *data, int32_t size)
{
    int32_t offset_begin = buffer->content.size;
    enum sfce_error_code error_code = sfce_string_push_back_buffer(&buffer->content, data, size);
    if (error_code != SFCE_ERROR_OK) {
        return error_code;
    }

    return sfce_string_buffer_recount_line_start_offsets(buffer, offset_begin, buffer->content.size);
}

enum sfce_error_code sfce_string_buffer_recount_line_start_offsets(struct sfce_string_buffer *buffer, int32_t offset_begin, int32_t offset_end)
{
    enum sfce_error_code error_code = SFCE_ERROR_OK;

    for (int32_t offset = offset_begin; offset < offset_end;) {
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

struct sfce_buffer_position sfce_string_buffer_get_end_position(struct sfce_string_buffer *buffer)
{
    struct sfce_buffer_position position = {};
    position.line_start_index = buffer->line_starts.count - 1;
    position.column = buffer->content.size - buffer->line_starts.offsets[position.line_start_index];
    return position;
}

struct sfce_buffer_position sfce_string_buffer_offset_to_position(struct sfce_string_buffer *buffer, int32_t offset)
{
    return sfce_line_starts_search_for_position(buffer->line_starts, 0, buffer->line_starts.count - 1, offset);
}

struct sfce_buffer_position sfce_string_buffer_piece_position_in_buffer(struct sfce_string_buffer *buffer, struct sfce_piece piece, int32_t offset_within_piece)
{
    int32_t line_low_index = piece.start.line_start_index;
    int32_t line_high_index = piece.end.line_start_index;
    int32_t offset = buffer->line_starts.offsets[piece.start.line_start_index] + piece.start.column + offset_within_piece;
    return sfce_line_starts_search_for_position(buffer->line_starts, line_low_index, line_high_index, offset);
}

int32_t sfce_string_buffer_line_number_offset_within_piece(struct sfce_string_buffer *string_buffer, struct sfce_piece piece, int32_t lines_within_piece)
{
    if (lines_within_piece <= 0) {
        return 0;
    }

    struct sfce_line_starts line_starts = string_buffer->line_starts;
    int32_t line_number_within_buffer = piece.start.line_start_index + lines_within_piece;

    if (line_number_within_buffer > piece.end.line_start_index) {
        return piece.length;
    }

    int32_t start_offset = line_starts.offsets[piece.start.line_start_index] + piece.start.column;
    return line_starts.offsets[line_number_within_buffer] - start_offset;
}

struct sfce_buffer_position sfce_string_buffer_move_position_by_offset(struct sfce_string_buffer *buffer, struct sfce_buffer_position position, int32_t offset)
{
    offset = sfce_string_buffer_position_to_offset(buffer, position) + offset;
    return sfce_string_buffer_offset_to_position(buffer, offset);
}

struct sfce_buffer_position _sfce_string_buffer_move_position_by_offset(struct sfce_string_buffer *buffer, struct sfce_buffer_position position, int32_t offset)
{
    int32_t offset_within_buffer = buffer->line_starts.offsets[position.line_start_index] + position.column + offset;

    if (offset_within_buffer <= 0) {
        return (struct sfce_buffer_position) { 0, 0 };
    }

    if (offset_within_buffer >= buffer->content.size) {
        return sfce_string_buffer_get_end_position(buffer);
    }

    while (1) {
        if (offset_within_buffer < buffer->line_starts.offsets[position.line_start_index]) {
            position.line_start_index -= 1;
        }
        else if (position.line_start_index + 1 >= buffer->line_starts.count) {
            break;
        }
        else if (offset_within_buffer >= buffer->line_starts.offsets[position.line_start_index + 1]) {
            position.line_start_index += 1;
        }
        else {
            break;
        }
    }

    position.column = offset_within_buffer - buffer->line_starts.offsets[position.line_start_index];
    return position;
}

int32_t sfce_string_buffer_position_to_offset(struct sfce_string_buffer *string_buffer, struct sfce_buffer_position position)
{
    return string_buffer->line_starts.offsets[position.line_start_index] + position.column;
}

struct sfce_piece_node *sfce_piece_node_create(struct sfce_piece piece)
{
    struct sfce_piece_node *node = malloc(sizeof *node);

    if (node == NULL) {
        return NULL;
    }

    *node = (struct sfce_piece_node) {
        .left = sentinel_ptr,
        .right = sentinel_ptr,
        .parent = sentinel_ptr,
        .piece = piece,
        .color = SFCE_COLOR_BLACK,
    };

    return node;
}

void sfce_piece_node_destroy(struct sfce_piece_node *node)
{
    if (node != sentinel_ptr && node != NULL) {
        sfce_piece_node_destroy(node->left);
        sfce_piece_node_destroy(node->right);
        sfce_piece_node_destroy_non_recursive(node);
    }
}

void sfce_piece_node_destroy_non_recursive(struct sfce_piece_node *node)
{
    if (node != sentinel_ptr && node != NULL) {
        free(node);
    }
}

int32_t sfce_piece_node_calculate_length(struct sfce_piece_node *node)
{
    int32_t length = 0;
    while (node != sentinel_ptr) {
        length += node->left_subtree_length + node->piece.length;
        node = node->right;
    }

    return length;
}

int32_t sfce_piece_node_calculate_line_count(struct sfce_piece_node *node)
{
    int32_t line_count = 0;
    while (node != sentinel_ptr) {
        line_count += node->left_subtree_line_count + node->piece.line_count;
        node = node->right;
    }

    return line_count;
}

int32_t sfce_piece_node_offset_from_start(struct sfce_piece_node *node)
{
    int32_t node_start_offset = node->left_subtree_length;
    while (node->parent != sentinel_ptr) {
        if (node->parent->right == node) {
            node_start_offset += node->parent->left_subtree_length + node->parent->piece.length;
        }

        node = node->parent;
    }

    return node_start_offset;
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

void sfce_piece_node_remove_node(struct sfce_piece_node **root, struct sfce_piece_node *z)
{
    enum sfce_red_black_color original_color = z->color;
    struct sfce_piece_node *x = sentinel_ptr;
    struct sfce_piece_node *y = z;

    if (z->left == sentinel_ptr) {
        x = z->right;
        sfce_piece_node_transplant(root, y, x);
        sfce_piece_node_recompute_metadata(root, x);
    }
    else if (z->right == sentinel_ptr) {
        x = z->left;
        sfce_piece_node_transplant(root, y, x);
        sfce_piece_node_recompute_metadata(root, x);
    }
    else {
        y = sfce_piece_node_leftmost(z->right);
        x = y->right;
        original_color = y->color;

        if (y->parent == z) {
            x->parent = y;
            sfce_piece_node_recompute_metadata(root, x);
        }
        else {
            sfce_piece_node_transplant(root, y, y->right);
            y->right = z->right;
            y->right->parent = y;

            sfce_piece_node_recompute_metadata(root, y->right);
        }

        sfce_piece_node_transplant(root, z, y);
        sfce_piece_node_recompute_metadata(root, y);

        y->left = z->left;
        y->left->parent = y;
        y->color = z->color;

        sfce_piece_node_recompute_metadata(root, y->left);
    }

    if (x->parent->left == x) {
        int32_t left_length = sfce_piece_node_calculate_length(x);
        int32_t left_line_count = sfce_piece_node_calculate_line_count(x);
        sfce_piece_node_update_metadata(
            root,
            x->parent,
            left_length - x->parent->left_subtree_length,
            left_line_count - x->parent->left_subtree_line_count
        );
    }

    sfce_piece_node_recompute_metadata(root, x->parent);

    if (original_color == SFCE_COLOR_BLACK) {
        sfce_piece_node_fix_remove_violation(root, x);
    }

    sfce_piece_node_destroy_non_recursive(z);
    sfce_piece_node_reset_sentinel();
}

void sfce_piece_node_transplant(struct sfce_piece_node **root, struct sfce_piece_node *where, struct sfce_piece_node *node_to_transplant)
{
    if (where == *root || where->parent == sentinel_ptr) {
        *root = node_to_transplant;
    }
    else if (where == where->parent->left) {
        where->parent->left = node_to_transplant;
    }
    else {
        where->parent->right = node_to_transplant;
    }

    node_to_transplant->parent = where->parent;
}

void sfce_piece_node_update_metadata(struct sfce_piece_node **root, struct sfce_piece_node *node, int32_t delta_length, int32_t delta_line_count)
{
    if (node == sentinel_ptr || (delta_length == 0 && delta_line_count == 0)) {
        return;
    }

    node->left_subtree_length += delta_length;
    node->left_subtree_line_count += delta_line_count;

    while (node != *root && node != sentinel_ptr) {
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

void sfce_piece_node_fix_remove_violation(struct sfce_piece_node **root, struct sfce_piece_node *node)
{
    struct sfce_piece_node *s;
    node->color = SFCE_COLOR_BLACK;
    while (node != *root && node->color == SFCE_COLOR_BLACK) {
        if (node == node->parent->left) {
            s = node->parent->right;

            if (s->color == SFCE_COLOR_RED) {
                s->color = SFCE_COLOR_BLACK;
                node->parent->color = SFCE_COLOR_RED;
                sfce_piece_node_rotate_left(root, node->parent);
                s = node->parent->right;
            }

            if (s->left->color == SFCE_COLOR_BLACK && s->right->color == SFCE_COLOR_BLACK) {
                s->color = SFCE_COLOR_RED;
                node = node->parent;
            }
            else {
                if (s->right->color == SFCE_COLOR_BLACK) {
                    s->left->color = SFCE_COLOR_BLACK;
                    s->color = SFCE_COLOR_RED;
                    sfce_piece_node_rotate_right(root, s);
                    s = node->parent->right;
                }

                s->color = node->parent->color;
                node->parent->color = SFCE_COLOR_BLACK;
                s->right->color = SFCE_COLOR_BLACK;
                sfce_piece_node_rotate_left(root, node->parent);
                node = *root;
            }
        }
        else {
            s = node->parent->left;

            if (s->color == SFCE_COLOR_RED) {
                s->color = SFCE_COLOR_BLACK;
                node->parent->color = SFCE_COLOR_RED;
                sfce_piece_node_rotate_right(root, node->parent);
                s = node->parent->left;
            }

            if (s->right->color == SFCE_COLOR_BLACK && s->right->color == SFCE_COLOR_BLACK) {
                s->color = SFCE_COLOR_RED;
                node = node->parent;
            }
            else {
                if (s->left->color == SFCE_COLOR_BLACK) {
                    s->right->color = SFCE_COLOR_BLACK;
                    s->color = SFCE_COLOR_RED;
                    sfce_piece_node_rotate_left(root, s);
                    s = node->parent->left;
                }

                s->color = node->parent->color;
                node->parent->color = SFCE_COLOR_BLACK;
                s->left->color = SFCE_COLOR_BLACK;
                sfce_piece_node_rotate_right(root, node->parent);
                node = *root;
            }
        }
    }

    node->color = SFCE_COLOR_BLACK;
    sfce_piece_node_reset_sentinel();
}

void sfce_piece_node_recompute_piece_length(struct sfce_piece_tree *tree, struct sfce_piece_node *node)
{
    struct sfce_string_view content = sfce_piece_tree_get_piece_content(tree, node->piece);
    node->piece.line_count = buffer_newline_count(content.data, content.size);
    node->piece.length = content.size;

    sfce_piece_node_recompute_metadata(&tree->root, node);
}

void sfce_piece_node_inorder_print(struct sfce_piece_tree *tree, struct sfce_piece_node *root)
{
    if (root == sentinel_ptr) {
        return;
    }

    sfce_piece_node_inorder_print(tree, root->left);

    struct sfce_string_view content = sfce_piece_tree_get_piece_content(tree, root->piece);
    printf("%.*s", content.size, content.data);

    sfce_piece_node_inorder_print(tree, root->right);
}

void sfce_piece_node_to_string(struct sfce_piece_tree *tree, struct sfce_piece_node *node, int32_t space, struct sfce_string *out)
{
    static const char *const node_color_list[] = {
        [SFCE_COLOR_BLACK] = "node(BLACK %p): '",
        [SFCE_COLOR_RED] = "node(RED %p): '",
    };

    enum { COUNT = 4 };

    if (node == sentinel_ptr) {
        return;
    }

    sfce_piece_node_to_string(tree, node->right, space + COUNT, out);

    struct sfce_string_view piece_content = sfce_piece_tree_get_piece_content(tree, node->piece);

    for (int32_t idx = 0; idx < space; ++idx) {
        sfce_string_push_back_byte(out, ' ');
    }

    sfce_string_nprintf(out, INT32_MAX, node_color_list[node->color], (void *)node);

    if (node->piece.length > 100) {
        sfce_string_nprintf(out, INT32_MAX, "...");
    }
    else {
        for (int32_t idx = 0; idx < piece_content.size; ++idx) {
            int32_t character = piece_content.data[idx];
            const char *buffer = make_character_printable(character);
            sfce_string_nprintf(out, INT32_MAX, buffer);
        }
    }

    sfce_string_nprintf(
        out,
        INT32_MAX,
        "' length: %d, line_count: %d | left_length: %d, left_line_count: %d\n",
        node->piece.length,
        node->piece.line_count,
        node->left_subtree_length,
        node->left_subtree_line_count
    );

    sfce_piece_node_to_string(tree, node->left, space + COUNT, out);
}

void sfce_piece_node_inorder_print_to_string(struct sfce_piece_tree *tree, struct sfce_piece_node *root, struct sfce_string *out)
{
    if (root == sentinel_ptr) {
        return;
    }

    sfce_piece_node_inorder_print(tree, root->left);

    struct sfce_string_view content = sfce_piece_tree_get_piece_content(tree, root->piece);
    sfce_string_nprintf(out, INT32_MAX, "%.*s", content.size, content.data);

    sfce_piece_node_inorder_print(tree, root->right);
}

void sfce_piece_node_print(struct sfce_piece_tree *tree, struct sfce_piece_node *node, int32_t space)
{
    static const char *const node_color_list[] = {
        [SFCE_COLOR_BLACK] = "node(BLACK): '",
        [SFCE_COLOR_RED] = "node(RED): '",
    };

    enum { COUNT = 4 };

    if (node == sentinel_ptr) {
        return;
    }

    sfce_piece_node_print(tree, node->right, space + COUNT);

    struct sfce_string_view piece_content = sfce_piece_tree_get_piece_content(tree, node->piece);

    for (int32_t idx = 0; idx < space; ++idx) {
        sfce_write(" ", 1);
    }

    sfce_write_zero_terminated_string(node_color_list[node->color]);

    for (int32_t idx = 0; idx < piece_content.size; ++idx) {
        int32_t character = piece_content.data[idx];
        const char *buffer = make_character_printable(character);
        sfce_write_zero_terminated_string(buffer);
    }

    printf("' length: %d, line_count: %d\n", node->piece.length, node->piece.line_count);

    sfce_piece_node_print(tree, node->left, space + COUNT);
}

void sfce_piece_node_reset_sentinel()
{
    sentinel = (struct sfce_piece_node) {
        .parent = &sentinel,
        .left   = &sentinel,
        .right  = &sentinel,
        .color  = SFCE_COLOR_BLACK,
    };
}

struct sfce_node_position sfce_node_position_move_by_offset(struct sfce_node_position position, int32_t offset)
{
    position.offset_within_piece += offset;

    while (position.node != sentinel_ptr) {
        if (position.offset_within_piece > position.node->piece.length) {
            struct sfce_piece_node *next = sfce_piece_node_next(position.node);
            if (next == sentinel_ptr) {
                return (struct sfce_node_position) {
                    .node = position.node,
                    .node_start_offset = position.node_start_offset,
                    .offset_within_piece = position.node->piece.length,
                };
            }

            position.offset_within_piece -= position.node->piece.length;
            position.node_start_offset += position.node->piece.length;
            position.node = next;
            continue;
        }

        if (position.offset_within_piece < 0) {
            struct sfce_piece_node *prev = sfce_piece_node_prev(position.node);
            if (prev == sentinel_ptr) {
                return (struct sfce_node_position) {
                    .node = position.node,
                    .node_start_offset = 0,
                    .offset_within_piece = 0,
                };
            }

            position.offset_within_piece += position.node->piece.length;
            position.node_start_offset -= position.node->piece.length;
            position.node = prev;
            continue;
        }

        return position;
    }

    return sentinel_node_position;
}

struct sfce_piece_tree *sfce_piece_tree_create()
{
    enum sfce_error_code error_code = SFCE_ERROR_OK;
    struct sfce_string_buffer string_buffer = {};

    error_code = sfce_line_starts_push_line_offset(&string_buffer.line_starts, 0);
    if (error_code != SFCE_ERROR_OK) {
        return NULL;
    }

    struct sfce_piece_tree *tree = malloc(sizeof *tree);

    if (tree != NULL) {
        *tree = (struct sfce_piece_tree) {
            .root = sentinel_ptr,
            .length = 0,
            .line_count = 1,
            .change_buffer_index = 0,
        };

        error_code = sfce_piece_tree_add_string_buffer(tree, string_buffer);
        if (error_code != SFCE_ERROR_OK) {
            sfce_string_buffer_destroy(&string_buffer);
            free(tree);
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

int32_t sfce_piece_tree_line_offset_in_piece(struct sfce_piece_tree *tree, struct sfce_piece piece, int32_t lines_within_piece)
{
    if (lines_within_piece <= 0) {
        return 0;
    }

    struct sfce_line_starts line_starts = tree->buffers[piece.buffer_index].line_starts;
    int32_t line_number_within_buffer = piece.start.line_start_index + lines_within_piece;

    if (line_number_within_buffer > piece.end.line_start_index) {
        return piece.length;
    }

    int32_t start_offset = line_starts.offsets[piece.start.line_start_index] + piece.start.column;
    return line_starts.offsets[line_number_within_buffer] - start_offset;
}

int32_t sfce_piece_tree_count_lines_in_piece_until_offset(struct sfce_piece_tree *tree, struct sfce_piece piece, int32_t offset_within_piece)
{
    struct sfce_line_starts line_starts = tree->buffers[piece.buffer_index].line_starts;
    int32_t line_low_index = piece.start.line_start_index;
    int32_t line_high_index = piece.end.line_start_index;
    int32_t offset = line_starts.offsets[piece.start.line_start_index] + piece.start.column + offset_within_piece;

    struct sfce_buffer_position position = sfce_line_starts_search_for_position(
        line_starts, line_low_index, line_high_index, offset
    );

    return position.line_start_index - piece.start.line_start_index;
}

int32_t sfce_piece_tree_offset_at_position(struct sfce_piece_tree *tree, const struct sfce_position position)
{
    struct sfce_piece_node *node = tree->root;
    int32_t node_start_offset = 0;
    int32_t subtree_line_count = position.row;

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
            node_start_offset += node->left_subtree_length;
            int32_t lines_within_piece = subtree_line_count - node->left_subtree_line_count;
            int32_t line_offset0 = sfce_piece_tree_line_offset_in_piece(tree, node->piece, lines_within_piece);
            return node_start_offset + line_offset0 + position.col;
        }
    }

    return node_start_offset;
}

// int32_t sfce_piece_tree_offset_at_position(struct sfce_piece_tree *tree, const struct sfce_position position)
// {
//     struct sfce_node_position node_position = sfce_piece_tree_node_at_position(tree, position.col, position.row);
//     return node_position.node_start_offset + node_position.offset_within_piece;
// }

#if 0
int32_t sfce_piece_tree_get_codepoint_from_node_position(struct sfce_piece_tree *tree, struct sfce_node_position node_position)
{
    struct sfce_string_view piece_content = sfce_piece_tree_get_piece_content(tree, node_position.node->piece);

    int32_t codepoint = sfce_codepoint_decode_utf8(
        &piece_content.data[node_position.offset_within_piece],
        piece_content.size - node_position.offset_within_piece
    );

    return codepoint;
}
#endif

int32_t sfce_piece_tree_get_codepoint_from_node_position(struct sfce_piece_tree *tree, struct sfce_node_position start)
{
    uint8_t bytes[4] = {};
    struct sfce_node_position end = sfce_node_position_move_by_offset(start, 4);
    int32_t length = sfce_piece_tree_read_into_buffer(tree, start, end, 4, bytes);
    int32_t codepoint = sfce_codepoint_decode_utf8(bytes, length);

    

    return codepoint;
}

int32_t sfce_piece_tree_get_character_length_from_node_position(struct sfce_piece_tree *tree, struct sfce_node_position start)
{
    uint8_t bytes[4] = {};
    struct sfce_node_position end = sfce_node_position_move_by_offset(start, 4);
    int32_t length = sfce_piece_tree_read_into_buffer(tree, start, end, 4, bytes);

    int32_t newline_size = newline_sequence_size(bytes, 4);
    if (newline_size != 0) return newline_size;

    int32_t codepoint = sfce_codepoint_decode_utf8(bytes, length);
    return sfce_codepoint_utf8_byte_count(codepoint);
}

int32_t sfce_piece_tree_get_line_length(struct sfce_piece_tree *tree, int32_t row)
{
    int32_t offset0 = sfce_piece_tree_offset_at_position(tree, (struct sfce_position) { 0, row });
    int32_t offset1 = sfce_piece_tree_offset_at_position(tree, (struct sfce_position) { 0, row + 1 });
    return offset1 - offset0;
}

uint8_t sfce_piece_tree_get_byte_at_node_position(struct sfce_piece_tree *tree, struct sfce_node_position node_position)
{
    struct sfce_string_view piece_content = sfce_piece_tree_get_piece_content(tree, node_position.node->piece);
    return piece_content.data[node_position.offset_within_piece];
}

int32_t sfce_piece_tree_read_into_buffer(struct sfce_piece_tree *tree, struct sfce_node_position start, struct sfce_node_position end, int32_t buffer_size, uint8_t *buffer)
{
    int32_t bytes_written = 0;
    struct sfce_string_view piece_content = {};

    if (start.node == end.node) {
        bytes_written = end.offset_within_piece - start.offset_within_piece;
        piece_content = sfce_piece_tree_get_piece_content(tree, start.node->piece);
        memcpy(buffer, &piece_content.data[start.offset_within_piece], bytes_written);
        return bytes_written;
    }
    else {
        int32_t remaining = buffer_size;
        int32_t size = start.node->piece.length - start.offset_within_piece;
        size = MIN(remaining, size);

        piece_content = sfce_piece_tree_get_piece_content(tree, start.node->piece);
        memcpy(buffer, &piece_content.data[start.offset_within_piece], size);
        remaining -= size;
        bytes_written += size;

        for (struct sfce_piece_node *node = sfce_piece_node_next(start.node); node != end.node && node != sentinel_ptr;) {
            piece_content = sfce_piece_tree_get_piece_content(tree, node->piece);

            size = MIN(remaining, piece_content.size);
            memcpy(buffer + bytes_written, piece_content.data, size);
            remaining -= size;
            bytes_written += size;

            node = sfce_piece_node_next(node);
        }

        piece_content = sfce_piece_tree_get_piece_content(tree, end.node->piece);

        size = MIN(remaining, end.offset_within_piece);
        memcpy(buffer + bytes_written, piece_content.data, size);
        remaining -= size;
        bytes_written += size;

        memcpy(buffer + bytes_written, &piece_content.data[0], end.offset_within_piece);
        return bytes_written;
    }
}

// 
// TODO: Could be more optimized
// 
int32_t sfce_piece_tree_get_column_from_render_column(struct sfce_piece_tree *tree, int32_t row, int32_t target_render_col)
{
    int32_t line_length = sfce_piece_tree_get_line_length(tree, row);
    for (int32_t offset = 0, render_width = 0; offset < line_length;) {
        struct sfce_node_position node_position = sfce_piece_tree_node_at_position(tree, offset, row);
        int32_t codepoint = sfce_piece_tree_get_codepoint_from_node_position(tree, node_position);
        int32_t byte_count = sfce_codepoint_utf8_byte_count(codepoint);
        int32_t codepoint_width = sfce_codepoint_width(codepoint);

        if (render_width + codepoint_width > target_render_col) {
            return offset;
        }

        render_width += codepoint_width;
        offset += byte_count;
    }

    return line_length;
}

int32_t sfce_piece_tree_get_render_column_from_column(struct sfce_piece_tree *tree, int32_t row, int32_t col)
{
    int32_t render_column = 0;
    return render_column;
}

struct sfce_string_view sfce_piece_tree_get_piece_content(const struct sfce_piece_tree *tree, struct sfce_piece piece)
{
    struct sfce_string_buffer *string_buffer = &tree->buffers[piece.buffer_index];
    int32_t offset0 = sfce_string_buffer_position_to_offset(string_buffer, piece.start);
    int32_t offset1 = sfce_string_buffer_position_to_offset(string_buffer, piece.end);

    return (struct sfce_string_view) {
        .data = &string_buffer->content.data[offset0],
        .size = offset1 - offset0,
    };
}

enum sfce_error_code sfce_piece_tree_get_node_content(const struct sfce_piece_tree *tree, struct sfce_piece_node *node, struct sfce_string *string)
{
    if (node == sentinel_ptr) {
        return SFCE_ERROR_OK;
    }

    enum sfce_error_code error_code;

    error_code = sfce_piece_tree_get_node_content(tree, node->left, string);

    if (error_code != SFCE_ERROR_OK) {
        return error_code;
    }

    struct sfce_string_view piece_content = sfce_piece_tree_get_piece_content(tree, node->piece);
    error_code = sfce_string_push_back_buffer(string, piece_content.data, piece_content.size);

    if (error_code != SFCE_ERROR_OK) {
        return error_code;
    }

    error_code = sfce_piece_tree_get_node_content(tree, node->right, string);

    if (error_code != SFCE_ERROR_OK) {
        return error_code;
    }

    return SFCE_ERROR_OK;
}

struct sfce_position sfce_piece_tree_position_at_offset(struct sfce_piece_tree *tree, int32_t offset)
{
    struct sfce_piece_node *node = tree->root;
    int32_t node_start_line_count = 0;
    int32_t subtree_offset = CLAMP(offset, 0, tree->length);

    while (node != sentinel_ptr) {
        if (node->left_subtree_length != 0 && subtree_offset <= node->left_subtree_length) {
            node = node->left;
        }
        else if (node->right != sentinel_ptr && subtree_offset > node->left_subtree_length + node->piece.length) {
            node_start_line_count += node->left_subtree_line_count + node->piece.line_count;
            subtree_offset -= node->left_subtree_length + node->piece.length;
            node = node->right;
        }
        else {
            node_start_line_count += node->left_subtree_line_count;
            int32_t offset_within_piece = subtree_offset - node->left_subtree_length;
            int32_t lines_within_piece = sfce_piece_tree_count_lines_in_piece_until_offset(tree, node->piece, offset_within_piece);

            struct sfce_position position = {
                .row = node_start_line_count + lines_within_piece,
                .col = 0,
            };

            int32_t current_line_start_offset = sfce_piece_tree_offset_at_position(tree, position);

            return (struct sfce_position) {
                .row = node_start_line_count + lines_within_piece,
                .col = offset - current_line_start_offset,
            };
        }
    }

    return (struct sfce_position) { 0, 0 };
}

// 
// TODO: Implement the sfce_piece_tree_move_position_by_offset function
// which increments the input position be the offset provided to the function.
// 
struct sfce_position sfce_piece_tree_move_position_by_offset(struct sfce_piece_tree *tree, struct sfce_position position, int32_t offset)
{
    return (struct sfce_position) {};
}

struct sfce_node_position sfce_piece_tree_node_at_offset(struct sfce_piece_tree *tree, int32_t offset)
{
    struct sfce_node_position position = { .node = tree->root };
    int32_t subtree_offset = offset;

    while (position.node != sentinel_ptr) {
        if (position.node->left != sentinel_ptr && subtree_offset <= position.node->left_subtree_length) {
            position.node = position.node->left;
        }
        else if (position.node->right != sentinel_ptr && subtree_offset > position.node->left_subtree_length + position.node->piece.length) {
            position.node_start_offset += position.node->left_subtree_length + position.node->piece.length;
            subtree_offset -= position.node->left_subtree_length + position.node->piece.length;
            position.node = position.node->right;
        }
        else {
            position.node_start_offset += position.node->left_subtree_length;
            position.offset_within_piece = subtree_offset - position.node->left_subtree_length;
            position.offset_within_piece = CLAMP(position.offset_within_piece, 0, position.node->piece.length);
            return position;
        }
    }

    return sentinel_node_position;
}

struct sfce_node_position sfce_piece_tree_node_at_position(struct sfce_piece_tree *tree, int32_t col, int32_t row)
{
    int32_t node_start_offset = 0;
    struct sfce_piece_node *node = tree->root;

    while (node != sentinel_ptr) {
        if (node->left != sentinel_ptr && node->left_subtree_line_count >= row) {
            node = node->left;
        }
        else if (node->left_subtree_line_count + node->piece.line_count > row) {
            int32_t line_offset_begin = sfce_piece_tree_line_offset_in_piece(tree, node->piece, row - node->left_subtree_line_count);
            int32_t line_offset_end = sfce_piece_tree_line_offset_in_piece(tree, node->piece, row - node->left_subtree_line_count + 1);

            node_start_offset += node->left_subtree_length;
            return (struct sfce_node_position) {
                .node = node,
                .offset_within_piece = min(line_offset_begin + col, line_offset_end),
                .node_start_offset = node_start_offset,
            };
        }
        else if (node->left_subtree_line_count + node->piece.line_count == row) {
            int32_t line_offset_begin = sfce_piece_tree_line_offset_in_piece(tree, node->piece, row - node->left_subtree_line_count);
            node_start_offset += node->left_subtree_length;

            if (line_offset_begin + col <= node->piece.length) {
                return (struct sfce_node_position) {
                    .node = node,
                    .offset_within_piece = line_offset_begin + col,
                    .node_start_offset = node_start_offset,
                };
            }

            col -= node->piece.length - line_offset_begin;

            node_start_offset += node->piece.length;
            node = sfce_piece_node_next(node);
            while (node != sentinel_ptr) {
                if (node->piece.line_count > 0) {
                    int32_t line_offset_end = sfce_piece_tree_line_offset_in_piece(tree, node->piece, 1);
                    return (struct sfce_node_position) {
                        .node = node,
                        .offset_within_piece = min(col, line_offset_end),
                        .node_start_offset = node_start_offset,
                    };
                }
                else if (node->piece.length >= col) {
                    return (struct sfce_node_position) {
                        .node = node,
                        .offset_within_piece = col,
                        .node_start_offset = node_start_offset,
                    };
                }

                col -= node->piece.length;
                node_start_offset += node->piece.length;
                node = sfce_piece_node_next(node);
            }

            return sentinel_node_position;
        }
        else {
            if (node->right == sentinel_ptr) {
                node_start_offset += node->left_subtree_length;
                return (struct sfce_node_position) {
                    .node = node,
                    .offset_within_piece = node->piece.length,
                    .node_start_offset = node_start_offset,
                };
            }

            row -= node->left_subtree_line_count + node->piece.line_count;
            node_start_offset += node->left_subtree_length + node->piece.length;
            node = node->right;
        }
    }

    return sentinel_node_position;
}

enum sfce_error_code sfce_piece_tree_get_substring(struct sfce_piece_tree *tree, int32_t offset, int32_t length, struct sfce_string *string)
{
    struct sfce_node_position position0 = sfce_piece_tree_node_at_offset(tree, offset);
    struct sfce_node_position position1 = sfce_piece_tree_node_at_offset(tree, offset + length);
    return sfce_piece_tree_get_content_between_node_positions(tree, position0, position1, string);
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
    enum sfce_error_code error_code = SFCE_ERROR_OK;
    struct sfce_string_buffer *string_buffer = &tree->buffers[tree->change_buffer_index];
    int32_t remaining_size = SFCE_STRING_BUFFER_SIZE_THRESHOLD - string_buffer->content.size;

    if (remaining_size < required_size) {
        tree->change_buffer_index = tree->buffer_count;

        struct sfce_string_buffer string_buffer = {};
        error_code = sfce_line_starts_push_line_offset(&string_buffer.line_starts, 0);
        if (error_code != SFCE_ERROR_OK) {
            return error_code;
        }

        error_code = sfce_piece_tree_add_string_buffer(tree, string_buffer);
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
            return SFCE_ERROR_OUT_OF_MEMORY;
        }
    }

    tree->buffer_count = buffer_count;
    return SFCE_ERROR_OK;
}

enum sfce_error_code sfce_piece_tree_add_string_buffer(struct sfce_piece_tree *tree, struct sfce_string_buffer string_buffer)
{
    enum sfce_error_code error_code = sfce_piece_tree_set_buffer_count(tree, tree->buffer_count + 1);

    if (error_code != SFCE_ERROR_OK) {
        return error_code;
    }

    tree->buffers[tree->buffer_count - 1] = string_buffer;
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

enum sfce_error_code sfce_piece_tree_create_node_subtree(struct sfce_piece_tree *tree, const uint8_t *buffer, int32_t buffer_size, struct sfce_piece_node **result)
{
    enum sfce_error_code error_code;
    struct sfce_piece_node *root = sentinel_ptr;
    struct sfce_piece_node *rightmost_node = sentinel_ptr;

    const uint8_t *buffer_end = buffer + buffer_size;

    while (buffer < buffer_end) {
        int32_t remaining = buffer_end - buffer;
        int32_t chunk_size = MIN(remaining, SFCE_STRING_BUFFER_SIZE_THRESHOLD);

        struct sfce_piece piece;
        error_code = sfce_piece_tree_create_piece(tree, buffer, chunk_size, &piece);
        if (error_code != SFCE_ERROR_OK) {
            goto error;
        }

        struct sfce_piece_node *new_node = sfce_piece_node_create(piece);

        if (new_node == NULL) {
            error_code = SFCE_ERROR_OUT_OF_MEMORY;
            goto error;
        }

        sfce_piece_node_insert_right(&root, rightmost_node, new_node);
        rightmost_node = new_node;
        buffer += chunk_size;
    }

    *result = root;
    return SFCE_ERROR_OK;

error:
    sfce_piece_node_destroy(root);
    return error_code;
}

enum sfce_error_code sfce_piece_tree_create_piece(struct sfce_piece_tree *tree, const void *data, int32_t byte_count, struct sfce_piece *result_piece)
{
    enum sfce_error_code error_code = sfce_piece_tree_ensure_change_buffer_size(tree, byte_count);
    if (error_code != SFCE_ERROR_OK) {
        return error_code;
    }

    struct sfce_string_buffer *string_buffer = &tree->buffers[tree->change_buffer_index];
    struct sfce_buffer_position start_position = sfce_string_buffer_get_end_position(string_buffer);

    error_code = sfce_string_buffer_append_content(string_buffer, data, byte_count);
    if (error_code != SFCE_ERROR_OK) {
        return error_code;
    }

    struct sfce_buffer_position end_position = sfce_string_buffer_get_end_position(string_buffer);
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

enum sfce_error_code sfce_piece_tree_insert_with_offset(struct sfce_piece_tree *tree, int32_t offset, const uint8_t *data, int32_t byte_count)
{
    struct sfce_node_position where = sfce_piece_tree_node_at_offset(tree, offset);
    return sfce_piece_tree_insert_with_node_position(tree, where, data, byte_count);
}

enum sfce_error_code sfce_piece_tree_erase_with_offset(struct sfce_piece_tree *tree, int32_t offset, int32_t byte_count)
{
    struct sfce_node_position start = sfce_piece_tree_node_at_offset(tree, offset);
    struct sfce_node_position end = sfce_piece_tree_node_at_offset(tree, offset + byte_count);
    return sfce_piece_tree_erase_with_node_position(tree, start, end);
}

enum sfce_error_code sfce_piece_tree_insert_with_position(struct sfce_piece_tree *tree, struct sfce_position position, const uint8_t *data, int32_t byte_count)
{
    struct sfce_node_position where = sfce_piece_tree_node_at_position(tree, position.col, position.row);
    return sfce_piece_tree_insert_with_node_position(tree, where, data, byte_count);
}

enum sfce_error_code sfce_piece_tree_erase_with_position(struct sfce_piece_tree *tree, struct sfce_position position, int32_t byte_count)
{
    struct sfce_node_position start_node = sfce_piece_tree_node_at_position(tree, position.col, position.row);
    struct sfce_node_position end_node = sfce_node_position_move_by_offset(start_node, byte_count);
    return sfce_piece_tree_erase_with_node_position(tree, start_node, end_node);
}

enum sfce_error_code sfce_piece_tree_insert_left_of_node(struct sfce_piece_tree *tree, struct sfce_piece_node *node, const uint8_t *data, int32_t byte_count)
{
    struct sfce_piece_node *subtree = sentinel_ptr;
    enum sfce_error_code error_code = sfce_piece_tree_create_node_subtree(tree, data, byte_count, &subtree);

    if (error_code != SFCE_ERROR_OK) {
        return error_code;
    }

    sfce_piece_node_insert_left(&tree->root, node, subtree);
    return SFCE_ERROR_OK;
}

enum sfce_error_code sfce_piece_tree_insert_right_of_node(struct sfce_piece_tree *tree, struct sfce_piece_node *node, const uint8_t *data, int32_t byte_count)
{
    enum sfce_error_code error_code;
    struct sfce_string_buffer *string_buffer = &tree->buffers[node->piece.buffer_index];
    int32_t offset = sfce_string_buffer_position_to_offset(string_buffer, node->piece.end);
    int32_t remaining = SFCE_STRING_BUFFER_SIZE_THRESHOLD - string_buffer->content.size;

    if (offset == string_buffer->content.size && remaining >= byte_count) {
        error_code = sfce_string_buffer_append_content(string_buffer, data, byte_count);

        if (error_code != SFCE_ERROR_OK) {
            return error_code;
        }

        node->piece.end = sfce_string_buffer_get_end_position(string_buffer);
        sfce_piece_node_recompute_piece_length(tree, node);
        sfce_piece_node_recompute_metadata(&tree->root, node);
        return SFCE_ERROR_OK;
    }

    struct sfce_piece_node *subtree = sentinel_ptr;
    error_code = sfce_piece_tree_create_node_subtree(tree, data, byte_count, &subtree);

    if (error_code != SFCE_ERROR_OK) {
        return error_code;
    }

    sfce_piece_node_insert_right(&tree->root, node, subtree);
    return SFCE_ERROR_OK;
}

enum sfce_error_code sfce_piece_tree_insert_middle_of_node_position(struct sfce_piece_tree *tree, struct sfce_node_position where, const uint8_t *data, int32_t byte_count)
{
    struct sfce_string_buffer *string_buffer = &tree->buffers[where.node->piece.buffer_index];
    struct sfce_piece_node *right_node = sfce_piece_node_create(where.node->piece);
    struct sfce_piece_node *left_node = where.node;

    if (right_node == NULL) {
        return SFCE_ERROR_OUT_OF_MEMORY;
    }

    struct sfce_buffer_position middle = sfce_string_buffer_move_position_by_offset(
        string_buffer, where.node->piece.start, where.offset_within_piece);

    right_node->piece.start = left_node->piece.end = middle;

    sfce_piece_node_recompute_piece_length(tree, left_node);
    sfce_piece_node_recompute_piece_length(tree, right_node);
    sfce_piece_node_recompute_metadata(&tree->root, where.node);

    struct sfce_piece_node *subtree = sentinel_ptr;
    enum sfce_error_code error_code = sfce_piece_tree_create_node_subtree(tree, data, byte_count, &subtree);
    if (error_code != SFCE_ERROR_OK) {
        return error_code;
    }

    sfce_piece_node_insert_right(&tree->root, left_node, right_node);
    sfce_piece_node_insert_right(&tree->root, left_node, subtree);
    return SFCE_ERROR_OK;
}

enum sfce_error_code sfce_piece_tree_insert_with_node_position(struct sfce_piece_tree *tree, struct sfce_node_position where, const uint8_t *data, int32_t byte_count)
{
    enum sfce_error_code error_code;

    if (where.node == sentinel_ptr && tree->root != sentinel_ptr) {
        return SFCE_ERROR_FAILED_INSERTION;
    }

    if (tree->root == sentinel_ptr) {
        struct sfce_piece_node *subtree = sentinel_ptr;
        error_code = sfce_piece_tree_create_node_subtree(tree, data, byte_count, &subtree);

        if (error_code != SFCE_ERROR_OK) {
            return error_code;
        }

        tree->root = subtree;
        tree->root->color = SFCE_COLOR_BLACK;
        sfce_piece_node_recompute_metadata(&tree->root, subtree);
    }
    else {
        if (where.offset_within_piece == 0) {
            error_code = sfce_piece_tree_insert_left_of_node(tree, where.node, data, byte_count);
        }
        else if (where.offset_within_piece >= where.node->piece.length) {
            error_code = sfce_piece_tree_insert_right_of_node(tree, where.node, data, byte_count);
        }
        else {
            error_code = sfce_piece_tree_insert_middle_of_node_position(tree, where, data, byte_count);
        }

        if (error_code != SFCE_ERROR_OK) {
            return error_code;
        }
    }

    sfce_piece_tree_recompute_metadata(tree);
    return SFCE_ERROR_OK;
}

enum sfce_error_code sfce_piece_tree_erase_with_node_position(struct sfce_piece_tree *tree, struct sfce_node_position start, struct sfce_node_position end)
{
    if (tree->length == 0) {
        return SFCE_ERROR_OK;
    }

    if (start.node == sentinel_ptr || end.node == sentinel_ptr) {
        return SFCE_ERROR_FAILED_ERASURE;
    }

    if (start.node == end.node) {
        struct sfce_piece_node *node = start.node;
        struct sfce_string_buffer *string_buffer = &tree->buffers[node->piece.buffer_index];

        if (start.offset_within_piece < 0 || start.offset_within_piece > node->piece.length) {
            return SFCE_ERROR_OUT_OF_BOUNDS;
        }

        if (end.offset_within_piece < 0 || end.offset_within_piece > node->piece.length) {
            return SFCE_ERROR_OUT_OF_BOUNDS;
        }

        if (start.offset_within_piece <= 0 && end.offset_within_piece >= node->piece.length) {
            sfce_piece_node_remove_node(&tree->root, node);
        }
        else {
            if (start.offset_within_piece == 0) {
                node->piece.start = sfce_string_buffer_move_position_by_offset(string_buffer, node->piece.start, end.offset_within_piece);
            }
            else if (end.offset_within_piece == node->piece.length) {
                node->piece.end = sfce_string_buffer_move_position_by_offset(string_buffer, node->piece.start, start.offset_within_piece);
            }
            else {
                struct sfce_piece_node *right = sfce_piece_node_create(node->piece);

                if (right == NULL) {
                    return SFCE_ERROR_OUT_OF_MEMORY;
                }

                right->piece.start = sfce_string_buffer_move_position_by_offset(string_buffer, node->piece.start, end.offset_within_piece);
                node->piece.end = sfce_string_buffer_move_position_by_offset(string_buffer, node->piece.start, start.offset_within_piece);

                sfce_piece_node_recompute_piece_length(tree, right);
                sfce_piece_node_insert_right(&tree->root, node, right);
            }

            sfce_piece_node_recompute_piece_length(tree, node);
        }
    }
    else {
        struct sfce_piece_node *node = sfce_piece_node_next(start.node);
        while (node != end.node && node != sentinel_ptr) {
            struct sfce_piece_node *next = sfce_piece_node_next(node);
            sfce_piece_node_remove_node(&tree->root, node);
            node = next;
        }

        if (start.offset_within_piece <= 0) {
            sfce_piece_node_remove_node(&tree->root, start.node);
        }
        else {
            start.node->piece.end = sfce_string_buffer_move_position_by_offset(
                &tree->buffers[start.node->piece.buffer_index],
                start.node->piece.start,
                start.offset_within_piece
            );
            sfce_piece_node_recompute_piece_length(tree, start.node);
        }

        if (end.offset_within_piece >= end.node->piece.length) {
            sfce_piece_node_remove_node(&tree->root, end.node);
        }
        else {
            end.node->piece.start = sfce_string_buffer_move_position_by_offset(
                &tree->buffers[end.node->piece.buffer_index],
                end.node->piece.start,
                end.offset_within_piece
            );
            sfce_piece_node_recompute_piece_length(tree, end.node);
        }
    }

    sfce_piece_tree_recompute_metadata(tree);
    return SFCE_ERROR_OK;
}

enum sfce_error_code sfce_piece_tree_write_to_file(struct sfce_piece_tree *tree, const char *filepath)
{
    FILE *fp = fopen(filepath, "wb+");

    if (fp == NULL) {
        return SFCE_ERROR_UNABLE_TO_CREATE_FILE;
    }

    struct sfce_piece_node *node = sfce_piece_node_leftmost(tree->root);
    while (node != sentinel_ptr) {
        struct sfce_string_view content = sfce_piece_tree_get_piece_content(tree, node->piece);

        if (fwrite(content.data, 1, content.size, fp) != (size_t)content.size) {
            fclose(fp);
            return SFCE_ERROR_FAILED_FILE_WRITE;
        }

        node = sfce_piece_node_next(node);
    }

    fclose(fp);
    return SFCE_ERROR_OK;
}

enum sfce_error_code sfce_piece_tree_load_file(struct sfce_piece_tree *tree, const char *filepath)
{
    FILE *fp = fopen(filepath, "rb+");
    enum sfce_error_code error_code = SFCE_ERROR_OK;

    if (fp == NULL) {
        return SFCE_ERROR_UNABLE_TO_OPEN_FILE;
    }

    struct sfce_string_buffer string_buffer = { .content.size = 0x7FFFFFFF };
    struct sfce_piece_node *rightmost = sfce_piece_node_rightmost(tree->root);

    while (SFCE_TRUE) {
        string_buffer = (struct sfce_string_buffer) {};

        error_code = sfce_string_reserve(&string_buffer.content, SFCE_STRING_BUFFER_SIZE_THRESHOLD);
        if (error_code != SFCE_ERROR_OK) goto error;

        string_buffer.content.size = fread(string_buffer.content.data, 1, SFCE_STRING_BUFFER_SIZE_THRESHOLD, fp);
        if (string_buffer.content.size == 0) goto error;

        error_code = sfce_line_starts_push_line_offset(&string_buffer.line_starts, 0);
        if (error_code != SFCE_ERROR_OK) goto error;

        error_code = sfce_string_buffer_recount_line_start_offsets(&string_buffer, 0, string_buffer.content.size);
        if (error_code != SFCE_ERROR_OK) goto error;

        error_code = sfce_piece_tree_add_string_buffer(tree, string_buffer);
        if (error_code != SFCE_ERROR_OK) goto error;

        struct sfce_piece_node *node = calloc(1, sizeof *node);
        if (node == NULL) {
            error_code = SFCE_ERROR_OUT_OF_MEMORY;
            goto done;
        }

        struct sfce_piece piece = {
            .buffer_index = tree->buffer_count - 1,
            .length = string_buffer.content.size,
            .line_count = string_buffer.line_starts.count - 1,
            .end.line_start_index = string_buffer.line_starts.count - 1,
            .end.column = string_buffer.content.size - string_buffer.line_starts.offsets[string_buffer.line_starts.count - 1],
        };

        *node = (struct sfce_piece_node) {
            .left   = sentinel_ptr,
            .right  = sentinel_ptr,
            .parent = sentinel_ptr,
            .piece  = piece,
            .color  = SFCE_COLOR_BLACK,
        };

        sfce_piece_node_insert_right(&tree->root, rightmost, node);
        rightmost = node;
    }

    __builtin_unreachable();

error:
    sfce_string_buffer_destroy(&string_buffer);
done:
    sfce_piece_tree_recompute_metadata(tree);
    fclose(fp);
    return error_code;
}

enum sfce_error_code sfce_piece_tree_get_line_content(struct sfce_piece_tree *tree, int32_t row, struct sfce_string *string)
{
    struct sfce_node_position node0 = sfce_piece_tree_node_at_position(tree, 0, row);
    struct sfce_node_position node1 = sfce_piece_tree_node_at_position(tree, 0, row + 1);
    return sfce_piece_tree_get_content_between_node_positions(tree, node0, node1, string);
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

enum sfce_error_code sfce_piece_tree_from_snapshot(struct sfce_piece_tree *tree, struct sfce_piece_tree_snapshot *snapshot)
{
    sfce_piece_node_destroy(tree->root);
    tree->root = sentinel_ptr;

    struct sfce_piece_node *rightmost = tree->root;
    for (int32_t idx = 0; idx < snapshot->piece_count; ++idx) {
        struct sfce_piece_node *node = sfce_piece_node_create(snapshot->pieces[idx]);

        if (node == NULL) {
            goto error_out_of_mem;
        }

        sfce_piece_node_insert_right(&tree->root, rightmost, node);
        rightmost = node;
    }

    sfce_piece_tree_recompute_metadata(tree);
    return SFCE_ERROR_OK;

error_out_of_mem:
    sfce_piece_tree_destroy(tree);
    return SFCE_ERROR_OUT_OF_MEMORY;
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
            return SFCE_ERROR_OUT_OF_MEMORY;
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

void sfce_console_buffer_destroy(struct sfce_console_buffer *console)
{
    sfce_string_destroy(&console->command);
    sfce_string_destroy(&console->temp_print_string);

    if (console->cells != NULL) {
        free(console->cells);
    }

    sfce_restore_console_state(&console->save_state);
}

void sfce_console_buffer_clear(struct sfce_console_buffer *console, struct sfce_console_style style)
{
    const struct sfce_console_cell blank_cell = { .codepoint = ' ', .style = style };
    const int32_t cell_count = console->window_size.width * console->window_size.height;

    for (int32_t idx = 0; idx < cell_count; ++idx) {
        console->cells[idx] = blank_cell;
    }
}

enum sfce_error_code sfce_console_buffer_create(struct sfce_console_buffer *console)
{
    if (console == NULL) {
        return SFCE_ERROR_NULL_POINTER;
    }

    struct sfce_console_state save_state = {};
    enum sfce_error_code error_code = sfce_setup_console(&save_state);
    if (error_code != SFCE_ERROR_OK) {
        return error_code;
    }

    struct sfce_window_size window_size = {};
    error_code = sfce_get_console_screen_size(&window_size);
    if (error_code != SFCE_ERROR_OK) {
        return error_code;
    }

    void *cells = calloc(window_size.width * window_size.height, sizeof *console->cells);
    if (cells == NULL) {
        return SFCE_ERROR_OUT_OF_MEMORY;
    }

    *console = (struct sfce_console_buffer) {
        .cells  = cells,
        .save_state = save_state,
        .window_size = window_size,
        .tab_size = SFCE_DEFAULT_TAB_SIZE,
    };

    error_code = sfce_console_buffer_update(console);
    if (error_code != SFCE_ERROR_OK) {
        return error_code;
    }

    return SFCE_ERROR_OK;
}

enum sfce_error_code sfce_console_buffer_update(struct sfce_console_buffer *console)
{
    struct sfce_window_size window_size = {};
    enum sfce_error_code error_code = sfce_get_console_screen_size(&window_size);

    if (error_code != SFCE_ERROR_OK) {
        return error_code;
    }

    if (window_size.width != console->window_size.width || window_size.height != console->window_size.height) {
        void *temp = console->cells;
        console->cells = realloc(console->cells, window_size.width * window_size.height * sizeof *console->cells);

        if (console->cells == NULL) {
            *console = (struct sfce_console_buffer) {};
            free(temp);
            return SFCE_ERROR_OUT_OF_MEMORY;
        }

        console->window_size = window_size;

        // 
        // The console buffer should avoid using callbacks
        // so the user isn't restricted access to certain variables.
        // Instead uses a event list where each event is stored in
        // a table which can be later process on the call side of 
        // any function which produces a event.
        // 
    }

    return SFCE_ERROR_OK;
}

enum sfce_error_code sfce_console_buffer_nprintf(struct sfce_console_buffer *console, int32_t col, int32_t row, struct sfce_console_style style, int32_t max_length, const char *format, ...)
{
    va_list va_args;
    va_start(va_args, format);

    sfce_string_clear(&console->temp_print_string);

    enum sfce_error_code error_code = sfce_string_vnprintf(&console->temp_print_string, max_length, format, va_args);
    if (error_code != SFCE_ERROR_OK) {
        return error_code;
    }

    va_end(va_args);
    return sfce_console_buffer_print_string(console, col, row, style, console->temp_print_string.data, console->temp_print_string.size);
}

enum sfce_error_code sfce_console_buffer_print_string(struct sfce_console_buffer *console, int32_t col, int32_t row, struct sfce_console_style style, const void *string, uint32_t length)
{
    const uint8_t *iter = string;
    const uint8_t *end  = iter + length;
    struct sfce_position position = { col, row };

    enum sfce_error_code error_code;
    while (iter < end) {
        int32_t remaining = end - iter;
        int32_t newline_size = newline_sequence_size(iter, remaining);
        int32_t codepoint = sfce_codepoint_decode_utf8(iter, remaining);
        int32_t codepoint_byte_count = sfce_codepoint_utf8_byte_count(codepoint);

        if (newline_size != 0) {
            iter += newline_size;
            position.row += 1;
            position.col = col;
        }
        else if (codepoint == '\t') {
            struct sfce_console_cell blank_cell = { .codepoint = ' ', .style = style };
            const int32_t distance_from_edge = position.col - col;
            const int32_t tab_width = console->tab_size - (distance_from_edge % console->tab_size);

            for (int32_t idx = 0; idx < tab_width; ++idx) {
                error_code = sfce_console_buffer_set_cell(console, position.col + idx, position.row, blank_cell);
                if (error_code != SFCE_ERROR_OK) {
                    return error_code;
                }
            }

            position.col += tab_width;
            iter += codepoint_byte_count;
        }
        else {
            if (!sfce_codepoint_is_print(codepoint)) {
                codepoint = ' ';
            }

            struct sfce_console_cell cell = {
                .codepoint = codepoint,
                .style = style,
            };

            error_code = sfce_console_buffer_set_cell(console, position.col, position.row, cell);
            if (error_code != SFCE_ERROR_OK) {
                return error_code;
            }

            iter += codepoint_byte_count;
            position.col += 1;
        }
    }

    return SFCE_ERROR_OK;
}

enum sfce_error_code sfce_console_buffer_set_style(struct sfce_console_buffer *console, int32_t col, int32_t row, struct sfce_console_style style)
{
    if (col < 0 || col >= console->window_size.width) {
        return SFCE_ERROR_OUT_OF_BOUNDS;
    }

    if (row < 0 || row >= console->window_size.height) {
        return SFCE_ERROR_OUT_OF_BOUNDS;
    }

    console->cells[row * console->window_size.width + col].style = style;
    return SFCE_ERROR_OK;
}

enum sfce_error_code sfce_console_buffer_set_cell(struct sfce_console_buffer *console, int32_t col, int32_t row, struct sfce_console_cell cell)
{
    if (col < 0 || col >= console->window_size.width) {
        return SFCE_ERROR_OUT_OF_BOUNDS;
    }

    if (row < 0 || row >= console->window_size.height) {
        return SFCE_ERROR_OUT_OF_BOUNDS;
    }

    console->cells[row * console->window_size.width + col] = cell;
    return SFCE_ERROR_OK;
}

enum sfce_error_code sfce_console_buffer_flush(struct sfce_console_buffer *console)
{
    uint8_t buffer[4] = {};
    struct sfce_console_cell current = {};
    struct sfce_console_cell previous = {};
    enum sfce_error_code error_code;

    (void)previous;

    sfce_string_clear(&console->command);

    // error_code = sfce_string_nprintf(&console->command, INT32_MAX, "\x1b[0m\x1b[2J");
    // if (error_code != SFCE_ERROR_OK) {
    //     return error_code;
    // }

    for (int32_t idx = 0, row = 0; row < console->window_size.height; ++row) {
        // error_code = sfce_string_nprintf(&console->command, INT32_MAX, "\x1b[%d;0H\x1b[2K", row + 1);
        error_code = sfce_string_nprintf(&console->command, INT32_MAX, "\x1b[%d;0H", row + 1);  

        if (error_code != SFCE_ERROR_OK) {
            return error_code;
        }

        for (int32_t col = 0; col < console->window_size.width; ++col, ++idx) {
            current = console->cells[idx];

            // if (current.style.foreground != previous.style.foreground) {
            if (1) {
                int32_t red   = (console->cells[idx].style.foreground >> 16) & 0xFF;
                int32_t green = (console->cells[idx].style.foreground >>  8) & 0xFF;
                int32_t blue  = (console->cells[idx].style.foreground >>  0) & 0xFF;
                error_code = sfce_string_nprintf(&console->command, INT32_MAX, "\x1b[38;2;%d;%d;%dm", red, green, blue);
                if (error_code != SFCE_ERROR_OK) {
                    return error_code;
                }
            }

            // if (current.style.background != previous.style.background) {
            if (1) {
                int32_t red   = (console->cells[idx].style.background >> 16) & 0xFF;
                int32_t green = (console->cells[idx].style.background >>  8) & 0xFF;
                int32_t blue  = (console->cells[idx].style.background >>  0) & 0xFF;
                error_code = sfce_string_nprintf(&console->command, INT32_MAX, "\x1b[48;2;%d;%d;%dm", red, green, blue);
                if (error_code != SFCE_ERROR_OK) {
                    return error_code;
                }
            }

            // if (current.style.attributes != previous.style.attributes) {
            // if (1) {
            //     error_code = sfce_string_nprintf(&console->command, INT32_MAX, "\x1b[%dm", current.style.attributes);
            //     if (error_code != SFCE_ERROR_OK) {
            //         return error_code;
            //     }
            // }

            // 
            // Should check if the current cell's codepoint width is non zero
            // also for non-printable characters
            // 
            if (sfce_codepoint_is_print(console->cells[idx].codepoint)) {
                uint8_t buffer_size = sfce_codepoint_encode_utf8(console->cells[idx].codepoint, buffer);
                error_code = sfce_string_push_back_buffer(&console->command, buffer, buffer_size);
            }
            else {
                error_code = sfce_string_push_back_byte(&console->command, ' ');
            }

            if (error_code != SFCE_ERROR_OK) {
                return error_code;
            }

            previous = current;
        }
    }

    error_code = sfce_string_push_back_buffer(&console->command, "\x1b[?25l", 6);
    if (error_code != SFCE_ERROR_OK) {
        return error_code;
    }

    error_code = sfce_write(console->command.data, console->command.size);
    if (error_code != SFCE_ERROR_OK) {
        return error_code;
    }

    return SFCE_ERROR_OK;
}

void sfce_editor_window_destroy(struct sfce_editor_window *window)
{
    if (window != NULL) {
        while (window->cursors != NULL) {
            sfce_cursor_destroy(window->cursors);
        }

        sfce_piece_tree_destroy(window->tree);

        sfce_editor_window_destroy(window->window0);
        sfce_editor_window_destroy(window->window1);
    }
}

enum sfce_error_code sfce_editor_window_display(struct sfce_editor_window *window, struct sfce_console_buffer *console, struct sfce_string *line_temp)
{
    enum sfce_error_code error_code;
    static const struct sfce_console_style style = {
        .foreground = 0x00FFFFFF,
        .background = 0x00000000,
    };

    static const struct sfce_console_style line_number_style = {
        .foreground = 0x00525252,
        .background = 0x00000000,
    };

    static const struct sfce_console_style status_style = {
        .foreground = 0x00FFFFFF,
        .background = 0x00525252,
    };

    int32_t window_width = window->rectangle.right - window->rectangle.left + 1;
    int32_t line_padding_size = 0;

    if (window->enable_line_numbering) {
        int32_t line_number = window->tree->line_count;
        int32_t digit_count = log10l(line_number) + 1;
        line_padding_size = round_multiple_of_two(digit_count, 2);
    }

    int32_t line_contents_start = window->rectangle.left + line_padding_size + window->enable_line_numbering;

    sfce_console_buffer_clear(console, style);

    sfce_string_clear(line_temp);
    for (int32_t row = window->rectangle.top, line_index = 0; row <= window->rectangle.bottom; ++row, ++line_index) {
        if (line_index < window->tree->line_count) {
            error_code = sfce_piece_tree_get_line_content(window->tree, line_index, line_temp);
            if (error_code != SFCE_ERROR_OK) {
                return error_code;
            }
        }

        if (window->enable_line_numbering) {
            int32_t line_number = line_index + 1;
            if (window->enable_relative_line_numbering && window->cursor_count >= 1) {
                line_number = line_index - window->cursors[0].position.row;
                line_number = line_number > 0 ? line_number : -line_number;
            }

            if (line_index >= window->tree->line_count) {
                struct sfce_console_cell cell = { '~', line_number_style };
                sfce_console_buffer_set_cell(console, window->rectangle.left, row, cell);
            }
            else {
                sfce_console_buffer_nprintf(console, window->rectangle.left, row, line_number_style, window_width, "%*d", line_padding_size, line_number);
            }
        }

        if (line_index < window->tree->line_count) {
            sfce_console_buffer_print_string(console, line_contents_start, row, style, line_temp->data, line_temp->size);
        }
    }

    const char *filepath = window->filepath == NULL ? "[Untitled File]" : window->filepath;

    struct sfce_console_cell cell = {
        .codepoint = ' ',
        .style = status_style,
    };

    struct sfce_console_style cursor_style = {
        .foreground = 0x00000000,
        .background = 0x00FFFFFF,
    };

    for (int32_t col = window->rectangle.left; col <= window->rectangle.right; ++col) {
        sfce_console_buffer_set_cell(console, col, window->rectangle.bottom, cell);
    }

    struct sfce_string *temp_string = &console->temp_print_string;
    struct sfce_position cursor_position = window->cursors->position;
    int64_t cursor_offset = sfce_piece_tree_offset_at_position(window->tree, cursor_position);

    sfce_string_clear(temp_string);
    sfce_string_nprintf(temp_string, INT32_MAX, "%s  ", filepath);
    sfce_string_nprintf(temp_string, INT32_MAX, "Col %d ", cursor_position.col);
    sfce_string_nprintf(temp_string, INT32_MAX, "Row %d ", cursor_position.row);
    sfce_string_nprintf(temp_string, INT32_MAX, "Offset %d ", cursor_offset);
    sfce_string_nprintf(temp_string, INT32_MAX, "Length: %d ", window->tree->length);
    sfce_string_nprintf(temp_string, INT32_MAX, "Line Count: %d ", window->tree->line_count);

    sfce_console_buffer_print_string(console, window->rectangle.left, window->rectangle.bottom, status_style, temp_string->data, temp_string->size);
    sfce_console_buffer_set_style(console, line_contents_start + cursor_position.col, window->rectangle.top + cursor_position.row, cursor_style);

    return 0;
}

struct sfce_cursor *sfce_cursor_create(struct sfce_editor_window *window)
{
    struct sfce_cursor *cursor = calloc(1, sizeof *cursor);

    if (cursor != 0) {
        *cursor = (struct sfce_cursor) {
            .window = window,
            .tree   = window->tree,
            .prev   = cursor,
            .next   = cursor,
        };

        ++window->cursor_count;
        if (window->cursors != NULL) {
            struct sfce_cursor *const prev = window->cursors->prev;
            struct sfce_cursor *const next = window->cursors;

            prev->next = next->prev = cursor;
            cursor->prev = prev;
            cursor->next = next;
        }
        else {
            window->cursors = cursor;
        }
    }

    return cursor;
}

void sfce_cursor_destroy(struct sfce_cursor *cursor)
{
    struct sfce_editor_window *const window = cursor->window;

    --window->cursor_count;
    if (window->cursors != cursor) {
        cursor->prev->next = cursor->next;
        cursor->next->prev = cursor->prev;
    }
    else if (window->cursors != window->cursors->next
    ||       window->cursors != window->cursors->prev) {
        window->cursors->prev->next = window->cursors->next;
        window->cursors->next->prev = window->cursors->prev;
        window->cursors = window->cursors->next;
    }
    else {
        window->cursors = NULL;
    }

    sfce_string_destroy(&cursor->copy_string);
    free(cursor);
}

/*
void sfce_cursor_destroy(struct sfce_cursor *cursor)
{
}

void sfce_cursor_move_left(struct sfce_cursor *cursor)
{
    if (cursor->position.col > 0) {
        cursor->position.col -= 1;
    }
    else if (cursor->position.row != 0) {
        cursor->position.row = cursor->position.row - 1;
        cursor->position.col = sfce_piece_tree_get_line_length(cursor->window->tree, cursor->position.row);;
    }

    cursor->target_render_col = sfce_piece_tree_get_render_column_from_column(cursor->window->tree, cursor->position.row, cursor->position.col);
}

void sfce_cursor_move_right(struct sfce_cursor *cursor)
{
    int32_t line_character_count = sfce_piece_tree_get_line_length(cursor->window->tree, cursor->position.row);

    if (cursor->position.col < line_character_count) {
        cursor->position.col += 1;
        cursor->target_render_col = sfce_piece_tree_get_render_column_from_column(cursor->window->tree, cursor->position.row, cursor->position.col);
    }
    else if (cursor->position.row + 1 < cursor->window->tree->line_count) {
        cursor->position.col = 0;
        cursor->position.row += 1;
        cursor->target_render_col = 0;
    }
}

void sfce_cursor_move_up(struct sfce_cursor *cursor)
{
    if (cursor->position.row > 0) {
        cursor->position.row = cursor->position.row - 1;
        cursor->position.col = sfce_piece_tree_get_column_from_render_column(cursor->window->tree, cursor->position.row, cursor->target_render_col);
    }
    else {
        cursor->position.col = 0;
        cursor->target_render_col = 0;
    }
}

void sfce_cursor_move_down(struct sfce_cursor *cursor)
{
    if (cursor->position.row < cursor->window->tree->line_count) {
        cursor->position.row = cursor->position.row + 1;
        cursor->position.col = sfce_piece_tree_get_column_from_render_column(cursor->window->tree, cursor->position.row, cursor->target_render_col);
    }
    else {
        cursor->position.col = sfce_piece_tree_get_line_length(cursor->window->tree, cursor->position.row);
        cursor->target_render_col = sfce_piece_tree_get_render_column_from_column(cursor->window->tree, cursor->position.row, cursor->position.col);
    }
}

uint8_t sfce_cursor_move_word_left(struct sfce_cursor *cursor)
{
    sfce_log_error("sfce_cursor_move_word_left is unimplemented!\n");
    return SFCE_FALSE;
}

uint8_t sfce_cursor_move_word_right(struct sfce_cursor *cursor)
{
    sfce_log_error("sfce_cursor_move_word_right is unimplemented!\n");
    return SFCE_FALSE;
}

uint8_t sfce_cursor_move_offset(struct sfce_cursor *cursor, int32_t offset)
{
    sfce_log_error("sfce_cursor_move_offset is unimplemented!\n");
    return SFCE_FALSE;
}

enum sfce_error_code sfce_cursor_insert_character(struct sfce_cursor *cursor, int32_t character)
{
    uint8_t bytes[4] = {};
    int32_t length = sfce_encode_utf8_codepoint(character, bytes);
    return sfce_piece_tree_insert_with_position(cursor->window->tree, cursor->position, bytes, length);
}

enum sfce_error_code sfce_cursor_insert(struct sfce_cursor *cursor, size_t size, const uint8_t *data)
{
    return sfce_piece_tree_insert_with_position(cursor->window->tree, cursor->position, data, size);
}

enum sfce_error_code sfce_cursor_erase_character(struct sfce_cursor *cursor)
{
    struct sfce_node_position node_position = sfce_piece_tree_node_at_position(cursor->window->tree, cursor->position.col, cursor->position.row);
    int32_t codepoint = sfce_piece_tree_get_codepoint_from_node_position(cursor->window->tree, node_position);
    int32_t byte_count = sfce_codepoint_utf8_byte_count(codepoint);
    return sfce_piece_tree_erase_with_position(cursor->window->tree, cursor->position, byte_count);
}

enum sfce_error_code sfce_cursor_erase(struct sfce_cursor *cursor, size_t size)
{
    return sfce_piece_tree_erase_with_position(cursor->window->tree, cursor->position, size);
}

int32_t sfce_cursor_get_character(const struct sfce_cursor *cursor)
{
}

int32_t sfce_cursor_get_prev_character(const struct sfce_cursor *cursor)
{
}
*/

void sfce_log_error(const char *format, ...)
{
    if (g_should_log_to_error_string) {
        va_list va_args;
        va_start(va_args, format);

        sfce_string_vnprintf(&g_logging_string, INT32_MAX, format, va_args);

        va_end(va_args);
    }
}
