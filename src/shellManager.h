#ifndef SHELL_MANAGER_H
#define SHELL_MANAGER_H

#include <ncurses.h>

void shellInit();
void shellRun();
void shellCleanup();
extern WINDOW* navWin;
extern WINDOW* inputWin;
extern WINDOW* resultWin;
extern WINDOW* historyWin;
extern WINDOW* activePopup; // Добавляем extern для activePopup

#endif