#define NUM_FUNCS 11

Value INT(Value, Value);
Value FLOAT(Value, Value);
Value TEXT(Value, Value);
Value NEG(Value, Value);
Value SUM(Value, Value);
Value SUB(Value, Value);
Value MUL(Value, Value);
Value DIV(Value, Value);
Value MIN(Value, Value);
Value MAX(Value, Value);
Value CONCAT(Value, Value);

extern const char *funcNames[NUM_FUNCS];
extern const char *unaryFuncNames[NUM_FUNCS];
extern Value (*funcPtrs[NUM_FUNCS])(Value, Value);
