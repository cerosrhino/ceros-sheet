# ceros-sheet
A terminal-based spreadsheet written in C99 and using ncurses (or compatible curses implementations).

## Why was this created?
This project was my submission for a university course that I attended. I thought it would be boring to simply solve the predefined problems the teacher gave us, and soon I came up with the idea to create a spreadsheet application that would work in a terminal. And so ceros-sheet was born.

## Is this a serious spreadsheet application?
Not really, more like a toy. See below for more thorough explanations.

## What can (can't) ceros-sheet do?
* There are three data types that you can use: `INT`, `FLOAT` and `TEXT`. You can convert between each (if a possible conversion is available) with the unary functions `INT()`, `FLOAT()` and `TEXT()`, respectively.
* There is a fixed number of cells: you get a 26*26 grid.
* Ranges are available: you can use e.g. `A1:C4` to collect values from all of the cells contained within the rectangle that spans from A1 to C4.
* No arithmetic expressions &mdash; you have to use `SUM`, `DIV` etc. (they are variadic).
* If you want to import (or export) MS Excel or LibreOffice files, then you will be disappointed, ceros-sheet only supports its own file format.
* If you want precise numeric calculations, you will also be disappointed, as the application simply uses the available long double type, without any correction of rounding errors etc.
* It runs on any system with an ncurses-compatible library (you'll have to replace the `#include <ncurses.h>` line in `sheet.h` though).
* There is support for two languages at the moment: English and Polish.

## How do I use ceros-sheet?
(This will be expanded soon)