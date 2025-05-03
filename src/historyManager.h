#ifndef HISTORY_MANAGER_H
#define HISTORY_MANAGER_H

#include <ncurses.h>
#include "../include/common.h"

void setNavWin(WINDOW* nw);
void historyManagerAdd(SearchQuery* query, int resultCount);
SearchQuery* historyManagerShow(WINDOW* historyWin);

#endif