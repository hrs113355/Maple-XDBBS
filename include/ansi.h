#ifndef INCLUDE_ANSI_H
#define INCLUDE_ANSI_H

/* hrs.0801115: 借用openPtt的ansi.h來簡化code */

// Escapes.
#define ESC_NUM (0x1b)
#define ESC_STR "\x1b"
#define ESC_CHR '\x1b'

// Common ANSI commands.
#define ANSI_RESET  ESC_STR "[m"
#define ANSI_COLOR(x) ESC_STR "[" #x "m"
#define ANSI_MOVETO(y,x) ESC_STR "[" #y ";" #x "H"
#define ANSI_CLRTOEND ESC_STR "[K"
#define ANSI_REVERSE	ANSI_COLOR(7)

#define ANSI_SAVEPOS ESC_STR "[s"
#define ANSI_RESTOREPOS ESC_STR "[u"

#define ANSI_IN_ESCAPE(x) (((x) >= '0' && (x) <= '9') || \
	(x) == ';' || (x) == ',' || (x) == '[')

#endif	/* INCLUDE_ANSI_H */

