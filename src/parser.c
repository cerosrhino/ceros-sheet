#include "sheet.h"
#include "funcs.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <ctype.h>

// these help handle back-references needed to clean up outdated items
unsigned thisX, thisY;
RefNode *backRefs[SIZE][SIZE] = {{ NULL }};
bool manualUpdate = FALSE;

Value computeText(char **);
Value computeFunction(char **, int);
Value computeNumber(char **, double, char *);
Value computeCellAddress(char **, int);
Value computeRange(Value, int);
Value getCellValue(unsigned, unsigned);

bool isOutOfBounds(unsigned x, unsigned y) {
    return x >= SIZE || y >= SIZE;
}

void freeString(Value val) {
    if (val.type == TYPE_TEXT) {
        free(AS_TEXT(val));
    }
}

// add a back-reference
void addBackRef(Cell *src) {
    if (src == NULL) {
        return;
    }

    RefNode *cur = backRefs[thisY][thisX];
    while (cur != NULL && cur->next != NULL) {
        cur = cur->next;
    }
    RefNode *newRef = malloc(sizeof(RefNode));
    newRef->x = src->x;
    newRef->y = src->y;
    newRef->next = NULL;

    if (cur == NULL) {
        backRefs[thisY][thisX] = newRef;
    } else {
        cur->next = newRef;
    }
}

// clean up back-references, removing dependencies
// from source cells to destination cells
void clearBackRefs(void) {
    RefNode *cur = backRefs[thisY][thisX];
    if (cur != NULL && cur->next == NULL) {
        removeCellRef(cur->x, cur->y, thisX, thisY);
        free(cur);
        backRefs[thisY][thisX] = NULL;
        return;
    }
    while (cur != NULL && cur->next != NULL) {
        RefNode *next = cur->next;
        removeCellRef(cur->x, cur->y, thisX, thisY);
        free(cur);
        cur = next;
    }
    if (cur != NULL) {
        removeCellRef(cur->x, cur->y, thisX, thisY);
        free(cur);
    }
    backRefs[thisY][thisX] = NULL;
}

// this function handles the actual interpreting, the inCell flag
// denotes the context - when TRUE, we're dealing with the cell context,
// which is the deepest recursion-wise
Value compute(char **input, bool inCell) {
    Value value;

    int len = strlen(*input);

    if (!inCell) {
        if ((*input)[0] == ',') {
            (*input)++;
        }

        len = strcspn(*input, "(),");

        if (len == 0) {
            SET_ERROR(value, ERROR_EMPTY);
            return value;
        }

        if ((*input)[0] == '"') {
            return computeText(input);
        }

        if (len < strlen(*input) && len > 1 && (*input)[len] == '(') {
            return computeFunction(input, len);
        }

        if (len > 0 && isupper((*input)[0])) {
            return computeCellAddress(input, len);
        }
    }

    char *end;
    double testNum = strtod(*input, &end);
    if (end - *input == len && len > 0) {
        return computeNumber(input, testNum, end);
    }

    if (inCell) {
        value.type = TYPE_TEXT;
        AS_TEXT(value) = malloc(len + 1);
        strcpy(AS_TEXT(value), *input);
        return value;
    }

    SET_ERROR(value, ERROR_GENERAL);
    return value;
}

// parse TEXT literals
Value computeText(char **input) {
    Value value;
    int escapes = 0;
    int i;
    for (i = 1; ; i++) {
        if ((*input)[i] == '\0') {
            SET_ERROR(value, ERROR_BAD_ARG);
            return value;
        }
        if ((*input)[i] == '\\') {
            if ((*input)[i + 1] == '\\' || (*input)[i + 1] == '"') {
                escapes++;
                i++;
            } else {
                SET_ERROR(value, ERROR_BAD_ARG);
                return value;
            }
        } else if ((*input)[i] == '"') {
            break;
        }
    }
    int len = i - escapes - 1;

    value.type = TYPE_TEXT;
    char *text = AS_TEXT(value) = malloc(len - 1);
    int j, k;
    for (j = 1, k = 0; j < i; j++) {
        if (j < i - 1 && (*input)[j] == '\\') {
            j++;
        }
        text[k++] = (*input)[j];
    }
    text[len] = '\0';

    (*input) += i + 1;
    return value;
}

// parse functions in the formula with support for unary and variadic functions;
// varying number of arguments is simulated here,
// while the underlying C functions have 2 parameters each
Value computeFunction(char **input, int len) {
    Value value, arg1, arg2;
    for (int i = 0; i < NUM_FUNCS; i++) {
        if (strncmp(funcNames[i], *input, len) == 0 &&
            strlen(funcNames[i]) == len) {
            (*input) += len + 1;

            arg1 = compute(input, FALSE);
            if (strcmp(funcNames[i], unaryFuncNames[i]) == 0) {
                if (*input[0] != ')') {
                    freeString(arg1);
                    SET_ERROR(value, ERROR_TOO_MANY_ARGS);
                    return value;
                }
                (*input)++;
                return funcPtrs[i](arg1, arg2);
            }

            arg2 = compute(input, FALSE);
            char type1 = arg1.type;
            if (arg1.type == TYPE_ERROR) {
                freeString(arg2);
                return arg1;
            }
            if (arg1.type == TYPE_RANGE) {
                arg1 = value = computeRange(arg1, i);
            }
            if (arg2.type == TYPE_RANGE) {
                arg2 = computeRange(arg2, i);
                value = funcPtrs[i](arg1, arg2);
            }
            if ((type1 != TYPE_RANGE && arg2.type != TYPE_ERROR) ||
                GET_ERROR(arg2) != ERROR_EMPTY) {
                    value = funcPtrs[i](arg1, arg2);
            } else if (type1 != TYPE_RANGE && arg2.type == TYPE_ERROR &&
                       GET_ERROR(arg2) == ERROR_EMPTY) {
                freeString(arg1);
                freeString(arg2);
                freeString(value);
                SET_ERROR(value, ERROR_TOO_FEW_ARGS);
            }

            while ((*input)[0] == ',') {
                (*input)++;
                Value argN = compute(input, FALSE);
                if (argN.type == TYPE_RANGE) {
                    argN = computeRange(argN, i);
                }
                value = funcPtrs[i](value, argN);
            }
            if (*input[0] != ')') {
                freeString(value);
                SET_ERROR(value, ERROR_GENERAL);
            }
            (*input)++;

            return value;
        }
    }
    SET_ERROR(value, ERROR_NO_SUCH_FUNC);
    return value;
}

// compute number (INT or FLOAT)
Value computeNumber(char **input, double testNum, char *end) {
    Value value;
    value.type = TYPE_INT;
    char *tmp;
    AS_INT(value) = strtoll(*input, &tmp, 10);
    if (tmp[0] == '.') {
        value.type = TYPE_FLOAT;
        AS_FLOAT(value) = testNum;
    }
    *input = end;
    return value;
}

// compute the address of a single cell or a range of cells
Value computeCellAddress(char **input, int len) {
    Value value;
    if (len > 1 && isdigit((*input)[1])) {
        int x1 = (*input)[0] - ALPHA_BASE;
        int y1 = (*input)[1] - DIGIT_BASE;
        (*input) += 2;
        if (len > 2 && isdigit((*input)[0])) {
            y1 *= 10;
            y1 += (*input)[0] - DIGIT_BASE;
            (*input)++;
        }
        y1--;
        if (len > 4) {
            if ((*input)[0] == ':' && isupper((*input)[1])) {
                int x2 = (*input)[1] - ALPHA_BASE;
                if (isdigit((*input)[2])) {
                    int y2 = (*input)[2] - DIGIT_BASE;
                    (*input) += 3;
                    if (isdigit((*input)[0])) {
                        y2 *= 10;
                        y2 += (*input)[0] - DIGIT_BASE;
                        (*input)++;
                    }
                    y2--;
                    if (x1 == x2 && y1 == y2) {
                        return getCellValue(x1, y1);
                    }
                    if (isOutOfBounds(x1, y1) || isOutOfBounds(x2, y2)) {
                        SET_ERROR(value, ERROR_OUT_OF_BOUNDS);
                    } else {
                        value.type = TYPE_RANGE;
                        AS_RANGE(value).x1 = fmin(x1, x2);
                        AS_RANGE(value).y1 = fmin(y1, y2);
                        AS_RANGE(value).x2 = fmax(x1, x2);
                        AS_RANGE(value).y2 = fmax(y1, y2);
                    }
                    return value;
                }
            }
        }
        return getCellValue(x1, y1);
    }
    SET_ERROR(value, ERROR_GENERAL);
    return value;
}

// executing functions on cell ranges (such as A1:C5)
Value computeRange(Value range, int i) {
    Value value;
    int j = 0;
    for (int x = AS_RANGE(range).x1;
         x <= AS_RANGE(range).x2; x++) {
        for (int y = AS_RANGE(range).y1;
             y <= AS_RANGE(range).y2; y++, j++) {
            if (j == 0) {
                value = getCellValue(x, y);
            } else {
                value = funcPtrs[i](value, getCellValue(x, y));
            }
        }
    }
    return value;
}

Value getCellValue(unsigned x, unsigned y) {
    Value value;
    if (x == thisX && y == thisY) {
        SET_ERROR(value, ERROR_CYCLE);
        return value;
    }
    if (isOutOfBounds(x, y)) {
        SET_ERROR(value, ERROR_OUT_OF_BOUNDS);
        return value;
    }
    // set the dependencies only when the user manually enters data
    if (manualUpdate) {
        addBackRef(addCellRef(x, y, thisX, thisY));
    }
    char *cellText = getCellText(x, y);
    char type = getCellType(x, y);
    if (type == TYPE_AUTO) {
        char *cellTextBase = cellText;
        value = compute(&cellText, TRUE);
        free(cellTextBase);
    } else {
        value.type = type;

        if (type == TYPE_TEXT) {
            AS_TEXT(value) = cellText;
        } else {
            if (type == TYPE_INT) {
                AS_INT(value) = strtoll(cellText, NULL, 10);
            } else if (type == TYPE_FLOAT) {
                AS_FLOAT(value) = strtod(cellText, NULL);
            } else {
                SET_ERROR(value, getCellErrorCode(x, y));
            }
            free(cellText);
        }
    }

    return value;
}

// entry point for the parser module, takes a formula and cell coordinates
Value parse(const char *inputFormula, unsigned x, unsigned y, bool manual) {
    thisX = x;
    thisY = y;
    char *formula = malloc(strlen(inputFormula) + 1);
    char *formulaBase = formula;
    strcpy(formula, inputFormula);
    if (manual) {
        clearBackRefs();
    }
    manualUpdate = manual;
    Value value;
    if (formula[0] == '=') {
        formula++;
        value = compute(&formula, FALSE);
        if (formula[0] != '\0') {
            freeString(value);
            SET_ERROR(value, ERROR_GENERAL);
        }
    } else {
        value.type = TYPE_TEXT;
        AS_TEXT(value) = malloc(strlen(formula) + 1);
        strcpy(AS_TEXT(value), formula);
    }

    free(formulaBase);

    return value;
}
