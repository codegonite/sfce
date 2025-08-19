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
#include <stdbool.h>
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

enum { SFCE_TRUE = 1, SFCE_FALSE = 0 };
enum { FNV_PRIME = 0x00000100000001b3 };
enum { FNV_OFFSET_BASIS = 0xcbf29ce484222325 };
enum { SFCE_DEFAULT_TAB_SIZE = 4 };
enum { SFCE_FILE_READ_CHUNK_SIZE = 1024 };
enum { SFCE_STRING_BUFFER_SIZE_THRESHOLD = 0xFFFF };
enum { SFCE_EDITOR_STYLE_BUCKET_COUNT = 0x100 };

//
// NOTE: All allocation sizes must be powers of two.
// Inorder for the "round_multiple_of_two" function
// to work correctly.
//
enum { SFCE_LINE_STARTS_ALLOCATION_SIZE = 16 };
enum { SFCE_STRING_BUFFER_ALLOCATION_SIZE = 16 };
enum { SFCE_SNAPSHOT_ALLOCATION_SIZE = 16 };
enum { SFCE_STRING_ALLOCATION_SIZE = 256 };

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
    SFCE_UNICODE_CATEGORY_CS = 4, // Surrrogate
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
    struct sfce_cursors       *prev;
    struct sfce_cursors       *next;
    struct sfce_editor_window *window;
    struct sfce_position       position;
    struct sfce_position       anchor;
    struct sfce_string         copy_buffer;
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
    struct sfce_editor_style_bucket* buckets[SFCE_EDITOR_STYLE_BUCKET_COUNT];
};

struct sfce_editor_window {
    struct sfce_cursor           main_cursor;
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
//     int32_t                    virutal_col;
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
int32_t sfce_to_upper(int32_t codepoint);
int32_t sfce_to_lower(int32_t codepoint);
const struct sfce_utf8_property *sfce_utf8_codepoint_property_unchecked(int32_t codepoint);
const struct sfce_utf8_property *sfce_utf8_codepoint_property(int32_t codepoint);
uint8_t sfce_utf8_continuation(uint8_t byte);
uint8_t sfce_encode_utf8_codepoint(int32_t codepoint, uint8_t *buffer);
int32_t sfce_decode_utf8_codepoint(const void *buffer, int32_t byte_count);
uint8_t sfce_codepoint_utf8_byte_count(int32_t codepoint);
uint8_t sfce_codepoint_width(int32_t codepoint);
int8_t sfce_codepoint_is_print(int32_t codepoint);
const char *make_character_printable(int32_t character);
enum sfce_error_code sfce_get_console_screen_size(struct sfce_window_size *window_size);
enum sfce_error_code sfce_write(const void *buffer, int32_t buffer_size);
enum sfce_error_code sfce_write_zero_terminated_string(const void *buffer);
enum sfce_error_code sfce_enable_console_temp_buffer();
enum sfce_error_code sfce_disable_console_temp_buffer();
int32_t sfce_parse_csi_parameter(int32_t *character);
struct sfce_keypress sfce_get_keypress();

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
enum sfce_error_code sfce_string_to_snake_case(const struct sfce_string *string, struct sfce_string *result_string);
enum sfce_error_code sfce_string_to_kebab_case(const struct sfce_string *string, struct sfce_string *result_string);
enum sfce_error_code sfce_string_to_title_case(const struct sfce_string *string, struct sfce_string *result_string);
enum sfce_error_code sfce_string_to_camel_case(const struct sfce_string *string, struct sfce_string *result_string);
enum sfce_error_code sfce_string_to_pascal_case(const struct sfce_string *string, struct sfce_string *result_string);
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
void sfce_piece_node_reset_sentinel();
int32_t sfce_piece_node_offset_from_start(struct sfce_piece_node *node);

static inline void sfce_piece_node_to_string(struct sfce_piece_tree *tree, struct sfce_piece_node *node, int32_t space, struct sfce_string *out);
static inline void sfce_piece_node_inorder_print_to_string(struct sfce_piece_tree *tree, struct sfce_piece_node *root, struct sfce_string *out);

struct sfce_node_position sfce_node_position_move_by_offset(struct sfce_node_position position, int32_t offset);

struct sfce_piece_tree *sfce_piece_tree_create();
void sfce_piece_tree_destroy(struct sfce_piece_tree *tree);
int32_t sfce_piece_tree_line_offset_in_piece(struct sfce_piece_tree *tree, struct sfce_piece piece, int32_t line_number);
int32_t sfce_piece_tree_count_lines_in_piece_until_offset(struct sfce_piece_tree *tree, struct sfce_piece piece, int32_t offset);
// int32_t sfce_piece_tree_character_at_node_position(struct sfce_piece_tree *tree, struct sfce_node_position position);
// int32_t sfce_piece_tree_codepoint_at_node_position(struct sfce_piece_tree *tree, struct sfce_node_position position);

int32_t sfce_piece_tree_offset_at_position(struct sfce_piece_tree *tree, const struct sfce_position position);
struct sfce_position sfce_piece_tree_position_at_offset(struct sfce_piece_tree *tree, int32_t offset);
struct sfce_node_position sfce_piece_tree_node_at_offset(struct sfce_piece_tree *tree, int32_t offset);
struct sfce_node_position sfce_piece_tree_node_at_position(struct sfce_piece_tree *tree, int32_t col, int32_t row);

struct sfce_string_view sfce_piece_tree_get_piece_content(const struct sfce_piece_tree *tree, struct sfce_piece piece);
enum sfce_error_code sfce_piece_tree_get_node_content(const struct sfce_piece_tree *tree, struct sfce_piece_node *node, struct sfce_string *string);
enum sfce_error_code sfce_piece_tree_get_substring(struct sfce_piece_tree *tree, int32_t offset, int32_t length, struct sfce_string *string);
int32_t sfce_piece_tree_get_codepoint_from_node_position(struct sfce_piece_tree *tree, struct sfce_node_position node_position);
enum sfce_error_code sfce_piece_tree_get_line_content(struct sfce_piece_tree *tree, int32_t line_number, struct sfce_string *string);
int32_t sfce_piece_tree_get_line_length(struct sfce_piece_tree *tree, int32_t row);
enum sfce_error_code sfce_piece_tree_get_content_between_node_positions(struct sfce_piece_tree *tree, struct sfce_node_position position0, struct sfce_node_position position1, struct sfce_string *string);
int32_t sfce_piece_tree_read_into_buffer(struct sfce_piece_tree *tree, struct sfce_node_position start, struct sfce_node_position end, int32_t buffer_size, uint8_t *buffer);

int32_t sfce_piece_tree_get_column_from_render_column(struct sfce_piece_tree *tree, int32_t row, int32_t target_render_col);

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

// 
// Internal function do not call directly
// 
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
// enum sfce_error_code sfce_console_buffer_nprintf(struct sfce_console_buffer *console, int32_t col, int32_t row, struct sfce_console_style style, int32_t max_length, const char *format, ...);
enum sfce_error_code sfce_console_buffer_nprintf(struct sfce_console_buffer *console, int32_t col, int32_t row, struct sfce_console_style style, int32_t max_length, const char *format, ...);
enum sfce_error_code sfce_console_buffer_print_string(struct sfce_console_buffer *console, int32_t col, int32_t row, struct sfce_console_style style, const void *string, uint32_t length);
enum sfce_error_code sfce_console_buffer_set_style(struct sfce_console_buffer *console, int32_t col, int32_t row, struct sfce_console_style style);
enum sfce_error_code sfce_console_buffer_set_cell(struct sfce_console_buffer *console, int32_t col, int32_t row, struct sfce_console_cell cell);
enum sfce_error_code sfce_console_buffer_flush(struct sfce_console_buffer *console);

void sfce_editor_window_destroy(struct sfce_editor_window *window);
enum sfce_error_code sfce_editor_window_display(struct sfce_editor_window *window, struct sfce_console_buffer *console, struct sfce_string *line_temp);

// struct sfce_cursor *sfce_cursor_create(struct sfce_piece_tree *tree);
void sfce_cursor_destroy(struct sfce_cursor *cursor);
void sfce_cursor_move_left(struct sfce_cursor *cursor);
void sfce_cursor_move_right(struct sfce_cursor *cursor);
void sfce_cursor_move_up(struct sfce_cursor *cursor);
void sfce_cursor_move_down(struct sfce_cursor *cursor);
bool sfce_cursor_move_word_left(struct sfce_cursor *cursor);
bool sfce_cursor_move_word_right(struct sfce_cursor *cursor);
bool sfce_cursor_move_offset(struct sfce_cursor *cursor, int32_t offset);
enum sfce_error_code sfce_cursor_insert_character(struct sfce_cursor *cursor, int32_t character);
enum sfce_error_code sfce_cursor_insert(struct sfce_cursor *cursor, size_t size, const uint8_t *data);
enum sfce_error_code sfce_cursor_erase_character(struct sfce_cursor *cursor);
enum sfce_error_code sfce_cursor_erase(struct sfce_cursor *cursor, size_t size);
int32_t sfce_cursor_get_character(const struct sfce_cursor *cursor);
int32_t sfce_cursor_get_prev_character(const struct sfce_cursor *cursor);

static inline void sfce_log_error(const char *format, ...);

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

static const struct sfce_utf8_property default_utf8_property = {
    SFCE_UNICODE_CATEGORY_CN, 0, SFCE_UNICODE_BIDI_CLASS_NONE, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 0, 0,
};

//
// Auto generated by utf8gen.js at 2025-02-06
//
static const struct sfce_utf8_property utf8_properties[3316] = {
    { SFCE_UNICODE_CATEGORY_CC, 0, SFCE_UNICODE_BIDI_CLASS_BN, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 1, 0 },
    { SFCE_UNICODE_CATEGORY_CC, 0, SFCE_UNICODE_BIDI_CLASS_S, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 1, 0 },
    { SFCE_UNICODE_CATEGORY_CC, 0, SFCE_UNICODE_BIDI_CLASS_B, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 1, 0 },
    { SFCE_UNICODE_CATEGORY_CC, 0, SFCE_UNICODE_BIDI_CLASS_WS, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 1, 0 },
    { SFCE_UNICODE_CATEGORY_ZS, 0, SFCE_UNICODE_BIDI_CLASS_WS, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 1, 0 },
    { SFCE_UNICODE_CATEGORY_PO, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 1, 0 },
    { SFCE_UNICODE_CATEGORY_PO, 0, SFCE_UNICODE_BIDI_CLASS_ET, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 1, 0 },
    { SFCE_UNICODE_CATEGORY_SC, 0, SFCE_UNICODE_BIDI_CLASS_ET, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 1, 0 },
    { SFCE_UNICODE_CATEGORY_PS, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 1, 1 },
    { SFCE_UNICODE_CATEGORY_PE, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 1, 1 },
    { SFCE_UNICODE_CATEGORY_SM, 0, SFCE_UNICODE_BIDI_CLASS_ES, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 1, 0 },
    { SFCE_UNICODE_CATEGORY_PO, 0, SFCE_UNICODE_BIDI_CLASS_CS, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 1, 0 },
    { SFCE_UNICODE_CATEGORY_PD, 0, SFCE_UNICODE_BIDI_CLASS_ES, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 1, 0 },
    { SFCE_UNICODE_CATEGORY_ND, 0, SFCE_UNICODE_BIDI_CLASS_EN, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 1, 0 },
    { SFCE_UNICODE_CATEGORY_SM, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 1, 1 },
    { SFCE_UNICODE_CATEGORY_SM, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 1, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 97, -1, 1, 1, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 98, -1, 1, 1, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 99, -1, 1, 1, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 100, -1, 1, 1, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 101, -1, 1, 1, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 102, -1, 1, 1, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 103, -1, 1, 1, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 104, -1, 1, 1, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 105, -1, 1, 1, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 106, -1, 1, 1, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 107, -1, 1, 1, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 108, -1, 1, 1, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 109, -1, 1, 1, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 110, -1, 1, 1, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 111, -1, 1, 1, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 112, -1, 1, 1, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 113, -1, 1, 1, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 114, -1, 1, 1, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 115, -1, 1, 1, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 116, -1, 1, 1, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 117, -1, 1, 1, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 118, -1, 1, 1, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 119, -1, 1, 1, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 120, -1, 1, 1, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 121, -1, 1, 1, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 122, -1, 1, 1, 0 },
    { SFCE_UNICODE_CATEGORY_SK, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 1, 0 },
    { SFCE_UNICODE_CATEGORY_PC, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 1, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 65, -1, 65, 1, 1, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66, -1, 66, 1, 1, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 67, -1, 67, 1, 1, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 68, -1, 68, 1, 1, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 69, -1, 69, 1, 1, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 70, -1, 70, 1, 1, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 71, -1, 71, 1, 1, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 72, -1, 72, 1, 1, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 73, -1, 73, 1, 1, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 74, -1, 74, 1, 1, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 75, -1, 75, 1, 1, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 76, -1, 76, 1, 1, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 77, -1, 77, 1, 1, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 78, -1, 78, 1, 1, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 79, -1, 79, 1, 1, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 80, -1, 80, 1, 1, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 81, -1, 81, 1, 1, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 82, -1, 82, 1, 1, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 83, -1, 83, 1, 1, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 84, -1, 84, 1, 1, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 85, -1, 85, 1, 1, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 86, -1, 86, 1, 1, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 87, -1, 87, 1, 1, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 88, -1, 88, 1, 1, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 89, -1, 89, 1, 1, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 90, -1, 90, 1, 1, 0 },
    { SFCE_UNICODE_CATEGORY_CC, 0, SFCE_UNICODE_BIDI_CLASS_BN, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_CC, 0, SFCE_UNICODE_BIDI_CLASS_B, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_ZS, 0, SFCE_UNICODE_BIDI_CLASS_CS, SFCE_UNICODE_DECOMPOSITION_NOBREAK, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_PO, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_SC, 0, SFCE_UNICODE_BIDI_CLASS_ET, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_SK, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_COMPAT, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_SUPER, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_PI, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 1 },
    { SFCE_UNICODE_CATEGORY_SM, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_CF, 0, SFCE_UNICODE_BIDI_CLASS_BN, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_ET, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_SM, 0, SFCE_UNICODE_BIDI_CLASS_ET, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_NO, 0, SFCE_UNICODE_BIDI_CLASS_EN, SFCE_UNICODE_DECOMPOSITION_SUPER, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 924, -1, 924, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_PF, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 1 },
    { SFCE_UNICODE_CATEGORY_NO, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_FRACTION, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 224, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 225, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 226, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 227, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 228, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 229, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 230, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 231, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 232, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 233, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 234, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 235, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 236, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 237, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 238, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 239, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 240, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 241, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 242, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 243, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 244, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 245, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 246, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 248, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 249, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 250, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 251, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 252, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 253, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 254, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 192, -1, 192, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 193, -1, 193, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 194, -1, 194, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 195, -1, 195, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 196, -1, 196, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 197, -1, 197, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 198, -1, 198, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 199, -1, 199, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 200, -1, 200, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 201, -1, 201, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 202, -1, 202, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 203, -1, 203, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 204, -1, 204, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 205, -1, 205, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 206, -1, 206, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 207, -1, 207, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 208, -1, 208, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 209, -1, 209, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 210, -1, 210, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 211, -1, 211, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 212, -1, 212, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 213, -1, 213, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 214, -1, 214, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 216, -1, 216, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 217, -1, 217, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 218, -1, 218, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 219, -1, 219, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 220, -1, 220, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 221, -1, 221, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 222, -1, 222, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 376, -1, 376, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 257, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 256, -1, 256, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 259, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 258, -1, 258, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 261, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 260, -1, 260, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 263, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 262, -1, 262, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 265, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 264, -1, 264, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 267, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 266, -1, 266, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 269, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 268, -1, 268, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 271, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 270, -1, 270, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 273, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 272, -1, 272, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 275, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 274, -1, 274, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 277, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 276, -1, 276, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 279, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 278, -1, 278, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 281, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 280, -1, 280, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 283, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 282, -1, 282, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 285, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 284, -1, 284, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 287, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 286, -1, 286, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 289, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 288, -1, 288, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 291, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 290, -1, 290, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 293, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 292, -1, 292, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 295, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 294, -1, 294, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 297, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 296, -1, 296, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 299, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 298, -1, 298, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 301, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 300, -1, 300, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 303, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 302, -1, 302, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 105, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 73, -1, 73, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, -1, 307, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 306, -1, 306, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 309, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 308, -1, 308, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 311, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 310, -1, 310, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 314, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 313, -1, 313, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 316, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 315, -1, 315, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 318, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 317, -1, 317, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, -1, 320, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 319, -1, 319, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 322, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 321, -1, 321, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 324, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 323, -1, 323, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 326, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 325, -1, 325, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 328, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 327, -1, 327, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 331, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 330, -1, 330, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 333, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 332, -1, 332, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 335, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 334, -1, 334, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 337, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 336, -1, 336, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 339, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 338, -1, 338, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 341, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 340, -1, 340, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 343, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 342, -1, 342, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 345, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 344, -1, 344, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 347, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 346, -1, 346, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 349, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 348, -1, 348, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 351, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 350, -1, 350, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 353, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 352, -1, 352, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 355, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 354, -1, 354, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 357, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 356, -1, 356, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 359, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 358, -1, 358, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 361, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 360, -1, 360, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 363, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 362, -1, 362, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 365, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 364, -1, 364, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 367, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 366, -1, 366, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 369, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 368, -1, 368, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 371, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 370, -1, 370, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 373, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 372, -1, 372, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 375, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 374, -1, 374, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 255, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 378, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 377, -1, 377, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 380, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 379, -1, 379, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 382, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 381, -1, 381, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 83, -1, 83, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 579, -1, 579, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 595, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 387, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 386, -1, 386, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 389, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 388, -1, 388, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 596, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 392, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 391, -1, 391, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 598, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 599, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 396, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 395, -1, 395, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 477, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 601, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 603, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 402, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 401, -1, 401, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 608, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 611, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 502, -1, 502, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 617, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 616, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 409, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 408, -1, 408, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 573, -1, 573, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42972, -1, 42972, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 623, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 626, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 544, -1, 544, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 629, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 417, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 416, -1, 416, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 419, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 418, -1, 418, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 421, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 420, -1, 420, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 640, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 424, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 423, -1, 423, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 643, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 429, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 428, -1, 428, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 648, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 432, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 431, -1, 431, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 650, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 651, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 436, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 435, -1, 435, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 438, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 437, -1, 437, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 658, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 441, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 440, -1, 440, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 445, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 444, -1, 444, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 503, -1, 503, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, -1, 454, 453, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LT, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 452, 454, 453, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 452, -1, 453, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, -1, 457, 456, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LT, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 455, 457, 456, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 455, -1, 456, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, -1, 460, 459, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LT, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 458, 460, 459, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 458, -1, 459, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 462, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 461, -1, 461, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 464, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 463, -1, 463, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 466, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 465, -1, 465, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 468, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 467, -1, 467, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 470, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 469, -1, 469, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 472, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 471, -1, 471, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 474, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 473, -1, 473, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 476, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 475, -1, 475, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 398, -1, 398, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 479, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 478, -1, 478, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 481, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 480, -1, 480, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 483, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 482, -1, 482, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 485, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 484, -1, 484, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 487, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 486, -1, 486, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 489, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 488, -1, 488, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 491, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 490, -1, 490, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 493, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 492, -1, 492, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 495, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 494, -1, 494, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, -1, 499, 498, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LT, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 497, 499, 498, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 497, -1, 498, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 501, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 500, -1, 500, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 405, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 447, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 505, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 504, -1, 504, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 507, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 506, -1, 506, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 509, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 508, -1, 508, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 511, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 510, -1, 510, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 513, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 512, -1, 512, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 515, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 514, -1, 514, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 517, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 516, -1, 516, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 519, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 518, -1, 518, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 521, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 520, -1, 520, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 523, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 522, -1, 522, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 525, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 524, -1, 524, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 527, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 526, -1, 526, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 529, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 528, -1, 528, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 531, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 530, -1, 530, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 533, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 532, -1, 532, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 535, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 534, -1, 534, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 537, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 536, -1, 536, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 539, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 538, -1, 538, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 541, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 540, -1, 540, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 543, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 542, -1, 542, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 414, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 547, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 546, -1, 546, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 549, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 548, -1, 548, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 551, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 550, -1, 550, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 553, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 552, -1, 552, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 555, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 554, -1, 554, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 557, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 556, -1, 556, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 559, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 558, -1, 558, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 561, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 560, -1, 560, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 563, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 562, -1, 562, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11365, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 572, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 571, -1, 571, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 410, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11366, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11390, -1, 11390, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11391, -1, 11391, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 578, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 577, -1, 577, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 384, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 649, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 652, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 583, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 582, -1, 582, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 585, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 584, -1, 584, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 587, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 586, -1, 586, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 589, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 588, -1, 588, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 591, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 590, -1, 590, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11375, -1, 11375, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11373, -1, 11373, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11376, -1, 11376, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 385, -1, 385, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 390, -1, 390, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 393, -1, 393, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 394, -1, 394, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 399, -1, 399, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 400, -1, 400, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42923, -1, 42923, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 403, -1, 403, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42924, -1, 42924, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 404, -1, 404, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42955, -1, 42955, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42893, -1, 42893, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42922, -1, 42922, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 407, -1, 407, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 406, -1, 406, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42926, -1, 42926, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11362, -1, 11362, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42925, -1, 42925, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 412, -1, 412, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11374, -1, 11374, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 413, -1, 413, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 415, -1, 415, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11364, -1, 11364, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 422, -1, 422, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42949, -1, 42949, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 425, -1, 425, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42929, -1, 42929, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 430, -1, 430, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 580, -1, 580, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 433, -1, 433, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 434, -1, 434, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 581, -1, 581, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 439, -1, 439, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42930, -1, 42930, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42928, -1, 42928, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LM, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_SUPER, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LM, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LM, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_SK, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 230, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 232, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 220, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 216, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 202, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 1, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 240, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, 921, -1, 921, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 0, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 233, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 234, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 881, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 880, -1, 880, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 883, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 882, -1, 882, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 887, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 886, -1, 886, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_CN, 0, SFCE_UNICODE_BIDI_CLASS_NONE, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 0, 0 },
    { SFCE_UNICODE_CATEGORY_LM, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1021, -1, 1021, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1022, -1, 1022, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1023, -1, 1023, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1011, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 940, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 941, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 942, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 943, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 972, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 973, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 974, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 945, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 946, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 947, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 948, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 949, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 950, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 951, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 952, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 953, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 954, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 955, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 956, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 957, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 958, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 959, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 960, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 961, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 963, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 964, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 965, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 966, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 967, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 968, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 969, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 970, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 971, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 902, -1, 902, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 904, -1, 904, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 905, -1, 905, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 906, -1, 906, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 913, -1, 913, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 914, -1, 914, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 915, -1, 915, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 916, -1, 916, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 917, -1, 917, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 918, -1, 918, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 919, -1, 919, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 920, -1, 920, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 921, -1, 921, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 922, -1, 922, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 923, -1, 923, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 924, -1, 924, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 925, -1, 925, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 926, -1, 926, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 927, -1, 927, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 928, -1, 928, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 929, -1, 929, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 931, -1, 931, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 932, -1, 932, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 933, -1, 933, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 934, -1, 934, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 935, -1, 935, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 936, -1, 936, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 937, -1, 937, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 938, -1, 938, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 939, -1, 939, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 908, -1, 908, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 910, -1, 910, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 911, -1, 911, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 983, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 914, -1, 914, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 920, -1, 920, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 934, -1, 934, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 928, -1, 928, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 975, -1, 975, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 985, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 984, -1, 984, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 987, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 986, -1, 986, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 989, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 988, -1, 988, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 991, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 990, -1, 990, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 993, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 992, -1, 992, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 995, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 994, -1, 994, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 997, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 996, -1, 996, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 999, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 998, -1, 998, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1001, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1000, -1, 1000, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1003, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1002, -1, 1002, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1005, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1004, -1, 1004, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1007, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1006, -1, 1006, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 922, -1, 922, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 929, -1, 929, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 1017, -1, 1017, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 895, -1, 895, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, -1, 952, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 917, -1, 917, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1016, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1015, -1, 1015, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, -1, 1010, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1019, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1018, -1, 1018, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 891, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 892, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 893, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1104, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1105, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1106, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1107, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1108, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1109, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1110, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1111, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1112, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1113, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1114, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1115, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1116, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1117, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1118, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1119, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1072, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1073, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1074, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1075, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1076, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1077, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1078, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1079, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1080, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1081, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1082, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1083, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1084, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1085, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1086, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1087, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1088, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1089, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1090, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1091, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1092, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1093, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1094, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1095, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1096, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1097, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1098, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1099, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1100, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1101, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1102, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1103, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1040, -1, 1040, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1041, -1, 1041, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1042, -1, 1042, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1043, -1, 1043, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1044, -1, 1044, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1045, -1, 1045, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1046, -1, 1046, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1047, -1, 1047, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1048, -1, 1048, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1049, -1, 1049, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1050, -1, 1050, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1051, -1, 1051, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1052, -1, 1052, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1053, -1, 1053, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1054, -1, 1054, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1055, -1, 1055, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1056, -1, 1056, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1057, -1, 1057, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1058, -1, 1058, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1059, -1, 1059, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1060, -1, 1060, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1061, -1, 1061, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1062, -1, 1062, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1063, -1, 1063, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1064, -1, 1064, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1065, -1, 1065, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1066, -1, 1066, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1067, -1, 1067, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1068, -1, 1068, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1069, -1, 1069, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1070, -1, 1070, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1071, -1, 1071, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1024, -1, 1024, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1025, -1, 1025, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1026, -1, 1026, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1027, -1, 1027, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1028, -1, 1028, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1029, -1, 1029, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1030, -1, 1030, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1031, -1, 1031, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1032, -1, 1032, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1033, -1, 1033, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1034, -1, 1034, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1035, -1, 1035, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1036, -1, 1036, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1037, -1, 1037, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1038, -1, 1038, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1039, -1, 1039, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1121, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1120, -1, 1120, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1123, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1122, -1, 1122, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1125, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1124, -1, 1124, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1127, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1126, -1, 1126, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1129, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1128, -1, 1128, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1131, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1130, -1, 1130, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1133, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1132, -1, 1132, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1135, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1134, -1, 1134, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1137, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1136, -1, 1136, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1139, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1138, -1, 1138, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1141, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1140, -1, 1140, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1143, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1142, -1, 1142, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1145, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1144, -1, 1144, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1147, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1146, -1, 1146, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1149, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1148, -1, 1148, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1151, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1150, -1, 1150, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1153, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1152, -1, 1152, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_ME, 0, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1163, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1162, -1, 1162, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1165, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1164, -1, 1164, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1167, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1166, -1, 1166, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1169, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1168, -1, 1168, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1171, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1170, -1, 1170, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1173, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1172, -1, 1172, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1175, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1174, -1, 1174, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1177, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1176, -1, 1176, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1179, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1178, -1, 1178, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1181, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1180, -1, 1180, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1183, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1182, -1, 1182, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1185, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1184, -1, 1184, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1187, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1186, -1, 1186, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1189, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1188, -1, 1188, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1191, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1190, -1, 1190, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1193, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1192, -1, 1192, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1195, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1194, -1, 1194, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1197, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1196, -1, 1196, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1199, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1198, -1, 1198, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1201, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1200, -1, 1200, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1203, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1202, -1, 1202, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1205, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1204, -1, 1204, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1207, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1206, -1, 1206, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1209, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1208, -1, 1208, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1211, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1210, -1, 1210, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1213, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1212, -1, 1212, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1215, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1214, -1, 1214, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1231, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1218, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1217, -1, 1217, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1220, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1219, -1, 1219, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1222, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1221, -1, 1221, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1224, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1223, -1, 1223, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1226, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1225, -1, 1225, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1228, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1227, -1, 1227, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1230, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1229, -1, 1229, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1216, -1, 1216, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1233, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1232, -1, 1232, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1235, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1234, -1, 1234, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1237, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1236, -1, 1236, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1239, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1238, -1, 1238, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1241, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1240, -1, 1240, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1243, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1242, -1, 1242, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1245, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1244, -1, 1244, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1247, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1246, -1, 1246, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1249, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1248, -1, 1248, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1251, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1250, -1, 1250, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1253, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1252, -1, 1252, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1255, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1254, -1, 1254, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1257, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1256, -1, 1256, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1259, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1258, -1, 1258, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1261, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1260, -1, 1260, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1263, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1262, -1, 1262, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1265, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1264, -1, 1264, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1267, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1266, -1, 1266, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1269, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1268, -1, 1268, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1271, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1270, -1, 1270, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1273, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1272, -1, 1272, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1275, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1274, -1, 1274, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1277, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1276, -1, 1276, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1279, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1278, -1, 1278, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1281, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1280, -1, 1280, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1283, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1282, -1, 1282, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1285, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1284, -1, 1284, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1287, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1286, -1, 1286, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1289, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1288, -1, 1288, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1291, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1290, -1, 1290, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1293, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1292, -1, 1292, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1295, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1294, -1, 1294, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1297, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1296, -1, 1296, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1299, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1298, -1, 1298, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1301, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1300, -1, 1300, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1303, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1302, -1, 1302, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1305, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1304, -1, 1304, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1307, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1306, -1, 1306, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1309, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1308, -1, 1308, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1311, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1310, -1, 1310, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1313, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1312, -1, 1312, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1315, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1314, -1, 1314, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1317, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1316, -1, 1316, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1319, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1318, -1, 1318, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1321, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1320, -1, 1320, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1323, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1322, -1, 1322, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1325, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1324, -1, 1324, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1327, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1326, -1, 1326, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1377, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1378, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1379, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1380, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1381, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1382, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1383, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1384, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1385, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1386, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1387, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1388, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1389, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1390, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1391, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1392, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1393, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1394, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1395, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1396, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1397, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1398, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1399, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1400, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1401, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1402, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1403, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1404, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1405, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1406, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1407, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1408, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1409, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1410, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1411, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1412, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1413, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 1414, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_PO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1329, -1, 1329, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1330, -1, 1330, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1331, -1, 1331, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1332, -1, 1332, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1333, -1, 1333, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1334, -1, 1334, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1335, -1, 1335, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1336, -1, 1336, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1337, -1, 1337, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1338, -1, 1338, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1339, -1, 1339, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1340, -1, 1340, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1341, -1, 1341, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1342, -1, 1342, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1343, -1, 1343, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1344, -1, 1344, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1345, -1, 1345, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1346, -1, 1346, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1347, -1, 1347, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1348, -1, 1348, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1349, -1, 1349, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1350, -1, 1350, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1351, -1, 1351, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1352, -1, 1352, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1353, -1, 1353, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1354, -1, 1354, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1355, -1, 1355, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1356, -1, 1356, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1357, -1, 1357, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1358, -1, 1358, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1359, -1, 1359, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1360, -1, 1360, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1361, -1, 1361, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1362, -1, 1362, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1363, -1, 1363, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1364, -1, 1364, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1365, -1, 1365, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1366, -1, 1366, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_PD, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 222, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 228, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 10, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 11, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 12, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 13, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 14, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 15, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 16, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 17, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 18, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 19, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 20, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 21, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 22, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_PD, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 23, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_PO, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 24, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 25, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LO, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_CF, 0, SFCE_UNICODE_BIDI_CLASS_AN, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_SM, 0, SFCE_UNICODE_BIDI_CLASS_AL, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_PO, 0, SFCE_UNICODE_BIDI_CLASS_ET, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_SC, 0, SFCE_UNICODE_BIDI_CLASS_AL, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_PO, 0, SFCE_UNICODE_BIDI_CLASS_CS, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_PO, 0, SFCE_UNICODE_BIDI_CLASS_AL, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 30, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 31, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 32, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_CF, 0, SFCE_UNICODE_BIDI_CLASS_AL, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LO, 0, SFCE_UNICODE_BIDI_CLASS_AL, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LM, 0, SFCE_UNICODE_BIDI_CLASS_AL, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 27, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 28, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 29, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 33, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 34, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_ND, 0, SFCE_UNICODE_BIDI_CLASS_AN, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_PO, 0, SFCE_UNICODE_BIDI_CLASS_AN, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 35, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LO, 0, SFCE_UNICODE_BIDI_CLASS_AL, SFCE_UNICODE_DECOMPOSITION_COMPAT, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_ND, 0, SFCE_UNICODE_BIDI_CLASS_EN, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_AL, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 36, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_ND, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LM, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_SC, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 2, 0 },
    { SFCE_UNICODE_CATEGORY_LO, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 230, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LM, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_PO, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 220, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LO, 0, SFCE_UNICODE_BIDI_CLASS_AL, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SK, 0, SFCE_UNICODE_BIDI_CLASS_AL, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_CF, 0, SFCE_UNICODE_BIDI_CLASS_AN, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LM, 0, SFCE_UNICODE_BIDI_CLASS_AL, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 27, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 28, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 29, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 0, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_MC, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 7, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 9, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_PO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_ND, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LM, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SC, 0, SFCE_UNICODE_BIDI_CLASS_ET, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_NO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 84, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 91, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_NO, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 103, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 107, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 118, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 122, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_PO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NOBREAK, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 216, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_PS, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 1 },
    { SFCE_UNICODE_CATEGORY_PE, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 1 },
    { SFCE_UNICODE_CATEGORY_MN, 129, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 130, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 132, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 0, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_COMPAT, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11520, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11521, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11522, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11523, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11524, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11525, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11526, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11527, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11528, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11529, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11530, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11531, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11532, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11533, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11534, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11535, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11536, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11537, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11538, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11539, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11540, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11541, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11542, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11543, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11544, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11545, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11546, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11547, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11548, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11549, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11550, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11551, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11552, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11553, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11554, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11555, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11556, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11557, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11559, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11565, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7312, -1, 4304, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7313, -1, 4305, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7314, -1, 4306, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7315, -1, 4307, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7316, -1, 4308, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7317, -1, 4309, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7318, -1, 4310, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7319, -1, 4311, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7320, -1, 4312, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7321, -1, 4313, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7322, -1, 4314, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7323, -1, 4315, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7324, -1, 4316, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7325, -1, 4317, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7326, -1, 4318, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7327, -1, 4319, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7328, -1, 4320, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7329, -1, 4321, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7330, -1, 4322, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7331, -1, 4323, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7332, -1, 4324, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7333, -1, 4325, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7334, -1, 4326, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7335, -1, 4327, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7336, -1, 4328, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7337, -1, 4329, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7338, -1, 4330, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7339, -1, 4331, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7340, -1, 4332, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7341, -1, 4333, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7342, -1, 4334, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7343, -1, 4335, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7344, -1, 4336, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7345, -1, 4337, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7346, -1, 4338, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7347, -1, 4339, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7348, -1, 4340, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7349, -1, 4341, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7350, -1, 4342, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7351, -1, 4343, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7352, -1, 4344, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7353, -1, 4345, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7354, -1, 4346, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LM, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_SUPER, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7357, -1, 4349, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7358, -1, 4350, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7359, -1, 4351, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43888, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43889, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43890, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43891, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43892, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43893, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43894, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43895, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43896, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43897, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43898, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43899, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43900, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43901, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43902, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43903, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43904, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43905, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43906, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43907, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43908, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43909, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43910, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43911, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43912, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43913, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43914, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43915, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43916, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43917, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43918, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43919, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43920, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43921, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43922, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43923, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43924, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43925, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43926, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43927, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43928, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43929, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43930, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43931, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43932, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43933, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43934, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43935, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43936, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43937, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43938, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43939, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43940, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43941, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43942, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43943, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43944, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43945, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43946, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43947, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43948, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43949, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43950, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43951, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43952, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43953, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43954, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43955, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43956, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43957, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43958, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43959, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43960, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43961, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43962, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43963, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43964, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43965, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43966, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43967, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 5112, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 5113, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 5114, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 5115, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 5116, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 5117, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5104, -1, 5104, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5105, -1, 5105, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5106, -1, 5106, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5107, -1, 5107, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5108, -1, 5108, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5109, -1, 5109, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_PD, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_ZS, 0, SFCE_UNICODE_BIDI_CLASS_WS, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_NL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_MC, 9, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_PO, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_CF, 0, SFCE_UNICODE_BIDI_CLASS_BN, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 228, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 222, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_ME, 0, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1042, -1, 1042, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1044, -1, 1044, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1054, -1, 1054, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1057, -1, 1057, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1058, -1, 1058, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1066, -1, 1066, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 1122, -1, 1122, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42570, -1, 42570, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7306, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7305, -1, 7305, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 4304, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 4305, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 4306, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 4307, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 4308, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 4309, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 4310, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 4311, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 4312, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 4313, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 4314, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 4315, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 4316, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 4317, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 4318, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 4319, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 4320, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 4321, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 4322, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 4323, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 4324, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 4325, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 4326, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 4327, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 4328, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 4329, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 4330, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 4331, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 4332, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 4333, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 4334, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 4335, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 4336, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 4337, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 4338, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 4339, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 4340, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 4341, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 4342, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 4343, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 4344, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 4345, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 4346, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 4349, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 4350, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 4351, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 1, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LM, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_SUB, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42877, -1, 42877, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11363, -1, 11363, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42950, -1, 42950, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 234, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 214, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 202, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 232, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 218, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 233, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7681, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7680, -1, 7680, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7683, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7682, -1, 7682, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7685, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7684, -1, 7684, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7687, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7686, -1, 7686, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7689, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7688, -1, 7688, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7691, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7690, -1, 7690, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7693, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7692, -1, 7692, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7695, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7694, -1, 7694, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7697, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7696, -1, 7696, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7699, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7698, -1, 7698, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7701, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7700, -1, 7700, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7703, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7702, -1, 7702, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7705, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7704, -1, 7704, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7707, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7706, -1, 7706, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7709, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7708, -1, 7708, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7711, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7710, -1, 7710, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7713, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7712, -1, 7712, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7715, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7714, -1, 7714, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7717, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7716, -1, 7716, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7719, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7718, -1, 7718, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7721, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7720, -1, 7720, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7723, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7722, -1, 7722, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7725, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7724, -1, 7724, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7727, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7726, -1, 7726, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7729, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7728, -1, 7728, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7731, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7730, -1, 7730, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7733, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7732, -1, 7732, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7735, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7734, -1, 7734, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7737, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7736, -1, 7736, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7739, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7738, -1, 7738, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7741, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7740, -1, 7740, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7743, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7742, -1, 7742, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7745, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7744, -1, 7744, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7747, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7746, -1, 7746, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7749, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7748, -1, 7748, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7751, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7750, -1, 7750, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7753, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7752, -1, 7752, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7755, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7754, -1, 7754, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7757, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7756, -1, 7756, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7759, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7758, -1, 7758, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7761, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7760, -1, 7760, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7763, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7762, -1, 7762, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7765, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7764, -1, 7764, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7767, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7766, -1, 7766, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7769, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7768, -1, 7768, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7771, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7770, -1, 7770, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7773, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7772, -1, 7772, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7775, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7774, -1, 7774, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7777, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7776, -1, 7776, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7779, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7778, -1, 7778, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7781, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7780, -1, 7780, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7783, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7782, -1, 7782, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7785, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7784, -1, 7784, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7787, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7786, -1, 7786, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7789, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7788, -1, 7788, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7791, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7790, -1, 7790, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7793, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7792, -1, 7792, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7795, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7794, -1, 7794, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7797, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7796, -1, 7796, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7799, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7798, -1, 7798, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7801, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7800, -1, 7800, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7803, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7802, -1, 7802, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7805, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7804, -1, 7804, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7807, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7806, -1, 7806, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7809, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7808, -1, 7808, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7811, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7810, -1, 7810, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7813, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7812, -1, 7812, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7815, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7814, -1, 7814, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7817, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7816, -1, 7816, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7819, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7818, -1, 7818, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7821, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7820, -1, 7820, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7823, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7822, -1, 7822, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7825, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7824, -1, 7824, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7827, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7826, -1, 7826, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7829, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7828, -1, 7828, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 223, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7841, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7840, -1, 7840, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7843, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7842, -1, 7842, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7845, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7844, -1, 7844, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7847, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7846, -1, 7846, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7849, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7848, -1, 7848, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7851, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7850, -1, 7850, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7853, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7852, -1, 7852, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7855, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7854, -1, 7854, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7857, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7856, -1, 7856, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7859, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7858, -1, 7858, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7861, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7860, -1, 7860, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7863, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7862, -1, 7862, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7865, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7864, -1, 7864, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7867, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7866, -1, 7866, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7869, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7868, -1, 7868, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7871, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7870, -1, 7870, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7873, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7872, -1, 7872, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7875, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7874, -1, 7874, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7877, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7876, -1, 7876, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7879, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7878, -1, 7878, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7881, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7880, -1, 7880, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7883, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7882, -1, 7882, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7885, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7884, -1, 7884, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7887, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7886, -1, 7886, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7889, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7888, -1, 7888, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7891, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7890, -1, 7890, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7893, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7892, -1, 7892, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7895, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7894, -1, 7894, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7897, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7896, -1, 7896, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7899, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7898, -1, 7898, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7901, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7900, -1, 7900, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7903, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7902, -1, 7902, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7905, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7904, -1, 7904, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7907, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7906, -1, 7906, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7909, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7908, -1, 7908, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7911, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7910, -1, 7910, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7913, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7912, -1, 7912, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7915, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7914, -1, 7914, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7917, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7916, -1, 7916, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7919, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7918, -1, 7918, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7921, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7920, -1, 7920, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7923, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7922, -1, 7922, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7925, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7924, -1, 7924, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7927, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7926, -1, 7926, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7929, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7928, -1, 7928, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7931, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7930, -1, 7930, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7933, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7932, -1, 7932, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7935, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7934, -1, 7934, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7944, -1, 7944, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7945, -1, 7945, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7946, -1, 7946, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7947, -1, 7947, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7948, -1, 7948, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7949, -1, 7949, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7950, -1, 7950, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7951, -1, 7951, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7936, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7937, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7938, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7939, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7940, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7941, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7942, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7943, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7960, -1, 7960, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7961, -1, 7961, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7962, -1, 7962, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7963, -1, 7963, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7964, -1, 7964, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7965, -1, 7965, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7952, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7953, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7954, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7955, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7956, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7957, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7976, -1, 7976, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7977, -1, 7977, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7978, -1, 7978, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7979, -1, 7979, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7980, -1, 7980, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7981, -1, 7981, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7982, -1, 7982, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7983, -1, 7983, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7968, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7969, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7970, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7971, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7972, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7973, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7974, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7975, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7992, -1, 7992, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7993, -1, 7993, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7994, -1, 7994, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7995, -1, 7995, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7996, -1, 7996, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7997, -1, 7997, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7998, -1, 7998, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 7999, -1, 7999, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7984, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7985, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7986, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7987, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7988, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7989, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7990, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7991, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 8008, -1, 8008, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 8009, -1, 8009, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 8010, -1, 8010, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 8011, -1, 8011, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 8012, -1, 8012, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 8013, -1, 8013, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 8000, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 8001, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 8002, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 8003, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 8004, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 8005, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 8025, -1, 8025, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 8027, -1, 8027, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 8029, -1, 8029, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 8031, -1, 8031, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 8017, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 8019, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 8021, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 8023, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 8040, -1, 8040, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 8041, -1, 8041, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 8042, -1, 8042, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 8043, -1, 8043, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 8044, -1, 8044, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 8045, -1, 8045, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 8046, -1, 8046, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 8047, -1, 8047, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 8032, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 8033, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 8034, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 8035, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 8036, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 8037, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 8038, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 8039, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 8122, -1, 8122, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 8123, -1, 8123, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 8136, -1, 8136, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 8137, -1, 8137, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 8138, -1, 8138, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 8139, -1, 8139, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 8154, -1, 8154, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 8155, -1, 8155, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 8184, -1, 8184, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 8185, -1, 8185, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 8170, -1, 8170, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 8171, -1, 8171, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 8186, -1, 8186, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 8187, -1, 8187, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 8072, -1, 8072, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 8073, -1, 8073, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 8074, -1, 8074, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 8075, -1, 8075, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 8076, -1, 8076, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 8077, -1, 8077, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 8078, -1, 8078, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 8079, -1, 8079, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LT, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 8064, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LT, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 8065, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LT, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 8066, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LT, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 8067, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LT, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 8068, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LT, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 8069, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LT, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 8070, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LT, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 8071, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 8088, -1, 8088, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 8089, -1, 8089, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 8090, -1, 8090, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 8091, -1, 8091, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 8092, -1, 8092, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 8093, -1, 8093, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 8094, -1, 8094, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 8095, -1, 8095, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LT, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 8080, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LT, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 8081, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LT, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 8082, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LT, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 8083, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LT, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 8084, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LT, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 8085, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LT, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 8086, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LT, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 8087, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 8104, -1, 8104, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 8105, -1, 8105, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 8106, -1, 8106, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 8107, -1, 8107, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 8108, -1, 8108, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 8109, -1, 8109, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 8110, -1, 8110, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 8111, -1, 8111, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LT, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 8096, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LT, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 8097, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LT, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 8098, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LT, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 8099, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LT, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 8100, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LT, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 8101, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LT, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 8102, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LT, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 8103, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 8120, -1, 8120, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 8121, -1, 8121, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 8124, -1, 8124, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 8112, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 8113, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 8048, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 8049, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LT, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 8115, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SK, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_COMPAT, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 921, -1, 921, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SK, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 8140, -1, 8140, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 8050, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 8051, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 8052, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 8053, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LT, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 8131, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 8152, -1, 8152, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 8153, -1, 8153, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 8144, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 8145, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 8054, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 8055, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 8168, -1, 8168, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 8169, -1, 8169, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 8172, -1, 8172, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 8160, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 8161, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 8058, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 8059, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 8165, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 8188, -1, 8188, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 8056, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 8057, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 8060, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 8061, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LT, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 8179, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_ZS, 0, SFCE_UNICODE_BIDI_CLASS_WS, SFCE_UNICODE_DECOMPOSITION_COMPAT, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_ZS, 0, SFCE_UNICODE_BIDI_CLASS_WS, SFCE_UNICODE_DECOMPOSITION_NOBREAK, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_CF, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_CF, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_PD, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NOBREAK, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_PO, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_COMPAT, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_PI, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_PF, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_PS, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_ZL, 0, SFCE_UNICODE_BIDI_CLASS_WS, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_ZP, 0, SFCE_UNICODE_BIDI_CLASS_B, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_CF, 0, SFCE_UNICODE_BIDI_CLASS_LRE, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_CF, 0, SFCE_UNICODE_BIDI_CLASS_RLE, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_CF, 0, SFCE_UNICODE_BIDI_CLASS_PDF, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_CF, 0, SFCE_UNICODE_BIDI_CLASS_LRO, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_CF, 0, SFCE_UNICODE_BIDI_CLASS_RLO, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_ZS, 0, SFCE_UNICODE_BIDI_CLASS_CS, SFCE_UNICODE_DECOMPOSITION_NOBREAK, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_PO, 0, SFCE_UNICODE_BIDI_CLASS_ET, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_PO, 0, SFCE_UNICODE_BIDI_CLASS_ET, SFCE_UNICODE_DECOMPOSITION_COMPAT, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_PI, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 1 },
    { SFCE_UNICODE_CATEGORY_PF, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 1 },
    { SFCE_UNICODE_CATEGORY_PC, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SM, 0, SFCE_UNICODE_BIDI_CLASS_CS, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SM, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_CF, 0, SFCE_UNICODE_BIDI_CLASS_LRI, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_CF, 0, SFCE_UNICODE_BIDI_CLASS_RLI, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_CF, 0, SFCE_UNICODE_BIDI_CLASS_FSI, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_CF, 0, SFCE_UNICODE_BIDI_CLASS_PDI, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_NO, 0, SFCE_UNICODE_BIDI_CLASS_EN, SFCE_UNICODE_DECOMPOSITION_SUPER, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SM, 0, SFCE_UNICODE_BIDI_CLASS_ES, SFCE_UNICODE_DECOMPOSITION_SUPER, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SM, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_SUPER, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_PS, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_SUPER, -1, -1, -1, 1, 3, 1 },
    { SFCE_UNICODE_CATEGORY_PE, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_SUPER, -1, -1, -1, 1, 3, 1 },
    { SFCE_UNICODE_CATEGORY_NO, 0, SFCE_UNICODE_BIDI_CLASS_EN, SFCE_UNICODE_DECOMPOSITION_SUB, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SM, 0, SFCE_UNICODE_BIDI_CLASS_ES, SFCE_UNICODE_DECOMPOSITION_SUB, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SM, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_SUB, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_PS, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_SUB, -1, -1, -1, 1, 3, 1 },
    { SFCE_UNICODE_CATEGORY_PE, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_SUB, -1, -1, -1, 1, 3, 1 },
    { SFCE_UNICODE_CATEGORY_SC, 0, SFCE_UNICODE_BIDI_CLASS_ET, SFCE_UNICODE_DECOMPOSITION_COMPAT, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_COMPAT, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_FONT, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_FONT, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_SUPER, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 969, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 107, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 229, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_ET, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 8526, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SM, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_FONT, -1, -1, -1, 1, 3, 1 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 8498, -1, 8498, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_NO, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_FRACTION, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_NL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, -1, 8560, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_NL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, -1, 8561, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_NL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, -1, 8562, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_NL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, -1, 8563, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_NL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, -1, 8564, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_NL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, -1, 8565, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_NL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, -1, 8566, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_NL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, -1, 8567, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_NL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, -1, 8568, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_NL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, -1, 8569, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_NL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, -1, 8570, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_NL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, -1, 8571, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_NL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, -1, 8572, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_NL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, -1, 8573, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_NL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, -1, 8574, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_NL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, -1, 8575, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_NL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 8544, -1, 8544, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_NL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 8545, -1, 8545, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_NL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 8546, -1, 8546, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_NL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 8547, -1, 8547, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_NL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 8548, -1, 8548, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_NL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 8549, -1, 8549, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_NL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 8550, -1, 8550, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_NL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 8551, -1, 8551, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_NL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 8552, -1, 8552, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_NL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 8553, -1, 8553, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_NL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 8554, -1, 8554, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_NL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 8555, -1, 8555, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_NL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 8556, -1, 8556, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_NL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 8557, -1, 8557, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_NL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 8558, -1, 8558, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_NL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 8559, -1, 8559, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 8580, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 8579, -1, 8579, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SM, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 1 },
    { SFCE_UNICODE_CATEGORY_SM, 0, SFCE_UNICODE_BIDI_CLASS_ES, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SM, 0, SFCE_UNICODE_BIDI_CLASS_ET, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SM, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_COMPAT, -1, -1, -1, 1, 3, 1 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_PS, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 2, 3, 1 },
    { SFCE_UNICODE_CATEGORY_PE, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 2, 3, 1 },
    { SFCE_UNICODE_CATEGORY_NO, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_CIRCLE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_NO, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_COMPAT, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_NO, 0, SFCE_UNICODE_BIDI_CLASS_EN, SFCE_UNICODE_DECOMPOSITION_COMPAT, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, -1, 9424, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, -1, 9425, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, -1, 9426, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, -1, 9427, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, -1, 9428, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, -1, 9429, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, -1, 9430, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, -1, 9431, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, -1, 9432, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, -1, 9433, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, -1, 9434, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, -1, 9435, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, -1, 9436, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, -1, 9437, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, -1, 9438, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, -1, 9439, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, -1, 9440, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, -1, 9441, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, -1, 9442, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, -1, 9443, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, -1, 9444, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, -1, 9445, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, -1, 9446, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, -1, 9447, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, -1, 9448, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, -1, 9449, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 9398, -1, 9398, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 9399, -1, 9399, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 9400, -1, 9400, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 9401, -1, 9401, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 9402, -1, 9402, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 9403, -1, 9403, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 9404, -1, 9404, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 9405, -1, 9405, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 9406, -1, 9406, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 9407, -1, 9407, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 9408, -1, 9408, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 9409, -1, 9409, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 9410, -1, 9410, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 9411, -1, 9411, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 9412, -1, 9412, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 9413, -1, 9413, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 9414, -1, 9414, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 9415, -1, 9415, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 9416, -1, 9416, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 9417, -1, 9417, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 9418, -1, 9418, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 9419, -1, 9419, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 9420, -1, 9420, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 9421, -1, 9421, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 9422, -1, 9422, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 9423, -1, 9423, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SM, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SM, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_COMPAT, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 1 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11312, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11313, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11314, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11315, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11316, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11317, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11318, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11319, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11320, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11321, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11322, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11323, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11324, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11325, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11326, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11327, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11328, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11329, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11330, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11331, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11332, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11333, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11334, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11335, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11336, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11337, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11338, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11339, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11340, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11341, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11342, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11343, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11344, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11345, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11346, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11347, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11348, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11349, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11350, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11351, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11352, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11353, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11354, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11355, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11356, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11357, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11358, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11359, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11264, -1, 11264, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11265, -1, 11265, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11266, -1, 11266, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11267, -1, 11267, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11268, -1, 11268, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11269, -1, 11269, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11270, -1, 11270, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11271, -1, 11271, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11272, -1, 11272, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11273, -1, 11273, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11274, -1, 11274, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11275, -1, 11275, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11276, -1, 11276, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11277, -1, 11277, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11278, -1, 11278, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11279, -1, 11279, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11280, -1, 11280, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11281, -1, 11281, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11282, -1, 11282, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11283, -1, 11283, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11284, -1, 11284, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11285, -1, 11285, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11286, -1, 11286, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11287, -1, 11287, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11288, -1, 11288, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11289, -1, 11289, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11290, -1, 11290, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11291, -1, 11291, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11292, -1, 11292, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11293, -1, 11293, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11294, -1, 11294, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11295, -1, 11295, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11296, -1, 11296, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11297, -1, 11297, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11298, -1, 11298, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11299, -1, 11299, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11300, -1, 11300, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11301, -1, 11301, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11302, -1, 11302, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11303, -1, 11303, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11304, -1, 11304, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11305, -1, 11305, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11306, -1, 11306, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11307, -1, 11307, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11308, -1, 11308, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11309, -1, 11309, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11310, -1, 11310, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11311, -1, 11311, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11361, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11360, -1, 11360, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 619, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7549, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 637, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 570, -1, 570, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 574, -1, 574, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11368, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11367, -1, 11367, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11370, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11369, -1, 11369, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11372, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11371, -1, 11371, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 593, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 625, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 592, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 594, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11379, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11378, -1, 11378, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11382, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11381, -1, 11381, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 575, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 576, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11393, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11392, -1, 11392, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11395, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11394, -1, 11394, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11397, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11396, -1, 11396, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11399, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11398, -1, 11398, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11401, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11400, -1, 11400, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11403, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11402, -1, 11402, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11405, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11404, -1, 11404, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11407, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11406, -1, 11406, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11409, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11408, -1, 11408, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11411, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11410, -1, 11410, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11413, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11412, -1, 11412, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11415, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11414, -1, 11414, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11417, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11416, -1, 11416, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11419, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11418, -1, 11418, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11421, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11420, -1, 11420, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11423, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11422, -1, 11422, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11425, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11424, -1, 11424, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11427, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11426, -1, 11426, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11429, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11428, -1, 11428, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11431, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11430, -1, 11430, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11433, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11432, -1, 11432, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11435, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11434, -1, 11434, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11437, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11436, -1, 11436, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11439, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11438, -1, 11438, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11441, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11440, -1, 11440, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11443, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11442, -1, 11442, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11445, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11444, -1, 11444, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11447, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11446, -1, 11446, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11449, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11448, -1, 11448, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11451, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11450, -1, 11450, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11453, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11452, -1, 11452, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11455, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11454, -1, 11454, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11457, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11456, -1, 11456, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11459, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11458, -1, 11458, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11461, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11460, -1, 11460, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11463, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11462, -1, 11462, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11465, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11464, -1, 11464, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11467, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11466, -1, 11466, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11469, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11468, -1, 11468, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11471, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11470, -1, 11470, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11473, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11472, -1, 11472, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11475, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11474, -1, 11474, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11477, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11476, -1, 11476, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11479, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11478, -1, 11478, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11481, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11480, -1, 11480, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11483, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11482, -1, 11482, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11485, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11484, -1, 11484, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11487, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11486, -1, 11486, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11489, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11488, -1, 11488, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11491, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11490, -1, 11490, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11500, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11499, -1, 11499, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11502, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11501, -1, 11501, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 11507, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 11506, -1, 11506, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 4256, -1, 4256, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 4257, -1, 4257, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 4258, -1, 4258, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 4259, -1, 4259, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 4260, -1, 4260, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 4261, -1, 4261, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 4262, -1, 4262, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 4263, -1, 4263, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 4264, -1, 4264, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 4265, -1, 4265, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 4266, -1, 4266, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 4267, -1, 4267, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 4268, -1, 4268, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 4269, -1, 4269, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 4270, -1, 4270, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 4271, -1, 4271, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 4272, -1, 4272, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 4273, -1, 4273, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 4274, -1, 4274, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 4275, -1, 4275, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 4276, -1, 4276, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 4277, -1, 4277, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 4278, -1, 4278, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 4279, -1, 4279, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 4280, -1, 4280, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 4281, -1, 4281, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 4282, -1, 4282, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 4283, -1, 4283, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 4284, -1, 4284, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 4285, -1, 4285, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 4286, -1, 4286, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 4287, -1, 4287, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 4288, -1, 4288, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 4289, -1, 4289, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 4290, -1, 4290, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 4291, -1, 4291, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 4292, -1, 4292, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 4293, -1, 4293, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 4295, -1, 4295, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 4301, -1, 4301, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LM, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_COMPAT, -1, -1, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_ZS, 0, SFCE_UNICODE_BIDI_CLASS_WS, SFCE_UNICODE_DECOMPOSITION_WIDE, -1, -1, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_PO, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LM, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_NL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_PD, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_PS, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_PE, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 218, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 228, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 232, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 222, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_MC, 224, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_NL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, -1, -1, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 8, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SK, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_COMPAT, -1, -1, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_VERTICAL, -1, -1, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, -1, -1, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_NO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_SUPER, -1, -1, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_SUPER, -1, -1, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, -1, -1, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_NO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, -1, -1, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, -1, -1, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_SQUARE, -1, -1, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_NO, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_CIRCLE, -1, -1, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_CIRCLE, -1, -1, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_NO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, -1, -1, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_SQUARE, -1, -1, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42561, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42560, -1, 42560, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42563, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42562, -1, 42562, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42565, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42564, -1, 42564, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42567, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42566, -1, 42566, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42569, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42568, -1, 42568, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42571, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42573, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42572, -1, 42572, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42575, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42574, -1, 42574, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42577, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42576, -1, 42576, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42579, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42578, -1, 42578, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42581, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42580, -1, 42580, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42583, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42582, -1, 42582, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42585, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42584, -1, 42584, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42587, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42586, -1, 42586, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42589, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42588, -1, 42588, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42591, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42590, -1, 42590, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42593, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42592, -1, 42592, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42595, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42594, -1, 42594, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42597, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42596, -1, 42596, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42599, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42598, -1, 42598, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42601, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42600, -1, 42600, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42603, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42602, -1, 42602, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42605, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42604, -1, 42604, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42625, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42624, -1, 42624, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42627, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42626, -1, 42626, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42629, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42628, -1, 42628, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42631, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42630, -1, 42630, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42633, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42632, -1, 42632, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42635, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42634, -1, 42634, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42637, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42636, -1, 42636, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42639, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42638, -1, 42638, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42641, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42640, -1, 42640, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42643, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42642, -1, 42642, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42645, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42644, -1, 42644, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42647, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42646, -1, 42646, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42649, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42648, -1, 42648, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42651, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42650, -1, 42650, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42787, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42786, -1, 42786, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42789, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42788, -1, 42788, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42791, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42790, -1, 42790, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42793, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42792, -1, 42792, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42795, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42794, -1, 42794, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42797, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42796, -1, 42796, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42799, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42798, -1, 42798, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42803, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42802, -1, 42802, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42805, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42804, -1, 42804, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42807, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42806, -1, 42806, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42809, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42808, -1, 42808, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42811, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42810, -1, 42810, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42813, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42812, -1, 42812, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42815, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42814, -1, 42814, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42817, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42816, -1, 42816, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42819, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42818, -1, 42818, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42821, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42820, -1, 42820, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42823, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42822, -1, 42822, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42825, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42824, -1, 42824, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42827, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42826, -1, 42826, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42829, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42828, -1, 42828, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42831, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42830, -1, 42830, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42833, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42832, -1, 42832, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42835, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42834, -1, 42834, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42837, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42836, -1, 42836, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42839, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42838, -1, 42838, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42841, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42840, -1, 42840, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42843, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42842, -1, 42842, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42845, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42844, -1, 42844, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42847, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42846, -1, 42846, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42849, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42848, -1, 42848, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42851, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42850, -1, 42850, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42853, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42852, -1, 42852, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42855, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42854, -1, 42854, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42857, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42856, -1, 42856, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42859, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42858, -1, 42858, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42861, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42860, -1, 42860, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42863, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42862, -1, 42862, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42874, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42873, -1, 42873, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42876, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42875, -1, 42875, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7545, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42879, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42878, -1, 42878, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42881, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42880, -1, 42880, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42883, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42882, -1, 42882, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42885, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42884, -1, 42884, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42887, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42886, -1, 42886, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SK, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42892, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42891, -1, 42891, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 613, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42897, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42896, -1, 42896, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42899, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42898, -1, 42898, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42948, -1, 42948, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42903, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42902, -1, 42902, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42905, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42904, -1, 42904, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42907, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42906, -1, 42906, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42909, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42908, -1, 42908, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42911, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42910, -1, 42910, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42913, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42912, -1, 42912, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42915, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42914, -1, 42914, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42917, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42916, -1, 42916, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42919, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42918, -1, 42918, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42921, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42920, -1, 42920, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 614, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 604, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 609, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 620, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 618, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 670, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 647, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 669, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 43859, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42933, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42932, -1, 42932, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42935, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42934, -1, 42934, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42937, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42936, -1, 42936, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42939, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42938, -1, 42938, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42941, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42940, -1, 42940, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42943, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42942, -1, 42942, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42945, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42944, -1, 42944, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42947, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42946, -1, 42946, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42900, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 642, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 7566, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42952, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42951, -1, 42951, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42954, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42953, -1, 42953, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 612, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42957, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42956, -1, 42956, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42961, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42960, -1, 42960, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42967, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42966, -1, 42966, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42969, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42968, -1, 42968, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42971, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42970, -1, 42970, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 411, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 42998, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42997, -1, 42997, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 42931, -1, 42931, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5024, -1, 5024, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5025, -1, 5025, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5026, -1, 5026, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5027, -1, 5027, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5028, -1, 5028, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5029, -1, 5029, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5030, -1, 5030, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5031, -1, 5031, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5032, -1, 5032, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5033, -1, 5033, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5034, -1, 5034, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5035, -1, 5035, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5036, -1, 5036, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5037, -1, 5037, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5038, -1, 5038, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5039, -1, 5039, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5040, -1, 5040, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5041, -1, 5041, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5042, -1, 5042, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5043, -1, 5043, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5044, -1, 5044, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5045, -1, 5045, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5046, -1, 5046, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5047, -1, 5047, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5048, -1, 5048, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5049, -1, 5049, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5050, -1, 5050, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5051, -1, 5051, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5052, -1, 5052, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5053, -1, 5053, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5054, -1, 5054, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5055, -1, 5055, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5056, -1, 5056, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5057, -1, 5057, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5058, -1, 5058, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5059, -1, 5059, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5060, -1, 5060, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5061, -1, 5061, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5062, -1, 5062, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5063, -1, 5063, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5064, -1, 5064, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5065, -1, 5065, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5066, -1, 5066, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5067, -1, 5067, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5068, -1, 5068, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5069, -1, 5069, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5070, -1, 5070, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5071, -1, 5071, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5072, -1, 5072, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5073, -1, 5073, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5074, -1, 5074, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5075, -1, 5075, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5076, -1, 5076, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5077, -1, 5077, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5078, -1, 5078, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5079, -1, 5079, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5080, -1, 5080, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5081, -1, 5081, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5082, -1, 5082, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5083, -1, 5083, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5084, -1, 5084, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5085, -1, 5085, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5086, -1, 5086, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5087, -1, 5087, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5088, -1, 5088, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5089, -1, 5089, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5090, -1, 5090, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5091, -1, 5091, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5092, -1, 5092, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5093, -1, 5093, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5094, -1, 5094, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5095, -1, 5095, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5096, -1, 5096, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5097, -1, 5097, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5098, -1, 5098, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5099, -1, 5099, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5100, -1, 5100, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5101, -1, 5101, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5102, -1, 5102, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 5103, -1, 5103, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_CS, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_CO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 26, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LO, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_FONT, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SM, 0, SFCE_UNICODE_BIDI_CLASS_ES, SFCE_UNICODE_DECOMPOSITION_FONT, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LO, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_COMPAT, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LO, 0, SFCE_UNICODE_BIDI_CLASS_AL, SFCE_UNICODE_DECOMPOSITION_ISOLATED, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LO, 0, SFCE_UNICODE_BIDI_CLASS_AL, SFCE_UNICODE_DECOMPOSITION_FINAL, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LO, 0, SFCE_UNICODE_BIDI_CLASS_AL, SFCE_UNICODE_DECOMPOSITION_INITIAL, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LO, 0, SFCE_UNICODE_BIDI_CLASS_AL, SFCE_UNICODE_DECOMPOSITION_MEDIAL, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_PE, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SC, 0, SFCE_UNICODE_BIDI_CLASS_AL, SFCE_UNICODE_DECOMPOSITION_ISOLATED, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_PO, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_VERTICAL, -1, -1, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_PS, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_VERTICAL, -1, -1, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_PE, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_VERTICAL, -1, -1, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_PD, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_VERTICAL, -1, -1, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_PC, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_VERTICAL, -1, -1, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_PO, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_COMPAT, -1, -1, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_PC, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_COMPAT, -1, -1, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_PO, 0, SFCE_UNICODE_BIDI_CLASS_CS, SFCE_UNICODE_DECOMPOSITION_SMALL, -1, -1, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_PO, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_SMALL, -1, -1, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_PD, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_SMALL, -1, -1, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_PS, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_SMALL, -1, -1, -1, 2, 3, 1 },
    { SFCE_UNICODE_CATEGORY_PE, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_SMALL, -1, -1, -1, 2, 3, 1 },
    { SFCE_UNICODE_CATEGORY_PO, 0, SFCE_UNICODE_BIDI_CLASS_ET, SFCE_UNICODE_DECOMPOSITION_SMALL, -1, -1, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SM, 0, SFCE_UNICODE_BIDI_CLASS_ES, SFCE_UNICODE_DECOMPOSITION_SMALL, -1, -1, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_PD, 0, SFCE_UNICODE_BIDI_CLASS_ES, SFCE_UNICODE_DECOMPOSITION_SMALL, -1, -1, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SM, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_SMALL, -1, -1, -1, 2, 3, 1 },
    { SFCE_UNICODE_CATEGORY_SM, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_SMALL, -1, -1, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SC, 0, SFCE_UNICODE_BIDI_CLASS_ET, SFCE_UNICODE_DECOMPOSITION_SMALL, -1, -1, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_PO, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_WIDE, -1, -1, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_PO, 0, SFCE_UNICODE_BIDI_CLASS_ET, SFCE_UNICODE_DECOMPOSITION_WIDE, -1, -1, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SC, 0, SFCE_UNICODE_BIDI_CLASS_ET, SFCE_UNICODE_DECOMPOSITION_WIDE, -1, -1, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_PS, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_WIDE, -1, -1, -1, 2, 3, 1 },
    { SFCE_UNICODE_CATEGORY_PE, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_WIDE, -1, -1, -1, 2, 3, 1 },
    { SFCE_UNICODE_CATEGORY_SM, 0, SFCE_UNICODE_BIDI_CLASS_ES, SFCE_UNICODE_DECOMPOSITION_WIDE, -1, -1, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_PO, 0, SFCE_UNICODE_BIDI_CLASS_CS, SFCE_UNICODE_DECOMPOSITION_WIDE, -1, -1, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_PD, 0, SFCE_UNICODE_BIDI_CLASS_ES, SFCE_UNICODE_DECOMPOSITION_WIDE, -1, -1, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_ND, 0, SFCE_UNICODE_BIDI_CLASS_EN, SFCE_UNICODE_DECOMPOSITION_WIDE, -1, -1, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SM, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_WIDE, -1, -1, -1, 2, 3, 1 },
    { SFCE_UNICODE_CATEGORY_SM, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_WIDE, -1, -1, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, -1, 65345, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, -1, 65346, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, -1, 65347, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, -1, 65348, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, -1, 65349, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, -1, 65350, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, -1, 65351, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, -1, 65352, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, -1, 65353, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, -1, 65354, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, -1, 65355, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, -1, 65356, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, -1, 65357, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, -1, 65358, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, -1, 65359, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, -1, 65360, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, -1, 65361, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, -1, 65362, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, -1, 65363, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, -1, 65364, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, -1, 65365, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, -1, 65366, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, -1, 65367, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, -1, 65368, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, -1, 65369, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, -1, 65370, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SK, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_WIDE, -1, -1, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_PC, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_WIDE, -1, -1, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 65313, -1, 65313, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 65314, -1, 65314, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 65315, -1, 65315, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 65316, -1, 65316, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 65317, -1, 65317, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 65318, -1, 65318, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 65319, -1, 65319, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 65320, -1, 65320, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 65321, -1, 65321, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 65322, -1, 65322, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 65323, -1, 65323, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 65324, -1, 65324, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 65325, -1, 65325, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 65326, -1, 65326, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 65327, -1, 65327, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 65328, -1, 65328, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 65329, -1, 65329, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 65330, -1, 65330, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 65331, -1, 65331, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 65332, -1, 65332, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 65333, -1, 65333, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 65334, -1, 65334, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 65335, -1, 65335, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 65336, -1, 65336, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 65337, -1, 65337, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 65338, -1, 65338, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_PO, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NARROW, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_PS, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NARROW, -1, -1, -1, 1, 3, 1 },
    { SFCE_UNICODE_CATEGORY_PE, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NARROW, -1, -1, -1, 1, 3, 1 },
    { SFCE_UNICODE_CATEGORY_LO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NARROW, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LM, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NARROW, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_WIDE, -1, -1, -1, 2, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NARROW, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_SM, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NARROW, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_CF, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 3, 0 },
    { SFCE_UNICODE_CATEGORY_LO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_PO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_PO, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_NO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_NL, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_NO, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 220, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_NO, 0, SFCE_UNICODE_BIDI_CLASS_EN, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_NL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 230, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66600, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66601, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66602, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66603, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66604, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66605, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66606, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66607, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66608, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66609, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66610, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66611, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66612, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66613, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66614, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66615, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66616, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66617, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66618, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66619, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66620, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66621, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66622, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66623, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66624, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66625, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66626, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66627, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66628, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66629, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66630, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66631, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66632, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66633, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66634, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66635, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66636, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66637, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66638, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66639, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66560, -1, 66560, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66561, -1, 66561, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66562, -1, 66562, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66563, -1, 66563, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66564, -1, 66564, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66565, -1, 66565, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66566, -1, 66566, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66567, -1, 66567, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66568, -1, 66568, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66569, -1, 66569, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66570, -1, 66570, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66571, -1, 66571, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66572, -1, 66572, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66573, -1, 66573, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66574, -1, 66574, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66575, -1, 66575, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66576, -1, 66576, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66577, -1, 66577, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66578, -1, 66578, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66579, -1, 66579, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66580, -1, 66580, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66581, -1, 66581, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66582, -1, 66582, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66583, -1, 66583, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66584, -1, 66584, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66585, -1, 66585, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66586, -1, 66586, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66587, -1, 66587, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66588, -1, 66588, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66589, -1, 66589, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66590, -1, 66590, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66591, -1, 66591, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66592, -1, 66592, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66593, -1, 66593, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66594, -1, 66594, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66595, -1, 66595, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66596, -1, 66596, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66597, -1, 66597, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66598, -1, 66598, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66599, -1, 66599, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_ND, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66776, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66777, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66778, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66779, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66780, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66781, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66782, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66783, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66784, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66785, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66786, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66787, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66788, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66789, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66790, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66791, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66792, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66793, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66794, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66795, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66796, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66797, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66798, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66799, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66800, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66801, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66802, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66803, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66804, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66805, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66806, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66807, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66808, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66809, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66810, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66811, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66736, -1, 66736, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66737, -1, 66737, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66738, -1, 66738, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66739, -1, 66739, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66740, -1, 66740, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66741, -1, 66741, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66742, -1, 66742, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66743, -1, 66743, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66744, -1, 66744, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66745, -1, 66745, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66746, -1, 66746, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66747, -1, 66747, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66748, -1, 66748, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66749, -1, 66749, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66750, -1, 66750, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66751, -1, 66751, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66752, -1, 66752, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66753, -1, 66753, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66754, -1, 66754, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66755, -1, 66755, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66756, -1, 66756, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66757, -1, 66757, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66758, -1, 66758, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66759, -1, 66759, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66760, -1, 66760, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66761, -1, 66761, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66762, -1, 66762, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66763, -1, 66763, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66764, -1, 66764, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66765, -1, 66765, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66766, -1, 66766, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66767, -1, 66767, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66768, -1, 66768, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66769, -1, 66769, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66770, -1, 66770, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66771, -1, 66771, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66967, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66968, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66969, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66970, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66971, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66972, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66973, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66974, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66975, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66976, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66977, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66979, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66980, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66981, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66982, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66983, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66984, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66985, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66986, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66987, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66988, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66989, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66990, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66991, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66992, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66993, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66995, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66996, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66997, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66998, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 66999, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 67000, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 67001, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 67003, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 67004, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66928, -1, 66928, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66929, -1, 66929, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66930, -1, 66930, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66931, -1, 66931, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66932, -1, 66932, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66933, -1, 66933, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66934, -1, 66934, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66935, -1, 66935, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66936, -1, 66936, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66937, -1, 66937, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66938, -1, 66938, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66940, -1, 66940, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66941, -1, 66941, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66942, -1, 66942, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66943, -1, 66943, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66944, -1, 66944, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66945, -1, 66945, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66946, -1, 66946, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66947, -1, 66947, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66948, -1, 66948, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66949, -1, 66949, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66950, -1, 66950, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66951, -1, 66951, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66952, -1, 66952, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66953, -1, 66953, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66954, -1, 66954, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66956, -1, 66956, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66957, -1, 66957, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66958, -1, 66958, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66959, -1, 66959, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66960, -1, 66960, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66961, -1, 66961, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66962, -1, 66962, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66964, -1, 66964, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 66965, -1, 66965, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LM, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LM, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_SUPER, -1, -1, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LO, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_PO, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_NO, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 0, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 1, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 9, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68800, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68801, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68802, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68803, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68804, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68805, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68806, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68807, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68808, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68809, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68810, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68811, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68812, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68813, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68814, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68815, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68816, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68817, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68818, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68819, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68820, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68821, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68822, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68823, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68824, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68825, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68826, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68827, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68828, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68829, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68830, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68831, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68832, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68833, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68834, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68835, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68836, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68837, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68838, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68839, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68840, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68841, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68842, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68843, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68844, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68845, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68846, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68847, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68848, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68849, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68850, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68736, -1, 68736, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68737, -1, 68737, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68738, -1, 68738, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68739, -1, 68739, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68740, -1, 68740, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68741, -1, 68741, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68742, -1, 68742, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68743, -1, 68743, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68744, -1, 68744, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68745, -1, 68745, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68746, -1, 68746, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68747, -1, 68747, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68748, -1, 68748, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68749, -1, 68749, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68750, -1, 68750, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68751, -1, 68751, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68752, -1, 68752, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68753, -1, 68753, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68754, -1, 68754, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68755, -1, 68755, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68756, -1, 68756, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68757, -1, 68757, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68758, -1, 68758, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68759, -1, 68759, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68760, -1, 68760, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68761, -1, 68761, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68762, -1, 68762, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68763, -1, 68763, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68764, -1, 68764, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68765, -1, 68765, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68766, -1, 68766, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68767, -1, 68767, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68768, -1, 68768, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68769, -1, 68769, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68770, -1, 68770, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68771, -1, 68771, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68772, -1, 68772, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68773, -1, 68773, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68774, -1, 68774, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68775, -1, 68775, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68776, -1, 68776, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68777, -1, 68777, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68778, -1, 68778, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68779, -1, 68779, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68780, -1, 68780, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68781, -1, 68781, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68782, -1, 68782, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68783, -1, 68783, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68784, -1, 68784, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68785, -1, 68785, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68786, -1, 68786, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LO, 0, SFCE_UNICODE_BIDI_CLASS_AL, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_ND, 0, SFCE_UNICODE_BIDI_CLASS_AN, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LM, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68976, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68977, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68978, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68979, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68980, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68981, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68982, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68983, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68984, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68985, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68986, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68987, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68988, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68989, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68990, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68991, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68992, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68993, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68994, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68995, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68996, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 68997, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_PD, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68944, -1, 68944, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68945, -1, 68945, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68946, -1, 68946, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68947, -1, 68947, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68948, -1, 68948, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68949, -1, 68949, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68950, -1, 68950, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68951, -1, 68951, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68952, -1, 68952, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68953, -1, 68953, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68954, -1, 68954, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68955, -1, 68955, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68956, -1, 68956, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68957, -1, 68957, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68958, -1, 68958, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68959, -1, 68959, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68960, -1, 68960, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68961, -1, 68961, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68962, -1, 68962, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68963, -1, 68963, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68964, -1, 68964, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 68965, -1, 68965, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_SM, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_NO, 0, SFCE_UNICODE_BIDI_CLASS_AN, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_PD, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_NO, 0, SFCE_UNICODE_BIDI_CLASS_AL, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_PO, 0, SFCE_UNICODE_BIDI_CLASS_AL, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_MC, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 7, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_CF, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_MC, 9, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 71872, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 71873, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 71874, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 71875, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 71876, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 71877, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 71878, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 71879, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 71880, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 71881, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 71882, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 71883, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 71884, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 71885, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 71886, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 71887, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 71888, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 71889, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 71890, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 71891, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 71892, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 71893, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 71894, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 71895, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 71896, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 71897, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 71898, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 71899, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 71900, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 71901, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 71902, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 71903, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 71840, -1, 71840, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 71841, -1, 71841, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 71842, -1, 71842, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 71843, -1, 71843, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 71844, -1, 71844, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 71845, -1, 71845, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 71846, -1, 71846, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 71847, -1, 71847, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 71848, -1, 71848, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 71849, -1, 71849, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 71850, -1, 71850, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 71851, -1, 71851, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 71852, -1, 71852, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 71853, -1, 71853, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 71854, -1, 71854, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 71855, -1, 71855, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 71856, -1, 71856, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 71857, -1, 71857, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 71858, -1, 71858, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 71859, -1, 71859, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 71860, -1, 71860, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 71861, -1, 71861, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 71862, -1, 71862, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 71863, -1, 71863, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 71864, -1, 71864, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 71865, -1, 71865, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 71866, -1, 71866, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 71867, -1, 71867, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 71868, -1, 71868, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 71869, -1, 71869, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 71870, -1, 71870, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 71871, -1, 71871, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 9, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_SC, 0, SFCE_UNICODE_BIDI_CLASS_ET, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 93792, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 93793, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 93794, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 93795, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 93796, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 93797, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 93798, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 93799, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 93800, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 93801, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 93802, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 93803, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 93804, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 93805, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 93806, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 93807, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 93808, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 93809, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 93810, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 93811, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 93812, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 93813, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 93814, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 93815, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 93816, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 93817, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 93818, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 93819, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 93820, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 93821, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 93822, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 93823, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 93760, -1, 93760, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 93761, -1, 93761, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 93762, -1, 93762, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 93763, -1, 93763, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 93764, -1, 93764, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 93765, -1, 93765, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 93766, -1, 93766, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 93767, -1, 93767, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 93768, -1, 93768, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 93769, -1, 93769, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 93770, -1, 93770, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 93771, -1, 93771, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 93772, -1, 93772, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 93773, -1, 93773, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 93774, -1, 93774, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 93775, -1, 93775, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 93776, -1, 93776, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 93777, -1, 93777, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 93778, -1, 93778, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 93779, -1, 93779, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 93780, -1, 93780, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 93781, -1, 93781, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 93782, -1, 93782, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 93783, -1, 93783, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 93784, -1, 93784, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 93785, -1, 93785, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 93786, -1, 93786, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 93787, -1, 93787, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 93788, -1, 93788, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 93789, -1, 93789, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 93790, -1, 93790, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 93791, -1, 93791, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LM, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 2, 4, 0 },
    { SFCE_UNICODE_CATEGORY_PO, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 2, 4, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 0, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 2, 4, 0 },
    { SFCE_UNICODE_CATEGORY_MC, 6, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 2, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 2, 4, 0 },
    { SFCE_UNICODE_CATEGORY_CF, 0, SFCE_UNICODE_BIDI_CLASS_BN, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_FONT, -1, -1, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_ND, 0, SFCE_UNICODE_BIDI_CLASS_EN, SFCE_UNICODE_DECOMPOSITION_FONT, -1, -1, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_MC, 216, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_MC, 226, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 2, 4, 0 },
    { SFCE_UNICODE_CATEGORY_NO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 2, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_FONT, -1, -1, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_FONT, -1, -1, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_SM, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_FONT, -1, -1, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_SM, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_FONT, -1, -1, -1, 1, 4, 1 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LM, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_SUB, -1, -1, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_MN, 232, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 125218, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 125219, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 125220, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 125221, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 125222, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 125223, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 125224, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 125225, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 125226, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 125227, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 125228, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 125229, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 125230, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 125231, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 125232, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 125233, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 125234, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 125235, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 125236, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 125237, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 125238, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 125239, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 125240, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 125241, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 125242, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 125243, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 125244, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 125245, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 125246, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 125247, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 125248, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 125249, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 125250, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LU, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, 125251, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 125184, -1, 125184, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 125185, -1, 125185, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 125186, -1, 125186, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 125187, -1, 125187, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 125188, -1, 125188, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 125189, -1, 125189, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 125190, -1, 125190, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 125191, -1, 125191, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 125192, -1, 125192, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 125193, -1, 125193, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 125194, -1, 125194, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 125195, -1, 125195, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 125196, -1, 125196, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 125197, -1, 125197, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 125198, -1, 125198, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 125199, -1, 125199, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 125200, -1, 125200, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 125201, -1, 125201, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 125202, -1, 125202, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 125203, -1, 125203, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 125204, -1, 125204, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 125205, -1, 125205, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 125206, -1, 125206, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 125207, -1, 125207, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 125208, -1, 125208, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 125209, -1, 125209, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 125210, -1, 125210, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 125211, -1, 125211, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 125212, -1, 125212, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 125213, -1, 125213, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 125214, -1, 125214, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 125215, -1, 125215, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 125216, -1, 125216, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LL, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 125217, -1, 125217, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_ND, 0, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_AL, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_SC, 0, SFCE_UNICODE_BIDI_CLASS_AL, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_LO, 0, SFCE_UNICODE_BIDI_CLASS_AL, SFCE_UNICODE_DECOMPOSITION_FONT, -1, -1, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_SM, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_NO, 0, SFCE_UNICODE_BIDI_CLASS_EN, SFCE_UNICODE_DECOMPOSITION_COMPAT, -1, -1, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, -1, -1, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, -1, -1, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_SQUARE, -1, -1, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_SUPER, -1, -1, -1, 1, 4, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 2, 4, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_SQUARE, -1, -1, -1, 2, 4, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, -1, -1, -1, 2, 4, 0 },
    { SFCE_UNICODE_CATEGORY_SO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, -1, -1, -1, 2, 4, 0 },
    { SFCE_UNICODE_CATEGORY_SK, 0, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 2, 4, 0 },
    { SFCE_UNICODE_CATEGORY_CO, 0, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, -1, -1, -1, 1, 4, 0 },
};

int32_t utf8_property_indices[37888] = {
    0,    0,    0,    0,    0,    0,    0,    0,    0,    1,    2,    1,
    3,    2,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    2,    2,    2,    1,    4,    5,    5,    6,
    7,    6,    5,    5,    8,    9,    5,    10,   11,   12,   11,   11,
    13,   13,   13,   13,   13,   13,   13,   13,   13,   13,   11,   5,
    14,   15,   14,   5,    5,    16,   17,   18,   19,   20,   21,   22,
    23,   24,   25,   26,   27,   28,   29,   30,   31,   32,   33,   34,
    35,   36,   37,   38,   39,   40,   41,   8,    5,    9,    42,   43,
    42,   44,   45,   46,   47,   48,   49,   50,   51,   52,   53,   54,
    55,   56,   57,   58,   59,   60,   61,   62,   63,   64,   65,   66,
    67,   68,   69,   8,    15,   9,    15,   0,    70,   70,   70,   70,
    70,   71,   70,   70,   70,   70,   70,   70,   70,   70,   70,   70,
    70,   70,   70,   70,   70,   70,   70,   70,   70,   70,   70,   70,
    70,   70,   70,   70,   72,   73,   74,   74,   74,   74,   75,   73,
    76,   75,   77,   78,   79,   80,   75,   76,   81,   82,   83,   83,
    76,   84,   73,   73,   76,   83,   77,   85,   86,   86,   86,   73,
    87,   88,   89,   90,   91,   92,   93,   94,   95,   96,   97,   98,
    99,   100,  101,  102,  103,  104,  105,  106,  107,  108,  109,  79,
    110,  111,  112,  113,  114,  115,  116,  117,  118,  119,  120,  121,
    122,  123,  124,  125,  126,  127,  128,  129,  130,  131,  132,  133,
    134,  135,  136,  137,  138,  139,  140,  79,   141,  142,  143,  144,
    145,  146,  147,  148,  149,  150,  151,  152,  153,  154,  155,  156,
    157,  158,  159,  160,  161,  162,  163,  164,  165,  166,  167,  168,
    169,  170,  171,  172,  173,  174,  175,  176,  177,  178,  179,  180,
    181,  182,  183,  184,  185,  186,  187,  188,  189,  190,  191,  192,
    193,  194,  195,  196,  197,  198,  199,  200,  201,  202,  203,  204,
    117,  205,  206,  207,  208,  209,  210,  211,  212,  213,  214,  215,
    216,  217,  218,  219,  220,  221,  222,  223,  224,  225,  226,  227,
    228,  229,  230,  231,  232,  233,  234,  235,  236,  237,  238,  239,
    240,  241,  242,  243,  244,  245,  246,  247,  248,  249,  250,  251,
    252,  253,  254,  255,  256,  257,  258,  259,  260,  261,  262,  263,
    264,  265,  266,  267,  268,  269,  270,  271,  272,  273,  274,  275,
    276,  277,  278,  279,  280,  281,  282,  283,  284,  285,  286,  287,
    288,  117,  289,  290,  291,  292,  293,  294,  295,  296,  297,  298,
    299,  300,  301,  302,  303,  304,  305,  306,  307,  308,  309,  310,
    311,  312,  313,  314,  315,  316,  117,  117,  317,  318,  319,  320,
    321,  322,  323,  324,  325,  326,  327,  328,  329,  330,  117,  331,
    332,  333,  117,  334,  331,  331,  331,  331,  335,  336,  337,  338,
    339,  340,  341,  342,  343,  344,  345,  346,  347,  348,  349,  350,
    351,  352,  353,  354,  355,  356,  357,  358,  359,  360,  361,  362,
    363,  364,  365,  366,  367,  368,  369,  370,  371,  372,  373,  374,
    375,  376,  377,  378,  117,  379,  380,  381,  382,  383,  384,  385,
    386,  387,  388,  389,  390,  391,  392,  393,  394,  395,  396,  397,
    398,  399,  400,  401,  402,  403,  404,  405,  406,  407,  408,  409,
    410,  411,  412,  413,  414,  415,  416,  417,  418,  419,  420,  421,
    422,  423,  424,  425,  426,  117,  427,  428,  429,  430,  431,  432,
    433,  434,  435,  436,  437,  438,  439,  440,  441,  442,  443,  444,
    117,  117,  117,  117,  117,  117,  445,  446,  447,  448,  449,  450,
    451,  452,  453,  454,  455,  456,  457,  458,  459,  460,  461,  462,
    463,  464,  465,  466,  467,  468,  469,  470,  471,  117,  472,  473,
    117,  474,  117,  475,  476,  117,  117,  117,  477,  478,  117,  479,
    480,  481,  482,  117,  483,  484,  485,  486,  487,  117,  117,  488,
    117,  489,  490,  117,  117,  491,  117,  117,  117,  117,  117,  117,
    117,  492,  117,  117,  493,  117,  494,  495,  117,  117,  117,  496,
    497,  498,  499,  500,  501,  117,  117,  117,  117,  117,  502,  117,
    331,  117,  117,  117,  117,  117,  117,  117,  117,  503,  504,  117,
    117,  117,  117,  117,  117,  117,  117,  117,  117,  117,  117,  117,
    117,  117,  117,  117,  505,  505,  505,  505,  505,  505,  505,  505,
    505,  506,  506,  507,  507,  507,  507,  507,  507,  507,  508,  508,
    508,  508,  506,  506,  506,  506,  506,  506,  506,  506,  506,  506,
    507,  507,  508,  508,  508,  508,  508,  508,  76,   76,   76,   76,
    76,   76,   508,  508,  505,  505,  505,  505,  505,  508,  508,  508,
    508,  508,  508,  508,  506,  508,  507,  508,  508,  508,  508,  508,
    508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,
    509,  509,  509,  509,  509,  509,  509,  509,  509,  509,  509,  509,
    509,  509,  509,  509,  509,  509,  509,  509,  509,  510,  511,  511,
    511,  511,  510,  512,  511,  511,  511,  511,  511,  513,  513,  511,
    511,  511,  511,  513,  513,  511,  511,  511,  511,  511,  511,  511,
    511,  511,  511,  511,  514,  514,  514,  514,  514,  511,  511,  511,
    511,  509,  509,  509,  509,  509,  509,  509,  509,  515,  509,  511,
    511,  511,  509,  509,  509,  511,  511,  516,  509,  509,  509,  511,
    511,  511,  511,  509,  510,  511,  511,  509,  517,  518,  518,  517,
    518,  518,  517,  509,  509,  509,  509,  509,  509,  509,  509,  509,
    509,  509,  509,  509,  519,  520,  521,  522,  506,  508,  523,  524,
    525,  525,  526,  527,  528,  529,  73,   530,  525,  525,  525,  525,
    76,   508,  531,  73,   532,  533,  534,  525,  535,  525,  536,  537,
    117,  538,  539,  540,  541,  542,  543,  544,  545,  546,  547,  548,
    549,  550,  551,  552,  553,  554,  525,  555,  556,  557,  558,  559,
    560,  561,  562,  563,  564,  565,  566,  567,  117,  568,  569,  570,
    571,  572,  573,  574,  575,  576,  577,  578,  579,  580,  581,  582,
    583,  584,  585,  585,  586,  587,  588,  589,  590,  591,  592,  593,
    594,  595,  596,  597,  598,  599,  600,  601,  601,  602,  603,  604,
    605,  606,  607,  608,  609,  610,  611,  612,  613,  614,  615,  616,
    617,  618,  619,  620,  621,  622,  623,  624,  625,  626,  627,  628,
    629,  630,  631,  632,  633,  634,  79,   635,  636,  637,  638,  639,
    117,  640,  641,  642,  643,  644,  645,  646,  647,  648,  649,  650,
    651,  652,  653,  654,  655,  656,  657,  658,  659,  660,  661,  662,
    663,  664,  665,  666,  667,  668,  669,  670,  671,  672,  673,  674,
    675,  676,  677,  678,  679,  680,  681,  682,  683,  684,  685,  686,
    687,  688,  689,  690,  691,  692,  693,  694,  695,  696,  697,  698,
    699,  700,  701,  702,  703,  704,  705,  706,  707,  708,  709,  710,
    711,  712,  713,  714,  715,  716,  717,  718,  719,  720,  721,  722,
    723,  724,  725,  726,  727,  728,  729,  730,  731,  732,  733,  734,
    735,  736,  737,  738,  739,  740,  741,  742,  743,  744,  745,  746,
    747,  748,  749,  750,  751,  752,  753,  754,  755,  756,  757,  758,
    759,  760,  761,  762,  763,  764,  765,  766,  767,  768,  769,  770,
    771,  772,  773,  509,  509,  509,  509,  509,  774,  774,  775,  776,
    777,  778,  779,  780,  781,  782,  783,  784,  785,  786,  787,  788,
    789,  790,  791,  792,  793,  794,  795,  796,  797,  798,  799,  800,
    801,  802,  803,  804,  805,  806,  807,  808,  809,  810,  811,  812,
    813,  814,  815,  816,  817,  818,  819,  820,  821,  822,  823,  824,
    825,  826,  827,  828,  829,  830,  831,  832,  833,  834,  835,  836,
    837,  838,  839,  840,  841,  842,  843,  844,  845,  846,  847,  848,
    849,  850,  851,  852,  853,  854,  855,  856,  857,  858,  859,  860,
    861,  862,  863,  864,  865,  866,  867,  868,  869,  870,  871,  872,
    873,  874,  875,  876,  877,  878,  879,  880,  881,  882,  883,  884,
    885,  886,  887,  888,  889,  890,  891,  892,  893,  894,  895,  896,
    897,  898,  899,  900,  901,  902,  903,  904,  905,  906,  907,  908,
    909,  910,  911,  912,  913,  914,  915,  916,  917,  918,  919,  920,
    921,  922,  923,  924,  925,  926,  927,  928,  929,  930,  931,  932,
    933,  934,  935,  936,  937,  938,  939,  940,  525,  941,  942,  943,
    944,  945,  946,  947,  948,  949,  950,  951,  952,  953,  954,  955,
    956,  957,  958,  959,  960,  961,  962,  963,  964,  965,  966,  967,
    968,  969,  970,  971,  972,  973,  974,  975,  976,  977,  978,  525,
    525,  507,  979,  979,  979,  979,  979,  979,  117,  980,  981,  982,
    983,  984,  985,  986,  987,  988,  989,  990,  991,  992,  993,  994,
    995,  996,  997,  998,  999,  1000, 1001, 1002, 1003, 1004, 1005, 1006,
    1007, 1008, 1009, 1010, 1011, 1012, 1013, 1014, 1015, 1016, 1017, 221,
    117,  979,  1018, 525,  525,  75,   75,   74,   525,  511,  509,  509,
    509,  509,  511,  509,  509,  509,  1019, 511,  509,  509,  509,  509,
    509,  509,  511,  511,  511,  511,  511,  511,  509,  509,  511,  509,
    509,  1019, 1020, 509,  1021, 1022, 1023, 1024, 1025, 1026, 1027, 1028,
    1029, 1030, 1030, 1031, 1032, 1033, 1034, 1035, 1036, 1037, 1038, 1036,
    509,  511,  1036, 1029, 525,  525,  525,  525,  525,  525,  525,  525,
    1039, 1039, 1039, 1039, 1039, 1039, 1039, 1039, 1039, 1039, 1039, 1039,
    1039, 1039, 1039, 1039, 1039, 1039, 1039, 1039, 1039, 1039, 1039, 1039,
    1039, 1039, 1039, 525,  525,  525,  525,  1039, 1039, 1039, 1039, 1036,
    1036, 525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    1040, 1040, 1040, 1040, 1040, 1040, 79,   79,   1041, 1042, 1042, 1043,
    1044, 1045, 75,   75,   509,  509,  509,  509,  509,  509,  509,  509,
    1046, 1047, 1048, 1045, 1049, 1045, 1045, 1045, 1050, 1050, 1050, 1050,
    1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050,
    1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050,
    1050, 1050, 1050, 1050, 1051, 1050, 1050, 1050, 1050, 1050, 1050, 1050,
    1050, 1050, 1050, 1052, 1053, 1054, 1046, 1047, 1048, 1055, 1056, 509,
    509,  511,  511,  509,  509,  509,  509,  509,  511,  509,  509,  511,
    1057, 1057, 1057, 1057, 1057, 1057, 1057, 1057, 1057, 1057, 1042, 1058,
    1058, 1045, 1050, 1050, 1059, 1050, 1050, 1050, 1050, 1060, 1060, 1060,
    1060, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050,
    1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050,
    1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050,
    1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050,
    1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050,
    1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050,
    1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050,
    1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1045, 1050, 509,  509,
    509,  509,  509,  509,  509,  1040, 75,   509,  509,  509,  509,  511,
    509,  1051, 1051, 509,  509,  75,   511,  509,  509,  511,  1050, 1050,
    1061, 1061, 1061, 1061, 1061, 1061, 1061, 1061, 1061, 1061, 1050, 1050,
    1050, 1062, 1062, 1050, 1045, 1045, 1045, 1045, 1045, 1045, 1045, 1045,
    1045, 1045, 1045, 1045, 1045, 1045, 525,  1049, 1050, 1063, 1050, 1050,
    1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050,
    1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050,
    1050, 1050, 1050, 1050, 509,  511,  509,  509,  511,  509,  509,  511,
    511,  511,  509,  511,  511,  509,  511,  509,  509,  509,  511,  509,
    511,  509,  511,  509,  511,  509,  509,  525,  525,  1050, 1050, 1050,
    1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050,
    1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050,
    1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050,
    1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050,
    1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050,
    1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050,
    1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050, 1050,
    1050, 1050, 516,  516,  516,  516,  516,  516,  516,  516,  516,  516,
    516,  1050, 525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  1064, 1064, 1064, 1064, 1064, 1064, 1064, 1064,
    1064, 1064, 1039, 1039, 1039, 1039, 1039, 1039, 1039, 1039, 1039, 1039,
    1039, 1039, 1039, 1039, 1039, 1039, 1039, 1039, 1039, 1039, 1039, 1039,
    1039, 1039, 1039, 1039, 1039, 1039, 1039, 1039, 1039, 1039, 1039, 509,
    509,  509,  509,  509,  509,  509,  511,  509,  1065, 1065, 75,   73,
    73,   73,   1065, 525,  525,  511,  1066, 1066, 1067, 1067, 1067, 1067,
    1067, 1067, 1067, 1067, 1067, 1067, 1067, 1067, 1067, 1067, 1067, 1067,
    1067, 1067, 1067, 1067, 1067, 1067, 1068, 1068, 1068, 1068, 1069, 1068,
    1068, 1068, 1068, 1068, 1068, 1068, 1068, 1068, 1069, 1068, 1068, 1068,
    1069, 1068, 1068, 1068, 1068, 1068, 525,  525,  1070, 1070, 1070, 1070,
    1070, 1070, 1070, 1070, 1070, 1070, 1070, 1070, 1070, 1070, 1070, 525,
    1067, 1067, 1067, 1067, 1067, 1067, 1067, 1067, 1067, 1067, 1067, 1067,
    1067, 1067, 1067, 1067, 1067, 1067, 1067, 1067, 1067, 1067, 1067, 1067,
    1067, 1071, 1071, 1071, 525,  525,  1070, 525,  1072, 1072, 1072, 1072,
    1072, 1072, 1072, 1072, 1072, 1072, 1072, 525,  525,  525,  525,  525,
    1072, 1072, 1072, 1072, 1072, 1072, 1072, 1072, 1072, 1072, 1072, 1072,
    1072, 1072, 1072, 1072, 1072, 1072, 1072, 1072, 1072, 1072, 1072, 1072,
    1073, 1072, 1072, 1072, 1072, 1072, 1072, 525,  1074, 1074, 525,  525,
    525,  525,  525,  1068, 1068, 1071, 1071, 1071, 1068, 1068, 1068, 1068,
    1072, 1072, 1072, 1072, 1072, 1072, 1072, 1072, 1072, 1072, 1072, 1072,
    1072, 1072, 1072, 1072, 1072, 1072, 1072, 1072, 1072, 1072, 1072, 1072,
    1072, 1072, 1072, 1072, 1072, 1072, 1072, 1072, 1072, 1072, 1072, 1072,
    1072, 1072, 1072, 1072, 1072, 1075, 1068, 1068, 1068, 1068, 1068, 1071,
    1071, 1071, 1071, 1071, 1068, 1068, 1068, 1068, 1068, 1068, 1068, 1068,
    1068, 1068, 1068, 1068, 1068, 1068, 1074, 1071, 1068, 1068, 1071, 1068,
    1068, 1071, 1068, 1068, 1068, 1071, 1071, 1071, 1076, 1077, 1078, 1068,
    1068, 1068, 1071, 1068, 1068, 1071, 1071, 1068, 1068, 1068, 1068, 1068,
    1079, 1079, 1079, 1080, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1079, 1080,
    1082, 1081, 1080, 1080, 1080, 1079, 1079, 1079, 1079, 1079, 1079, 1079,
    1079, 1080, 1080, 1080, 1080, 1083, 1080, 1080, 1081, 1068, 1071, 1068,
    1068, 1079, 1079, 1079, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1079, 1079, 1084, 1084, 1085, 1085, 1085, 1085, 1085, 1085,
    1085, 1085, 1085, 1085, 1084, 1086, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1079, 1080, 1080,
    525,  1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 525,  525,  1081,
    1081, 525,  525,  1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 525,  1081, 1081, 1081, 1081, 1081, 1081, 1081, 525,  1081, 525,
    525,  525,  1081, 1081, 1081, 1081, 525,  525,  1082, 1081, 1080, 1080,
    1080, 1079, 1079, 1079, 1079, 525,  525,  1080, 1080, 525,  525,  1080,
    1080, 1083, 1081, 525,  525,  525,  525,  525,  525,  525,  525,  1080,
    525,  525,  525,  525,  1081, 1081, 525,  1081, 1081, 1081, 1079, 1079,
    525,  525,  1085, 1085, 1085, 1085, 1085, 1085, 1085, 1085, 1085, 1085,
    1081, 1081, 1087, 1087, 1088, 1088, 1088, 1088, 1088, 1088, 1089, 1087,
    1081, 1084, 1068, 525,  525,  1079, 1079, 1080, 525,  1081, 1081, 1081,
    1081, 1081, 1081, 525,  525,  525,  525,  1081, 1081, 525,  525,  1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 525,  1081, 1081,
    1081, 1081, 1081, 1081, 1081, 525,  1081, 1081, 525,  1081, 1081, 525,
    1081, 1081, 525,  525,  1082, 525,  1080, 1080, 1080, 1079, 1079, 525,
    525,  525,  525,  1079, 1079, 525,  525,  1079, 1079, 1083, 525,  525,
    525,  1079, 525,  525,  525,  525,  525,  525,  525,  1081, 1081, 1081,
    1081, 525,  1081, 525,  525,  525,  525,  525,  525,  525,  1085, 1085,
    1085, 1085, 1085, 1085, 1085, 1085, 1085, 1085, 1079, 1079, 1081, 1081,
    1081, 1079, 1084, 525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  1079, 1079, 1080, 525,  1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 525,  1081, 1081, 1081, 525,  1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 525,  1081, 1081, 1081, 1081, 1081, 1081,
    1081, 525,  1081, 1081, 525,  1081, 1081, 1081, 1081, 1081, 525,  525,
    1082, 1081, 1080, 1080, 1080, 1079, 1079, 1079, 1079, 1079, 525,  1079,
    1079, 1080, 525,  1080, 1080, 1083, 525,  525,  1081, 525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    1081, 1081, 1079, 1079, 525,  525,  1085, 1085, 1085, 1085, 1085, 1085,
    1085, 1085, 1085, 1085, 1084, 1087, 525,  525,  525,  525,  525,  525,
    525,  1081, 1079, 1079, 1079, 1079, 1079, 1079, 525,  1079, 1080, 1080,
    525,  1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 525,  525,  1081,
    1081, 525,  525,  1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 525,  1081, 1081, 1081, 1081, 1081, 1081, 1081, 525,  1081, 1081,
    525,  1081, 1081, 1081, 1081, 1081, 525,  525,  1082, 1081, 1080, 1079,
    1080, 1079, 1079, 1079, 1079, 525,  525,  1080, 1080, 525,  525,  1080,
    1080, 1083, 525,  525,  525,  525,  525,  525,  525,  1079, 1079, 1080,
    525,  525,  525,  525,  1081, 1081, 525,  1081, 1081, 1081, 1079, 1079,
    525,  525,  1085, 1085, 1085, 1085, 1085, 1085, 1085, 1085, 1085, 1085,
    1089, 1081, 1088, 1088, 1088, 1088, 1088, 1088, 525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  1079, 1081, 525,  1081, 1081, 1081,
    1081, 1081, 1081, 525,  525,  525,  1081, 1081, 1081, 525,  1081, 1081,
    1081, 1081, 525,  525,  525,  1081, 1081, 525,  1081, 525,  1081, 1081,
    525,  525,  525,  1081, 1081, 525,  525,  525,  1081, 1081, 1081, 525,
    525,  525,  1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 525,  525,  525,  525,  1080, 1080, 1079, 1080, 1080, 525,
    525,  525,  1080, 1080, 1080, 525,  1080, 1080, 1080, 1083, 525,  525,
    1081, 525,  525,  525,  525,  525,  525,  1080, 525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  1085, 1085,
    1085, 1085, 1085, 1085, 1085, 1085, 1085, 1085, 1088, 1088, 1088, 1090,
    1090, 1090, 1090, 1090, 1090, 1087, 1090, 525,  525,  525,  525,  525,
    1079, 1080, 1080, 1080, 1079, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 525,  1081, 1081, 1081, 525,  1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 525,  1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 525,  525,
    1082, 1081, 1079, 1079, 1079, 1080, 1080, 1080, 1080, 525,  1079, 1079,
    1079, 525,  1079, 1079, 1079, 1083, 525,  525,  525,  525,  525,  525,
    525,  1091, 1092, 525,  1081, 1081, 1081, 525,  525,  1081, 525,  525,
    1081, 1081, 1079, 1079, 525,  525,  1085, 1085, 1085, 1085, 1085, 1085,
    1085, 1085, 1085, 1085, 525,  525,  525,  525,  525,  525,  525,  1084,
    1093, 1093, 1093, 1093, 1093, 1093, 1093, 1089, 1081, 1079, 1080, 1080,
    1084, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 525,  1081, 1081,
    1081, 525,  1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 525,  1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    525,  1081, 1081, 1081, 1081, 1081, 525,  525,  1082, 1081, 1080, 1094,
    1080, 1080, 1080, 1080, 1080, 525,  1094, 1080, 1080, 525,  1080, 1080,
    1079, 1083, 525,  525,  525,  525,  525,  525,  525,  1080, 1080, 525,
    525,  525,  525,  525,  525,  1081, 1081, 525,  1081, 1081, 1079, 1079,
    525,  525,  1085, 1085, 1085, 1085, 1085, 1085, 1085, 1085, 1085, 1085,
    525,  1081, 1081, 1080, 525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  1079, 1079, 1080, 1080, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 525,  1081, 1081, 1081, 525,  1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1083, 1083, 1081, 1080, 1080, 1080, 1079, 1079, 1079,
    1079, 525,  1080, 1080, 1080, 525,  1080, 1080, 1080, 1083, 1081, 1089,
    525,  525,  525,  525,  1081, 1081, 1081, 1080, 1088, 1088, 1088, 1088,
    1088, 1088, 1088, 1081, 1081, 1081, 1079, 1079, 525,  525,  1085, 1085,
    1085, 1085, 1085, 1085, 1085, 1085, 1085, 1085, 1088, 1088, 1088, 1088,
    1088, 1088, 1088, 1088, 1088, 1089, 1081, 1081, 1081, 1081, 1081, 1081,
    525,  1079, 1080, 1080, 525,  1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 525,
    525,  525,  1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 525,  1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    525,  1081, 525,  525,  1081, 1081, 1081, 1081, 1081, 1081, 1081, 525,
    525,  525,  1083, 525,  525,  525,  525,  1080, 1080, 1080, 1079, 1079,
    1079, 525,  1079, 525,  1080, 1080, 1080, 1080, 1080, 1080, 1080, 1080,
    525,  525,  525,  525,  525,  525,  1085, 1085, 1085, 1085, 1085, 1085,
    1085, 1085, 1085, 1085, 525,  525,  1080, 1080, 1084, 525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1079, 1081, 1095,
    1079, 1079, 1079, 1079, 1096, 1096, 1083, 525,  525,  525,  525,  1087,
    1081, 1081, 1081, 1081, 1081, 1081, 1086, 1079, 1097, 1097, 1097, 1097,
    1079, 1079, 1079, 1084, 1085, 1085, 1085, 1085, 1085, 1085, 1085, 1085,
    1085, 1085, 1084, 1084, 525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  1081, 1081, 525,  1081, 525,  1081, 1081,
    1081, 1081, 1081, 525,  1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 525,  1081, 525,  1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1079, 1081, 1095, 1079, 1079, 1079, 1079,
    1098, 1098, 1083, 1079, 1079, 1081, 525,  525,  1081, 1081, 1081, 1081,
    1081, 525,  1086, 525,  1099, 1099, 1099, 1099, 1079, 1079, 1079, 525,
    1085, 1085, 1085, 1085, 1085, 1085, 1085, 1085, 1085, 1085, 525,  525,
    1095, 1095, 1081, 1081, 525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    1081, 1089, 1089, 1089, 1084, 1084, 1084, 1084, 1084, 1084, 1084, 1084,
    1100, 1084, 1084, 1084, 1084, 1084, 1084, 1089, 1084, 1089, 1089, 1089,
    1071, 1071, 1089, 1089, 1089, 1089, 1089, 1089, 1085, 1085, 1085, 1085,
    1085, 1085, 1085, 1085, 1085, 1085, 1088, 1088, 1088, 1088, 1088, 1088,
    1088, 1088, 1088, 1088, 1089, 1071, 1089, 1071, 1089, 1101, 1102, 1103,
    1102, 1103, 1080, 1080, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    525,  1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 525,  525,  525,  525,  1104, 1105, 1079, 1106, 1079, 1079, 1107,
    1079, 1107, 1105, 1105, 1105, 1105, 1079, 1080, 1105, 1079, 1068, 1068,
    1083, 1084, 1068, 1068, 1081, 1081, 1081, 1081, 1081, 1079, 1079, 1079,
    1079, 1079, 1079, 1079, 1079, 1079, 1079, 1079, 525,  1079, 1079, 1079,
    1079, 1079, 1079, 1079, 1079, 1079, 1079, 1079, 1079, 1079, 1079, 1079,
    1079, 1079, 1079, 1079, 1079, 1079, 1079, 1079, 1079, 1079, 1079, 1079,
    1079, 1079, 1079, 1079, 1079, 1079, 1079, 1079, 1079, 525,  1089, 1089,
    1089, 1089, 1089, 1089, 1089, 1089, 1071, 1089, 1089, 1089, 1089, 1089,
    1089, 525,  1089, 1089, 1084, 1084, 1084, 1084, 1084, 1089, 1089, 1089,
    1089, 1084, 1084, 525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1080,
    1080, 1079, 1079, 1079, 1079, 1080, 1079, 1079, 1079, 1079, 1079, 1082,
    1080, 1083, 1083, 1080, 1080, 1079, 1079, 1081, 1085, 1085, 1085, 1085,
    1085, 1085, 1085, 1085, 1085, 1085, 1084, 1084, 1084, 1084, 1084, 1084,
    1081, 1081, 1081, 1081, 1081, 1081, 1080, 1080, 1079, 1079, 1081, 1081,
    1081, 1081, 1079, 1079, 1079, 1081, 1080, 1080, 1080, 1081, 1081, 1080,
    1080, 1080, 1080, 1080, 1080, 1080, 1081, 1081, 1081, 1079, 1079, 1079,
    1079, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1079, 1080, 1080, 1079, 1079, 1080, 1080, 1080, 1080, 1080,
    1080, 1071, 1081, 1080, 1085, 1085, 1085, 1085, 1085, 1085, 1085, 1085,
    1085, 1085, 1080, 1080, 1080, 1079, 1089, 1089, 1108, 1109, 1110, 1111,
    1112, 1113, 1114, 1115, 1116, 1117, 1118, 1119, 1120, 1121, 1122, 1123,
    1124, 1125, 1126, 1127, 1128, 1129, 1130, 1131, 1132, 1133, 1134, 1135,
    1136, 1137, 1138, 1139, 1140, 1141, 1142, 1143, 1144, 1145, 525,  1146,
    525,  525,  525,  525,  525,  1147, 525,  525,  1148, 1149, 1150, 1151,
    1152, 1153, 1154, 1155, 1156, 1157, 1158, 1159, 1160, 1161, 1162, 1163,
    1164, 1165, 1166, 1167, 1168, 1169, 1170, 1171, 1172, 1173, 1174, 1175,
    1176, 1177, 1178, 1179, 1180, 1181, 1182, 1183, 1184, 1185, 1186, 1187,
    1188, 1189, 1190, 1084, 1191, 1192, 1193, 1194, 1195, 1195, 1195, 1195,
    1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195,
    1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195,
    1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195,
    1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195,
    1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195,
    1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195,
    1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195,
    1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 525,  1081, 1081, 1081, 1081, 525,  525,  1081, 1081, 1081, 1081,
    1081, 1081, 1081, 525,  1081, 525,  1081, 1081, 1081, 1081, 525,  525,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 525,  1081, 1081, 1081, 1081, 525,  525,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 525,  1081, 1081,
    1081, 1081, 525,  525,  1081, 1081, 1081, 1081, 1081, 1081, 1081, 525,
    1081, 525,  1081, 1081, 1081, 1081, 525,  525,  1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 525,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 525,  1081, 1081,
    1081, 1081, 525,  525,  1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 525,
    525,  1068, 1068, 1068, 1084, 1084, 1084, 1084, 1084, 1084, 1084, 1084,
    1084, 1088, 1088, 1088, 1088, 1088, 1088, 1088, 1088, 1088, 1088, 1088,
    1088, 1088, 1088, 1088, 1088, 1088, 1088, 1088, 1088, 525,  525,  525,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090,
    1090, 1090, 525,  525,  525,  525,  525,  525,  1196, 1197, 1198, 1199,
    1200, 1201, 1202, 1203, 1204, 1205, 1206, 1207, 1208, 1209, 1210, 1211,
    1212, 1213, 1214, 1215, 1216, 1217, 1218, 1219, 1220, 1221, 1222, 1223,
    1224, 1225, 1226, 1227, 1228, 1229, 1230, 1231, 1232, 1233, 1234, 1235,
    1236, 1237, 1238, 1239, 1240, 1241, 1242, 1243, 1244, 1245, 1246, 1247,
    1248, 1249, 1250, 1251, 1252, 1253, 1254, 1255, 1256, 1257, 1258, 1259,
    1260, 1261, 1262, 1263, 1264, 1265, 1266, 1267, 1268, 1269, 1270, 1271,
    1272, 1273, 1274, 1275, 1276, 1277, 1278, 1279, 1280, 1281, 525,  525,
    1282, 1283, 1284, 1285, 1286, 1287, 525,  525,  1288, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1089, 1084, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1289, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1102, 1103, 525,  525,  525,  1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1084,
    1084, 1084, 1290, 1290, 1290, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 525,  525,  525,  525,  525,  525,  525,  1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1079, 1079, 1083, 1291, 525,  525,  525,  525,  525,  525,
    525,  525,  525,  1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1079, 1079,
    1291, 1084, 1084, 525,  525,  525,  525,  525,  525,  525,  525,  525,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1079, 1079, 525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 525,  1081, 1081,
    1081, 525,  1079, 1079, 525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1079, 1079, 1080, 1079,
    1079, 1079, 1079, 1079, 1079, 1079, 1080, 1080, 1080, 1080, 1080, 1080,
    1080, 1080, 1079, 1080, 1080, 1079, 1079, 1079, 1079, 1079, 1079, 1079,
    1079, 1079, 1083, 1079, 1084, 1084, 1084, 1086, 1084, 1084, 1084, 1087,
    1081, 1068, 525,  525,  1085, 1085, 1085, 1085, 1085, 1085, 1085, 1085,
    1085, 1085, 525,  525,  525,  525,  525,  525,  1093, 1093, 1093, 1093,
    1093, 1093, 1093, 1093, 1093, 1093, 525,  525,  525,  525,  525,  525,
    1292, 1292, 1292, 1292, 1292, 1292, 1288, 1292, 1292, 1292, 1292, 1079,
    1079, 1079, 1293, 1079, 1085, 1085, 1085, 1085, 1085, 1085, 1085, 1085,
    1085, 1085, 525,  525,  525,  525,  525,  525,  1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1086, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 525,  525,  525,  525,  525,  525,  525,  1081, 1081, 1081, 1081,
    1081, 1079, 1079, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1294, 1081, 525,  525,  525,  525,  525,  1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 525,
    1079, 1079, 1079, 1080, 1080, 1080, 1080, 1079, 1079, 1080, 1080, 1080,
    525,  525,  525,  525,  1080, 1080, 1079, 1080, 1080, 1080, 1080, 1080,
    1080, 1295, 1068, 1071, 525,  525,  525,  525,  1090, 525,  525,  525,
    1292, 1292, 1085, 1085, 1085, 1085, 1085, 1085, 1085, 1085, 1085, 1085,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 525,  525,  1081, 1081, 1081, 1081,
    1081, 525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 525,  525,  525,  525,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 525,  525,  525,  525,  525,  525,  1085, 1085, 1085, 1085,
    1085, 1085, 1085, 1085, 1085, 1085, 1088, 525,  525,  525,  1090, 1090,
    1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090,
    1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090,
    1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1068, 1071, 1080, 1080, 1079,
    525,  525,  1084, 1084, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1080, 1079, 1080,
    1079, 1079, 1079, 1079, 1079, 1079, 1079, 525,  1083, 1080, 1079, 1080,
    1080, 1079, 1079, 1079, 1079, 1079, 1079, 1079, 1079, 1080, 1080, 1080,
    1080, 1080, 1080, 1079, 1079, 1068, 1068, 1068, 1068, 1068, 1068, 1068,
    1068, 525,  525,  1071, 1085, 1085, 1085, 1085, 1085, 1085, 1085, 1085,
    1085, 1085, 525,  525,  525,  525,  525,  525,  1085, 1085, 1085, 1085,
    1085, 1085, 1085, 1085, 1085, 1085, 525,  525,  525,  525,  525,  525,
    1084, 1084, 1084, 1084, 1084, 1084, 1084, 1086, 1084, 1084, 1084, 1084,
    1084, 1084, 525,  525,  1068, 1068, 1068, 1068, 1068, 1071, 1071, 1071,
    1071, 1071, 1071, 1068, 1068, 1071, 1296, 1071, 1071, 1068, 1068, 1071,
    1071, 1068, 1068, 1068, 1068, 1068, 1071, 1068, 1068, 1068, 1068, 525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    1079, 1079, 1079, 1079, 1080, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1082, 1080, 1079, 1079, 1079, 1079, 1079, 1080,
    1079, 1080, 1080, 1080, 1080, 1080, 1079, 1080, 1291, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 525,  1084, 1084, 1085, 1085, 1085, 1085,
    1085, 1085, 1085, 1085, 1085, 1085, 1084, 1084, 1084, 1084, 1084, 1084,
    1084, 1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089, 1068,
    1071, 1068, 1068, 1068, 1068, 1068, 1068, 1068, 1089, 1089, 1089, 1089,
    1089, 1089, 1089, 1089, 1089, 1084, 1084, 1084, 1079, 1079, 1080, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1080, 1079, 1079, 1079, 1079, 1080, 1080,
    1079, 1079, 1291, 1083, 1079, 1079, 1081, 1081, 1085, 1085, 1085, 1085,
    1085, 1085, 1085, 1085, 1085, 1085, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1082, 1080, 1079, 1079, 1080, 1080, 1080, 1079, 1080, 1079,
    1079, 1079, 1291, 1291, 525,  525,  525,  525,  525,  525,  525,  525,
    1084, 1084, 1084, 1084, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1080, 1080, 1080, 1080, 1080, 1080, 1080, 1080,
    1079, 1079, 1079, 1079, 1079, 1079, 1079, 1079, 1080, 1080, 1079, 1082,
    525,  525,  525,  1084, 1084, 1084, 1084, 1084, 1085, 1085, 1085, 1085,
    1085, 1085, 1085, 1085, 1085, 1085, 525,  525,  525,  1081, 1081, 1081,
    1085, 1085, 1085, 1085, 1085, 1085, 1085, 1085, 1085, 1085, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1086, 1086, 1086, 1086, 1086, 1086, 1084, 1084,
    1297, 1298, 1299, 1300, 1301, 1301, 1302, 1303, 1304, 1305, 1306, 525,
    525,  525,  525,  525,  1307, 1308, 1309, 1310, 1311, 1312, 1313, 1314,
    1315, 1316, 1317, 1318, 1319, 1320, 1321, 1322, 1323, 1324, 1325, 1326,
    1327, 1328, 1329, 1330, 1331, 1332, 1333, 1334, 1335, 1336, 1337, 1338,
    1339, 1340, 1341, 1342, 1343, 1344, 1345, 1346, 1347, 1348, 1349, 525,
    525,  1350, 1351, 1352, 1084, 1084, 1084, 1084, 1084, 1084, 1084, 1084,
    525,  525,  525,  525,  525,  525,  525,  525,  1068, 1068, 1068, 1084,
    1353, 1071, 1071, 1071, 1071, 1071, 1068, 1068, 1071, 1071, 1071, 1071,
    1068, 1080, 1353, 1353, 1353, 1353, 1353, 1353, 1353, 1081, 1081, 1081,
    1081, 1071, 1081, 1081, 1081, 1081, 1081, 1081, 1068, 1081, 1081, 1080,
    1068, 1068, 1081, 525,  525,  525,  525,  525,  1354, 1354, 1354, 1354,
    1354, 1354, 1354, 1354, 1354, 1354, 1354, 1354, 1354, 1354, 1354, 1354,
    1354, 1354, 1354, 1354, 1354, 1354, 1354, 1354, 1354, 1354, 1354, 1354,
    1354, 1354, 1354, 1354, 1354, 1354, 1354, 1354, 1354, 1354, 1354, 1354,
    1354, 1354, 1354, 1354, 1191, 1191, 1191, 1086, 1191, 1191, 1191, 1191,
    1191, 1191, 1191, 1191, 1191, 1191, 1191, 1086, 1191, 1191, 1191, 1191,
    1191, 1191, 1191, 1191, 1191, 1191, 1191, 1191, 1191, 1191, 1191, 1191,
    1191, 1191, 1086, 1191, 1191, 1191, 1191, 1191, 1191, 1191, 1191, 1191,
    1191, 1191, 1191, 1191, 1191, 1191, 1191, 1191, 1191, 1191, 1355, 1355,
    1355, 1355, 1355, 1355, 1355, 1355, 1355, 1354, 1354, 1354, 1354, 1354,
    1354, 1354, 1354, 1354, 1354, 1354, 1354, 1354, 1191, 1356, 1354, 1354,
    1354, 1357, 1354, 1354, 1354, 1354, 1354, 1354, 1354, 1354, 1354, 1354,
    1354, 1354, 1354, 1354, 1354, 1354, 1358, 1354, 1354, 1354, 1354, 1354,
    1354, 1354, 1354, 1354, 1354, 1354, 1354, 1191, 1191, 1191, 1191, 1191,
    1191, 1191, 1191, 1191, 1191, 1191, 1191, 1191, 1191, 1191, 1191, 1191,
    1191, 1191, 1191, 1191, 1191, 1191, 1191, 1191, 1191, 1191, 1191, 1191,
    1191, 1191, 1191, 1191, 1191, 1191, 1191, 1191, 1068, 1068, 1071, 1068,
    1068, 1068, 1068, 1068, 1068, 1068, 1071, 1068, 1068, 1359, 1360, 1071,
    1361, 1068, 1068, 1068, 1068, 1068, 1068, 1068, 1068, 1068, 1068, 1068,
    1068, 1068, 1068, 1068, 1068, 1068, 1068, 1068, 1068, 1068, 1068, 1068,
    1068, 1068, 1068, 1068, 1068, 1068, 1068, 1068, 1068, 1068, 1068, 1068,
    1068, 1068, 1362, 1294, 1294, 1071, 1363, 1068, 1364, 1071, 1068, 1071,
    1365, 1366, 1367, 1368, 1369, 1370, 1371, 1372, 1373, 1374, 1375, 1376,
    1377, 1378, 1379, 1380, 1381, 1382, 1383, 1384, 1385, 1386, 1387, 1388,
    1389, 1390, 1391, 1392, 1393, 1394, 1395, 1396, 1397, 1398, 1399, 1400,
    1401, 1402, 1403, 1404, 1405, 1406, 1407, 1408, 1409, 1410, 1411, 1412,
    1413, 1414, 1415, 1416, 1417, 1418, 1419, 1420, 1421, 1422, 1423, 1424,
    1425, 1426, 1427, 1428, 1429, 1430, 1431, 1432, 1433, 1434, 1435, 1436,
    1437, 1438, 1439, 1440, 1441, 1442, 1443, 1444, 1445, 1446, 1447, 1448,
    1449, 1450, 1451, 1452, 1453, 1454, 1455, 1456, 1457, 1458, 1459, 1460,
    1461, 1462, 1463, 1464, 1465, 1466, 1467, 1468, 1469, 1470, 1471, 1472,
    1473, 1474, 1475, 1476, 1477, 1478, 1479, 1480, 1481, 1482, 1483, 1484,
    1485, 1486, 1487, 1488, 1489, 1490, 1491, 1492, 1493, 1494, 1495, 1496,
    1497, 1498, 1499, 1500, 1501, 1502, 1503, 1504, 1505, 1506, 1507, 1508,
    1509, 1510, 1511, 1512, 1513, 1514, 1354, 1354, 1354, 1354, 1515, 1462,
    1354, 1354, 1516, 1354, 1517, 1518, 1519, 1520, 1521, 1522, 1523, 1524,
    1525, 1526, 1527, 1528, 1529, 1530, 1531, 1532, 1533, 1534, 1535, 1536,
    1537, 1538, 1539, 1540, 1541, 1542, 1543, 1544, 1545, 1546, 1547, 1548,
    1549, 1550, 1551, 1552, 1553, 1554, 1555, 1556, 1557, 1558, 1559, 1560,
    1561, 1562, 1563, 1564, 1565, 1566, 1567, 1568, 1569, 1570, 1571, 1572,
    1573, 1574, 1575, 1576, 1577, 1578, 1579, 1580, 1581, 1582, 1583, 1584,
    1585, 1586, 1587, 1588, 1589, 1590, 1591, 1592, 1593, 1594, 1595, 1596,
    1597, 1598, 1599, 1600, 1601, 1602, 1603, 1604, 1605, 1606, 1607, 1608,
    1609, 1610, 1611, 1612, 1613, 1614, 1615, 1616, 1617, 1618, 1619, 1620,
    1621, 1622, 1623, 1624, 1625, 1626, 1627, 1628, 1629, 1630, 1631, 1632,
    1633, 1634, 525,  525,  1635, 1636, 1637, 1638, 1639, 1640, 525,  525,
    1641, 1642, 1643, 1644, 1645, 1646, 1647, 1648, 1649, 1650, 1651, 1652,
    1653, 1654, 1655, 1656, 1657, 1658, 1659, 1660, 1661, 1662, 1663, 1664,
    1665, 1666, 1667, 1668, 1669, 1670, 1671, 1672, 1673, 1674, 1675, 1676,
    1677, 1678, 525,  525,  1679, 1680, 1681, 1682, 1683, 1684, 525,  525,
    1354, 1685, 1354, 1686, 1354, 1687, 1354, 1688, 525,  1689, 525,  1690,
    525,  1691, 525,  1692, 1693, 1694, 1695, 1696, 1697, 1698, 1699, 1700,
    1701, 1702, 1703, 1704, 1705, 1706, 1707, 1708, 1709, 1710, 1711, 1712,
    1713, 1714, 1715, 1716, 1717, 1718, 1719, 1720, 1721, 1722, 525,  525,
    1723, 1724, 1725, 1726, 1727, 1728, 1729, 1730, 1731, 1732, 1733, 1734,
    1735, 1736, 1737, 1738, 1739, 1740, 1741, 1742, 1743, 1744, 1745, 1746,
    1747, 1748, 1749, 1750, 1751, 1752, 1753, 1754, 1755, 1756, 1757, 1758,
    1759, 1760, 1761, 1762, 1763, 1764, 1765, 1766, 1767, 1768, 1769, 1770,
    1771, 1772, 1354, 1773, 1354, 525,  1354, 1354, 1774, 1775, 1776, 1777,
    1778, 1779, 1780, 1779, 1779, 1781, 1354, 1782, 1354, 525,  1354, 1354,
    1783, 1784, 1785, 1786, 1787, 1781, 1781, 1781, 1788, 1789, 1354, 1354,
    525,  525,  1354, 1354, 1790, 1791, 1792, 1793, 525,  1781, 1781, 1781,
    1794, 1795, 1354, 1354, 1354, 1796, 1354, 1354, 1797, 1798, 1799, 1800,
    1801, 1781, 1781, 1781, 525,  525,  1354, 1802, 1354, 525,  1354, 1354,
    1803, 1804, 1805, 1806, 1807, 1781, 1779, 525,  1289, 1289, 1808, 1808,
    1808, 1808, 1808, 1809, 1808, 1808, 1808, 1293, 1293, 1293, 1810, 1811,
    1288, 1812, 1288, 1288, 1288, 1288, 1292, 1813, 1814, 1815, 1816, 1814,
    1814, 1815, 1816, 1814, 1292, 1292, 1292, 1292, 1813, 1813, 1813, 1292,
    1817, 1818, 1819, 1820, 1821, 1822, 1823, 1824, 1825, 1825, 1825, 1826,
    1826, 1292, 1813, 1813, 1292, 1827, 1828, 1292, 1813, 1292, 1813, 1829,
    1829, 1292, 1292, 1292, 1830, 1102, 1103, 1813, 1813, 1813, 1292, 1292,
    1292, 1292, 1292, 1292, 1292, 1292, 1831, 1292, 1829, 1292, 1292, 1813,
    1292, 1292, 1292, 1292, 1292, 1292, 1292, 1808, 1293, 1293, 1293, 1293,
    1293, 525,  1832, 1833, 1834, 1835, 1293, 1293, 1293, 1293, 1293, 1293,
    1836, 1191, 525,  525,  1836, 1836, 1836, 1836, 1836, 1836, 1837, 1837,
    1838, 1839, 1840, 1191, 1841, 1841, 1841, 1841, 1841, 1841, 1841, 1841,
    1841, 1841, 1842, 1842, 1843, 1844, 1845, 525,  1355, 1355, 1355, 1355,
    1355, 1355, 1355, 1355, 1355, 1355, 1355, 1355, 1355, 525,  525,  525,
    1087, 1087, 1087, 1087, 1087, 1087, 1087, 1087, 1846, 1087, 1087, 1087,
    1087, 1087, 1087, 1087, 1087, 1087, 1087, 1087, 1087, 1087, 1087, 1087,
    1087, 1087, 1087, 1087, 1087, 1087, 1087, 1087, 1087, 525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    1068, 1068, 1353, 1353, 1068, 1068, 1068, 1068, 1353, 1353, 1353, 1068,
    1068, 1296, 1296, 1296, 1296, 1068, 1296, 1296, 1296, 1353, 1353, 1068,
    1071, 1068, 1353, 1353, 1071, 1071, 1071, 1071, 1068, 525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    1847, 1847, 1848, 1847, 1090, 1847, 1847, 1849, 1090, 1847, 1850, 1848,
    1848, 1848, 1850, 1850, 1848, 1848, 1848, 1850, 1090, 1848, 1847, 1090,
    1831, 1848, 1848, 1848, 1848, 1848, 1090, 1090, 1851, 1847, 1851, 1090,
    1848, 1090, 1852, 1090, 1848, 1090, 1853, 1854, 1848, 1848, 1855, 1850,
    1848, 1848, 1856, 1848, 1850, 1095, 1095, 1095, 1095, 1850, 1090, 1847,
    1850, 1850, 1848, 1848, 1857, 1831, 1831, 1831, 1831, 1848, 1850, 1850,
    1850, 1850, 1090, 1831, 1090, 1090, 1858, 1089, 1859, 1859, 1859, 1859,
    1859, 1859, 1859, 1859, 1859, 1859, 1859, 1859, 1859, 1859, 1859, 1859,
    1860, 1861, 1862, 1863, 1864, 1865, 1866, 1867, 1868, 1869, 1870, 1871,
    1872, 1873, 1874, 1875, 1876, 1877, 1878, 1879, 1880, 1881, 1882, 1883,
    1884, 1885, 1886, 1887, 1888, 1889, 1890, 1891, 1290, 1290, 1290, 1892,
    1893, 1290, 1290, 1290, 1290, 1859, 1090, 1090, 525,  525,  525,  525,
    1831, 1831, 1831, 1831, 1831, 1090, 1090, 1090, 1090, 1090, 1831, 1831,
    1090, 1090, 1090, 1090, 1831, 1090, 1090, 1831, 1090, 1090, 1831, 1090,
    1090, 1090, 1090, 1090, 1090, 1090, 1831, 1090, 1090, 1090, 1090, 1090,
    1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090,
    1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090,
    1090, 1090, 1831, 1831, 1090, 1090, 1831, 1090, 1831, 1090, 1090, 1090,
    1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090,
    1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090,
    1090, 1090, 1090, 1090, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831,
    1831, 1831, 1831, 1831, 1831, 1894, 1894, 1894, 1894, 1831, 1831, 1831,
    1894, 1894, 1894, 1894, 1894, 1894, 1831, 1831, 1831, 1894, 1895, 1896,
    1831, 1894, 1894, 1831, 1831, 1831, 1894, 1894, 1894, 1894, 1831, 1894,
    1894, 1894, 1894, 1831, 1894, 1831, 1894, 1831, 1831, 1831, 1831, 1894,
    1897, 1897, 1894, 1897, 1897, 1894, 1894, 1894, 1831, 1831, 1831, 1831,
    1831, 1894, 1831, 1894, 1894, 1894, 1894, 1894, 1894, 1894, 1894, 1894,
    1894, 1894, 1894, 1894, 1894, 1894, 1894, 1894, 1894, 1831, 1831, 1831,
    1831, 1831, 1894, 1894, 1894, 1894, 1831, 1831, 1831, 1831, 1831, 1831,
    1831, 1831, 1831, 1894, 1894, 1831, 1894, 1831, 1894, 1894, 1894, 1894,
    1894, 1894, 1894, 1894, 1831, 1894, 1894, 1894, 1894, 1894, 1894, 1894,
    1894, 1894, 1894, 1894, 1894, 1894, 1894, 1894, 1894, 1894, 1894, 1894,
    1894, 1894, 1894, 1894, 1894, 1894, 1894, 1894, 1894, 1894, 1894, 1894,
    1894, 1831, 1831, 1894, 1894, 1894, 1894, 1831, 1831, 1831, 1831, 1831,
    1894, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1894, 1894,
    1831, 1831, 1894, 1894, 1894, 1894, 1894, 1894, 1894, 1894, 1894, 1894,
    1894, 1894, 1894, 1894, 1894, 1894, 1894, 1894, 1894, 1831, 1831, 1831,
    1831, 1831, 1894, 1894, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831,
    1831, 1894, 1894, 1894, 1894, 1894, 1831, 1831, 1894, 1894, 1831, 1831,
    1831, 1831, 1894, 1894, 1894, 1894, 1894, 1894, 1894, 1894, 1894, 1894,
    1894, 1894, 1894, 1894, 1894, 1894, 1894, 1894, 1894, 1894, 1894, 1894,
    1894, 1894, 1831, 1831, 1894, 1894, 1894, 1894, 1894, 1894, 1894, 1894,
    1894, 1894, 1894, 1894, 1894, 1894, 1894, 1894, 1090, 1090, 1090, 1090,
    1090, 1090, 1090, 1090, 1102, 1103, 1102, 1103, 1090, 1090, 1090, 1090,
    1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1898, 1898,
    1090, 1090, 1090, 1090, 1894, 1894, 1090, 1090, 1090, 1090, 1090, 1090,
    1090, 1899, 1900, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090,
    1090, 1090, 1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089,
    1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089,
    1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089,
    1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089,
    1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089,
    1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089, 1090,
    1831, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090,
    1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090,
    1090, 1089, 1090, 1090, 1090, 1090, 1090, 1831, 1831, 1831, 1831, 1831,
    1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831,
    1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1090, 1090, 1090, 1090,
    1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090,
    1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090,
    1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090,
    1831, 1831, 1831, 1831, 1831, 1831, 1090, 1090, 1090, 1090, 1090, 1090,
    1090, 1898, 1898, 1898, 1898, 1090, 1090, 1090, 1898, 1090, 1090, 1898,
    1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090,
    1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090,
    1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090,
    1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090,
    1090, 1090, 1090, 1090, 1090, 1090, 525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090,
    1090, 1090, 1090, 525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    1901, 1901, 1901, 1901, 1901, 1901, 1901, 1901, 1901, 1901, 1901, 1901,
    1901, 1901, 1901, 1901, 1901, 1901, 1901, 1901, 1902, 1902, 1902, 1902,
    1902, 1902, 1902, 1902, 1902, 1902, 1902, 1902, 1902, 1902, 1902, 1902,
    1902, 1902, 1902, 1902, 1903, 1903, 1903, 1903, 1903, 1903, 1903, 1903,
    1903, 1903, 1903, 1903, 1903, 1903, 1903, 1903, 1903, 1903, 1903, 1903,
    1904, 1904, 1904, 1904, 1904, 1904, 1904, 1904, 1904, 1904, 1904, 1904,
    1904, 1904, 1904, 1904, 1904, 1904, 1904, 1904, 1904, 1904, 1904, 1904,
    1904, 1904, 1905, 1906, 1907, 1908, 1909, 1910, 1911, 1912, 1913, 1914,
    1915, 1916, 1917, 1918, 1919, 1920, 1921, 1922, 1923, 1924, 1925, 1926,
    1927, 1928, 1929, 1930, 1931, 1932, 1933, 1934, 1935, 1936, 1937, 1938,
    1939, 1940, 1941, 1942, 1943, 1944, 1945, 1946, 1947, 1948, 1949, 1950,
    1951, 1952, 1953, 1954, 1955, 1956, 1901, 1093, 1093, 1093, 1093, 1093,
    1093, 1093, 1093, 1093, 1093, 1093, 1093, 1093, 1093, 1093, 1093, 1093,
    1093, 1093, 1093, 1093, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090,
    1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090,
    1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090,
    1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090,
    1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090,
    1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090,
    1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090,
    1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090,
    1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090,
    1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090,
    1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090,
    1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090,
    1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090,
    1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090,
    1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090,
    1090, 1090, 1090, 1090, 1090, 1090, 1090, 1831, 1090, 1090, 1090, 1090,
    1090, 1090, 1090, 1090, 1090, 1831, 1090, 1090, 1090, 1090, 1090, 1090,
    1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090,
    1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090,
    1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090,
    1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090,
    1831, 1831, 1831, 1831, 1831, 1957, 1957, 1831, 1090, 1090, 1090, 1090,
    1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090,
    1090, 1090, 1090, 1090, 1898, 1898, 1090, 1090, 1090, 1090, 1090, 1090,
    1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090,
    1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1898, 1898, 1898, 1898,
    1898, 1898, 1898, 1898, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090,
    1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1898, 1898, 1898, 1898,
    1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1090, 1090, 1090, 1090,
    1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090,
    1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1831,
    1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090,
    1090, 1090, 1090, 1898, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090,
    1090, 1090, 1898, 1898, 1898, 1898, 1898, 1898, 1090, 1090, 1090, 1898,
    1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090,
    1090, 1898, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1898, 1898,
    1089, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090,
    1090, 1090, 1090, 1090, 1090, 1898, 1898, 1090, 1090, 1090, 1090, 1090,
    1898, 1898, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1898, 1090,
    1090, 1090, 1090, 1090, 1898, 1090, 1090, 1090, 1090, 1090, 1090, 1090,
    1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090,
    1090, 1090, 1898, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1898, 1898,
    1090, 1898, 1090, 1090, 1090, 1090, 1898, 1090, 1090, 1898, 1090, 1090,
    1090, 1090, 1090, 1090, 1090, 1898, 1090, 1090, 1090, 1090, 1898, 1898,
    1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090,
    1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090,
    1090, 1090, 1090, 1090, 1898, 1090, 1090, 1090, 1090, 1090, 1090, 1090,
    1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090,
    1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090,
    1090, 1090, 1090, 1090, 1898, 1090, 1898, 1090, 1090, 1090, 1090, 1898,
    1898, 1898, 1090, 1898, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090,
    1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1102, 1103, 1102, 1103,
    1102, 1103, 1102, 1103, 1102, 1103, 1102, 1103, 1102, 1103, 1093, 1093,
    1093, 1093, 1093, 1093, 1093, 1093, 1093, 1093, 1093, 1093, 1093, 1093,
    1093, 1093, 1093, 1093, 1093, 1093, 1093, 1093, 1093, 1093, 1093, 1093,
    1093, 1093, 1093, 1093, 1090, 1898, 1898, 1898, 1090, 1090, 1090, 1090,
    1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090,
    1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1898, 1090, 1090, 1090,
    1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1898,
    1894, 1831, 1831, 1894, 1894, 1102, 1103, 1831, 1894, 1894, 1831, 1894,
    1894, 1894, 1831, 1831, 1831, 1831, 1831, 1894, 1894, 1894, 1894, 1831,
    1831, 1831, 1831, 1831, 1894, 1894, 1894, 1831, 1831, 1831, 1894, 1894,
    1894, 1894, 1102, 1103, 1102, 1103, 1102, 1103, 1102, 1103, 1102, 1103,
    1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831,
    1831, 1831, 1831, 1831, 1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089,
    1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089,
    1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089,
    1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089,
    1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089,
    1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089,
    1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089,
    1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089,
    1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089,
    1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089,
    1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089, 1089,
    1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831,
    1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831,
    1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831,
    1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831,
    1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831,
    1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831,
    1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831,
    1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831,
    1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831,
    1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831,
    1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1102,
    1103, 1102, 1103, 1102, 1103, 1102, 1103, 1102, 1103, 1102, 1103, 1102,
    1103, 1102, 1103, 1102, 1103, 1102, 1103, 1102, 1103, 1831, 1831, 1894,
    1894, 1894, 1894, 1894, 1894, 1831, 1894, 1894, 1894, 1894, 1894, 1894,
    1894, 1894, 1894, 1894, 1894, 1894, 1894, 1894, 1831, 1831, 1831, 1831,
    1831, 1831, 1831, 1831, 1894, 1831, 1831, 1831, 1831, 1831, 1831, 1831,
    1894, 1894, 1894, 1894, 1894, 1894, 1831, 1831, 1831, 1894, 1831, 1831,
    1831, 1831, 1894, 1894, 1894, 1894, 1894, 1831, 1894, 1894, 1831, 1831,
    1102, 1103, 1102, 1103, 1894, 1831, 1831, 1831, 1831, 1894, 1831, 1894,
    1894, 1894, 1831, 1831, 1894, 1894, 1831, 1831, 1831, 1831, 1831, 1831,
    1831, 1831, 1831, 1831, 1894, 1894, 1894, 1894, 1894, 1894, 1831, 1831,
    1102, 1103, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831,
    1831, 1831, 1894, 1894, 1897, 1894, 1894, 1894, 1894, 1894, 1894, 1894,
    1894, 1894, 1894, 1894, 1894, 1894, 1894, 1894, 1894, 1831, 1894, 1894,
    1894, 1894, 1831, 1831, 1894, 1831, 1894, 1831, 1831, 1894, 1831, 1894,
    1894, 1894, 1894, 1831, 1831, 1831, 1831, 1831, 1894, 1894, 1831, 1831,
    1831, 1831, 1831, 1831, 1894, 1894, 1894, 1831, 1831, 1831, 1831, 1831,
    1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831,
    1831, 1831, 1831, 1831, 1831, 1831, 1831, 1894, 1894, 1831, 1831, 1831,
    1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1894, 1894, 1831, 1831,
    1831, 1831, 1894, 1894, 1894, 1894, 1831, 1894, 1894, 1831, 1831, 1894,
    1897, 1958, 1958, 1831, 1831, 1894, 1894, 1894, 1894, 1894, 1894, 1894,
    1894, 1894, 1894, 1894, 1894, 1894, 1894, 1894, 1894, 1894, 1894, 1894,
    1894, 1894, 1894, 1894, 1894, 1894, 1894, 1894, 1894, 1894, 1894, 1894,
    1894, 1894, 1894, 1894, 1894, 1894, 1894, 1894, 1894, 1894, 1894, 1894,
    1831, 1831, 1894, 1894, 1894, 1894, 1894, 1894, 1894, 1894, 1831, 1894,
    1894, 1894, 1894, 1894, 1894, 1894, 1894, 1894, 1894, 1894, 1894, 1894,
    1894, 1894, 1894, 1894, 1894, 1894, 1894, 1894, 1894, 1894, 1894, 1894,
    1894, 1894, 1894, 1894, 1894, 1894, 1894, 1894, 1894, 1894, 1894, 1894,
    1894, 1894, 1894, 1831, 1831, 1831, 1831, 1831, 1894, 1831, 1894, 1831,
    1831, 1831, 1894, 1894, 1894, 1894, 1894, 1831, 1831, 1831, 1831, 1831,
    1894, 1894, 1894, 1831, 1831, 1831, 1831, 1894, 1831, 1831, 1831, 1894,
    1894, 1894, 1894, 1894, 1831, 1894, 1831, 1831, 1090, 1090, 1090, 1090,
    1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090,
    1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1898,
    1898, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090,
    1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1831, 1831, 1831, 1831,
    1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831,
    1831, 1831, 1831, 1831, 1831, 1090, 1090, 1831, 1831, 1831, 1831, 1831,
    1831, 1090, 1090, 1090, 1898, 1090, 1090, 1090, 1090, 1898, 1090, 1090,
    1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090,
    1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090,
    1090, 1090, 1090, 1090, 525,  525,  1090, 1090, 1090, 1090, 1090, 1090,
    1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090,
    1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090,
    1090, 1090, 525,  1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090,
    1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090,
    1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090,
    1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090,
    1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090,
    1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090,
    1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090,
    1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090,
    1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1959, 1090,
    1960, 1961, 1962, 1963, 1964, 1965, 1966, 1967, 1968, 1969, 1970, 1971,
    1972, 1973, 1974, 1975, 1976, 1977, 1978, 1979, 1980, 1981, 1982, 1983,
    1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991, 1992, 1993, 1994, 1995,
    1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007,
    2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
    2020, 2021, 2022, 2023, 2024, 2025, 2026, 2027, 2028, 2029, 2030, 2031,
    2032, 2033, 2034, 2035, 2036, 2037, 2038, 2039, 2040, 2041, 2042, 2043,
    2044, 2045, 2046, 2047, 2048, 2049, 2050, 2051, 2052, 2053, 2054, 2055,
    2056, 2057, 2058, 2059, 2060, 2061, 2062, 2063, 2064, 2065, 2066, 2067,
    2068, 2069, 2070, 2071, 2072, 1354, 2073, 2074, 1354, 2075, 2076, 1354,
    1354, 1354, 1354, 1354, 1355, 1191, 2077, 2078, 2079, 2080, 2081, 2082,
    2083, 2084, 2085, 2086, 2087, 2088, 2089, 2090, 2091, 2092, 2093, 2094,
    2095, 2096, 2097, 2098, 2099, 2100, 2101, 2102, 2103, 2104, 2105, 2106,
    2107, 2108, 2109, 2110, 2111, 2112, 2113, 2114, 2115, 2116, 2117, 2118,
    2119, 2120, 2121, 2122, 2123, 2124, 2125, 2126, 2127, 2128, 2129, 2130,
    2131, 2132, 2133, 2134, 2135, 2136, 2137, 2138, 2139, 2140, 2141, 2142,
    2143, 2144, 2145, 2146, 2147, 2148, 2149, 2150, 2151, 2152, 2153, 2154,
    2155, 2156, 2157, 2158, 2159, 2160, 2161, 2162, 2163, 2164, 2165, 2166,
    2167, 2168, 2169, 2170, 2171, 2172, 2173, 2174, 2175, 2176, 2177, 2178,
    1354, 1090, 1090, 1090, 1090, 1090, 1090, 2179, 2180, 2181, 2182, 1068,
    1068, 1068, 2183, 2184, 525,  525,  525,  525,  525,  1292, 1292, 1292,
    1292, 1093, 1292, 1292, 2185, 2186, 2187, 2188, 2189, 2190, 2191, 2192,
    2193, 2194, 2195, 2196, 2197, 2198, 2199, 2200, 2201, 2202, 2203, 2204,
    2205, 2206, 2207, 2208, 2209, 2210, 2211, 2212, 2213, 2214, 2215, 2216,
    2217, 2218, 2219, 2220, 2221, 2222, 525,  2223, 525,  525,  525,  525,
    525,  2224, 525,  525,  1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    525,  525,  525,  525,  525,  525,  525,  1191, 1084, 525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  1083,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 525,
    525,  525,  525,  525,  525,  525,  525,  525,  1081, 1081, 1081, 1081,
    1081, 1081, 1081, 525,  1081, 1081, 1081, 1081, 1081, 1081, 1081, 525,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 525,  1081, 1081, 1081, 1081,
    1081, 1081, 1081, 525,  1081, 1081, 1081, 1081, 1081, 1081, 1081, 525,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 525,  1081, 1081, 1081, 1081,
    1081, 1081, 1081, 525,  1081, 1081, 1081, 1081, 1081, 1081, 1081, 525,
    1068, 1068, 1068, 1068, 1068, 1068, 1068, 1068, 1068, 1068, 1068, 1068,
    1068, 1068, 1068, 1068, 1068, 1068, 1068, 1068, 1068, 1068, 1068, 1068,
    1068, 1068, 1068, 1068, 1068, 1068, 1068, 1068, 1292, 1292, 1827, 1828,
    1827, 1828, 1292, 1292, 1292, 1827, 1828, 1292, 1827, 1828, 1292, 1292,
    1292, 1292, 1292, 1292, 1292, 1292, 1292, 1288, 1292, 1292, 1288, 1292,
    1827, 1828, 1292, 1292, 1827, 1828, 1102, 1103, 1102, 1103, 1102, 1103,
    1102, 1103, 1292, 1292, 1292, 1292, 1292, 2225, 1292, 1292, 1292, 1292,
    1292, 1292, 1292, 1292, 1292, 1292, 1288, 1288, 1292, 1292, 1292, 1292,
    1288, 1292, 1816, 1292, 1292, 1292, 1292, 1292, 1292, 1292, 1292, 1292,
    1292, 1292, 1292, 1292, 1090, 1090, 1292, 1292, 1292, 1102, 1103, 1102,
    1103, 1102, 1103, 1102, 1103, 1288, 525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898,
    1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898,
    1898, 1898, 1898, 1898, 1898, 1898, 525,  1898, 1898, 1898, 1898, 2226,
    1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898,
    1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898,
    1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898,
    1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898,
    1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898,
    1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898,
    1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 2226,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226,
    2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226,
    2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226,
    2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226,
    2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226,
    2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226,
    2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226,
    2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226,
    2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226,
    2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226,
    2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226,
    2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226,
    2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226,
    2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226,
    2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226,
    2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226,
    2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226,
    2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226, 2226, 525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898,
    1898, 1898, 1898, 1898, 2227, 2228, 2228, 2228, 1898, 2229, 1195, 2230,
    1899, 1900, 1899, 1900, 1899, 1900, 1899, 1900, 1899, 1900, 1898, 1898,
    1899, 1900, 1899, 1900, 1899, 1900, 1899, 1900, 2231, 2232, 2233, 2233,
    1898, 2230, 2230, 2230, 2230, 2230, 2230, 2230, 2230, 2230, 2234, 2235,
    2236, 2237, 2238, 2238, 2231, 2229, 2229, 2229, 2229, 2229, 2226, 1898,
    2239, 2239, 2239, 2229, 1195, 2228, 1898, 1090, 525,  1195, 1195, 1195,
    1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195,
    1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195,
    1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195,
    1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195,
    1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195,
    1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195,
    1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 525,
    525,  2240, 2240, 2241, 2241, 2229, 2229, 2242, 2231, 1195, 1195, 1195,
    1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195,
    1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195,
    1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195,
    1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195,
    1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195,
    1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195,
    1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195,
    1195, 1195, 1195, 2228, 2229, 2229, 2229, 2242, 525,  525,  525,  525,
    525,  1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195,
    1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195,
    1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195,
    1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 525,  2243, 2243, 2243,
    2243, 2243, 2243, 2243, 2243, 2243, 2243, 2243, 2243, 2243, 2243, 2243,
    2243, 2243, 2243, 2243, 2243, 2243, 2243, 2243, 2243, 2243, 2243, 2243,
    2243, 2243, 2243, 2243, 2243, 2243, 2243, 2243, 2243, 2243, 2243, 2243,
    2243, 2243, 2243, 2243, 2243, 2243, 2243, 2243, 2243, 2243, 2243, 2243,
    2243, 2243, 2243, 2243, 2243, 2243, 2243, 2243, 2243, 2243, 2243, 2243,
    2243, 2243, 2243, 2243, 2243, 2243, 2243, 2243, 2243, 2243, 2243, 2243,
    2243, 2243, 2243, 2243, 2243, 2243, 2243, 2243, 2243, 2243, 2243, 2243,
    2243, 2243, 2243, 2243, 2243, 2243, 2243, 525,  2244, 2244, 2245, 2245,
    2245, 2245, 2246, 2246, 2246, 2246, 2246, 2246, 2246, 2246, 2246, 2246,
    1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195,
    1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195,
    1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1898, 1898, 1898, 1898,
    1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898,
    1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898,
    1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 525,  525,
    525,  525,  525,  525,  525,  525,  525,  1898, 1195, 1195, 1195, 1195,
    1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195,
    2247, 2247, 2247, 2247, 2247, 2247, 2247, 2247, 2247, 2247, 2247, 2247,
    2247, 2247, 2247, 2247, 2247, 2247, 2247, 2247, 2247, 2247, 2247, 2247,
    2247, 2247, 2247, 2247, 2247, 2226, 2226, 525,  2248, 2248, 2248, 2248,
    2248, 2248, 2248, 2248, 2248, 2248, 2247, 2247, 2247, 2247, 2247, 2247,
    2247, 2247, 2247, 2247, 2247, 2247, 2247, 2247, 2247, 2247, 2247, 2247,
    2247, 2247, 2247, 2247, 2247, 2247, 2247, 2247, 2249, 2249, 2249, 2249,
    1088, 1088, 1088, 1088, 1088, 1088, 1088, 1088, 2250, 2251, 2251, 2251,
    2251, 2251, 2251, 2251, 2251, 2251, 2251, 2251, 2251, 2251, 2251, 2251,
    2249, 2249, 2249, 2249, 2249, 2249, 2249, 2249, 2249, 2249, 2249, 2249,
    2249, 2249, 2249, 2249, 2249, 2249, 2249, 2249, 2249, 2249, 2249, 2249,
    2249, 2249, 2249, 2249, 2252, 2252, 2252, 2244, 2253, 2253, 2253, 2253,
    2253, 2253, 2253, 2253, 2253, 2253, 2249, 2249, 2249, 2249, 2249, 2249,
    2249, 2249, 2249, 2249, 2249, 2249, 2249, 2249, 2249, 2249, 2249, 2249,
    2249, 2249, 2249, 2249, 2249, 2249, 2249, 2249, 2249, 2249, 2249, 2249,
    2249, 2249, 2249, 2249, 2249, 2249, 2249, 2249, 2249, 2251, 2251, 2251,
    2251, 2251, 2251, 2251, 2251, 2251, 2251, 2251, 2251, 2251, 2251, 2251,
    2247, 2247, 2247, 2247, 2247, 2247, 2247, 2247, 2247, 2247, 2247, 2247,
    2250, 2250, 2250, 2250, 2249, 2249, 2249, 2249, 2249, 2249, 2249, 2249,
    2249, 2249, 2249, 2249, 2249, 2249, 2249, 2249, 2249, 2249, 2249, 2249,
    2249, 2249, 2249, 2249, 2249, 2249, 2249, 2249, 2249, 2249, 2249, 2249,
    2249, 2249, 2249, 2249, 2249, 2249, 2249, 2249, 2249, 2249, 2249, 2249,
    2249, 2249, 2249, 2254, 2254, 2254, 2254, 2254, 2254, 2254, 2254, 2254,
    2254, 2254, 2254, 2254, 2254, 2254, 2254, 2254, 2254, 2254, 2254, 2254,
    2254, 2254, 2254, 2254, 2254, 2254, 2254, 2254, 2254, 2254, 2254, 2254,
    2254, 2254, 2254, 2254, 2254, 2254, 2254, 2254, 2254, 2254, 2254, 2254,
    2254, 2254, 2254, 2254, 2254, 2254, 2254, 2254, 2254, 2254, 2254, 2254,
    2254, 2254, 2254, 2254, 2254, 2254, 2254, 2254, 2254, 2254, 2254, 2254,
    2254, 2254, 2254, 2254, 2254, 2254, 2254, 2254, 2254, 2254, 2254, 2254,
    2254, 2254, 2254, 2254, 2254, 2254, 2254, 2254, 2247, 2247, 2247, 2247,
    2247, 2247, 2247, 2247, 2247, 2247, 2247, 2247, 2247, 2247, 2247, 2247,
    2247, 2247, 2247, 2247, 2247, 2247, 2247, 2247, 2247, 2254, 2254, 2254,
    2254, 2254, 2254, 2250, 2250, 2250, 2250, 2254, 2254, 2254, 2254, 2254,
    2254, 2254, 2254, 2254, 2254, 2254, 2254, 2254, 2254, 2254, 2254, 2254,
    2254, 2254, 2254, 2254, 2254, 2254, 2254, 2254, 2254, 2254, 2254, 2254,
    2254, 2254, 2254, 2254, 2254, 2254, 2254, 2254, 2254, 2254, 2254, 2254,
    2254, 2254, 2254, 2254, 2254, 2254, 2254, 2254, 2254, 2254, 2254, 2254,
    2254, 2254, 2254, 2254, 2254, 2254, 2254, 2254, 2254, 2254, 2254, 2254,
    2254, 2254, 2254, 2254, 2254, 2254, 2254, 2254, 2254, 2254, 2254, 2254,
    2254, 2254, 2254, 2254, 2254, 2254, 2254, 2254, 2254, 2254, 2254, 2254,
    2254, 2254, 2254, 2254, 2254, 2254, 2254, 2254, 2254, 2254, 2250, 2250,
    2247, 2247, 2247, 2247, 2247, 2247, 2247, 2247, 2247, 2247, 2247, 2247,
    2247, 2247, 2247, 2247, 2247, 2247, 2247, 2247, 2247, 2247, 2247, 2247,
    2247, 2247, 2247, 2247, 2247, 2247, 2247, 2250, 1195, 525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  1195, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898,
    1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898,
    1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898,
    1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898,
    1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898,
    1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195,
    1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195,
    1195, 2229, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195,
    1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195,
    1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195,
    1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195,
    1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195,
    1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195,
    1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195,
    1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195,
    1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195,
    1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195,
    1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195,
    1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195,
    1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195,
    1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195,
    1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195,
    1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195,
    1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195,
    1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195,
    1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195,
    1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195,
    1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 525,  525,  525,
    1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898,
    1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898,
    1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898,
    1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898, 1898,
    1898, 1898, 1898, 1898, 1898, 1898, 1898, 525,  525,  525,  525,  525,
    525,  525,  525,  525,  1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1086, 1086, 1086, 1086,
    1086, 1086, 1084, 1084, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1086, 1292, 1292, 1292, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1085, 1085, 1085, 1085, 1085, 1085, 1085, 1085, 1085, 1085, 1081, 1081,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  2255, 2256, 2257, 2258,
    2259, 2260, 2261, 2262, 2263, 2264, 2265, 1304, 2266, 2267, 2268, 2269,
    2270, 2271, 2272, 2273, 2274, 2275, 2276, 2277, 2278, 2279, 2280, 2281,
    2282, 2283, 2284, 2285, 2286, 2287, 2288, 2289, 2290, 2291, 2292, 2293,
    2294, 2295, 2296, 2297, 2298, 2299, 1081, 1068, 1296, 1296, 1296, 1292,
    1068, 1068, 1068, 1068, 1068, 1068, 1068, 1068, 1068, 1068, 1292, 2225,
    2300, 2301, 2302, 2303, 2304, 2305, 2306, 2307, 2308, 2309, 2310, 2311,
    2312, 2313, 2314, 2315, 2316, 2317, 2318, 2319, 2320, 2321, 2322, 2323,
    2324, 2325, 2326, 2327, 1191, 1191, 1068, 1068, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1290, 1290, 1290, 1290, 1290, 1290,
    1290, 1290, 1290, 1290, 1068, 1068, 1084, 1084, 1084, 1084, 1084, 1084,
    525,  525,  525,  525,  525,  525,  525,  525,  1781, 1781, 1781, 1781,
    1781, 1781, 1781, 1781, 1781, 1781, 1781, 1781, 1781, 1781, 1781, 1781,
    1781, 1781, 1781, 1781, 1781, 1781, 1781, 2225, 2225, 2225, 2225, 2225,
    2225, 2225, 2225, 2225, 1781, 1781, 2328, 2329, 2330, 2331, 2332, 2333,
    2334, 2335, 2336, 2337, 2338, 2339, 2340, 2341, 1354, 1354, 2342, 2343,
    2344, 2345, 2346, 2347, 2348, 2349, 2350, 2351, 2352, 2353, 2354, 2355,
    2356, 2357, 2358, 2359, 2360, 2361, 2362, 2363, 2364, 2365, 2366, 2367,
    2368, 2369, 2370, 2371, 2372, 2373, 2374, 2375, 2376, 2377, 2378, 2379,
    2380, 2381, 2382, 2383, 2384, 2385, 2386, 2387, 2388, 2389, 2390, 2391,
    2392, 2393, 2394, 2395, 2396, 2397, 2398, 2399, 2400, 2401, 2402, 2403,
    1191, 1354, 1354, 1354, 1354, 1354, 1354, 1354, 1354, 2404, 2405, 2406,
    2407, 2408, 2409, 2410, 2411, 2412, 2413, 2414, 2415, 2416, 2417, 2418,
    2225, 2419, 2419, 2420, 2421, 2422, 1354, 1081, 2423, 2424, 2425, 2426,
    2427, 1354, 2428, 2429, 2430, 2431, 2432, 2433, 2434, 2435, 2436, 2437,
    2438, 2439, 2440, 2441, 2442, 2443, 2444, 2445, 2446, 2447, 2448, 2449,
    2450, 2451, 2452, 1354, 2453, 2454, 2455, 2456, 2457, 2458, 2459, 2460,
    2461, 2462, 2463, 2464, 2465, 2466, 2467, 2468, 2469, 2470, 2471, 2472,
    2473, 2474, 2475, 2476, 2477, 2478, 2479, 2480, 2481, 2482, 525,  525,
    2483, 2484, 525,  1354, 525,  1354, 2485, 2486, 2487, 2488, 2489, 2490,
    2491, 525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  1191, 1191,
    1191, 2492, 2493, 1081, 1191, 1191, 1354, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1079, 1081, 1081, 1081, 1083, 1081, 1081, 1081, 1081, 1079,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1080,
    1080, 1079, 1079, 1080, 1090, 1090, 1090, 1090, 1083, 525,  525,  525,
    1088, 1088, 1088, 1088, 1088, 1088, 1089, 1089, 1087, 1855, 525,  525,
    525,  525,  525,  525,  1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1292, 1292, 1292, 1292,
    525,  525,  525,  525,  525,  525,  525,  525,  1080, 1080, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1080, 1080, 1080, 1080, 1080, 1080, 1080, 1080, 1080, 1080, 1080, 1080,
    1080, 1080, 1080, 1080, 1083, 1079, 525,  525,  525,  525,  525,  525,
    525,  525,  1084, 1084, 1085, 1085, 1085, 1085, 1085, 1085, 1085, 1085,
    1085, 1085, 525,  525,  525,  525,  525,  525,  1068, 1068, 1068, 1068,
    1068, 1068, 1068, 1068, 1068, 1068, 1068, 1068, 1068, 1068, 1068, 1068,
    1068, 1068, 1081, 1081, 1081, 1081, 1081, 1081, 1084, 1084, 1084, 1081,
    1084, 1081, 1081, 1079, 1085, 1085, 1085, 1085, 1085, 1085, 1085, 1085,
    1085, 1085, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1079, 1079, 1079, 1079, 1079, 1071,
    1071, 1071, 1084, 1084, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1079, 1079, 1079, 1079, 1079, 1079, 1079, 1079, 1079,
    1079, 1079, 1080, 1291, 525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  1084, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195,
    1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195,
    1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 525,  525,  525,
    1079, 1079, 1079, 1080, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1082, 1080, 1080, 1079, 1079, 1079, 1079, 1080, 1080,
    1079, 1079, 1080, 1080, 1291, 1084, 1084, 1084, 1084, 1084, 1084, 1084,
    1084, 1084, 1084, 1084, 1084, 1084, 525,  1086, 1085, 1085, 1085, 1085,
    1085, 1085, 1085, 1085, 1085, 1085, 525,  525,  525,  525,  1084, 1084,
    1081, 1081, 1081, 1081, 1081, 1079, 1086, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1085, 1085, 1085, 1085, 1085, 1085, 1085, 1085,
    1085, 1085, 1081, 1081, 1081, 1081, 1081, 525,  1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1079, 1079, 1079, 1079, 1079, 1079, 1080, 1080, 1079, 1079, 1080,
    1080, 1079, 1079, 525,  525,  525,  525,  525,  525,  525,  525,  525,
    1081, 1081, 1081, 1079, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1079, 1080, 525,  525,  1085, 1085, 1085, 1085, 1085, 1085, 1085, 1085,
    1085, 1085, 525,  525,  1084, 1084, 1084, 1084, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1086, 1081, 1081, 1081, 1081, 1081, 1081, 1089, 1089, 1089, 1081, 1080,
    1079, 1080, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1068, 1081, 1068, 1068, 1071, 1081, 1081, 1068,
    1068, 1081, 1081, 1081, 1081, 1081, 1068, 1068, 1081, 1068, 1081, 525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  1081,
    1081, 1086, 1084, 1084, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1080, 1079, 1079, 1080, 1080, 1084, 1084, 1081, 1086,
    1086, 1080, 1083, 525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  1081, 1081, 1081, 1081, 1081, 1081, 525,  525,  1081, 1081, 1081,
    1081, 1081, 1081, 525,  525,  1081, 1081, 1081, 1081, 1081, 1081, 525,
    525,  525,  525,  525,  525,  525,  525,  525,  1081, 1081, 1081, 1081,
    1081, 1081, 1081, 525,  1081, 1081, 1081, 1081, 1081, 1081, 1081, 525,
    1354, 1354, 1354, 1354, 1354, 1354, 1354, 1354, 1354, 1354, 1354, 1354,
    1354, 1354, 1354, 1354, 1354, 1354, 1354, 1354, 1354, 1354, 1354, 1354,
    1354, 1354, 1354, 1354, 1354, 1354, 1354, 1354, 1354, 1354, 1354, 2494,
    1354, 1354, 1354, 1354, 1354, 1354, 1354, 2419, 1191, 1191, 1191, 1191,
    1354, 1354, 1354, 1354, 1354, 1354, 1354, 1354, 1354, 1191, 1781, 1781,
    525,  525,  525,  525,  2495, 2496, 2497, 2498, 2499, 2500, 2501, 2502,
    2503, 2504, 2505, 2506, 2507, 2508, 2509, 2510, 2511, 2512, 2513, 2514,
    2515, 2516, 2517, 2518, 2519, 2520, 2521, 2522, 2523, 2524, 2525, 2526,
    2527, 2528, 2529, 2530, 2531, 2532, 2533, 2534, 2535, 2536, 2537, 2538,
    2539, 2540, 2541, 2542, 2543, 2544, 2545, 2546, 2547, 2548, 2549, 2550,
    2551, 2552, 2553, 2554, 2555, 2556, 2557, 2558, 2559, 2560, 2561, 2562,
    2563, 2564, 2565, 2566, 2567, 2568, 2569, 2570, 2571, 2572, 2573, 2574,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1080,
    1080, 1079, 1080, 1080, 1079, 1080, 1080, 1084, 1080, 1083, 525,  525,
    1085, 1085, 1085, 1085, 1085, 1085, 1085, 1085, 1085, 1085, 525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  1195, 525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 525,  525,  525,  525,  1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081,
    1081, 1081, 1081, 1081, 1081, 1081, 1081, 1081, 525,  525,  525,  525,
    2575, 525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  2575, 2575, 525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  2575,
    2576, 525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  2576, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195,
    1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195,
    1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195,
    1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195,
    1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195,
    1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195,
    1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195,
    1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195,
    1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195,
    1195, 1195, 1195, 1195, 1195, 1195, 525,  525,  1195, 1195, 1195, 1195,
    1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195,
    1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195,
    1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195,
    1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195,
    1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195,
    1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195,
    1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195,
    1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195, 1195,
    1195, 1195, 1195, 1195, 1195, 1195, 525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  1515, 1515, 1515, 1515,
    1515, 1515, 1515, 525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  1515, 1515, 1515, 1515, 1515, 525,  525,  525,  525,
    525,  1067, 2577, 1067, 2578, 2578, 2578, 2578, 2578, 2578, 2578, 2578,
    2578, 2579, 1067, 1067, 1067, 1067, 1067, 1067, 1067, 1067, 1067, 1067,
    1067, 1067, 1067, 525,  1067, 1067, 1067, 1067, 1067, 525,  1067, 525,
    1067, 1067, 525,  1067, 1067, 525,  1067, 1067, 1067, 1067, 1067, 1067,
    1067, 1067, 1067, 2580, 2581, 2582, 2581, 2582, 2583, 2584, 2581, 2582,
    2583, 2584, 2581, 2582, 2583, 2584, 2581, 2582, 2583, 2584, 2581, 2582,
    2583, 2584, 2581, 2582, 2583, 2584, 2581, 2582, 2583, 2584, 2581, 2582,
    2583, 2584, 2581, 2582, 2583, 2584, 2581, 2582, 2583, 2584, 2581, 2582,
    2583, 2584, 2581, 2582, 2583, 2584, 2581, 2582, 2581, 2582, 2581, 2582,
    2581, 2582, 2581, 2582, 2581, 2582, 2581, 2582, 2583, 2584, 2581, 2582,
    2583, 2584, 2581, 2582, 2583, 2584, 2581, 2582, 2583, 2584, 2581, 2582,
    2581, 2582, 2583, 2584, 2581, 2582, 2581, 2582, 2583, 2584, 2581, 2582,
    2583, 2584, 2581, 2582, 2581, 2582, 1073, 1073, 1073, 1073, 1073, 1073,
    1073, 1073, 1073, 1073, 1073, 1073, 1073, 1073, 1073, 1073, 1073, 525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  2581, 2582, 2583, 2584, 2581, 2582, 2581, 2582, 2581,
    2582, 2581, 2581, 2582, 2581, 2582, 2581, 2582, 2581, 2582, 2583, 2584,
    2583, 2584, 2581, 2582, 2581, 2582, 2581, 2582, 2581, 2582, 2581, 2582,
    2581, 2582, 2581, 2582, 2583, 2581, 2582, 2583, 2581, 2582, 2583, 2584,
    2581, 2581, 2581, 2581, 2581, 2581, 2581, 2581, 2581, 2581, 2581, 2581,
    2581, 2581, 2581, 2581, 2581, 2581, 2581, 2581, 2581, 2581, 2581, 2581,
    2581, 2581, 2581, 2581, 2581, 2581, 2581, 2581, 2581, 2581, 2581, 2581,
    2581, 2581, 2581, 2581, 2581, 2581, 2581, 2581, 2581, 2581, 2581, 2581,
    2581, 2581, 2581, 2581, 2581, 2581, 2581, 2581, 2581, 2581, 2581, 2581,
    2581, 2581, 2581, 2581, 2581, 2581, 2581, 2581, 2581, 2581, 2581, 2581,
    2581, 2581, 2581, 2581, 2581, 2581, 2581, 2581, 2581, 2581, 2581, 2581,
    2581, 2581, 2581, 2581, 2581, 2581, 2581, 2581, 2581, 2581, 2581, 2581,
    2581, 2581, 2581, 2581, 2582, 2582, 2582, 2582, 2582, 2582, 2582, 2582,
    2582, 2582, 2582, 2582, 2582, 2582, 2582, 2582, 2582, 2582, 2582, 2582,
    2582, 2582, 2582, 2582, 2582, 2582, 2582, 2582, 2582, 2582, 2582, 2582,
    2582, 2582, 2582, 2582, 2582, 2582, 2582, 2582, 2582, 2582, 2582, 2582,
    2582, 2582, 2582, 2582, 2582, 2582, 2582, 2583, 2583, 2583, 2583, 2583,
    2583, 2583, 2583, 2583, 2583, 2583, 2583, 2583, 2583, 2583, 2583, 2583,
    2583, 2583, 2583, 2583, 2583, 2583, 2583, 2583, 2583, 2583, 2583, 2583,
    2583, 2583, 2583, 2583, 2583, 2583, 2583, 2583, 2583, 2583, 2583, 2583,
    2583, 2583, 2583, 2583, 2583, 2583, 2583, 2583, 2583, 2583, 2583, 2583,
    2583, 2583, 2583, 2583, 2583, 2583, 2583, 2583, 2583, 2583, 2583, 2583,
    2583, 2583, 2583, 2583, 2583, 2583, 2583, 2584, 2584, 2584, 2584, 2584,
    2584, 2584, 2584, 2584, 2584, 2584, 2584, 2584, 2584, 2584, 2584, 2584,
    2584, 2584, 2584, 2584, 2584, 2581, 2581, 2581, 2581, 2581, 2581, 2581,
    2581, 2581, 2581, 2581, 2581, 2581, 2581, 2581, 2581, 2581, 2581, 2581,
    2581, 2581, 2581, 2581, 2581, 2581, 2581, 2581, 2581, 2582, 2582, 2582,
    2582, 2582, 2582, 2582, 2582, 2582, 2582, 2582, 2582, 2582, 2582, 2582,
    2582, 2582, 2582, 2582, 2582, 2582, 2582, 2582, 2582, 2582, 2582, 2582,
    2582, 2583, 2583, 2583, 2583, 2583, 2583, 2583, 2584, 2584, 2584, 2584,
    2584, 2584, 2584, 2584, 2582, 2581, 2585, 1816, 1090, 1090, 1090, 1090,
    1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090, 1090,
    2583, 2582, 2583, 2583, 2583, 2583, 2583, 2583, 2582, 2583, 2582, 2582,
    2583, 2583, 2582, 2582, 2583, 2583, 2582, 2583, 2582, 2583, 2582, 2582,
    2583, 2582, 2582, 2583, 2582, 2583, 2582, 2582, 2583, 2582, 2583, 2583,
    2582, 2582, 2582, 2583, 2582, 2582, 2582, 2582, 2582, 2583, 2582, 2582,
    2582, 2582, 2582, 2583, 2582, 2582, 2583, 2582, 2583, 2583, 2583, 2582,
    2583, 2583, 2583, 2583, 525,  525,  2583, 2583, 2583, 2583, 2582, 2582,
    2583, 2582, 2582, 2582, 2582, 2583, 2582, 2582, 2582, 2582, 2582, 2582,
    2582, 2582, 2582, 2582, 2582, 2582, 2582, 2582, 2582, 2582, 2582, 2582,
    2582, 2582, 2582, 2582, 2583, 2583, 2582, 2582, 2583, 2582, 2583, 2582,
    2582, 2582, 2582, 2582, 2582, 2582, 2582, 2583, 2583, 2583, 2582, 2582,
    525,  525,  525,  525,  525,  525,  525,  1090, 525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  2581, 2581, 2581, 2581, 2581, 2581, 2581, 2581,
    2581, 2581, 2581, 2581, 2586, 1090, 1090, 1090, 1079, 1079, 1079, 1079,
    1079, 1079, 1079, 1079, 1079, 1079, 1079, 1079, 1079, 1079, 1079, 1079,
    2587, 2587, 2587, 2587, 2587, 2587, 2587, 2588, 2589, 2587, 525,  525,
    525,  525,  525,  525,  1068, 1068, 1068, 1068, 1068, 1068, 1068, 1071,
    1071, 1071, 1071, 1071, 1071, 1071, 1068, 1068, 2587, 2590, 2590, 2591,
    2591, 2588, 2589, 2588, 2589, 2588, 2589, 2588, 2589, 2588, 2589, 2588,
    2589, 2588, 2589, 2588, 2589, 2228, 2228, 2588, 2589, 2592, 2592, 2592,
    2592, 2593, 2593, 2593, 2594, 2595, 2594, 525,  2595, 2594, 2595, 2595,
    2596, 2597, 2598, 2597, 2598, 2597, 2598, 2599, 2595, 2595, 2600, 2601,
    2602, 2602, 2603, 525,  2595, 2604, 2599, 2595, 525,  525,  525,  525,
    2581, 2584, 2581, 1072, 2581, 525,  2581, 2584, 2581, 2584, 2581, 2584,
    2581, 2584, 2581, 2584, 2581, 2581, 2582, 2581, 2582, 2581, 2582, 2581,
    2582, 2581, 2582, 2583, 2584, 2581, 2582, 2581, 2582, 2583, 2584, 2581,
    2582, 2581, 2582, 2583, 2584, 2581, 2582, 2583, 2584, 2581, 2582, 2583,
    2584, 2581, 2582, 2583, 2584, 2581, 2582, 2583, 2584, 2581, 2582, 2581,
    2582, 2581, 2582, 2581, 2582, 2581, 2582, 2583, 2584, 2581, 2582, 2583,
    2584, 2581, 2582, 2583, 2584, 2581, 2582, 2583, 2584, 2581, 2582, 2583,
    2584, 2581, 2582, 2583, 2584, 2581, 2582, 2583, 2584, 2581, 2582, 2583,
    2584, 2581, 2582, 2583, 2584, 2581, 2582, 2583, 2584, 2581, 2582, 2583,
    2584, 2581, 2582, 2583, 2584, 2581, 2582, 2583, 2584, 2581, 2582, 2583,
    2584, 2581, 2582, 2583, 2584, 2581, 2582, 2581, 2582, 2581, 2582, 2583,
    2584, 2581, 2582, 2581, 2582, 2581, 2582, 2581, 2582, 525,  525,  1293,
    525,  2605, 2605, 2606, 2607, 2606, 2605, 2605, 2608, 2609, 2605, 2610,
    2611, 2612, 2611, 2611, 2613, 2613, 2613, 2613, 2613, 2613, 2613, 2613,
    2613, 2613, 2611, 2605, 2614, 2615, 2614, 2605, 2605, 2616, 2617, 2618,
    2619, 2620, 2621, 2622, 2623, 2624, 2625, 2626, 2627, 2628, 2629, 2630,
    2631, 2632, 2633, 2634, 2635, 2636, 2637, 2638, 2639, 2640, 2641, 2608,
    2605, 2609, 2642, 2643, 2642, 2644, 2645, 2646, 2647, 2648, 2649, 2650,
    2651, 2652, 2653, 2654, 2655, 2656, 2657, 2658, 2659, 2660, 2661, 2662,
    2663, 2664, 2665, 2666, 2667, 2668, 2669, 2608, 2615, 2609, 2615, 2608,
    2609, 2670, 2671, 2672, 2670, 2670, 2673, 2673, 2673, 2673, 2673, 2673,
    2673, 2673, 2673, 2673, 2674, 2673, 2673, 2673, 2673, 2673, 2673, 2673,
    2673, 2673, 2673, 2673, 2673, 2673, 2673, 2673, 2673, 2673, 2673, 2673,
    2673, 2673, 2673, 2673, 2673, 2673, 2673, 2673, 2673, 2673, 2673, 2673,
    2673, 2673, 2673, 2673, 2673, 2673, 2673, 2673, 2673, 2673, 2673, 2673,
    2673, 2673, 2674, 2674, 2673, 2673, 2673, 2673, 2673, 2673, 2673, 2673,
    2673, 2673, 2673, 2673, 2673, 2673, 2673, 2673, 2673, 2673, 2673, 2673,
    2673, 2673, 2673, 2673, 2673, 2673, 2673, 2673, 2673, 2673, 2673, 525,
    525,  525,  2673, 2673, 2673, 2673, 2673, 2673, 525,  525,  2673, 2673,
    2673, 2673, 2673, 2673, 525,  525,  2673, 2673, 2673, 2673, 2673, 2673,
    525,  525,  2673, 2673, 2673, 525,  525,  525,  2607, 2607, 2615, 2642,
    2675, 2607, 2607, 525,  2676, 2677, 2677, 2677, 2677, 2676, 2676, 525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  2678, 2678, 2678,
    1090, 1090, 525,  525,  2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 525,  2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 525,  2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 525,  2679, 2679, 525,  2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 525,  525,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 525,  525,  525,  525,  525,  2680, 2681, 2680, 525,
    525,  525,  525,  2682, 2682, 2682, 2682, 2682, 2682, 2682, 2682, 2682,
    2682, 2682, 2682, 2682, 2682, 2682, 2682, 2682, 2682, 2682, 2682, 2682,
    2682, 2682, 2682, 2682, 2682, 2682, 2682, 2682, 2682, 2682, 2682, 2682,
    2682, 2682, 2682, 2682, 2682, 2682, 2682, 2682, 2682, 2682, 2682, 2682,
    525,  525,  525,  2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683,
    2684, 2684, 2684, 2684, 2684, 2684, 2684, 2684, 2684, 2684, 2684, 2684,
    2684, 2684, 2684, 2684, 2684, 2684, 2684, 2684, 2684, 2684, 2684, 2684,
    2684, 2684, 2684, 2684, 2684, 2684, 2684, 2684, 2684, 2684, 2684, 2684,
    2684, 2684, 2684, 2684, 2684, 2684, 2684, 2684, 2684, 2684, 2684, 2684,
    2684, 2684, 2684, 2684, 2684, 2685, 2685, 2685, 2685, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2685, 2685, 2686, 2683, 2683, 525,  2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 525,  525,  525,
    2686, 525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683,
    2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683,
    2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683,
    2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2687, 525,  525,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 525,  525,  525,  2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    2687, 2688, 2688, 2688, 2688, 2688, 2688, 2688, 2688, 2688, 2688, 2688,
    2688, 2688, 2688, 2688, 2688, 2688, 2688, 2688, 2688, 2688, 2688, 2688,
    2688, 2688, 2688, 2688, 525,  525,  525,  525,  2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2682, 2682, 2682, 2682, 525,  525,  525,  525,
    525,  525,  525,  525,  525,  2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2689, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2689, 525,
    525,  525,  525,  525,  2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2690, 2690, 2690, 2690, 2690, 525,
    525,  525,  525,  525,  2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 525,  2680,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    525,  525,  525,  525,  2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2680, 2689, 2689, 2689, 2689, 2689, 525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    2691, 2692, 2693, 2694, 2695, 2696, 2697, 2698, 2699, 2700, 2701, 2702,
    2703, 2704, 2705, 2706, 2707, 2708, 2709, 2710, 2711, 2712, 2713, 2714,
    2715, 2716, 2717, 2718, 2719, 2720, 2721, 2722, 2723, 2724, 2725, 2726,
    2727, 2728, 2729, 2730, 2731, 2732, 2733, 2734, 2735, 2736, 2737, 2738,
    2739, 2740, 2741, 2742, 2743, 2744, 2745, 2746, 2747, 2748, 2749, 2750,
    2751, 2752, 2753, 2754, 2755, 2756, 2757, 2758, 2759, 2760, 2761, 2762,
    2763, 2764, 2765, 2766, 2767, 2768, 2769, 2770, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 525,  525,  2771, 2771, 2771, 2771, 2771, 2771, 2771, 2771,
    2771, 2771, 525,  525,  525,  525,  525,  525,  2772, 2773, 2774, 2775,
    2776, 2777, 2778, 2779, 2780, 2781, 2782, 2783, 2784, 2785, 2786, 2787,
    2788, 2789, 2790, 2791, 2792, 2793, 2794, 2795, 2796, 2797, 2798, 2799,
    2800, 2801, 2802, 2803, 2804, 2805, 2806, 2807, 525,  525,  525,  525,
    2808, 2809, 2810, 2811, 2812, 2813, 2814, 2815, 2816, 2817, 2818, 2819,
    2820, 2821, 2822, 2823, 2824, 2825, 2826, 2827, 2828, 2829, 2830, 2831,
    2832, 2833, 2834, 2835, 2836, 2837, 2838, 2839, 2840, 2841, 2842, 2843,
    525,  525,  525,  525,  2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 525,  525,  525,  525,
    525,  525,  525,  525,  2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  2680, 2844, 2845, 2846, 2847,
    2848, 2849, 2850, 2851, 2852, 2853, 2854, 525,  2855, 2856, 2857, 2858,
    2859, 2860, 2861, 2862, 2863, 2864, 2865, 2866, 2867, 2868, 2869, 525,
    2870, 2871, 2872, 2873, 2874, 2875, 2876, 525,  2877, 2878, 525,  2879,
    2880, 2881, 2882, 2883, 2884, 2885, 2886, 2887, 2888, 2889, 525,  2890,
    2891, 2892, 2893, 2894, 2895, 2896, 2897, 2898, 2899, 2900, 2901, 2902,
    2903, 2904, 525,  2905, 2906, 2907, 2908, 2909, 2910, 2911, 525,  2912,
    2913, 525,  525,  525,  2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 525,
    525,  525,  525,  525,  525,  525,  525,  525,  2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    2914, 2915, 2915, 2915, 2915, 2915, 525,  2915, 2915, 2915, 2915, 2915,
    2915, 2915, 2915, 2915, 2915, 2915, 2915, 2915, 2915, 2915, 2915, 2915,
    2915, 2915, 2915, 2915, 2915, 2915, 2915, 2915, 2915, 2915, 2915, 2915,
    2915, 2915, 2915, 2915, 2915, 2915, 2915, 2915, 2915, 2915, 2915, 2915,
    2915, 525,  2915, 2915, 2915, 2915, 2915, 2915, 2915, 2915, 2915, 525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  2916, 2916, 2916, 2916,
    2916, 2916, 525,  525,  2916, 525,  2916, 2916, 2916, 2916, 2916, 2916,
    2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916,
    2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916,
    2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916,
    2916, 2916, 525,  2916, 2916, 525,  525,  525,  2916, 525,  525,  2916,
    2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916,
    2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 525,  2917,
    2918, 2918, 2918, 2918, 2918, 2918, 2918, 2918, 2916, 2916, 2916, 2916,
    2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916,
    2916, 2916, 2916, 2916, 2916, 2916, 2916, 2919, 2919, 2918, 2918, 2918,
    2918, 2918, 2918, 2918, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916,
    2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916,
    2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 525,
    525,  525,  525,  525,  525,  525,  525,  2918, 2918, 2918, 2918, 2918,
    2918, 2918, 2918, 2918, 525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916,
    2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 525,
    2916, 2916, 525,  525,  525,  525,  525,  2918, 2918, 2918, 2918, 2918,
    2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916,
    2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2918, 2918,
    2918, 2918, 2918, 2918, 525,  525,  525,  2681, 2916, 2916, 2916, 2916,
    2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916,
    2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 525,  525,
    525,  525,  525,  2917, 525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  2916, 2916, 2916, 2916,
    2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916,
    2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916,
    2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916,
    2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916,
    2916, 2916, 2916, 2916, 525,  525,  525,  525,  2918, 2918, 2916, 2916,
    2918, 2918, 2918, 2918, 2918, 2918, 2918, 2918, 2918, 2918, 2918, 2918,
    2918, 2918, 2918, 2918, 525,  525,  2918, 2918, 2918, 2918, 2918, 2918,
    2918, 2918, 2918, 2918, 2918, 2918, 2918, 2918, 2918, 2918, 2918, 2918,
    2918, 2918, 2918, 2918, 2918, 2918, 2918, 2918, 2918, 2918, 2918, 2918,
    2918, 2918, 2918, 2918, 2918, 2918, 2918, 2918, 2918, 2918, 2918, 2918,
    2918, 2918, 2918, 2918, 2916, 2920, 2920, 2920, 525,  2920, 2920, 525,
    525,  525,  525,  525,  2920, 2687, 2920, 2690, 2916, 2916, 2916, 2916,
    525,  2916, 2916, 2916, 525,  2916, 2916, 2916, 2916, 2916, 2916, 2916,
    2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916,
    2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 525,  525,
    2690, 2921, 2687, 525,  525,  525,  525,  2922, 2918, 2918, 2918, 2918,
    2918, 2918, 2918, 2918, 2918, 525,  525,  525,  525,  525,  525,  525,
    2917, 2917, 2917, 2917, 2917, 2917, 2917, 2917, 2917, 525,  525,  525,
    525,  525,  525,  525,  2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916,
    2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916,
    2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2918, 2918, 2917,
    2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916,
    2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916,
    2916, 2916, 2916, 2916, 2916, 2918, 2918, 2918, 525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916,
    2919, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916,
    2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916,
    2916, 2916, 2916, 2916, 2916, 2690, 2687, 525,  525,  525,  525,  2918,
    2918, 2918, 2918, 2918, 2917, 2917, 2917, 2917, 2917, 2917, 2917, 525,
    525,  525,  525,  525,  525,  525,  525,  525,  2916, 2916, 2916, 2916,
    2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916,
    2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916,
    2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916,
    2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916,
    2916, 2916, 525,  525,  525,  2681, 2681, 2681, 2681, 2681, 2681, 2681,
    2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916,
    2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 525,  525,
    2918, 2918, 2918, 2918, 2918, 2918, 2918, 2918, 2916, 2916, 2916, 2916,
    2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916,
    2916, 2916, 2916, 525,  525,  525,  525,  525,  2918, 2918, 2918, 2918,
    2918, 2918, 2918, 2918, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916,
    2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 525,  525,
    525,  525,  525,  525,  525,  2917, 2917, 2917, 2917, 525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  2918, 2918, 2918,
    2918, 2918, 2918, 2918, 525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916,
    2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916,
    2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916,
    2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916,
    2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916,
    2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916,
    2916, 525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  2923, 2924, 2925, 2926,
    2927, 2928, 2929, 2930, 2931, 2932, 2933, 2934, 2935, 2936, 2937, 2938,
    2939, 2940, 2941, 2942, 2943, 2944, 2945, 2946, 2947, 2948, 2949, 2950,
    2951, 2952, 2953, 2954, 2955, 2956, 2957, 2958, 2959, 2960, 2961, 2962,
    2963, 2964, 2965, 2966, 2967, 2968, 2969, 2970, 2971, 2972, 2973, 525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    2974, 2975, 2976, 2977, 2978, 2979, 2980, 2981, 2982, 2983, 2984, 2985,
    2986, 2987, 2988, 2989, 2990, 2991, 2992, 2993, 2994, 2995, 2996, 2997,
    2998, 2999, 3000, 3001, 3002, 3003, 3004, 3005, 3006, 3007, 3008, 3009,
    3010, 3011, 3012, 3013, 3014, 3015, 3016, 3017, 3018, 3019, 3020, 3021,
    3022, 3023, 3024, 525,  525,  525,  525,  525,  525,  525,  2918, 2918,
    2918, 2918, 2918, 2918, 3025, 3025, 3025, 3025, 3025, 3025, 3025, 3025,
    3025, 3025, 3025, 3025, 3025, 3025, 3025, 3025, 3025, 3025, 3025, 3025,
    3025, 3025, 3025, 3025, 3025, 3025, 3025, 3025, 3025, 3025, 3025, 3025,
    3025, 3025, 3025, 3025, 2690, 2690, 2690, 2690, 525,  525,  525,  525,
    525,  525,  525,  525,  3026, 3026, 3026, 3026, 3026, 3026, 3026, 3026,
    3026, 3026, 525,  525,  525,  525,  525,  525,  3026, 3026, 3026, 3026,
    3026, 3026, 3026, 3026, 3026, 3026, 2916, 2916, 2916, 2916, 3027, 2916,
    3028, 3029, 3030, 3031, 3032, 3033, 3034, 3035, 3036, 3037, 3038, 3039,
    3040, 3041, 3042, 3043, 3044, 3045, 3046, 3047, 3048, 3049, 525,  525,
    525,  2690, 2690, 2690, 2690, 2690, 3050, 3027, 3051, 3052, 3053, 3054,
    3055, 3056, 3057, 3058, 3059, 3060, 3061, 3062, 3063, 3064, 3065, 3066,
    3067, 3068, 3069, 3070, 3071, 3072, 525,  525,  525,  525,  525,  525,
    525,  525,  3073, 3073, 525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  3074, 3074, 3074, 3074,
    3074, 3074, 3074, 3074, 3074, 3074, 3074, 3074, 3074, 3074, 3074, 3074,
    3074, 3074, 3074, 3074, 3074, 3074, 3074, 3074, 3074, 3074, 3074, 3074,
    3074, 3074, 3074, 525,  2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916,
    2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916,
    2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916,
    2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 525,  2690,
    2690, 3075, 525,  525,  2916, 2916, 525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  3025, 3025,
    3025, 525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  2920, 2687, 2687, 2687,
    2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916,
    2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916,
    2916, 2916, 2916, 2916, 2916, 2918, 2918, 2918, 2918, 2918, 2918, 2918,
    2918, 2918, 2918, 2916, 525,  525,  525,  525,  525,  525,  525,  525,
    3025, 3025, 3025, 3025, 3025, 3025, 3025, 3025, 3025, 3025, 3025, 3025,
    3025, 3025, 3025, 3025, 3025, 3025, 3025, 3025, 3025, 3025, 2687, 2687,
    2690, 2690, 2690, 2687, 2690, 2687, 2687, 2687, 2687, 3076, 3076, 3076,
    3076, 3077, 3077, 3077, 3077, 3077, 525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916,
    2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2690, 2687,
    2690, 2687, 2917, 2917, 2917, 2917, 525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  2916, 2916, 2916, 2916,
    2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916,
    2916, 2916, 2916, 2916, 2916, 2918, 2918, 2918, 2918, 2918, 2918, 2918,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  2916, 2916, 2916, 2916,
    2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916,
    2916, 2916, 2916, 2916, 2916, 2916, 2916, 525,  525,  525,  525,  525,
    525,  525,  525,  525,  3078, 2920, 3078, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920,
    2920, 2920, 2922, 2680, 2680, 2680, 2680, 2680, 2680, 2680, 525,  525,
    525,  525,  2685, 2685, 2685, 2685, 2685, 2685, 2685, 2685, 2685, 2685,
    2685, 2685, 2685, 2685, 2685, 2685, 2685, 2685, 2685, 2685, 2771, 2771,
    2771, 2771, 2771, 2771, 2771, 2771, 2771, 2771, 2922, 2679, 2679, 2920,
    2920, 2679, 525,  525,  525,  525,  525,  525,  525,  525,  525,  2922,
    2920, 2920, 3078, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    3078, 3078, 3078, 2920, 2920, 2920, 2920, 3078, 3078, 2922, 3079, 2680,
    2680, 3080, 2680, 2680, 2680, 2680, 2920, 525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  3080, 525,  525,  2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 525,  525,  525,
    525,  525,  525,  525,  2771, 2771, 2771, 2771, 2771, 2771, 2771, 2771,
    2771, 2771, 525,  525,  525,  525,  525,  525,  2690, 2690, 2690, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2920,
    2920, 2920, 2920, 2920, 3078, 2920, 2920, 2920, 2920, 2920, 2920, 2922,
    2922, 525,  2771, 2771, 2771, 2771, 2771, 2771, 2771, 2771, 2771, 2771,
    2680, 2680, 2680, 2680, 2679, 3078, 3078, 2679, 525,  525,  525,  525,
    525,  525,  525,  525,  2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 3079, 2680, 2680, 2679, 525,  525,  525,  525,  525,
    525,  525,  525,  525,  2920, 2920, 3078, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 3078, 3078, 3078, 2920, 2920,
    2920, 2920, 2920, 2920, 2920, 2920, 2920, 3078, 3081, 2679, 2679, 2679,
    2679, 2680, 2680, 2680, 2680, 2920, 3079, 2920, 2920, 2680, 3078, 2920,
    2771, 2771, 2771, 2771, 2771, 2771, 2771, 2771, 2771, 2771, 2679, 2680,
    2679, 2680, 2680, 2680, 525,  2682, 2682, 2682, 2682, 2682, 2682, 2682,
    2682, 2682, 2682, 2682, 2682, 2682, 2682, 2682, 2682, 2682, 2682, 2682,
    2682, 525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 525,  2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 3078, 3078, 3078, 2920,
    2920, 2920, 3078, 3078, 2920, 3081, 3079, 2920, 2680, 2680, 2680, 2680,
    2680, 2680, 2920, 2679, 2679, 2920, 525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  2679, 2679, 2679, 2679,
    2679, 2679, 2679, 525,  2679, 525,  2679, 2679, 2679, 2679, 525,  2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 525,  2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2680, 525,  525,  525,  525,  525,  525,  2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2920, 3078, 3078, 3078, 2920,
    2920, 2920, 2920, 2920, 2920, 3079, 2922, 525,  525,  525,  525,  525,
    2771, 2771, 2771, 2771, 2771, 2771, 2771, 2771, 2771, 2771, 525,  525,
    525,  525,  525,  525,  2920, 2920, 3078, 3078, 525,  2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 525,  525,  2679, 2679, 525,  525,  2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 525,  2679, 2679,
    2679, 2679, 2679, 2679, 2679, 525,  2679, 2679, 525,  2679, 2679, 2679,
    2679, 2679, 525,  3079, 3079, 2679, 3078, 3078, 2920, 3078, 3078, 3078,
    3078, 525,  525,  3078, 3078, 525,  525,  3078, 3078, 3081, 525,  525,
    2679, 525,  525,  525,  525,  525,  525,  3078, 525,  525,  525,  525,
    525,  2679, 2679, 2679, 2679, 2679, 3078, 3078, 525,  525,  2690, 2690,
    2690, 2690, 2690, 2690, 2690, 525,  525,  525,  2690, 2690, 2690, 2690,
    2690, 525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 525,  2679,
    525,  525,  2679, 525,  2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 525,  2679, 3078, 3078, 3078, 2920,
    2920, 2920, 2920, 2920, 2920, 525,  3078, 525,  525,  3078, 525,  3078,
    3078, 3078, 3078, 525,  3078, 3078, 2922, 3081, 2922, 2679, 2920, 2679,
    2680, 2680, 525,  2680, 2680, 525,  525,  525,  525,  525,  525,  525,
    525,  2920, 2920, 525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 3078, 3078, 3078, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920,
    3078, 3078, 2922, 2920, 2920, 3078, 3079, 2679, 2679, 2679, 2679, 2680,
    2680, 2680, 2680, 2680, 2771, 2771, 2771, 2771, 2771, 2771, 2771, 2771,
    2771, 2771, 2680, 2680, 525,  2680, 2690, 2679, 2679, 2679, 525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 3078, 3078, 3078, 2920, 2920, 2920, 2920, 2920,
    2920, 3078, 2920, 3078, 3078, 3078, 3078, 2920, 2920, 3078, 2922, 3079,
    2679, 2679, 2680, 2679, 525,  525,  525,  525,  525,  525,  525,  525,
    2771, 2771, 2771, 2771, 2771, 2771, 2771, 2771, 2771, 2771, 525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 3078,
    3078, 3078, 2920, 2920, 2920, 2920, 525,  525,  3078, 3078, 3078, 3078,
    2920, 2920, 3078, 2922, 3079, 2680, 2680, 2680, 2680, 2680, 2680, 2680,
    2680, 2680, 2680, 2680, 2680, 2680, 2680, 2680, 2680, 2680, 2680, 2680,
    2680, 2680, 2680, 2680, 2679, 2679, 2679, 2679, 2920, 2920, 525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 3078, 3078, 3078, 2920,
    2920, 2920, 2920, 2920, 2920, 2920, 2920, 3078, 3078, 2920, 3078, 2922,
    2920, 2680, 2680, 2680, 2679, 525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  2771, 2771, 2771, 2771, 2771, 2771, 2771, 2771,
    2771, 2771, 525,  525,  525,  525,  525,  525,  2681, 2681, 2681, 2681,
    2681, 2681, 2681, 2681, 2681, 2681, 2681, 2681, 2681, 525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2920,
    3078, 2920, 3078, 3078, 2920, 2920, 2920, 2920, 2920, 2920, 3081, 3079,
    2679, 2680, 525,  525,  525,  525,  525,  525,  2771, 2771, 2771, 2771,
    2771, 2771, 2771, 2771, 2771, 2771, 525,  525,  525,  525,  525,  525,
    2771, 2771, 2771, 2771, 2771, 2771, 2771, 2771, 2771, 2771, 2771, 2771,
    2771, 2771, 2771, 2771, 2771, 2771, 2771, 2771, 525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 525,  525,  2920, 3078, 2920, 3078, 3078, 2920, 2920,
    2920, 2920, 3078, 2920, 2920, 2920, 2920, 2922, 525,  525,  525,  525,
    2771, 2771, 2771, 2771, 2771, 2771, 2771, 2771, 2771, 2771, 2682, 2682,
    2680, 2680, 2680, 2683, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 3078, 3078, 3078, 2920, 2920, 2920, 2920, 2920,
    2920, 2920, 2920, 2920, 3078, 2922, 3079, 2680, 525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    3082, 3083, 3084, 3085, 3086, 3087, 3088, 3089, 3090, 3091, 3092, 3093,
    3094, 3095, 3096, 3097, 3098, 3099, 3100, 3101, 3102, 3103, 3104, 3105,
    3106, 3107, 3108, 3109, 3110, 3111, 3112, 3113, 3114, 3115, 3116, 3117,
    3118, 3119, 3120, 3121, 3122, 3123, 3124, 3125, 3126, 3127, 3128, 3129,
    3130, 3131, 3132, 3133, 3134, 3135, 3136, 3137, 3138, 3139, 3140, 3141,
    3142, 3143, 3144, 3145, 2771, 2771, 2771, 2771, 2771, 2771, 2771, 2771,
    2771, 2771, 2682, 2682, 2682, 2682, 2682, 2682, 2682, 2682, 2682, 525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 525,  525,  2679, 525,  525,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 525,  2679, 2679, 525,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    3078, 3078, 3078, 3078, 3078, 3078, 525,  3078, 3078, 525,  525,  2920,
    2920, 3081, 2922, 2679, 3078, 2679, 3078, 3079, 2680, 2680, 2680, 525,
    525,  525,  525,  525,  525,  525,  525,  525,  2771, 2771, 2771, 2771,
    2771, 2771, 2771, 2771, 2771, 2771, 525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    525,  525,  2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 3078, 3078, 3078, 2920, 2920, 2920, 2920,
    525,  525,  2920, 2920, 3078, 3078, 3078, 3078, 2922, 2679, 2680, 2679,
    3078, 525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  2679, 2920, 2920, 2920, 2920, 2920, 2920, 3146,
    3146, 2920, 2920, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2920, 2922, 2920, 2920, 2920,
    2920, 3078, 2679, 2920, 2920, 2920, 2920, 2680, 2680, 2680, 2680, 2680,
    2680, 2680, 2680, 2922, 525,  525,  525,  525,  525,  525,  525,  525,
    2679, 2920, 2920, 2920, 2920, 2920, 2920, 3078, 3078, 2920, 2920, 2920,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2920, 2920,
    2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 3078,
    2920, 2922, 2680, 2680, 2680, 2679, 2680, 2680, 2680, 2680, 2680, 525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 525,  525,  525,  525,  525,  525,  525,  2680, 2680, 2680, 2680,
    2680, 2680, 2680, 2680, 2680, 2680, 525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2680, 525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  2771, 2771, 2771, 2771,
    2771, 2771, 2771, 2771, 2771, 2771, 525,  525,  525,  525,  525,  525,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 525,  2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 3078,
    2920, 2920, 2920, 2920, 2920, 2920, 2920, 525,  2920, 2920, 2920, 2920,
    2920, 2920, 3078, 3147, 2679, 2680, 2680, 2680, 2680, 2680, 525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  2771, 2771, 2771, 2771,
    2771, 2771, 2771, 2771, 2771, 2771, 2682, 2682, 2682, 2682, 2682, 2682,
    2682, 2682, 2682, 2682, 2682, 2682, 2682, 2682, 2682, 2682, 2682, 2682,
    2682, 525,  525,  525,  2680, 2680, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    525,  525,  2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920,
    2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920,
    525,  3078, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 3078, 2920, 2920,
    3078, 2920, 2920, 525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  2679, 2679, 2679, 2679, 2679, 2679, 2679, 525,
    2679, 2679, 525,  2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2920, 2920, 2920, 2920, 2920, 2920, 525,
    525,  525,  2920, 525,  2920, 2920, 525,  2920, 2920, 2920, 3079, 2920,
    2922, 2922, 2679, 2920, 525,  525,  525,  525,  525,  525,  525,  525,
    2771, 2771, 2771, 2771, 2771, 2771, 2771, 2771, 2771, 2771, 525,  525,
    525,  525,  525,  525,  2679, 2679, 2679, 2679, 2679, 2679, 525,  2679,
    2679, 525,  2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 3078, 3078,
    3078, 3078, 3078, 525,  2920, 2920, 525,  3078, 3078, 2920, 3078, 2922,
    2679, 525,  525,  525,  525,  525,  525,  525,  2771, 2771, 2771, 2771,
    2771, 2771, 2771, 2771, 2771, 2771, 525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2920, 2920, 3078, 3078, 2680, 2680, 525,  525,  525,
    525,  525,  525,  525,  2920, 2920, 2679, 3078, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 525,  2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 3078, 3078, 2920, 2920,
    2920, 2920, 2920, 525,  525,  525,  3078, 3078, 2920, 3081, 2922, 2680,
    2680, 2680, 2680, 2680, 2680, 2680, 2680, 2680, 2680, 2680, 2680, 2680,
    2771, 2771, 2771, 2771, 2771, 2771, 2771, 2771, 2771, 2771, 2920, 525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    2679, 525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  2682, 2682, 2682, 2682, 2682, 2682, 2682, 2682,
    2682, 2682, 2682, 2682, 2682, 2682, 2682, 2682, 2682, 2682, 2682, 2682,
    2682, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 3148, 3148, 3148,
    3148, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  2680, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689,
    2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689,
    2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689,
    2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689,
    2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689,
    2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689,
    2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689,
    2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689,
    2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689,
    2689, 2689, 2689, 2689, 2689, 2689, 2689, 525,  2680, 2680, 2680, 2680,
    2680, 525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2680, 2680, 525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 3080, 3080, 3080, 3080, 3080, 3080, 3080, 3080,
    3080, 3080, 3080, 3080, 3080, 3080, 3080, 3080, 2920, 2679, 2679, 2679,
    2679, 2679, 2679, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920,
    2920, 2920, 2920, 2920, 2920, 2920, 525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920,
    2920, 2920, 3078, 3078, 3078, 2920, 2920, 2922, 2771, 2771, 2771, 2771,
    2771, 2771, 2771, 2771, 2771, 2771, 525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 525,  525,  525,  525,  525,  525,  525,  2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 525,  2771, 2771, 2771, 2771, 2771, 2771, 2771, 2771,
    2771, 2771, 525,  525,  525,  525,  2680, 2680, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 525,  2771, 2771, 2771, 2771, 2771, 2771, 2771, 2771,
    2771, 2771, 525,  525,  525,  525,  525,  525,  2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 525,  525,  2921, 2921, 2921, 2921, 2921, 2680, 525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2690, 2690, 2690, 2690,
    2690, 2690, 2690, 2680, 2680, 2680, 2680, 2680, 2683, 2683, 2683, 2683,
    2914, 2914, 2914, 2914, 2680, 2683, 525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  2771, 2771, 2771, 2771, 2771, 2771, 2771, 2771,
    2771, 2771, 525,  2682, 2682, 2682, 2682, 2682, 2682, 2682, 525,  2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 525,  525,  525,  525,
    525,  2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  2914, 2914, 2914, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2914,
    2914, 2680, 2680, 2680, 2771, 2771, 2771, 2771, 2771, 2771, 2771, 2771,
    2771, 2771, 525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    3149, 3150, 3151, 3152, 3153, 3154, 3155, 3156, 3157, 3158, 3159, 3160,
    3161, 3162, 3163, 3164, 3165, 3166, 3167, 3168, 3169, 3170, 3171, 3172,
    3173, 3174, 3175, 3176, 3177, 3178, 3179, 3180, 3181, 3182, 3183, 3184,
    3185, 3186, 3187, 3188, 3189, 3190, 3191, 3192, 3193, 3194, 3195, 3196,
    3197, 3198, 3199, 3200, 3201, 3202, 3203, 3204, 3205, 3206, 3207, 3208,
    3209, 3210, 3211, 3212, 2682, 2682, 2682, 2682, 2682, 2682, 2682, 2682,
    2682, 2682, 2682, 2682, 2682, 2682, 2682, 2682, 2682, 2682, 2682, 2682,
    2682, 2682, 2682, 2680, 2680, 2680, 2680, 525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 525,  525,  525,  525,  2920, 2679, 3078, 3078, 3078,
    3078, 3078, 3078, 3078, 3078, 3078, 3078, 3078, 3078, 3078, 3078, 3078,
    3078, 3078, 3078, 3078, 3078, 3078, 3078, 3078, 3078, 3078, 3078, 3078,
    3078, 3078, 3078, 3078, 3078, 3078, 3078, 3078, 3078, 3078, 3078, 3078,
    3078, 3078, 3078, 3078, 3078, 3078, 3078, 3078, 3078, 3078, 3078, 3078,
    3078, 3078, 3078, 3078, 525,  525,  525,  525,  525,  525,  525,  2920,
    2920, 2920, 2920, 2914, 2914, 2914, 2914, 2914, 2914, 2914, 2914, 2914,
    2914, 2914, 2914, 2914, 525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  3213, 3213, 3214, 3213,
    3215, 525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    3216, 3216, 525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  3217, 525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  3217,
    525,  525,  525,  525,  525,  525,  525,  525,  3217, 3217, 3217, 3217,
    3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217,
    3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217,
    3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217,
    3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217,
    3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217,
    3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217,
    3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217,
    3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217,
    3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217,
    3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217,
    3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217,
    3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217,
    3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217,
    3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217,
    3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217,
    3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217,
    3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217,
    3217, 3217, 3217, 3217, 3217, 3217, 525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  3217,
    3217, 525,  525,  525,  525,  525,  525,  525,  3217, 525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    3213, 3213, 3213, 3213, 525,  3213, 3213, 3213, 3213, 3213, 3213, 3213,
    525,  3213, 3213, 525,  3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217,
    3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217,
    3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217,
    3217, 3217, 3217, 525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  3217, 525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    3217, 3217, 3217, 525,  525,  3217, 525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  3217, 3217, 3217, 3217,
    525,  525,  525,  525,  525,  525,  525,  525,  3217, 3217, 3217, 3217,
    3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217,
    3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217,
    3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217,
    3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217,
    3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217,
    3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217,
    3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217,
    3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217,
    3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217,
    3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217,
    3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217,
    3217, 3217, 3217, 3217, 525,  525,  525,  525,  2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 525,  525,  525,  525,  525,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 525,  525,  525,  2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 525,  525,  525,  525,  525,  525,  525,  2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 525,  525,  2683, 2920, 2921, 2680,
    3218, 3218, 3218, 3218, 525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 3219, 3219,
    3219, 3219, 3219, 3219, 3219, 3219, 3219, 3219, 3219, 3219, 3219, 3219,
    3219, 3219, 3219, 3219, 3219, 3219, 3219, 3219, 3219, 3219, 3219, 3219,
    3220, 3220, 3220, 3220, 3220, 3220, 3220, 3220, 3220, 3220, 525,  525,
    525,  525,  525,  525,  2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920,
    2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920,
    2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920,
    2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 525,  525,
    2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920,
    2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 525,
    525,  525,  525,  525,  525,  525,  525,  525,  2683, 2683, 2683, 2683,
    2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683,
    2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683,
    2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683,
    2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683,
    2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683,
    2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683,
    2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683,
    2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683,
    2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683,
    2683, 2683, 2683, 2683, 525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683,
    2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683,
    2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683,
    2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683,
    2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683,
    2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683,
    2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683,
    2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683,
    2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683,
    2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683,
    2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683,
    2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683,
    2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683,
    2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683,
    2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683,
    2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683,
    2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683,
    2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683,
    2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683,
    2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683,
    2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  2683, 2683, 2683, 2683,
    2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683,
    2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683,
    2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 525,
    525,  2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683,
    2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683,
    2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683,
    2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683,
    2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683,
    2683, 3221, 3221, 2921, 2921, 2921, 2683, 2683, 2683, 3222, 3221, 3221,
    3221, 3221, 3221, 3218, 3218, 3218, 3218, 3218, 3218, 3218, 3218, 2687,
    2687, 2687, 2687, 2687, 2687, 2687, 2687, 2683, 2683, 2690, 2690, 2690,
    2690, 2690, 2687, 2687, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683,
    2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683,
    2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2690, 2690,
    2690, 2690, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683,
    2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683,
    2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683,
    2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683,
    2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683,
    2683, 2686, 2686, 525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2690, 2690, 2690, 2686, 525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    2682, 2682, 2682, 2682, 2682, 2682, 2682, 2682, 2682, 2682, 2682, 2682,
    2682, 2682, 2682, 2682, 2682, 2682, 2682, 2682, 525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  2682, 2682, 2682, 2682,
    2682, 2682, 2682, 2682, 2682, 2682, 2682, 2682, 2682, 2682, 2682, 2682,
    2682, 2682, 2682, 2682, 525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 525,  525,  525,  525,  525,
    525,  525,  525,  525,  3224, 3224, 3224, 3224, 3224, 3224, 3224, 3224,
    3224, 3224, 3224, 3224, 3224, 3224, 3224, 3224, 3224, 3224, 3224, 3224,
    3224, 3224, 3224, 2682, 2682, 525,  525,  525,  525,  525,  525,  525,
    3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225,
    3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225,
    3225, 3225, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226,
    3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226,
    3226, 3226, 3226, 3226, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225,
    3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225,
    3225, 3225, 3225, 3225, 3225, 3225, 3226, 3226, 3226, 3226, 3226, 3226,
    3226, 525,  3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226,
    3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3225, 3225, 3225, 3225,
    3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225,
    3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3226, 3226,
    3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226,
    3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226,
    3225, 525,  3225, 3225, 525,  525,  3225, 525,  525,  3225, 3225, 525,
    525,  3225, 3225, 3225, 3225, 525,  3225, 3225, 3225, 3225, 3225, 3225,
    3225, 3225, 3226, 3226, 3226, 3226, 525,  3226, 525,  3226, 3226, 3226,
    3226, 3226, 3226, 3226, 525,  3226, 3226, 3226, 3226, 3226, 3226, 3226,
    3226, 3226, 3226, 3226, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225,
    3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225,
    3225, 3225, 3225, 3225, 3225, 3225, 3226, 3226, 3226, 3226, 3226, 3226,
    3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226,
    3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3225, 3225, 525,  3225,
    3225, 3225, 3225, 525,  525,  3225, 3225, 3225, 3225, 3225, 3225, 3225,
    3225, 525,  3225, 3225, 3225, 3225, 3225, 3225, 3225, 525,  3226, 3226,
    3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226,
    3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226,
    3225, 3225, 525,  3225, 3225, 3225, 3225, 525,  3225, 3225, 3225, 3225,
    3225, 525,  3225, 525,  525,  525,  3225, 3225, 3225, 3225, 3225, 3225,
    3225, 525,  3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226,
    3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226,
    3226, 3226, 3226, 3226, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225,
    3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225,
    3225, 3225, 3225, 3225, 3225, 3225, 3226, 3226, 3226, 3226, 3226, 3226,
    3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226,
    3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3225, 3225, 3225, 3225,
    3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225,
    3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3226, 3226,
    3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226,
    3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226,
    3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225,
    3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225,
    3225, 3225, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226,
    3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226,
    3226, 3226, 3226, 3226, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225,
    3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225,
    3225, 3225, 3225, 3225, 3225, 3225, 3226, 3226, 3226, 3226, 3226, 3226,
    3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226,
    3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3225, 3225, 3225, 3225,
    3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225,
    3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3226, 3226,
    3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226,
    3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226,
    3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225,
    3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225,
    3225, 3225, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226,
    3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226,
    3226, 3226, 3226, 3226, 3226, 3226, 525,  525,  3225, 3225, 3225, 3225,
    3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225,
    3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3227, 3226, 3226,
    3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226,
    3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3228,
    3226, 3226, 3226, 3226, 3226, 3226, 3225, 3225, 3225, 3225, 3225, 3225,
    3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225,
    3225, 3225, 3225, 3225, 3225, 3225, 3225, 3227, 3226, 3226, 3226, 3226,
    3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226,
    3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3228, 3226, 3226,
    3226, 3226, 3226, 3226, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225,
    3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225,
    3225, 3225, 3225, 3225, 3225, 3227, 3226, 3226, 3226, 3226, 3226, 3226,
    3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226,
    3226, 3226, 3226, 3226, 3226, 3226, 3226, 3228, 3226, 3226, 3226, 3226,
    3226, 3226, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225,
    3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225,
    3225, 3225, 3225, 3227, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226,
    3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226,
    3226, 3226, 3226, 3226, 3226, 3228, 3226, 3226, 3226, 3226, 3226, 3226,
    3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225,
    3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225, 3225,
    3225, 3227, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226,
    3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226, 3226,
    3226, 3226, 3226, 3228, 3226, 3226, 3226, 3226, 3226, 3226, 3225, 3226,
    525,  525,  3220, 3220, 3220, 3220, 3220, 3220, 3220, 3220, 3220, 3220,
    3220, 3220, 3220, 3220, 3220, 3220, 3220, 3220, 3220, 3220, 3220, 3220,
    3220, 3220, 3220, 3220, 3220, 3220, 3220, 3220, 3220, 3220, 3220, 3220,
    3220, 3220, 3220, 3220, 3220, 3220, 3220, 3220, 3220, 3220, 3220, 3220,
    3220, 3220, 3220, 3220, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920,
    2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920,
    2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920,
    2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920,
    2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2683,
    2683, 2683, 2683, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920,
    2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920,
    2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920,
    2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920,
    2920, 2920, 2920, 2920, 2920, 2683, 2683, 2683, 2683, 2683, 2683, 2683,
    2683, 2920, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683,
    2683, 2683, 2683, 2683, 2920, 2683, 2683, 2680, 2680, 2680, 2680, 2680,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  2920, 2920, 2920, 2920, 2920, 525,  2920, 2920, 2920,
    2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  3229, 3229, 3229, 3229,
    3229, 3229, 3229, 3229, 3229, 3229, 2679, 3229, 3229, 3229, 3229, 3229,
    3229, 3229, 3229, 3229, 3229, 3229, 3229, 3229, 3229, 3229, 3229, 3229,
    3229, 3229, 3229, 525,  525,  525,  525,  525,  525,  3229, 3229, 3229,
    3229, 3229, 3229, 525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  2690, 2690, 2690, 2690, 2690, 2690, 2690, 525,
    2690, 2690, 2690, 2690, 2690, 2690, 2690, 2690, 2690, 2690, 2690, 2690,
    2690, 2690, 2690, 2690, 2690, 525,  525,  2690, 2690, 2690, 2690, 2690,
    2690, 2690, 525,  2690, 2690, 525,  2690, 2690, 2690, 2690, 2690, 525,
    525,  525,  525,  525,  2915, 2915, 2915, 2915, 2915, 2915, 2915, 2915,
    2915, 2915, 2915, 2915, 2915, 2915, 2915, 2915, 2915, 2915, 2915, 2915,
    2915, 2915, 2915, 2915, 2915, 2915, 2915, 2915, 2915, 2915, 2915, 2915,
    2915, 3230, 3230, 3230, 3230, 3230, 3230, 3230, 3230, 3230, 3230, 3230,
    3230, 3230, 3230, 3230, 3230, 3230, 3230, 3230, 3230, 3230, 3230, 3230,
    3230, 3230, 3230, 2915, 2915, 2915, 525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  2690, 525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 525,  525,  525,  2690, 2690, 2690, 2690,
    2690, 2690, 2690, 2914, 2914, 2914, 2914, 2914, 2914, 2914, 525,  525,
    2771, 2771, 2771, 2771, 2771, 2771, 2771, 2771, 2771, 2771, 525,  525,
    525,  525,  2679, 2683, 525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2690, 525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2690, 2690, 2690, 2690, 2771, 2771, 2771, 2771,
    2771, 2771, 2771, 2771, 2771, 2771, 525,  525,  525,  525,  525,  3148,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2914,
    3231, 3231, 2687, 2690, 2771, 2771, 2771, 2771, 2771, 2771, 2771, 2771,
    2771, 2771, 525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2690, 2687,
    2679, 2771, 2771, 2771, 2771, 2771, 2771, 2771, 2771, 2771, 2771, 525,
    525,  525,  525,  2680, 525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  2679, 2679, 2679, 2679, 2679, 2679, 2679, 525,
    2679, 2679, 2679, 2679, 525,  2679, 2679, 525,  2679, 2679, 2679, 2679,
    2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 2679, 525,
    2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916,
    2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916,
    2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916,
    2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916,
    2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916,
    2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916,
    2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916,
    2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916,
    2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916,
    2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916,
    2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916,
    2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916,
    2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916,
    2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916,
    2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916,
    2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916, 2916,
    2916, 2916, 2916, 2916, 2916, 525,  525,  2918, 2918, 2918, 2918, 2918,
    2918, 2918, 2918, 2918, 2687, 2687, 2687, 2687, 2687, 2687, 2687, 525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  3232, 3233, 3234, 3235, 3236, 3237, 3238, 3239,
    3240, 3241, 3242, 3243, 3244, 3245, 3246, 3247, 3248, 3249, 3250, 3251,
    3252, 3253, 3254, 3255, 3256, 3257, 3258, 3259, 3260, 3261, 3262, 3263,
    3264, 3265, 3266, 3267, 3268, 3269, 3270, 3271, 3272, 3273, 3274, 3275,
    3276, 3277, 3278, 3279, 3280, 3281, 3282, 3283, 3284, 3285, 3286, 3287,
    3288, 3289, 3290, 3291, 3292, 3293, 3294, 3295, 3296, 3297, 3298, 3299,
    2690, 2690, 2690, 2690, 2690, 2690, 3079, 3027, 525,  525,  525,  525,
    3300, 3300, 3300, 3300, 3300, 3300, 3300, 3300, 3300, 3300, 525,  525,
    525,  525,  2917, 2917, 525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  3076, 3076, 3076, 3076, 3076, 3076, 3076,
    3076, 3076, 3076, 3076, 3076, 3076, 3076, 3076, 3076, 3076, 3076, 3076,
    3076, 3076, 3076, 3076, 3076, 3076, 3076, 3076, 3076, 3076, 3076, 3076,
    3076, 3076, 3076, 3076, 3076, 3076, 3076, 3076, 3076, 3076, 3076, 3076,
    3076, 3076, 3076, 3076, 3076, 3076, 3076, 3076, 3076, 3076, 3076, 3076,
    3076, 3076, 3076, 3076, 3301, 3076, 3076, 3076, 3302, 3076, 3076, 3076,
    3076, 525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  3076, 3076, 3076, 3076, 3076, 3076, 3076,
    3076, 3076, 3076, 3076, 3076, 3076, 3076, 3076, 3076, 3076, 3076, 3076,
    3076, 3076, 3076, 3076, 3076, 3076, 3076, 3076, 3076, 3076, 3076, 3076,
    3076, 3076, 3076, 3076, 3076, 3076, 3076, 3076, 3076, 3076, 3076, 3076,
    3076, 3076, 3301, 3076, 3076, 3076, 3076, 3076, 3076, 3076, 3076, 3076,
    3076, 3076, 3076, 3076, 3076, 3076, 525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    3303, 3303, 3303, 3303, 525,  3303, 3303, 3303, 3303, 3303, 3303, 3303,
    3303, 3303, 3303, 3303, 3303, 3303, 3303, 3303, 3303, 3303, 3303, 3303,
    3303, 3303, 3303, 3303, 3303, 3303, 3303, 3303, 525,  3303, 3303, 525,
    3303, 525,  525,  3303, 525,  3303, 3303, 3303, 3303, 3303, 3303, 3303,
    3303, 3303, 3303, 525,  3303, 3303, 3303, 3303, 525,  3303, 525,  3303,
    525,  525,  525,  525,  525,  525,  3303, 525,  525,  525,  525,  3303,
    525,  3303, 525,  3303, 525,  3303, 3303, 3303, 525,  3303, 3303, 525,
    3303, 525,  525,  3303, 525,  3303, 525,  3303, 525,  3303, 525,  3303,
    525,  3303, 3303, 525,  3303, 525,  525,  3303, 3303, 3303, 3303, 525,
    3303, 3303, 3303, 3303, 3303, 3303, 3303, 525,  3303, 3303, 3303, 3303,
    525,  3303, 3303, 3303, 3303, 525,  3303, 525,  3303, 3303, 3303, 3303,
    3303, 3303, 3303, 3303, 3303, 3303, 525,  3303, 3303, 3303, 3303, 3303,
    3303, 3303, 3303, 3303, 3303, 3303, 3303, 3303, 3303, 3303, 3303, 3303,
    525,  525,  525,  525,  525,  3303, 3303, 3303, 525,  3303, 3303, 3303,
    3303, 3303, 525,  3303, 3303, 3303, 3303, 3303, 3303, 3303, 3303, 3303,
    3303, 3303, 3303, 3303, 3303, 3303, 3303, 3303, 525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    3304, 3304, 525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  2686, 2686, 2686, 2686, 3223, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    525,  525,  525,  525,  2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 525,
    525,  2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 525,  2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 3223, 525,  2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  3305, 3305, 3305, 3305,
    3305, 3305, 3305, 3305, 3305, 3305, 3305, 2685, 2685, 2686, 2686, 2686,
    3306, 3306, 3306, 3306, 3306, 3306, 3306, 3306, 3306, 3306, 3306, 3306,
    3306, 3306, 3306, 3306, 3306, 3306, 3306, 3306, 3306, 3306, 3306, 3306,
    3306, 3306, 3306, 3307, 3307, 3307, 3307, 2686, 3308, 3308, 3308, 3308,
    3308, 3308, 3308, 3308, 3308, 3308, 3308, 3308, 3308, 3308, 3308, 3308,
    3308, 3308, 3308, 3308, 3308, 3308, 3308, 3308, 3308, 3308, 3308, 3308,
    3308, 3308, 3308, 3308, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683,
    2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683,
    2683, 2683, 2683, 2683, 2683, 2683, 3309, 3309, 3309, 2686, 2686, 2686,
    2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683,
    2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683,
    2683, 2683, 2683, 2683, 2683, 2683, 3310, 2683, 3308, 3310, 3310, 3310,
    3310, 3310, 3310, 3310, 3310, 3310, 3310, 2683, 2683, 2683, 2683, 2683,
    2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683,
    2683, 2686, 525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  2683, 2683,
    2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683,
    2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683, 2683,
    3311, 3311, 3311, 525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  3311, 3311, 3311, 3311, 3311, 3311, 3311, 3311,
    3311, 3311, 3311, 3311, 3311, 3311, 3311, 3311, 3311, 3311, 3311, 3311,
    3311, 3311, 3311, 3311, 3311, 3311, 3311, 3311, 3311, 3311, 3311, 3311,
    3311, 3311, 3311, 3311, 3311, 3311, 3311, 3311, 3311, 3311, 3311, 3311,
    525,  525,  525,  525,  3312, 3312, 3312, 3312, 3312, 3312, 3312, 3312,
    3312, 525,  525,  525,  525,  525,  525,  525,  3313, 3313, 525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    3223, 3223, 3223, 3223, 3223, 3223, 525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 2686, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 2686, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 2686, 2686, 2686, 2686, 3223,
    3223, 3223, 3223, 3223, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 2686, 2686, 2686,
    3223, 2686, 2686, 2686, 3223, 3223, 3223, 3314, 3314, 3314, 3314, 3314,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 2686, 3223, 2686, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 2686, 2686, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 3223, 3223, 3223, 3223, 2686,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 3223, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 3223, 3223, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    3223, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 2686, 2686, 2686, 2686, 2686, 2686, 3223, 2686, 2686, 2686,
    3223, 3223, 3223, 2686, 2686, 3223, 3223, 3223, 525,  525,  525,  525,
    3223, 3223, 3223, 3223, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 3223, 3223, 525,  525,  525,  2686, 2686, 2686, 2686,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 525,  525,  525,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 525,
    525,  525,  525,  2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 525,  525,  525,  525,  525,  525,  3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 525,  525,  525,  525,
    3223, 525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 525,  525,  525,  525,  2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 525,  525,  525,  525,  525,  525,  525,  525,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 525,  525,
    525,  525,  525,  525,  2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 525,  525,  525,  525,
    525,  525,  525,  525,  2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 525,  525,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    525,  525,  525,  525,  2686, 2686, 525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 2686, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 2686, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 525,  525,  3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 525,  525,  525,  3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 525,  525,  525,  525,  525,  3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 525,  525,  525,  525,  525,
    525,  525,  3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 525,  525,  3223, 3223, 3223, 3223, 3223,
    3223, 3223, 3223, 3223, 3223, 3223, 525,  525,  525,  525,  525,  525,
    3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 3223, 525,  525,  525,
    525,  525,  525,  525,  2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 525,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686,
    2686, 2686, 2686, 2686, 2686, 2686, 2686, 2686, 3220, 3220, 3220, 3220,
    3220, 3220, 3220, 3220, 3220, 3220, 525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  3217,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  3217, 525,  525,  525,  525,  525,  525,
    3217, 525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  3217, 525,  525,
    3217, 525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  3217, 525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    3217, 525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  3217, 525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    3217, 525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  3217, 525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217,
    3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217, 3217,
    3217, 3217, 3217, 3217, 3217, 3217, 525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  3217, 525,
    525,  525,  525,  525,  3217, 525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  3217, 525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  3218, 525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  3218, 3218, 3218, 3218,
    3218, 3218, 3218, 3218, 3218, 3218, 3218, 3218, 3218, 3218, 3218, 3218,
    3218, 3218, 3218, 3218, 3218, 3218, 3218, 3218, 3218, 3218, 3218, 3218,
    3218, 3218, 3218, 3218, 3218, 3218, 3218, 3218, 3218, 3218, 3218, 3218,
    3218, 3218, 3218, 3218, 3218, 3218, 3218, 3218, 3218, 3218, 3218, 3218,
    3218, 3218, 3218, 3218, 3218, 3218, 3218, 3218, 3218, 3218, 3218, 3218,
    3218, 3218, 3218, 3218, 3218, 3218, 3218, 3218, 3218, 3218, 3218, 3218,
    3218, 3218, 3218, 3218, 3218, 3218, 3218, 3218, 3218, 3218, 3218, 3218,
    3218, 3218, 3218, 3218, 3218, 3218, 3218, 3218, 2920, 2920, 2920, 2920,
    2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920,
    2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920,
    2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920,
    2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920,
    2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920,
    2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920,
    2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920,
    2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920,
    2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920,
    2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920,
    2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920,
    2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920,
    2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920,
    2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920,
    2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920,
    2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920,
    2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920,
    2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920,
    2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920,
    2920, 2920, 2920, 2920, 2920, 2920, 2920, 2920, 525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    3315, 525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,  525,
    525,  3315, 525,  525,
};

int32_t utf8_property_page_offsets[8704] = {
    0,     128,   256,   384,   512,   640,   768,   896,   1024,  1152,
    1280,  1408,  1536,  1664,  1792,  1920,  2048,  2176,  2304,  2432,
    2560,  2688,  2816,  2944,  3072,  3200,  3328,  3456,  3584,  3712,
    3840,  3968,  4096,  4224,  4352,  4480,  4608,  4736,  4864,  4992,
    5120,  4480,  4480,  4480,  5248,  5376,  5504,  5632,  5760,  5888,
    6016,  6144,  6272,  6400,  6528,  6656,  6784,  6912,  7040,  7168,
    7296,  7424,  7552,  7680,  7808,  7936,  8064,  8192,  8320,  8448,
    8576,  8704,  8832,  8960,  9088,  9216,  9344,  9472,  9600,  9728,
    9856,  9856,  9984,  10112, 10240, 10368, 10496, 10624, 10752, 10880,
    11008, 11136, 11264, 11392, 11520, 11648, 11776, 11904, 12032, 12160,
    12288, 12416, 12544, 12672, 12800, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 13056, 12800, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 13184,
    13312, 13440, 13440, 13440, 13440, 13440, 13440, 13440, 13440, 13568,
    4480,  4480,  13696, 13824, 13952, 14080, 14208, 14336, 14464, 14592,
    14720, 14848, 14976, 15104, 12800, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 15232, 15360, 12928, 12928, 12928, 12928, 12928, 15488, 15616,
    15360, 12928, 12928, 12928, 12928, 12928, 12928, 15488, 15744, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 15872, 13440, 13440,
    16000, 16128, 16256, 16384, 16512, 16640, 16768, 16896, 17024, 17152,
    17280, 17408, 17536, 17664, 17792, 17920, 12928, 18048, 18176, 18304,
    18432, 18560, 18688, 18816, 18944, 18944, 19072, 19200, 19328, 19456,
    19584, 19712, 19840, 19968, 20096, 20224, 20352, 20480, 20608, 20736,
    20864, 20992, 21120, 21248, 21376, 21504, 21632, 21760, 21888, 22016,
    22144, 22272, 22400, 22528, 12928, 22656, 22784, 22912, 23040, 12928,
    23168, 23296, 23424, 23552, 23680, 23808, 23936, 24064, 24192, 24320,
    24448, 24576, 12928, 24704, 24832, 24960, 18944, 18944, 18944, 18944,
    18944, 18944, 18944, 25088, 25216, 18944, 25344, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 25472, 18944, 18944,
    18944, 18944, 18944, 18944, 18944, 18944, 25600, 18944, 18944, 18944,
    18944, 18944, 18944, 18944, 18944, 18944, 18944, 18944, 18944, 18944,
    18944, 18944, 18944, 18944, 18944, 18944, 18944, 18944, 18944, 18944,
    18944, 18944, 18944, 18944, 18944, 18944, 18944, 17664, 18944, 18944,
    18944, 18944, 25728, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 25856, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    18944, 18944, 18944, 18944, 25984, 26112, 26240, 26368, 12928, 12928,
    26496, 12928, 26624, 26752, 26880, 27008, 27136, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 27264, 27392, 27392, 27392, 27392, 27392, 27392,
    27392, 27392, 27392, 27520, 27648, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 27776, 27392, 27392, 27904, 27392, 27392, 28032,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 28160, 28288,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    28416, 28544, 28416, 28416, 28416, 28672, 28800, 28928, 29056, 29184,
    29312, 29440, 29568, 29696, 29824, 12928, 29952, 30080, 30208, 30336,
    30464, 30592, 30720, 30848, 29056, 29056, 29056, 29056, 30976, 31104,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 31232, 12928,
    31360, 31488, 31616, 12928, 12928, 31744, 12928, 12928, 12928, 31872,
    12928, 32000, 12928, 12928, 12928, 32128, 32256, 32384, 32512, 12928,
    12928, 12928, 12928, 12928, 32640, 32768, 32896, 12928, 33024, 33152,
    12928, 12928, 33280, 33408, 33536, 33664, 33792, 12928, 33920, 34048,
    34176, 34304, 34432, 34560, 34688, 34816, 34944, 35072, 35200, 35328,
    35456, 35584, 35712, 35840, 28416, 35968, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 27136, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 36096, 27136, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    36224, 12928, 36352, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 36480, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 36608, 12928, 12928, 12928, 12928,
    36736, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    27392, 27392, 27392, 27392, 36864, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 27136, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 36992, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 37120, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 37248, 12928,
    37376, 37504, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    37632, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 37760, 37632, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928, 12928,
    12928, 12928, 12928, 37760,
};

const char *sfce_error_code_names[] = {
#define o(name) #name,
    SFCE_ERROR_CODE(o)
#undef o
};

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
        .main_cursor = {},
        .rectangle.left = 0,
        .rectangle.top = 0,
        .rectangle.right = console.window_size.width - 1,
        .rectangle.bottom = console.window_size.height - 1,
        .tree = tree,
        .split_kind = SFCE_SPLIT_NONE,
        .enable_line_numbering = 1,
        .enable_relative_line_numbering = 1,
        .filepath = argv[1],
    };

    struct sfce_keypress keypress = {};
    int32_t running = SFCE_TRUE;
    int32_t should_render = SFCE_TRUE;
    while (running) {
        keypress = sfce_get_keypress();
        switch (keypress.keycode) {
        case SFCE_KEYCODE_NO_KEY_PRESS: {
            if (should_render) {
                should_render = SFCE_FALSE;
                goto render_console;
            }

            continue;
        }

        case SFCE_KEYCODE_ESCAPE: {
            running = SFCE_FALSE;
            continue;
        }

        case SFCE_KEYCODE_DELETE: {
            error_code = sfce_piece_tree_erase_with_position(window.tree, window.main_cursor.position, 1);

            if (error_code != SFCE_ERROR_OK) {
                goto error;
            }
        } break;

        case SFCE_KEYCODE_F10: {
            error_code = sfce_piece_tree_write_to_file(window.tree, window.filepath);
            if (error_code != SFCE_ERROR_OK) {
                goto error;
            }

            break;
        }

        case SFCE_KEYCODE_BACKSPACE: {
            int32_t position_offset = sfce_piece_tree_offset_at_position(window.tree, window.main_cursor.position);
            window.main_cursor.position = sfce_piece_tree_position_at_offset(window.tree, position_offset - 1);
            error_code = sfce_piece_tree_erase_with_position(window.tree, window.main_cursor.position, 1);

            if (error_code != SFCE_ERROR_OK) {
                goto error;
            }

            break;
        }

        case SFCE_KEYCODE_ARROW_RIGHT: {
            int32_t position_offset = sfce_piece_tree_offset_at_position(window.tree, window.main_cursor.position);
            window.main_cursor.position = sfce_piece_tree_position_at_offset(window.tree, position_offset + 1);

            break;
        }

        case SFCE_KEYCODE_ARROW_LEFT: {
            int32_t position_offset = sfce_piece_tree_offset_at_position(window.tree, window.main_cursor.position);
            window.main_cursor.position = sfce_piece_tree_position_at_offset(window.tree, position_offset - 1);

            break;
        }

        case SFCE_KEYCODE_ARROW_UP: {
            window.main_cursor.position.row = MAX(window.main_cursor.position.row - 1, 0);

            break;
        }

        case SFCE_KEYCODE_ARROW_DOWN: {
            window.main_cursor.position.row = MIN(window.main_cursor.position.row + 1, window.tree->line_count - 1);

            break;
        }

        default: {
            uint8_t buffer[4] = {};
            int32_t buffer_length = sfce_encode_utf8_codepoint(keypress.codepoint, buffer);
            error_code = sfce_piece_tree_insert_with_position(tree, window.main_cursor.position, buffer, buffer_length);
            int32_t codepoint_length = sfce_codepoint_utf8_byte_count(keypress.codepoint);
            int32_t position_offset = sfce_piece_tree_offset_at_position(window.tree, window.main_cursor.position);
            window.main_cursor.position = sfce_piece_tree_position_at_offset(window.tree, position_offset + codepoint_length);
            if (error_code != SFCE_ERROR_OK) {
                goto error;
            }

            break;
        }
        }

render_console:
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

enum sfce_error_code sfce_write_zero_terminated_string(const void *buffer)
{
    return sfce_write(buffer, strlen((const char *)buffer));
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

int32_t sfce_to_upper(int32_t codepoint)
{
    const struct sfce_utf8_property *property = sfce_utf8_codepoint_property(codepoint);

    if (property->uppercase_mapping != -1) {
        return property->uppercase_mapping;
    }

    return codepoint;
}

int32_t sfce_to_lower(int32_t codepoint)
{
    const struct sfce_utf8_property *property = sfce_utf8_codepoint_property(codepoint);

    if (property->lowercase_mapping != -1) {
        return property->lowercase_mapping;
    }

    return codepoint;
}

const struct sfce_utf8_property *sfce_utf8_codepoint_property_unchecked(int32_t codepoint)
{
    uint32_t page_index = utf8_property_page_offsets[codepoint >> 7];
    uint32_t index = utf8_property_indices[page_index + (codepoint & 0x7F)];
    return &utf8_properties[index];
}

const struct sfce_utf8_property *sfce_utf8_codepoint_property(int32_t codepoint)
{
    if (codepoint > 0x10FFFF || codepoint < 0x000000) {
        return &default_utf8_property;
    }

    return sfce_utf8_codepoint_property_unchecked(codepoint);
}

uint8_t sfce_utf8_continuation(uint8_t byte)
{
    return (byte & 0xC0) == 0x80;
}

uint8_t sfce_encode_utf8_codepoint(int32_t codepoint, uint8_t *bytes)
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

int32_t sfce_decode_utf8_codepoint(const void *buffer, int32_t buffer_size)
{
    if (buffer_size <= 0) {
        return -1;
    }

    const uint8_t *bytes = buffer;
    if ((bytes[0] & 0xF8) == 0xF0) {
        if ( buffer_size < 4
        ||  !sfce_utf8_continuation(bytes[1])
        ||  !sfce_utf8_continuation(bytes[2])
        ||  !sfce_utf8_continuation(bytes[3])) {
            return -1;
        }

        // four-byte sequence
        return ((bytes[0] & 0x07) << 18)
        |      ((bytes[1] & 0x3F) << 12)
        |      ((bytes[2] & 0x3F) <<  6)
        |      ((bytes[3] & 0x3F)      );
    }
    else if ((bytes[0] & 0xF0) == 0xE0) {
        if (buffer_size < 3 || !sfce_utf8_continuation(bytes[1]) || !sfce_utf8_continuation(bytes[2])) {
            return -1;
        }

        // three-byte sequence
        return ((bytes[0] & 0x0F) << 12)
        |      ((bytes[1] & 0x3F) <<  6)
        |      ((bytes[2] & 0x3F)      );
    }
    else if ((bytes[0] & 0xE0) == 0xC0) {
        if (buffer_size < 2 || !sfce_utf8_continuation(bytes[1])) {
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

uint8_t sfce_codepoint_width(int32_t codepoint)
{
    return sfce_utf8_codepoint_property(codepoint)->width;
}

int8_t sfce_codepoint_is_print(int32_t codepoint)
{
    enum sfce_unicode_category category = sfce_utf8_codepoint_property(codepoint)->category;

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

            int32_t codepoint = sfce_decode_utf8_codepoint(buffer, character_count);
            return (struct sfce_keypress) { codepoint, codepoint, 0 };
        }

        return (struct sfce_keypress) { character, character, 0 };
    }
    }

    return NO_KEYPRESS;
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
    int32_t buffer_size = sfce_encode_utf8_codepoint(codepoint, buffer);
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
        int32_t codepoint = sfce_decode_utf8_codepoint(&string->data[idx], string->size - idx);
        int32_t codepoint_size = sfce_codepoint_utf8_byte_count(codepoint);
        enum sfce_error_code error_code = sfce_string_push_back_codepoint(result_string, sfce_to_upper(codepoint));

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
        int32_t codepoint = sfce_decode_utf8_codepoint(&string->data[idx], string->size - idx);
        int32_t codepoint_size = sfce_codepoint_utf8_byte_count(codepoint);
        enum sfce_error_code error_code = sfce_string_push_back_codepoint(result_string, sfce_to_lower(codepoint));

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

    // while (node != *root) {
    // while (node->parent != node) {
    while (node != sentinel_ptr) {
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
    // sfce_piece_node_recompute_metadata(root, node);
    // sfce_piece_node_reset_sentinel();

    int64_t counter = 0;

    struct sfce_piece_node *s;
    node->color = SFCE_COLOR_BLACK;
    while (node != *root && node->color == SFCE_COLOR_BLACK) {
        if (counter++ >= 100000) {
            break;
        }

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

static inline void sfce_piece_node_inorder_print_to_string(struct sfce_piece_tree *tree, struct sfce_piece_node *root, struct sfce_string *out)
{
    if (root == sentinel_ptr) {
        return;
    }

    sfce_piece_node_inorder_print(tree, root->left);

    struct sfce_string_view content = sfce_piece_tree_get_piece_content(tree, root->piece);
    sfce_string_nprintf(out, INT32_MAX, "%.*s", content.size, content.data);

    sfce_piece_node_inorder_print(tree, root->right);
}

static inline void sfce_piece_node_to_string(struct sfce_piece_tree *tree, struct sfce_piece_node *node, int32_t space, struct sfce_string *out)
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

static inline int32_t sfce_get_accumulated_value(struct sfce_piece_tree *tree, struct sfce_piece_node *node, int32_t index)
{
    if (index < 0) {
        return 0;
    }

    struct sfce_piece piece = node->piece;
    struct sfce_line_starts *lineStarts = &tree->buffers[piece.buffer_index].line_starts;
    int32_t expectedLineStartIndex = piece.start.line_start_index + index + 1;
    if (expectedLineStartIndex > piece.end.line_start_index) {
        return lineStarts->offsets[piece.end.line_start_index] + piece.end.column - lineStarts->offsets[piece.start.line_start_index] - piece.start.column;
    }
    else {
        return lineStarts->offsets[expectedLineStartIndex] - lineStarts->offsets[piece.start.line_start_index] - piece.start.column;
    }
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

int32_t sfce_piece_tree_get_codepoint_from_node_position(struct sfce_piece_tree *tree, struct sfce_node_position node_position)
{
    struct sfce_string_view piece_content = sfce_piece_tree_get_piece_content(tree, node_position.node->piece);

    int32_t codepoint = sfce_decode_utf8_codepoint(
        &piece_content.data[node_position.offset_within_piece],
        piece_content.size - node_position.offset_within_piece
    );

    return codepoint;
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

    int32_t chunk_size = INT32_MAX;
    struct sfce_piece_node *rightmost = sfce_piece_node_rightmost(tree->root);
    for (int32_t idx = tree->buffer_count; chunk_size != 0; ++idx) {
        struct sfce_string_buffer string_buffer = {};

        error_code = sfce_string_resize(&string_buffer.content, SFCE_STRING_BUFFER_SIZE_THRESHOLD);
        if (error_code != SFCE_ERROR_OK) {
            goto error;
        }

        chunk_size = fread(string_buffer.content.data, 1, SFCE_STRING_BUFFER_SIZE_THRESHOLD, fp);

        if (chunk_size != 0) {
            error_code = sfce_string_resize(&string_buffer.content, chunk_size);
            if (error_code != SFCE_ERROR_OK) goto error;

            error_code = sfce_line_starts_push_line_offset(&string_buffer.line_starts, 0);
            if (error_code != SFCE_ERROR_OK) goto error;

            error_code = sfce_string_buffer_recount_line_start_offsets(&string_buffer, 0, string_buffer.content.size);
            if (error_code != SFCE_ERROR_OK) goto error;

            error_code = sfce_piece_tree_add_string_buffer(tree, string_buffer);
            if (error_code != SFCE_ERROR_OK) goto error;
        }
        else {
error:
            sfce_string_buffer_destroy(&string_buffer);
            break;
        }

        struct sfce_piece_node *node = calloc(1, sizeof *node);

        if (node == NULL) {
            error_code = SFCE_ERROR_OUT_OF_MEMORY;
            goto done;
        }

        struct sfce_piece piece = {};
        piece.buffer_index = idx;
        piece.length = string_buffer.content.size;
        piece.line_count = string_buffer.line_starts.count - 1;
        piece.end.line_start_index = string_buffer.line_starts.count - 1;
        piece.end.column = string_buffer.content.size - string_buffer.line_starts.offsets[piece.end.line_start_index];

        *node = (struct sfce_piece_node) {
            .left = sentinel_ptr,
            .right = sentinel_ptr,
            .parent = sentinel_ptr,
            .piece = piece,
            .color = SFCE_COLOR_BLACK,
        };

        sfce_piece_node_insert_right(&tree->root, rightmost, node);
        rightmost = node;
    }

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

int32_t sfce_piece_tree_get_line_length(struct sfce_piece_tree *tree, int32_t row)
{
    int32_t offset0 = sfce_piece_tree_offset_at_position(tree, (struct sfce_position) { 0, row });
    int32_t offset1 = sfce_piece_tree_offset_at_position(tree, (struct sfce_position) { 0, row + 1 });
    return offset1 - offset0;
}

int32_t sfce_piece_tree_get_render_column(struct sfce_piece_tree *tree, int32_t row, int32_t col)
{
    int32_t render_column = 0;
    return 0;
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
        int32_t codepoint = sfce_decode_utf8_codepoint(iter, remaining);
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
                uint8_t buffer_size = sfce_encode_utf8_codepoint(console->cells[idx].codepoint, buffer);
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
        if (window->cursors != NULL) {
            free(window->cursors);
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
    struct sfce_position cursor_position = window->main_cursor.position;
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

    cursor->target_render_col = sfce_piece_tree_get_render_column(cursor->window->tree, cursor->position.row, cursor->position.col);
}

void sfce_cursor_move_right(struct sfce_cursor *cursor)
{
    int32_t line_character_count = sfce_piece_tree_get_line_length(cursor->window->tree, cursor->position.row);

    if (cursor->position.col < line_character_count) {
        cursor->position.col += 1;
        cursor->target_render_col = sfce_piece_tree_get_render_column(cursor->window->tree, cursor->position.row, cursor->position.col);
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
        cursor->target_render_col = sfce_piece_tree_get_render_column(cursor->window->tree, cursor->position.row, cursor->position.col);
    }
}

bool sfce_cursor_move_word_left(struct sfce_cursor *cursor)
{
    sfce_log_error("sfce_cursor_move_word_left is unimplemented!\n");
    return false;
}

bool sfce_cursor_move_word_right(struct sfce_cursor *cursor)
{
    sfce_log_error("sfce_cursor_move_word_right is unimplemented!\n");
    return false;
}

bool sfce_cursor_move_offset(struct sfce_cursor *cursor, int32_t offset)
{
    sfce_log_error("sfce_cursor_move_offset is unimplemented!\n");
    return false;
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

static inline void sfce_log_error(const char *format, ...)
{
    if (g_should_log_to_error_string) {
        va_list va_args;
        va_start(va_args, format);

        sfce_string_vnprintf(&g_logging_string, INT32_MAX, format, va_args);

        va_end(va_args);
    }
}
