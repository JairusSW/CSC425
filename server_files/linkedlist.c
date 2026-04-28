#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>


/* Declare the linked list structures */

// Definition of each node
struct node {
   int data;
   struct node *next;
};

// The head node (start of list)
struct node *head = NULL;

// Variable to represent the current node
struct node *current = NULL;



/* Main ========================================================= */
int main(int argc, char** argv) {
    int option_char = 0;
    int number = 100;



    setbuf(stdout, NULL);  // disable buffering

    if ((number < 0) || (number > 100000)) {
        fprintf(stderr, "ERROR: %s @ %d: number (%d)\n", __FILE__, __LINE__,
            number);
        exit(1);
    }

}




void addElement(){

    // Add an element to the list
}



