#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include <ncurses.h>
#include "../include/common.h"

char* inputHandlerProcess(WINDOW* inputWin, int ch, char* currentInput);
SearchQuery* inputHandlerShowFilters(WINDOW* parentWin, WINDOW* inputWin, char** input);
char* formatQueryString(SearchQuery* query);

#endif