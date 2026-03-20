#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <limits.h>
#include <quadmath.h>
#include <math.h>

// Генерация случайного вещественного числа с p знаками после запятой
__float128 randReal(double a, double b, size_t p)
{
    // генерим в double
    double u = (double)rand() / ((double)RAND_MAX + 1.0);
    __float128 x = (__float128)(a + (b - a) * u);

    // scale = 10^p 
    __float128 scale = 1.0Q;
    for (size_t i = 0; i < p; ++i) scale *= 10.0Q;

    // округление до p знаков после запятой (к ближайшему)
    if (x >= 0)
        x = floorq(x * scale + 0.5Q) / scale;
    else
        x = -floorq((-x) * scale + 0.5Q) / scale;

    return x;
}

void toMachine(
        const void* number, // Число, которое нужно представить в машинном виде
        size_t number_type, // Сколько байт выделяется для типа данных переменной number
        size_t bit_qt,      // Сколько бит нужно для представления
        char out[]          // Массив, в котором по символам хранится машинное представление 
    )
{
    // Прочитать исходное значение в максимально широкую переменную
    __float128 x;

    if (number_type == sizeof(float))
    {
        float t;
        memcpy(&t, number, sizeof t);
        x = (__float128)t;
    } 
    else if (number_type == sizeof(double)) 
    {
        double t;
        memcpy(&t, number, sizeof t);
        x = (__float128)t;
    } 
    else if (number_type == sizeof(__float128)) 
        memcpy(&x, number, sizeof x);
    else return;

    // Привести к целевому формату и считать биты
    unsigned long long lo = 0, hi = 0;

    if (bit_qt == 32)
    {
        float y = (float)x; // округление до float
        unsigned int u32 = 0;
        memcpy(&u32, &y, sizeof u32); 
        lo = (unsigned long long)u32; // 32 бита в младшей части
    }
    else if (bit_qt == 64) 
    {
        double y = (double)x;       // округление до double
        memcpy(&lo, &y, sizeof y);  // 64 бита
    }
    else if (bit_qt == 128) 
    {
        __float128 y = x; // уже __float128
        memcpy(&lo, (const unsigned char*)&y + 0, 8);
        memcpy(&hi, (const unsigned char*)&y + 8, 8);
    }
    else return;

    // Вывести биты 
    for (size_t i = 0; i < bit_qt; ++i) {
        unsigned sh = (unsigned)(bit_qt - 1 - i);
        unsigned bit;

        if (bit_qt == 32 || bit_qt == 64) 
            bit = (unsigned)((lo >> sh) & 1ull);
        else 
        {
            if (sh < 64) bit = (unsigned)((lo >> sh) & 1ull);
            else bit = (unsigned)((hi >> (sh - 64)) & 1ull);
        }
        out[i] = bit ? '1' : '0';
    }
    out[bit_qt] = '\0';
}

static unsigned long long bits_to_u(const char *bits, size_t n)
{
    unsigned long long u = 0;
    for (size_t i = 0; i < n; ++i)
        u = (u << 1) | (bits[i] == '1' ? 1ull : 0ull);
    return u;
}

__float128 toReal(const char *bits, size_t bit_qt)
{
    if (bit_qt == 32) {
        unsigned int u32 = (unsigned int)bits_to_u(bits, 32);
        float f;
        memcpy(&f, &u32, sizeof f);
        return (__float128)f;
    }

    if (bit_qt == 64) {
        unsigned long long u64 = bits_to_u(bits, 64);
        double d;
        memcpy(&d, &u64, sizeof d);
        return (__float128)d;
    }

    if (bit_qt == 128) {
        // bits[0] = самый левый бит, bits[127] = самый правый
        unsigned long long hi = bits_to_u(bits, 64);       // старшие 64 бита
        unsigned long long lo = bits_to_u(bits + 64, 64);  // младшие 64 бита

        __float128 q = 0;
        // кладём в память q так же, как ты в toMachine lo/hi:
        // первые 8 байт в lo, следующие 8 байт в hi
        memcpy((unsigned char*)&q + 0, &lo, 8);
        memcpy((unsigned char*)&q + 8, &hi, 8);
        return q;
    }

    return 0;
}

int main() 
{
    srand(time(NULL));

    size_t n;       // Количество вариантов
    size_t k;       // Количество заданий в вариантах
    size_t bit_qt;  // Разрядность чисел в тестовых заданиях
    double a, b;    // Диапозон вещественных чисел в заданиях
    size_t p;       // Количество знаков после запятой в числах

    // FILE *input = fopen("inp32.txt", "r");  // Открытие файл с данными для 32 бит
    // FILE *input = fopen("inp64.txt", "r");  // Открытие файл с данными для 64 бит
    FILE *input = fopen("inp128.txt", "r"); // Открытие файл с данными для 128 бит

    fscanf(input, "%zu", &n);         // Запись количества вариантов
    fscanf(input, "%zu", &k);         // Запись количества заданий в вариантах
    fscanf(input, "%zu", &bit_qt);    // Запись разрядности чисел в тестовых заданиях
    fscanf(input, "%lf %lf", &a, &b); // Запись диапозона вещественных чисел в заданиях
    fscanf(input, "%zu", &p);         // Запись количества знаков после запятой в числах

    fclose(input); // Закрытие файла с данными
    
    if (bit_qt != 32 && bit_qt != 64 && bit_qt != 128) return -7;

    __float128  number128;
    double number64;
    float number32;

    char machine[bit_qt+1]; // Массив - представление вещественного числа в машинном виде
    // bit_qt+1, тк machine[bit_qt] = '\0'

    
    system("mkdir -p Задания");    // Создание каталога с заданиями для студентов 
    system("mkdir -p Проверка");   // Создание каталога с проверкой заданий

    // Cоздание файлов заданий
    for (int i = 1; i < n+1; i++)
    {
        char filename[128]; // имя файла

    // Создание файла и запись заголовка для ученика
        snprintf(filename, sizeof(filename), "Задания/Вариант%d.md", i); 
        FILE *fileStudent = fopen(filename, "w");

        fprintf(fileStudent, "# Вариант %d.\n\n", i);
        fprintf(fileStudent, "## Приведите представленные числа к машинному представлению %zu бит.\n\n", bit_qt);
        
        // Заголовок для ученика
        fprintf(fileStudent, "| N | x |\n");
        fprintf(fileStudent, "|---:|---:|\n");

    // Создание файла и запись заголовка для преподавателя
        snprintf(filename, sizeof(filename), "Проверка/Вариант%d.md", i); 
        FILE *fileTeacher = fopen(filename, "w");

        fprintf(fileTeacher, "# Вариант %d\n", i);
        
        // Заголовок для преподавателя
        fprintf(fileTeacher, "| N | X | машинное представление %zu бит | Ошибка |\n", bit_qt);
        fprintf(fileTeacher, "|---:|---:|---:|---:|\n");

        for (int j = 1; j <= k; j++)
        {
            __float128 src = randReal(a, b, p); // исходник числа

            if (bit_qt == 32)
            {
                // Округляем исходник src до float
                number32 = (float)src;

                // Строка с числом 
                fprintf(fileStudent, "| %d | %.*f |\n", j, (int)p, number32);

                // Машинное представление этого числа
                toMachine(&number32, sizeof(number32), bit_qt, machine);

                // Восстановили число из битов 
                __float128 back = toReal(machine, bit_qt);

                // Ошибка округления: разница между исходным src и числом в формате float
                __float128 err = fabsq(src - back);

                // Строка с чилом, машинным предсталением и округлением
                fprintf(fileTeacher, "| %d | %.*f | %s | %.20lf |\n", j, (int)p, number32, machine, (double)err);
            }

            if (bit_qt == 64)
            {
                // Округляем исходник src до double
                number64 = (double)src;

                fprintf(fileStudent, "| %d | %.*f |\n", j, (int)p, number64);

                toMachine(&number64, sizeof(number64), bit_qt, machine);

                // Восстановили число из битов (это будет то же самое, что number64, но как __float128)
                __float128 back = toReal(machine, bit_qt);

                // Ошибка округления: разница между исходным src и числом в формате double
                __float128 err = fabsq(src - back);

                fprintf(fileTeacher, "| %d | %.*f | %s | %.20lf |\n", j, (int)p, number64, machine, (double)err);
            }

            if (bit_qt == 128)
            {
                // Для 128: целевой формат тот же, что и исходник src
                number128 = src;

                fprintf(fileStudent, "| %d | %.*f |\n", j, (int)p, (double)number128);

                toMachine(&number128, sizeof(number128), bit_qt, machine);

                // Восстановили число из битов
                __float128 back = toReal(machine, bit_qt);

                // Ошибка округления: для 128 (если src уже __float128) обычно 0
                __float128 err = fabsq(src - back);

                fprintf(fileTeacher, "| %d | %.*f | %s | %.20lf |\n", j, (int)p, (double)number128, machine, (double)err);
            }
        }
    }

    return 0;
}

