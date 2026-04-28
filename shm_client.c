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