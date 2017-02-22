sheet : main.o parser.o funcs.o
	gcc -o sheet main.o parser.o funcs.o -lncurses -lm
parser-test.o : parser-test.c sheet.h
	gcc -c parser-test.c -std=c99 -pedantic
main.o : main.c sheet.h
	gcc -c main.c -std=c99 -pedantic
parser.o : parser.c sheet.h funcs.h
	gcc -c parser.c -std=c99 -pedantic
funcs.o : funcs.c sheet.h funcs.h
	gcc -c funcs.c -std=c99 -pedantic
clean :
	rm sheet *.o
