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

#define	CURSES_LIBRARY 1
#include <curses.h>

/* undefine any macros for functions defined in this module */
#undef getch
#undef wgetch
#undef mvgetch
#undef mvwgetch
#undef ungetch

/* undefine any macros for functions called by this module if in debug mode */
#ifdef PDCDEBUG
# undef wrefresh
# undef nocbreak
# undef move
# undef wmove
#endif

#ifdef PDCDEBUG
const char *rcsid_getch =
	"$Id: getch.c,v 1.24 2006/02/16 22:59:49 wmcbrine Exp $";
#endif

/*man-start*********************************************************************

  Name:                                                         getch

  Synopsis:
	int getch(void);
	int wgetch(WINDOW *win);
	int mvgetch(int y, int x);
	int mvwgetch(WINDOW *win, int y, int x);
	int ungetch(int ch);

  X/Open Description:
	With the getch(), wgetch(), mvgetch(), and mvwgetch() functions, 
	a character is read from the terminal associated with the window. 
	In nodelay mode, if there is no input waiting, the value ERR is 
	returned. In delay mode, the program will hang until the system 
	passes text through to the program. Depending on the setting of 
	cbreak(), this will be after one character or after the first 
	newline.  Unless noecho() has been set, the character will also 
	be echoed into the designated window.

	If keypad() is TRUE, and a function key is pressed, the token for
	that function key will be returned instead of the raw characters.
	Possible function keys are defined in <curses.h> with integers
	beginning with 0401, whose names begin with KEY_.  If a character
	is received that could be the beginning of a function key (such as
	escape), curses will set a timer.  If the remainder of the sequence
	does not come in within the designated time, the character will be
	passed through, otherwise the function key value will be returned.
	For this reason, on many terminals, there will be a delay after a
	user presses the escape key before the escape is returned to the
	program.  (Use by a programmer of the escape key for a single
	character function is discouraged.)

	If nodelay(win,TRUE) has been called on the window and no input
	is waiting, the value ERR is returned.

	The ungetch() function places ch back onto the input queue to be
	returned by the next call to wgetch().

	NOTE: getch(), mvgetch() and mvwgetch() are implemented as macros.

  PDCurses Description:
	Given the nature of the PC, there is no such timer set for an
	incoming ESCAPE value, because function keys generate unique
	scan codes that are not prefixed with the ESCAPE character.

	Also, note that the getch() definition will conflict  with
	many DOS compiler's runtime libraries.

  X/Open Return Value:
	This functions return ERR or the value of the character, meta 
	character or function key token.

  X/Open Errors:
	No errors are defined for this function.

  Portability				     X/Open    BSD    SYS V
					     Dec '88
	getch					Y	Y	Y
	wgetch					Y	Y	Y
	mvgetch					Y	Y	Y
	mvwgetch				Y	Y	Y
	ungetch					Y	Y	Y

**man-end**********************************************************************/

/* these defines to get around DOS library conflicts */

#define getch PDC_getch
#define ungetch PDC_ungetch

int PDC_getch(void)
{
	PDC_LOG(("getch() - called\n"));

	return wgetch(stdscr);
}

int wgetch(WINDOW *win)
{
	extern int c_pindex;		/* putter index	*/
	extern int c_gindex;		/* getter index	*/
	extern int c_ungind;		/* ungetch() push index */
	extern int c_ungch[NUNGETCH];	/* array of ungotten chars */
	extern WINDOW *_getch_win_;

	static int buffer[_INBUFSIZ];	/* character buffer */
	int key, display_key, waitcount;

	PDC_LOG(("wgetch() - called\n"));

	if (win == (WINDOW *)NULL)
		return ERR;

	display_key = 0x100;
	waitcount = 0;

	 /* set the number of 1/20th second napms() calls */

	if (SP->delaytenths)
		waitcount = 2 * SP->delaytenths;
	else
		if (win->_delayms)
		{
			/* Can't really do millisecond intervals, so 
			   delay in 1/20ths of a second (50ms) */

			waitcount = win->_delayms / 50;
			if (waitcount == 0)
				waitcount = 1;
		}

	PDC_LOG(("initial: %d delaytenths %d delayms %d\n",
		waitcount, SP->delaytenths, win->_delayms));

	/* wrs (7/31/93) -- System V curses refreshes window when wgetch 
	   is called if there have been changes to it and it is not a pad */

	if (!(win->_flags & _PAD) && is_wintouched(win))
		wrefresh(win);

	_getch_win_ = win;

	/* if ungotten char exists, remove and return it */

	if (c_ungind)
		return c_ungch[--c_ungind];

	/* if normal and data in buffer */

	if ((!SP->raw_inp && !SP->cbreak) && (c_gindex < c_pindex))
		return buffer[c_gindex++];

	/* prepare to buffer data */

	c_pindex = 0;
	c_gindex = 0;

	/* to get here, no keys are buffered. go and get one. */

	for (;;)			/* loop for any buffering */
	{

#ifdef XCURSES
		key = PDC_rawgetch();
		if (!(_getch_win_->_use_keypad) && (unsigned int)key > 255)
			key = -1;
#else
		if (SP->raw_inp)
			key = PDC_rawgetch();	/* get a raw character */
		else
		{
			/* get a system character if break return proper */

			bool cbr = PDC_get_ctrl_break();
			PDC_set_ctrl_break(SP->orgcbr);
			key = PDC_sysgetch();
			PDC_set_ctrl_break(cbr);
		}
#endif
		/* Handle timeout() and halfdelay() */

		if (SP->delaytenths || win->_delayms)
		{
			PDC_LOG(("waitcount: %d delaytenths %d delayms %d\n",
			    waitcount, SP->delaytenths, win->_delayms));

			if (waitcount == 0 && key == -1)
				return ERR;

			if (key == -1)
			{
				waitcount--;
				napms(50);	/* sleep for 1/20th second */
				continue;
			}
		}
		else
			if ((win->_nodelay) && (key == -1))
				return ERR;

		/* translate CR */

		if ((key == '\r') && (SP->autocr) && (!SP->raw_inp))
			key = '\n';

		/* if echo is enabled */

		if (SP->echo && (key < display_key))
		{
			waddch(win, key);
			wrefresh(win);
		}

		/* if no buffering */

		if ((SP->raw_inp || SP->cbreak))
			return key;

		/* if no overflow, put data in buffer */

		if (c_pindex < _INBUFSIZ - 2)
			buffer[c_pindex++] = key;

		/* if we got a line */

		if ((key == '\n') || (key == '\r'))
			return buffer[c_gindex++];
	}
}

int mvgetch(int y, int x)
{
	PDC_LOG(("mvgetch() - called\n"));

	if (move(y, x) == ERR)
		return ERR;

	return wgetch(stdscr);
}

int mvwgetch(WINDOW *win, int y, int x)
{
	PDC_LOG(("mvwgetch() - called\n"));

	if (wmove(win, y, x) == ERR)
		return ERR;

	return wgetch(win);
}

int PDC_ungetch(int ch)
{
	extern int c_ungind;		/* ungetch() push index */
	extern int c_ungch[NUNGETCH];	/* array of ungotten chars */

	PDC_LOG(("ungetch() - called\n"));

	if (c_ungind >= NUNGETCH)	/* pushback stack full */
		return ERR;

	c_ungch[c_ungind++] = ch;

	return OK;
}
