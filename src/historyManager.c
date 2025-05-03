#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include "historyManager.h"
#include "inputHandler.h"
#include "configManager.h"
#include "shellManager.h"

void historyManagerAdd(SearchQuery* query, int resultCount) {
    char* historyPath = "./find_history";
    FILE* fp = fopen(historyPath, "a");
    if (!fp) {
        mvwprintw(navWin, 0, 0, "Ошибка записи истории: %s", strerror(errno));
        touchwin(navWin);
        wrefresh(navWin);
        return;
    }

    char* queryString = formatQueryString(query);
    time_t now = time(NULL);
    fprintf(fp, "%ld|%s|%d\n", now, queryString, resultCount);
    free(queryString);
    fclose(fp);

    Config* config = configManagerLoad();
    fp = fopen(historyPath, "r");
    if (fp) {
        int lines = 0;
        char line[512];
        while (fgets(line, sizeof(line), fp)) lines++;
        fclose(fp);
        if (lines > config->maxHistory) {
            FILE* temp = fopen("temp_history", "w");
            fp = fopen(historyPath, "r");
            int skip = lines - config->maxHistory;
            while (skip-- > 0) fgets(line, sizeof(line), fp);
            while (fgets(line, sizeof(line), fp)) fputs(line, temp);
            fclose(fp);
            fclose(temp);
            rename("temp_history", historyPath);
        }
    }
    for (int i = 0; i < 10; i++) free(config->keyMap[i]);
    free(config);
}

SearchQuery* historyManagerShow(WINDOW* historyWin) {
    activePopup = historyWin;
    keypad(historyWin, TRUE);

    char* historyPath = "./find_history";
    FILE* fp = fopen(historyPath, "r");
    if (!fp) {
        wclear(historyWin);
        box(historyWin, 0, 0);
        mvwprintw(historyWin, 1, 1, "Ошибка чтения истории: %s", strerror(errno));
        touchwin(historyWin);
        wrefresh(historyWin);
        wgetch(historyWin);
        activePopup = NULL;
        touchwin(resultWin);
        wrefresh(resultWin);
        return NULL;
    }

    typedef struct {
        time_t timestamp;
        char* queryString;
        int resultCount;
    } HistoryEntry;
    HistoryEntry* entries = malloc(100 * sizeof(HistoryEntry));
    int count = 0;
    char line[512];
    while (fgets(line, sizeof(line), fp) && count < 100) {
        line[strcspn(line, "\n")] = 0;
        char* queryStr = strchr(line, '|');
        if (!queryStr) continue;
        queryStr++;
        char* resultStr = strrchr(line, '|');
        if (!resultStr || resultStr == queryStr) continue;
        resultStr++;
        entries[count].timestamp = atol(line);
        entries[count].queryString = strdup(queryStr);
        entries[count].queryString[resultStr - queryStr - 1] = '\0';
        entries[count].resultCount = atoi(resultStr);
        count++;
    }
    fclose(fp);

    if (count == 0) {
        wclear(historyWin);
        box(historyWin, 0, 0);
        mvwprintw(historyWin, 1, 1, "История пуста");
        touchwin(historyWin);
        wrefresh(historyWin);
        wgetch(historyWin);
        activePopup = NULL;
        touchwin(resultWin);
        wrefresh(resultWin);
        for (int i = 0; i < count; i++) free(entries[i].queryString);
        free(entries);
        return NULL;
    }

    int pos = 0, ch;
    while (true) {
        wclear(historyWin);
        box(historyWin, 0, 0);
        mvwprintw(historyWin, 0, 1, "История (Enter - выбрать, F4 - редактировать, Ctrl+D - очистить, Esc - выйти):");
        int maxY = getmaxy(historyWin) - 2;
        for (int i = pos; i < count && i < pos + maxY; i++) {
            char timeStr[26];
            ctime_r(&entries[i].timestamp, timeStr);
            timeStr[strcspn(timeStr, "\n")] = 0;
            mvwprintw(historyWin, i - pos + 1, 1, "%s%d. %s [%s, %d рез.]",
                      i == pos ? "> " : "  ", i + 1, entries[i].queryString, timeStr, entries[i].resultCount);
        }
        touchwin(historyWin);
        wrefresh(historyWin);
        doupdate();

        ch = wgetch(historyWin);

        if (ch == 27) { // Escape
            for (int i = 0; i < count; i++) free(entries[i].queryString);
            free(entries);
            activePopup = NULL;
            touchwin(resultWin);
            wrefresh(resultWin);
            return NULL;
        }
        if (ch == 259 && pos > 0) { // Стрелка вверх (код 259)
            pos--;
            continue;
        }
        if (ch == 258 && pos < count - 1) { // Стрелка вниз (код 258)
            pos++;
            continue;
        }
        if (ch == '\n') {
            SearchQuery* selected = malloc(sizeof(SearchQuery));
            char* q = strdup(entries[pos].queryString);
            char* path = strtok(q, " ");
            if (!path) {
                free(q);
                free(selected);
                for (int i = 0; i < count; i++) free(entries[i].queryString);
                free(entries);
                activePopup = NULL;
                touchwin(resultWin);
                wrefresh(resultWin);
                return NULL;
            }
            char* pattern = strtok(NULL, " ");
            selected->path = strdup(path);
            selected->pattern = pattern ? strdup(pattern) : strdup("*");
            selected->type = NULL;
            selected->perm = NULL;
            selected->caseSensitive = 1;
            char* token;
            while ((token = strtok(NULL, " "))) {
                if (strcmp(token, "-t") == 0) {
                    char* t = strtok(NULL, " ");
                    if (t) selected->type = strdup(t);
                }
                if (strcmp(token, "-p") == 0) {
                    char* p = strtok(NULL, " ");
                    if (p) selected->perm = strdup(p);
                }
                if (strcmp(token, "-i") == 0) selected->caseSensitive = 0;
            }
            free(q);
            for (int i = 0; i < count; i++) free(entries[i].queryString);
            free(entries);
            activePopup = NULL;
            touchwin(resultWin);
            wrefresh(resultWin);
            return selected;
        }
        if (ch == 268) { // F4 (код 268)
            char* tempInput = strdup(entries[pos].queryString);
            SearchQuery* edited = inputHandlerShowFilters(historyWin, inputWin, &tempInput);
            if (edited) {
                SearchQuery* selected = malloc(sizeof(SearchQuery));
                *selected = *edited;
                free(edited);
                free(tempInput);
                for (int i = 0; i < count; i++) free(entries[i].queryString);
                free(entries);
                activePopup = NULL;
                touchwin(resultWin);
                wrefresh(resultWin);
                return selected;
            }
            free(tempInput);
            for (int i = 0; i < count; i++) free(entries[i].queryString);
            free(entries);
            activePopup = NULL;
            touchwin(resultWin);
            wrefresh(resultWin);
            return NULL;
        }
        if (ch == 4) { // Ctrl+D
            wclear(historyWin);
            box(historyWin, 0, 0);
            mvwprintw(historyWin, 1, 1, "Очистить историю? (y/n)");
            touchwin(historyWin);
            wrefresh(historyWin);
            ch = wgetch(historyWin);
            if (ch == 'y') {
                fp = fopen(historyPath, "w");
                if (fp) fclose(fp);
                else mvwprintw(navWin, 0, 0, "Ошибка очистки истории");
                for (int i = 0; i < count; i++) free(entries[i].queryString);
                free(entries);
                activePopup = NULL;
                touchwin(resultWin);
                wrefresh(resultWin);
                return NULL;
            }
        }
    }

    for (int i = 0; i < count; i++) free(entries[i].queryString);
    free(entries);
    activePopup = NULL;
    touchwin(resultWin);
    wrefresh(resultWin);
    return NULL;
}