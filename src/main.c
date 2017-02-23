#include "sheet.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

Cell cells[SIZE][SIZE]; // this stores all cells' data
int curX = 0, curY = 0; // cursor coordinates (cell selection)
char language = LANG_EN;

// this is needed to allow the user to force types for cells
char types[] = {TYPE_AUTO, TYPE_INT, TYPE_FLOAT, TYPE_TEXT};

// language support (English, Polish)
char *strings[2][NUM_STRINGS] = {
    {
        "Formula", "Value",
        "Save as:", "^Q to cancel", "^Q to quit",
        "^S or ENTER to save", "Incorrect file name."
    },
    {
        "Formula", "Wartosc",
        "Zapisz jako:", "^Q by anulowac", "^Q by wyjsc",
        "^S lub ENTER by zapisac", "Niepoprawna nazwa pliku."
    }
};
char *errors[2][NUM_ERRORS] = {
    {
        "INCORRECT FORMULA!", "TOO MANY ARGUMENTS!", "TOO FEW ARGUMENTS!",
        "INCORRECT ARGUMENT!", "TOO FEW ARGUMENTS!", "DIVISION BY ZERO!",
        "OUT OF BOUNDS!", "INFINITE CYCLE!", "NO SUCH FUNCTION!"
    },
    {
        "BLEDNA FORMULA!", "ZA DUZO ARGUMENTOW!", "ZA MALO ARGUMENTOW!",
        "NIEPOPRAWNY ARGUMENT!", "ZA MALO ARGUMENTOW!", "DZIELENIE PRZEZ ZERO!",
        "WYJSCIE POZA ZAKRES!", "NIESKONCZONY CYKL!", "NIEISTNIEJACA FUNKCJA!"
    }
};

void initCell(Cell *cell, WINDOW *pad, unsigned x, unsigned y) {
    cell->pad = subpad(pad, 1, 9, y, x * 9);
    memset(cell->formula, '\0', FORMULA_LENGTH);
    cell->text = malloc(1);
    cell->text[0] = '\0';
    cell->textScroll = 0;
    cell->x = x;
    cell->y = y;
    cell->type = TYPE_AUTO;
    cell->curType = 0;
    cell->refs = NULL;
}

// link the "source" and "destination" cells to handle dependencies;
// this is required so that dependent cells can be updated automatically
Cell *addCellRef(unsigned srcX, unsigned srcY, unsigned dstX, unsigned dstY) {
    Cell *src = &(CELL(srcX, srcY));
    RefNode *cur = src->refs;
    while (cur != NULL && cur->next != NULL) {
        if (cur->x == dstX && cur->y == dstY) {
            return NULL;
        }
        cur = cur->next;
    }
    RefNode *newRef = malloc(sizeof(RefNode));
    newRef->x = dstX;
    newRef->y = dstY;
    newRef->next = NULL;
    
    if (cur == NULL) {
        src->refs = newRef;
    } else {
        cur->next = newRef;
    }

    return src;
}

void removeCellRef(unsigned srcX, unsigned srcY, unsigned dstX, unsigned dstY) {
    Cell *src = &(CELL(srcX, srcY));
    RefNode *cur = src->refs;
    if (cur != NULL && cur->x == dstX && cur->y == dstY) {
        RefNode *next = cur->next;
        free(cur);
        src->refs = next;
        return;
    }
    while (cur != NULL && cur->next != NULL) {
        RefNode *next = cur->next->next;
        if (cur->next->x == dstX && cur->next->y == dstY) {
            free(cur->next);
            cur->next = next;
            return;
        }
        cur->next = next;
    }
}

void updateCell(Cell *, char *, bool);

// "rozwiazanie" odwolania do komorki docelowej, aktualizacja
// komorek zaleznych
void resolveRef(Cell *cell) {
    RefNode *cur = cell->refs;
    // iteracja po komorkach zaleznych i ich aktualizacja
    while (cur != NULL) {
        Cell *dst = &(CELL(cur->x, cur->y));
        Cell *sel = &(CELL(curX, curY));
        if (cur->x == curX && cur->y == curY) {
            // cycle - return relevant error code
            sel->errorCode = ERROR_CYCLE;
            free(sel->text);
            ALLOC_SPRINTF(sel->text, "%s", errors[language][ERROR_CYCLE]);
            strncpy(sel->view, sel->text, 9);
            mvwprintw(sel->pad, 0, 0, "%-9s", sel->view);
            sel->type = TYPE_ERROR;
            mvwaddch(sel->pad, 0, 8, TYPE_ERROR | COLOR_PAIR(4));
        } else {
            updateCell(dst, dst->formula, FALSE);
        }
        cur = cur->next;
    }
}

// refresh cell value
void updateCell(Cell *cell, char *formula, bool manual) {
    char *output;
    
    // input formula is parsed here
    Value value = parse(formula, cell->x, cell->y, manual);

    // if an error was thrown, "freeze" the cell;
    // input needs to be entered again
    if (value.type == TYPE_ERROR) {
        cell->type = TYPE_ERROR;
        cell->errorCode = GET_ERROR(value);
    } else {
        cell->type = types[cell->curType];
    }

    // return properly formatted data as text
    if (value.type == TYPE_INT) {
        ALLOC_SPRINTF(output, "%lld", AS_INT(value));
    } else if (value.type == TYPE_FLOAT) {
        ALLOC_SPRINTF(output, "%.3lf", AS_FLOAT(value));
    } else if (value.type == TYPE_TEXT) {
        char *text = AS_TEXT(value);
        ALLOC_SPRINTF(output, "%s", text);
        free(text);
    } else if (value.type == TYPE_ERROR) {
        ALLOC_SPRINTF(output, "%s", errors[language][GET_ERROR(value)]);
    }

    // populate cell fields based on the received text
    if (cell->formula != formula) {
        strcpy(cell->formula, formula);
    }
    cell->text = realloc(cell->text, strlen(output) + 1);
    strcpy(cell->text, output);
    strncpy(cell->view, output, 9);
    // print the new value on the screen
    mvwprintw(cell->pad, 0, 0, "%-9s", cell->view);
    // draw a green "type" box if the type differs from AUTO
    if (cell->type != TYPE_AUTO) {
        mvwaddch(cell->pad, 0, 8, cell->type | COLOR_PAIR(4));
    }

    free(output);

    cell->textScroll = fmin(fmax(strlen(cell->text) - VISIBLE_TEXT_LENGTH, 0),
                            cell->textScroll);

    // update dependent cells
    resolveRef(cell);
}

// ncurses-related stuff
void refreshPads(WINDOW *pad, WINDOW *cols, WINDOW *rows,
                 int scrollX, int scrollY) {
    prefresh(rows, scrollY, 0, 3, 0, fmin(SIZE + 2, LINES - 1), 7);
    prefresh(cols, 0, scrollX * 9, 2, 8, 2, 79);
    prefresh(pad, scrollY, scrollX * 9, 3, 8, fmin(SIZE + 2, LINES - 1), 79);
}

// cell selection (refresh needed because colors must be updated properly)
void selectCell(Cell deselect, Cell select,
                WINDOW *pad, WINDOW *cols, WINDOW *rows,
                char *formula, unsigned *index) {
    static int scrollX = 0, scrollY = 0;

    // reset formatting on the cell being deselected
    mvwprintw(deselect.pad, 0, 0, "%-9s", deselect.view);

    // if the deselected cell's type differs from AUTO, draw a green box
    // denoting the forced type
    if (deselect.type != TYPE_AUTO) {
        mvwaddch(deselect.pad, 0, 8, deselect.type | COLOR_PAIR(4));
    }

    // draw the reference row (numeric indices) and column (alphabetic) bars
    wattron(rows, COLOR_PAIR(deselect.y % 2 + 1));
    mvwprintw(rows, deselect.y, 0, "   %2d   ", deselect.y + 1);
    wattroff(rows, COLOR_PAIR(deselect.y % 2 + 1));

    wattron(cols, COLOR_PAIR(deselect.x % 2 + 1));
    mvwprintw(cols, 0, deselect.x * 9,
              "    %c    ", ALPHA_BASE + deselect.x);
    wattroff(cols, COLOR_PAIR(deselect.x % 2 + 1));

    wattron(rows, COLOR_PAIR(3));
    wattron(cols, COLOR_PAIR(3));
    mvwprintw(rows, select.y, 0, "   %2d   ", select.y + 1);
    mvwprintw(cols, 0, select.x * 9,
              "    %c    ", ALPHA_BASE + select.x);
    wattroff(rows, COLOR_PAIR(3));
    wattroff(cols, COLOR_PAIR(3));

    // print the selected cell's value in proper colors
    wattron(select.pad, COLOR_PAIR(3));
    mvwprintw(select.pad, 0, 0, "%-9s", select.view);
    wattroff(select.pad, COLOR_PAIR(3));

    // if the deselected cell's type differs from AUTO, draw a green box
    // denoting the forced type
    if (select.type != TYPE_AUTO) {
        mvwaddch(select.pad, 0, 8, select.type | COLOR_PAIR(4));
    }

    // copy the local (cell) formula to the global formula
    strcpy(formula, select.formula);
    *index = strlen(formula); // text cursor position
    // print formula and value
    mvprintw(0, 0, "%7s: %-71s", strings[language][STRING_FORMULA], formula);
    mvprintw(1, 0, "%7s: %-71.70s",
             strings[language][STRING_VALUE], select.text + select.textScroll);
    if (strlen(select.text) > VISIBLE_TEXT_LENGTH) {
        if (select.textScroll > 0) {
            mvaddch(1, 8, '<' | COLOR_PAIR(4));
        }
        if (select.textScroll < strlen(select.text) - VISIBLE_TEXT_LENGTH) {
            mvaddch(1, 79, '>' | COLOR_PAIR(4));
        }
    }
    // move text cursor to proper position
    move(0, 9 + strlen(formula));

    // sheet scrolling
    if (select.x == 0) {
        scrollX = 0;
    } else if (select.x == SIZE - 1) {
        scrollX = 18;
    } else {
        int diffX = select.x - scrollX;
        if (diffX == 6 && select.x != SIZE - 2) {
            scrollX++;
        } else if (diffX == 1) {
            scrollX--;
        }
    }

    if (select.y == 0) {
        scrollY = 0;
    } else if (select.y == SIZE - 1) {
        scrollY = fmax(29 - LINES, 0);
    } else {
        int diffY = select.y - scrollY;
        if (diffY == LINES - 7 && select.y != SIZE - 4) {
            scrollY++;
        } else if (diffY == 3) {
            scrollY--;
        }
    }

    refreshPads(pad, cols, rows, scrollX, scrollY);
}

char *getCellText(unsigned x, unsigned y) {
    char *text = malloc(strlen(CELL(x, y).text) + 1);
    strcpy(text, CELL(x, y).text);
    return text;
}

char getCellType(unsigned x, unsigned y) {
    return CELL(x, y).type;
}

int getCellErrorCode(unsigned x, unsigned y) {
    return CELL(x, y).errorCode;
}

void loadFile(char *fileName) {
    if (fileName != NULL) {
        FILE *file = fopen(fileName, "rb");
        if (file != NULL) {
            char signature[9];
            char *result = fgets(signature, 9, file);
            if (result != NULL && strcmp(signature, FILE_SIGNATURE) == 0) {
                /*curX = */fgetc(file);
                /*curY = */fgetc(file);
                while (TRUE) {
                    curX = fgetc(file);
                    if (curX == EOF) {
                        break;
                    }
                    curY = fgetc(file);
                    int type = fgetc(file);
                    int curType = fgetc(file);
                    Cell *cell = &(CELL(curX, curY));
                    cell->type = type;
                    cell->curType = curType;
                    fscanf(file, "%zu", &cell->textScroll);
                    fgetc(file);
                    size_t length;
                    fscanf(file, "%zu", &length);
                    fgetc(file);
                    fgets(cell->formula, length + 1, file);
                    updateCell(cell, cell->formula, TRUE);
                }
            }

            fclose(file);
        }
    }
    curX = 0;
    curY = 0;
    // updateCell(&(CELL(curX, curY)), CELL(curX, curY).formula, TRUE);
}

bool saveFile(char *defaultName, bool recover, bool quit) {
    static char *lastFileName = NULL;
    char *fileName;
    size_t index = 0;
    if (lastFileName != NULL) {
        fileName = malloc(strlen(lastFileName) + 1);
        strcpy(fileName, lastFileName);
    } else if (defaultName != NULL) {
        fileName = malloc(strlen(defaultName) + 1);
        strcpy(fileName, defaultName);
    } else {
        fileName = malloc(1);
        fileName[0] = '\0';
    }
    index = strlen(fileName);
    size_t scroll = fmax(strlen(fileName) - VISIBLE_FILE_NAME_LENGTH, 0);
    
    WINDOW *saveWin = newwin(7, 50, (LINES - 7) / 2, (COLS - 50) / 2);
    keypad(saveWin, TRUE);
    box(saveWin, 0, 0);
    wmove(saveWin, 2, 4);
    if (recover) {
        wprintw(saveWin, "%s ", strings[language][STRING_BAD_FILE_NAME]);
    }
    wprintw(saveWin, strings[language][STRING_SAVE_PROMPT]);
    int msg = quit ? STRING_SAVE_QUIT : STRING_SAVE_CANCEL;
    mvwprintw(saveWin, 4, 4, strings[language][msg]);
    wprintw(saveWin, ", %s", strings[language][STRING_SAVE_CONFIRM]);
    mvwprintw(saveWin, 3, 4, "%-42.42s", fileName + scroll);
    wrefresh(saveWin);

    int ch;
    bool stop = FALSE;
    if (strlen(fileName) > VISIBLE_FILE_NAME_LENGTH) {
        mvwaddch(saveWin, 3, 3, '<' | COLOR_PAIR(4));
    }
    wmove(saveWin, 3, fmin(4 + index, 46));
    while (!stop && (ch = wgetch(saveWin))) {
        switch (ch) {
            // cancel action
            case 17: // ^Q
                delwin(saveWin);
                free(fileName);
                if (quit) {
                    free(lastFileName);
                }
                return TRUE;
            // confirm action
            case 19: // ^S
            case KEY_ENTER:
            case '\r':
            case '\n':
                if (strlen(fileName) > 0) {
                    stop = TRUE;
                }
                break;
            // handle backspace as expected by the user
            case KEY_BACKSPACE:
                if (index > 0) {
                    if (scroll > 0) {
                        scroll--;
                    }
                    fileName[--index] = '\0';
                }
                break;
            // other keys - user input
            default:
                if (ch >= PRINTABLE_ASCII_START &&
                    ch <= PRINTABLE_ASCII_END) {
                        fileName = realloc(fileName, strlen(fileName) + 2);
                        fileName[index++] = ch;
                        fileName[index] = '\0';
                        if (index > VISIBLE_FILE_NAME_LENGTH) {
                            scroll++;
                        }
                }
                break;
        }
        if (strlen(fileName) > VISIBLE_FILE_NAME_LENGTH && scroll > 0) {
            mvwaddch(saveWin, 3, 3, '<' | COLOR_PAIR(4));
        } else {
            mvwaddch(saveWin, 3, 3, ' ' | COLOR_PAIR(2));
        }
        mvwprintw(saveWin, 3, 4, "%-42.42s", fileName + scroll);
        wmove(saveWin, 3, fmin(4 + index, 46));
    }

    // copy the filename to a static variable that keeps the recently used name
    if (lastFileName == NULL) {
        lastFileName = malloc(strlen(fileName) + 1);
    } else {
        lastFileName = realloc(lastFileName, strlen(fileName) + 1);
    }
    strcpy(lastFileName, fileName);

    // saving sheet data to file
    FILE *file = fopen(fileName, "wb");
    if (file == NULL) {
        delwin(saveWin);
        free(fileName);
        return FALSE;
    } else {
        fprintf(file, FILE_SIGNATURE); // magic number
        fprintf(file, "%c%c", curX, curY); // selected cell position
        for (int x = 0; x < SIZE; x++) {
            for (int y = 0; y < SIZE; y++) {
                Cell *cell = &(CELL(x, y));
                if (strlen(cell->formula) > 0 || cell->type != TYPE_AUTO) {
                    fprintf(file, "%c%c%c%c%zu%c%zu%c",
                            x, y, cell->type, cell->curType,
                            cell->textScroll, '\0',
                            strlen(cell->formula), '\0');
                    fprintf(file, "%s", cell->formula);
                }
            }
        }
        fclose(file);
    }

    delwin(saveWin);
    free(fileName);
    return TRUE;
}

void toggleLanguage(void) {
    language = (language + 1) % 2;
    for (int x = 0; x < SIZE; x++) {
        for (int y = 0; y < SIZE; y++) {
            Cell *cell = &(CELL(x, y));
            if (cell->type == TYPE_ERROR) {
                free(cell->text);
                ALLOC_SPRINTF(cell->text, "%s",
                              errors[language][cell->errorCode]);
                strncpy(cell->view, errors[language][cell->errorCode], 9);
                mvwprintw(cell->pad, 0, 0, "%-9s", cell->view);
                mvwaddch(cell->pad, 0, 8, TYPE_ERROR | COLOR_PAIR(4));
            }
        }
    }
}

void init(WINDOW **pad, WINDOW **cols, WINDOW **rows, 
          char *fileName,
          char *formula, unsigned *index) {
    initscr();
    keypad(stdscr, TRUE);
    noecho();
    raw();

    *pad = newpad(SIZE, SIZE * 9);
    *cols = newpad(1, SIZE * 9);
    *rows = newpad(SIZE, 8);

    start_color();
    init_pair(1, COLOR_WHITE, COLOR_RED);
    init_pair(2, COLOR_WHITE, COLOR_BLACK);
    init_pair(3, COLOR_WHITE, COLOR_BLUE);
    init_pair(4, COLOR_BLACK, COLOR_GREEN);

    // draw the reference bars (rows - numeric indices, columns - alphabetical)
    for (int i = 0; i < SIZE; i++) {
        wattron(*rows, COLOR_PAIR(i % 2 + 1));
        wattron(*cols, COLOR_PAIR(i % 2 + 1));
        mvwprintw(*rows, i, 0, "   %2d   ", i + 1);
        mvwprintw(*cols, 0, i * 9, "    %c    ", ALPHA_BASE + i);
        wattroff(*cols, COLOR_PAIR(i % 2 + 1));
        wattroff(*rows, COLOR_PAIR(i % 2 + 1));
    }
    refresh();

    // init cells and select the A1 cell
    for (int x = 0; x < SIZE; x++) {
        for (int y = 0; y < SIZE; y++) {
            initCell(&(CELL(x, y)), *pad, x, y);
        }
    }

    loadFile(fileName);

    selectCell(CELL(curX, curY), CELL(curX, curY),
               *pad, *cols, *rows, formula, index);

    refreshPads(*pad, *cols, *rows, 0, 0);
}

void processKeys(WINDOW *pad, WINDOW *cols, WINDOW *rows,
                 char *fileName, char *formula, unsigned *index) {
    int ch;
    while ((ch = getch()) != 17) { // 17 is ^Q
        int oldx = curX, oldy = curY;
        bool selecting = TRUE;
        switch (ch) {
            // navigation
            case KEY_UP:
                curY--;
                break;
            case KEY_RIGHT:
                curX++;
                break;
            case KEY_DOWN:
                curY++;
                break;
            case KEY_LEFT:
                curX--;
                break;
            // enter key (CR/LF, ASCII 13 and 10, respectively)
            case KEY_ENTER:
            case '\r':
            case '\n':
                updateCell(&(CELL(curX, curY)), formula, TRUE);
                break;
            // tab key - toggle the forced type for a cell
            case '\t':
                {
                    Cell *cur = &CELL(curX, curY);
                    if (cur->type != TYPE_ERROR) {
                        cur->curType = (cur->curType + 1) % 4;
                        cur->type = types[cur->curType];
                        updateCell(cur, cur->formula, TRUE);
                    }
                }
                break;
            // ^L - language switch
            case 12:
                toggleLanguage();
                break;
            // ^S - save file
            case 19:
            {
                bool success = saveFile(fileName, FALSE, FALSE);
                while (!success) {
                    success = saveFile(fileName, TRUE, FALSE);
                }
                break;
            }
            // home, end, page up and page down - scroll text field
            case KEY_HOME:
                CELL(curX, curY).textScroll = 0;
                break;
            case KEY_END:
                CELL(curX, curY).textScroll =
                    fmax(strlen(CELL(curX, curY).text) -
                         VISIBLE_TEXT_LENGTH, 0);
                break;
            case KEY_PPAGE:
            {
                Cell * cur = &(CELL(curX, curY));
                cur->textScroll =
                    WRAP(cur->textScroll - 1,
                         (int)fmax(strlen(cur->text) -
                                   VISIBLE_TEXT_LENGTH + 1, 0));
                break;
            }
            case KEY_NPAGE:
            {
                Cell * cur = &(CELL(curX, curY));
                cur->textScroll =
                    WRAP(cur->textScroll + 1,
                         (int)fmax(strlen(cur->text) -
                                   VISIBLE_TEXT_LENGTH + 1, 0));
                break;
            }
            // handle backspace as expected by the user
            case KEY_BACKSPACE:
            {
                selecting = FALSE;
                int x, y;
                getyx(stdscr, y, x);
                if (*index > 0) {
                    mvaddch(y, x - 1, ' ');
                    move(y, x - 1);
                    formula[--(*index)] = '\0';
                }
                break;
            }
            // other keys - user input
            default:
                if (ch >= PRINTABLE_ASCII_START &&
                    ch <= PRINTABLE_ASCII_END &&
                    *index < FORMULA_LENGTH) {
                        formula[(*index)++] = ch;
                        formula[*index] = '\0';
                        addch(ch);
                }
                selecting = FALSE;
                break;
        }
        if (selecting) {
            selectCell(CELL(oldx, oldy), CELL(curX, curY), pad, cols, rows,
                       formula, index);
        }
        refresh();
    }
}

void cleanUp(WINDOW *pad, WINDOW *cols, WINDOW *rows, char *formula) {
    // calling updateCell with an empty formula as the argument
    // causes removal of remaining dependencies from a cell
    for (int x = 0; x < SIZE; x++) {
        for (int y = 0; y < SIZE; y++) {
            updateCell(&(CELL(x, y)), "", TRUE);
        }
    }
    // ncurses stuff + free memory used by text in each cell
    for (int x = 0; x < SIZE; x++) {
        for (int y = 0; y < SIZE; y++) {
            delwin(CELL(x, y).pad);
            free(CELL(x, y).text);
        }
    }

    // ncurses stuff again
    delwin(rows);
    delwin(cols);
    delwin(pad);
    endwin();
}

int main(int argc, char *argv[]) {
    // the file name to read from / save to
    char *fileName = NULL;
    if (argc > 1) {
        fileName = argv[1];
    }

    // the global formula and text cursor position
    char formula[FORMULA_LENGTH] = { '\0' };
    unsigned index = 0;

    // ncurses stuff
    WINDOW *pad, *cols, *rows;

    init(&pad, &cols, &rows, fileName, formula, &index);

    processKeys(pad, cols, rows, fileName, formula, &index);

    bool success = saveFile(fileName, FALSE, TRUE);
    while (!success) {
        success = saveFile(fileName, TRUE, TRUE);
    }

    cleanUp(pad, cols, rows, formula);

    return 0;
}
