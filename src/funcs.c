#include "sheet.h"
#include "funcs.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

const char *funcNames[NUM_FUNCS] = {
    "INT",
    "FLOAT",
    "TEXT",
    "NEG",
    "SUM",
    "SUB",
    "MUL",
    "DIV",
    "MIN",
    "MAX",
    "CONCAT"
};

// these accept (from the user's point of view) only a single argument
const char *unaryFuncNames[NUM_FUNCS] = {
    "INT",
    "FLOAT",
    "TEXT",
    "NEG",
    "",
    "",
    "",
    "",
    "",
    "",
    ""
};

Value (*funcPtrs[NUM_FUNCS])(Value, Value) = {
    INT,
    FLOAT,
    TEXT,
    NEG,
    SUM,
    SUB,
    MUL,
    DIV,
    MIN,
    MAX,
    CONCAT
};


// this helps with error bubbling
bool errorCheck(Value *ret, Value arg1, Value arg2) {
    if (arg1.type == TYPE_ERROR) {
        *ret = arg1;
        if (arg2.type == TYPE_TEXT) {
            free(AS_TEXT(arg2));
        }
        return TRUE;
    }
    if (arg2.type == TYPE_ERROR) {
        *ret = arg2;
        if (arg1.type == TYPE_TEXT) {
            free(AS_TEXT(arg1));
        }
        return TRUE;
    }
    return FALSE;
}

// memory freeing helper
void freeStrings(Value arg1, Value arg2) {
    if (arg1.type == TYPE_TEXT) {
        free(AS_TEXT(arg1));
    }
    if (arg2.type == TYPE_TEXT) {
        free(AS_TEXT(arg2));
    }
}

// conversion to INT (unary)
Value INT(Value arg1, Value arg2) {
    if (arg1.type == TYPE_FLOAT) {
        arg1.type = TYPE_INT;
        AS_INT(arg1) = AS_FLOAT(arg1);
    } else if (arg1.type == TYPE_TEXT) {
        arg1.type = TYPE_INT;
        char *text = AS_TEXT(arg1);
        AS_INT(arg1) = strtoll(text, NULL, 10);
        free(text);
    }
    return arg1;
}

// helper
long long toInt(Value arg1) {
    Value arg2;
    return AS_INT(INT(arg1, arg2));
}

// convert to FLOAT (unary)
Value FLOAT(Value arg1, Value arg2) {
    if (arg1.type == TYPE_INT) {
        arg1.type = TYPE_FLOAT;
        AS_FLOAT(arg1) = AS_INT(arg1);
    } else if (arg1.type == TYPE_TEXT) {
        arg1.type = TYPE_FLOAT;
        char *text = AS_TEXT(arg1);
        AS_FLOAT(arg1) = strtod(text, NULL);
        free(text);
    }
    return arg1;
}

// helper
double toFloat(Value arg1) {
    Value arg2;
    return AS_FLOAT(FLOAT(arg1, arg2));
}

// convert to TEXT (unary)
Value TEXT(Value arg1, Value arg2) {
    if (arg1.type == TYPE_TEXT || arg1.type == TYPE_ERROR) {
        return arg1;
    }

    Value ret;
    ret.type = TYPE_TEXT;

    if (arg1.type == TYPE_INT) {
        ALLOC_SPRINTF(AS_TEXT(ret), "%lld", AS_INT(arg1));
    } else if (arg1.type == TYPE_FLOAT) {
        ALLOC_SPRINTF(AS_TEXT(ret), "%.3lf", AS_FLOAT(arg1));
    }

    return ret;
}

// negation (unary, accepts either INT or FLOAT)
Value NEG(Value arg1, Value arg2) {
    if (arg1.type == TYPE_INT) {
        AS_INT(arg1) = -AS_INT(arg1);
    } else if (arg1.type == TYPE_FLOAT) {
        AS_FLOAT(arg1) = -AS_FLOAT(arg1);
    } else if (arg1.type == TYPE_TEXT) {
        free(AS_TEXT(arg1));
        SET_ERROR(arg1, ERROR_BAD_ARG);
    }
    return arg1;
}

// function that helps determine whether
// at least one of the arguments received is of type FLOAT
bool useFloat(Value arg1, Value arg2) {
    return (arg1.type == TYPE_FLOAT && arg2.type == TYPE_FLOAT) ||
           (arg1.type == TYPE_FLOAT && arg2.type == TYPE_INT) ||
           (arg1.type == TYPE_INT && arg2.type == TYPE_FLOAT);
}

// sum (from the user's point of view, this accepts two or more arguments)
Value SUM(Value arg1, Value arg2) {
    Value ret;
    if (!errorCheck(&ret, arg1, arg2)) {
        if (useFloat(arg1, arg2)) {
            ret.type = TYPE_FLOAT;
            AS_FLOAT(ret) = toFloat(arg1) + toFloat(arg2);
        } else if (arg1.type == TYPE_INT && arg2.type == TYPE_INT) {
            ret.type = TYPE_INT;
            AS_INT(ret) = toInt(arg1) + toInt(arg2);
        } else {
            freeStrings(arg1, arg2);
            SET_ERROR(ret, ERROR_BAD_ARG);
        }
    }
    return ret;
}

// subtraction (with more than 2 args, the way they are ordered matters)
Value SUB(Value arg1, Value arg2) {
    Value ret;
    if (!errorCheck(&ret, arg1, arg2)) {
        if (useFloat(arg1, arg2)) {
            ret.type = TYPE_FLOAT;
            AS_FLOAT(ret) = toFloat(arg1) - toFloat(arg2);
        } else if (arg1.type == TYPE_INT && arg2.type == TYPE_INT) {
            ret.type = TYPE_INT;
            AS_INT(ret) = toInt(arg1) - toInt(arg2);
        } else {
            freeStrings(arg1, arg2);
            SET_ERROR(ret, ERROR_BAD_ARG);
        }
    }
    return ret;
}

// multiplication (2+ args)
Value MUL(Value arg1, Value arg2) {
    Value ret;
    if (!errorCheck(&ret, arg1, arg2)) {
        if (useFloat(arg1, arg2)) {
            ret.type = TYPE_FLOAT;
            AS_FLOAT(ret) = toFloat(arg1) * toFloat(arg2);
        } else if (arg1.type == TYPE_INT && arg2.type == TYPE_INT) {
            ret.type = TYPE_INT;
            AS_INT(ret) = toInt(arg1) * toInt(arg2);
        } else {
            freeStrings(arg1, arg2);
            SET_ERROR(ret, ERROR_BAD_ARG);
        }
    }
    return ret;
}

// division (2+ args, "throws" an error on attempts to divide by zero)
Value DIV(Value arg1, Value arg2) {
    Value ret;
    if (!errorCheck(&ret, arg1, arg2)) {
        if ((arg2.type == TYPE_INT && AS_INT(arg2) == 0) ||
            (arg2.type == TYPE_FLOAT && AS_FLOAT(arg2) == 0)) {
                SET_ERROR(ret, ERROR_DIV_0);
        } else if (useFloat(arg1, arg2)) {
            ret.type = TYPE_FLOAT;
            AS_FLOAT(ret) = toFloat(arg1) / toFloat(arg2);
        } else if (arg1.type == TYPE_INT && arg2.type == TYPE_INT) {
            ret.type = TYPE_INT;
            AS_INT(ret) = toInt(arg1) / toInt(arg2);
        } else {
            freeStrings(arg1, arg2);
            SET_ERROR(ret, ERROR_BAD_ARG);
        }
    }
    return ret;
}

// minimum value of 2+ supplied arguments
Value MIN(Value arg1, Value arg2) {
    Value ret;
    if (!errorCheck(&ret, arg1, arg2)) {
        if (useFloat(arg1, arg2)) {
            ret.type = TYPE_FLOAT;
            AS_FLOAT(ret) = fmin(toFloat(arg1), toFloat(arg2));
        } else if (arg1.type == TYPE_INT && arg2.type == TYPE_INT) {
            ret.type = TYPE_INT;
            AS_INT(ret) = fmin(toInt(arg1), toInt(arg2));
        } else {
            freeStrings(arg1, arg2);
            SET_ERROR(ret, ERROR_BAD_ARG);
        }
    }
    return ret;
}

// maximum value of 2+ supplied arguments
Value MAX(Value arg1, Value arg2) {
    Value ret;
    if (!errorCheck(&ret, arg1, arg2)) {
        if (useFloat(arg1, arg2)) {
            ret.type = TYPE_FLOAT;
            AS_FLOAT(ret) = fmax(toFloat(arg1), toFloat(arg2));
        } else if (arg1.type == TYPE_INT && arg2.type == TYPE_INT) {
            ret.type = TYPE_INT;
            AS_INT(ret) = fmax(toInt(arg1), toInt(arg2));
        } else {
            freeStrings(arg1, arg2);
            SET_ERROR(ret, ERROR_BAD_ARG);
        }
    }
    return ret;
}

// concatenation of 2+ TEXT values
Value CONCAT(Value arg1, Value arg2) {
    Value ret;
    if (!errorCheck(&ret, arg1, arg2)) {
        if (arg1.type == TYPE_TEXT && arg2.type == TYPE_TEXT) {
            ret.type = TYPE_TEXT;
            char *arg1text = AS_TEXT(arg1);
            char *arg2text = AS_TEXT(arg2);
            char *retText = AS_TEXT(ret) =
                    malloc(strlen(arg1text) + strlen(arg2text) + 1);
            strcpy(retText, arg1text);
            strcat(retText, arg2text);
            freeStrings(arg1, arg2);
        } else {
            SET_ERROR(ret, ERROR_BAD_ARG);
        }
    }
    return ret;
}
