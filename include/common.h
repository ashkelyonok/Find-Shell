#ifndef COMMON_H
#define COMMON_H

#include <time.h>

// Структура для хранения параметров поискового запроса
typedef struct {
    char* path;          // Путь для поиска (например, "/etc")
    char* pattern;       // Шаблон имени файла (например, "*.txt")
    char* type;          // Тип файла (f - файл, d - директория, l - ссылка)
    char* perm;          // Права доступа (например, "644")
    int caseSensitive;   // Чувствительность к регистру (1 - да, 0 - нет)
} SearchQuery;

// Структура для хранения результатов поиска
typedef struct {
    char** paths;        // Массив путей к найденным файлам
    int count;           // Количество найденных файлов
    char* error;         // Сообщение об ошибке, если поиск не удался
} SearchResult;

// Структура для записи в истории
typedef struct {
    SearchQuery query;   // Параметры запроса
    time_t timestamp;    // Время выполнения запроса
    int resultCount;     // Количество результатов
} HistoryEntry;

// Структура для хранения настроек программы
typedef struct {
    int fgColor;         // Цвет текста
    int bgColor;         // Цвет фона
    int maxHistory;      // Максимальное количество записей в истории
    char* keyMap[10];    // Настраиваемые клавиши (не используется в текущей версии)
} Config;

#endif