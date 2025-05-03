#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include "shellManager.h"
#include "inputHandler.h"
#include "findExecutor.h"
#include "historyManager.h"
#include "uiRenderer.h"
#include "configManager.h"
#include "../include/common.h"

WINDOW* navWin;
WINDOW* inputWin;
WINDOW* resultWin;
WINDOW* historyWin;
WINDOW* activePopup = NULL;
static Config* config;
static int scrollPos = 0;

// Функция для закрытия текущего всплывающего окна
void closeActivePopup() {
    if (activePopup) {
        delwin(activePopup);
        activePopup = NULL;
        touchwin(resultWin);
        wrefresh(resultWin);
    }
}

// Функция для обновления строки ввода
void updateInputWin(const char* input) {
    wclear(inputWin);
    box(inputWin, 0, 0);
    mvwprintw(inputWin, 0, 1, "%s", input ? input : "");
    touchwin(inputWin);
    wrefresh(inputWin);
    wmove(inputWin, 0, 1 + (input ? strlen(input) : 0)); // Устанавливаем курсор в конец строки
}

// Функция для перерисовки resultWin с инструкциями
void redrawInstructions() {
    wclear(resultWin);
    box(resultWin, 0, 0);
    mvwprintw(resultWin, 1, 1, "Добро пожаловать в ncurses-оболочку для find!");
    mvwprintw(resultWin, 2, 1, "F2 - фильтры: задайте путь, тип, права");
    mvwprintw(resultWin, 3, 1, "F3 - история: выберите запрос (Enter) или редактируйте (F4)");
    mvwprintw(resultWin, 4, 1, "F4 - редактировать последний запрос");
    mvwprintw(resultWin, 5, 1, "F5 - копировать путь в буфер (требуется xclip)");
    mvwprintw(resultWin, 6, 1, "Enter - выполнить поиск");
    mvwprintw(resultWin, 7, 1, "Backspace - удалить символ, Escape - очистить ввод");
    mvwprintw(resultWin, 8, 1, "Ctrl+D в истории - очистить историю");
    mvwprintw(resultWin, 9, 1, "Ctrl+C - выход");
    touchwin(resultWin);
    wrefresh(resultWin);
}

void shellInit() {
    initscr();
    start_color();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    config = configManagerLoad();
    init_pair(1, config->fgColor, config->bgColor);
    init_pair(2, COLOR_RED, config->bgColor);

    int maxY, maxX;
    getmaxyx(stdscr, maxY, maxX);
    inputWin = newwin(1, maxX, 0, 0);
    resultWin = newwin(maxY - 2, maxX, 1, 0);
    navWin = newwin(1, maxX, maxY - 1, 0);
    historyWin = newwin(10, maxX - 40, 2, 5);

    keypad(inputWin, TRUE);
    keypad(historyWin, TRUE);
    keypad(resultWin, TRUE); // Включаем keypad для resultWin

    uiRendererInit(inputWin, resultWin, navWin, historyWin);

    wclear(navWin);
    mvwprintw(navWin, 0, 0, "Поиск по подстроке в текущей директории (например, 'test'). Для пути: '/path test'. F2 - расширенный поиск");
    touchwin(navWin);
    wrefresh(navWin);

    updateInputWin("");
    redrawInstructions();
}

void shellRun() {
    int ch;
    char* input = malloc(256);
    if (!input) {
        endwin();
        fprintf(stderr, "Ошибка выделения памяти для input\n");
        exit(1);
    }
    input[0] = '\0';
    SearchQuery* query = NULL;
    SearchResult* result = NULL;
    bool inputMode = true;

    while (true) {
        // Обновляем navWin в начале каждого цикла
        wclear(navWin);
        if (inputMode) {
            mvwprintw(navWin, 0, 0, "Режим ввода: Enter - поиск | Esc - очистить | F2 - фильтры | F3 - история | F4 - редактировать");
        } else {
            mvwprintw(navWin, 0, 0, "Режим навигации: Стрелки - выбор | F5 - копировать | F4 - редактировать | Esc - вернуться к вводу");
        }
        touchwin(navWin);
        wrefresh(navWin);

        // Получаем ввод в зависимости от режима
        ch = inputMode ? wgetch(inputWin) : wgetch(resultWin);
        if (ch == 3) break; // Ctrl+C

        if (!inputMode) {
            // Навигационный режим: обрабатываем Esc, F4, F5, стрелки
            if (ch == 27) { // Escape
                inputMode = true;
                free(input);
                input = malloc(256);
                if (!input) {
                    endwin();
                    fprintf(stderr, "Ошибка выделения памяти для input\n");
                    exit(1);
                }
                input[0] = '\0';
                redrawInstructions();
                updateInputWin(input);
                wmove(inputWin, 0, 1);
                wrefresh(inputWin);
                continue;
            } else if (ch == 268 && result) { // F4 - Переход в режим ввода для редактирования
                inputMode = true;
                wmove(inputWin, 0, 1 + strlen(input));
                wrefresh(inputWin);
                continue;
            } else if (ch == 269 && result && result->count > scrollPos) { // F5 - Копировать путь
                char cmd[512];
                snprintf(cmd, 512, "echo '%s' | xclip -selection clipboard", result->paths[scrollPos]);
                system(cmd);
                wclear(navWin);
                mvwprintw(navWin, 0, 0, "Путь скопирован: %s", result->paths[scrollPos]);
                touchwin(navWin);
                wrefresh(navWin);
                touchwin(resultWin);
                wrefresh(resultWin);
                continue;
            } else if (ch == 259 && result && scrollPos > 0) { // Стрелка вверх
                scrollPos--;
                uiRendererDraw(resultWin, result, &scrollPos);
                touchwin(resultWin);
                wrefresh(resultWin);
                continue;
            } else if (ch == 258 && result && scrollPos < result->count - 1) { // Стрелка вниз
                scrollPos++;
                uiRendererDraw(resultWin, result, &scrollPos);
                touchwin(resultWin);
                wrefresh(resultWin);
                continue;
            }
            // Игнорируем остальные клавиши
            continue;
        }

        // Режим ввода: обрабатываем F1-F5, Enter, Backspace, Esc, и ввод символов
        if (ch >= 265 && ch <= 269) {
            if (ch == 265) { // F1
                wclear(navWin);
                mvwprintw(navWin, 0, 0, "F1 pressed (not implemented)");
                touchwin(navWin);
                wrefresh(navWin);
            } else if (ch == 266) { // F2
                closeActivePopup();
                query = inputHandlerShowFilters(resultWin, inputWin, &input);
                if (query) {
                    SearchResult* newResult = findExecutorRun(query);
                    if (newResult) {
                        // Устанавливаем новый результат
                        result = newResult;
                        scrollPos = 0;
                        if (result->count == 0 && result->error) {
                            wclear(resultWin);
                            box(resultWin, 0, 0);
                            if (strcmp(result->error, "No results found") == 0) {
                                mvwprintw(resultWin, 1, 1, "Нет результатов поиска");
                            } else {
                                mvwprintw(resultWin, 1, 1, "Ошибка: %s", result->error);
                            }
                            touchwin(resultWin);
                            wrefresh(resultWin);
                            inputMode = true;
                            wmove(inputWin, 0, 1 + strlen(input));
                            wrefresh(inputWin);
                        } else {
                            uiRendererDraw(resultWin, result, &scrollPos);
                            historyManagerAdd(query, result->count);
                            char* newInput = formatQueryString(query);
                            free(input);
                            input = newInput;
                            if (!input) {
                                endwin();
                                fprintf(stderr, "Ошибка выделения памяти для input\n");
                                exit(1);
                            }
                            updateInputWin(input);
                            inputMode = false;
                            // Ensure resultWin is active for key events
                            touchwin(resultWin);
                            wrefresh(resultWin);
                        }
                    } else {
                        wclear(navWin);
                        mvwprintw(navWin, 0, 0, "Ошибка: не удалось выполнить поиск");
                        touchwin(navWin);
                        wrefresh(navWin);
                    }
                } else {
                    redrawInstructions();
                }
                updateInputWin(input);
                closeActivePopup();
            } else if (ch == 267) { // F3
                closeActivePopup();
                SearchQuery* selected = historyManagerShow(historyWin);
                if (selected) {
                    query = selected;
                    SearchResult* newResult = findExecutorRun(query);
                    if (newResult) {
                        // Устанавливаем новый результат
                        result = newResult;
                        scrollPos = 0;
                        if (result->count == 0 && result->error) {
                            wclear(resultWin);
                            box(resultWin, 0, 0);
                            if (strcmp(result->error, "No results found") == 0) {
                                mvwprintw(resultWin, 1, 1, "Нет результатов поиска");
                            } else {
                                mvwprintw(resultWin, 1, 1, "Ошибка: %s", result->error);
                            }
                            touchwin(resultWin);
                            wrefresh(resultWin);
                            inputMode = true;
                            wmove(inputWin, 0, 1 + strlen(input));
                            wrefresh(inputWin);
                        } else {
                            uiRendererDraw(resultWin, result, &scrollPos);
                            historyManagerAdd(query, result->count);
                            char* newInput = formatQueryString(query);
                            free(input);
                            input = newInput;
                            if (!input) {
                                endwin();
                                fprintf(stderr, "Ошибка выделения памяти для input\n");
                                exit(1);
                            }
                            updateInputWin(input);
                            inputMode = false;
                            // Ensure resultWin is active for key events
                            touchwin(resultWin);
                            wrefresh(resultWin);
                        }
                    } else {
                        wclear(navWin);
                        mvwprintw(navWin, 0, 0, "Ошибка: не удалось выполнить поиск");
                        touchwin(navWin);
                        wrefresh(navWin);
                    }
                    // Освобождаем query, так как он был выделен в historyManagerShow
                    if (query) {
                        free(query->path);
                        free(query->pattern);
                        free(query->type);
                        free(query->perm);
                        free(query);
                        query = NULL;
                    }
                } else {
                    redrawInstructions();
                }
                updateInputWin(input);
                closeActivePopup();
            }
            continue;
        }

        if (ch == '\n') {
            if (!input || !input[0]) {
                wclear(navWin);
                mvwprintw(navWin, 0, 0, "Ошибка: введите имя файла или используйте F2");
                touchwin(navWin);
                wrefresh(navWin);
            } else {
                query = malloc(sizeof(SearchQuery));
                if (!query) {
                    endwin();
                    fprintf(stderr, "Ошибка выделения памяти для query\n");
                    exit(1);
                }
                char* q = strdup(input);
                if (!q) {
                    free(query);
                    endwin();
                    fprintf(stderr, "Ошибка выделения памяти для q\n");
                    exit(1);
                }
                char* path = strtok(q, " ");
                char* pattern = strtok(NULL, " ");
                query->path = path ? strdup(path) : strdup(".");
                if (!query->path) {
                    free(q);
                    free(query);
                    endwin();
                    fprintf(stderr, "Ошибка выделения памяти для query->path\n");
                    exit(1);
                }

                if (pattern && !strchr(pattern, '*') && !strchr(pattern, '?')) {
                    char* new_pattern = malloc(strlen(pattern) + 3);
                    if (!new_pattern) {
                        free(query->path);
                        free(q);
                        free(query);
                        endwin();
                        fprintf(stderr, "Ошибка выделения памяти для new_pattern\n");
                        exit(1);
                    }
                    snprintf(new_pattern, strlen(pattern) + 3, "*%s*", pattern);
                    query->pattern = new_pattern;
                } else {
                    query->pattern = pattern ? strdup(pattern) : strdup("*");
                    if (!query->pattern) {
                        free(query->path);
                        free(q);
                        free(query);
                        endwin();
                        fprintf(stderr, "Ошибка выделения памяти для query->pattern\n");
                        exit(1);
                    }
                }

                query->type = NULL;
                query->perm = NULL;
                query->caseSensitive = 1;
                char* token;
                while ((token = strtok(NULL, " "))) {
                    if (strcmp(token, "-t") == 0) {
                        char* t = strtok(NULL, " ");
                        if (t) {
                            query->type = strdup(t);
                            if (!query->type) {
                                free(query->path);
                                free(query->pattern);
                                free(q);
                                free(query);
                                endwin();
                                fprintf(stderr, "Ошибка выделения памяти для query->type\n");
                                exit(1);
                            }
                        }
                    }
                    if (strcmp(token, "-p") == 0) {
                        char* p = strtok(NULL, " ");
                        if (p) {
                            query->perm = strdup(p);
                            if (!query->perm) {
                                free(query->path);
                                free(query->pattern);
                                free(query->type);
                                free(q);
                                free(query);
                                endwin();
                                fprintf(stderr, "Ошибка выделения памяти для query->perm\n");
                                exit(1);
                            }
                        }
                    }
                    if (strcmp(token, "-i") == 0) query->caseSensitive = 0;
                }
                free(q);

                char* newInput = formatQueryString(query);
                free(input);
                input = newInput;
                if (!input) {
                    free(query->path);
                    free(query->pattern);
                    free(query->type);
                    free(query->perm);
                    free(query);
                    endwin();
                    fprintf(stderr, "Ошибка выделения памяти для input\n");
                    exit(1);
                }

                SearchResult* newResult = findExecutorRun(query);
                if (newResult) {
                    // Устанавливаем новый результат
                    result = newResult;
                    scrollPos = 0;
                    if (result->count == 0 && result->error) {
                        wclear(resultWin);
                        box(resultWin, 0, 0);
                        if (strcmp(result->error, "No results found") == 0) {
                            mvwprintw(resultWin, 1, 1, "Нет результатов поиска");
                        } else {
                            mvwprintw(resultWin, 1, 1, "Ошибка: %s", result->error);
                        }
                        touchwin(resultWin);
                        wrefresh(resultWin);
                        inputMode = true;
                        wmove(inputWin, 0, 1 + strlen(input));
                        wrefresh(inputWin);
                    } else {
                        uiRendererDraw(resultWin, result, &scrollPos);
                        historyManagerAdd(query, result->count);
                        updateInputWin(input);
                        inputMode = false;
                        // Ensure resultWin is active for key events
                        touchwin(resultWin);
                        wrefresh(resultWin);
                    }
                } else {
                    wclear(navWin);
                    mvwprintw(navWin, 0, 0, "Ошибка: не удалось выполнить поиск");
                    touchwin(navWin);
                    wrefresh(navWin);
                }
            }
        } else if (ch == KEY_BACKSPACE || ch == 127) {
            if (inputMode && strlen(input) > 0) {
                input[strlen(input) - 1] = '\0';
                updateInputWin(input);
                wmove(inputWin, 0, 1 + strlen(input));
                wrefresh(inputWin);
            } else if (inputMode) {
                wclear(navWin);
                mvwprintw(navWin, 0, 0, "Нечего удалять");
                touchwin(navWin);
                wrefresh(navWin);
                wmove(inputWin, 0, 1);
                wrefresh(inputWin);
            }
        } else if (ch == 27) { // Escape
            free(input);
            input = malloc(256);
            if (!input) {
                endwin();
                fprintf(stderr, "Ошибка выделения памяти для input\n");
                exit(1);
            }
            input[0] = '\0';
            updateInputWin(input);
            redrawInstructions();
            wmove(inputWin, 0, 1);
            wrefresh(inputWin);
        } else if (ch >= 32 && ch <= 126 && ch != 127 && inputMode) { // Обрабатываем ввод только в режиме ввода
            input = inputHandlerProcess(inputWin, ch, input);
            updateInputWin(input);
        }

        // Освобождаем память только после использования
        if (query) {
            free(query->path);
            free(query->pattern);
            free(query->type);
            free(query->perm);
            free(query);
            query = NULL;
        }
        if (result) {
            for (int i = 0; i < result->count; i++) free(result->paths[i]);
            free(result->paths);
            free(result->error);
            free(result);
            result = NULL;
        }
    }

    if (input) free(input);
}

void shellCleanup() {
    closeActivePopup();
    delwin(inputWin);
    delwin(resultWin);
    delwin(navWin);
    delwin(historyWin);
    for (int i = 0; i < 10; i++) free(config->keyMap[i]);
    free(config);
    endwin();
}