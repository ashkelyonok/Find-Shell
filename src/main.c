#include "shellManager.h"
#include <locale.h>

// Точка входа в программу
int main() {
    setlocale(LC_ALL, "");
    shellInit();    // Инициализация интерфейса и настроек
    shellRun();     // Запуск основного цикла обработки ввода
    shellCleanup(); // Очистка ресурсов перед выходом
    return 0;
}