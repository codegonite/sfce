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

// ☆*: .｡. o(≧▽≦)o .｡.:*☆
// 😋

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

#include <assert.h>
#include <locale.h>
#include <math.h>

#if defined(SFCE_PLATFORM_WINDOWS)
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

#define DEBUG_CHARACTERS

enum {
    SFCE_TRUE = 1,
    SFCE_FALSE = 0,
};

enum {
    FNV_PRIME = 0x00000100000001b3,
    FNV_OFFSET_BASIS = 0xcbf29ce484222325,
};

enum { SFCE_DEFAULT_TAB_SIZE = 4 };
enum { SFCE_FILEPATH_MAX = 0x1000 };
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

enum sfce_action_type {
    SFCE_ACTION_NONE,
    SFCE_ACTION_INSERT,
    SFCE_ACTION_REMOVE,
    SFCE_ACTION_INSERT_CHARACTER,
    SFCE_ACTION_REMOVE_CHARACTER,
    SFCE_ACTION_INSERT_LINE,
    SFCE_ACTION_REMOVE_LINE,
    SFCE_ACTION_REPLACE,
    SFCE_ACTION_GROUP,
    SFCE_ACTION_COUNT,
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
    enum sfce_unicode_category      category: 5;
    enum sfce_unicode_bidi_class    bidi_class: 5;
    enum sfce_unicode_decomposition decomposition: 5;
    unsigned                        bidi_mirrored: 1;
    unsigned                        width: 2;
    uint8_t                         combining_class;
    int32_t                         uppercase_mapping;
    int32_t                         lowercase_mapping;
    int32_t                         titlecase_mapping;
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

struct sfce_action {
    struct sfce_action   *parent;
    struct sfce_action   *children;
    struct sfce_action   *next;
    enum sfce_action_type type;
    struct sfce_string    data;
    int32_t               character;
    int32_t               col0;
    int32_t               row0;
    int32_t               col1;
    int32_t               row1;
    int32_t               cursor_index;
};

struct sfce_action_history {
    struct buffer_action * actions;
    size_t                 action_count;
    size_t                 action_capacity;
    size_t                 next_undo_index;
};

struct sfce_editor_window {
    // struct sfce_string           filepath_zero_terminated;
    char                         filepath[SFCE_FILEPATH_MAX + 1];
    struct sfce_piece_tree      *tree;
    struct sfce_cursor          *cursors;
    uint32_t                     cursor_count;
    uint32_t                     scroll_col;
    uint32_t                     scroll_row;
    struct sfce_action_history   history;
    struct sfce_string           status_message;
    struct sfce_rectangle        rectangle;
    struct sfce_editor_window   *parent;
    struct sfce_editor_window   *window0;
    struct sfce_editor_window   *window1;
    enum sfce_split_kind         split_kind;
    uint8_t                      split_percentage;
    uint8_t                      should_close: 1;
    uint8_t                      enable_line_numbering: 1;
    uint8_t                      enable_relative_line_numbering: 1;
    uint8_t                      disable_cursor_scroll: 1;
    uint8_t                      auto_close_brace: 1;
    uint8_t                      auto_indent: 1;
    uint8_t                      display_status: 1;
};

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

const struct sfce_utf8_property *sfce_codepoint_utf8_property_unchecked(int32_t codepoint);
const struct sfce_utf8_property *sfce_codepoint_utf8_property(int32_t codepoint);
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
int32_t sfce_piece_tree_codepoint_at_node_position(struct sfce_piece_tree *tree, struct sfce_node_position node_position);
int32_t sfce_piece_tree_codepoint_at_position(struct sfce_piece_tree *tree, int32_t col, int32_t row);
int32_t sfce_piece_tree_codepoint_at_offset(struct sfce_piece_tree *tree, int32_t offset);
int32_t sfce_piece_tree_character_length_at_node_position(struct sfce_piece_tree *tree, struct sfce_node_position start);
int32_t sfce_piece_tree_get_line_length(struct sfce_piece_tree *tree, int32_t row);
int32_t sfce_piece_tree_get_line_length_without_newline(struct sfce_piece_tree *tree, int32_t row);
uint8_t sfce_piece_tree_byte_at_node_position(struct sfce_piece_tree *tree, struct sfce_node_position node_position);
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
void sfce_editor_window_remove_from_parent(struct sfce_editor_window *window);
enum sfce_error_code sfce_editor_window_display(struct sfce_editor_window *window, struct sfce_console_buffer *console, struct sfce_string *line_temp);

struct sfce_cursor *sfce_cursor_create(struct sfce_editor_window *window);
void sfce_cursor_destroy(struct sfce_cursor *cursor);
void sfce_cursor_move_left(struct sfce_cursor *cursor);
void sfce_cursor_move_right(struct sfce_cursor *cursor);
void sfce_cursor_move_up(struct sfce_cursor *cursor);
void sfce_cursor_move_down(struct sfce_cursor *cursor);
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
static const int g_should_log_to_error_string = 1;

static const struct sfce_utf8_property default_utf8_property = {
    .category          =  SFCE_UNICODE_CATEGORY_CN,
    .bidi_class        =  SFCE_UNICODE_BIDI_CLASS_NONE,
    .decomposition     =  SFCE_UNICODE_DECOMPOSITION_NONE,
    .bidi_mirrored     =  0,
    .width             =  1,
    .combining_class   =  0,
    .uppercase_mapping = -1,
    .lowercase_mapping = -1,
    .titlecase_mapping = -1,
};

//
// Auto generated by utf8gen.js at 2025-02-06
//
static const struct sfce_utf8_property utf8_properties[3189] = {
    { SFCE_UNICODE_CATEGORY_CC, SFCE_UNICODE_BIDI_CLASS_BN, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_CC, SFCE_UNICODE_BIDI_CLASS_S, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_CC, SFCE_UNICODE_BIDI_CLASS_B, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_CC, SFCE_UNICODE_BIDI_CLASS_WS, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_ZS, SFCE_UNICODE_BIDI_CLASS_WS, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_PO, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_PO, SFCE_UNICODE_BIDI_CLASS_ET, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_SC, SFCE_UNICODE_BIDI_CLASS_ET, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_PS, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, 1, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_PE, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, 1, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_SM, SFCE_UNICODE_BIDI_CLASS_ES, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_PO, SFCE_UNICODE_BIDI_CLASS_CS, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_PD, SFCE_UNICODE_BIDI_CLASS_ES, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_ND, SFCE_UNICODE_BIDI_CLASS_EN, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_SM, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, 1, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_SM, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 97, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 98, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 99, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 100, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 101, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 102, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 103, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 104, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 105, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 106, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 107, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 108, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 109, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 110, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 111, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 112, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 113, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 114, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 115, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 116, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 117, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 118, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 119, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 120, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 121, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 122, -1 },
    { SFCE_UNICODE_CATEGORY_SK, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_PC, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 65, -1, 65 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66, -1, 66 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 67, -1, 67 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68, -1, 68 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 69, -1, 69 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 70, -1, 70 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 71, -1, 71 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 72, -1, 72 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 73, -1, 73 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 74, -1, 74 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 75, -1, 75 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 76, -1, 76 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 77, -1, 77 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 78, -1, 78 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 79, -1, 79 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 80, -1, 80 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 81, -1, 81 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 82, -1, 82 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 83, -1, 83 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 84, -1, 84 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 85, -1, 85 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 86, -1, 86 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 87, -1, 87 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 88, -1, 88 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 89, -1, 89 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 90, -1, 90 },
    { SFCE_UNICODE_CATEGORY_ZS, SFCE_UNICODE_BIDI_CLASS_CS, SFCE_UNICODE_DECOMPOSITION_NOBREAK, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_SK, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_LO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_SUPER, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_PI, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, 1, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_CF, SFCE_UNICODE_BIDI_CLASS_BN, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_ET, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_SM, SFCE_UNICODE_BIDI_CLASS_ET, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_NO, SFCE_UNICODE_BIDI_CLASS_EN, SFCE_UNICODE_DECOMPOSITION_SUPER, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, 924, -1, 924 },
    { SFCE_UNICODE_CATEGORY_PF, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, 1, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_NO, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_FRACTION, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 224, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 225, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 226, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 227, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 228, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 229, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 230, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 231, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 232, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 233, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 234, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 235, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 236, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 237, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 238, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 239, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 240, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 241, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 242, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 243, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 244, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 245, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 246, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 248, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 249, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 250, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 251, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 252, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 253, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 254, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 192, -1, 192 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 193, -1, 193 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 194, -1, 194 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 195, -1, 195 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 196, -1, 196 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 197, -1, 197 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 198, -1, 198 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 199, -1, 199 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 200, -1, 200 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 201, -1, 201 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 202, -1, 202 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 203, -1, 203 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 204, -1, 204 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 205, -1, 205 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 206, -1, 206 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 207, -1, 207 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 208, -1, 208 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 209, -1, 209 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 210, -1, 210 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 211, -1, 211 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 212, -1, 212 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 213, -1, 213 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 214, -1, 214 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 216, -1, 216 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 217, -1, 217 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 218, -1, 218 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 219, -1, 219 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 220, -1, 220 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 221, -1, 221 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 222, -1, 222 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 376, -1, 376 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 257, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 256, -1, 256 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 259, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 258, -1, 258 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 261, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 260, -1, 260 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 263, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 262, -1, 262 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 265, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 264, -1, 264 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 267, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 266, -1, 266 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 269, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 268, -1, 268 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 271, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 270, -1, 270 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 273, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 272, -1, 272 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 275, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 274, -1, 274 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 277, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 276, -1, 276 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 279, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 278, -1, 278 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 281, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 280, -1, 280 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 283, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 282, -1, 282 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 285, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 284, -1, 284 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 287, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 286, -1, 286 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 289, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 288, -1, 288 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 291, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 290, -1, 290 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 293, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 292, -1, 292 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 295, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 294, -1, 294 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 297, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 296, -1, 296 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 299, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 298, -1, 298 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 301, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 300, -1, 300 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 303, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 302, -1, 302 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, -1, 307, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, 306, -1, 306 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 309, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 308, -1, 308 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 311, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 310, -1, 310 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 314, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 313, -1, 313 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 316, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 315, -1, 315 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 318, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 317, -1, 317 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, -1, 320, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, 319, -1, 319 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 322, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 321, -1, 321 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 324, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 323, -1, 323 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 326, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 325, -1, 325 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 328, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 327, -1, 327 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 331, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 330, -1, 330 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 333, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 332, -1, 332 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 335, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 334, -1, 334 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 337, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 336, -1, 336 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 339, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 338, -1, 338 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 341, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 340, -1, 340 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 343, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 342, -1, 342 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 345, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 344, -1, 344 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 347, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 346, -1, 346 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 349, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 348, -1, 348 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 351, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 350, -1, 350 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 353, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 352, -1, 352 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 355, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 354, -1, 354 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 357, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 356, -1, 356 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 359, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 358, -1, 358 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 361, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 360, -1, 360 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 363, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 362, -1, 362 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 365, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 364, -1, 364 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 367, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 366, -1, 366 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 369, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 368, -1, 368 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 371, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 370, -1, 370 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 373, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 372, -1, 372 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 375, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 374, -1, 374 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 255, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 378, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 377, -1, 377 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 380, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 379, -1, 379 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 382, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 381, -1, 381 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, 83, -1, 83 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 579, -1, 579 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 595, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 387, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 386, -1, 386 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 389, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 388, -1, 388 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 596, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 392, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 391, -1, 391 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 598, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 599, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 396, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 395, -1, 395 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 477, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 601, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 603, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 402, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 401, -1, 401 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 608, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 611, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 502, -1, 502 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 617, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 616, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 409, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 408, -1, 408 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 573, -1, 573 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42972, -1, 42972 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 623, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 626, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 544, -1, 544 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 629, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 417, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 416, -1, 416 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 419, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 418, -1, 418 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 421, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 420, -1, 420 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 640, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 424, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 423, -1, 423 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 643, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 429, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 428, -1, 428 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 648, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 432, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 431, -1, 431 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 650, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 651, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 436, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 435, -1, 435 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 438, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 437, -1, 437 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 658, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 441, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 440, -1, 440 },
    { SFCE_UNICODE_CATEGORY_LO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 445, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 444, -1, 444 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 503, -1, 503 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, -1, 454, 453 },
    { SFCE_UNICODE_CATEGORY_LT, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, 452, 454, 453 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, 452, -1, 453 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, -1, 457, 456 },
    { SFCE_UNICODE_CATEGORY_LT, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, 455, 457, 456 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, 455, -1, 456 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, -1, 460, 459 },
    { SFCE_UNICODE_CATEGORY_LT, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, 458, 460, 459 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, 458, -1, 459 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 462, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 461, -1, 461 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 464, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 463, -1, 463 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 466, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 465, -1, 465 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 468, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 467, -1, 467 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 470, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 469, -1, 469 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 472, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 471, -1, 471 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 474, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 473, -1, 473 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 476, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 475, -1, 475 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 398, -1, 398 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 479, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 478, -1, 478 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 481, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 480, -1, 480 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 483, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 482, -1, 482 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 485, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 484, -1, 484 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 487, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 486, -1, 486 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 489, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 488, -1, 488 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 491, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 490, -1, 490 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 493, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 492, -1, 492 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 495, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 494, -1, 494 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, -1, 499, 498 },
    { SFCE_UNICODE_CATEGORY_LT, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, 497, 499, 498 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, 497, -1, 498 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 501, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 500, -1, 500 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 405, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 447, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 505, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 504, -1, 504 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 507, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 506, -1, 506 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 509, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 508, -1, 508 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 511, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 510, -1, 510 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 513, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 512, -1, 512 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 515, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 514, -1, 514 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 517, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 516, -1, 516 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 519, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 518, -1, 518 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 521, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 520, -1, 520 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 523, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 522, -1, 522 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 525, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 524, -1, 524 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 527, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 526, -1, 526 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 529, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 528, -1, 528 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 531, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 530, -1, 530 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 533, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 532, -1, 532 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 535, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 534, -1, 534 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 537, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 536, -1, 536 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 539, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 538, -1, 538 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 541, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 540, -1, 540 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 543, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 542, -1, 542 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 414, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 547, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 546, -1, 546 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 549, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 548, -1, 548 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 551, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 550, -1, 550 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 553, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 552, -1, 552 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 555, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 554, -1, 554 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 557, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 556, -1, 556 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 559, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 558, -1, 558 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 561, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 560, -1, 560 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 563, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 562, -1, 562 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11365, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 572, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 571, -1, 571 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 410, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11366, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11390, -1, 11390 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11391, -1, 11391 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 578, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 577, -1, 577 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 384, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 649, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 652, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 583, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 582, -1, 582 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 585, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 584, -1, 584 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 587, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 586, -1, 586 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 589, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 588, -1, 588 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 591, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 590, -1, 590 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11375, -1, 11375 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11373, -1, 11373 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11376, -1, 11376 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 385, -1, 385 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 390, -1, 390 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 393, -1, 393 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 394, -1, 394 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 399, -1, 399 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 400, -1, 400 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42923, -1, 42923 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 403, -1, 403 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42924, -1, 42924 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 404, -1, 404 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42955, -1, 42955 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42893, -1, 42893 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42922, -1, 42922 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 407, -1, 407 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 406, -1, 406 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42926, -1, 42926 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11362, -1, 11362 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42925, -1, 42925 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 412, -1, 412 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11374, -1, 11374 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 413, -1, 413 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 415, -1, 415 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11364, -1, 11364 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 422, -1, 422 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42949, -1, 42949 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 425, -1, 425 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42929, -1, 42929 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 430, -1, 430 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 580, -1, 580 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 433, -1, 433 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 434, -1, 434 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 581, -1, 581 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 439, -1, 439 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42930, -1, 42930 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42928, -1, 42928 },
    { SFCE_UNICODE_CATEGORY_LM, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_SUPER, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_LM, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_LM, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_MN, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 230, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_MN, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 232, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_MN, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 220, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_MN, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 216, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_MN, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 202, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_MN, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 1, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_MN, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 240, 921, -1, 921 },
    { SFCE_UNICODE_CATEGORY_MN, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_MN, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 233, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_MN, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 234, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 881, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 880, -1, 880 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 883, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 882, -1, 882 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 887, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 886, -1, 886 },
    { SFCE_UNICODE_CATEGORY_CN, SFCE_UNICODE_BIDI_CLASS_NONE, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_LM, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1021, -1, 1021 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1022, -1, 1022 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1023, -1, 1023 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1011, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 940, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 941, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 942, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 943, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 972, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 973, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 974, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 945, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 946, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 947, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 948, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 949, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 950, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 951, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 952, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 953, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 954, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 955, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 956, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 957, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 958, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 959, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 960, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 961, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 963, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 964, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 965, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 966, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 967, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 968, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 969, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 970, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 971, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 902, -1, 902 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 904, -1, 904 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 905, -1, 905 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 906, -1, 906 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 913, -1, 913 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 914, -1, 914 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 915, -1, 915 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 916, -1, 916 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 917, -1, 917 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 918, -1, 918 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 919, -1, 919 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 920, -1, 920 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 921, -1, 921 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 922, -1, 922 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 923, -1, 923 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 924, -1, 924 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 925, -1, 925 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 926, -1, 926 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 927, -1, 927 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 928, -1, 928 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 929, -1, 929 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 931, -1, 931 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 932, -1, 932 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 933, -1, 933 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 934, -1, 934 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 935, -1, 935 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 936, -1, 936 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 937, -1, 937 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 938, -1, 938 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 939, -1, 939 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 908, -1, 908 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 910, -1, 910 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 911, -1, 911 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 983, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, 914, -1, 914 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, 920, -1, 920 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, 934, -1, 934 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, 928, -1, 928 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 975, -1, 975 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 985, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 984, -1, 984 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 987, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 986, -1, 986 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 989, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 988, -1, 988 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 991, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 990, -1, 990 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 993, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 992, -1, 992 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 995, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 994, -1, 994 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 997, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 996, -1, 996 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 999, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 998, -1, 998 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1001, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1000, -1, 1000 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1003, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1002, -1, 1002 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1005, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1004, -1, 1004 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1007, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1006, -1, 1006 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, 922, -1, 922 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, 929, -1, 929 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, 1017, -1, 1017 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 895, -1, 895 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, -1, 952, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, 917, -1, 917 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1016, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1015, -1, 1015 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, -1, 1010, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1019, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1018, -1, 1018 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 891, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 892, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 893, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1104, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1105, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1106, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1107, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1108, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1109, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1110, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1111, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1112, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1113, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1114, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1115, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1116, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1117, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1118, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1119, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1072, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1073, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1074, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1075, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1076, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1077, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1078, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1079, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1080, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1081, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1082, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1083, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1084, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1085, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1086, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1087, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1088, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1089, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1090, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1091, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1092, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1093, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1094, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1095, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1096, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1097, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1098, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1099, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1100, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1101, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1102, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1103, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1040, -1, 1040 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1041, -1, 1041 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1042, -1, 1042 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1043, -1, 1043 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1044, -1, 1044 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1045, -1, 1045 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1046, -1, 1046 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1047, -1, 1047 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1048, -1, 1048 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1049, -1, 1049 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1050, -1, 1050 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1051, -1, 1051 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1052, -1, 1052 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1053, -1, 1053 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1054, -1, 1054 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1055, -1, 1055 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1056, -1, 1056 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1057, -1, 1057 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1058, -1, 1058 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1059, -1, 1059 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1060, -1, 1060 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1061, -1, 1061 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1062, -1, 1062 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1063, -1, 1063 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1064, -1, 1064 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1065, -1, 1065 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1066, -1, 1066 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1067, -1, 1067 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1068, -1, 1068 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1069, -1, 1069 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1070, -1, 1070 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1071, -1, 1071 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1024, -1, 1024 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1025, -1, 1025 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1026, -1, 1026 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1027, -1, 1027 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1028, -1, 1028 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1029, -1, 1029 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1030, -1, 1030 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1031, -1, 1031 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1032, -1, 1032 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1033, -1, 1033 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1034, -1, 1034 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1035, -1, 1035 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1036, -1, 1036 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1037, -1, 1037 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1038, -1, 1038 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1039, -1, 1039 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1121, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1120, -1, 1120 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1123, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1122, -1, 1122 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1125, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1124, -1, 1124 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1127, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1126, -1, 1126 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1129, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1128, -1, 1128 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1131, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1130, -1, 1130 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1133, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1132, -1, 1132 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1135, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1134, -1, 1134 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1137, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1136, -1, 1136 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1139, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1138, -1, 1138 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1141, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1140, -1, 1140 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1143, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1142, -1, 1142 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1145, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1144, -1, 1144 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1147, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1146, -1, 1146 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1149, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1148, -1, 1148 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1151, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1150, -1, 1150 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1153, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1152, -1, 1152 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_ME, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1163, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1162, -1, 1162 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1165, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1164, -1, 1164 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1167, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1166, -1, 1166 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1169, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1168, -1, 1168 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1171, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1170, -1, 1170 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1173, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1172, -1, 1172 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1175, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1174, -1, 1174 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1177, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1176, -1, 1176 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1179, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1178, -1, 1178 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1181, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1180, -1, 1180 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1183, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1182, -1, 1182 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1185, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1184, -1, 1184 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1187, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1186, -1, 1186 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1189, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1188, -1, 1188 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1191, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1190, -1, 1190 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1193, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1192, -1, 1192 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1195, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1194, -1, 1194 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1197, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1196, -1, 1196 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1199, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1198, -1, 1198 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1201, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1200, -1, 1200 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1203, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1202, -1, 1202 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1205, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1204, -1, 1204 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1207, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1206, -1, 1206 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1209, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1208, -1, 1208 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1211, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1210, -1, 1210 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1213, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1212, -1, 1212 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1215, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1214, -1, 1214 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1231, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1218, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1217, -1, 1217 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1220, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1219, -1, 1219 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1222, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1221, -1, 1221 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1224, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1223, -1, 1223 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1226, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1225, -1, 1225 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1228, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1227, -1, 1227 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1230, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1229, -1, 1229 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1216, -1, 1216 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1233, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1232, -1, 1232 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1235, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1234, -1, 1234 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1237, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1236, -1, 1236 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1239, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1238, -1, 1238 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1241, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1240, -1, 1240 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1243, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1242, -1, 1242 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1245, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1244, -1, 1244 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1247, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1246, -1, 1246 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1249, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1248, -1, 1248 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1251, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1250, -1, 1250 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1253, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1252, -1, 1252 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1255, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1254, -1, 1254 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1257, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1256, -1, 1256 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1259, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1258, -1, 1258 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1261, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1260, -1, 1260 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1263, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1262, -1, 1262 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1265, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1264, -1, 1264 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1267, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1266, -1, 1266 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1269, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1268, -1, 1268 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1271, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1270, -1, 1270 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1273, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1272, -1, 1272 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1275, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1274, -1, 1274 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1277, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1276, -1, 1276 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1279, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1278, -1, 1278 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1281, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1280, -1, 1280 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1283, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1282, -1, 1282 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1285, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1284, -1, 1284 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1287, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1286, -1, 1286 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1289, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1288, -1, 1288 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1291, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1290, -1, 1290 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1293, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1292, -1, 1292 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1295, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1294, -1, 1294 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1297, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1296, -1, 1296 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1299, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1298, -1, 1298 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1301, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1300, -1, 1300 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1303, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1302, -1, 1302 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1305, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1304, -1, 1304 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1307, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1306, -1, 1306 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1309, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1308, -1, 1308 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1311, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1310, -1, 1310 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1313, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1312, -1, 1312 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1315, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1314, -1, 1314 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1317, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1316, -1, 1316 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1319, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1318, -1, 1318 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1321, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1320, -1, 1320 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1323, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1322, -1, 1322 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1325, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1324, -1, 1324 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1327, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1326, -1, 1326 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1377, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1378, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1379, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1380, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1381, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1382, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1383, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1384, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1385, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1386, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1387, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1388, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1389, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1390, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1391, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1392, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1393, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1394, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1395, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1396, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1397, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1398, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1399, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1400, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1401, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1402, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1403, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1404, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1405, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1406, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1407, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1408, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1409, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1410, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1411, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1412, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1413, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 1414, -1 },
    { SFCE_UNICODE_CATEGORY_PO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1329, -1, 1329 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1330, -1, 1330 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1331, -1, 1331 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1332, -1, 1332 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1333, -1, 1333 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1334, -1, 1334 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1335, -1, 1335 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1336, -1, 1336 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1337, -1, 1337 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1338, -1, 1338 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1339, -1, 1339 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1340, -1, 1340 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1341, -1, 1341 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1342, -1, 1342 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1343, -1, 1343 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1344, -1, 1344 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1345, -1, 1345 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1346, -1, 1346 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1347, -1, 1347 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1348, -1, 1348 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1349, -1, 1349 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1350, -1, 1350 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1351, -1, 1351 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1352, -1, 1352 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1353, -1, 1353 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1354, -1, 1354 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1355, -1, 1355 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1356, -1, 1356 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1357, -1, 1357 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1358, -1, 1358 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1359, -1, 1359 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1360, -1, 1360 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1361, -1, 1361 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1362, -1, 1362 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1363, -1, 1363 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1364, -1, 1364 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1365, -1, 1365 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 1366, -1, 1366 },
    { SFCE_UNICODE_CATEGORY_PD, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_MN, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 222, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_MN, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 228, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_MN, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 10, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_MN, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 11, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_MN, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 12, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_MN, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 13, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_MN, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 14, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_MN, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 15, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_MN, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 16, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_MN, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 17, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_MN, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 18, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_MN, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 19, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_MN, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 20, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_MN, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 21, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_MN, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 22, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_PD, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_MN, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 23, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_PO, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_MN, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 24, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_MN, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 25, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_LO, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_CF, SFCE_UNICODE_BIDI_CLASS_AN, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_SM, SFCE_UNICODE_BIDI_CLASS_AL, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_SC, SFCE_UNICODE_BIDI_CLASS_AL, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_PO, SFCE_UNICODE_BIDI_CLASS_AL, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_MN, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 30, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_MN, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 31, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_MN, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 32, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_CF, SFCE_UNICODE_BIDI_CLASS_AL, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_LO, SFCE_UNICODE_BIDI_CLASS_AL, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_LM, SFCE_UNICODE_BIDI_CLASS_AL, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_MN, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 27, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_MN, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 28, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_MN, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 29, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_MN, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 33, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_MN, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 34, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_ND, SFCE_UNICODE_BIDI_CLASS_AN, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_PO, SFCE_UNICODE_BIDI_CLASS_AN, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_MN, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 35, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_LO, SFCE_UNICODE_BIDI_CLASS_AL, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_AL, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_MN, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 36, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_ND, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_LM, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_SC, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_SK, SFCE_UNICODE_BIDI_CLASS_AL, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_MC, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_MN, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 7, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_MN, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 9, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_ND, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_NO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_MN, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 84, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_MN, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 91, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_NO, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_MN, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_LO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_MN, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 103, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_MN, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 107, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_MN, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 118, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_MN, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 122, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_PO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NOBREAK, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_MN, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 129, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_MN, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 130, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_MN, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 132, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_MN, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11520, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11521, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11522, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11523, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11524, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11525, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11526, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11527, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11528, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11529, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11530, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11531, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11532, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11533, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11534, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11535, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11536, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11537, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11538, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11539, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11540, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11541, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11542, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11543, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11544, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11545, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11546, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11547, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11548, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11549, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11550, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11551, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11552, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11553, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11554, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11555, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11556, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11557, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11559, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11565, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7312, -1, 4304 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7313, -1, 4305 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7314, -1, 4306 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7315, -1, 4307 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7316, -1, 4308 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7317, -1, 4309 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7318, -1, 4310 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7319, -1, 4311 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7320, -1, 4312 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7321, -1, 4313 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7322, -1, 4314 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7323, -1, 4315 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7324, -1, 4316 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7325, -1, 4317 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7326, -1, 4318 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7327, -1, 4319 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7328, -1, 4320 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7329, -1, 4321 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7330, -1, 4322 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7331, -1, 4323 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7332, -1, 4324 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7333, -1, 4325 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7334, -1, 4326 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7335, -1, 4327 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7336, -1, 4328 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7337, -1, 4329 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7338, -1, 4330 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7339, -1, 4331 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7340, -1, 4332 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7341, -1, 4333 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7342, -1, 4334 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7343, -1, 4335 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7344, -1, 4336 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7345, -1, 4337 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7346, -1, 4338 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7347, -1, 4339 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7348, -1, 4340 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7349, -1, 4341 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7350, -1, 4342 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7351, -1, 4343 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7352, -1, 4344 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7353, -1, 4345 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7354, -1, 4346 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7357, -1, 4349 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7358, -1, 4350 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7359, -1, 4351 },
    { SFCE_UNICODE_CATEGORY_LO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 2, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43888, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43889, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43890, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43891, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43892, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43893, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43894, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43895, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43896, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43897, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43898, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43899, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43900, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43901, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43902, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43903, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43904, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43905, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43906, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43907, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43908, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43909, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43910, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43911, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43912, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43913, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43914, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43915, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43916, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43917, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43918, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43919, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43920, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43921, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43922, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43923, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43924, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43925, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43926, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43927, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43928, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43929, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43930, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43931, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43932, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43933, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43934, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43935, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43936, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43937, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43938, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43939, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43940, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43941, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43942, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43943, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43944, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43945, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43946, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43947, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43948, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43949, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43950, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43951, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43952, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43953, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43954, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43955, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43956, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43957, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43958, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43959, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43960, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43961, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43962, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43963, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43964, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43965, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43966, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43967, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 5112, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 5113, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 5114, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 5115, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 5116, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 5117, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5104, -1, 5104 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5105, -1, 5105 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5106, -1, 5106 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5107, -1, 5107 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5108, -1, 5108 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5109, -1, 5109 },
    { SFCE_UNICODE_CATEGORY_NL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_MC, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 9, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42570, -1, 42570 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7306, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7305, -1, 7305 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 4304, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 4305, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 4306, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 4307, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 4308, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 4309, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 4310, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 4311, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 4312, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 4313, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 4314, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 4315, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 4316, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 4317, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 4318, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 4319, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 4320, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 4321, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 4322, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 4323, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 4324, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 4325, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 4326, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 4327, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 4328, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 4329, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 4330, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 4331, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 4332, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 4333, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 4334, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 4335, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 4336, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 4337, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 4338, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 4339, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 4340, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 4341, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 4342, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 4343, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 4344, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 4345, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 4346, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 4349, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 4350, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 4351, -1 },
    { SFCE_UNICODE_CATEGORY_LM, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_SUB, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42877, -1, 42877 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11363, -1, 11363 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42950, -1, 42950 },
    { SFCE_UNICODE_CATEGORY_MN, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 214, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_MN, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 218, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7681, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7680, -1, 7680 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7683, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7682, -1, 7682 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7685, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7684, -1, 7684 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7687, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7686, -1, 7686 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7689, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7688, -1, 7688 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7691, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7690, -1, 7690 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7693, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7692, -1, 7692 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7695, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7694, -1, 7694 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7697, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7696, -1, 7696 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7699, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7698, -1, 7698 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7701, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7700, -1, 7700 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7703, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7702, -1, 7702 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7705, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7704, -1, 7704 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7707, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7706, -1, 7706 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7709, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7708, -1, 7708 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7711, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7710, -1, 7710 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7713, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7712, -1, 7712 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7715, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7714, -1, 7714 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7717, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7716, -1, 7716 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7719, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7718, -1, 7718 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7721, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7720, -1, 7720 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7723, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7722, -1, 7722 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7725, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7724, -1, 7724 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7727, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7726, -1, 7726 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7729, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7728, -1, 7728 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7731, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7730, -1, 7730 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7733, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7732, -1, 7732 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7735, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7734, -1, 7734 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7737, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7736, -1, 7736 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7739, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7738, -1, 7738 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7741, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7740, -1, 7740 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7743, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7742, -1, 7742 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7745, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7744, -1, 7744 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7747, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7746, -1, 7746 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7749, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7748, -1, 7748 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7751, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7750, -1, 7750 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7753, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7752, -1, 7752 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7755, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7754, -1, 7754 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7757, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7756, -1, 7756 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7759, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7758, -1, 7758 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7761, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7760, -1, 7760 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7763, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7762, -1, 7762 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7765, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7764, -1, 7764 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7767, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7766, -1, 7766 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7769, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7768, -1, 7768 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7771, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7770, -1, 7770 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7773, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7772, -1, 7772 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7775, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7774, -1, 7774 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7777, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7776, -1, 7776 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7779, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7778, -1, 7778 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7781, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7780, -1, 7780 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7783, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7782, -1, 7782 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7785, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7784, -1, 7784 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7787, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7786, -1, 7786 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7789, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7788, -1, 7788 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7791, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7790, -1, 7790 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7793, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7792, -1, 7792 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7795, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7794, -1, 7794 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7797, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7796, -1, 7796 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7799, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7798, -1, 7798 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7801, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7800, -1, 7800 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7803, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7802, -1, 7802 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7805, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7804, -1, 7804 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7807, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7806, -1, 7806 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7809, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7808, -1, 7808 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7811, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7810, -1, 7810 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7813, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7812, -1, 7812 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7815, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7814, -1, 7814 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7817, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7816, -1, 7816 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7819, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7818, -1, 7818 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7821, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7820, -1, 7820 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7823, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7822, -1, 7822 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7825, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7824, -1, 7824 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7827, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7826, -1, 7826 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7829, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7828, -1, 7828 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 223, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7841, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7840, -1, 7840 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7843, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7842, -1, 7842 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7845, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7844, -1, 7844 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7847, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7846, -1, 7846 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7849, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7848, -1, 7848 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7851, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7850, -1, 7850 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7853, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7852, -1, 7852 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7855, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7854, -1, 7854 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7857, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7856, -1, 7856 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7859, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7858, -1, 7858 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7861, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7860, -1, 7860 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7863, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7862, -1, 7862 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7865, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7864, -1, 7864 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7867, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7866, -1, 7866 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7869, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7868, -1, 7868 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7871, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7870, -1, 7870 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7873, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7872, -1, 7872 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7875, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7874, -1, 7874 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7877, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7876, -1, 7876 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7879, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7878, -1, 7878 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7881, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7880, -1, 7880 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7883, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7882, -1, 7882 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7885, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7884, -1, 7884 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7887, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7886, -1, 7886 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7889, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7888, -1, 7888 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7891, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7890, -1, 7890 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7893, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7892, -1, 7892 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7895, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7894, -1, 7894 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7897, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7896, -1, 7896 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7899, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7898, -1, 7898 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7901, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7900, -1, 7900 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7903, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7902, -1, 7902 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7905, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7904, -1, 7904 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7907, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7906, -1, 7906 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7909, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7908, -1, 7908 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7911, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7910, -1, 7910 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7913, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7912, -1, 7912 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7915, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7914, -1, 7914 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7917, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7916, -1, 7916 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7919, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7918, -1, 7918 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7921, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7920, -1, 7920 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7923, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7922, -1, 7922 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7925, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7924, -1, 7924 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7927, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7926, -1, 7926 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7929, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7928, -1, 7928 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7931, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7930, -1, 7930 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7933, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7932, -1, 7932 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7935, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7934, -1, 7934 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7944, -1, 7944 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7945, -1, 7945 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7946, -1, 7946 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7947, -1, 7947 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7948, -1, 7948 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7949, -1, 7949 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7950, -1, 7950 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7951, -1, 7951 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7936, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7937, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7938, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7939, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7940, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7941, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7942, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7943, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7960, -1, 7960 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7961, -1, 7961 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7962, -1, 7962 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7963, -1, 7963 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7964, -1, 7964 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7965, -1, 7965 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7952, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7953, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7954, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7955, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7956, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7957, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7976, -1, 7976 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7977, -1, 7977 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7978, -1, 7978 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7979, -1, 7979 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7980, -1, 7980 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7981, -1, 7981 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7982, -1, 7982 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7983, -1, 7983 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7968, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7969, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7970, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7971, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7972, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7973, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7974, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7975, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7992, -1, 7992 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7993, -1, 7993 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7994, -1, 7994 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7995, -1, 7995 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7996, -1, 7996 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7997, -1, 7997 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7998, -1, 7998 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 7999, -1, 7999 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7984, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7985, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7986, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7987, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7988, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7989, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7990, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7991, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 8008, -1, 8008 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 8009, -1, 8009 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 8010, -1, 8010 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 8011, -1, 8011 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 8012, -1, 8012 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 8013, -1, 8013 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 8000, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 8001, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 8002, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 8003, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 8004, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 8005, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 8025, -1, 8025 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 8027, -1, 8027 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 8029, -1, 8029 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 8031, -1, 8031 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 8017, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 8019, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 8021, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 8023, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 8040, -1, 8040 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 8041, -1, 8041 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 8042, -1, 8042 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 8043, -1, 8043 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 8044, -1, 8044 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 8045, -1, 8045 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 8046, -1, 8046 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 8047, -1, 8047 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 8032, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 8033, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 8034, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 8035, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 8036, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 8037, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 8038, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 8039, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 8122, -1, 8122 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 8123, -1, 8123 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 8136, -1, 8136 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 8137, -1, 8137 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 8138, -1, 8138 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 8139, -1, 8139 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 8154, -1, 8154 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 8155, -1, 8155 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 8184, -1, 8184 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 8185, -1, 8185 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 8170, -1, 8170 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 8171, -1, 8171 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 8186, -1, 8186 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 8187, -1, 8187 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 8072, -1, 8072 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 8073, -1, 8073 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 8074, -1, 8074 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 8075, -1, 8075 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 8076, -1, 8076 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 8077, -1, 8077 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 8078, -1, 8078 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 8079, -1, 8079 },
    { SFCE_UNICODE_CATEGORY_LT, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 8064, -1 },
    { SFCE_UNICODE_CATEGORY_LT, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 8065, -1 },
    { SFCE_UNICODE_CATEGORY_LT, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 8066, -1 },
    { SFCE_UNICODE_CATEGORY_LT, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 8067, -1 },
    { SFCE_UNICODE_CATEGORY_LT, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 8068, -1 },
    { SFCE_UNICODE_CATEGORY_LT, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 8069, -1 },
    { SFCE_UNICODE_CATEGORY_LT, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 8070, -1 },
    { SFCE_UNICODE_CATEGORY_LT, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 8071, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 8088, -1, 8088 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 8089, -1, 8089 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 8090, -1, 8090 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 8091, -1, 8091 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 8092, -1, 8092 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 8093, -1, 8093 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 8094, -1, 8094 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 8095, -1, 8095 },
    { SFCE_UNICODE_CATEGORY_LT, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 8080, -1 },
    { SFCE_UNICODE_CATEGORY_LT, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 8081, -1 },
    { SFCE_UNICODE_CATEGORY_LT, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 8082, -1 },
    { SFCE_UNICODE_CATEGORY_LT, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 8083, -1 },
    { SFCE_UNICODE_CATEGORY_LT, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 8084, -1 },
    { SFCE_UNICODE_CATEGORY_LT, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 8085, -1 },
    { SFCE_UNICODE_CATEGORY_LT, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 8086, -1 },
    { SFCE_UNICODE_CATEGORY_LT, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 8087, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 8104, -1, 8104 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 8105, -1, 8105 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 8106, -1, 8106 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 8107, -1, 8107 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 8108, -1, 8108 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 8109, -1, 8109 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 8110, -1, 8110 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 8111, -1, 8111 },
    { SFCE_UNICODE_CATEGORY_LT, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 8096, -1 },
    { SFCE_UNICODE_CATEGORY_LT, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 8097, -1 },
    { SFCE_UNICODE_CATEGORY_LT, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 8098, -1 },
    { SFCE_UNICODE_CATEGORY_LT, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 8099, -1 },
    { SFCE_UNICODE_CATEGORY_LT, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 8100, -1 },
    { SFCE_UNICODE_CATEGORY_LT, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 8101, -1 },
    { SFCE_UNICODE_CATEGORY_LT, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 8102, -1 },
    { SFCE_UNICODE_CATEGORY_LT, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 8103, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 8120, -1, 8120 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 8121, -1, 8121 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 8124, -1, 8124 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 8112, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 8113, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 8048, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 8049, -1 },
    { SFCE_UNICODE_CATEGORY_LT, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 8115, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 8140, -1, 8140 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 8050, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 8051, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 8052, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 8053, -1 },
    { SFCE_UNICODE_CATEGORY_LT, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 8131, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 8152, -1, 8152 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 8153, -1, 8153 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 8144, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 8145, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 8054, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 8055, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 8168, -1, 8168 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 8169, -1, 8169 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 8172, -1, 8172 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 8160, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 8161, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 8058, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 8059, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 8165, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 8188, -1, 8188 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 8056, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 8057, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 8060, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 8061, -1 },
    { SFCE_UNICODE_CATEGORY_LT, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 8179, -1 },
    { SFCE_UNICODE_CATEGORY_ZS, SFCE_UNICODE_BIDI_CLASS_WS, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_ZS, SFCE_UNICODE_BIDI_CLASS_WS, SFCE_UNICODE_DECOMPOSITION_NOBREAK, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_CF, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_CF, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_PD, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NOBREAK, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_PO, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_PI, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_PF, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_PS, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_ZL, SFCE_UNICODE_BIDI_CLASS_WS, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_ZP, SFCE_UNICODE_BIDI_CLASS_B, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_CF, SFCE_UNICODE_BIDI_CLASS_LRE, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_CF, SFCE_UNICODE_BIDI_CLASS_RLE, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_CF, SFCE_UNICODE_BIDI_CLASS_PDF, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_CF, SFCE_UNICODE_BIDI_CLASS_LRO, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_CF, SFCE_UNICODE_BIDI_CLASS_RLO, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_PO, SFCE_UNICODE_BIDI_CLASS_ET, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_SM, SFCE_UNICODE_BIDI_CLASS_CS, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_CF, SFCE_UNICODE_BIDI_CLASS_LRI, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_CF, SFCE_UNICODE_BIDI_CLASS_RLI, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_CF, SFCE_UNICODE_BIDI_CLASS_FSI, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_CF, SFCE_UNICODE_BIDI_CLASS_PDI, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_SM, SFCE_UNICODE_BIDI_CLASS_ES, SFCE_UNICODE_DECOMPOSITION_SUPER, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_SM, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_SUPER, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_PS, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_SUPER, 1, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_PE, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_SUPER, 1, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_NO, SFCE_UNICODE_BIDI_CLASS_EN, SFCE_UNICODE_DECOMPOSITION_SUB, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_SM, SFCE_UNICODE_BIDI_CLASS_ES, SFCE_UNICODE_DECOMPOSITION_SUB, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_SM, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_SUB, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_PS, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_SUB, 1, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_PE, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_SUB, 1, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_SC, SFCE_UNICODE_BIDI_CLASS_ET, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_FONT, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_FONT, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_SUPER, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 8526, -1 },
    { SFCE_UNICODE_CATEGORY_SM, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_FONT, 1, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 8498, -1, 8498 },
    { SFCE_UNICODE_CATEGORY_NL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, -1, 8560, -1 },
    { SFCE_UNICODE_CATEGORY_NL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, -1, 8561, -1 },
    { SFCE_UNICODE_CATEGORY_NL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, -1, 8562, -1 },
    { SFCE_UNICODE_CATEGORY_NL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, -1, 8563, -1 },
    { SFCE_UNICODE_CATEGORY_NL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, -1, 8564, -1 },
    { SFCE_UNICODE_CATEGORY_NL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, -1, 8565, -1 },
    { SFCE_UNICODE_CATEGORY_NL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, -1, 8566, -1 },
    { SFCE_UNICODE_CATEGORY_NL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, -1, 8567, -1 },
    { SFCE_UNICODE_CATEGORY_NL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, -1, 8568, -1 },
    { SFCE_UNICODE_CATEGORY_NL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, -1, 8569, -1 },
    { SFCE_UNICODE_CATEGORY_NL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, -1, 8570, -1 },
    { SFCE_UNICODE_CATEGORY_NL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, -1, 8571, -1 },
    { SFCE_UNICODE_CATEGORY_NL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, -1, 8572, -1 },
    { SFCE_UNICODE_CATEGORY_NL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, -1, 8573, -1 },
    { SFCE_UNICODE_CATEGORY_NL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, -1, 8574, -1 },
    { SFCE_UNICODE_CATEGORY_NL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, -1, 8575, -1 },
    { SFCE_UNICODE_CATEGORY_NL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, 8544, -1, 8544 },
    { SFCE_UNICODE_CATEGORY_NL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, 8545, -1, 8545 },
    { SFCE_UNICODE_CATEGORY_NL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, 8546, -1, 8546 },
    { SFCE_UNICODE_CATEGORY_NL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, 8547, -1, 8547 },
    { SFCE_UNICODE_CATEGORY_NL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, 8548, -1, 8548 },
    { SFCE_UNICODE_CATEGORY_NL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, 8549, -1, 8549 },
    { SFCE_UNICODE_CATEGORY_NL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, 8550, -1, 8550 },
    { SFCE_UNICODE_CATEGORY_NL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, 8551, -1, 8551 },
    { SFCE_UNICODE_CATEGORY_NL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, 8552, -1, 8552 },
    { SFCE_UNICODE_CATEGORY_NL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, 8553, -1, 8553 },
    { SFCE_UNICODE_CATEGORY_NL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, 8554, -1, 8554 },
    { SFCE_UNICODE_CATEGORY_NL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, 8555, -1, 8555 },
    { SFCE_UNICODE_CATEGORY_NL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, 8556, -1, 8556 },
    { SFCE_UNICODE_CATEGORY_NL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, 8557, -1, 8557 },
    { SFCE_UNICODE_CATEGORY_NL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, 8558, -1, 8558 },
    { SFCE_UNICODE_CATEGORY_NL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, 8559, -1, 8559 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 8580, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 8579, -1, 8579 },
    { SFCE_UNICODE_CATEGORY_SM, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_COMPAT, 1, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 2, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_PS, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, 1, 2, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_PE, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, 1, 2, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_NO, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_NO, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_NO, SFCE_UNICODE_BIDI_CLASS_EN, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 0, 1, 0, -1, 9424, -1 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 0, 1, 0, -1, 9425, -1 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 0, 1, 0, -1, 9426, -1 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 0, 1, 0, -1, 9427, -1 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 0, 1, 0, -1, 9428, -1 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 0, 1, 0, -1, 9429, -1 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 0, 1, 0, -1, 9430, -1 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 0, 1, 0, -1, 9431, -1 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 0, 1, 0, -1, 9432, -1 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 0, 1, 0, -1, 9433, -1 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 0, 1, 0, -1, 9434, -1 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 0, 1, 0, -1, 9435, -1 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 0, 1, 0, -1, 9436, -1 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 0, 1, 0, -1, 9437, -1 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 0, 1, 0, -1, 9438, -1 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 0, 1, 0, -1, 9439, -1 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 0, 1, 0, -1, 9440, -1 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 0, 1, 0, -1, 9441, -1 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 0, 1, 0, -1, 9442, -1 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 0, 1, 0, -1, 9443, -1 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 0, 1, 0, -1, 9444, -1 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 0, 1, 0, -1, 9445, -1 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 0, 1, 0, -1, 9446, -1 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 0, 1, 0, -1, 9447, -1 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 0, 1, 0, -1, 9448, -1 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 0, 1, 0, -1, 9449, -1 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 0, 1, 0, 9398, -1, 9398 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 0, 1, 0, 9399, -1, 9399 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 0, 1, 0, 9400, -1, 9400 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 0, 1, 0, 9401, -1, 9401 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 0, 1, 0, 9402, -1, 9402 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 0, 1, 0, 9403, -1, 9403 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 0, 1, 0, 9404, -1, 9404 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 0, 1, 0, 9405, -1, 9405 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 0, 1, 0, 9406, -1, 9406 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 0, 1, 0, 9407, -1, 9407 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 0, 1, 0, 9408, -1, 9408 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 0, 1, 0, 9409, -1, 9409 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 0, 1, 0, 9410, -1, 9410 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 0, 1, 0, 9411, -1, 9411 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 0, 1, 0, 9412, -1, 9412 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 0, 1, 0, 9413, -1, 9413 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 0, 1, 0, 9414, -1, 9414 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 0, 1, 0, 9415, -1, 9415 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 0, 1, 0, 9416, -1, 9416 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 0, 1, 0, 9417, -1, 9417 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 0, 1, 0, 9418, -1, 9418 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 0, 1, 0, 9419, -1, 9419 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 0, 1, 0, 9420, -1, 9420 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 0, 1, 0, 9421, -1, 9421 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 0, 1, 0, 9422, -1, 9422 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 0, 1, 0, 9423, -1, 9423 },
    { SFCE_UNICODE_CATEGORY_SM, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 2, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_SM, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, 1, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11312, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11313, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11314, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11315, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11316, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11317, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11318, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11319, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11320, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11321, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11322, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11323, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11324, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11325, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11326, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11327, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11328, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11329, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11330, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11331, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11332, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11333, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11334, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11335, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11336, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11337, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11338, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11339, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11340, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11341, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11342, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11343, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11344, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11345, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11346, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11347, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11348, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11349, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11350, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11351, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11352, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11353, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11354, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11355, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11356, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11357, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11358, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11359, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11264, -1, 11264 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11265, -1, 11265 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11266, -1, 11266 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11267, -1, 11267 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11268, -1, 11268 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11269, -1, 11269 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11270, -1, 11270 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11271, -1, 11271 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11272, -1, 11272 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11273, -1, 11273 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11274, -1, 11274 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11275, -1, 11275 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11276, -1, 11276 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11277, -1, 11277 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11278, -1, 11278 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11279, -1, 11279 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11280, -1, 11280 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11281, -1, 11281 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11282, -1, 11282 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11283, -1, 11283 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11284, -1, 11284 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11285, -1, 11285 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11286, -1, 11286 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11287, -1, 11287 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11288, -1, 11288 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11289, -1, 11289 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11290, -1, 11290 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11291, -1, 11291 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11292, -1, 11292 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11293, -1, 11293 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11294, -1, 11294 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11295, -1, 11295 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11296, -1, 11296 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11297, -1, 11297 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11298, -1, 11298 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11299, -1, 11299 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11300, -1, 11300 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11301, -1, 11301 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11302, -1, 11302 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11303, -1, 11303 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11304, -1, 11304 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11305, -1, 11305 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11306, -1, 11306 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11307, -1, 11307 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11308, -1, 11308 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11309, -1, 11309 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11310, -1, 11310 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11311, -1, 11311 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11361, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11360, -1, 11360 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 619, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7549, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 637, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 570, -1, 570 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 574, -1, 574 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11368, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11367, -1, 11367 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11370, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11369, -1, 11369 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11372, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11371, -1, 11371 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 593, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 625, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 592, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 594, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11379, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11378, -1, 11378 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11382, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11381, -1, 11381 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 575, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 576, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11393, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11392, -1, 11392 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11395, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11394, -1, 11394 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11397, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11396, -1, 11396 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11399, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11398, -1, 11398 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11401, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11400, -1, 11400 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11403, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11402, -1, 11402 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11405, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11404, -1, 11404 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11407, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11406, -1, 11406 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11409, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11408, -1, 11408 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11411, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11410, -1, 11410 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11413, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11412, -1, 11412 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11415, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11414, -1, 11414 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11417, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11416, -1, 11416 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11419, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11418, -1, 11418 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11421, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11420, -1, 11420 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11423, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11422, -1, 11422 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11425, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11424, -1, 11424 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11427, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11426, -1, 11426 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11429, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11428, -1, 11428 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11431, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11430, -1, 11430 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11433, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11432, -1, 11432 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11435, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11434, -1, 11434 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11437, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11436, -1, 11436 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11439, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11438, -1, 11438 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11441, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11440, -1, 11440 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11443, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11442, -1, 11442 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11445, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11444, -1, 11444 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11447, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11446, -1, 11446 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11449, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11448, -1, 11448 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11451, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11450, -1, 11450 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11453, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11452, -1, 11452 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11455, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11454, -1, 11454 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11457, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11456, -1, 11456 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11459, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11458, -1, 11458 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11461, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11460, -1, 11460 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11463, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11462, -1, 11462 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11465, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11464, -1, 11464 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11467, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11466, -1, 11466 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11469, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11468, -1, 11468 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11471, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11470, -1, 11470 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11473, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11472, -1, 11472 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11475, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11474, -1, 11474 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11477, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11476, -1, 11476 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11479, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11478, -1, 11478 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11481, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11480, -1, 11480 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11483, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11482, -1, 11482 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11485, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11484, -1, 11484 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11487, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11486, -1, 11486 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11489, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11488, -1, 11488 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11491, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11490, -1, 11490 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11500, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11499, -1, 11499 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11502, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11501, -1, 11501 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 11507, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 11506, -1, 11506 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 4256, -1, 4256 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 4257, -1, 4257 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 4258, -1, 4258 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 4259, -1, 4259 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 4260, -1, 4260 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 4261, -1, 4261 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 4262, -1, 4262 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 4263, -1, 4263 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 4264, -1, 4264 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 4265, -1, 4265 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 4266, -1, 4266 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 4267, -1, 4267 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 4268, -1, 4268 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 4269, -1, 4269 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 4270, -1, 4270 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 4271, -1, 4271 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 4272, -1, 4272 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 4273, -1, 4273 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 4274, -1, 4274 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 4275, -1, 4275 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 4276, -1, 4276 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 4277, -1, 4277 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 4278, -1, 4278 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 4279, -1, 4279 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 4280, -1, 4280 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 4281, -1, 4281 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 4282, -1, 4282 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 4283, -1, 4283 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 4284, -1, 4284 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 4285, -1, 4285 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 4286, -1, 4286 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 4287, -1, 4287 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 4288, -1, 4288 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 4289, -1, 4289 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 4290, -1, 4290 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 4291, -1, 4291 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 4292, -1, 4292 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 4293, -1, 4293 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 4295, -1, 4295 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 4301, -1, 4301 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 2, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_ZS, SFCE_UNICODE_BIDI_CLASS_WS, SFCE_UNICODE_DECOMPOSITION_WIDE, 0, 2, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_PO, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 2, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_LM, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 2, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_NL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 2, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_PD, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 2, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_PS, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 2, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_PE, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 2, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_MN, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 2, 218, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_MN, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 2, 228, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_MN, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 2, 232, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_MN, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 2, 222, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_MC, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 2, 224, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_NL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 2, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_MN, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 2, 8, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_SK, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 2, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_LO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_VERTICAL, 0, 2, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_LO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 2, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 2, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_NO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_SUPER, 0, 2, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_SUPER, 0, 2, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 2, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_NO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 2, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 0, 2, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_SQUARE, 0, 2, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_NO, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 0, 2, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 0, 2, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_NO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 0, 2, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_SQUARE, 0, 2, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42561, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42560, -1, 42560 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42563, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42562, -1, 42562 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42565, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42564, -1, 42564 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42567, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42566, -1, 42566 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42569, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42568, -1, 42568 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42571, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42573, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42572, -1, 42572 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42575, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42574, -1, 42574 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42577, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42576, -1, 42576 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42579, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42578, -1, 42578 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42581, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42580, -1, 42580 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42583, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42582, -1, 42582 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42585, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42584, -1, 42584 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42587, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42586, -1, 42586 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42589, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42588, -1, 42588 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42591, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42590, -1, 42590 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42593, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42592, -1, 42592 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42595, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42594, -1, 42594 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42597, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42596, -1, 42596 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42599, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42598, -1, 42598 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42601, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42600, -1, 42600 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42603, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42602, -1, 42602 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42605, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42604, -1, 42604 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42625, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42624, -1, 42624 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42627, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42626, -1, 42626 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42629, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42628, -1, 42628 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42631, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42630, -1, 42630 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42633, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42632, -1, 42632 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42635, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42634, -1, 42634 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42637, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42636, -1, 42636 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42639, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42638, -1, 42638 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42641, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42640, -1, 42640 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42643, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42642, -1, 42642 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42645, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42644, -1, 42644 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42647, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42646, -1, 42646 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42649, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42648, -1, 42648 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42651, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42650, -1, 42650 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42787, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42786, -1, 42786 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42789, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42788, -1, 42788 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42791, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42790, -1, 42790 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42793, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42792, -1, 42792 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42795, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42794, -1, 42794 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42797, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42796, -1, 42796 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42799, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42798, -1, 42798 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42803, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42802, -1, 42802 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42805, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42804, -1, 42804 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42807, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42806, -1, 42806 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42809, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42808, -1, 42808 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42811, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42810, -1, 42810 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42813, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42812, -1, 42812 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42815, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42814, -1, 42814 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42817, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42816, -1, 42816 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42819, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42818, -1, 42818 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42821, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42820, -1, 42820 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42823, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42822, -1, 42822 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42825, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42824, -1, 42824 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42827, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42826, -1, 42826 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42829, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42828, -1, 42828 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42831, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42830, -1, 42830 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42833, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42832, -1, 42832 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42835, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42834, -1, 42834 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42837, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42836, -1, 42836 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42839, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42838, -1, 42838 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42841, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42840, -1, 42840 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42843, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42842, -1, 42842 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42845, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42844, -1, 42844 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42847, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42846, -1, 42846 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42849, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42848, -1, 42848 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42851, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42850, -1, 42850 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42853, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42852, -1, 42852 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42855, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42854, -1, 42854 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42857, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42856, -1, 42856 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42859, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42858, -1, 42858 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42861, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42860, -1, 42860 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42863, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42862, -1, 42862 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42874, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42873, -1, 42873 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42876, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42875, -1, 42875 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7545, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42879, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42878, -1, 42878 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42881, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42880, -1, 42880 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42883, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42882, -1, 42882 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42885, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42884, -1, 42884 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42887, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42886, -1, 42886 },
    { SFCE_UNICODE_CATEGORY_SK, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42892, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42891, -1, 42891 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 613, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42897, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42896, -1, 42896 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42899, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42898, -1, 42898 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42948, -1, 42948 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42903, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42902, -1, 42902 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42905, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42904, -1, 42904 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42907, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42906, -1, 42906 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42909, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42908, -1, 42908 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42911, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42910, -1, 42910 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42913, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42912, -1, 42912 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42915, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42914, -1, 42914 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42917, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42916, -1, 42916 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42919, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42918, -1, 42918 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42921, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42920, -1, 42920 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 614, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 604, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 609, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 620, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 618, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 670, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 647, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 669, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 43859, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42933, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42932, -1, 42932 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42935, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42934, -1, 42934 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42937, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42936, -1, 42936 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42939, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42938, -1, 42938 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42941, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42940, -1, 42940 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42943, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42942, -1, 42942 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42945, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42944, -1, 42944 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42947, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42946, -1, 42946 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42900, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 642, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 7566, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42952, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42951, -1, 42951 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42954, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42953, -1, 42953 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 612, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42957, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42956, -1, 42956 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42961, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42960, -1, 42960 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42967, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42966, -1, 42966 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42969, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42968, -1, 42968 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42971, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42970, -1, 42970 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 411, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 42998, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42997, -1, 42997 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 42931, -1, 42931 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5024, -1, 5024 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5025, -1, 5025 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5026, -1, 5026 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5027, -1, 5027 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5028, -1, 5028 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5029, -1, 5029 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5030, -1, 5030 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5031, -1, 5031 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5032, -1, 5032 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5033, -1, 5033 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5034, -1, 5034 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5035, -1, 5035 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5036, -1, 5036 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5037, -1, 5037 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5038, -1, 5038 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5039, -1, 5039 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5040, -1, 5040 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5041, -1, 5041 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5042, -1, 5042 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5043, -1, 5043 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5044, -1, 5044 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5045, -1, 5045 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5046, -1, 5046 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5047, -1, 5047 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5048, -1, 5048 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5049, -1, 5049 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5050, -1, 5050 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5051, -1, 5051 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5052, -1, 5052 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5053, -1, 5053 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5054, -1, 5054 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5055, -1, 5055 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5056, -1, 5056 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5057, -1, 5057 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5058, -1, 5058 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5059, -1, 5059 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5060, -1, 5060 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5061, -1, 5061 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5062, -1, 5062 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5063, -1, 5063 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5064, -1, 5064 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5065, -1, 5065 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5066, -1, 5066 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5067, -1, 5067 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5068, -1, 5068 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5069, -1, 5069 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5070, -1, 5070 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5071, -1, 5071 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5072, -1, 5072 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5073, -1, 5073 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5074, -1, 5074 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5075, -1, 5075 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5076, -1, 5076 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5077, -1, 5077 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5078, -1, 5078 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5079, -1, 5079 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5080, -1, 5080 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5081, -1, 5081 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5082, -1, 5082 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5083, -1, 5083 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5084, -1, 5084 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5085, -1, 5085 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5086, -1, 5086 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5087, -1, 5087 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5088, -1, 5088 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5089, -1, 5089 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5090, -1, 5090 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5091, -1, 5091 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5092, -1, 5092 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5093, -1, 5093 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5094, -1, 5094 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5095, -1, 5095 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5096, -1, 5096 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5097, -1, 5097 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5098, -1, 5098 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5099, -1, 5099 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5100, -1, 5100 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5101, -1, 5101 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5102, -1, 5102 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 5103, -1, 5103 },
    { SFCE_UNICODE_CATEGORY_CS, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_CO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_MN, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 26, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_LO, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_FONT, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_SM, SFCE_UNICODE_BIDI_CLASS_ES, SFCE_UNICODE_DECOMPOSITION_FONT, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_LO, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_LO, SFCE_UNICODE_BIDI_CLASS_AL, SFCE_UNICODE_DECOMPOSITION_ISOLATED, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_LO, SFCE_UNICODE_BIDI_CLASS_AL, SFCE_UNICODE_DECOMPOSITION_FINAL, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_LO, SFCE_UNICODE_BIDI_CLASS_AL, SFCE_UNICODE_DECOMPOSITION_INITIAL, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_LO, SFCE_UNICODE_BIDI_CLASS_AL, SFCE_UNICODE_DECOMPOSITION_MEDIAL, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_PE, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_SC, SFCE_UNICODE_BIDI_CLASS_AL, SFCE_UNICODE_DECOMPOSITION_ISOLATED, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_PO, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_VERTICAL, 0, 2, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_PS, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_VERTICAL, 0, 2, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_PE, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_VERTICAL, 0, 2, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_PD, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_VERTICAL, 0, 2, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_PC, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_VERTICAL, 0, 2, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_PO, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 2, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_PC, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_COMPAT, 0, 2, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_PO, SFCE_UNICODE_BIDI_CLASS_CS, SFCE_UNICODE_DECOMPOSITION_SMALL, 0, 2, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_PO, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_SMALL, 0, 2, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_PD, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_SMALL, 0, 2, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_PS, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_SMALL, 1, 2, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_PE, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_SMALL, 1, 2, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_PO, SFCE_UNICODE_BIDI_CLASS_ET, SFCE_UNICODE_DECOMPOSITION_SMALL, 0, 2, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_SM, SFCE_UNICODE_BIDI_CLASS_ES, SFCE_UNICODE_DECOMPOSITION_SMALL, 0, 2, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_PD, SFCE_UNICODE_BIDI_CLASS_ES, SFCE_UNICODE_DECOMPOSITION_SMALL, 0, 2, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_SM, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_SMALL, 1, 2, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_SM, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_SMALL, 0, 2, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_SC, SFCE_UNICODE_BIDI_CLASS_ET, SFCE_UNICODE_DECOMPOSITION_SMALL, 0, 2, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_PO, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_WIDE, 0, 2, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_PO, SFCE_UNICODE_BIDI_CLASS_ET, SFCE_UNICODE_DECOMPOSITION_WIDE, 0, 2, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_SC, SFCE_UNICODE_BIDI_CLASS_ET, SFCE_UNICODE_DECOMPOSITION_WIDE, 0, 2, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_PS, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_WIDE, 1, 2, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_PE, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_WIDE, 1, 2, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_SM, SFCE_UNICODE_BIDI_CLASS_ES, SFCE_UNICODE_DECOMPOSITION_WIDE, 0, 2, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_PO, SFCE_UNICODE_BIDI_CLASS_CS, SFCE_UNICODE_DECOMPOSITION_WIDE, 0, 2, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_PD, SFCE_UNICODE_BIDI_CLASS_ES, SFCE_UNICODE_DECOMPOSITION_WIDE, 0, 2, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_ND, SFCE_UNICODE_BIDI_CLASS_EN, SFCE_UNICODE_DECOMPOSITION_WIDE, 0, 2, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_SM, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_WIDE, 1, 2, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_SM, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_WIDE, 0, 2, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 0, 2, 0, -1, 65345, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 0, 2, 0, -1, 65346, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 0, 2, 0, -1, 65347, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 0, 2, 0, -1, 65348, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 0, 2, 0, -1, 65349, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 0, 2, 0, -1, 65350, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 0, 2, 0, -1, 65351, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 0, 2, 0, -1, 65352, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 0, 2, 0, -1, 65353, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 0, 2, 0, -1, 65354, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 0, 2, 0, -1, 65355, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 0, 2, 0, -1, 65356, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 0, 2, 0, -1, 65357, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 0, 2, 0, -1, 65358, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 0, 2, 0, -1, 65359, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 0, 2, 0, -1, 65360, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 0, 2, 0, -1, 65361, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 0, 2, 0, -1, 65362, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 0, 2, 0, -1, 65363, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 0, 2, 0, -1, 65364, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 0, 2, 0, -1, 65365, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 0, 2, 0, -1, 65366, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 0, 2, 0, -1, 65367, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 0, 2, 0, -1, 65368, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 0, 2, 0, -1, 65369, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 0, 2, 0, -1, 65370, -1 },
    { SFCE_UNICODE_CATEGORY_SK, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_WIDE, 0, 2, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_PC, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_WIDE, 0, 2, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 0, 2, 0, 65313, -1, 65313 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 0, 2, 0, 65314, -1, 65314 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 0, 2, 0, 65315, -1, 65315 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 0, 2, 0, 65316, -1, 65316 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 0, 2, 0, 65317, -1, 65317 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 0, 2, 0, 65318, -1, 65318 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 0, 2, 0, 65319, -1, 65319 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 0, 2, 0, 65320, -1, 65320 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 0, 2, 0, 65321, -1, 65321 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 0, 2, 0, 65322, -1, 65322 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 0, 2, 0, 65323, -1, 65323 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 0, 2, 0, 65324, -1, 65324 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 0, 2, 0, 65325, -1, 65325 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 0, 2, 0, 65326, -1, 65326 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 0, 2, 0, 65327, -1, 65327 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 0, 2, 0, 65328, -1, 65328 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 0, 2, 0, 65329, -1, 65329 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 0, 2, 0, 65330, -1, 65330 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 0, 2, 0, 65331, -1, 65331 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 0, 2, 0, 65332, -1, 65332 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 0, 2, 0, 65333, -1, 65333 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 0, 2, 0, 65334, -1, 65334 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 0, 2, 0, 65335, -1, 65335 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 0, 2, 0, 65336, -1, 65336 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 0, 2, 0, 65337, -1, 65337 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_WIDE, 0, 2, 0, 65338, -1, 65338 },
    { SFCE_UNICODE_CATEGORY_PO, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NARROW, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_PS, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NARROW, 1, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_PE, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NARROW, 1, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_LO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NARROW, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_LM, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NARROW, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_WIDE, 0, 2, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NARROW, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_SM, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NARROW, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_CF, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_NL, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_NO, SFCE_UNICODE_BIDI_CLASS_EN, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66600, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66601, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66602, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66603, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66604, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66605, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66606, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66607, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66608, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66609, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66610, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66611, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66612, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66613, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66614, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66615, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66616, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66617, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66618, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66619, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66620, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66621, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66622, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66623, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66624, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66625, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66626, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66627, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66628, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66629, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66630, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66631, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66632, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66633, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66634, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66635, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66636, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66637, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66638, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66639, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66560, -1, 66560 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66561, -1, 66561 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66562, -1, 66562 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66563, -1, 66563 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66564, -1, 66564 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66565, -1, 66565 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66566, -1, 66566 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66567, -1, 66567 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66568, -1, 66568 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66569, -1, 66569 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66570, -1, 66570 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66571, -1, 66571 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66572, -1, 66572 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66573, -1, 66573 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66574, -1, 66574 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66575, -1, 66575 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66576, -1, 66576 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66577, -1, 66577 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66578, -1, 66578 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66579, -1, 66579 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66580, -1, 66580 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66581, -1, 66581 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66582, -1, 66582 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66583, -1, 66583 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66584, -1, 66584 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66585, -1, 66585 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66586, -1, 66586 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66587, -1, 66587 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66588, -1, 66588 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66589, -1, 66589 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66590, -1, 66590 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66591, -1, 66591 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66592, -1, 66592 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66593, -1, 66593 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66594, -1, 66594 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66595, -1, 66595 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66596, -1, 66596 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66597, -1, 66597 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66598, -1, 66598 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66599, -1, 66599 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66776, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66777, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66778, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66779, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66780, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66781, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66782, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66783, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66784, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66785, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66786, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66787, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66788, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66789, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66790, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66791, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66792, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66793, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66794, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66795, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66796, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66797, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66798, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66799, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66800, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66801, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66802, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66803, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66804, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66805, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66806, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66807, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66808, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66809, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66810, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66811, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66736, -1, 66736 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66737, -1, 66737 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66738, -1, 66738 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66739, -1, 66739 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66740, -1, 66740 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66741, -1, 66741 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66742, -1, 66742 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66743, -1, 66743 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66744, -1, 66744 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66745, -1, 66745 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66746, -1, 66746 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66747, -1, 66747 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66748, -1, 66748 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66749, -1, 66749 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66750, -1, 66750 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66751, -1, 66751 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66752, -1, 66752 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66753, -1, 66753 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66754, -1, 66754 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66755, -1, 66755 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66756, -1, 66756 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66757, -1, 66757 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66758, -1, 66758 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66759, -1, 66759 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66760, -1, 66760 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66761, -1, 66761 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66762, -1, 66762 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66763, -1, 66763 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66764, -1, 66764 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66765, -1, 66765 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66766, -1, 66766 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66767, -1, 66767 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66768, -1, 66768 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66769, -1, 66769 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66770, -1, 66770 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66771, -1, 66771 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66967, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66968, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66969, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66970, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66971, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66972, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66973, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66974, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66975, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66976, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66977, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66979, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66980, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66981, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66982, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66983, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66984, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66985, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66986, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66987, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66988, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66989, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66990, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66991, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66992, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66993, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66995, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66996, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66997, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66998, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 66999, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 67000, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 67001, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 67003, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 67004, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66928, -1, 66928 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66929, -1, 66929 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66930, -1, 66930 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66931, -1, 66931 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66932, -1, 66932 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66933, -1, 66933 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66934, -1, 66934 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66935, -1, 66935 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66936, -1, 66936 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66937, -1, 66937 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66938, -1, 66938 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66940, -1, 66940 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66941, -1, 66941 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66942, -1, 66942 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66943, -1, 66943 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66944, -1, 66944 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66945, -1, 66945 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66946, -1, 66946 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66947, -1, 66947 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66948, -1, 66948 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66949, -1, 66949 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66950, -1, 66950 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66951, -1, 66951 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66952, -1, 66952 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66953, -1, 66953 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66954, -1, 66954 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66956, -1, 66956 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66957, -1, 66957 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66958, -1, 66958 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66959, -1, 66959 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66960, -1, 66960 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66961, -1, 66961 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66962, -1, 66962 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66964, -1, 66964 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 66965, -1, 66965 },
    { SFCE_UNICODE_CATEGORY_NO, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68800, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68801, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68802, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68803, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68804, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68805, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68806, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68807, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68808, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68809, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68810, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68811, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68812, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68813, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68814, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68815, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68816, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68817, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68818, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68819, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68820, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68821, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68822, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68823, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68824, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68825, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68826, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68827, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68828, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68829, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68830, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68831, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68832, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68833, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68834, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68835, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68836, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68837, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68838, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68839, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68840, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68841, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68842, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68843, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68844, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68845, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68846, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68847, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68848, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68849, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68850, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68736, -1, 68736 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68737, -1, 68737 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68738, -1, 68738 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68739, -1, 68739 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68740, -1, 68740 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68741, -1, 68741 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68742, -1, 68742 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68743, -1, 68743 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68744, -1, 68744 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68745, -1, 68745 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68746, -1, 68746 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68747, -1, 68747 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68748, -1, 68748 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68749, -1, 68749 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68750, -1, 68750 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68751, -1, 68751 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68752, -1, 68752 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68753, -1, 68753 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68754, -1, 68754 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68755, -1, 68755 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68756, -1, 68756 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68757, -1, 68757 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68758, -1, 68758 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68759, -1, 68759 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68760, -1, 68760 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68761, -1, 68761 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68762, -1, 68762 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68763, -1, 68763 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68764, -1, 68764 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68765, -1, 68765 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68766, -1, 68766 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68767, -1, 68767 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68768, -1, 68768 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68769, -1, 68769 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68770, -1, 68770 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68771, -1, 68771 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68772, -1, 68772 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68773, -1, 68773 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68774, -1, 68774 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68775, -1, 68775 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68776, -1, 68776 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68777, -1, 68777 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68778, -1, 68778 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68779, -1, 68779 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68780, -1, 68780 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68781, -1, 68781 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68782, -1, 68782 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68783, -1, 68783 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68784, -1, 68784 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68785, -1, 68785 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68786, -1, 68786 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68976, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68977, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68978, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68979, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68980, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68981, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68982, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68983, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68984, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68985, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68986, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68987, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68988, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68989, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68990, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68991, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68992, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68993, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68994, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68995, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68996, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 68997, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68944, -1, 68944 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68945, -1, 68945 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68946, -1, 68946 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68947, -1, 68947 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68948, -1, 68948 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68949, -1, 68949 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68950, -1, 68950 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68951, -1, 68951 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68952, -1, 68952 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68953, -1, 68953 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68954, -1, 68954 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68955, -1, 68955 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68956, -1, 68956 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68957, -1, 68957 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68958, -1, 68958 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68959, -1, 68959 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68960, -1, 68960 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68961, -1, 68961 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68962, -1, 68962 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68963, -1, 68963 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68964, -1, 68964 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 68965, -1, 68965 },
    { SFCE_UNICODE_CATEGORY_SM, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_NO, SFCE_UNICODE_BIDI_CLASS_AN, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_NO, SFCE_UNICODE_BIDI_CLASS_AL, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 71872, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 71873, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 71874, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 71875, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 71876, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 71877, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 71878, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 71879, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 71880, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 71881, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 71882, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 71883, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 71884, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 71885, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 71886, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 71887, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 71888, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 71889, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 71890, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 71891, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 71892, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 71893, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 71894, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 71895, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 71896, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 71897, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 71898, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 71899, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 71900, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 71901, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 71902, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 71903, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 71840, -1, 71840 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 71841, -1, 71841 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 71842, -1, 71842 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 71843, -1, 71843 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 71844, -1, 71844 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 71845, -1, 71845 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 71846, -1, 71846 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 71847, -1, 71847 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 71848, -1, 71848 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 71849, -1, 71849 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 71850, -1, 71850 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 71851, -1, 71851 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 71852, -1, 71852 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 71853, -1, 71853 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 71854, -1, 71854 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 71855, -1, 71855 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 71856, -1, 71856 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 71857, -1, 71857 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 71858, -1, 71858 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 71859, -1, 71859 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 71860, -1, 71860 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 71861, -1, 71861 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 71862, -1, 71862 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 71863, -1, 71863 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 71864, -1, 71864 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 71865, -1, 71865 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 71866, -1, 71866 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 71867, -1, 71867 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 71868, -1, 71868 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 71869, -1, 71869 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 71870, -1, 71870 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 71871, -1, 71871 },
    { SFCE_UNICODE_CATEGORY_MN, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 9, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 93792, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 93793, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 93794, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 93795, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 93796, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 93797, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 93798, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 93799, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 93800, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 93801, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 93802, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 93803, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 93804, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 93805, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 93806, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 93807, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 93808, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 93809, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 93810, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 93811, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 93812, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 93813, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 93814, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 93815, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 93816, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 93817, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 93818, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 93819, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 93820, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 93821, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 93822, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 93823, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 93760, -1, 93760 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 93761, -1, 93761 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 93762, -1, 93762 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 93763, -1, 93763 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 93764, -1, 93764 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 93765, -1, 93765 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 93766, -1, 93766 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 93767, -1, 93767 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 93768, -1, 93768 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 93769, -1, 93769 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 93770, -1, 93770 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 93771, -1, 93771 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 93772, -1, 93772 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 93773, -1, 93773 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 93774, -1, 93774 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 93775, -1, 93775 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 93776, -1, 93776 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 93777, -1, 93777 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 93778, -1, 93778 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 93779, -1, 93779 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 93780, -1, 93780 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 93781, -1, 93781 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 93782, -1, 93782 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 93783, -1, 93783 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 93784, -1, 93784 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 93785, -1, 93785 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 93786, -1, 93786 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 93787, -1, 93787 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 93788, -1, 93788 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 93789, -1, 93789 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 93790, -1, 93790 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 93791, -1, 93791 },
    { SFCE_UNICODE_CATEGORY_MN, SFCE_UNICODE_BIDI_CLASS_NSM, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 2, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_MC, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 2, 6, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_FONT, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_ND, SFCE_UNICODE_BIDI_CLASS_EN, SFCE_UNICODE_DECOMPOSITION_FONT, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_MC, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 216, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_MC, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 226, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_NO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 2, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_SM, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_FONT, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 125218, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 125219, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 125220, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 125221, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 125222, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 125223, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 125224, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 125225, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 125226, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 125227, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 125228, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 125229, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 125230, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 125231, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 125232, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 125233, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 125234, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 125235, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 125236, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 125237, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 125238, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 125239, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 125240, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 125241, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 125242, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 125243, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 125244, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 125245, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 125246, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 125247, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 125248, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 125249, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 125250, -1 },
    { SFCE_UNICODE_CATEGORY_LU, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, -1, 125251, -1 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 125184, -1, 125184 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 125185, -1, 125185 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 125186, -1, 125186 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 125187, -1, 125187 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 125188, -1, 125188 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 125189, -1, 125189 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 125190, -1, 125190 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 125191, -1, 125191 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 125192, -1, 125192 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 125193, -1, 125193 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 125194, -1, 125194 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 125195, -1, 125195 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 125196, -1, 125196 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 125197, -1, 125197 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 125198, -1, 125198 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 125199, -1, 125199 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 125200, -1, 125200 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 125201, -1, 125201 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 125202, -1, 125202 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 125203, -1, 125203 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 125204, -1, 125204 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 125205, -1, 125205 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 125206, -1, 125206 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 125207, -1, 125207 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 125208, -1, 125208 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 125209, -1, 125209 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 125210, -1, 125210 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 125211, -1, 125211 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 125212, -1, 125212 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 125213, -1, 125213 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 125214, -1, 125214 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 125215, -1, 125215 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 125216, -1, 125216 },
    { SFCE_UNICODE_CATEGORY_LL, SFCE_UNICODE_BIDI_CLASS_R, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 1, 0, 125217, -1, 125217 },
    { SFCE_UNICODE_CATEGORY_LO, SFCE_UNICODE_BIDI_CLASS_AL, SFCE_UNICODE_DECOMPOSITION_FONT, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_CIRCLE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_SO, SFCE_UNICODE_BIDI_CLASS_L, SFCE_UNICODE_DECOMPOSITION_SQUARE, 0, 1, 0, -1, -1, -1 },
    { SFCE_UNICODE_CATEGORY_SK, SFCE_UNICODE_BIDI_CLASS_ON, SFCE_UNICODE_DECOMPOSITION_NONE, 0, 2, 0, -1, -1, -1 },
};

uint16_t utf8_property_indices[37120] = {
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
    67,   68,   69,   8,    15,   9,    15,   0,    0,    0,    0,    0,
    0,    2,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    70,   5,    7,    7,    7,    7,    71,   5,
    72,   71,   73,   74,   15,   75,   71,   72,   76,   77,   78,   78,
    72,   79,   5,    5,    72,   78,   73,   80,   81,   81,   81,   5,
    82,   83,   84,   85,   86,   87,   88,   89,   90,   91,   92,   93,
    94,   95,   96,   97,   98,   99,   100,  101,  102,  103,  104,  15,
    105,  106,  107,  108,  109,  110,  111,  112,  113,  114,  115,  116,
    117,  118,  119,  120,  121,  122,  123,  124,  125,  126,  127,  128,
    129,  130,  131,  132,  133,  134,  135,  15,   136,  137,  138,  139,
    140,  141,  142,  143,  144,  145,  146,  147,  148,  149,  150,  151,
    152,  153,  154,  155,  156,  157,  158,  159,  160,  161,  162,  163,
    164,  165,  166,  167,  168,  169,  170,  171,  172,  173,  174,  175,
    176,  177,  178,  179,  180,  181,  182,  183,  184,  185,  186,  187,
    188,  189,  190,  191,  24,   52,   192,  193,  194,  195,  196,  197,
    112,  198,  199,  200,  201,  202,  203,  204,  205,  206,  207,  208,
    209,  210,  211,  212,  213,  214,  215,  216,  217,  218,  219,  220,
    221,  222,  223,  224,  225,  226,  227,  228,  229,  230,  231,  232,
    233,  234,  235,  236,  237,  238,  239,  240,  241,  242,  243,  244,
    245,  246,  247,  248,  249,  250,  251,  252,  253,  254,  255,  256,
    257,  258,  259,  260,  261,  262,  263,  264,  265,  266,  267,  268,
    269,  270,  271,  272,  273,  274,  275,  276,  277,  278,  279,  280,
    281,  112,  282,  283,  284,  285,  286,  287,  288,  289,  290,  291,
    292,  293,  294,  295,  296,  297,  298,  299,  300,  301,  302,  303,
    304,  305,  306,  307,  308,  309,  112,  112,  310,  311,  312,  313,
    314,  315,  316,  317,  318,  319,  320,  321,  322,  323,  112,  324,
    325,  326,  112,  327,  324,  324,  324,  324,  328,  329,  330,  331,
    332,  333,  334,  335,  336,  337,  338,  339,  340,  341,  342,  343,
    344,  345,  346,  347,  348,  349,  350,  351,  352,  353,  354,  355,
    356,  357,  358,  359,  360,  361,  362,  363,  364,  365,  366,  367,
    368,  369,  370,  371,  112,  372,  373,  374,  375,  376,  377,  378,
    379,  380,  381,  382,  383,  384,  385,  386,  387,  388,  389,  390,
    391,  392,  393,  394,  395,  396,  397,  398,  399,  400,  401,  402,
    403,  404,  405,  406,  407,  408,  409,  410,  411,  412,  413,  414,
    415,  416,  417,  418,  419,  112,  420,  421,  422,  423,  424,  425,
    426,  427,  428,  429,  430,  431,  432,  433,  434,  435,  436,  437,
    112,  112,  112,  112,  112,  112,  438,  439,  440,  441,  442,  443,
    444,  445,  446,  447,  448,  449,  450,  451,  452,  453,  454,  455,
    456,  457,  458,  459,  460,  461,  462,  463,  464,  112,  465,  466,
    112,  467,  112,  468,  469,  112,  112,  112,  470,  471,  112,  472,
    473,  474,  475,  112,  476,  477,  478,  479,  480,  112,  112,  481,
    112,  482,  483,  112,  112,  484,  112,  112,  112,  112,  112,  112,
    112,  485,  112,  112,  486,  112,  487,  488,  112,  112,  112,  489,
    490,  491,  492,  493,  494,  112,  112,  112,  112,  112,  495,  112,
    324,  112,  112,  112,  112,  112,  112,  112,  112,  496,  497,  112,
    112,  112,  112,  112,  112,  112,  112,  112,  112,  112,  112,  112,
    112,  112,  112,  112,  498,  498,  498,  498,  498,  498,  498,  498,
    498,  499,  499,  500,  500,  500,  500,  500,  500,  500,  42,   42,
    42,   42,   499,  499,  499,  499,  499,  499,  499,  499,  499,  499,
    500,  500,  42,   42,   42,   42,   42,   42,   72,   72,   72,   72,
    72,   72,   42,   42,   498,  498,  498,  498,  498,  42,   42,   42,
    42,   42,   42,   42,   499,  42,   500,  42,   42,   42,   42,   42,
    42,   42,   42,   42,   42,   42,   42,   42,   42,   42,   42,   42,
    501,  501,  501,  501,  501,  501,  501,  501,  501,  501,  501,  501,
    501,  501,  501,  501,  501,  501,  501,  501,  501,  502,  503,  503,
    503,  503,  502,  504,  503,  503,  503,  503,  503,  505,  505,  503,
    503,  503,  503,  505,  505,  503,  503,  503,  503,  503,  503,  503,
    503,  503,  503,  503,  506,  506,  506,  506,  506,  503,  503,  503,
    503,  501,  501,  501,  501,  501,  501,  501,  501,  507,  501,  503,
    503,  503,  501,  501,  501,  503,  503,  508,  501,  501,  501,  503,
    503,  503,  503,  501,  502,  503,  503,  501,  509,  510,  510,  509,
    510,  510,  509,  501,  501,  501,  501,  501,  501,  501,  501,  501,
    501,  501,  501,  501,  511,  512,  513,  514,  499,  42,   515,  516,
    517,  517,  518,  519,  520,  521,  5,    522,  517,  517,  517,  517,
    72,   42,   523,  5,    524,  525,  526,  517,  527,  517,  528,  529,
    112,  530,  531,  532,  533,  534,  535,  536,  537,  538,  539,  540,
    541,  542,  543,  544,  545,  546,  517,  547,  548,  549,  550,  551,
    552,  553,  554,  555,  556,  557,  558,  559,  112,  560,  561,  562,
    563,  564,  565,  566,  567,  568,  569,  570,  571,  572,  573,  574,
    575,  576,  577,  577,  578,  579,  580,  581,  582,  583,  584,  585,
    586,  587,  588,  589,  590,  591,  592,  593,  593,  594,  595,  596,
    597,  598,  599,  600,  601,  602,  603,  604,  605,  606,  607,  608,
    609,  610,  611,  612,  613,  614,  615,  616,  617,  618,  619,  620,
    621,  622,  623,  624,  625,  626,  15,   627,  628,  629,  630,  631,
    112,  632,  633,  634,  635,  636,  637,  638,  639,  640,  641,  642,
    643,  644,  645,  646,  647,  648,  649,  650,  651,  652,  653,  654,
    655,  656,  657,  658,  659,  660,  661,  662,  663,  664,  665,  666,
    667,  668,  669,  670,  671,  672,  673,  674,  675,  676,  677,  678,
    679,  680,  681,  682,  683,  684,  685,  686,  687,  688,  689,  690,
    691,  692,  693,  694,  695,  696,  697,  698,  699,  700,  701,  702,
    703,  704,  705,  706,  707,  708,  709,  710,  711,  712,  713,  714,
    715,  716,  717,  718,  719,  720,  721,  722,  723,  724,  725,  726,
    727,  728,  729,  730,  731,  732,  733,  734,  735,  736,  737,  738,
    739,  740,  741,  742,  743,  744,  745,  746,  747,  748,  749,  750,
    751,  752,  753,  754,  755,  756,  757,  758,  759,  760,  761,  762,
    763,  764,  765,  501,  501,  501,  501,  501,  766,  766,  767,  768,
    769,  770,  771,  772,  773,  774,  775,  776,  777,  778,  779,  780,
    781,  782,  783,  784,  785,  786,  787,  788,  789,  790,  791,  792,
    793,  794,  795,  796,  797,  798,  799,  800,  801,  802,  803,  804,
    805,  806,  807,  808,  809,  810,  811,  812,  813,  814,  815,  816,
    817,  818,  819,  820,  821,  822,  823,  824,  825,  826,  827,  828,
    829,  830,  831,  832,  833,  834,  835,  836,  837,  838,  839,  840,
    841,  842,  843,  844,  845,  846,  847,  848,  849,  850,  851,  852,
    853,  854,  855,  856,  857,  858,  859,  860,  861,  862,  863,  864,
    865,  866,  867,  868,  869,  870,  871,  872,  873,  874,  875,  876,
    877,  878,  879,  880,  881,  882,  883,  884,  885,  886,  887,  888,
    889,  890,  891,  892,  893,  894,  895,  896,  897,  898,  899,  900,
    901,  902,  903,  904,  905,  906,  907,  908,  909,  910,  911,  912,
    913,  914,  915,  916,  917,  918,  919,  920,  921,  922,  923,  924,
    925,  926,  927,  928,  929,  930,  931,  932,  517,  933,  934,  935,
    936,  937,  938,  939,  940,  941,  942,  943,  944,  945,  946,  947,
    948,  949,  950,  951,  952,  953,  954,  955,  956,  957,  958,  959,
    960,  961,  962,  963,  964,  965,  966,  967,  968,  969,  970,  517,
    517,  500,  971,  971,  971,  971,  971,  971,  112,  972,  973,  974,
    975,  976,  977,  978,  979,  980,  981,  982,  983,  984,  985,  986,
    987,  988,  989,  990,  991,  992,  993,  994,  995,  996,  997,  998,
    999,  1000, 1001, 1002, 1003, 1004, 1005, 1006, 1007, 1008, 1009, 214,
    112,  971,  1010, 517,  517,  71,   71,   7,    517,  503,  501,  501,
    501,  501,  503,  501,  501,  501,  1011, 503,  501,  501,  501,  501,
    501,  501,  503,  503,  503,  503,  503,  503,  501,  501,  503,  501,
    501,  1011, 1012, 501,  1013, 1014, 1015, 1016, 1017, 1018, 1019, 1020,
    1021, 1022, 1022, 1023, 1024, 1025, 1026, 1027, 1028, 1029, 1030, 1028,
    501,  503,  1028, 1021, 517,  517,  517,  517,  517,  517,  517,  517,
    1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031,
    1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031,
    1031, 1031, 1031, 517,  517,  517,  517,  1031, 1031, 1031, 1031, 1028,
    1028, 517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    1032, 1032, 1032, 1032, 1032, 1032, 15,   15,   1033, 6,    6,    1034,
    11,   1035, 71,   71,   501,  501,  501,  501,  501,  501,  501,  501,
    1036, 1037, 1038, 1035, 1039, 1035, 1035, 1035, 1040, 1040, 1040, 1040,
    1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040,
    1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040,
    1040, 1040, 1040, 1040, 1041, 1040, 1040, 1040, 1040, 1040, 1040, 1040,
    1040, 1040, 1040, 1042, 1043, 1044, 1036, 1037, 1038, 1045, 1046, 501,
    501,  503,  503,  501,  501,  501,  501,  501,  503,  501,  501,  503,
    1047, 1047, 1047, 1047, 1047, 1047, 1047, 1047, 1047, 1047, 6,    1048,
    1048, 1035, 1040, 1040, 1049, 1040, 1040, 1040, 1040, 1050, 1050, 1050,
    1050, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040,
    1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040,
    1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040,
    1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040,
    1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040,
    1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040,
    1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040,
    1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1035, 1040, 501,  501,
    501,  501,  501,  501,  501,  1032, 71,   501,  501,  501,  501,  503,
    501,  1041, 1041, 501,  501,  71,   503,  501,  501,  503,  1040, 1040,
    13,   13,   13,   13,   13,   13,   13,   13,   13,   13,   1040, 1040,
    1040, 1051, 1051, 1040, 1035, 1035, 1035, 1035, 1035, 1035, 1035, 1035,
    1035, 1035, 1035, 1035, 1035, 1035, 517,  1039, 1040, 1052, 1040, 1040,
    1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040,
    1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040,
    1040, 1040, 1040, 1040, 501,  503,  501,  501,  503,  501,  501,  503,
    503,  503,  501,  503,  503,  501,  503,  501,  501,  501,  503,  501,
    503,  501,  503,  501,  503,  501,  501,  517,  517,  1040, 1040, 1040,
    1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040,
    1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040,
    1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040,
    1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040,
    1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040,
    1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040,
    1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040,
    1040, 1040, 508,  508,  508,  508,  508,  508,  508,  508,  508,  508,
    508,  1040, 517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  1053, 1053, 1053, 1053, 1053, 1053, 1053, 1053,
    1053, 1053, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031,
    1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031,
    1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 501,
    501,  501,  501,  501,  501,  501,  503,  501,  1054, 1054, 71,   5,
    5,    5,    1054, 517,  517,  503,  1055, 1055, 1031, 1031, 1031, 1031,
    1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031,
    1031, 1031, 1031, 1031, 1031, 1031, 501,  501,  501,  501,  1054, 501,
    501,  501,  501,  501,  501,  501,  501,  501,  1054, 501,  501,  501,
    1054, 501,  501,  501,  501,  501,  517,  517,  1028, 1028, 1028, 1028,
    1028, 1028, 1028, 1028, 1028, 1028, 1028, 1028, 1028, 1028, 1028, 517,
    1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031,
    1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031,
    1031, 503,  503,  503,  517,  517,  1028, 517,  1040, 1040, 1040, 1040,
    1040, 1040, 1040, 1040, 1040, 1040, 1040, 517,  517,  517,  517,  517,
    1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040,
    1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040,
    1056, 1040, 1040, 1040, 1040, 1040, 1040, 517,  1032, 1032, 517,  517,
    517,  517,  517,  501,  501,  503,  503,  503,  501,  501,  501,  501,
    1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040,
    1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040,
    1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040,
    1040, 1040, 1040, 1040, 1040, 1041, 501,  501,  501,  501,  501,  503,
    503,  503,  503,  503,  501,  501,  501,  501,  501,  501,  501,  501,
    501,  501,  501,  501,  501,  501,  1032, 503,  501,  501,  503,  501,
    501,  503,  501,  501,  501,  503,  503,  503,  1042, 1043, 1044, 501,
    501,  501,  503,  501,  501,  503,  503,  501,  501,  501,  501,  501,
    508,  508,  508,  1057, 324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  508,  1057,
    1058, 324,  1057, 1057, 1057, 508,  508,  508,  508,  508,  508,  508,
    508,  1057, 1057, 1057, 1057, 1059, 1057, 1057, 324,  501,  503,  501,
    501,  508,  508,  508,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  508,  508,  971,  971,  1060, 1060, 1060, 1060, 1060, 1060,
    1060, 1060, 1060, 1060, 971,  500,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  508,  1057, 1057,
    517,  324,  324,  324,  324,  324,  324,  324,  324,  517,  517,  324,
    324,  517,  517,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  517,  324,  324,  324,  324,  324,  324,  324,  517,  324,  517,
    517,  517,  324,  324,  324,  324,  517,  517,  1058, 324,  1057, 1057,
    1057, 508,  508,  508,  508,  517,  517,  1057, 1057, 517,  517,  1057,
    1057, 1059, 324,  517,  517,  517,  517,  517,  517,  517,  517,  1057,
    517,  517,  517,  517,  324,  324,  517,  324,  324,  324,  508,  508,
    517,  517,  1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060,
    324,  324,  7,    7,    1061, 1061, 1061, 1061, 1061, 1061, 765,  7,
    324,  971,  501,  517,  517,  508,  508,  1057, 517,  324,  324,  324,
    324,  324,  324,  517,  517,  517,  517,  324,  324,  517,  517,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  517,  324,  324,
    324,  324,  324,  324,  324,  517,  324,  324,  517,  324,  324,  517,
    324,  324,  517,  517,  1058, 517,  1057, 1057, 1057, 508,  508,  517,
    517,  517,  517,  508,  508,  517,  517,  508,  508,  1059, 517,  517,
    517,  508,  517,  517,  517,  517,  517,  517,  517,  324,  324,  324,
    324,  517,  324,  517,  517,  517,  517,  517,  517,  517,  1060, 1060,
    1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060, 508,  508,  324,  324,
    324,  508,  971,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  508,  508,  1057, 517,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  517,  324,  324,  324,  517,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  517,  324,  324,  324,  324,  324,  324,
    324,  517,  324,  324,  517,  324,  324,  324,  324,  324,  517,  517,
    1058, 324,  1057, 1057, 1057, 508,  508,  508,  508,  508,  517,  508,
    508,  1057, 517,  1057, 1057, 1059, 517,  517,  324,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    324,  324,  508,  508,  517,  517,  1060, 1060, 1060, 1060, 1060, 1060,
    1060, 1060, 1060, 1060, 971,  7,    517,  517,  517,  517,  517,  517,
    517,  324,  508,  508,  508,  508,  508,  508,  517,  508,  1057, 1057,
    517,  324,  324,  324,  324,  324,  324,  324,  324,  517,  517,  324,
    324,  517,  517,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  517,  324,  324,  324,  324,  324,  324,  324,  517,  324,  324,
    517,  324,  324,  324,  324,  324,  517,  517,  1058, 324,  1057, 508,
    1057, 508,  508,  508,  508,  517,  517,  1057, 1057, 517,  517,  1057,
    1057, 1059, 517,  517,  517,  517,  517,  517,  517,  508,  508,  1057,
    517,  517,  517,  517,  324,  324,  517,  324,  324,  324,  508,  508,
    517,  517,  1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060,
    765,  324,  1061, 1061, 1061, 1061, 1061, 1061, 517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  508,  324,  517,  324,  324,  324,
    324,  324,  324,  517,  517,  517,  324,  324,  324,  517,  324,  324,
    324,  324,  517,  517,  517,  324,  324,  517,  324,  517,  324,  324,
    517,  517,  517,  324,  324,  517,  517,  517,  324,  324,  324,  517,
    517,  517,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  517,  517,  517,  517,  1057, 1057, 508,  1057, 1057, 517,
    517,  517,  1057, 1057, 1057, 517,  1057, 1057, 1057, 1059, 517,  517,
    324,  517,  517,  517,  517,  517,  517,  1057, 517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  1060, 1060,
    1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060, 1061, 1061, 1061, 71,
    71,   71,   71,   71,   71,   7,    71,   517,  517,  517,  517,  517,
    508,  1057, 1057, 1057, 508,  324,  324,  324,  324,  324,  324,  324,
    324,  517,  324,  324,  324,  517,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  517,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  517,  517,
    1058, 324,  508,  508,  508,  1057, 1057, 1057, 1057, 517,  508,  508,
    508,  517,  508,  508,  508,  1059, 517,  517,  517,  517,  517,  517,
    517,  1062, 1063, 517,  324,  324,  324,  517,  517,  324,  517,  517,
    324,  324,  508,  508,  517,  517,  1060, 1060, 1060, 1060, 1060, 1060,
    1060, 1060, 1060, 1060, 517,  517,  517,  517,  517,  517,  517,  971,
    1064, 1064, 1064, 1064, 1064, 1064, 1064, 765,  324,  508,  1057, 1057,
    971,  324,  324,  324,  324,  324,  324,  324,  324,  517,  324,  324,
    324,  517,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  517,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    517,  324,  324,  324,  324,  324,  517,  517,  1058, 324,  1057, 1065,
    1057, 1057, 1057, 1057, 1057, 517,  1065, 1057, 1057, 517,  1057, 1057,
    508,  1059, 517,  517,  517,  517,  517,  517,  517,  1057, 1057, 517,
    517,  517,  517,  517,  517,  324,  324,  517,  324,  324,  508,  508,
    517,  517,  1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060,
    517,  324,  324,  1057, 517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  508,  508,  1057, 1057, 324,  324,  324,  324,
    324,  324,  324,  324,  324,  517,  324,  324,  324,  517,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  1059, 1059, 324,  1057, 1057, 1057, 508,  508,  508,
    508,  517,  1057, 1057, 1057, 517,  1057, 1057, 1057, 1059, 324,  765,
    517,  517,  517,  517,  324,  324,  324,  1057, 1061, 1061, 1061, 1061,
    1061, 1061, 1061, 324,  324,  324,  508,  508,  517,  517,  1060, 1060,
    1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060, 1061, 1061, 1061, 1061,
    1061, 1061, 1061, 1061, 1061, 765,  324,  324,  324,  324,  324,  324,
    517,  508,  1057, 1057, 517,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  517,
    517,  517,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  517,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    517,  324,  517,  517,  324,  324,  324,  324,  324,  324,  324,  517,
    517,  517,  1059, 517,  517,  517,  517,  1057, 1057, 1057, 508,  508,
    508,  517,  508,  517,  1057, 1057, 1057, 1057, 1057, 1057, 1057, 1057,
    517,  517,  517,  517,  517,  517,  1060, 1060, 1060, 1060, 1060, 1060,
    1060, 1060, 1060, 1060, 517,  517,  1057, 1057, 971,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  508,  324,  1066,
    508,  508,  508,  508,  1067, 1067, 1059, 517,  517,  517,  517,  7,
    324,  324,  324,  324,  324,  324,  500,  508,  1068, 1068, 1068, 1068,
    508,  508,  508,  971,  1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060,
    1060, 1060, 971,  971,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  324,  324,  517,  324,  517,  324,  324,
    324,  324,  324,  517,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  517,  324,  517,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  508,  324,  1066, 508,  508,  508,  508,
    1069, 1069, 1059, 508,  508,  324,  517,  517,  324,  324,  324,  324,
    324,  517,  500,  517,  1070, 1070, 1070, 1070, 508,  508,  508,  517,
    1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060, 517,  517,
    1066, 1066, 324,  324,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    324,  765,  765,  765,  971,  971,  971,  971,  971,  971,  971,  971,
    1071, 971,  971,  971,  971,  971,  971,  765,  971,  765,  765,  765,
    503,  503,  765,  765,  765,  765,  765,  765,  1060, 1060, 1060, 1060,
    1060, 1060, 1060, 1060, 1060, 1060, 1061, 1061, 1061, 1061, 1061, 1061,
    1061, 1061, 1061, 1061, 765,  503,  765,  503,  765,  504,  8,    9,
    8,    9,    1057, 1057, 324,  324,  324,  324,  324,  324,  324,  324,
    517,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  517,  517,  517,  517,  1072, 1073, 508,  1074, 508,  508,  1075,
    508,  1075, 1073, 1073, 1073, 1073, 508,  1057, 1073, 508,  501,  501,
    1059, 971,  501,  501,  324,  324,  324,  324,  324,  508,  508,  508,
    508,  508,  508,  508,  508,  508,  508,  508,  517,  508,  508,  508,
    508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,
    508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,
    508,  508,  508,  508,  508,  508,  508,  508,  508,  517,  765,  765,
    765,  765,  765,  765,  765,  765,  503,  765,  765,  765,  765,  765,
    765,  517,  765,  765,  971,  971,  971,  971,  971,  765,  765,  765,
    765,  971,  971,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  1057,
    1057, 508,  508,  508,  508,  1057, 508,  508,  508,  508,  508,  1058,
    1057, 1059, 1059, 1057, 1057, 508,  508,  324,  1060, 1060, 1060, 1060,
    1060, 1060, 1060, 1060, 1060, 1060, 971,  971,  971,  971,  971,  971,
    324,  324,  324,  324,  324,  324,  1057, 1057, 508,  508,  324,  324,
    324,  324,  508,  508,  508,  324,  1057, 1057, 1057, 324,  324,  1057,
    1057, 1057, 1057, 1057, 1057, 1057, 324,  324,  324,  508,  508,  508,
    508,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  508,  1057, 1057, 508,  508,  1057, 1057, 1057, 1057, 1057,
    1057, 503,  324,  1057, 1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060,
    1060, 1060, 1057, 1057, 1057, 508,  765,  765,  1076, 1077, 1078, 1079,
    1080, 1081, 1082, 1083, 1084, 1085, 1086, 1087, 1088, 1089, 1090, 1091,
    1092, 1093, 1094, 1095, 1096, 1097, 1098, 1099, 1100, 1101, 1102, 1103,
    1104, 1105, 1106, 1107, 1108, 1109, 1110, 1111, 1112, 1113, 517,  1114,
    517,  517,  517,  517,  517,  1115, 517,  517,  1116, 1117, 1118, 1119,
    1120, 1121, 1122, 1123, 1124, 1125, 1126, 1127, 1128, 1129, 1130, 1131,
    1132, 1133, 1134, 1135, 1136, 1137, 1138, 1139, 1140, 1141, 1142, 1143,
    1144, 1145, 1146, 1147, 1148, 1149, 1150, 1151, 1152, 1153, 1154, 1155,
    1156, 1157, 1158, 971,  498,  1159, 1160, 1161, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  517,  324,  324,  324,  324,  517,  517,  324,  324,  324,  324,
    324,  324,  324,  517,  324,  517,  324,  324,  324,  324,  517,  517,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  517,  324,  324,  324,  324,  517,  517,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  517,  324,  324,
    324,  324,  517,  517,  324,  324,  324,  324,  324,  324,  324,  517,
    324,  517,  324,  324,  324,  324,  517,  517,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  517,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  517,  324,  324,
    324,  324,  517,  517,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  517,
    517,  501,  501,  501,  971,  971,  971,  971,  971,  971,  971,  971,
    971,  1061, 1061, 1061, 1061, 1061, 1061, 1061, 1061, 1061, 1061, 1061,
    1061, 1061, 1061, 1061, 1061, 1061, 1061, 1061, 1061, 517,  517,  517,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   517,  517,  517,  517,  517,  517,  1163, 1164, 1165, 1166,
    1167, 1168, 1169, 1170, 1171, 1172, 1173, 1174, 1175, 1176, 1177, 1178,
    1179, 1180, 1181, 1182, 1183, 1184, 1185, 1186, 1187, 1188, 1189, 1190,
    1191, 1192, 1193, 1194, 1195, 1196, 1197, 1198, 1199, 1200, 1201, 1202,
    1203, 1204, 1205, 1206, 1207, 1208, 1209, 1210, 1211, 1212, 1213, 1214,
    1215, 1216, 1217, 1218, 1219, 1220, 1221, 1222, 1223, 1224, 1225, 1226,
    1227, 1228, 1229, 1230, 1231, 1232, 1233, 1234, 1235, 1236, 1237, 1238,
    1239, 1240, 1241, 1242, 1243, 1244, 1245, 1246, 1247, 1248, 517,  517,
    1249, 1250, 1251, 1252, 1253, 1254, 517,  517,  1010, 324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  765,  971,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    4,    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  8,    9,    517,  517,  517,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  971,
    971,  971,  1255, 1255, 1255, 324,  324,  324,  324,  324,  324,  324,
    324,  517,  517,  517,  517,  517,  517,  517,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  508,  508,  1059, 1256, 517,  517,  517,  517,  517,  517,
    517,  517,  517,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  508,  508,
    1256, 971,  971,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  508,  508,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  517,  324,  324,
    324,  517,  508,  508,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  508,  508,  1057, 508,
    508,  508,  508,  508,  508,  508,  1057, 1057, 1057, 1057, 1057, 1057,
    1057, 1057, 508,  1057, 1057, 508,  508,  508,  508,  508,  508,  508,
    508,  508,  1059, 508,  971,  971,  971,  500,  971,  971,  971,  7,
    324,  501,  517,  517,  1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060,
    1060, 1060, 517,  517,  517,  517,  517,  517,  1064, 1064, 1064, 1064,
    1064, 1064, 1064, 1064, 1064, 1064, 517,  517,  517,  517,  517,  517,
    5,    5,    5,    5,    5,    5,    1010, 5,    5,    5,    5,    508,
    508,  508,  75,   508,  1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060,
    1060, 1060, 517,  517,  517,  517,  517,  517,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  500,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  517,  517,  517,  517,  517,  517,  517,  324,  324,  324,  324,
    324,  508,  508,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  1012, 324,  517,  517,  517,  517,  517,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  517,
    508,  508,  508,  1057, 1057, 1057, 1057, 508,  508,  1057, 1057, 1057,
    517,  517,  517,  517,  1057, 1057, 508,  1057, 1057, 1057, 1057, 1057,
    1057, 1011, 501,  503,  517,  517,  517,  517,  71,   517,  517,  517,
    5,    5,    1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  517,  517,  324,  324,  324,  324,
    324,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  517,  517,  517,  517,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  517,  517,  517,  517,  517,  517,  1060, 1060, 1060, 1060,
    1060, 1060, 1060, 1060, 1060, 1060, 1061, 517,  517,  517,  71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  501,  503,  1057, 1057, 508,
    517,  517,  971,  971,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  1057, 508,  1057,
    508,  508,  508,  508,  508,  508,  508,  517,  1059, 1057, 508,  1057,
    1057, 508,  508,  508,  508,  508,  508,  508,  508,  1057, 1057, 1057,
    1057, 1057, 1057, 508,  508,  501,  501,  501,  501,  501,  501,  501,
    501,  517,  517,  503,  1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060,
    1060, 1060, 517,  517,  517,  517,  517,  517,  1060, 1060, 1060, 1060,
    1060, 1060, 1060, 1060, 1060, 1060, 517,  517,  517,  517,  517,  517,
    971,  971,  971,  971,  971,  971,  971,  500,  971,  971,  971,  971,
    971,  971,  517,  517,  501,  501,  501,  501,  501,  503,  503,  503,
    503,  503,  503,  501,  501,  503,  766,  503,  503,  501,  501,  503,
    503,  501,  501,  501,  501,  501,  503,  501,  501,  501,  501,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    508,  508,  508,  508,  1057, 324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  1058, 1057, 508,  508,  508,  508,  508,  1057,
    508,  1057, 1057, 1057, 1057, 1057, 508,  1057, 1256, 324,  324,  324,
    324,  324,  324,  324,  324,  517,  971,  971,  1060, 1060, 1060, 1060,
    1060, 1060, 1060, 1060, 1060, 1060, 971,  971,  971,  971,  971,  971,
    971,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  501,
    503,  501,  501,  501,  501,  501,  501,  501,  765,  765,  765,  765,
    765,  765,  765,  765,  765,  971,  971,  971,  508,  508,  1057, 324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  1057, 508,  508,  508,  508,  1057, 1057,
    508,  508,  1256, 1059, 508,  508,  324,  324,  1060, 1060, 1060, 1060,
    1060, 1060, 1060, 1060, 1060, 1060, 324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  1058, 1057, 508,  508,  1057, 1057, 1057, 508,  1057, 508,
    508,  508,  1256, 1256, 517,  517,  517,  517,  517,  517,  517,  517,
    971,  971,  971,  971,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  1057, 1057, 1057, 1057, 1057, 1057, 1057, 1057,
    508,  508,  508,  508,  508,  508,  508,  508,  1057, 1057, 508,  1058,
    517,  517,  517,  971,  971,  971,  971,  971,  1060, 1060, 1060, 1060,
    1060, 1060, 1060, 1060, 1060, 1060, 517,  517,  517,  324,  324,  324,
    1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060, 324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  500,  500,  500,  500,  500,  500,  971,  971,
    685,  687,  697,  700,  701,  701,  709,  734,  1257, 1258, 1259, 517,
    517,  517,  517,  517,  1260, 1261, 1262, 1263, 1264, 1265, 1266, 1267,
    1268, 1269, 1270, 1271, 1272, 1273, 1274, 1275, 1276, 1277, 1278, 1279,
    1280, 1281, 1282, 1283, 1284, 1285, 1286, 1287, 1288, 1289, 1290, 1291,
    1292, 1293, 1294, 1295, 1296, 1297, 1298, 1299, 1300, 1301, 1302, 517,
    517,  1303, 1304, 1305, 971,  971,  971,  971,  971,  971,  971,  971,
    517,  517,  517,  517,  517,  517,  517,  517,  501,  501,  501,  971,
    506,  503,  503,  503,  503,  503,  501,  501,  503,  503,  503,  503,
    501,  1057, 506,  506,  506,  506,  506,  506,  506,  324,  324,  324,
    324,  503,  324,  324,  324,  324,  324,  324,  501,  324,  324,  1057,
    501,  501,  324,  517,  517,  517,  517,  517,  112,  112,  112,  112,
    112,  112,  112,  112,  112,  112,  112,  112,  112,  112,  112,  112,
    112,  112,  112,  112,  112,  112,  112,  112,  112,  112,  112,  112,
    112,  112,  112,  112,  112,  112,  112,  112,  112,  112,  112,  112,
    112,  112,  112,  112,  498,  498,  498,  500,  498,  498,  498,  498,
    498,  498,  498,  498,  498,  498,  498,  500,  498,  498,  498,  498,
    498,  498,  498,  498,  498,  498,  498,  498,  498,  498,  498,  498,
    498,  498,  500,  498,  498,  498,  498,  498,  498,  498,  498,  498,
    498,  498,  498,  498,  498,  498,  498,  498,  498,  498,  1306, 1306,
    1306, 1306, 1306, 1306, 1306, 1306, 1306, 112,  112,  112,  112,  112,
    112,  112,  112,  112,  112,  112,  112,  112,  498,  1307, 112,  112,
    112,  1308, 112,  112,  112,  112,  112,  112,  112,  112,  112,  112,
    112,  112,  112,  112,  112,  112,  1309, 112,  112,  112,  112,  112,
    112,  112,  112,  112,  112,  112,  112,  498,  498,  498,  498,  498,
    498,  498,  498,  498,  498,  498,  498,  498,  498,  498,  498,  498,
    498,  498,  498,  498,  498,  498,  498,  498,  498,  498,  498,  498,
    498,  498,  498,  498,  498,  498,  498,  498,  501,  501,  503,  501,
    501,  501,  501,  501,  501,  501,  503,  501,  501,  510,  1310, 503,
    505,  501,  501,  501,  501,  501,  501,  501,  501,  501,  501,  501,
    501,  501,  501,  501,  501,  501,  501,  501,  501,  501,  501,  501,
    501,  501,  501,  501,  501,  501,  501,  501,  501,  501,  501,  501,
    501,  501,  502,  1012, 1012, 503,  1311, 501,  509,  503,  501,  503,
    1312, 1313, 1314, 1315, 1316, 1317, 1318, 1319, 1320, 1321, 1322, 1323,
    1324, 1325, 1326, 1327, 1328, 1329, 1330, 1331, 1332, 1333, 1334, 1335,
    1336, 1337, 1338, 1339, 1340, 1341, 1342, 1343, 1344, 1345, 1346, 1347,
    1348, 1349, 1350, 1351, 1352, 1353, 1354, 1355, 1356, 1357, 1358, 1359,
    1360, 1361, 1362, 1363, 1364, 1365, 1366, 1367, 1368, 1369, 1370, 1371,
    1372, 1373, 1374, 1375, 1376, 1377, 1378, 1379, 1380, 1381, 1382, 1383,
    1384, 1385, 1386, 1387, 1388, 1389, 1390, 1391, 1392, 1393, 1394, 1395,
    1396, 1397, 1398, 1399, 1400, 1401, 1402, 1403, 1404, 1405, 1406, 1407,
    1408, 1409, 1410, 1411, 1412, 1413, 1414, 1415, 1416, 1417, 1418, 1419,
    1420, 1421, 1422, 1423, 1424, 1425, 1426, 1427, 1428, 1429, 1430, 1431,
    1432, 1433, 1434, 1435, 1436, 1437, 1438, 1439, 1440, 1441, 1442, 1443,
    1444, 1445, 1446, 1447, 1448, 1449, 1450, 1451, 1452, 1453, 1454, 1455,
    1456, 1457, 1458, 1459, 1460, 1461, 112,  112,  112,  112,  214,  1409,
    112,  112,  1462, 112,  1463, 1464, 1465, 1466, 1467, 1468, 1469, 1470,
    1471, 1472, 1473, 1474, 1475, 1476, 1477, 1478, 1479, 1480, 1481, 1482,
    1483, 1484, 1485, 1486, 1487, 1488, 1489, 1490, 1491, 1492, 1493, 1494,
    1495, 1496, 1497, 1498, 1499, 1500, 1501, 1502, 1503, 1504, 1505, 1506,
    1507, 1508, 1509, 1510, 1511, 1512, 1513, 1514, 1515, 1516, 1517, 1518,
    1519, 1520, 1521, 1522, 1523, 1524, 1525, 1526, 1527, 1528, 1529, 1530,
    1531, 1532, 1533, 1534, 1535, 1536, 1537, 1538, 1539, 1540, 1541, 1542,
    1543, 1544, 1545, 1546, 1547, 1548, 1549, 1550, 1551, 1552, 1553, 1554,
    1555, 1556, 1557, 1558, 1559, 1560, 1561, 1562, 1563, 1564, 1565, 1566,
    1567, 1568, 1569, 1570, 1571, 1572, 1573, 1574, 1575, 1576, 1577, 1578,
    1579, 1580, 517,  517,  1581, 1582, 1583, 1584, 1585, 1586, 517,  517,
    1587, 1588, 1589, 1590, 1591, 1592, 1593, 1594, 1595, 1596, 1597, 1598,
    1599, 1600, 1601, 1602, 1603, 1604, 1605, 1606, 1607, 1608, 1609, 1610,
    1611, 1612, 1613, 1614, 1615, 1616, 1617, 1618, 1619, 1620, 1621, 1622,
    1623, 1624, 517,  517,  1625, 1626, 1627, 1628, 1629, 1630, 517,  517,
    112,  1631, 112,  1632, 112,  1633, 112,  1634, 517,  1635, 517,  1636,
    517,  1637, 517,  1638, 1639, 1640, 1641, 1642, 1643, 1644, 1645, 1646,
    1647, 1648, 1649, 1650, 1651, 1652, 1653, 1654, 1655, 1656, 1657, 1658,
    1659, 1660, 1661, 1662, 1663, 1664, 1665, 1666, 1667, 1668, 517,  517,
    1669, 1670, 1671, 1672, 1673, 1674, 1675, 1676, 1677, 1678, 1679, 1680,
    1681, 1682, 1683, 1684, 1685, 1686, 1687, 1688, 1689, 1690, 1691, 1692,
    1693, 1694, 1695, 1696, 1697, 1698, 1699, 1700, 1701, 1702, 1703, 1704,
    1705, 1706, 1707, 1708, 1709, 1710, 1711, 1712, 1713, 1714, 1715, 1716,
    1717, 1718, 112,  1719, 112,  517,  112,  112,  1720, 1721, 1722, 1723,
    1724, 72,   568,  72,   72,   42,   112,  1725, 112,  517,  112,  112,
    1726, 1727, 1728, 1729, 1730, 42,   42,   42,   1731, 1732, 112,  112,
    517,  517,  112,  112,  1733, 1734, 1735, 1736, 517,  42,   42,   42,
    1737, 1738, 112,  112,  112,  1739, 112,  112,  1740, 1741, 1742, 1743,
    1744, 42,   42,   42,   517,  517,  112,  1745, 112,  517,  112,  112,
    1746, 1747, 1748, 1749, 1750, 42,   72,   517,  4,    4,    1751, 1751,
    1751, 1751, 1751, 1752, 1751, 1751, 1751, 75,   75,   75,   1753, 1754,
    1010, 1755, 1010, 1010, 1010, 1010, 5,    1756, 1757, 1758, 1759, 1757,
    1757, 1758, 1759, 1757, 5,    5,    5,    5,    1756, 1756, 1756, 5,
    1760, 1761, 1762, 1763, 1764, 1765, 1766, 70,   6,    6,    6,    1767,
    1767, 5,    1756, 1756, 5,    74,   80,   5,    1756, 5,    1756, 43,
    43,   5,    5,    5,    1768, 8,    9,    1756, 1756, 1756, 5,    5,
    5,    5,    5,    5,    5,    5,    15,   5,    43,   5,    5,    1756,
    5,    5,    5,    5,    5,    5,    5,    1751, 75,   75,   75,   75,
    75,   517,  1769, 1770, 1771, 1772, 75,   75,   75,   75,   75,   75,
    78,   498,  517,  517,  78,   78,   78,   78,   78,   78,   1773, 1773,
    1774, 1775, 1776, 498,  1777, 1777, 1777, 1777, 1777, 1777, 1777, 1777,
    1777, 1777, 1778, 1778, 1779, 1780, 1781, 517,  1306, 1306, 1306, 1306,
    1306, 1306, 1306, 1306, 1306, 1306, 1306, 1306, 1306, 517,  517,  517,
    7,    7,    7,    7,    7,    7,    7,    7,    1782, 7,    7,    7,
    7,    7,    7,    7,    7,    7,    7,    7,    7,    7,    7,    7,
    7,    7,    7,    7,    7,    7,    7,    7,    7,    517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    501,  501,  506,  506,  501,  501,  501,  501,  506,  506,  506,  501,
    501,  766,  766,  766,  766,  501,  766,  766,  766,  506,  506,  501,
    503,  501,  506,  506,  503,  503,  503,  503,  501,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    1783, 1783, 1784, 1783, 71,   1783, 1783, 592,  71,   1783, 1785, 1784,
    1784, 1784, 1785, 1785, 1784, 1784, 1784, 1785, 71,   1784, 1783, 71,
    15,   1784, 1784, 1784, 1784, 1784, 71,   71,   1786, 1783, 1786, 71,
    1784, 71,   553,  71,   1784, 71,   26,   87,   1784, 1784, 76,   1785,
    1784, 1784, 1787, 1784, 1785, 1066, 1066, 1066, 1066, 1785, 71,   1783,
    1785, 1785, 1784, 1784, 1788, 15,   15,   15,   15,   1784, 1785, 1785,
    1785, 1785, 71,   15,   71,   71,   1789, 765,  81,   81,   81,   81,
    81,   81,   81,   81,   81,   81,   81,   81,   81,   81,   81,   81,
    1790, 1791, 1792, 1793, 1794, 1795, 1796, 1797, 1798, 1799, 1800, 1801,
    1802, 1803, 1804, 1805, 1806, 1807, 1808, 1809, 1810, 1811, 1812, 1813,
    1814, 1815, 1816, 1817, 1818, 1819, 1820, 1821, 1255, 1255, 1255, 1822,
    1823, 1255, 1255, 1255, 1255, 81,   71,   71,   517,  517,  517,  517,
    15,   15,   15,   15,   15,   71,   71,   71,   71,   71,   15,   15,
    71,   71,   71,   71,   15,   71,   71,   15,   71,   71,   15,   71,
    71,   71,   71,   71,   71,   71,   15,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   15,   15,   71,   71,   15,   71,   15,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   15,   15,   15,   15,   15,   15,   15,   15,
    15,   15,   15,   15,   15,   14,   14,   14,   14,   15,   15,   15,
    14,   14,   14,   14,   14,   14,   15,   15,   15,   14,   10,   77,
    15,   14,   14,   15,   15,   15,   14,   14,   14,   14,   15,   14,
    14,   14,   14,   15,   14,   15,   14,   15,   15,   15,   15,   14,
    1824, 1824, 14,   1824, 1824, 14,   14,   14,   15,   15,   15,   15,
    15,   14,   15,   14,   14,   14,   14,   14,   14,   14,   14,   14,
    14,   14,   14,   14,   14,   14,   14,   14,   14,   15,   15,   15,
    15,   15,   14,   14,   14,   14,   15,   15,   15,   15,   15,   15,
    15,   15,   15,   14,   14,   15,   14,   15,   14,   14,   14,   14,
    14,   14,   14,   14,   15,   14,   14,   14,   14,   14,   14,   14,
    14,   14,   14,   14,   14,   14,   14,   14,   14,   14,   14,   14,
    14,   14,   14,   14,   14,   14,   14,   14,   14,   14,   14,   14,
    14,   15,   15,   14,   14,   14,   14,   15,   15,   15,   15,   15,
    14,   15,   15,   15,   15,   15,   15,   15,   15,   15,   14,   14,
    15,   15,   14,   14,   14,   14,   14,   14,   14,   14,   14,   14,
    14,   14,   14,   14,   14,   14,   14,   14,   14,   15,   15,   15,
    15,   15,   14,   14,   15,   15,   15,   15,   15,   15,   15,   15,
    15,   14,   14,   14,   14,   14,   15,   15,   14,   14,   15,   15,
    15,   15,   14,   14,   14,   14,   14,   14,   14,   14,   14,   14,
    14,   14,   14,   14,   14,   14,   14,   14,   14,   14,   14,   14,
    14,   14,   15,   15,   14,   14,   14,   14,   14,   14,   14,   14,
    14,   14,   14,   14,   14,   14,   14,   14,   71,   71,   71,   71,
    71,   71,   71,   71,   8,    9,    8,    9,    71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   1825, 1825,
    71,   71,   71,   71,   14,   14,   71,   71,   71,   71,   71,   71,
    71,   1826, 1827, 71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   765,  765,  765,  765,  765,  765,  765,  765,  765,  765,
    765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,
    765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,
    765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,
    765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,
    765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  71,
    15,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   765,  71,   71,   71,   71,   71,   15,   15,   15,   15,   15,
    15,   15,   15,   15,   15,   15,   15,   15,   15,   15,   15,   15,
    15,   15,   15,   15,   15,   15,   15,   15,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    15,   15,   15,   15,   15,   15,   71,   71,   71,   71,   71,   71,
    71,   1825, 1825, 1825, 1825, 71,   71,   71,   1825, 71,   71,   1825,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    1828, 1828, 1828, 1828, 1828, 1828, 1828, 1828, 1828, 1828, 1828, 1828,
    1828, 1828, 1828, 1828, 1828, 1828, 1828, 1828, 1829, 1829, 1829, 1829,
    1829, 1829, 1829, 1829, 1829, 1829, 1829, 1829, 1829, 1829, 1829, 1829,
    1829, 1829, 1829, 1829, 1830, 1830, 1830, 1830, 1830, 1830, 1830, 1830,
    1830, 1830, 1830, 1830, 1830, 1830, 1830, 1830, 1830, 1830, 1830, 1830,
    1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831,
    1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831,
    1831, 1831, 1832, 1833, 1834, 1835, 1836, 1837, 1838, 1839, 1840, 1841,
    1842, 1843, 1844, 1845, 1846, 1847, 1848, 1849, 1850, 1851, 1852, 1853,
    1854, 1855, 1856, 1857, 1858, 1859, 1860, 1861, 1862, 1863, 1864, 1865,
    1866, 1867, 1868, 1869, 1870, 1871, 1872, 1873, 1874, 1875, 1876, 1877,
    1878, 1879, 1880, 1881, 1882, 1883, 1828, 1064, 1064, 1064, 1064, 1064,
    1064, 1064, 1064, 1064, 1064, 1064, 1064, 1064, 1064, 1064, 1064, 1064,
    1064, 1064, 1064, 1064, 71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   15,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   15,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    15,   15,   15,   15,   15,   1884, 1884, 15,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   1825, 1825, 71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   15,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   1825, 71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   1825, 1825, 1825, 1825, 1825, 1825, 71,   71,   71,   1825,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   1825, 71,   71,   71,   71,   71,   71,   71,   71,   1825, 1825,
    765,  71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   1825, 1825, 71,   71,   71,   71,   71,
    1825, 1825, 71,   71,   71,   71,   71,   71,   71,   71,   1825, 71,
    71,   71,   71,   71,   1825, 71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   1825, 71,   71,   71,   71,   71,   71,   71,   1825, 1825,
    71,   1825, 71,   71,   71,   71,   1825, 71,   71,   1825, 71,   71,
    71,   71,   71,   71,   71,   1825, 71,   71,   71,   71,   1825, 1825,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   1825, 71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   1825, 71,   1825, 71,   71,   71,   71,   1825,
    1825, 1825, 71,   1825, 71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   8,    9,    8,    9,
    8,    9,    8,    9,    8,    9,    8,    9,    8,    9,    1064, 1064,
    1064, 1064, 1064, 1064, 1064, 1064, 1064, 1064, 1064, 1064, 1064, 1064,
    1064, 1064, 1064, 1064, 1064, 1064, 1064, 1064, 1064, 1064, 1064, 1064,
    1064, 1064, 1064, 1064, 71,   1825, 1825, 1825, 71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   1825, 71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   1825,
    14,   15,   15,   14,   14,   8,    9,    15,   14,   14,   15,   14,
    14,   14,   15,   15,   15,   15,   15,   14,   14,   14,   14,   15,
    15,   15,   15,   15,   14,   14,   14,   15,   15,   15,   14,   14,
    14,   14,   8,    9,    8,    9,    8,    9,    8,    9,    8,    9,
    15,   15,   15,   15,   15,   15,   15,   15,   15,   15,   15,   15,
    15,   15,   15,   15,   765,  765,  765,  765,  765,  765,  765,  765,
    765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,
    765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,
    765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,
    765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,
    765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,
    765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,
    765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,
    765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,
    765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,
    765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,
    15,   15,   15,   15,   15,   15,   15,   15,   15,   15,   15,   15,
    15,   15,   15,   15,   15,   15,   15,   15,   15,   15,   15,   15,
    15,   15,   15,   15,   15,   15,   15,   15,   15,   15,   15,   15,
    15,   15,   15,   15,   15,   15,   15,   15,   15,   15,   15,   15,
    15,   15,   15,   15,   15,   15,   15,   15,   15,   15,   15,   15,
    15,   15,   15,   15,   15,   15,   15,   15,   15,   15,   15,   15,
    15,   15,   15,   15,   15,   15,   15,   15,   15,   15,   15,   15,
    15,   15,   15,   15,   15,   15,   15,   15,   15,   15,   15,   15,
    15,   15,   15,   15,   15,   15,   15,   15,   15,   15,   15,   15,
    15,   15,   15,   15,   15,   15,   15,   15,   15,   15,   15,   15,
    15,   15,   15,   15,   15,   15,   15,   15,   15,   15,   15,   8,
    9,    8,    9,    8,    9,    8,    9,    8,    9,    8,    9,    8,
    9,    8,    9,    8,    9,    8,    9,    8,    9,    15,   15,   14,
    14,   14,   14,   14,   14,   15,   14,   14,   14,   14,   14,   14,
    14,   14,   14,   14,   14,   14,   14,   14,   15,   15,   15,   15,
    15,   15,   15,   15,   14,   15,   15,   15,   15,   15,   15,   15,
    14,   14,   14,   14,   14,   14,   15,   15,   15,   14,   15,   15,
    15,   15,   14,   14,   14,   14,   14,   15,   14,   14,   15,   15,
    8,    9,    8,    9,    14,   15,   15,   15,   15,   14,   15,   14,
    14,   14,   15,   15,   14,   14,   15,   15,   15,   15,   15,   15,
    15,   15,   15,   15,   14,   14,   14,   14,   14,   14,   15,   15,
    8,    9,    15,   15,   15,   15,   15,   15,   15,   15,   15,   15,
    15,   15,   14,   14,   1824, 14,   14,   14,   14,   14,   14,   14,
    14,   14,   14,   14,   14,   14,   14,   14,   14,   15,   14,   14,
    14,   14,   15,   15,   14,   15,   14,   15,   15,   14,   15,   14,
    14,   14,   14,   15,   15,   15,   15,   15,   14,   14,   15,   15,
    15,   15,   15,   15,   14,   14,   14,   15,   15,   15,   15,   15,
    15,   15,   15,   15,   15,   15,   15,   15,   15,   15,   15,   15,
    15,   15,   15,   15,   15,   15,   15,   14,   14,   15,   15,   15,
    15,   15,   15,   15,   15,   15,   15,   15,   14,   14,   15,   15,
    15,   15,   14,   14,   14,   14,   15,   14,   14,   15,   15,   14,
    1824, 1885, 1885, 15,   15,   14,   14,   14,   14,   14,   14,   14,
    14,   14,   14,   14,   14,   14,   14,   14,   14,   14,   14,   14,
    14,   14,   14,   14,   14,   14,   14,   14,   14,   14,   14,   14,
    14,   14,   14,   14,   14,   14,   14,   14,   14,   14,   14,   14,
    15,   15,   14,   14,   14,   14,   14,   14,   14,   14,   15,   14,
    14,   14,   14,   14,   14,   14,   14,   14,   14,   14,   14,   14,
    14,   14,   14,   14,   14,   14,   14,   14,   14,   14,   14,   14,
    14,   14,   14,   14,   14,   14,   14,   14,   14,   14,   14,   14,
    14,   14,   14,   15,   15,   15,   15,   15,   14,   15,   14,   15,
    15,   15,   14,   14,   14,   14,   14,   15,   15,   15,   15,   15,
    14,   14,   14,   15,   15,   15,   15,   14,   15,   15,   15,   14,
    14,   14,   14,   14,   15,   14,   15,   15,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   1825,
    1825, 71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   15,   15,   15,   15,
    15,   15,   15,   15,   15,   15,   15,   15,   15,   15,   15,   15,
    15,   15,   15,   15,   15,   71,   71,   15,   15,   15,   15,   15,
    15,   71,   71,   71,   1825, 71,   71,   71,   71,   1825, 71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   517,  517,  71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   517,  71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   1886, 71,
    1887, 1888, 1889, 1890, 1891, 1892, 1893, 1894, 1895, 1896, 1897, 1898,
    1899, 1900, 1901, 1902, 1903, 1904, 1905, 1906, 1907, 1908, 1909, 1910,
    1911, 1912, 1913, 1914, 1915, 1916, 1917, 1918, 1919, 1920, 1921, 1922,
    1923, 1924, 1925, 1926, 1927, 1928, 1929, 1930, 1931, 1932, 1933, 1934,
    1935, 1936, 1937, 1938, 1939, 1940, 1941, 1942, 1943, 1944, 1945, 1946,
    1947, 1948, 1949, 1950, 1951, 1952, 1953, 1954, 1955, 1956, 1957, 1958,
    1959, 1960, 1961, 1962, 1963, 1964, 1965, 1966, 1967, 1968, 1969, 1970,
    1971, 1972, 1973, 1974, 1975, 1976, 1977, 1978, 1979, 1980, 1981, 1982,
    1983, 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991, 1992, 1993, 1994,
    1995, 1996, 1997, 1998, 1999, 112,  2000, 2001, 112,  2002, 2003, 112,
    112,  112,  112,  112,  1306, 498,  2004, 2005, 2006, 2007, 2008, 2009,
    2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021,
    2022, 2023, 2024, 2025, 2026, 2027, 2028, 2029, 2030, 2031, 2032, 2033,
    2034, 2035, 2036, 2037, 2038, 2039, 2040, 2041, 2042, 2043, 2044, 2045,
    2046, 2047, 2048, 2049, 2050, 2051, 2052, 2053, 2054, 2055, 2056, 2057,
    2058, 2059, 2060, 2061, 2062, 2063, 2064, 2065, 2066, 2067, 2068, 2069,
    2070, 2071, 2072, 2073, 2074, 2075, 2076, 2077, 2078, 2079, 2080, 2081,
    2082, 2083, 2084, 2085, 2086, 2087, 2088, 2089, 2090, 2091, 2092, 2093,
    2094, 2095, 2096, 2097, 2098, 2099, 2100, 2101, 2102, 2103, 2104, 2105,
    112,  71,   71,   71,   71,   71,   71,   2106, 2107, 2108, 2109, 501,
    501,  501,  2110, 2111, 517,  517,  517,  517,  517,  5,    5,    5,
    5,    1064, 5,    5,    2112, 2113, 2114, 2115, 2116, 2117, 2118, 2119,
    2120, 2121, 2122, 2123, 2124, 2125, 2126, 2127, 2128, 2129, 2130, 2131,
    2132, 2133, 2134, 2135, 2136, 2137, 2138, 2139, 2140, 2141, 2142, 2143,
    2144, 2145, 2146, 2147, 2148, 2149, 517,  2150, 517,  517,  517,  517,
    517,  2151, 517,  517,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    517,  517,  517,  517,  517,  517,  517,  498,  971,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  1059,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  324,  324,  324,  324,
    324,  324,  324,  517,  324,  324,  324,  324,  324,  324,  324,  517,
    324,  324,  324,  324,  324,  324,  324,  517,  324,  324,  324,  324,
    324,  324,  324,  517,  324,  324,  324,  324,  324,  324,  324,  517,
    324,  324,  324,  324,  324,  324,  324,  517,  324,  324,  324,  324,
    324,  324,  324,  517,  324,  324,  324,  324,  324,  324,  324,  517,
    501,  501,  501,  501,  501,  501,  501,  501,  501,  501,  501,  501,
    501,  501,  501,  501,  501,  501,  501,  501,  501,  501,  501,  501,
    501,  501,  501,  501,  501,  501,  501,  501,  5,    5,    74,   80,
    74,   80,   5,    5,    5,    74,   80,   5,    74,   80,   5,    5,
    5,    5,    5,    5,    5,    5,    5,    1010, 5,    5,    1010, 5,
    74,   80,   5,    5,    74,   80,   8,    9,    8,    9,    8,    9,
    8,    9,    5,    5,    5,    5,    5,    499,  5,    5,    5,    5,
    5,    5,    5,    5,    5,    5,    1010, 1010, 5,    5,    5,    5,
    1010, 5,    1759, 5,    5,    5,    5,    5,    5,    5,    5,    5,
    5,    5,    5,    5,    71,   71,   5,    5,    5,    8,    9,    8,
    9,    8,    9,    8,    9,    1010, 517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 517,  1825, 1825, 1825, 1825, 2152,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 2152,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152,
    2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152,
    2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152,
    2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152,
    2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152,
    2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152,
    2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152,
    2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152,
    2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152,
    2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152,
    2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152,
    2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152,
    2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152,
    2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152,
    2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152,
    2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152,
    2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152,
    2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152, 2152, 517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 2153, 2154, 2154, 2154, 1825, 2155, 1162, 2156,
    1826, 1827, 1826, 1827, 1826, 1827, 1826, 1827, 1826, 1827, 1825, 1825,
    1826, 1827, 1826, 1827, 1826, 1827, 1826, 1827, 2157, 2158, 2159, 2159,
    1825, 2156, 2156, 2156, 2156, 2156, 2156, 2156, 2156, 2156, 2160, 2161,
    2162, 2163, 2164, 2164, 2157, 2155, 2155, 2155, 2155, 2155, 2152, 1825,
    2165, 2165, 2165, 2155, 1162, 2154, 1825, 71,   517,  1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 517,
    517,  2166, 2166, 2167, 2167, 2155, 2155, 2168, 2157, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 2154, 2155, 2155, 2155, 2168, 517,  517,  517,  517,
    517,  1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 517,  2169, 2169, 2169,
    2169, 2169, 2169, 2169, 2169, 2169, 2169, 2169, 2169, 2169, 2169, 2169,
    2169, 2169, 2169, 2169, 2169, 2169, 2169, 2169, 2169, 2169, 2169, 2169,
    2169, 2169, 2169, 2169, 2169, 2169, 2169, 2169, 2169, 2169, 2169, 2169,
    2169, 2169, 2169, 2169, 2169, 2169, 2169, 2169, 2169, 2169, 2169, 2169,
    2169, 2169, 2169, 2169, 2169, 2169, 2169, 2169, 2169, 2169, 2169, 2169,
    2169, 2169, 2169, 2169, 2169, 2169, 2169, 2169, 2169, 2169, 2169, 2169,
    2169, 2169, 2169, 2169, 2169, 2169, 2169, 2169, 2169, 2169, 2169, 2169,
    2169, 2169, 2169, 2169, 2169, 2169, 2169, 517,  2170, 2170, 2171, 2171,
    2171, 2171, 2172, 2172, 2172, 2172, 2172, 2172, 2172, 2172, 2172, 2172,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 517,  517,
    517,  517,  517,  517,  517,  517,  517,  1825, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    2173, 2173, 2173, 2173, 2173, 2173, 2173, 2173, 2173, 2173, 2173, 2173,
    2173, 2173, 2173, 2173, 2173, 2173, 2173, 2173, 2173, 2173, 2173, 2173,
    2173, 2173, 2173, 2173, 2173, 2152, 2152, 517,  2174, 2174, 2174, 2174,
    2174, 2174, 2174, 2174, 2174, 2174, 2173, 2173, 2173, 2173, 2173, 2173,
    2173, 2173, 2173, 2173, 2173, 2173, 2173, 2173, 2173, 2173, 2173, 2173,
    2173, 2173, 2173, 2173, 2173, 2173, 2173, 2173, 2175, 2175, 2175, 2175,
    1061, 1061, 1061, 1061, 1061, 1061, 1061, 1061, 2176, 2177, 2177, 2177,
    2177, 2177, 2177, 2177, 2177, 2177, 2177, 2177, 2177, 2177, 2177, 2177,
    2175, 2175, 2175, 2175, 2175, 2175, 2175, 2175, 2175, 2175, 2175, 2175,
    2175, 2175, 2175, 2175, 2175, 2175, 2175, 2175, 2175, 2175, 2175, 2175,
    2175, 2175, 2175, 2175, 2178, 2178, 2178, 2170, 2179, 2179, 2179, 2179,
    2179, 2179, 2179, 2179, 2179, 2179, 2175, 2175, 2175, 2175, 2175, 2175,
    2175, 2175, 2175, 2175, 2175, 2175, 2175, 2175, 2175, 2175, 2175, 2175,
    2175, 2175, 2175, 2175, 2175, 2175, 2175, 2175, 2175, 2175, 2175, 2175,
    2175, 2175, 2175, 2175, 2175, 2175, 2175, 2175, 2175, 2177, 2177, 2177,
    2177, 2177, 2177, 2177, 2177, 2177, 2177, 2177, 2177, 2177, 2177, 2177,
    2173, 2173, 2173, 2173, 2173, 2173, 2173, 2173, 2173, 2173, 2173, 2173,
    2176, 2176, 2176, 2176, 2175, 2175, 2175, 2175, 2175, 2175, 2175, 2175,
    2175, 2175, 2175, 2175, 2175, 2175, 2175, 2175, 2175, 2175, 2175, 2175,
    2175, 2175, 2175, 2175, 2175, 2175, 2175, 2175, 2175, 2175, 2175, 2175,
    2175, 2175, 2175, 2175, 2175, 2175, 2175, 2175, 2175, 2175, 2175, 2175,
    2175, 2175, 2175, 2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180,
    2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180,
    2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180,
    2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180,
    2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180,
    2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180,
    2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180,
    2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180, 2173, 2173, 2173, 2173,
    2173, 2173, 2173, 2173, 2173, 2173, 2173, 2173, 2173, 2173, 2173, 2173,
    2173, 2173, 2173, 2173, 2173, 2173, 2173, 2173, 2173, 2180, 2180, 2180,
    2180, 2180, 2180, 2176, 2176, 2176, 2176, 2180, 2180, 2180, 2180, 2180,
    2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180,
    2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180,
    2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180,
    2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180,
    2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180,
    2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180,
    2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180,
    2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180, 2176, 2176,
    2173, 2173, 2173, 2173, 2173, 2173, 2173, 2173, 2173, 2173, 2173, 2173,
    2173, 2173, 2173, 2173, 2173, 2173, 2173, 2173, 2173, 2173, 2173, 2173,
    2173, 2173, 2173, 2173, 2173, 2173, 2173, 2176, 1162, 517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  1162, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 2155, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 517,  517,  517,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 517,  517,  517,  517,  517,
    517,  517,  517,  517,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  500,  500,  500,  500,
    500,  500,  971,  971,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  500,  5,    5,    5,    324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060, 324,  324,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  2181, 2182, 2183, 2184,
    2185, 2186, 2187, 2188, 2189, 2190, 2191, 1257, 2192, 2193, 2194, 2195,
    2196, 2197, 2198, 2199, 2200, 2201, 2202, 2203, 2204, 2205, 2206, 2207,
    2208, 2209, 2210, 2211, 2212, 2213, 2214, 2215, 2216, 2217, 2218, 2219,
    2220, 2221, 2222, 2223, 2224, 2225, 324,  501,  766,  766,  766,  5,
    501,  501,  501,  501,  501,  501,  501,  501,  501,  501,  5,    499,
    2226, 2227, 2228, 2229, 2230, 2231, 2232, 2233, 2234, 2235, 2236, 2237,
    2238, 2239, 2240, 2241, 2242, 2243, 2244, 2245, 2246, 2247, 2248, 2249,
    2250, 2251, 2252, 2253, 498,  498,  501,  501,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  1255, 1255, 1255, 1255, 1255, 1255,
    1255, 1255, 1255, 1255, 501,  501,  971,  971,  971,  971,  971,  971,
    517,  517,  517,  517,  517,  517,  517,  517,  42,   42,   42,   42,
    42,   42,   42,   42,   42,   42,   42,   42,   42,   42,   42,   42,
    42,   42,   42,   42,   42,   42,   42,   499,  499,  499,  499,  499,
    499,  499,  499,  499,  42,   42,   2254, 2255, 2256, 2257, 2258, 2259,
    2260, 2261, 2262, 2263, 2264, 2265, 2266, 2267, 112,  112,  2268, 2269,
    2270, 2271, 2272, 2273, 2274, 2275, 2276, 2277, 2278, 2279, 2280, 2281,
    2282, 2283, 2284, 2285, 2286, 2287, 2288, 2289, 2290, 2291, 2292, 2293,
    2294, 2295, 2296, 2297, 2298, 2299, 2300, 2301, 2302, 2303, 2304, 2305,
    2306, 2307, 2308, 2309, 2310, 2311, 2312, 2313, 2314, 2315, 2316, 2317,
    2318, 2319, 2320, 2321, 2322, 2323, 2324, 2325, 2326, 2327, 2328, 2329,
    498,  112,  112,  112,  112,  112,  112,  112,  112,  2330, 2331, 2332,
    2333, 2334, 2335, 2336, 2337, 2338, 2339, 2340, 2341, 2342, 2343, 2344,
    499,  2345, 2345, 2346, 2347, 2348, 112,  324,  2349, 2350, 2351, 2352,
    2353, 112,  2354, 2355, 2356, 2357, 2358, 2359, 2360, 2361, 2362, 2363,
    2364, 2365, 2366, 2367, 2368, 2369, 2370, 2371, 2372, 2373, 2374, 2375,
    2376, 2377, 2378, 112,  2379, 2380, 2381, 2382, 2383, 2384, 2385, 2386,
    2387, 2388, 2389, 2390, 2391, 2392, 2393, 2394, 2395, 2396, 2397, 2398,
    2399, 2400, 2401, 2402, 2403, 2404, 2405, 2406, 2407, 2408, 517,  517,
    2409, 2410, 517,  112,  517,  112,  2411, 2412, 2413, 2414, 2415, 2416,
    2417, 517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  498,  498,
    498,  2418, 2419, 324,  498,  498,  112,  324,  324,  324,  324,  324,
    324,  324,  508,  324,  324,  324,  1059, 324,  324,  324,  324,  508,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  1057,
    1057, 508,  508,  1057, 71,   71,   71,   71,   1059, 517,  517,  517,
    1061, 1061, 1061, 1061, 1061, 1061, 765,  765,  7,    76,   517,  517,
    517,  517,  517,  517,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  5,    5,    5,    5,
    517,  517,  517,  517,  517,  517,  517,  517,  1057, 1057, 324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    1057, 1057, 1057, 1057, 1057, 1057, 1057, 1057, 1057, 1057, 1057, 1057,
    1057, 1057, 1057, 1057, 1059, 508,  517,  517,  517,  517,  517,  517,
    517,  517,  971,  971,  1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060,
    1060, 1060, 517,  517,  517,  517,  517,  517,  501,  501,  501,  501,
    501,  501,  501,  501,  501,  501,  501,  501,  501,  501,  501,  501,
    501,  501,  324,  324,  324,  324,  324,  324,  971,  971,  971,  324,
    971,  324,  324,  508,  1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060,
    1060, 1060, 324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  508,  508,  508,  508,  508,  503,
    503,  503,  971,  971,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  508,  508,  508,  508,  508,  508,  508,  508,  508,
    508,  508,  1057, 1256, 517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  971,  1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 517,  517,  517,
    508,  508,  508,  1057, 324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  1058, 1057, 1057, 508,  508,  508,  508,  1057, 1057,
    508,  508,  1057, 1057, 1256, 971,  971,  971,  971,  971,  971,  971,
    971,  971,  971,  971,  971,  971,  517,  500,  1060, 1060, 1060, 1060,
    1060, 1060, 1060, 1060, 1060, 1060, 517,  517,  517,  517,  971,  971,
    324,  324,  324,  324,  324,  508,  500,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060,
    1060, 1060, 324,  324,  324,  324,  324,  517,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  508,  508,  508,  508,  508,  508,  1057, 1057, 508,  508,  1057,
    1057, 508,  508,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    324,  324,  324,  508,  324,  324,  324,  324,  324,  324,  324,  324,
    508,  1057, 517,  517,  1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060,
    1060, 1060, 517,  517,  971,  971,  971,  971,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    500,  324,  324,  324,  324,  324,  324,  765,  765,  765,  324,  1057,
    508,  1057, 324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  501,  324,  501,  501,  503,  324,  324,  501,
    501,  324,  324,  324,  324,  324,  501,  501,  324,  501,  324,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  324,
    324,  500,  971,  971,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  1057, 508,  508,  1057, 1057, 971,  971,  324,  500,
    500,  1057, 1059, 517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  324,  324,  324,  324,  324,  324,  517,  517,  324,  324,  324,
    324,  324,  324,  517,  517,  324,  324,  324,  324,  324,  324,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  324,  324,  324,  324,
    324,  324,  324,  517,  324,  324,  324,  324,  324,  324,  324,  517,
    112,  112,  112,  112,  112,  112,  112,  112,  112,  112,  112,  112,
    112,  112,  112,  112,  112,  112,  112,  112,  112,  112,  112,  112,
    112,  112,  112,  112,  112,  112,  112,  112,  112,  112,  112,  2420,
    112,  112,  112,  112,  112,  112,  112,  2345, 498,  498,  498,  498,
    112,  112,  112,  112,  112,  112,  112,  112,  112,  498,  42,   42,
    517,  517,  517,  517,  2421, 2422, 2423, 2424, 2425, 2426, 2427, 2428,
    2429, 2430, 2431, 2432, 2433, 2434, 2435, 2436, 2437, 2438, 2439, 2440,
    2441, 2442, 2443, 2444, 2445, 2446, 2447, 2448, 2449, 2450, 2451, 2452,
    2453, 2454, 2455, 2456, 2457, 2458, 2459, 2460, 2461, 2462, 2463, 2464,
    2465, 2466, 2467, 2468, 2469, 2470, 2471, 2472, 2473, 2474, 2475, 2476,
    2477, 2478, 2479, 2480, 2481, 2482, 2483, 2484, 2485, 2486, 2487, 2488,
    2489, 2490, 2491, 2492, 2493, 2494, 2495, 2496, 2497, 2498, 2499, 2500,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  1057,
    1057, 508,  1057, 1057, 508,  1057, 1057, 971,  1057, 1059, 517,  517,
    1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060, 517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  1162, 517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  517,  517,  517,  517,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  517,  517,  517,  517,
    2501, 517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  2501, 2501, 517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  2501,
    2502, 517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  2502, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 517,  517,  1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  214,  214,  214,  214,
    214,  214,  214,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  214,  214,  214,  214,  214,  517,  517,  517,  517,
    517,  1031, 2503, 1031, 2504, 2504, 2504, 2504, 2504, 2504, 2504, 2504,
    2504, 2505, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031,
    1031, 1031, 1031, 517,  1031, 1031, 1031, 1031, 1031, 517,  1031, 517,
    1031, 1031, 517,  1031, 1031, 517,  1031, 1031, 1031, 1031, 1031, 1031,
    1031, 1031, 1031, 2506, 2507, 2508, 2507, 2508, 2509, 2510, 2507, 2508,
    2509, 2510, 2507, 2508, 2509, 2510, 2507, 2508, 2509, 2510, 2507, 2508,
    2509, 2510, 2507, 2508, 2509, 2510, 2507, 2508, 2509, 2510, 2507, 2508,
    2509, 2510, 2507, 2508, 2509, 2510, 2507, 2508, 2509, 2510, 2507, 2508,
    2509, 2510, 2507, 2508, 2509, 2510, 2507, 2508, 2507, 2508, 2507, 2508,
    2507, 2508, 2507, 2508, 2507, 2508, 2507, 2508, 2509, 2510, 2507, 2508,
    2509, 2510, 2507, 2508, 2509, 2510, 2507, 2508, 2509, 2510, 2507, 2508,
    2507, 2508, 2509, 2510, 2507, 2508, 2507, 2508, 2509, 2510, 2507, 2508,
    2509, 2510, 2507, 2508, 2507, 2508, 1056, 1056, 1056, 1056, 1056, 1056,
    1056, 1056, 1056, 1056, 1056, 1056, 1056, 1056, 1056, 1056, 1056, 517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  2507, 2508, 2509, 2510, 2507, 2508, 2507, 2508, 2507,
    2508, 2507, 2507, 2508, 2507, 2508, 2507, 2508, 2507, 2508, 2509, 2510,
    2509, 2510, 2507, 2508, 2507, 2508, 2507, 2508, 2507, 2508, 2507, 2508,
    2507, 2508, 2507, 2508, 2509, 2507, 2508, 2509, 2507, 2508, 2509, 2510,
    2507, 2507, 2507, 2507, 2507, 2507, 2507, 2507, 2507, 2507, 2507, 2507,
    2507, 2507, 2507, 2507, 2507, 2507, 2507, 2507, 2507, 2507, 2507, 2507,
    2507, 2507, 2507, 2507, 2507, 2507, 2507, 2507, 2507, 2507, 2507, 2507,
    2507, 2507, 2507, 2507, 2507, 2507, 2507, 2507, 2507, 2507, 2507, 2507,
    2507, 2507, 2507, 2507, 2507, 2507, 2507, 2507, 2507, 2507, 2507, 2507,
    2507, 2507, 2507, 2507, 2507, 2507, 2507, 2507, 2507, 2507, 2507, 2507,
    2507, 2507, 2507, 2507, 2507, 2507, 2507, 2507, 2507, 2507, 2507, 2507,
    2507, 2507, 2507, 2507, 2507, 2507, 2507, 2507, 2507, 2507, 2507, 2507,
    2507, 2507, 2507, 2507, 2508, 2508, 2508, 2508, 2508, 2508, 2508, 2508,
    2508, 2508, 2508, 2508, 2508, 2508, 2508, 2508, 2508, 2508, 2508, 2508,
    2508, 2508, 2508, 2508, 2508, 2508, 2508, 2508, 2508, 2508, 2508, 2508,
    2508, 2508, 2508, 2508, 2508, 2508, 2508, 2508, 2508, 2508, 2508, 2508,
    2508, 2508, 2508, 2508, 2508, 2508, 2508, 2509, 2509, 2509, 2509, 2509,
    2509, 2509, 2509, 2509, 2509, 2509, 2509, 2509, 2509, 2509, 2509, 2509,
    2509, 2509, 2509, 2509, 2509, 2509, 2509, 2509, 2509, 2509, 2509, 2509,
    2509, 2509, 2509, 2509, 2509, 2509, 2509, 2509, 2509, 2509, 2509, 2509,
    2509, 2509, 2509, 2509, 2509, 2509, 2509, 2509, 2509, 2509, 2509, 2509,
    2509, 2509, 2509, 2509, 2509, 2509, 2509, 2509, 2509, 2509, 2509, 2509,
    2509, 2509, 2509, 2509, 2509, 2509, 2509, 2510, 2510, 2510, 2510, 2510,
    2510, 2510, 2510, 2510, 2510, 2510, 2510, 2510, 2510, 2510, 2510, 2510,
    2510, 2510, 2510, 2510, 2510, 2507, 2507, 2507, 2507, 2507, 2507, 2507,
    2507, 2507, 2507, 2507, 2507, 2507, 2507, 2507, 2507, 2507, 2507, 2507,
    2507, 2507, 2507, 2507, 2507, 2507, 2507, 2507, 2507, 2508, 2508, 2508,
    2508, 2508, 2508, 2508, 2508, 2508, 2508, 2508, 2508, 2508, 2508, 2508,
    2508, 2508, 2508, 2508, 2508, 2508, 2508, 2508, 2508, 2508, 2508, 2508,
    2508, 2509, 2509, 2509, 2509, 2509, 2509, 2509, 2510, 2510, 2510, 2510,
    2510, 2510, 2510, 2510, 2508, 2507, 2511, 1759, 71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    2509, 2508, 2509, 2509, 2509, 2509, 2509, 2509, 2508, 2509, 2508, 2508,
    2509, 2509, 2508, 2508, 2509, 2509, 2508, 2509, 2508, 2509, 2508, 2508,
    2509, 2508, 2508, 2509, 2508, 2509, 2508, 2508, 2509, 2508, 2509, 2509,
    2508, 2508, 2508, 2509, 2508, 2508, 2508, 2508, 2508, 2509, 2508, 2508,
    2508, 2508, 2508, 2509, 2508, 2508, 2509, 2508, 2509, 2509, 2509, 2508,
    2509, 2509, 2509, 2509, 517,  517,  2509, 2509, 2509, 2509, 2508, 2508,
    2509, 2508, 2508, 2508, 2508, 2509, 2508, 2508, 2508, 2508, 2508, 2508,
    2508, 2508, 2508, 2508, 2508, 2508, 2508, 2508, 2508, 2508, 2508, 2508,
    2508, 2508, 2508, 2508, 2509, 2509, 2508, 2508, 2509, 2508, 2509, 2508,
    2508, 2508, 2508, 2508, 2508, 2508, 2508, 2509, 2509, 2509, 2508, 2508,
    517,  517,  517,  517,  517,  517,  517,  71,   517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  2507, 2507, 2507, 2507, 2507, 2507, 2507, 2507,
    2507, 2507, 2507, 2507, 2512, 71,   71,   71,   508,  508,  508,  508,
    508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,
    2513, 2513, 2513, 2513, 2513, 2513, 2513, 2514, 2515, 2513, 517,  517,
    517,  517,  517,  517,  501,  501,  501,  501,  501,  501,  501,  503,
    503,  503,  503,  503,  503,  503,  501,  501,  2513, 2516, 2516, 2517,
    2517, 2514, 2515, 2514, 2515, 2514, 2515, 2514, 2515, 2514, 2515, 2514,
    2515, 2514, 2515, 2514, 2515, 2154, 2154, 2514, 2515, 2518, 2518, 2518,
    2518, 2519, 2519, 2519, 2520, 2521, 2520, 517,  2521, 2520, 2521, 2521,
    2522, 2523, 2524, 2523, 2524, 2523, 2524, 2525, 2521, 2521, 2526, 2527,
    2528, 2528, 2529, 517,  2521, 2530, 2525, 2521, 517,  517,  517,  517,
    2507, 2510, 2507, 1040, 2507, 517,  2507, 2510, 2507, 2510, 2507, 2510,
    2507, 2510, 2507, 2510, 2507, 2507, 2508, 2507, 2508, 2507, 2508, 2507,
    2508, 2507, 2508, 2509, 2510, 2507, 2508, 2507, 2508, 2509, 2510, 2507,
    2508, 2507, 2508, 2509, 2510, 2507, 2508, 2509, 2510, 2507, 2508, 2509,
    2510, 2507, 2508, 2509, 2510, 2507, 2508, 2509, 2510, 2507, 2508, 2507,
    2508, 2507, 2508, 2507, 2508, 2507, 2508, 2509, 2510, 2507, 2508, 2509,
    2510, 2507, 2508, 2509, 2510, 2507, 2508, 2509, 2510, 2507, 2508, 2509,
    2510, 2507, 2508, 2509, 2510, 2507, 2508, 2509, 2510, 2507, 2508, 2509,
    2510, 2507, 2508, 2509, 2510, 2507, 2508, 2509, 2510, 2507, 2508, 2509,
    2510, 2507, 2508, 2509, 2510, 2507, 2508, 2509, 2510, 2507, 2508, 2509,
    2510, 2507, 2508, 2509, 2510, 2507, 2508, 2507, 2508, 2507, 2508, 2509,
    2510, 2507, 2508, 2507, 2508, 2507, 2508, 2507, 2508, 517,  517,  75,
    517,  2531, 2531, 2532, 2533, 2532, 2531, 2531, 2534, 2535, 2531, 2536,
    2537, 2538, 2537, 2537, 2539, 2539, 2539, 2539, 2539, 2539, 2539, 2539,
    2539, 2539, 2537, 2531, 2540, 2541, 2540, 2531, 2531, 2542, 2543, 2544,
    2545, 2546, 2547, 2548, 2549, 2550, 2551, 2552, 2553, 2554, 2555, 2556,
    2557, 2558, 2559, 2560, 2561, 2562, 2563, 2564, 2565, 2566, 2567, 2534,
    2531, 2535, 2568, 2569, 2568, 2570, 2571, 2572, 2573, 2574, 2575, 2576,
    2577, 2578, 2579, 2580, 2581, 2582, 2583, 2584, 2585, 2586, 2587, 2588,
    2589, 2590, 2591, 2592, 2593, 2594, 2595, 2534, 2541, 2535, 2541, 2534,
    2535, 2596, 2597, 2598, 2596, 2596, 2599, 2599, 2599, 2599, 2599, 2599,
    2599, 2599, 2599, 2599, 2600, 2599, 2599, 2599, 2599, 2599, 2599, 2599,
    2599, 2599, 2599, 2599, 2599, 2599, 2599, 2599, 2599, 2599, 2599, 2599,
    2599, 2599, 2599, 2599, 2599, 2599, 2599, 2599, 2599, 2599, 2599, 2599,
    2599, 2599, 2599, 2599, 2599, 2599, 2599, 2599, 2599, 2599, 2599, 2599,
    2599, 2599, 2600, 2600, 2599, 2599, 2599, 2599, 2599, 2599, 2599, 2599,
    2599, 2599, 2599, 2599, 2599, 2599, 2599, 2599, 2599, 2599, 2599, 2599,
    2599, 2599, 2599, 2599, 2599, 2599, 2599, 2599, 2599, 2599, 2599, 517,
    517,  517,  2599, 2599, 2599, 2599, 2599, 2599, 517,  517,  2599, 2599,
    2599, 2599, 2599, 2599, 517,  517,  2599, 2599, 2599, 2599, 2599, 2599,
    517,  517,  2599, 2599, 2599, 517,  517,  517,  2533, 2533, 2541, 2568,
    2601, 2533, 2533, 517,  2602, 2603, 2603, 2603, 2603, 2602, 2602, 517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  2604, 2604, 2604,
    71,   71,   517,  517,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  517,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  517,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  517,  324,  324,  517,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  517,  517,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  517,  517,  517,  517,  517,  971,  5,    971,  517,
    517,  517,  517,  1061, 1061, 1061, 1061, 1061, 1061, 1061, 1061, 1061,
    1061, 1061, 1061, 1061, 1061, 1061, 1061, 1061, 1061, 1061, 1061, 1061,
    1061, 1061, 1061, 1061, 1061, 1061, 1061, 1061, 1061, 1061, 1061, 1061,
    1061, 1061, 1061, 1061, 1061, 1061, 1061, 1061, 1061, 1061, 1061, 1061,
    517,  517,  517,  765,  765,  765,  765,  765,  765,  765,  765,  765,
    2605, 2605, 2605, 2605, 2605, 2605, 2605, 2605, 2605, 2605, 2605, 2605,
    2605, 2605, 2605, 2605, 2605, 2605, 2605, 2605, 2605, 2605, 2605, 2605,
    2605, 2605, 2605, 2605, 2605, 2605, 2605, 2605, 2605, 2605, 2605, 2605,
    2605, 2605, 2605, 2605, 2605, 2605, 2605, 2605, 2605, 2605, 2605, 2605,
    2605, 2605, 2605, 2605, 2605, 1064, 1064, 1064, 1064, 71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   1064, 1064, 71,   765,  765,  517,  71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   517,  517,  517,
    71,   517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,
    765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,
    765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,
    765,  765,  765,  765,  765,  765,  765,  765,  765,  503,  517,  517,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  517,  517,  517,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    503,  2606, 2606, 2606, 2606, 2606, 2606, 2606, 2606, 2606, 2606, 2606,
    2606, 2606, 2606, 2606, 2606, 2606, 2606, 2606, 2606, 2606, 2606, 2606,
    2606, 2606, 2606, 2606, 517,  517,  517,  517,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  1061, 1061, 1061, 1061, 517,  517,  517,  517,
    517,  517,  517,  517,  517,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  1255, 324,  324,  324,  324,  324,  324,  324,  324,  1255, 517,
    517,  517,  517,  517,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  501,  501,  501,  501,  501,  517,
    517,  517,  517,  517,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  517,  971,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    517,  517,  517,  517,  324,  324,  324,  324,  324,  324,  324,  324,
    971,  1255, 1255, 1255, 1255, 1255, 517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    2607, 2608, 2609, 2610, 2611, 2612, 2613, 2614, 2615, 2616, 2617, 2618,
    2619, 2620, 2621, 2622, 2623, 2624, 2625, 2626, 2627, 2628, 2629, 2630,
    2631, 2632, 2633, 2634, 2635, 2636, 2637, 2638, 2639, 2640, 2641, 2642,
    2643, 2644, 2645, 2646, 2647, 2648, 2649, 2650, 2651, 2652, 2653, 2654,
    2655, 2656, 2657, 2658, 2659, 2660, 2661, 2662, 2663, 2664, 2665, 2666,
    2667, 2668, 2669, 2670, 2671, 2672, 2673, 2674, 2675, 2676, 2677, 2678,
    2679, 2680, 2681, 2682, 2683, 2684, 2685, 2686, 324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  517,  517,  1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060,
    1060, 1060, 517,  517,  517,  517,  517,  517,  2687, 2688, 2689, 2690,
    2691, 2692, 2693, 2694, 2695, 2696, 2697, 2698, 2699, 2700, 2701, 2702,
    2703, 2704, 2705, 2706, 2707, 2708, 2709, 2710, 2711, 2712, 2713, 2714,
    2715, 2716, 2717, 2718, 2719, 2720, 2721, 2722, 517,  517,  517,  517,
    2723, 2724, 2725, 2726, 2727, 2728, 2729, 2730, 2731, 2732, 2733, 2734,
    2735, 2736, 2737, 2738, 2739, 2740, 2741, 2742, 2743, 2744, 2745, 2746,
    2747, 2748, 2749, 2750, 2751, 2752, 2753, 2754, 2755, 2756, 2757, 2758,
    517,  517,  517,  517,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  517,  517,  517,  517,
    517,  517,  517,  517,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  971,  2759, 2760, 2761, 2762,
    2763, 2764, 2765, 2766, 2767, 2768, 2769, 517,  2770, 2771, 2772, 2773,
    2774, 2775, 2776, 2777, 2778, 2779, 2780, 2781, 2782, 2783, 2784, 517,
    2785, 2786, 2787, 2788, 2789, 2790, 2791, 517,  2792, 2793, 517,  2794,
    2795, 2796, 2797, 2798, 2799, 2800, 2801, 2802, 2803, 2804, 517,  2805,
    2806, 2807, 2808, 2809, 2810, 2811, 2812, 2813, 2814, 2815, 2816, 2817,
    2818, 2819, 517,  2820, 2821, 2822, 2823, 2824, 2825, 2826, 517,  2827,
    2828, 517,  517,  517,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  324,  324,  324,  324,
    324,  324,  324,  324,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  500,  498,  498,  498,  498,  498,  517,  498,
    498,  498,  498,  498,  498,  498,  498,  498,  498,  498,  498,  498,
    498,  498,  498,  498,  498,  498,  498,  498,  498,  498,  498,  498,
    498,  498,  498,  498,  498,  498,  498,  498,  498,  498,  498,  498,
    498,  498,  498,  498,  498,  517,  498,  498,  498,  498,  498,  498,
    498,  498,  498,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    1031, 1031, 1031, 1031, 1031, 1031, 517,  517,  1031, 517,  1031, 1031,
    1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031,
    1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031,
    1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031,
    1031, 1031, 1031, 1031, 1031, 1031, 517,  1031, 1031, 517,  517,  517,
    1031, 517,  517,  1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031,
    1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031,
    1031, 1031, 517,  1028, 2829, 2829, 2829, 2829, 2829, 2829, 2829, 2829,
    1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031,
    1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 2830,
    2830, 2829, 2829, 2829, 2829, 2829, 2829, 2829, 1031, 1031, 1031, 1031,
    1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031,
    1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031,
    1031, 1031, 1031, 517,  517,  517,  517,  517,  517,  517,  517,  2829,
    2829, 2829, 2829, 2829, 2829, 2829, 2829, 2829, 517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  1031, 1031, 1031, 1031,
    1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031,
    1031, 1031, 1031, 517,  1031, 1031, 517,  517,  517,  517,  517,  2829,
    2829, 2829, 2829, 2829, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031,
    1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031,
    1031, 1031, 2829, 2829, 2829, 2829, 2829, 2829, 517,  517,  517,  5,
    1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031,
    1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031,
    1031, 1031, 517,  517,  517,  517,  517,  1028, 517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031,
    1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031,
    1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031,
    1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031,
    1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 517,  517,  517,  517,
    2829, 2829, 1031, 1031, 2829, 2829, 2829, 2829, 2829, 2829, 2829, 2829,
    2829, 2829, 2829, 2829, 2829, 2829, 2829, 2829, 517,  517,  2829, 2829,
    2829, 2829, 2829, 2829, 2829, 2829, 2829, 2829, 2829, 2829, 2829, 2829,
    2829, 2829, 2829, 2829, 2829, 2829, 2829, 2829, 2829, 2829, 2829, 2829,
    2829, 2829, 2829, 2829, 2829, 2829, 2829, 2829, 2829, 2829, 2829, 2829,
    2829, 2829, 2829, 2829, 2829, 2829, 2829, 2829, 1031, 508,  508,  508,
    517,  508,  508,  517,  517,  517,  517,  517,  508,  503,  508,  501,
    1031, 1031, 1031, 1031, 517,  1031, 1031, 1031, 517,  1031, 1031, 1031,
    1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031,
    1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031,
    1031, 1031, 517,  517,  501,  506,  503,  517,  517,  517,  517,  1059,
    2829, 2829, 2829, 2829, 2829, 2829, 2829, 2829, 2829, 517,  517,  517,
    517,  517,  517,  517,  1028, 1028, 1028, 1028, 1028, 1028, 1028, 1028,
    1028, 517,  517,  517,  517,  517,  517,  517,  1031, 1031, 1031, 1031,
    1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031,
    1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031,
    1031, 2829, 2829, 1028, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031,
    1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031,
    1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 2829, 2829, 2829,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  1031, 1031, 1031, 1031,
    1031, 1031, 1031, 1031, 2830, 1031, 1031, 1031, 1031, 1031, 1031, 1031,
    1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031,
    1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 501,  503,  517,
    517,  517,  517,  2829, 2829, 2829, 2829, 2829, 1028, 1028, 1028, 1028,
    1028, 1028, 1028, 517,  517,  517,  517,  517,  517,  517,  517,  517,
    1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031,
    1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031,
    1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031,
    1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031,
    1031, 1031, 1031, 1031, 1031, 1031, 517,  517,  517,  5,    5,    5,
    5,    5,    5,    5,    1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031,
    1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031,
    1031, 1031, 517,  517,  2829, 2829, 2829, 2829, 2829, 2829, 2829, 2829,
    1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031,
    1031, 1031, 1031, 1031, 1031, 1031, 1031, 517,  517,  517,  517,  517,
    2829, 2829, 2829, 2829, 2829, 2829, 2829, 2829, 1031, 1031, 1031, 1031,
    1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031,
    1031, 1031, 517,  517,  517,  517,  517,  517,  517,  1028, 1028, 1028,
    1028, 517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  2829, 2829, 2829, 2829, 2829, 2829, 2829, 517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031,
    1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031,
    1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031,
    1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031,
    1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031,
    1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031,
    1031, 1031, 1031, 1031, 1031, 517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    2831, 2832, 2833, 2834, 2835, 2836, 2837, 2838, 2839, 2840, 2841, 2842,
    2843, 2844, 2845, 2846, 2847, 2848, 2849, 2850, 2851, 2852, 2853, 2854,
    2855, 2856, 2857, 2858, 2859, 2860, 2861, 2862, 2863, 2864, 2865, 2866,
    2867, 2868, 2869, 2870, 2871, 2872, 2873, 2874, 2875, 2876, 2877, 2878,
    2879, 2880, 2881, 517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  2882, 2883, 2884, 2885, 2886, 2887, 2888, 2889,
    2890, 2891, 2892, 2893, 2894, 2895, 2896, 2897, 2898, 2899, 2900, 2901,
    2902, 2903, 2904, 2905, 2906, 2907, 2908, 2909, 2910, 2911, 2912, 2913,
    2914, 2915, 2916, 2917, 2918, 2919, 2920, 2921, 2922, 2923, 2924, 2925,
    2926, 2927, 2928, 2929, 2930, 2931, 2932, 517,  517,  517,  517,  517,
    517,  517,  2829, 2829, 2829, 2829, 2829, 2829, 1040, 1040, 1040, 1040,
    1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040,
    1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040,
    1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 501,  501,  501,  501,
    517,  517,  517,  517,  517,  517,  517,  517,  1047, 1047, 1047, 1047,
    1047, 1047, 1047, 1047, 1047, 1047, 517,  517,  517,  517,  517,  517,
    1047, 1047, 1047, 1047, 1047, 1047, 1047, 1047, 1047, 1047, 1031, 1031,
    1031, 1031, 1054, 1031, 2933, 2934, 2935, 2936, 2937, 2938, 2939, 2940,
    2941, 2942, 2943, 2944, 2945, 2946, 2947, 2948, 2949, 2950, 2951, 2952,
    2953, 2954, 517,  517,  517,  501,  501,  501,  501,  501,  1010, 1054,
    2955, 2956, 2957, 2958, 2959, 2960, 2961, 2962, 2963, 2964, 2965, 2966,
    2967, 2968, 2969, 2970, 2971, 2972, 2973, 2974, 2975, 2976, 517,  517,
    517,  517,  517,  517,  517,  517,  2977, 2977, 517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    2978, 2978, 2978, 2978, 2978, 2978, 2978, 2978, 2978, 2978, 2978, 2978,
    2978, 2978, 2978, 2978, 2978, 2978, 2978, 2978, 2978, 2978, 2978, 2978,
    2978, 2978, 2978, 2978, 2978, 2978, 2978, 517,  1031, 1031, 1031, 1031,
    1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031,
    1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031,
    1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031,
    1031, 1031, 517,  501,  501,  1026, 517,  517,  1031, 1031, 517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  1040, 1040, 1040, 517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    508,  503,  503,  503,  1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031,
    1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031,
    1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 2829, 2829, 2829,
    2829, 2829, 2829, 2829, 2829, 2829, 2829, 1031, 517,  517,  517,  517,
    517,  517,  517,  517,  1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040,
    1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040, 1040,
    1040, 1040, 503,  503,  501,  501,  501,  503,  501,  503,  503,  503,
    503,  2979, 2979, 2979, 2979, 1035, 1035, 1035, 1035, 1035, 517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  1031, 1031, 1031, 1031,
    1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031,
    1031, 1031, 501,  503,  501,  503,  1028, 1028, 1028, 1028, 517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031,
    1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 2829, 2829, 2829,
    2829, 2829, 2829, 2829, 517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031,
    1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 517,
    517,  517,  517,  517,  517,  517,  517,  517,  1057, 508,  1057, 324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  508,  508,  508,  508,  508,  508,  508,  508,
    508,  508,  508,  508,  508,  508,  1059, 971,  971,  971,  971,  971,
    971,  971,  517,  517,  517,  517,  1064, 1064, 1064, 1064, 1064, 1064,
    1064, 1064, 1064, 1064, 1064, 1064, 1064, 1064, 1064, 1064, 1064, 1064,
    1064, 1064, 1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060,
    1059, 324,  324,  508,  508,  324,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  1059, 508,  508,  1057, 324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  1057, 1057, 1057, 508,  508,  508,  508,  1057,
    1057, 1059, 1058, 971,  971,  1753, 971,  971,  971,  971,  508,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  1753, 517,  517,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  517,  517,  517,  517,  517,  517,  517,  1060, 1060, 1060, 1060,
    1060, 1060, 1060, 1060, 1060, 1060, 517,  517,  517,  517,  517,  517,
    501,  501,  501,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  508,  508,  508,  508,  508,  1057, 508,  508,  508,
    508,  508,  508,  1059, 1059, 517,  1060, 1060, 1060, 1060, 1060, 1060,
    1060, 1060, 1060, 1060, 971,  971,  971,  971,  324,  1057, 1057, 324,
    517,  517,  517,  517,  517,  517,  517,  517,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  1058, 971,  971,  324,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  508,  508,  1057, 324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  1057,
    1057, 1057, 508,  508,  508,  508,  508,  508,  508,  508,  508,  1057,
    1256, 324,  324,  324,  324,  971,  971,  971,  971,  508,  1058, 508,
    508,  971,  1057, 508,  1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060,
    1060, 1060, 324,  971,  324,  971,  971,  971,  517,  1061, 1061, 1061,
    1061, 1061, 1061, 1061, 1061, 1061, 1061, 1061, 1061, 1061, 1061, 1061,
    1061, 1061, 1061, 1061, 1061, 517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  517,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    1057, 1057, 1057, 508,  508,  508,  1057, 1057, 508,  1256, 1058, 508,
    971,  971,  971,  971,  971,  971,  508,  324,  324,  508,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    324,  324,  324,  324,  324,  324,  324,  517,  324,  517,  324,  324,
    324,  324,  517,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  517,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  971,  517,  517,  517,  517,  517,  517,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  508,
    1057, 1057, 1057, 508,  508,  508,  508,  508,  508,  1058, 1059, 517,
    517,  517,  517,  517,  1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060,
    1060, 1060, 517,  517,  517,  517,  517,  517,  508,  508,  1057, 1057,
    517,  324,  324,  324,  324,  324,  324,  324,  324,  517,  517,  324,
    324,  517,  517,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  517,  324,  324,  324,  324,  324,  324,  324,  517,  324,  324,
    517,  324,  324,  324,  324,  324,  517,  1058, 1058, 324,  1057, 1057,
    508,  1057, 1057, 1057, 1057, 517,  517,  1057, 1057, 517,  517,  1057,
    1057, 1256, 517,  517,  324,  517,  517,  517,  517,  517,  517,  1057,
    517,  517,  517,  517,  517,  324,  324,  324,  324,  324,  1057, 1057,
    517,  517,  501,  501,  501,  501,  501,  501,  501,  517,  517,  517,
    501,  501,  501,  501,  501,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  517,  324,  517,  517,  324,  517,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  517,  324,
    1057, 1057, 1057, 508,  508,  508,  508,  508,  508,  517,  1057, 517,
    517,  1057, 517,  1057, 1057, 1057, 1057, 517,  1057, 1057, 1059, 1256,
    1059, 324,  508,  324,  971,  971,  517,  971,  971,  517,  517,  517,
    517,  517,  517,  517,  517,  508,  508,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  1057, 1057, 1057, 508,  508,  508,  508,
    508,  508,  508,  508,  1057, 1057, 1059, 508,  508,  1057, 1058, 324,
    324,  324,  324,  971,  971,  971,  971,  971,  1060, 1060, 1060, 1060,
    1060, 1060, 1060, 1060, 1060, 1060, 971,  971,  517,  971,  501,  324,
    324,  324,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  1057, 1057, 1057, 508,
    508,  508,  508,  508,  508,  1057, 508,  1057, 1057, 1057, 1057, 508,
    508,  1057, 1059, 1058, 324,  324,  971,  324,  517,  517,  517,  517,
    517,  517,  517,  517,  1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060,
    1060, 1060, 517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  1057, 1057, 1057, 508,  508,  508,  508,  517,  517,
    1057, 1057, 1057, 1057, 508,  508,  1057, 1059, 1058, 971,  971,  971,
    971,  971,  971,  971,  971,  971,  971,  971,  971,  971,  971,  971,
    971,  971,  971,  971,  971,  971,  971,  971,  324,  324,  324,  324,
    508,  508,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    1057, 1057, 1057, 508,  508,  508,  508,  508,  508,  508,  508,  1057,
    1057, 508,  1057, 1059, 508,  971,  971,  971,  324,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  1060, 1060, 1060, 1060,
    1060, 1060, 1060, 1060, 1060, 1060, 517,  517,  517,  517,  517,  517,
    5,    5,    5,    5,    5,    5,    5,    5,    5,    5,    5,    5,
    5,    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  508,  1057, 508,  1057, 1057, 508,  508,  508,  508,
    508,  508,  1256, 1058, 324,  971,  517,  517,  517,  517,  517,  517,
    1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060, 517,  517,
    517,  517,  517,  517,  1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060,
    1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  517,  517,  508,  1057, 508,
    1057, 1057, 508,  508,  508,  508,  1057, 508,  508,  508,  508,  1059,
    517,  517,  517,  517,  1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060,
    1060, 1060, 1061, 1061, 971,  971,  971,  765,  324,  324,  324,  324,
    324,  324,  324,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  1057, 1057, 1057, 508,
    508,  508,  508,  508,  508,  508,  508,  508,  1057, 1059, 1058, 971,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  2980, 2981, 2982, 2983, 2984, 2985, 2986, 2987,
    2988, 2989, 2990, 2991, 2992, 2993, 2994, 2995, 2996, 2997, 2998, 2999,
    3000, 3001, 3002, 3003, 3004, 3005, 3006, 3007, 3008, 3009, 3010, 3011,
    3012, 3013, 3014, 3015, 3016, 3017, 3018, 3019, 3020, 3021, 3022, 3023,
    3024, 3025, 3026, 3027, 3028, 3029, 3030, 3031, 3032, 3033, 3034, 3035,
    3036, 3037, 3038, 3039, 3040, 3041, 3042, 3043, 1060, 1060, 1060, 1060,
    1060, 1060, 1060, 1060, 1060, 1060, 1061, 1061, 1061, 1061, 1061, 1061,
    1061, 1061, 1061, 517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  324,  324,  324,  324,  324,  324,  324,  324,  517,
    517,  324,  517,  517,  324,  324,  324,  324,  324,  324,  324,  324,
    517,  324,  324,  517,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  1057, 1057, 1057, 1057, 1057, 1057, 517,  1057,
    1057, 517,  517,  508,  508,  1256, 1059, 324,  1057, 324,  1057, 1058,
    971,  971,  971,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060, 517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  324,  324,  324,  324,
    324,  324,  324,  324,  517,  517,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  1057, 1057, 1057,
    508,  508,  508,  508,  517,  517,  508,  508,  1057, 1057, 1057, 1057,
    1059, 324,  971,  324,  1057, 517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  324,  508,  508,  508,
    508,  508,  508,  1065, 1065, 508,  508,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  508,
    1059, 508,  508,  508,  508,  1057, 324,  508,  508,  508,  508,  971,
    971,  971,  971,  971,  971,  971,  971,  1059, 517,  517,  517,  517,
    517,  517,  517,  517,  324,  508,  508,  508,  508,  508,  508,  1057,
    1057, 508,  508,  508,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,
    508,  508,  508,  1057, 508,  1059, 971,  971,  971,  324,  971,  971,
    971,  971,  971,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  517,  517,  517,  517,  517,  517,  517,
    971,  971,  971,  971,  971,  971,  971,  971,  971,  971,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  971,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060, 517,  517,
    517,  517,  517,  517,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  517,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  1057, 508,  508,  508,  508,  508,  508,  508,  517,
    508,  508,  508,  508,  508,  508,  1057, 3044, 324,  971,  971,  971,
    971,  971,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060, 1061, 1061,
    1061, 1061, 1061, 1061, 1061, 1061, 1061, 1061, 1061, 1061, 1061, 1061,
    1061, 1061, 1061, 1061, 1061, 517,  517,  517,  971,  971,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  517,  517,  508,  508,  508,  508,  508,  508,
    508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,
    508,  508,  508,  508,  517,  1057, 508,  508,  508,  508,  508,  508,
    508,  1057, 508,  508,  1057, 508,  508,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  324,  324,  324,  324,
    324,  324,  324,  517,  324,  324,  517,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  508,  508,  508,
    508,  508,  508,  517,  517,  517,  508,  517,  508,  508,  517,  508,
    508,  508,  1058, 508,  1059, 1059, 324,  508,  517,  517,  517,  517,
    517,  517,  517,  517,  1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060,
    1060, 1060, 517,  517,  517,  517,  517,  517,  324,  324,  324,  324,
    324,  324,  517,  324,  324,  517,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  1057, 1057, 1057, 1057, 1057, 517,  508,  508,  517,  1057,
    1057, 508,  1057, 1059, 324,  517,  517,  517,  517,  517,  517,  517,
    1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060, 517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  508,  508,  1057, 1057, 971,
    971,  517,  517,  517,  517,  517,  517,  517,  508,  508,  324,  1057,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  517,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    1057, 1057, 508,  508,  508,  508,  508,  517,  517,  517,  1057, 1057,
    508,  1256, 1059, 971,  971,  971,  971,  971,  971,  971,  971,  971,
    971,  971,  971,  971,  1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060,
    1060, 1060, 508,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  324,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  1061, 1061, 1061, 1061,
    1061, 1061, 1061, 1061, 1061, 1061, 1061, 1061, 1061, 1061, 1061, 1061,
    1061, 1061, 1061, 1061, 1061, 71,   71,   71,   71,   71,   71,   71,
    71,   7,    7,    7,    7,    71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  971,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  1255, 1255, 1255, 1255,
    1255, 1255, 1255, 1255, 1255, 1255, 1255, 1255, 1255, 1255, 1255, 1255,
    1255, 1255, 1255, 1255, 1255, 1255, 1255, 1255, 1255, 1255, 1255, 1255,
    1255, 1255, 1255, 1255, 1255, 1255, 1255, 1255, 1255, 1255, 1255, 1255,
    1255, 1255, 1255, 1255, 1255, 1255, 1255, 1255, 1255, 1255, 1255, 1255,
    1255, 1255, 1255, 1255, 1255, 1255, 1255, 1255, 1255, 1255, 1255, 1255,
    1255, 1255, 1255, 1255, 1255, 1255, 1255, 1255, 1255, 1255, 1255, 1255,
    1255, 1255, 1255, 1255, 1255, 1255, 1255, 1255, 1255, 1255, 1255, 1255,
    1255, 1255, 1255, 1255, 1255, 1255, 1255, 1255, 1255, 1255, 1255, 1255,
    1255, 1255, 1255, 1255, 1255, 1255, 1255, 1255, 1255, 1255, 1255, 517,
    971,  971,  971,  971,  971,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  971,  971,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  1753, 1753, 1753, 1753,
    1753, 1753, 1753, 1753, 1753, 1753, 1753, 1753, 1753, 1753, 1753, 1753,
    508,  324,  324,  324,  324,  324,  324,  508,  508,  508,  508,  508,
    508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  508,  508,  508,  508,  508,  508,
    508,  508,  508,  508,  508,  508,  1057, 1057, 1057, 508,  508,  1059,
    1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060, 517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  517,  517,  517,  517,  517,  517,  517,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  517,  1060, 1060, 1060, 1060,
    1060, 1060, 1060, 1060, 1060, 1060, 517,  517,  517,  517,  971,  971,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  517,  1060, 1060, 1060, 1060,
    1060, 1060, 1060, 1060, 1060, 1060, 517,  517,  517,  517,  517,  517,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  517,  517,  506,  506,  506,  506,
    506,  971,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    501,  501,  501,  501,  501,  501,  501,  971,  971,  971,  971,  971,
    765,  765,  765,  765,  500,  500,  500,  500,  971,  765,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  1060, 1060, 1060, 1060,
    1060, 1060, 1060, 1060, 1060, 1060, 517,  1061, 1061, 1061, 1061, 1061,
    1061, 1061, 517,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    517,  517,  517,  517,  517,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  500,  500,  500,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  500,  500,  971,  971,  971,  1060, 1060, 1060, 1060,
    1060, 1060, 1060, 1060, 1060, 1060, 517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  3045, 3046, 3047, 3048, 3049, 3050, 3051, 3052,
    3053, 3054, 3055, 3056, 3057, 3058, 3059, 3060, 3061, 3062, 3063, 3064,
    3065, 3066, 3067, 3068, 3069, 3070, 3071, 3072, 3073, 3074, 3075, 3076,
    3077, 3078, 3079, 3080, 3081, 3082, 3083, 3084, 3085, 3086, 3087, 3088,
    3089, 3090, 3091, 3092, 3093, 3094, 3095, 3096, 3097, 3098, 3099, 3100,
    3101, 3102, 3103, 3104, 3105, 3106, 3107, 3108, 1061, 1061, 1061, 1061,
    1061, 1061, 1061, 1061, 1061, 1061, 1061, 1061, 1061, 1061, 1061, 1061,
    1061, 1061, 1061, 1061, 1061, 1061, 1061, 971,  971,  971,  971,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  517,  517,  517,  517,  508,
    324,  1057, 1057, 1057, 1057, 1057, 1057, 1057, 1057, 1057, 1057, 1057,
    1057, 1057, 1057, 1057, 1057, 1057, 1057, 1057, 1057, 1057, 1057, 1057,
    1057, 1057, 1057, 1057, 1057, 1057, 1057, 1057, 1057, 1057, 1057, 1057,
    1057, 1057, 1057, 1057, 1057, 1057, 1057, 1057, 1057, 1057, 1057, 1057,
    1057, 1057, 1057, 1057, 1057, 1057, 1057, 1057, 517,  517,  517,  517,
    517,  517,  517,  508,  508,  508,  508,  500,  500,  500,  500,  500,
    500,  500,  500,  500,  500,  500,  500,  500,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    2155, 2155, 2154, 2155, 3109, 517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  3110, 3110, 517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  1162, 517,  517,  517,  517,
    517,  517,  517,  517,  1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  1162,
    1162, 517,  517,  517,  517,  517,  517,  517,  1162, 517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    2155, 2155, 2155, 2155, 517,  2155, 2155, 2155, 2155, 2155, 2155, 2155,
    517,  2155, 2155, 517,  1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  1162, 517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    1162, 1162, 1162, 517,  517,  1162, 517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  1162, 1162, 1162, 1162,
    517,  517,  517,  517,  517,  517,  517,  517,  1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 517,  517,  517,  517,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  517,  517,  517,  517,  517,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  517,  517,  517,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  517,  517,  517,  517,  517,  517,  517,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  517,  517,  765,  508,  506,  971,
    75,   75,   75,   75,   517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   3111, 3111, 3111, 3111, 3111, 3111, 3111, 3111, 3111, 3111,
    3111, 3111, 3111, 3111, 3111, 3111, 3111, 3111, 3111, 3111, 3111, 3111,
    3111, 3111, 3111, 3111, 3112, 3112, 3112, 3112, 3112, 3112, 3112, 3112,
    3112, 3112, 517,  517,  517,  517,  517,  517,  71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  508,  508,  508,  508,  508,  508,  508,  508,
    508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,
    508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,
    508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,
    508,  508,  517,  517,  508,  508,  508,  508,  508,  508,  508,  508,
    508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,
    508,  508,  508,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,
    765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,
    765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,
    765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,
    765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,
    765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,
    765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,
    765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,
    765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,
    765,  765,  765,  765,  765,  765,  765,  765,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  765,  765,  765,  765,
    765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,
    765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,
    765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,
    765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,
    765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,
    765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,
    765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,
    765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,
    765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,
    765,  765,  765,  765,  765,  765,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  765,  765,  765,  765,  765,  765,  765,  765,
    765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,
    765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,
    765,  765,  765,  765,  765,  765,  765,  517,  517,  765,  765,  765,
    765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,
    765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,
    765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,
    765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,
    765,  765,  765,  765,  765,  765,  765,  765,  765,  3113, 3113, 506,
    506,  506,  765,  765,  765,  3114, 3113, 3113, 3113, 3113, 3113, 75,
    75,   75,   75,   75,   75,   75,   75,   503,  503,  503,  503,  503,
    503,  503,  503,  765,  765,  501,  501,  501,  501,  501,  503,  503,
    765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,
    765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,
    765,  765,  765,  765,  765,  765,  501,  501,  501,  501,  765,  765,
    765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,
    765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,
    765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,
    765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,
    765,  765,  765,  765,  765,  765,  765,  765,  765,  71,   71,   517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   501,  501,  501,  71,   517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  1061, 1061, 1061, 1061,
    1061, 1061, 1061, 1061, 1061, 1061, 1061, 1061, 1061, 1061, 1061, 1061,
    1061, 1061, 1061, 1061, 517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  1061, 1061, 1061, 1061, 1061, 1061, 1061, 1061,
    1061, 1061, 1061, 1061, 1061, 1061, 1061, 1061, 1061, 1061, 1061, 1061,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 517,  517,  517,  517,  517,  517,  517,  517,  517,
    3115, 3115, 3115, 3115, 3115, 3115, 3115, 3115, 3115, 3115, 3115, 3115,
    3115, 3115, 3115, 3115, 3115, 3115, 3115, 3115, 3115, 3115, 3115, 1061,
    1061, 517,  517,  517,  517,  517,  517,  517,  1784, 1784, 1784, 1784,
    1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784,
    1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1785, 1785,
    1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785,
    1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785,
    1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784,
    1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784,
    1784, 1784, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 517,  1785, 1785,
    1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785,
    1785, 1785, 1785, 1785, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784,
    1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784,
    1784, 1784, 1784, 1784, 1784, 1784, 1785, 1785, 1785, 1785, 1785, 1785,
    1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785,
    1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1784, 517,  1784, 1784,
    517,  517,  1784, 517,  517,  1784, 1784, 517,  517,  1784, 1784, 1784,
    1784, 517,  1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1785, 1785,
    1785, 1785, 517,  1785, 517,  1785, 1785, 1785, 1785, 1785, 1785, 1785,
    517,  1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785,
    1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784,
    1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784,
    1784, 1784, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785,
    1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785,
    1785, 1785, 1785, 1785, 1784, 1784, 517,  1784, 1784, 1784, 1784, 517,
    517,  1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 517,  1784, 1784,
    1784, 1784, 1784, 1784, 1784, 517,  1785, 1785, 1785, 1785, 1785, 1785,
    1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785,
    1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1784, 1784, 517,  1784,
    1784, 1784, 1784, 517,  1784, 1784, 1784, 1784, 1784, 517,  1784, 517,
    517,  517,  1784, 1784, 1784, 1784, 1784, 1784, 1784, 517,  1785, 1785,
    1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785,
    1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785,
    1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784,
    1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784,
    1784, 1784, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785,
    1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785,
    1785, 1785, 1785, 1785, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784,
    1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784,
    1784, 1784, 1784, 1784, 1784, 1784, 1785, 1785, 1785, 1785, 1785, 1785,
    1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785,
    1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1784, 1784, 1784, 1784,
    1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784,
    1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1785, 1785,
    1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785,
    1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785,
    1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784,
    1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784,
    1784, 1784, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785,
    1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785,
    1785, 1785, 1785, 1785, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784,
    1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784,
    1784, 1784, 1784, 1784, 1784, 1784, 1785, 1785, 1785, 1785, 1785, 1785,
    1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785,
    1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1784, 1784, 1784, 1784,
    1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784,
    1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1785, 1785,
    1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785,
    1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785,
    1785, 1785, 517,  517,  1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784,
    1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784,
    1784, 1784, 1784, 1784, 1784, 3116, 1785, 1785, 1785, 1785, 1785, 1785,
    1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785,
    1785, 1785, 1785, 1785, 1785, 1785, 1785, 1788, 1785, 1785, 1785, 1785,
    1785, 1785, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784,
    1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784,
    1784, 1784, 1784, 3116, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785,
    1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785,
    1785, 1785, 1785, 1785, 1785, 1788, 1785, 1785, 1785, 1785, 1785, 1785,
    1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784,
    1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784,
    1784, 3116, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785,
    1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785,
    1785, 1785, 1785, 1788, 1785, 1785, 1785, 1785, 1785, 1785, 1784, 1784,
    1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784,
    1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 3116,
    1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785,
    1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785,
    1785, 1788, 1785, 1785, 1785, 1785, 1785, 1785, 1784, 1784, 1784, 1784,
    1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784,
    1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 1784, 3116, 1785, 1785,
    1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785,
    1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1785, 1788,
    1785, 1785, 1785, 1785, 1785, 1785, 1784, 1785, 517,  517,  3112, 3112,
    3112, 3112, 3112, 3112, 3112, 3112, 3112, 3112, 3112, 3112, 3112, 3112,
    3112, 3112, 3112, 3112, 3112, 3112, 3112, 3112, 3112, 3112, 3112, 3112,
    3112, 3112, 3112, 3112, 3112, 3112, 3112, 3112, 3112, 3112, 3112, 3112,
    3112, 3112, 3112, 3112, 3112, 3112, 3112, 3112, 3112, 3112, 3112, 3112,
    508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,
    508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,
    508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,
    508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,
    508,  508,  508,  508,  508,  508,  508,  765,  765,  765,  765,  508,
    508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,
    508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,
    508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,
    508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,
    508,  765,  765,  765,  765,  765,  765,  765,  765,  508,  765,  765,
    765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,
    508,  765,  765,  971,  971,  971,  971,  971,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  508,
    508,  508,  508,  508,  517,  508,  508,  508,  508,  508,  508,  508,
    508,  508,  508,  508,  508,  508,  508,  508,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  112,  112,  112,  112,  112,  112,  112,  112,
    112,  112,  324,  112,  112,  112,  112,  112,  112,  112,  112,  112,
    112,  112,  112,  112,  112,  112,  112,  112,  112,  112,  112,  517,
    517,  517,  517,  517,  517,  112,  112,  112,  112,  112,  112,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    501,  501,  501,  501,  501,  501,  501,  517,  501,  501,  501,  501,
    501,  501,  501,  501,  501,  501,  501,  501,  501,  501,  501,  501,
    501,  517,  517,  501,  501,  501,  501,  501,  501,  501,  517,  501,
    501,  517,  501,  501,  501,  501,  501,  517,  517,  517,  517,  517,
    498,  498,  498,  498,  498,  498,  498,  498,  498,  498,  498,  498,
    498,  498,  498,  498,  498,  498,  498,  498,  498,  498,  498,  498,
    498,  498,  498,  498,  498,  498,  498,  498,  498,  1306, 1306, 1306,
    1306, 1306, 1306, 1306, 1306, 1306, 1306, 1306, 1306, 1306, 1306, 1306,
    1306, 1306, 1306, 1306, 1306, 1306, 1306, 1306, 1306, 1306, 1306, 498,
    498,  498,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  501,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  517,  517,  517,  501,  501,  501,  501,  501,  501,  501,  500,
    500,  500,  500,  500,  500,  500,  517,  517,  1060, 1060, 1060, 1060,
    1060, 1060, 1060, 1060, 1060, 1060, 517,  517,  517,  517,  324,  765,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  501,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    501,  501,  501,  501,  1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060,
    1060, 1060, 517,  517,  517,  517,  517,  7,    517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  500,  502,  502,  503,  501,
    1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060, 1060, 517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  501,  503,  324,  1060, 1060, 1060,
    1060, 1060, 1060, 1060, 1060, 1060, 1060, 517,  517,  517,  517,  971,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    324,  324,  324,  324,  324,  324,  324,  517,  324,  324,  324,  324,
    517,  324,  324,  517,  324,  324,  324,  324,  324,  324,  324,  324,
    324,  324,  324,  324,  324,  324,  324,  517,  1031, 1031, 1031, 1031,
    1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031,
    1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031,
    1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031,
    1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031,
    1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031,
    1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031,
    1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031,
    1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031,
    1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031,
    1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031,
    1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031,
    1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031,
    1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031,
    1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031,
    1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031,
    1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031, 1031,
    1031, 517,  517,  2829, 2829, 2829, 2829, 2829, 2829, 2829, 2829, 2829,
    503,  503,  503,  503,  503,  503,  503,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    3117, 3118, 3119, 3120, 3121, 3122, 3123, 3124, 3125, 3126, 3127, 3128,
    3129, 3130, 3131, 3132, 3133, 3134, 3135, 3136, 3137, 3138, 3139, 3140,
    3141, 3142, 3143, 3144, 3145, 3146, 3147, 3148, 3149, 3150, 3151, 3152,
    3153, 3154, 3155, 3156, 3157, 3158, 3159, 3160, 3161, 3162, 3163, 3164,
    3165, 3166, 3167, 3168, 3169, 3170, 3171, 3172, 3173, 3174, 3175, 3176,
    3177, 3178, 3179, 3180, 3181, 3182, 3183, 3184, 501,  501,  501,  501,
    501,  501,  1058, 1054, 517,  517,  517,  517,  1053, 1053, 1053, 1053,
    1053, 1053, 1053, 1053, 1053, 1053, 517,  517,  517,  517,  1028, 1028,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  2979, 2979, 2979, 2979, 2979, 2979, 2979, 2979, 2979, 2979, 2979,
    2979, 2979, 2979, 2979, 2979, 2979, 2979, 2979, 2979, 2979, 2979, 2979,
    2979, 2979, 2979, 2979, 2979, 2979, 2979, 2979, 2979, 2979, 2979, 2979,
    2979, 2979, 2979, 2979, 2979, 2979, 2979, 2979, 2979, 2979, 2979, 2979,
    2979, 2979, 2979, 2979, 2979, 2979, 2979, 2979, 2979, 2979, 2979, 2979,
    1051, 2979, 2979, 2979, 1034, 2979, 2979, 2979, 2979, 517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  2979, 2979, 2979, 2979, 2979, 2979, 2979, 2979, 2979, 2979, 2979,
    2979, 2979, 2979, 2979, 2979, 2979, 2979, 2979, 2979, 2979, 2979, 2979,
    2979, 2979, 2979, 2979, 2979, 2979, 2979, 2979, 2979, 2979, 2979, 2979,
    2979, 2979, 2979, 2979, 2979, 2979, 2979, 2979, 2979, 2979, 1051, 2979,
    2979, 2979, 2979, 2979, 2979, 2979, 2979, 2979, 2979, 2979, 2979, 2979,
    2979, 2979, 517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  3185, 3185, 3185, 3185,
    517,  3185, 3185, 3185, 3185, 3185, 3185, 3185, 3185, 3185, 3185, 3185,
    3185, 3185, 3185, 3185, 3185, 3185, 3185, 3185, 3185, 3185, 3185, 3185,
    3185, 3185, 3185, 3185, 517,  3185, 3185, 517,  3185, 517,  517,  3185,
    517,  3185, 3185, 3185, 3185, 3185, 3185, 3185, 3185, 3185, 3185, 517,
    3185, 3185, 3185, 3185, 517,  3185, 517,  3185, 517,  517,  517,  517,
    517,  517,  3185, 517,  517,  517,  517,  3185, 517,  3185, 517,  3185,
    517,  3185, 3185, 3185, 517,  3185, 3185, 517,  3185, 517,  517,  3185,
    517,  3185, 517,  3185, 517,  3185, 517,  3185, 517,  3185, 3185, 517,
    3185, 517,  517,  3185, 3185, 3185, 3185, 517,  3185, 3185, 3185, 3185,
    3185, 3185, 3185, 517,  3185, 3185, 3185, 3185, 517,  3185, 3185, 3185,
    3185, 517,  3185, 517,  3185, 3185, 3185, 3185, 3185, 3185, 3185, 3185,
    3185, 3185, 517,  3185, 3185, 3185, 3185, 3185, 3185, 3185, 3185, 3185,
    3185, 3185, 3185, 3185, 3185, 3185, 3185, 3185, 517,  517,  517,  517,
    517,  3185, 3185, 3185, 517,  3185, 3185, 3185, 3185, 3185, 517,  3185,
    3185, 3185, 3185, 3185, 3185, 3185, 3185, 3185, 3185, 3185, 3185, 3185,
    3185, 3185, 3185, 3185, 517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  15,   15,   517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    71,   71,   71,   71,   1825, 71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   517,  517,  517,  517,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   517,  517,  71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    517,  71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   1825, 517,  71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  1830, 1830, 1830, 1830, 1830, 1830, 1830, 1830,
    1830, 1830, 1830, 1064, 1064, 71,   71,   71,   1831, 1831, 1831, 1831,
    1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831,
    1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 1831, 3186,
    3186, 3186, 3186, 71,   3187, 3187, 3187, 3187, 3187, 3187, 3187, 3187,
    3187, 3187, 3187, 3187, 3187, 3187, 3187, 3187, 3187, 3187, 3187, 3187,
    3187, 3187, 3187, 3187, 3187, 3187, 3187, 3187, 3187, 3187, 3187, 3187,
    765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,
    765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,
    765,  765,  1786, 1786, 1786, 71,   71,   71,   765,  765,  765,  765,
    765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,
    765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,
    765,  765,  2170, 765,  3187, 2170, 2170, 2170, 2170, 2170, 2170, 2170,
    2170, 2170, 2170, 765,  765,  765,  765,  765,  765,  765,  765,  765,
    765,  765,  765,  765,  765,  765,  765,  765,  765,  71,   517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  765,  765,  765,  765,  765,  765,
    765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,  765,
    765,  765,  765,  765,  765,  765,  765,  765,  2180, 2180, 2180, 517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180,
    2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180,
    2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180,
    2180, 2180, 2180, 2180, 2180, 2180, 2180, 2180, 517,  517,  517,  517,
    2173, 2173, 2173, 2173, 2173, 2173, 2173, 2173, 2173, 517,  517,  517,
    517,  517,  517,  517,  2175, 2175, 517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  1825, 1825, 1825, 1825,
    1825, 1825, 517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 71,   1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 71,   1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 71,   71,   71,   71,   1825, 1825, 1825, 1825, 1825,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 71,   71,   71,   1825, 71,   71,   71,
    1825, 1825, 1825, 3188, 3188, 3188, 3188, 3188, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 71,
    1825, 71,   1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 71,   71,   1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   1825, 1825, 1825, 1825, 71,   1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   1825, 71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   1825, 1825, 71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   1825, 71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 71,   71,
    71,   71,   71,   71,   1825, 71,   71,   71,   1825, 1825, 1825, 71,
    71,   1825, 1825, 1825, 517,  517,  517,  517,  1825, 1825, 1825, 1825,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   1825,
    1825, 517,  517,  517,  71,   71,   71,   71,   1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 517,  517,  517,  71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   517,  517,  517,  517,  71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   517,  517,
    517,  517,  517,  517,  1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 517,  517,  517,  517,  1825, 517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    517,  517,  517,  517,  71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    517,  517,  517,  517,  517,  517,  517,  517,  71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   517,  517,  517,  517,  517,  517,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   517,  517,  517,  517,  517,  517,  517,  517,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   517,  517,  71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   517,  517,  517,  517,
    71,   71,   517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 71,   1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 71,   1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   517,  517,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 517,  517,  517,  1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 517,  517,  517,  517,  517,  1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 1825, 517,  517,  517,  517,  517,  517,  517,  1825, 1825,
    1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 517,  517,  1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825, 1825,
    1825, 1825, 517,  517,  517,  517,  517,  517,  1825, 1825, 1825, 1825,
    1825, 1825, 1825, 1825, 1825, 517,  517,  517,  517,  517,  517,  517,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   517,  71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,   71,
    71,   71,   71,   71,   3112, 3112, 3112, 3112, 3112, 3112, 3112, 3112,
    3112, 3112, 517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  1162, 517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  1162, 517,  517,  517,  517,  517,  517,  1162, 517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  1162, 517,  517,  1162, 517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  1162, 517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  1162, 517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  1162, 517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  1162, 517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  1162, 517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162, 1162,
    1162, 1162, 517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  1162, 517,  517,  517,  517,  517,
    1162, 517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  1162,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  75,   517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  75,   75,   75,   75,   75,   75,   75,   75,
    75,   75,   75,   75,   75,   75,   75,   75,   75,   75,   75,   75,
    75,   75,   75,   75,   75,   75,   75,   75,   75,   75,   75,   75,
    75,   75,   75,   75,   75,   75,   75,   75,   75,   75,   75,   75,
    75,   75,   75,   75,   75,   75,   75,   75,   75,   75,   75,   75,
    75,   75,   75,   75,   75,   75,   75,   75,   75,   75,   75,   75,
    75,   75,   75,   75,   75,   75,   75,   75,   75,   75,   75,   75,
    75,   75,   75,   75,   75,   75,   75,   75,   75,   75,   75,   75,
    75,   75,   75,   75,   508,  508,  508,  508,  508,  508,  508,  508,
    508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,
    508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,
    508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,
    508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,
    508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,
    508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,
    508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,
    508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,
    508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,
    508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,
    508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,
    508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,
    508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,
    508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,
    508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,
    508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,
    508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,
    508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,
    508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,  508,
    508,  508,  508,  508,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,  517,
    517,  2502, 517,  517,
};

uint8_t utf8_property_page_offsets[4352] = {
    0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   10,  11,  12,  13,  14,
    15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,
    30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,
    45,  46,  47,  48,  49,  50,  51,  52,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  54,  52,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  55,  56,  57,  57,  57,  58,
    21,  59,  60,  61,  62,  63,  64,  52,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  65,  66,  53,  53,  67,  66,  53,  53,  68,  69,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  70,  57,  71,  72,  73,  74,  75,
    76,  77,  78,  79,  80,  81,  82,  21,  83,  84,  85,  86,  87,  88,  89,
    90,  91,  92,  93,  94,  95,  96,  97,  98,  99,  100, 101, 102, 103, 104,
    105, 106, 107, 21,  21,  21,  108, 109, 110, 53,  53,  53,  53,  53,  53,
    53,  53,  53,  111, 21,  21,  21,  21,  112, 21,  21,  21,  21,  21,  21,
    21,  21,  21,  21,  21,  21,  21,  21,  113, 21,  21,  114, 53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  115, 53,  53,  53,  53,  53,  53,
    21,  21,  116, 117, 53,  118, 119, 120, 52,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  121, 57,  57,  57,  57,  122, 123, 53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  124, 57,  125, 126,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  127, 53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  128, 129, 130, 131, 132,
    133, 134, 135, 136, 137, 138, 139, 40,  40,  140, 53,  53,  53,  53,  141,
    142, 143, 144, 53,  145, 146, 53,  147, 148, 149, 53,  53,  150, 151, 152,
    53,  153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 53,  53,
    53,  53,  52,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  165, 52,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  166, 167, 53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  168, 53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  169, 53,  53,
    170, 53,  53,  53,  53,  53,  53,  53,  53,  53,  57,  57,  171, 53,  53,
    53,  53,  53,  52,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  172, 53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  173, 53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  174,
    175, 53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    69,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    176, 69,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,  53,
    53,  176,
};

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
        // .filepath = argv[1],
    };

    if (argc > 1) {
        strncpy(window.filepath, argv[1], SFCE_FILEPATH_MAX);

        error_code = sfce_piece_tree_load_file(tree, window.filepath);
        if (error_code != SFCE_ERROR_OK && error_code != SFCE_ERROR_UNABLE_TO_OPEN_FILE) {
            goto error;
        }
    }

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
            // g_should_log_to_error_string = SFCE_TRUE;
            sfce_cursor_move_right(window.cursors);
            // g_should_log_to_error_string = SFCE_FALSE;

            // int32_t position_offset = sfce_piece_tree_offset_at_position(window.tree, window.cursors->position);
            // struct sfce_node_position node_position = sfce_piece_tree_node_at_position(window.tree, window.cursors->position.col, window.cursors->position.row);
            // int32_t character_length = sfce_piece_tree_character_length_at_node_position(window.tree, node_position);
            // window.cursors->position = sfce_piece_tree_position_at_offset(window.tree, position_offset + character_length);

            break;
        }

        // case SFCE_KEYCODE_ARROW_LEFT: {
        //     should_render = SFCE_TRUE;

        //     struct sfce_node_position node_position = sfce_piece_tree_node_at_position(
        //         window.tree,
        //         window.cursors->position.col,
        //         window.cursors->position.row
        //     );

        //     sfce_node_position_move_by_offset(node_position, -1);

        //     uint8_t byte;
        //     do {
        //         sfce_node_position_move_by_offset(node_position, -1);
        //         byte = sfce_piece_tree_byte_at_node_position(window.tree, node_position);
        //     } while (sfce_codepoint_utf8_continuation(byte));

        //     window.cursors->position = sfce_piece_tree_position_at_offset(window.tree, node_position.node_start_offset + node_position.offset_within_piece);

        //     // int32_t position_offset = sfce_piece_tree_offset_at_position(window.tree, window.cursors->position);
        //     // window.cursors->position = sfce_piece_tree_position_at_offset(window.tree, position_offset - 1);

        //     break;
        // }

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
            if (error_code != SFCE_ERROR_OK) {
                goto error;
            }

            sfce_cursor_move_right(window.cursors);

            // uint8_t buffer[4] = {};
            // int32_t buffer_length = sfce_codepoint_encode_utf8(keypress.codepoint, buffer);
            // error_code = sfce_piece_tree_insert_with_position(tree, window.cursors->position, buffer, buffer_length);
            // int32_t codepoint_length = sfce_codepoint_utf8_byte_count(keypress.codepoint);
            // int32_t position_offset = sfce_piece_tree_offset_at_position(window.tree, window.cursors->position);
            // window.cursors->position = sfce_piece_tree_position_at_offset(window.tree, position_offset + codepoint_length);
            // if (error_code != SFCE_ERROR_OK) {
            //     goto error;
            // }

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

            // g_should_log_to_error_string = 0;
            error_code = sfce_editor_window_display(&window, &console, &line_contents);
            // g_should_log_to_error_string = 1;
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

const struct sfce_utf8_property *sfce_codepoint_utf8_property_unchecked(int32_t codepoint)
{
    uint32_t page_offset = utf8_property_page_offsets[codepoint >> 8] << 8;
    uint32_t index = utf8_property_indices[page_offset + (codepoint & 0xFF)];
    return &utf8_properties[index];
}

const struct sfce_utf8_property *sfce_codepoint_utf8_property(int32_t codepoint)
{
    if (codepoint > 0x10FFFF || codepoint < 0x000000) {
        return &default_utf8_property;
    }

    return sfce_codepoint_utf8_property_unchecked(codepoint);
}

enum sfce_unicode_category sfce_codepoint_category(int32_t codepoint)
{
    return sfce_codepoint_utf8_property(codepoint)->category;
}

int32_t sfce_codepoint_to_upper(int32_t codepoint)
{
    const struct sfce_utf8_property *property = sfce_codepoint_utf8_property(codepoint);

    if (property->uppercase_mapping != -1) {
        return property->uppercase_mapping;
    }

    return codepoint;
}

int32_t sfce_codepoint_to_lower(int32_t codepoint)
{
    const struct sfce_utf8_property *property = sfce_codepoint_utf8_property(codepoint);

    if (property->lowercase_mapping != -1) {
        return property->lowercase_mapping;
    }

    return codepoint;
}

uint8_t sfce_codepoint_width(int32_t codepoint)
{
    return sfce_codepoint_utf8_property(codepoint)->width;
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
#ifdef DEBUG_CHARACTERS
    if (codepoint == '\n' || codepoint == '\r') {
        // sfce_log_error("Found newline codepoint: %02x\n", codepoint);
        return SFCE_FALSE;
    }
#endif

    enum sfce_unicode_category category = sfce_codepoint_category(codepoint);

    switch (category) {
    case SFCE_UNICODE_CATEGORY_LL:
    case SFCE_UNICODE_CATEGORY_LM:
    case SFCE_UNICODE_CATEGORY_LO:
    case SFCE_UNICODE_CATEGORY_LT:
    case SFCE_UNICODE_CATEGORY_LU:
        return SFCE_TRUE;
    case SFCE_UNICODE_CATEGORY_ND:
    case SFCE_UNICODE_CATEGORY_NL:
    case SFCE_UNICODE_CATEGORY_NO:
        return SFCE_TRUE;
    case SFCE_UNICODE_CATEGORY_MC:
    case SFCE_UNICODE_CATEGORY_ME:
    case SFCE_UNICODE_CATEGORY_MN:
        return SFCE_TRUE;
    case SFCE_UNICODE_CATEGORY_PC:
    case SFCE_UNICODE_CATEGORY_PD:
    case SFCE_UNICODE_CATEGORY_PE:
    case SFCE_UNICODE_CATEGORY_PF:
    case SFCE_UNICODE_CATEGORY_PI:
    case SFCE_UNICODE_CATEGORY_PO:
    case SFCE_UNICODE_CATEGORY_PS:
        return SFCE_TRUE;
    case SFCE_UNICODE_CATEGORY_SC:
    case SFCE_UNICODE_CATEGORY_SK:
    case SFCE_UNICODE_CATEGORY_SM:
    case SFCE_UNICODE_CATEGORY_SO:
        return SFCE_TRUE;
    case SFCE_UNICODE_CATEGORY_ZS:
        return SFCE_TRUE;
    default:
        return SFCE_FALSE;
    }

    return SFCE_FALSE;
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
    int32_t size_to_allocate = formatted_string_size < max_length ? formatted_string_size : max_length;

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
        if (position.offset_within_piece < 0) {
            struct sfce_piece_node *prev = sfce_piece_node_prev(position.node);
            if (prev == sentinel_ptr) {
                position.node_start_offset = 0;
                position.offset_within_piece = 0;
                return position;
            }

            position.offset_within_piece += prev->piece.length;
            position.node_start_offset -= prev->piece.length;
            position.node = prev;
        }
        else if (position.offset_within_piece > position.node->piece.length) {
            struct sfce_piece_node *next = sfce_piece_node_next(position.node);
            if (next == sentinel_ptr) {
                position.offset_within_piece = position.node->piece.length;
                return position;
            }

            position.offset_within_piece -= position.node->piece.length;
            position.node_start_offset += position.node->piece.length;
            position.node = next;
        }
        else {
            return position;
        }
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

int32_t sfce_piece_tree_codepoint_at_node_position(struct sfce_piece_tree *tree, struct sfce_node_position start)
{
    uint8_t bytes[4] = {};
    struct sfce_node_position end = sfce_node_position_move_by_offset(start, 4);
    int32_t length = sfce_piece_tree_read_into_buffer(tree, start, end, 4, bytes);
    int32_t codepoint = sfce_codepoint_decode_utf8(bytes, length);

    return codepoint;
}

int32_t sfce_piece_tree_codepoint_at_position(struct sfce_piece_tree *tree, int32_t col, int32_t row)
{
    struct sfce_node_position node_position = sfce_piece_tree_node_at_position(tree, col, row);
    return sfce_piece_tree_codepoint_at_node_position(tree, node_position);
}

int32_t sfce_piece_tree_codepoint_at_offset(struct sfce_piece_tree *tree, int32_t offset)
{
    struct sfce_node_position node_position = sfce_piece_tree_node_at_offset(tree, offset);
    return sfce_piece_tree_codepoint_at_node_position(tree, node_position);
}

int32_t sfce_piece_tree_character_length_at_node_position(struct sfce_piece_tree *tree, struct sfce_node_position start)
{
    uint8_t bytes[4] = {};
    struct sfce_node_position end = sfce_node_position_move_by_offset(start, 4);
    int32_t length = sfce_piece_tree_read_into_buffer(tree, start, end, 4, bytes);
    int32_t codepoint = sfce_codepoint_decode_utf8(bytes, length);

    int32_t newline_size = newline_sequence_size(bytes, 4);
    if (newline_size != 0) return newline_size;

    return sfce_codepoint_utf8_byte_count(codepoint);
}

int32_t sfce_piece_tree_get_line_length(struct sfce_piece_tree *tree, int32_t row)
{
    int32_t offset0 = sfce_piece_tree_offset_at_position(tree, (struct sfce_position) { 0, row });
    int32_t offset1 = sfce_piece_tree_offset_at_position(tree, (struct sfce_position) { 0, row + 1 });
    return offset1 - offset0;
}

int32_t sfce_piece_tree_get_line_length_without_newline(struct sfce_piece_tree *tree, int32_t row)
{
    uint8_t buffer[4] = {};
    struct sfce_node_position node0 = sfce_piece_tree_node_at_position(tree, 0, row);
    struct sfce_node_position node1 = sfce_piece_tree_node_at_position(tree, 0, row + 1);

    int32_t offset0 = node0.node_start_offset + node0.offset_within_piece;
    int32_t offset1 = node1.node_start_offset + node1.offset_within_piece;
    int32_t line_length_with_newline = offset1 - offset0;

    int32_t backwards_advance = line_length_with_newline > 1 ? 2 : 1;
    struct sfce_node_position start = sfce_node_position_move_by_offset(node1, -backwards_advance);

    int32_t length = sfce_piece_tree_read_into_buffer(tree, start, node1, backwards_advance, buffer);
    int32_t newline_length0 = newline_sequence_size(buffer, length);
    int32_t newline_length1 = newline_sequence_size(buffer + 1, length - 1);
    int32_t newline_length = newline_length0 > newline_length1 ? newline_length0 : newline_length1;

    return line_length_with_newline - newline_length;
}

uint8_t sfce_piece_tree_byte_at_node_position(struct sfce_piece_tree *tree, struct sfce_node_position node_position)
{
    struct sfce_string_buffer *string_buffer = &tree->buffers[node_position.node->piece.buffer_index];
    int32_t offset0 = sfce_string_buffer_position_to_offset(string_buffer, node_position.node->piece.start);
    int32_t offset1 = sfce_string_buffer_position_to_offset(string_buffer, node_position.node->piece.end);

    if (offset0 + node_position.offset_within_piece < offset1) {
        return string_buffer->content.data[offset0 + node_position.offset_within_piece];
    }

    return 0;
}

int32_t sfce_piece_tree_read_into_buffer(struct sfce_piece_tree *tree, struct sfce_node_position start, struct sfce_node_position end, int32_t buffer_size, uint8_t *buffer)
{
    int32_t bytes_written = 0;
    struct sfce_string_view piece_content = {};

    if (start.node == end.node) {
        bytes_written = end.offset_within_piece - start.offset_within_piece;
        piece_content = sfce_piece_tree_get_piece_content(tree, start.node->piece);
        bytes_written = bytes_written < buffer_size ? bytes_written : buffer_size;
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

        struct sfce_piece_node *node = sfce_piece_node_next(start.node);
        while (remaining != 0 && node != end.node && node != sentinel_ptr) {
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
        int32_t codepoint = sfce_piece_tree_codepoint_at_node_position(tree, node_position);
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
    struct sfce_node_position node_position0 = sfce_piece_tree_node_at_position(tree, 0, row);
    struct sfce_node_position node_position1 = sfce_piece_tree_node_at_position(tree, col, row);

    int32_t render_column = 0;
    while (node_position0.node_start_offset + node_position0.offset_within_piece < node_position1.node_start_offset + node_position1.offset_within_piece) {
        int32_t codepoint = sfce_piece_tree_codepoint_at_node_position(tree, node_position0);
        int32_t byte_count = sfce_codepoint_utf8_byte_count(codepoint);
        node_position0 = sfce_node_position_move_by_offset(node_position0, byte_count);

#ifdef DEBUG_CHARACTERS
        if (!sfce_codepoint_is_print(codepoint)) {
            render_column += strlen(make_character_printable(codepoint));
        }
        else {
            render_column += sfce_codepoint_width(codepoint);
        }
#else
        render_column += sfce_codepoint_width(codepoint);
#endif
    }

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
#ifdef DEBUG_CHARACTERS
            goto print_character;
#endif
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
#ifdef DEBUG_CHARACTERS
            if (!sfce_codepoint_is_print(codepoint)) {
print_character:
                const char *codepoint_string = make_character_printable(codepoint);
                int32_t     codepoint_string_length = strlen(codepoint_string);

                error_code = sfce_console_buffer_print_string(
                    console,
                    position.col,
                    position.row,
                    style,
                    codepoint_string,
                    codepoint_string_length
                );

                if (error_code != SFCE_ERROR_OK) {
                    return error_code;
                }

                position.col += codepoint_string_length;
                iter += codepoint_byte_count;
            }
            else {
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
#else
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
#endif
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
        sfce_editor_window_remove_from_parent(window);
        sfce_editor_window_destroy(window->window0);
        sfce_editor_window_destroy(window->window1);

        while (window->cursors != NULL) {
            sfce_cursor_destroy(window->cursors);
        }

        // sfce_string_destroy(&window->filepath);
        sfce_string_destroy(&window->status_message);

        sfce_piece_tree_destroy(window->tree);
        // free(window);
    }
}

void sfce_editor_window_remove_from_parent(struct sfce_editor_window *window)
{
    if (window->parent != NULL) {
        if (window->parent->window0 == window) {
            window->parent->window0 = NULL;
        }
        else {
            window->parent->window1 = NULL;
        }
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

    const char *filepath = *window->filepath == 0 ? "[Untitled File]" : window->filepath;

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
    // struct sfce_node_position cursor_node_position = sfce_piece_tree_node_at_position(window->tree, cursor_position.col, cursor_position.row);
    // int32_t codepoint = sfce_piece_tree_codepoint_at_offset(window->tree, cursor_offset);
    // int32_t character = sfce_piece_tree_byte_at_node_position(window->tree, cursor_node_position);
    // int32_t line_length = sfce_piece_tree_get_line_length(window->tree, cursor_position.row);
    // int32_t line_length_no_newline = sfce_piece_tree_get_line_length_without_newline(window->tree, cursor_position.row);

    sfce_string_clear(temp_string);
    // sfce_string_nprintf(temp_string, INT32_MAX, "%.*s  ", filepath.size, filepath.data);
    sfce_string_nprintf(temp_string, INT32_MAX, "%s  ", filepath);
    sfce_string_nprintf(temp_string, INT32_MAX, "Col %d ", cursor_position.col);
    sfce_string_nprintf(temp_string, INT32_MAX, "Row %d ", cursor_position.row);
    sfce_string_nprintf(temp_string, INT32_MAX, "Offset %d ", cursor_offset);
    sfce_string_nprintf(temp_string, INT32_MAX, "Length: %d ", window->tree->length);
    sfce_string_nprintf(temp_string, INT32_MAX, "Line Count: %d ", window->tree->line_count);
    // sfce_string_nprintf(temp_string, INT32_MAX, "Codepoint: %08x ", codepoint);
    // sfce_string_nprintf(temp_string, INT32_MAX, "Character: %02x ", character);
    // sfce_string_nprintf(temp_string, INT32_MAX, "Line Length: %d ", line_length);
    // sfce_string_nprintf(temp_string, INT32_MAX, "NoNewline: %d ", line_length_no_newline);

    sfce_console_buffer_print_string(console, window->rectangle.left, window->rectangle.bottom, status_style, temp_string->data, temp_string->size);
    // sfce_console_buffer_set_style(console, line_contents_start + cursor_position.col, window->rectangle.top + cursor_position.row, cursor_style);
    sfce_console_buffer_set_style(console, line_contents_start + window->cursors->target_render_col, window->rectangle.top + window->cursors->position.row, cursor_style);

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

void sfce_cursor_move_left(struct sfce_cursor *cursor)
{
    if (cursor->position.col > 0) {
        cursor->position.col -= 1;
    }
    else if (cursor->position.row != 0) {
        cursor->position.row = cursor->position.row - 1;
        cursor->position.col = sfce_piece_tree_get_line_length(
            cursor->window->tree,
            cursor->position.row
        );
    }

    cursor->target_render_col = sfce_piece_tree_get_render_column_from_column(
        cursor->window->tree,
        cursor->position.row,
        cursor->position.col
    );
}

void sfce_cursor_move_right(struct sfce_cursor *cursor)
{
    struct sfce_editor_window *window = cursor->window;
    int32_t line_byte_count = sfce_piece_tree_get_line_length_without_newline(
        window->tree,
        cursor->position.row
    );

    if (cursor->position.col < line_byte_count) {
        struct sfce_node_position node_position = sfce_piece_tree_node_at_position(
            cursor->tree,
            cursor->position.col,
            cursor->position.row
        );

        cursor->position.col += sfce_piece_tree_character_length_at_node_position(cursor->tree, node_position);
        cursor->target_render_col = sfce_piece_tree_get_render_column_from_column(
            window->tree,
            cursor->position.row,
            cursor->position.col
        );
    }
    else if (cursor->position.row + 1 < window->tree->line_count) {
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

/*
void sfce_cursor_destroy(struct sfce_cursor *cursor)
{
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
