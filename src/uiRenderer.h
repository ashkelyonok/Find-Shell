#ifndef UI_RENDERER_H
#define UI_RENDERER_H

#include <ncurses.h>
#include "../include/common.h"

void uiRendererInit(WINDOW* inputWin, WINDOW* resultWin, WINDOW* navWin, WINDOW* historyWin);
void uiRendererDraw(WINDOW* resultWin, SearchResult* result, int* scrollPos);

#endif