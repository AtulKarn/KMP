#include <string.h> 
#include <stdlib.h> //malloc
#include <stdio.h>
#include <time.h>
#include <dirent.h>
#include <omp.h>



#define max( a, b ) ( ((a) > (b)) ? (a) : (b) )

//Dynamicaly growing array implementation
typedef struct {
    char* fileName;
    int* array;
    size_t used;
    size_t size;
} FileFindings;

void initFileFindings(FileFindings* a, size_t initialSize) {
    a->array = (int*)malloc(initialSize * sizeof(int));
    a->used = 0;
    a->size = initialSize;
}

void insertMatch(FileFindings* a, int element) {
    if (a->used == a->size) {
        a->size *= 2;
        a->array = (int*)realloc(a->array, a->size * sizeof(int));
    }
    a->array[a->used++] = element;
}

void freeFileFindings(FileFindings* a) {
    free(a->array);
    a->array = NULL;
    a->used = a->size = 0;
}

void checkPartialMatch(char* pattern, int T[]) {
    int position, t;
    int patternLen = strlen(pattern);

    T[0] = 0; T[1] = 0; t = 0;
    for (position = 2; position <= patternLen; position++)
    {
        while ((t > 0) && (pattern[t] != pattern[position - 1]))
        {
            t = T[t];
        }
        if (pattern[t] == pattern[position - 1])
        {
            t++;
        }
        T[position] = t;
    }
}

size_t getFilesCount(char userPath[])
{
    struct dirent* file;
    DIR* path;
    size_t filesCount = 0;
    if ((path = opendir(userPath)))
    {
        while ((file = readdir(path)))
        {
            char lastCharacter = file->d_name[strlen(file->d_name) - 1];
            if (lastCharacter != '~' && file->d_type == DT_REG)
            {
                filesCount++;
            }
        }
    }
    return filesCount++;
}

FileFindings* fillFilePaths(char userPath[], int length)
{
    struct dirent* file;
    DIR* path;
    char userFilePath[1000];
    int i = 0;
    FileFindings* result = (FileFindings*)malloc(length * sizeof * result);


    if (result && (path = opendir(userPath)))
    {
        while ((file = readdir(path)))
        {
            char lastCharacter = file->d_name[strlen(file->d_name) - 1];
            if (lastCharacter != '~' && file->d_type == DT_REG)
            {
                strcpy(userFilePath, userPath);
                strcat(userFilePath, file->d_name);
                result[i].fileName = strdup(userFilePath);
                i++;
            }
        }
        closedir(path);
        return result;
    }
    else
    {
        return NULL;
    }
}

void kmp(int T[], char keyWord[], char searchString[], FileFindings* fileFinding)
{

    int position = 1;
    int matchesCount = 0;
    unsigned int targetLen = strlen(searchString);
    int patternLen = strlen(keyWord);
    initFileFindings(fileFinding, 1);

    for (position = 1; position <= targetLen - patternLen + 1; position = position + max(1, matchesCount - T[matchesCount]))//dopóki mieścimy się w tekście
    {
        matchesCount = T[matchesCount];
        while ((matchesCount < patternLen) && (keyWord[matchesCount] == searchString[position + matchesCount - 1]))
        {
            matchesCount++;
        }
        if (matchesCount == patternLen)
        {
            insertMatch(fileFinding, position);
        }
    }
}

int main(int argc, char** argv)
{
    FileFindings* fileFindings;

    char keyWord[100];
    char userPath[1000];
    int filesCount = 0;
    int T[100];

    printf("Enter path to directory with files (ended with '/')\n");
    scanf(" %[^\n]s", userPath);
    printf("Keyword to search (max 100 characeters): ");
    scanf(" %[^\n]s", keyWord);

    checkPartialMatch(keyWord, T);
    if (strlen(userPath) < 2)
    {
        strcpy(userPath, "/home/marcin/Education/files/");
    }
    filesCount = getFilesCount(userPath);
    fileFindings = fillFilePaths(userPath, filesCount);

    //Seq measuring
    //clock_t start = clock();

    //OMP measuring
    double start_time = omp_get_wtime();

#pragma omp parallel for default(none) shared(filesCount, fileFindings, T, keyWord)
    for (int i = 0; i < filesCount; i++)
    {
        FILE* f = fopen(fileFindings[i].fileName, "rb");//binary mode (najlepsza standaryzacja)
        fseek(f, 0, SEEK_END);
        long fsize = ftell(f);
        fseek(f, 0, SEEK_SET);
        char* fileString = (char*)malloc(fsize + 1);
        fread(fileString, fsize, 1, f);
        kmp(T, keyWord, fileString, &fileFindings[i]);
        fclose(f);
        fileString[fsize] = 0;
        free(fileString);
    }

    //clock_t end = clock();
    //float time = (float)(end - start) / CLOCKS_PER_SEC;
    //OMP measuring
    double time = omp_get_wtime() - start_time;
    puts("wyniki wyszukiwania:");




    //Results
    int sumaWystapien = 0;
    for (int i = 0; i < filesCount; i++) {
        //printf("========================\n");
        //puts(fileFindings[i].fileName);
        sumaWystapien += fileFindings[i].used;
        for (int i2 = 0; i2 < fileFindings[i].used; i2++)
        {
            //printf("%d\n", fileFindings[i].array[i2]);
        }
    }
    printf("Czas: ");
    printf("%f\n", time);
    printf("Suma wystąpień:");
    printf("%d\n", sumaWystapien);


    //clean up
    for (int i = 0; i < filesCount; i++) {
        for (int i2 = 0; i2 < fileFindings[i].used; i2++)
        {
            freeFileFindings(&fileFindings[i]);
        }
    }
    free(fileFindings);

    return 0;
}