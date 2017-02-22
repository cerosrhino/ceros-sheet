#include <ncurses.h>

// spreadsheet size (SIZE * SIZE cells)
#define SIZE 26

// ASCII offsets
#define ALPHA_BASE 0x41
#define DIGIT_BASE 0x30

// symbols for the types available
#define TYPE_AUTO '?'
#define TYPE_INT 'I'
#define TYPE_FLOAT 'F'
#define TYPE_TEXT 'T'
#define TYPE_ERROR 'E'
#define TYPE_RANGE 'R'

#define WRAP(num, max) (((num % max) + max) % max)
// cell access helper macro
#define CELL(x, y) cells[WRAP(y, SIZE)][WRAP(x, SIZE)]
// allocate exactly as many bytes as needed and store the value
#define ALLOC_SPRINTF(ptr, format, val) \
        ptr = malloc(snprintf(NULL, 0, format, val) + 1); \
        sprintf(ptr, format, val);

// macros operating on the Value structure, different views of underlying data
#define AS_INT(value) value.data.integer
#define AS_FLOAT(value) value.data.fp
#define AS_TEXT(value) value.data.text
#define AS_RANGE(value) value.data.range

// error code support, utilizes the integer field
#define SET_ERROR(value, code) value.type = TYPE_ERROR;\
                               value.data.integer = code;
#define GET_ERROR(value) AS_INT(value)
#define NUM_ERRORS 9
#define ERROR_GENERAL 0
#define ERROR_TOO_MANY_ARGS 1
#define ERROR_TOO_FEW_ARGS 2
#define ERROR_BAD_ARG 3
#define ERROR_EMPTY 4
#define ERROR_DIV_0 5
#define ERROR_OUT_OF_BOUNDS 6
#define ERROR_CYCLE 7
#define ERROR_NO_SUCH_FUNC 8

#define PRINTABLE_ASCII_START 32
#define PRINTABLE_ASCII_END 126

// no formula can be longer than this (though values can have indefinite length)
#define FORMULA_LENGTH 70

// maximum range of text (cell value) visible without scrolling
#define VISIBLE_TEXT_LENGTH 70

// ditto, only for the file save dialog
#define VISIBLE_FILE_NAME_LENGTH 42

// magic number
#define FILE_SIGNATURE "WSSHEET\x01"

// language support (English, Polish)
#define NUM_STRINGS 7
#define STRING_FORMULA 0
#define STRING_VALUE 1
#define STRING_SAVE_PROMPT 2
#define STRING_SAVE_CANCEL 3
#define STRING_SAVE_QUIT 4
#define STRING_SAVE_CONFIRM 5
#define STRING_BAD_FILE_NAME 6
#define LANG_EN 0
#define LANG_PL 1

// the Value type - stores values of type INT, FLOAT, TEXT, ERROR and RANGE
typedef struct {
    char type;
    union {
        char *text;
        long long integer;
        double fp;
        struct {
            char x1, x2, y1, y2;
        } range;
    } data;
} Value;

// this is used for the list of references to destination cells
// (so that they can be updated when one or more dependencies change)
typedef struct _RefNode {
    unsigned x, y;
    struct _RefNode *next;
} RefNode;

// this stores all data associated with a cell
typedef struct {
    int x, y;
    char formula[FORMULA_LENGTH];
    char *text;
    size_t textScroll;
    char view[10];
    char type;
    char curType;
    char errorCode;
    RefNode *refs;
    WINDOW *pad;
} Cell;

// this parses the formula
Value parse(const char *, unsigned, unsigned, bool);
// these operate on references to "destination" cells
Cell *addCellRef(unsigned, unsigned, unsigned, unsigned);
void removeCellRef(unsigned, unsigned, unsigned, unsigned);
// these serve as an interface between "main" and "parser"
char *getCellText(unsigned, unsigned);
char getCellType(unsigned, unsigned);
int getCellErrorCode(unsigned, unsigned);
