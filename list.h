#ifndef _LIST_H
#define _LIST_H

#include "ui.h"

void		init_list();
void		close_list();
void            refresh_list();
void		print_list_line(int i, int line);
void		list_headerline();
void            scroll_up();
void            scroll_down();
void		page_up();
void		page_down();
void            select_none();
void            select_all();
void            move_curitem(int direction);
void		goto_home();
void		goto_end();
void		highlight_line(WINDOW *win, int line);
int		selected_items();
void		invert_selection();
int		list_current_item();
int		list_is_empty();


enum {
	MOVE_ITEM_UP,
	MOVE_ITEM_DOWN
};

#define LIST_TOP        3
#define LIST_BOTTOM     (LINES-3)

#define LIST_LINES	(LIST_BOTTOM-LIST_TOP)
#define LIST_COLS	COLS

#define NAMEPOS		2
#define EMAILPOS        options_get_int("emailpos")
#define EXTRAPOS	options_get_int("extrapos")

#define NAMELEN		(EMAILPOS-NAMEPOS -1)
#define EMAILLEN        (EXTRAPOS-EMAILPOS -1)
#define EXTRALEN	(COLS-EXTRAPOS)

#define LAST_LIST_ITEM	( first_list_item + LIST_LINES - 1 )

#endif