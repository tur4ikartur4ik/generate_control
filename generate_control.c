#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <math.h>

typedef union {
    // Реальное представление
    float f;
    double d;
    long double ld;

    // Машинное представление
    uint32_t u32;
    uint64_t u64;
    __uint128_t u128;
} U;

int getMachine(const void* x, size_t type, __uint128_t* out)
{
    U u = {0};

    if (x == NULL || out == NULL)
        return 0;

    switch (type)
    {
    case 4:     // float -> 32 бит
        u.f = *(const float*)x;
        *out = u.u32;
        return 1;

    case 8:     // double -> 64 бит
        u.d = *(const double*)x;
        *out = u.u64;
        return 1;

    case 16:    // long double -> 128 бит
        u.ld = *(const long double*)x;
        *out = u.u128;
        return 1;

    default:
        return 0;
    }
}

int getReal(__uint128_t x, size_t type, void* out)
{
    U u = {0};

    if (out == NULL)
        return 0;

    switch (type)
    {
    case 4:     // 32 бит -> float
        u.u32 = (uint32_t)x;
        *(float*)out = u.f;
        return 1;

    case 8:     // 64 бит -> double
        u.u64 = (uint64_t)x;
        *(double*)out = u.d;
        return 1;

    case 16:    // 128 бит -> long double
        u.u128 = x;
        *(long double*)out = u.ld;
        return 1;

    default:
        return 0;
    }
}

// Запись машинного представления в файл f
void print_bits(FILE* f, __uint128_t x, int bits_count)
{
    for (int i = bits_count - 1; i >= 0; i--)
        fputc(((x >> i) & 1) ? '1' : '0', f);
    // x >> i  : сдвигаем число вправо на i бит,
    // чтобы интересующий нас бит оказался на младшей позиции
    // & 1 : оставляем только этот младший бит (0 или 1)
    // ? '1':'0' : если бит равен 1, печатаем символ '1', иначе печатаем символ '0'
    // fputc(..., f) : записываем этот символ в файл f
}

// Генерация случайного вещественного числа от a до b с p знаками после запятой
// Реализовано после main
long double rand_num(long double a, long double b, int p);

// Создание файла с заголовком
// Реализовано после main
FILE* createFile(const char* repository, int numVar, int bits, const char* header, const char* h);

int main() {
    // Для случайных чисел
    srand(time(NULL));

    int n;       // Количество вариантов
    int k;       // Количество заданий в вариантах
    int bits;  // Разрядность чисел в тестовых заданиях
    double a, b;    // Диапозон вещественных чисел в заданиях
    int p;       // Количество знаков после запятой в числах
    
    long double number;

    FILE *input = fopen("inp32.txt", "r");  // Открытие файл с данными для 32 бит
    // FILE *input = fopen("inp64.txt", "r");  // Открытие файл с данными для 64 бит
    // FILE *input = fopen("inp128.txt", "r"); // Открытие файл с данными для 128 бит

    fscanf(input, "%d", &n);         // Запись количества вариантов
    fscanf(input, "%d", &k);         // Запись количества заданий в вариантах
    fscanf(input, "%d", &bits);    // Запись разрядности чисел в тестовых заданиях
    fscanf(input, "%lf %lf", &a, &b); // Запись диапозона вещественных чисел в заданиях
    fscanf(input, "%d", &p);         // Запись количества знаков после запятой в числах

    fclose(input); // Закрытие файла с данными
    
    if (bits != 32 && bits != 64 && bits != 128) {
        printf("В файле оказались неверные данные (битность)\n");
        return -1;
    }

    // Создаем папки
    system("mkdir -p Задания");
    system("mkdir -p Проверка");
    
    // Каждый вариант
    for (int i = 1; i <= n; i++) 
    {   
        // Создаём файлы с заголовками
        FILE* student = createFile("Задания", i, bits, "| N | x |\n", "|:-|:-|\n");

        FILE* teacher = createFile("Проверка", i, 0, 
            "| N | X | Машинное представление | Ошибка |\n", "|:-|:-|:-|:-|\n");

        for (int j = 1; j <= k; j++)
        {
            number = rand_num(a, b, p); // Вещественное число

            long double err = 0.0L; // Ошибка

            fprintf(student, "| %d | %.*Lf |\n", j, p, number); // Записываем задание

            if (bits == 32)
            {
                float x = (float)number;  // Исходное число приводим к типу float
                float y;        // Сюда восстановим число обратно из машинного представления
                __uint128_t m;  // Здесь будет храниться машинное представление числа x

                getMachine(&x, sizeof(float), &m); // Переводим float x в его машинное представление
                getReal(m, sizeof(float), &y); // Восстанавливаем число y из машинного представления m

                err = fabsl(number - (long double)y); // Вычисляем ошибку после перевода и восстановления

                fprintf(teacher, "| %d | %.*Lf | ", j, p, number); // Печатаем номер и исходное число
                print_bits(teacher, m, 32); // Печатаем 32 бита машинного представления
                fprintf(teacher, " | %Le |\n", err); // Печатаем ошибку
            }
            else if (bits == 64)
            {
                double x = (double)number;
                double y;
                __uint128_t m;

                getMachine(&x, sizeof(double), &m);
                getReal(m, sizeof(double), &y);

                err = fabsl(number - (long double)y);

                fprintf(teacher, "| %d | %.*Lf | ", j, p, number);
                print_bits(teacher, m, 64);
                fprintf(teacher, " | %Le |\n", err);
            }
            else if (bits == 128)
            {
                long double x = number;
                long double y;
                __uint128_t m;

                getMachine(&x, sizeof(long double), &m);
                getReal(m, sizeof(long double), &y);

                err = fabsl(number - y);

                fprintf(teacher, "| %d | %.*Lf | ", j, p, number);
                print_bits(teacher, m, 128);
                fprintf(teacher, " | %Le |\n", err);
            }
        }

        fclose(student);
        fclose(teacher);
    }
    
    printf("Готово.\n");    
    return 0;
}

// Генерация случайного вещественного числа от a до b с p знаками после запятой
long double rand_num(long double a, long double b, int p) {
    long double r = a + (long double)rand() / RAND_MAX * (b - a);
    
    // Округление до P знаков
    long double mult = powl(10, p);
    r = roundl(r * mult) / mult;

    return r;
}

// Создание файла с заголовком
FILE* createFile(const char* repository, int numVar, int bits, const char* header, const char* h)
{
    char filename[128];

    sprintf(filename, "%s/Вариант%d.md", repository, numVar);

    FILE* f = fopen(filename, "w");

    fprintf(f, "# Вариант %d.\n\n", numVar);
    if (bits != 0)
        fprintf(f, "## Приведите представленные числа к машинному представлению %d бит.\n\n", bits);

    fprintf(f, "%s", header);
    fprintf(f, "%s", h);

    return f;
}
