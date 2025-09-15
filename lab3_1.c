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
    for(int i = 0; i < size; i+=2){
        __m128d reg_arr = _mm_loadu_pd(arr+i);
        __m128d reg_max = _mm_set1_pd(*max);
    
        __m128d reg_new_max = _mm_max_pd(reg_max, reg_arr);

        __m128d cmp_res = _mm_cmpneq_pd(reg_new_max, reg_max);
                
        int mask = _mm_movemask_pd(cmp_res);

        if(mask & 0b01){
            *index = i;
        }
        else if(mask & 0b10){
            *index = i+1;
        }
        else if(mask & 0b00){
            __m128d swap = _mm_shuffle_pd(reg_new_max, reg_new_max, _MM_SHUFFLE2(0, 1));
            __m128d cmp = _mm_cmpgt_pd(reg_new_max, swap);
            mask = _mm_movemask_pd(cmp);
            if(mask & 0b01){
                *index = i;
            }
            else{
                *index = i+1;
            }
        }

        reg_new_max = _mm_max_pd(reg_new_max, _mm_shuffle_pd(reg_new_max, reg_new_max, _MM_SHUFFLE2(0, 1)));
        *max = _mm_cvtsd_f64(reg_new_max);
    }

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

    free(arr);
    return 0;
}