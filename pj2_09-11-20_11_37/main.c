/* Sistemas Operativos, DEI/IST/ULisboa 2019-20 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include "fs.h"
#include "constants.h"
#include "lib/timer.h"
#include "sync.h"
#include "lib/hash.h"

int count;

char *global_inputFile = NULL;
char *global_outputFile = NULL;
int numberThreads = 0;
int numberBuckets = 0;
int prodPst = 0;
int consPst = 0;
pthread_mutex_t commandsLock;
sem_t sem_cons;
sem_t sem_prod;
pthread_mutex_t Lock;
tecnicofs *fs;

char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
int numberCommands = 0;
int headQueue = 0;

int endFlag = 0;

static void displayUsage(const char *appName) {
    printf("Usage: %s input_filepath output_filepath threads_number\n", appName);
    exit(EXIT_FAILURE);
}

static void parseArgs(long argc, char *const argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Invalid format:\n");
        displayUsage(argv[0]);
    }

    global_inputFile = argv[1];
    global_outputFile = argv[2];
    numberThreads = atoi(argv[3]);
    if (!numberThreads) {
        fprintf(stderr, "Invalid number of threads\n");
        displayUsage(argv[0]);
    }
    numberBuckets = atoi(argv[4]);
    if (!numberBuckets) {
        fprintf(stderr, "Invalid number of buckets\n");
        displayUsage(argv[0]);
    }
}

int insertCommand(char *data) {

    sem_wait(&sem_prod);
    mutex_lock(&commandsLock);

    strcpy(inputCommands[prodPst], data);
    prodPst = (prodPst + 1) % MAX_COMMANDS;

    mutex_unlock(&commandsLock);
    sem_post(&sem_cons);

    return 1;
}

char *removeCommand() {
    char *command;

    sem_wait(&sem_cons);
    mutex_lock(&commandsLock);

    command = inputCommands[consPst];
    if(strcmp(command,"F"))
        consPst = (consPst + 1) % MAX_COMMANDS;
    else
        sem_post(&sem_cons);



    return command;
}

void errorParse(int lineNumber) {
    fprintf(stderr, "Error: line %d invalid\n", lineNumber);
    exit(EXIT_FAILURE);
}

void *processInput() {
    FILE *inputFile;
    inputFile = fopen(global_inputFile, "r");
    if (!inputFile) {
        fprintf(stderr, "Error: Could not read %s\n", global_inputFile);
        exit(EXIT_FAILURE);
    }
    char line[MAX_INPUT_SIZE];
    int lineNumber = 0;

    while (fgets(line, sizeof(line) / sizeof(char), inputFile)) {
        char token;
        char name[MAX_INPUT_SIZE];
        char rename[MAX_INPUT_SIZE];
        lineNumber++;

        int numTokens = sscanf(line, "%c %s %s", &token, name, rename);

        /* perform minimal validation */
        if (numTokens < 1) {
            continue;
        }
        switch (token) {
            case 'r':
                if (numTokens != 3)
                    errorParse(lineNumber);
                if (insertCommand(line))
                    break;
                fclose(inputFile);
                return NULL;
            case 'c':
            case 'l':
            case 'd':
                if (numTokens != 2)
                    errorParse(lineNumber);
                if (insertCommand(line))
                    break;
                fclose(inputFile);
                return NULL;
            case '#':
                break;
            default: { /* error */
                errorParse(lineNumber);
            }
        }
    }
    insertCommand("F");
    fclose(inputFile);
    return NULL;
}

FILE *openOutputFile() {
    FILE *fp;
    fp = fopen(global_outputFile, "w");
    if (fp == NULL) {
        perror("Error opening output file");
        exit(EXIT_FAILURE);
    }
    return fp;
}

void *applyCommands() {
    while (1) {
        const char *command = removeCommand();
        if (command == NULL) {
            mutex_unlock(&commandsLock);
            continue;
        }
        char token;
        char name[MAX_INPUT_SIZE];
        char rename[MAX_INPUT_SIZE];
        sscanf(command, "%c %s %s", &token, name, rename);
        printf("%s\n",command);

        int bstNum;
        bstNum = hash(name, numberBuckets);

        int iNumber;
        switch (token) {
            case 'c':
                iNumber = obtainNewInumber(fs);
                mutex_unlock(&commandsLock);
                sem_post(&sem_prod);
                create(fs, name, iNumber, bstNum);
                printf("created %s  iNum=%d\n",name,iNumber);
                break;
            case 'l':
                mutex_unlock(&commandsLock);
                sem_post(&sem_prod);
                int searchResult = lookup(fs, name, bstNum);
                if (!searchResult)
                    printf("%s not found\n", name);
                else
                    printf("%s found with inumber %d\n", name, searchResult);
                break;
            case 'd':
                mutex_unlock(&commandsLock);
                sem_post(&sem_prod);
                delete(fs, name, bstNum);
                break;
            case 'r':
                mutex_unlock(&commandsLock);
                sem_post(&sem_prod);
                int id = (lookup(fs, name, bstNum));
                if (id && !lookup(fs, rename, hash(rename, numberBuckets))) {
                    delete(fs, name, bstNum);
                    create(fs, rename, id, hash(rename, numberBuckets));
                }
                break;
            case 'F':
                mutex_unlock(&commandsLock);
                sem_post(&sem_prod);
                return NULL;

            default: { /* error */
                mutex_unlock(&commandsLock);
                sem_post(&sem_prod);
                fprintf(stderr, "Error: commands to apply\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

void runThreads(FILE *timeFp) {
    TIMER_T startTime, stopTime;
    pthread_t* workers = (pthread_t*) malloc(numberThreads * sizeof(pthread_t));
    pthread_t loader;

    TIMER_READ(startTime);

    if (pthread_create(&loader, NULL, processInput, NULL)){
        perror("Can't create thread");
        exit(EXIT_FAILURE);
    }
    for(int i = 0; i < numberThreads; i++){
            int err = pthread_create(&workers[i], NULL, applyCommands, NULL);
            if (err != 0){
                perror("Can't create thread");
                exit(EXIT_FAILURE);
            }
        }
    if(pthread_join(loader, NULL)) {
        perror("Can't join thread");
    }
    for(int i = 0; i < numberThreads; i++) {
        if(pthread_join(workers[i], NULL)) {
            perror("Can't join thread");
        }
    }

    TIMER_READ(stopTime);
    fprintf(timeFp, "TecnicoFS completed in %.4f seconds.\n", TIMER_DIFF_SECONDS(startTime, stopTime));
    free(workers);
}

void init() {
    mutex_init(&commandsLock);
    init_sem(&sem_cons, 0);
    init_sem(&sem_prod, MAX_COMMANDS);
}

void destroy() {
    mutex_destroy(&commandsLock);
    destroy_sem(&sem_prod);
    destroy_sem(&sem_cons);
}

int main(int argc, char *argv[]) {
    parseArgs(argc, argv);
    FILE *outputFp = openOutputFile();
    init();
    fs = new_tecnicofs(numberBuckets);

    runThreads(stdout);
    print_tecnicofs_tree(stdout, fs, numberBuckets);
    fflush(outputFp);
    fclose(outputFp);

    destroy();
    free_tecnicofs(fs, numberBuckets);
    exit(EXIT_SUCCESS);
}
