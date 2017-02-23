# ceros-sheet
A terminal-based spreadsheet written in C99 and using ncurses (or compatible curses implementations).

## Why was this created?
This project was my submission for a university course that I attended. I thought it would be boring to simply solve the predefined problems the teacher gave us, and soon I came up with the idea to create a spreadsheet application that would work in a terminal. And so ceros-sheet was born.

## Is this a serious spreadsheet application?
Not really, more like a toy. See below for more thorough explanations.

## What can (can't) ceros-sheet do?
* There are three data types that you can use: `INT`, `FLOAT` and `TEXT`. You can convert between each (if a possible conversion is available) with the unary functions `INT()`, `FLOAT()` and `TEXT()`, respectively.
* There is a fixed number of cells: you get a 26*26 grid.
* Ranges are available: you can use e.g. `A1:C4` to collect values from all of the cells contained within the rectangle that spans from `A1` to `C4`.
* No arithmetic expressions &mdash; you have to use `SUM()`, `DIV()` etc. (they are variadic).
* If you want to import (or export) MS Excel or LibreOffice files, then you will be disappointed, ceros-sheet only supports its own file format.
* If you want precise numeric calculations, you will also be disappointed, as the application simply uses the available long double type, without any correction of rounding errors etc. Furthermore, floating point values are formatted to output with 3 decimal places.
* It runs on any system with an ncurses-compatible library (you'll have to replace the `#include <ncurses.h>` line in `sheet.h` though).
* There is support for two languages at the moment: English and Polish.

## How do I use ceros-sheet?
### Navigation
Move your selection with the arrow keys. The selection will wrap around the edges of the sheet.
### Inputting text
As soon as you've selected a cell, start typing and confirm your input by pressing Enter. If you don't press it and instead move to another cell after having typed something, you'll lose your changes.
### Inputting formulas
Inputting a formula is very similar to inputting text. The only difference is that formulas need to be preceded by the `=` character (so, for example, `SUM(3,5,8)` is normal text, but `=SUM(3,5,8)` is a formula that tells ceros-sheet to add the three integers, 3, 5 and 8).
### Force cell type
By default, cells have type referred to as `AUTO`. It means that when you use such cell as input in a formula, ceros-sheet will do its best to determine the most suitable data type. You can, however, toggle other types by pressing the Tab key repeatedly. A green box will appear next to the active cell with a character symbolizing the current type. No box means `AUTO`, `I` stands for `INT`, `F` &mdash; for `FLOAT` and `T` &mdash; for `TEXT`. For example, you might have `123` in cell `A1` and use `=CONCAT(A1,"456")` in `B3` &mdash; `B3` will then yield an error because of type incompatibility (`CONCAT()` is a function designed to concatenate &mdash; or join &mdash; two `TEXT` values, and `A1` has been automatically determined as `INT`). There are two approaches to solve this: you can either convert the value of `A1` to `TEXT` explicitly (`=CONCAT(TEXT(A1),"456")`), or toggle through available types on `A1` until you reach `T` (`TEXT`), which will have the same effect.
### Scrolling
Formulas are limited in terms of length (you can't go past what you see on the screen), but cell values (outputs of formulas) can be of any length. If a cell value doesn't fit on the screen, use Page Up to scroll to the left, Page Down to scroll to the right, Home to scroll to the beginning and End to scroll to the end. The current scroll position will be remembered in the session and in the files you save.
### Language
You can toggle between English and Polish by hitting `^L` (Ctrl+L) while working on a sheet.
### Save and/or quit
If you want to save your work, but don't wish to quit yet, hit `^S` (Ctrl+S) and follow the instructions on the screen. If you want to quit, hit `^Q` (Ctrl+Q) &mdash; you will then be prompted to save your work to a file, where you can either confirm that you want to save or discard your work by pressing `^Q` again.

## Using formulas
### Literals
You can use literal values in your formulas. These include integers (`123`), floating point numbers (`123.456789`) and text (`"some text"`). That means you can not only act on other cells' data, but also type numeric and textual constants right into your formula. Examples include: `=512`, which will simply yield `512`, `=123.45678`, which will result in `=123.457` (before being displayed, floats are always rounded to three decimal places), or `="some\\text\"here"` for `some\text"here`. As you probably noticed, there are special escape sequences in the `TEXT` type &mdash; normally, strings are delimited with the `"` character, and there has to be a way for you to use it in a meaningful way. So when you want to actually print `"`, prepend it with a backslash. If you want to print a backslash instead, also prepend it with another `\`.
### Functions
Functions are the heart of ceros-sheet. They actually perform actions on the supplied data. A function is simply an uppercase name followed by `(`, then some arguments separated by `,`, and finally `)`. For example: `=DIV(8,4)` (note that there is no space between the arguments, it wouldn't work with a space).
Functions can be used in a recursive manner, e.g. `=SUM(MUL(3,NEG(5)),19)` could be understood as `3 * (-5) + 19`.
As of now, there are 11 functions available for use in formulas. These include unary functions (those that accept only one argument), and variadic functions (accepting two or more arguments).
#### Unary functions
* `INT()` &mdash; convert any value to `INT`
* `FLOAT()` &mdash; convert any value to `FLOAT`
* `TEXT()` &mdash; convert any value to `TEXT`
* `NEG()` &mdash; negate a number (`INT` or `FLOAT`)
#### Variadic functions
* `SUM()` &mdash; sums numbers (`INT` or `FLOAT`)
* `SUB()` &mdash; subtracts numbers; order matters
* `MUL()` &mdash; multiply numbers
* `DIV()` &mdash; divide; division by zero results in an error
* `MIN()` &mdash; find minimum value in the numbers provided
* `MAX()` &mdash; find maximum value in the numbers provided
* `CONCAT()` &mdash; concatenates (or joins) two `TEXT` values
### Cell addresses
You can refer to a particular cell when supplying arguments. The address to use looks like `C5` or `A21` (never `5C` or `21A`).
For example, if you have `5` in cell `B2` and `9` in `C4`, using `=SUM(B2,C4)` anywhere would result in `14`.
### Cell ranges
For variadic functions, you can specify ranges of cells. Ranges are written as two cell addresses separated by `:`. For example, you might want to sum all numbers from the cell `A1` through `D4` (16 cells total). You would then write: `=SUM(A1:D4)`.