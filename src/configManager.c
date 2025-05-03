#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include "configManager.h"

// Загрузка настроек из файла
Config* configManagerLoad() {
    Config* config = malloc(sizeof(Config));
    // Значения по умолчанию
    config->fgColor = COLOR_WHITE;
    config->bgColor = COLOR_BLACK;
    config->maxHistory = 10;
    for (int i = 0; i < 10; i++) {
        config->keyMap[i] = strdup("");
    }

    // Чтение настроек из файла
    FILE* fp = fopen("~/.find_shell_config", "r");
    if (fp) {
        char line[256];
        while (fgets(line, sizeof(line), fp)) {
            line[strcspn(line, "\n")] = 0;
            if (strncmp(line, "fgColor=", 8) == 0) {
                config->fgColor = atoi(line + 8);
            } else if (strncmp(line, "bgColor=", 8) == 0) {
                config->bgColor = atoi(line + 8);
            } else if (strncmp(line, "maxHistory=", 11) == 0) {
                config->maxHistory = atoi(line + 11);
            }
        }
        fclose(fp);
    }
    return config;
}
