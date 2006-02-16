/************************************************************************** 
* This file is part of PDCurses. PDCurses is public domain software;
* you may use it for any purpose. This software is provided AS IS with
* NO WARRANTY whatsoever.
*
* If you use PDCurses in an application, an acknowledgement would be
* appreciated, but is not mandatory. If you make corrections or
* enhancements to PDCurses, please forward them to the current
* maintainer for the benefit of other users.
*
* No distribution of modified PDCurses code may be made under the name
* "PDCurses", except by the current maintainer. (Although PDCurses is
* public domain, the name is a trademark.)
*
* See the file maintain.er for details of the current maintainer.
**************************************************************************/

#define CURSES_LIBRARY 1
#include <curses.h>

/* undefine any macros for functions defined in this module */
#undef scroll
#undef scrl
#undef wscrl

#ifdef PDCDEBUG
const char *rcsid_scroll =
	"$Id: scroll.c,v 1.15 2006/02/16 22:59:49 wmcbrine Exp $";
#endif

/*man-start*********************************************************************

  Name:                                                        scroll

  Synopsis:
	int scroll(WINDOW *win);
	int scrl(int n);
	int wscrl(WINDOW *win, int n);

  X/Open Description:
	scroll() causes the window to scroll up one line.  This involves 
	moving the lines in the window data strcture.
 
	With the scrl() and wscrl() routines, for positive n scroll the 
	window up n lines (line i+n becomes i); otherwise scroll the 
	window down n lines.
 
	For these functions to work, scrolling must be enabled via 
	scrollok().
 
	Note that scrl() and scroll() may be macros.
 
	Note also that scrolling is not allowed if the supplied window 
	is a PAD.

  X/Open Return Value:
	All functions return OK on success and ERR on error.

  X/Open Errors:
	No errors are defined for this function.

  NOTE:
	The behaviour of Unix curses is to clear the line with a space
	and attributes of A_NORMAL. PDCurses clears the line with the
	window's current attributes (including current colour). To get
	the behaviour of PDCurses, #define PDCURSES_WCLR in curses.h or
	add -DPDCURSES_WCLR to the compile switches.

  Portability				     X/Open    BSD    SYS V
					     Dec '88
	scroll					Y	Y	Y
	scrl					-	-      4.0
	wscrl					-	-      4.0

**man-end**********************************************************************/

int scroll(WINDOW *win)
{
	PDC_LOG(("scroll() - called\n"));

	return wscrl(win, 1);
}

int wscrl(WINDOW *win, int n)
{
	int i, l;
	chtype blank, *ptr, *temp;

	/* Check if window scrolls. Valid for window AND pad */

	if ((win == (WINDOW *)NULL) || !win->_scroll)
		return ERR;

#if defined(PDCURSES_WCLR)
	blank = win->_blank | win->_attrs;
#else
	/* wrs (4/10/93) account for window background */

	blank = win->_bkgd;
#endif
	/* wrs -- 7/11/93 -- quick add to original scroll() routine to 
	   implement scrolling for a specified number of lines (not very 
	   efficient for more than 1 line) */

	if (n >= 0)
	{
		for (l = 0; l < n; l++) 
		{
			temp = win->_y[win->_tmarg];

			/* re-arrange line pointers */

			for (i = win->_tmarg; i < win->_bmarg; i++)
			{
				win->_y[i] = win->_y[i + 1];
				win->_firstch[i] = 0;
				win->_lastch[i] = win->_maxx - 1;
			}

			/* make a blank line */

			for (ptr = temp; (ptr - temp) < win->_maxx; ptr++)
				*ptr = blank;

			win->_y[win->_bmarg] = temp;
			win->_firstch[win->_bmarg] = 0;
			win->_lastch[win->_bmarg] = win->_maxx - 1;
		}
	}
	else 
	{
		for (l = n; l < 0; l++)
		{
			temp = win->_y[win->_bmarg];

			/* re-arrange line pointers */

			for (i = win->_bmarg; i > win->_tmarg; i--)
			{
				win->_y[i] = win->_y[i - 1];
				win->_firstch[i] = 0;
				win->_lastch[i] = win->_maxx - 1;
			}

			/* make a blank line */

			for (ptr = temp; (ptr - temp) < win->_maxx; ptr++)
				*ptr = blank;

			win->_y[win->_tmarg] = temp;
			win->_firstch[win->_tmarg] = 0;
			win->_lastch[win->_tmarg] = win->_maxx - 1;
		}
	}

	PDC_sync(win);
	return OK;
}

int scrl(int n)
{
	PDC_LOG(("scrl() - called\n"));

	return wscrl(stdscr, n);
}
