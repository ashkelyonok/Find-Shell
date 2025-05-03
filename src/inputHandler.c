#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "inputHandler.h"
#include "shellManager.h"

char* inputHandlerProcess(WINDOW* inputWin, int ch, char* currentInput) {
    if (!currentInput) {
        currentInput = malloc(256);
        currentInput[0] = '\0';
    }

    int len = strlen(currentInput);
    if (len < 255 && strchr("|&;<>", ch) == NULL) {
        currentInput[len] = ch;
        currentInput[len + 1] = '\0';
    } else if (len >= 255) {
        mvwprintw(navWin, 0, 0, "Ошибка: слишком длинный запрос");
        touchwin(navWin);
        wrefresh(navWin);
    } else {
        mvwprintw(navWin, 0, 0, "Ошибка: запрещенный символ");
        touchwin(navWin);
        wrefresh(navWin);
    }
    return currentInput;
}

char* formatQueryString(SearchQuery* query) {
    char* result = malloc(256);
    snprintf(result, 256, "%s%s%s%s%s",
             query->path ? query->path : ".",
             query->pattern ? " " : "",
             query->pattern ? query->pattern : "",
             query->type ? " -t " : "",
             query->type ? query->type : "");
    if (query->perm) {
        strcat(result, " -p ");
        strcat(result, query->perm);
    }
    if (!query->caseSensitive) strcat(result, " -i");
    return result;
}

SearchQuery* inputHandlerShowFilters(WINDOW* parentWin, WINDOW* inputWin, char** input) {
    SearchQuery* query = malloc(sizeof(SearchQuery));
    *query = (SearchQuery){strdup("."), strdup("*"), NULL, NULL, 1};
    int fileType = 0, dirType = 0, linkType = 0;
    int pos = 1;
    char* preview = NULL;

    int maxX;
    getmaxyx(stdscr, maxX, maxX);
    // Уменьшаем размер и корректируем положение filterWin
    WINDOW* filterWin = newwin(10, maxX - 40, 2, 5);
    if (!filterWin) {
        mvwprintw(navWin, 0, 0, "Ошибка: не удалось создать окно фильтров");
        touchwin(navWin);
        wrefresh(navWin);
        return NULL;
    }
    activePopup = filterWin;
    wattron(filterWin, COLOR_PAIR(1));
    box(filterWin, 0, 0);
    keypad(filterWin, TRUE);
    nodelay(filterWin, FALSE);

    int ch;
    bool editing = false;
    char tempInput[256] = "";
    int cursorX = 0;

    while (true) {
        free(preview);
        preview = formatQueryString(query);

        wclear(filterWin);
        box(filterWin, 0, 0);
        mvwprintw(filterWin, 0, 1, "Фильтры поиска:");
        mvwprintw(filterWin, 1, 1, "%s1. Путь: %s", pos == 1 ? "> " : "  ", query->path ? query->path : "текущая директория");
        mvwprintw(filterWin, 2, 1, "%s2. Имя файла: %s", pos == 2 ? "> " : "  ", query->pattern ? query->pattern : "любое");
        mvwprintw(filterWin, 3, 1, "%s3. Тип: [%c] Файл [%c] Директория [%c] Ссылка (3/4/5)", pos == 3 ? "> " : "  ",
                  fileType ? 'x' : ' ', dirType ? 'x' : ' ', linkType ? 'x' : ' ');
        mvwprintw(filterWin, 4, 1, "%s4. Права: %s", pos == 4 ? "> " : "  ", query->perm ? query->perm : "любые");
        mvwprintw(filterWin, 5, 1, "%s5. Регистр: %s (r)", pos == 5 ? "> " : "  ", query->caseSensitive ? "да" : "нет");
        mvwprintw(filterWin, 6, 1, "Текущий запрос: %s", preview);
        mvwprintw(filterWin, 7, 1, "Стрелки - выбор | Enter - редактировать | F5 - применить | Esc - выйти");

        if (editing) {
            if (pos == 1) {
                mvwprintw(filterWin, 1, 10, "%s", tempInput);
                wmove(filterWin, 1, 10 + cursorX);
                curs_set(1);
            } else if (pos == 2) {
                mvwprintw(filterWin, 2, 15, "%s", tempInput);
                wmove(filterWin, 2, 15 + cursorX);
                curs_set(1);
            } else if (pos == 4) {
                mvwprintw(filterWin, 4, 10, "%s", tempInput);
                wmove(filterWin, 4, 10 + cursorX);
                curs_set(1);
            }
        } else {
            curs_set(0);
        }

        touchwin(filterWin);
        wrefresh(filterWin);
        doupdate();

        free(*input);
        *input = strdup(preview);
        wclear(inputWin);
        box(inputWin, 0, 0);
        mvwprintw(inputWin, 0, 1, "%s", *input);
        touchwin(inputWin);
        wrefresh(inputWin);
        doupdate();

        ch = wgetch(filterWin);
        //debug
        mvwprintw(navWin, 0, 0, "Key code: %d", ch);
        wrefresh(navWin);

        if (ch == 27) { // Escape
            if (editing) {
                editing = false;
                tempInput[0] = '\0';
                curs_set(0);
                continue;
            }
            break;
        }

        if (editing) {
            if (ch == '\n') {
                if (pos == 1) {
                    free(query->path);
                    query->path = tempInput[0] ? strdup(tempInput) : strdup(".");
                } else if (pos == 2) {
                    free(query->pattern);
                    if (tempInput[0] == '\0') {
                        query->pattern = strdup("*");
                    } else if (!strchr(tempInput, '*') && !strchr(tempInput, '?')) {
                        char* new_pattern = malloc(strlen(tempInput) + 3);
                        snprintf(new_pattern, strlen(tempInput) + 3, "*%s*", tempInput);
                        query->pattern = new_pattern;
                    } else {
                        query->pattern = strdup(tempInput);
                    }
                } else if (pos == 4) {
                    if (strlen(tempInput) == 3 && strspn(tempInput, "01234567") == 3) {
                        free(query->perm);
                        query->perm = strdup(tempInput);
                    } else if (!tempInput[0]) {
                        free(query->perm);
                        query->perm = NULL;
                    }
                }
                editing = false;
                tempInput[0] = '\0';
                cursorX = 0;
                curs_set(0);
            } else if (ch == 259 || ch == 127) { // Стрелка вверх или Backspace
                if (cursorX > 0) {
                    tempInput[--cursorX] = '\0';
                }
            } else if (ch >= 32 && ch <= 126) {
                int len = strlen(tempInput);
                if (len < 255) {
                    tempInput[cursorX++] = ch;
                    tempInput[cursorX] = '\0';
                }
            }
            continue;
        }

        if (ch == 259 && pos > 1) { // Стрелка вверх (код 259)
            pos--;
        } else if (ch == 258 && pos < 5) { // Стрелка вниз (код 258)
            pos++;
        } else if (ch == '\n') {
            if (pos == 1 || pos == 2 || pos == 4) {
                editing = true;
                tempInput[0] = '\0';
                cursorX = 0;
                echo();
            }
        } else if (pos == 3 && (ch == '3' || ch == '4' || ch == '5')) {
            if (ch == '3') { fileType = !fileType; dirType = 0; linkType = 0; }
            else if (ch == '4') { fileType = 0; dirType = !dirType; linkType = 0; }
            else if (ch == '5') { fileType = 0; dirType = 0; linkType = !linkType; }
            free(query->type);
            query->type = fileType ? strdup("f") : dirType ? strdup("d") : linkType ? strdup("l") : NULL;
        } else if (pos == 5 && ch == 'r') {
            query->caseSensitive = !query->caseSensitive;
        } else if (ch == 269) { // F5 для применения фильтров
            if (editing) {
                if (pos == 1) {
                    free(query->path);
                    query->path = tempInput[0] ? strdup(tempInput) : strdup(".");
                } else if (pos == 2) {
                    free(query->pattern);
                    if (tempInput[0] == '\0') {
                        query->pattern = strdup("*");
                    } else if (!strchr(tempInput, '*') && !strchr(tempInput, '?')) {
                        char* new_pattern = malloc(strlen(tempInput) + 3);
                        snprintf(new_pattern, strlen(tempInput) + 3, "*%s*", tempInput);
                        query->pattern = new_pattern;
                    } else {
                        query->pattern = strdup(tempInput);
                    }
                } else if (pos == 4) {
                    if (strlen(tempInput) == 3 && strspn(tempInput, "01234567") == 3) {
                        free(query->perm);
                        query->perm = strdup(tempInput);
                    } else if (!tempInput[0]) {
                        free(query->perm);
                        query->perm = NULL;
                    }
                }
                editing = false;
                tempInput[0] = '\0';
                cursorX = 0;
                curs_set(0);
            }
            break;
        }
    }

    free(preview);
    if (ch == 27) {
        free(query->path);
        free(query->pattern);
        free(query->type);
        free(query->perm);
        free(query);
        activePopup = NULL;
        delwin(filterWin);
        touchwin(parentWin);
        wrefresh(parentWin);
        return NULL;
    }

    free(*input);
    *input = formatQueryString(query);
    activePopup = NULL;
    delwin(filterWin);
    touchwin(parentWin);
    wrefresh(parentWin);
    return query;
}