
/***************************************************************************
*                            COPYRIGHT NOTICE                              *
****************************************************************************
*                ncurses is copyright (C) 1992-1995                        *
*                          Zeyd M. Ben-Halim                               *
*                          zmbenhal@netcom.com                             *
*                          Eric S. Raymond                                 *
*                          esr@snark.thyrsus.com                           *
*                                                                          *
*        Permission is hereby granted to reproduce and distribute ncurses  *
*        by any means and for any fee, whether alone or as part of a       *
*        larger distribution, in source or in binary form, PROVIDED        *
*        this notice is included with any such distribution, and is not    *
*        removed from any of its header files. Mention of ncurses in any   *
*        applications linked with it is highly appreciated.                *
*                                                                          *
*        ncurses comes AS IS with no warranty, implied or expressed.       *
*                                                                          *
***************************************************************************/

/*
**	lib_addch.c
**
**	The routines waddch(), wattr_on(), wattr_off(), wchgat().
**
*/

#include <curses.priv.h>
#include <ctype.h>
#include "unctrl.h"


int wattr_on(WINDOW *win, const attr_t at)
{
	T(("wattr_on(%p,%s) current = %s", win, _traceattr(at), _traceattr(win->_attrs)));
	toggle_attr_on(win->_attrs,at);
	return OK;
}

int wattr_off(WINDOW *win, const attr_t at)
{
	T(("wattr_off(%p,%s) current = %s", win, _traceattr(at), _traceattr(win->_attrs)));
	toggle_attr_off(win->_attrs,at);
	return OK;
}

int wchgat(WINDOW *win, int n, attr_t attr, short color, const void *opts)
{
    int	i;

    for (i = win->_curx; i <= win->_maxx && (n == -1 || (n-- > 0)); i++)
	win->_line[win->_cury].text[i]
	    = TextOf(win->_line[win->_cury].text[i])
		| attr
		| COLOR_PAIR(color);

    return OK;
}

/*
 * Ugly microtweaking alert.  Everything from here to end of module is
 * likely to be speed-critical -- profiling data sure says it is!
 * Most of the important screen-painting functions are shells around
 * waddch().  So we make every effort to reduce function-call overhead
 * by inlining stuff, even at the cost of making wrapped copies for
 * export.  Also we supply some internal versions that don't call the
 * window sync hook, for use by string-put functions.
 */

static inline chtype render_char(WINDOW *win, chtype oldch, chtype newch)
/* compute a rendition of the given char correct for the current context */
{
	if (TextOf(oldch) == ' ')
		newch |= win->_bkgd;
	else if (!(newch & A_ATTRIBUTES))
		newch |= (win->_bkgd & A_ATTRIBUTES);
	TR(TRACE_VIRTPUT, ("bkg = %lx -> ch = %lx", win->_bkgd, newch));

	return(newch);
}

chtype _nc_render(WINDOW *win, chtype oldch, chtype newch)
/* make render_char() visible while still allowing us to inline it below */
{
    return(render_char(win, oldch, newch));
}

/* check if position is legal; if not, return error */
#define CHECK_POSITION(win, x, y) \
	if (y > win->_maxy || x > win->_maxx || y < 0 || x < 0) { \
		TR(TRACE_VIRTPUT, ("Alert! Win=%p _curx = %d, _cury = %d " \
				   "(_maxx = %d, _maxy = %d)", win, x, y, \
				   win->_maxx, win->_maxy)); \
	    	return(ERR); \
	}

static inline
int waddch_literal(WINDOW *win, chtype ch)
{
register int x, y;

	x = win->_curx;
	y = win->_cury;

	CHECK_POSITION(win, x, y);

	/*
	 * If we're trying to add a character at the lower-right corner more
	 * than once, fail.  (Moving the cursor will clear the flag).
	 */
#if 0	/* FIXME: ncurses 'p' test failed with this enabled */
	if ((win->_flags & _WRAPPED)
	 && (x != 0))
		return (ERR);
#endif

	/*
	 * We used to pass in
	 *	win->_line[y].text[x]
	 * as a second argument, but the value of the old character
	 * is not relevant here.
	 */
	ch = render_char(win, 0, ch);

	TR(TRACE_VIRTPUT, ("win attr = %s", _traceattr(win->_attrs)));
	ch |= win->_attrs;

	if (win->_line[y].text[x] != ch) {
		if (win->_line[y].firstchar == _NOCHANGE)
			win->_line[y].firstchar = win->_line[y].lastchar = x;
		else if (x < win->_line[y].firstchar)
			win->_line[y].firstchar = x;
		else if (x > win->_line[y].lastchar)
			win->_line[y].lastchar = x;

	}

	win->_line[y].text[x++] = ch;
	TR(TRACE_VIRTPUT, ("(%d, %d) = %s", y, x, _tracechtype(ch)));
	if (x > win->_maxx) {
		/*
		 * The _WRAPPED flag is useful only for telling an application
		 * that we've just wrapped the cursor.  We don't do anything
		 * with this flag except set it when wrapping, and clear it
		 * whenever we move the cursor.  If we try to wrap at the
		 * lower-right corner of a window, we cannot move the cursor
		 * (since that wouldn't be legal).  So we return an error
		 * (which is what svr4 does).  Unlike svr4, we can successfully
		 * add a character to the lower-right corner.
		 */
		win->_flags |= _WRAPPED;
		if (++y > win->_regbottom) {
			y = win->_regbottom;
			x = win->_maxx;
			if (win->_scroll)
				scroll(win);
			else {
				win->_curx = x;
				win->_cury = y;
				return (ERR);
			}
		}
		x = 0;
	}

	win->_curx = x;
	win->_cury = y;

	return OK;
}

static inline
int waddch_nosync(WINDOW *win, const chtype c)
/* the workhorse function -- add a character to the given window */
{
register chtype	ch = c;
register int	x, y;

	x = win->_curx;
	y = win->_cury;

	CHECK_POSITION(win, x, y);

	if (ch & A_ALTCHARSET)
		goto noctrl;

	switch ((int)TextOf(ch)) {
    	case '\t':
		x += (TABSIZE-(x%TABSIZE));

		/*
		 * Space-fill the tab on the bottom line so that we'll get the
		 * "correct" cursor position.
		 */
		if ((! win->_scroll && (y == win->_regbottom))
		 || (x <= win->_maxx)) {
			while (win->_curx < x) {
	    			if (waddch_literal(win, (' ' | AttrOf(ch))) == ERR)
					return(ERR);
			}
			break;
		} else {
			win->_flags |= _WRAPPED;
			if (++y > win->_regbottom) {
				x = win->_maxx;
				y--;
				if (win->_scroll) {
					scroll(win);
					x = 0;
				}
			} else {
				wclrtoeol(win);
				x = 0;
			}
		}
		break;
    	case '\n':
		x = 0;
		if (++y > win->_regbottom) {
			y--;
			if (win->_scroll)
				scroll(win);
			else
				return (ERR);
		} else {
			wclrtoeol(win);
			win->_flags &= ~_WRAPPED;
		}
		break;
    	case '\r':
		x = 0;
		win->_flags &= ~_WRAPPED;
		break;
    	case '\b':
		if (x > 0) {
			x--;
			win->_flags &= ~_WRAPPED;
		}
		break;
    	default:
		if (is7bits(TextOf(ch)) && iscntrl(TextOf(ch)))
		    	return(waddstr(win, unctrl((unsigned char)ch)));

		/* FALLTHRU */
        noctrl:
		waddch_literal(win, ch);
		return(OK);
	}

	win->_curx = x;
	win->_cury = y;

	return(OK);
}

int _nc_waddch_nosync(WINDOW *win, const chtype c)
/* export copy of waddch_nosync() so the string-put functions can use it */
{
    return(waddch_nosync(win, c));
}

/*
 * The versions below call _nc_synhook().  We wanted to avoid this in the
 * version exported for string puts; they'll call _nc_synchook once at end
 * of run.
 */

/* These are actual entry points */

int waddch(WINDOW *win, const chtype ch)
{
	TR(TRACE_VIRTPUT, ("waddch(%p, %s) called", win, _tracechtype(ch)));

	if (waddch_nosync(win, ch) == ERR)
		return(ERR);
	else
	{
		_nc_synchook(win);
		TR(TRACE_VIRTPUT, ("waddch() is done"));
		return(OK);
	}
}

int wechochar(WINDOW *win, const chtype ch)
{
	TR(TRACE_VIRTPUT, ("wechochar(%p, %s) called", win, _tracechtype(ch)));

	if (waddch_literal(win, ch) == ERR)
		return(ERR);
	else
	{
		_nc_synchook(win);
		TR(TRACE_VIRTPUT, ("wechochar() is done"));
		return(OK);
	}
}
