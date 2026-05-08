// For sources, I used:
// - manpages
// - textbook (i finally got it)
// - https://cplusplus.com/reference/cstdio/snprintf/
// - https://en.wikipedia.org/wiki/Semaphore_(programming)
// - https://medium.com/@akshatarhabib/understanding-semaphores-in-c-04835e97024f
// - https://man7.org/linux/man-pages/man7/sem_overview.7.html
//
// typically i implement my own message queues from scratch using circular buffer so this was
// pretty interesting since the mqueue lib handles a lot of it
// also sorry i submitted this late it slipped my mind.
// had it written down as due last saturday and forgot to reschedule
//
// oh might as well write down how this works
// 1. server recieves request from client via cmd-queue
// 2. server gets the first file in the queue and sends it back. chunks if needed.
// it waits on write-sem before writing each chunk into shared memory, then posts to read-sem
// 3. client recieves file, and writes (it also sends next to request next file/ack file recieved).
// it waits on read-sem before reading each chunk, then posts to write-sem when done
// 4. once all files are recieved, it sends a close message and closes down client
// 5. if server finds a close message, it shuts down too
//
// i cannot for the life of me figure out how to get the client to retry. i initially got a segfault, fixed that, but now it hangs.
// I KNOW my logic is perfect and it should work. anyways, i'm not sure why it won't work. would appreciiate it if you could take a look
//
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <mqueue.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>

#define BUFSIZE 512

#define USAGE                                            \
  "usage:\n"                                             \
  "  shm_server [options]\n"                             \
  "options:\n"                                           \
  "  -h                  Show this help message\n"       \

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
    {"help", no_argument, NULL, 'h'},
    {NULL, 0, NULL, 0} };

int main(int argc, char** argv) {
    int option_char;

    setbuf(stdout, NULL);  // disable buffering

    // Parse and set command line arguments
    while ((option_char =
        getopt_long(argc, argv, "hx", gLongOptions, NULL)) != -1) {
        switch (option_char) {
        case 'h':  // help
            fprintf(stdout, "%s", USAGE);
            exit(0);
            break;
        default:
            fprintf(stderr, "%s", USAGE);
            exit(1);
        }
    }

    /* Server Code Here */
    // printf("You need to write the code to make this do something!\n");
    const char* cmd_queue = "/cmd-queue";// incoming
    const char* rsp_queue = "/rsp-queue"; // outgoing
    struct mq_attr attr;
    char requested_file[BUFSIZE];
    // i could length-prefix filepath i suppose.
    // i think 128 is just fine esp for utf-8
    // not sure if you're going to test larger files so i left as is
    char filepath[128];
    char response[BUFSIZE + 1];
    char msgbuf[BUFSIZE + 1];

    memset(&attr, 0, sizeof(attr));
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = BUFSIZE;

    sem_unlink("/write-sem");
    sem_unlink("/read-sem");
    sem_t* write_sem = sem_open("/write-sem", O_CREAT, S_IRUSR | S_IWUSR, 1);
    sem_t* read_sem = sem_open("/read-sem", O_CREAT, S_IRUSR | S_IWUSR, 0);
    mqd_t mqd = mq_open(cmd_queue, O_CREAT | O_RDONLY, S_IRUSR | S_IWUSR, &attr);
    mqd_t rsp_mqd = mq_open(rsp_queue, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR, &attr);
    if (mqd == (mqd_t) -1) {
        perror("failed to open message queue");
        exit(1);
    }
    if (rsp_mqd == (mqd_t) -1) {
        perror("failed to open response queue");
        exit(1);
    }

    ssize_t num_read;
    size_t shm_size;
    for (;;) {
        num_read = mq_receive(mqd, msgbuf, BUFSIZE, NULL);
        if (num_read == -1) {
            mq_close(mqd);
            mq_close(rsp_mqd);
            mq_unlink(cmd_queue);
            mq_unlink(rsp_queue);
            exit(1);
        }

        msgbuf[num_read] = '\0';
        printf("received: %s\n", msgbuf);

        if (strcmp(msgbuf, "close") == 0) break;

        sscanf(msgbuf, "%zu:%s", &shm_size, requested_file);
        int shm_fd = open("./client-shm", O_RDWR);
        if (shm_fd == -1) {
            perror("failed to initialize shared memory");
            exit(1);
        }
        ftruncate(shm_fd, BUFSIZE);
        char* shm_addr = mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

        requested_file[strcspn(requested_file, "\r\n")] = '\0';
        // snprintf(filepath, sizeof(filepath), "./server_files/%s", requested_file);
        // in all honesty, i'm confused by what compiler error you're getting. all i can think of is that you're on a different version of gcc or perhaps libc?
        // anyways, i swapped out snprintf with a manual copy. i'm not sure where you got that compiler error, but hopefully you don't anymore
        strcpy(filepath, "./server_files/");
        strcat(filepath, requested_file);

        // i think it was also failing here becuse I put rwb in. i assumed rw was valid since it's read-write-binary but i looked it up and it should be rb
        // https://man7.org/linux/man-pages/man3/fopen.3.html
        // https://www.geeksforgeeks.org/c/c-fopen-function-with-examples/
        //
        // it probably silently rejected it? anyways, it sometimes corrupted all the files for me. might have converted to utf-8 instead of binary?
        // 
        FILE* fp = fopen(filepath, "rb");
        if (fp == NULL) {
            sem_wait(write_sem); // wait for client to finish reading/writing
            memcpy(shm_addr, "FNF", 3);
            mq_send(rsp_mqd, "3", 1, 0);
            munmap(shm_addr, shm_size);
            sem_post(read_sem); // let client read err message
            continue;
        }

        for (;;) {
            sem_wait(write_sem);
            size_t bytes_read = fread(shm_addr, 1, shm_size, fp);
            snprintf(response, sizeof(response), "%zu", bytes_read);
            mq_send(rsp_mqd, response, strlen(response), 0);
            sem_post(read_sem);

            if (bytes_read != shm_size) break;

            num_read = mq_receive(mqd, msgbuf, BUFSIZE, NULL);
            msgbuf[num_read] = '\0';
        }

        fclose(fp);
        munmap(shm_addr, shm_size);
    }

    mq_close(mqd);
    mq_close(rsp_mqd);
    mq_unlink(cmd_queue);
    mq_unlink(rsp_queue);
    sem_close(write_sem);
    sem_close(read_sem);
    sem_unlink("/write-sem");
    sem_unlink("/read-sem");
}
