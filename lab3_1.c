#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <tmmintrin.h>
#include <smmintrin.h>

// Я использовал double вместо float, так как в ином случае числа из файла
// теряли свою точность (относительно файла) при использовании atof()
int find_max(double *arr, double *max, int *index, int size){
    if(arr == NULL || size == 0){
        return 1;
    }

    if(max == NULL || index == NULL){
        return 2;
    }

    *max = arr[0];
    *index = 0;
    for(int i = 0; i+1 < size; i+=2){
        // Загружаем в регистр два элемента из массива
        __m128d reg_arr = _mm_loadu_pd(arr+i); // [a, b]

        // Инициализируем регистр временным максимумом
        __m128d reg_max = _mm_set1_pd(*max); // [max, max]

        // Получаем наибольшые значение между элементами массивва и максимальным значением
        __m128d reg_new_max = _mm_max_pd(reg_max, reg_arr); // [max, max] или [a, max] или [max, b] или [a, b]

        // Сравниваем (!=) "новый" максимум с текущим
        __m128d cmp_res = _mm_cmpneq_pd(reg_new_max, reg_max); // [0, 0] или [-1, 0] или [0, -1] или [-1, -1]

        // Получаем маску
        int mask = _mm_movemask_pd(cmp_res); // 00 или 10 или 01 или 11

        // Если оба числа в "новом" максимуме больше
        if(mask & 0b11){
            // Свапаем значения в "новом" максимуме
            __m128d swap = _mm_shuffle_pd(reg_new_max, reg_new_max, _MM_SHUFFLE2(0, 1));
            
            // Сравниваем (>=) "новый" максимум с его свапнутой версией, чтобы понять какое из чисел больше
            __m128d cmp = _mm_cmpge_pd(reg_new_max, swap);

            // Получаем маску
            mask = _mm_movemask_pd(cmp);
        }

        // Если второе больше
        if(mask == 2){
            *index = i+1; // Берём индекс второго числа
        }
        // Если первое больше или они равны
        else if (mask == 1 || mask == 3){
            *index = i; // Берём индекс первого числа
        }

        // Заполняем регистр наибольшим из двух чисел
        reg_new_max = _mm_max_pd(reg_new_max, _mm_shuffle_pd(reg_new_max, reg_new_max, _MM_SHUFFLE2(0, 1)));
        
        // Берём первое число из регистра
        *max = _mm_cvtsd_f64(reg_new_max);
    }

    // Если размер массива не кратен двум, то дополнительно проверяем последний элемент массива
    if(size % 2 != 0 && arr[size-1] > *max){
        *max = arr[size-1];
        *index = size-1;
    }

    return 0;
}

int main(int argc, char **argv){
    int c;
    char *file_path;
    int file_size;
    char arr_size[11];

    while((c = getopt(argc, argv, ":i:")) != -1){
        switch(c){
            case 'i': file_path = optarg;
                break;
            case ':': fprintf(stderr, "Error: missing argument\n");
                return 1;
            default: fprintf(stderr, "Error: invalid option '%c'\n", optopt);
                return 1;
        }
    }

    FILE *file = fopen(file_path, "rb");
    if(!file){
        fprintf(stderr, "Error: couldn't open the file\n");
        return 1;
    }

    if(fgets(arr_size, 11, file) == NULL){
        fprintf(stderr, "Error: couldn't read the file\n");
        return 1;
    }

    int arr_size_i = atoi(arr_size);
    if(arr_size_i < 1){
        fprintf(stderr, "Error: invalid array size\n");
        return 1;
    }
    
    double *arr = (double*)malloc(arr_size_i*sizeof(double));
    int i = 0; // arr index
    int j = 0; // str index
    char *str = (char*)calloc(64, sizeof(char));
    float num;
    while((c = fgetc(file)) != EOF){
        switch(c){
            case ';':
                arr[i++] = atof(str);
                memset(str, '\0', 64);
                j = 0;
                break;
            case ',': str[j++] = '.';
                break;
            default: str[j++] = (char)c;
                break;
        }
    }
    free(str);
    fclose(file);

    if(i != arr_size_i){
        fprintf(stderr, "Error: couldn't read an array of numbers from the file\n");
        free(arr);
        return 1;
    }

    double max;
    int index;

    find_max(arr, &max, &index, arr_size_i);

    printf("Max element: %f\nIndex: %d\n", max, index);
    // printf("%f\n", arr[index]);

    free(arr);
    return 0;
}