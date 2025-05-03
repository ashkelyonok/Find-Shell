#include <ncurses.h>
#include <string.h>
#include "uiRenderer.h"
#include "shellManager.h"

// Инициализация интерфейса и отображение начального экрана
void uiRendererInit(WINDOW* iw, WINDOW* rw, WINDOW* nw, WINDOW* hw) {
    inputWin = iw;
    resultWin = rw;
    navWin = nw;
    historyWin = hw;

    // Установка цветов для окон
    wattron(inputWin, COLOR_PAIR(1));
    wattron(resultWin, COLOR_PAIR(1));
    wattron(navWin, COLOR_PAIR(1));
    box(inputWin, 0, 0);
    box(resultWin, 0, 0);
    box(navWin, 0, 0);

    // Отображение начального экрана с инструкциями
    mvwprintw(resultWin, 1, 1, "Добро пожаловать в ncurses-оболочку для find!");
    mvwprintw(resultWin, 2, 1, "F2 - фильтры: задайте путь, тип, права");
    mvwprintw(resultWin, 3, 1, "F3 - история: выберите запрос (Enter) или редактируйте (F4)");
    mvwprintw(resultWin, 4, 1, "F4 - редактировать последний запрос");
    mvwprintw(resultWin, 5, 1, "F5 - копировать путь в буфер (требуется xclip)");
    mvwprintw(resultWin, 6, 1, "Enter - выполнить поиск");
    mvwprintw(resultWin, 7, 1, "Backspace - удалить символ, Escape - очистить ввод");
    mvwprintw(resultWin, 8, 1, "Ctrl+D в истории - очистить историю");
    mvwprintw(resultWin, 9, 1, "Ctrl+C - выход");
    wrefresh(resultWin);
    wrefresh(inputWin);
    wrefresh(navWin);
}

// Отображение результатов поиска
void uiRendererDraw(WINDOW* resultWin, SearchResult* result, int* scrollPos) {
    wclear(resultWin);
    box(resultWin, 0, 0);
    if (result->error) {
        wattron(resultWin, COLOR_PAIR(2));
        mvwprintw(resultWin, 1, 1, "Ошибка: %s", result->error);
        wattroff(resultWin, COLOR_PAIR(2));
    } else if (result->count == 0) {
        mvwprintw(resultWin, 1, 1, "Ничего не найдено");
    } else {
        int maxY = getmaxy(resultWin) - 2;
        // Вычисляем начальный индекс так, чтобы выделенный элемент был виден
        int displayStart = *scrollPos - (*scrollPos % maxY);
        if (*scrollPos < displayStart) {
            displayStart = *scrollPos - (*scrollPos % maxY);
        }
        for (int i = displayStart; i < result->count && i < displayStart + maxY; i++) {
            if (i == *scrollPos) { // Подсвечиваем текущую строку и добавляем стрелку
                wattron(resultWin, A_REVERSE);
                mvwprintw(resultWin, i - displayStart + 1, 1, "-> %s", result->paths[i]);
                wattroff(resultWin, A_REVERSE);
            } else {
                mvwprintw(resultWin, i - displayStart + 1, 1, "   %s", result->paths[i]);
            }
        }
        int totalPages = (result->count + maxY - 1) / maxY;
        int currentPage = (*scrollPos / maxY) + 1;
        mvwprintw(resultWin, 0, 1, "Результаты (%d, страница %d/%d):", result->count, currentPage, totalPages);
    }
    wrefresh(resultWin);
}