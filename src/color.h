/**\file    color.h
 * \ingroup Color
 */
#ifndef _COLOR_H
#define _COLOR_H

#include <string.h>
#include <stdarg.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The app using color.c must set to 1 prior to
 * calling the below \c C_xx() functions.
 * For CygWin, if this is > 1, it means to use ANSI-sequences to set colours.
 */
extern int C_use_colours;

/**
 * For CygWin, this variable means we must use ANSI-sequences to set colours.
 */
extern int C_use_ansi_colours;

/**
 * Unless this is set. Then CygWin also uses WinCon API to set colours.
 */
extern int C_no_ansi;

/**
 * Set this to 1 to use \c fwrite() in \c C_flush().
 */
extern int C_use_fwrite;

/*
 * Defined in newer <sal.h> for MSVC.
 */
#ifndef _Printf_format_string_
#define _Printf_format_string_
#endif

/**
 * Count of unneeded \c C_flush() calls. I.e. calls where length of buffer is 0.
 */
extern unsigned C_redundant_flush;

extern int C_printf (_Printf_format_string_ const char *fmt, ...)
  #if defined(__GNUC__)
    __attribute__ ((format(printf,1,2)))
  #endif
   ;

extern void (*C_write_hook) (const char *buf);

extern int    C_vprintf  (const char *fmt, va_list args);
extern int    C_puts     (const char *str);
extern int    C_putsn    (const char *str, size_t len);
extern int    C_putc     (int ch);
extern int    C_putc_raw (int ch);
extern int    C_setraw   (int raw);
extern int    C_setbin   (int bin);
extern size_t C_flush    (void);
extern void   C_reset    (void);
extern void   C_set_ansi (unsigned short col);
extern void   C_puts_long_line (const char *start, size_t indent);

#ifdef __cplusplus
}
#endif

#endif
