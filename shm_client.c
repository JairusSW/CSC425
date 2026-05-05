#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <mqueue.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>

int readFilenames();



#define BUFSIZE 512

// Variables for reading input filenames
#define MAX_LENGTH 64
char files[5][MAX_LENGTH];
char buffer[MAX_LENGTH];
char* filename = "filelist.txt";

#define USAGE                                                \
  "usage:\n"                                                 \
  "  shm_client [options]\n"                                 \
  "options:\n"                                               \
  "  -n                  Number of files to transfer\n"      \
  "  -h                  Show this help message\n"

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = { {"help", no_argument, NULL, 'h'},
                                       {NULL, 0, NULL, 0} };

/* Main ========================================================= */
int main(int argc, char** argv) {
    int option_char = 0;
    int num_files = 20;

    setbuf(stdout, NULL);

    /* Parse and set command line arguments */
    while ((option_char =
        getopt_long(argc, argv, "xn:", gLongOptions, NULL)) != -1) {
        switch (option_char) {
        case 'n': // num files
            num_files = atoi(optarg);
            break;
        case 'h':  // help
            fprintf(stdout, "%s", USAGE);
            exit(0);
            break;
        default:
            fprintf(stderr, "%s", USAGE);
            exit(1);
        }
    }

    // Read in the names of the files to transfer
    readFilenames();

    /* This section will let you verify that you can loop through the list of files the appropriate number of times.
     *  You can modify this to call your code to request a file "num_files" times
     */
    fprintf(stdout, "Num files: %d\n", num_files);

    for (int i = 0; i < num_files; i++) {
        printf("%d - ", i);
        printf("%s", files[i % 5]);
    }

    const char* cmd_queue = "/cmd-queue";
    const char* rsp_queue = "/rsp-queue";

    char msg[BUFSIZE];
    char response[BUFSIZE + 1];
    char output_path[BUFSIZE * 2];

    printf("========================\n");
    int shm_fd = open("./client-shm", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (shm_fd == -1) {
        perror("failed to initialize shared memory");
        exit(1);
    }
    ftruncate(shm_fd, BUFSIZE);
    char* shm_addr = mmap(NULL, BUFSIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    mqd_t mqd;
    mqd_t rsp_mqd;
    while ((mqd = mq_open(cmd_queue, O_WRONLY)) == (mqd_t) -1) {
        printf("connecting...\n");
        sleep(1);
    }
    while ((rsp_mqd = mq_open(rsp_queue, O_RDONLY)) == (mqd_t) -1) sleep(1);
    sem_t* write_sem;
    sem_t* read_sem;
    while ((write_sem = sem_open("/write-sem", 0)) == SEM_FAILED && errno == ENOENT) sleep(1);
    while ((read_sem = sem_open("/read-sem", 0)) == SEM_FAILED && errno == ENOENT) sleep(1);

    if (mqd == (mqd_t) -1) {
        perror("failed to open message queue");
        exit(1);
    }
    if (rsp_mqd == (mqd_t) -1) {
        perror("failed to open response queue");
        exit(1);
    }


    for (int i = 0; i < num_files; i++) {
        files[i % 5][strcspn(files[i % 5], "\r\n")] = '\0';
        snprintf(msg, sizeof(msg), "%zu:%s", (size_t)BUFSIZE, files[i % 5]);
        mq_send(mqd, msg, strlen(msg), 0);
        snprintf(output_path, sizeof(output_path), "./client_files/%s", files[i % 5]);
        FILE* fp = fopen(output_path, "wb");

        for (;;) {
            size_t rsp_len = mq_receive(rsp_mqd, response, BUFSIZE, NULL);
            response[rsp_len] = '\0';
            size_t bytes = strtoull(response, NULL, 10);
            sem_wait(read_sem); // wait until the server finishes writing next chunk
            if (bytes == 3 && memcmp(shm_addr, "FNF", 3) == 0) {
                sem_post(write_sem);
                break;
            }
            fwrite(shm_addr, 1, bytes, fp);
            sem_post(write_sem); // let server write another chunk
            if (bytes != BUFSIZE) break;

            mq_send(mqd, "next", 4, 0);
        }

        fclose(fp);
        printf("sent fil %d - %s\n", i, files[i % 5]);
    }

    mq_send(mqd, "close", 8, 0);
    mq_close(mqd);
    mq_close(rsp_mqd);
    munmap(shm_addr, BUFSIZE);
    unlink("./client-shm");
    sem_close(write_sem);
    sem_close(read_sem);
}

// Read in the filenames to transfer
int readFilenames() {

    int count = 0;
    FILE* fp = fopen(filename, "r");

    if (fp == NULL) {
        return 1;
    }

    while (fgets(buffer, MAX_LENGTH, fp)) {
        strcpy(files[count], buffer);
        count++;
    }

    fclose(fp);
    return 0;
}
