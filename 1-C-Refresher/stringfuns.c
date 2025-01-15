#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define BUFFER_SZ 50

//prototypes
void usage(char *);
void print_buff(char *, int);
int setup_buff(char *, char *, int);

//prototypes for functions to handle required functionality
int count_words(char *, int, int);
void reverse_string(char *, int);
void print_words(char *, int);

int setup_buff(char *buff, char *user_str, int len) {
    int i = 0, j = 0;
    int space_flag = 0;

    while (user_str[i] != '\0') {
        if (j >= len) return -1; // Input string too large

        if (user_str[i] == ' ' || user_str[i] == '\t') {
            if (!space_flag) {
                buff[j++] = ' ';
                space_flag = 1;
            }
        } else {
            buff[j++] = user_str[i];
            space_flag = 0;
        }
        i++;
    }

    while (j < len) {
        buff[j++] = '.'; // Fill remaining buffer with dots
    }

    return j;
}

void print_buff(char *buff, int len) {
    printf("Buffer:  ");
    for (int i = 0; i < len; i++) {
        putchar(*(buff + i));
    }
    putchar('\n');
}

void usage(char *exename) {
    printf("usage: %s [-h|c|r|w|x] \"string\" [other args]\n", exename);
}

int count_words(char *buff, int len, int str_len) {
    int count = 0;
    int in_word = 0;

    for (int i = 0; i < str_len && buff[i] != '.'; i++) {
        if (buff[i] != ' ' && !in_word) {
            in_word = 1;
            count++;
        } else if (buff[i] == ' ') {
            in_word = 0;
        }
    }

    return count;
}

void reverse_string(char *buff, int len) {
    int start = 0, end = len - 1;
    while (end >= 0 && buff[end] == '.') end--; // Ignore trailing dots

    while (start < end) {
        char temp = buff[start];
        buff[start] = buff[end];
        buff[end] = temp;
        start++;
        end--;
    }
}

void print_words(char *buff, int len) {
    int word_start = -1;
    int word_count = 1;

    for (int i = 0; i < len && buff[i] != '.'; i++) {
        if (buff[i] != ' ' && word_start == -1) {
            word_start = i;
        } else if ((buff[i] == ' ' || buff[i + 1] == '.') && word_start != -1) {
            printf("%d. %.*s (%d)\n", word_count++, i - word_start + 1, buff + word_start, i - word_start + 1);
            word_start = -1;
        }
    }
}

int main(int argc, char *argv[]) {
    char *buff;             //placehoder for the internal buffer
    char *input_string;     //holds the string provided by the user on cmd line
    char opt;               //used to capture user option from cmd line
    int rc;                 //used for return codes
    int user_str_len;       //length of user supplied string

    //TODO:  #1. WHY IS THIS SAFE, aka what if argv[1] does not exist?
    //      The program exits early if fewer than 2 arguments are passed
    if ((argc < 2) || (*argv[1] != '-')) {
        usage(argv[0]);
        exit(1);
    }

    opt = (char)*(argv[1] + 1);   //get the option flag

    //handle the help flag and then exit normally
    if (opt == 'h') {
        usage(argv[0]);
        exit(0);
    }

    //TODO:  #2 Document the purpose of the if statement below
    //      Making sure there is a second argument
    if (argc < 3) {
        usage(argv[0]);
        exit(1);
    }

    input_string = argv[2]; //capture the user input string

    //TODO:  #3 Allocate space for the buffer using malloc and
    //          handle error if malloc fails by exiting with a 
    //          return code of 99
    buff = (char *)malloc(BUFFER_SZ);
    if (!buff) {
        fprintf(stderr, "Error: memory allocation failed\n");
        exit(99);
    }

    user_str_len = setup_buff(buff, input_string, BUFFER_SZ);
    if (user_str_len < 0) {
        printf("Error setting up buffer, error = %d", user_str_len);
        free(buff);
        exit(2);
    }

    switch (opt) {
        case 'c':
            rc = count_words(buff, BUFFER_SZ, user_str_len);  
            if (rc < 0) {
                printf("Error counting words, rc = %d", rc);
                free(buff);
                exit(2);
            }
            printf("Word Count: %d\n", rc);
            break;
        case 'r':
            reverse_string(buff, user_str_len);
            printf("Reversed String: %s\n", buff);
            break;
        case 'w':
            print_words(buff, user_str_len);
            break;
        default:
            usage(argv[0]);
            free(buff);
            exit(1);
    }

    //TODO:  #6 Dont forget to free your buffer before exiting
    print_buff(buff, BUFFER_SZ);
    free(buff);
    exit(0);
}

//TODO:  #7  Notice all of the helper functions provided in the 
//          starter take both the buffer as well as the length.  Why
//          do you think providing both the pointer and the length
//          is a good practice, after all we know from main() that 
//          the buff variable will have exactly 50 bytes?
//  
//          Might have a different buffer size and prevents errors caused by accessing memory beyond the buffer range
