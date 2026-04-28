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
    printf("You need to write the code to make this do something!\n");
}
