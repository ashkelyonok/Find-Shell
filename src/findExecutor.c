#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "findExecutor.h"

SearchResult* findExecutorRun(SearchQuery* query) {
    SearchResult* result = malloc(sizeof(SearchResult));
    result->paths = malloc(10 * sizeof(char*));
    result->count = 0;
    result->error = NULL;

    // Проверка валидности пути
    if (!query->path || access(query->path, F_OK) != 0) {
        result->error = strdup("Путь не существует");
        return result;
    }

    // Проверка прав доступа к пути
    if (access(query->path, R_OK) != 0) {
        result->error = strdup("Нет прав доступа к пути");
        return result;
    }

    // Проверка pattern
    if (!query->pattern || strlen(query->pattern) == 0) {
        result->error = strdup("Шаблон имени файла не указан");
        return result;
    }

    // Формирование команды find
    char* cmd = malloc(1024);
    snprintf(cmd, 1024, "find %s%s%s%s%s%s%s%s", query->path,
             query->pattern ? (query->caseSensitive ? " -name '" : " -iname '") : "",
             query->pattern ? query->pattern : "", query->pattern ? "'" : "",
             query->type ? " -type " : "", query->type ? query->type : "",
             query->perm ? " -perm " : "", query->perm ? query->perm : "");

    FILE* fp = popen(cmd, "r");
    free(cmd);
    if (!fp) {
        result->error = strdup("Ошибка выполнения find");
        return result;
    }

    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        if (result->count % 10 == 0 && result->count != 0) {
            result->paths = realloc(result->paths, (result->count + 10) * sizeof(char*));
        }
        line[strcspn(line, "\n")] = 0;
        result->paths[result->count++] = strdup(line);
    }

    int status = pclose(fp);
    if (status != 0 && result->count == 0) {
        // Если find завершился с ошибкой и ничего не найдено, проверяем причину
        if (access(query->path, R_OK) != 0) {
            result->error = strdup("Нет прав доступа к пути");
        } else {
            // Если путь существует и доступен, но ничего не найдено, это не ошибка
            result->error = strdup("No results found");
        }
    } else if (result->count == 0) {
        // Если find завершился успешно, но ничего не найдено
        result->error = strdup("No results found");
    }

    return result;
}