#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdbool.h>

#include "db.h"
#include "sdbsc.h"

int open_db(char *dbFile, bool should_truncate)
{
    // Set permissions: rw-rw----
    // see sys/stat.h for constants
    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP;

    // open the file if it exists for Read and Write,
    // create it if it does not exist
    int flags = O_RDWR | O_CREAT;

    if (should_truncate)
        flags += O_TRUNC;

    // Now open file
    int fd = open(dbFile, flags, mode);

    if (fd == -1)
    {
        // Handle the error
        printf(M_ERR_DB_OPEN);
        return ERR_DB_FILE;
    }

    return fd;
}

int get_student(int fd, int id, student_t *s) {
    off_t offset = id * STUDENT_RECORD_SIZE;
    if (lseek(fd, offset, SEEK_SET) == -1) {
        printf(M_ERR_DB_READ);
        return ERR_DB_FILE;
    }
    
    if (read(fd, s, STUDENT_RECORD_SIZE) != STUDENT_RECORD_SIZE) {
        return SRCH_NOT_FOUND;
    }
    
    if (s->id == 0) {
        return SRCH_NOT_FOUND;
    }
    return NO_ERROR;
}

int add_student(int fd, int id, char *fname, char *lname, int gpa) {
    student_t student;
    if (get_student(fd, id, &student) == NO_ERROR) {
        printf(M_ERR_DB_ADD_DUP, id);
        return ERR_DB_OP;
    }
    
    student_t new_student = {id, "", "", gpa};
    strncpy(new_student.fname, fname, sizeof(new_student.fname) - 1);
    strncpy(new_student.lname, lname, sizeof(new_student.lname) - 1);
    
    off_t offset = id * STUDENT_RECORD_SIZE;
    if (lseek(fd, offset, SEEK_SET) == -1) {
        printf(M_ERR_DB_READ);
        return ERR_DB_FILE;
    }
    
    if (write(fd, &new_student, STUDENT_RECORD_SIZE) != STUDENT_RECORD_SIZE) {
        printf(M_ERR_DB_WRITE);
        return ERR_DB_FILE;
    }
    
    printf(M_STD_ADDED, id);
    return NO_ERROR;
}

int del_student(int fd, int id) {
    student_t student;
    if (get_student(fd, id, &student) != NO_ERROR) {
        printf(M_STD_NOT_FND_MSG, id);
        return ERR_DB_OP;
    }
    
    off_t offset = id * STUDENT_RECORD_SIZE;
    if (lseek(fd, offset, SEEK_SET) == -1) {
        printf(M_ERR_DB_READ);
        return ERR_DB_FILE;
    }
    
    if (write(fd, &EMPTY_STUDENT_RECORD, STUDENT_RECORD_SIZE) != STUDENT_RECORD_SIZE) {
        printf(M_ERR_DB_WRITE);
        return ERR_DB_FILE;
    }
    
    printf(M_STD_DEL_MSG, id);
    return NO_ERROR;
}

int count_db_records(int fd) {
    student_t student;
    int count = 0;
    lseek(fd, 0, SEEK_SET);
    
    while (read(fd, &student, STUDENT_RECORD_SIZE) == STUDENT_RECORD_SIZE) {
        if (memcmp(&student, &EMPTY_STUDENT_RECORD, STUDENT_RECORD_SIZE) != 0) {
            count++;
        }
    }
    
    if (count == 0) {
        printf(M_DB_EMPTY);
    } else {
        printf(M_DB_RECORD_CNT, count);
    }
    return count;
}

int print_db(int fd) {
    student_t student;
    int found = 0;
    lseek(fd, 0, SEEK_SET);
    
    while (read(fd, &student, STUDENT_RECORD_SIZE) == STUDENT_RECORD_SIZE) {
        if (memcmp(&student, &EMPTY_STUDENT_RECORD, STUDENT_RECORD_SIZE) != 0) {
            if (!found) {
                printf(STUDENT_PRINT_HDR_STRING, "ID", "FIRST NAME", "LAST_NAME", "GPA");
                found = 1;
            }
            printf(STUDENT_PRINT_FMT_STRING, student.id, student.fname, student.lname, student.gpa / 100.0);
        }
    }
    
    if (!found) {
        printf(M_DB_EMPTY);
    }
    return NO_ERROR;
}

void print_student(student_t *s) {
    if (!s || s->id == 0) {
        printf(M_ERR_STD_PRINT);
        return;
    }
    printf(STUDENT_PRINT_HDR_STRING, "ID", "FIRST NAME", "LAST_NAME", "GPA");
    printf(STUDENT_PRINT_FMT_STRING, s->id, s->fname, s->lname, s->gpa / 100.0);
}

int validate_range(int id, int gpa)
{

    if ((id < MIN_STD_ID) || (id > MAX_STD_ID))
        return EXIT_FAIL_ARGS;

    if ((gpa < MIN_STD_GPA) || (gpa > MAX_STD_GPA))
        return EXIT_FAIL_ARGS;

    return NO_ERROR;
}

void usage(char *exename)
{
    printf("usage: %s -[h|a|c|d|f|p|z] options.  Where:\n", exename);
    printf("\t-h:  prints help\n");
    printf("\t-a id first_name last_name gpa(as 3 digit int):  adds a student\n");
    printf("\t-c:  counts the records in the database\n");
    printf("\t-d id:  deletes a student\n");
    printf("\t-f id:  finds and prints a student in the database\n");
    printf("\t-p:  prints all records in the student database\n");
    printf("\t-x:  compress the database file [EXTRA CREDIT]\n");
    printf("\t-z:  zero db file (remove all records)\n");
}

int main(int argc, char *argv[])
{
    char opt;      // user selected option
    int fd;        // file descriptor of database files
    int rc;        // return code from various operations
    int exit_code; // exit code to shell
    int id;        // userid from argv[2]
    int gpa;       // gpa from argv[5]

    student_t student = {0};

    if ((argc < 2) || (*argv[1] != '-'))
    {
        usage(argv[0]);
        exit(1);
    }

    opt = (char)*(argv[1] + 1); // get the option flag

    if (opt == 'h')
    {
        usage(argv[0]);
        exit(EXIT_OK);
    }

    fd = open_db(DB_FILE, false);
    if (fd < 0)
    {
        exit(EXIT_FAIL_DB);
    }

    exit_code = EXIT_OK;
    switch (opt)
    {
    case 'a':
        if (argc != 6)
        {
            usage(argv[0]);
            exit_code = EXIT_FAIL_ARGS;
            break;
        }

        id = atoi(argv[2]);
        gpa = atoi(argv[5]);

        exit_code = validate_range(id, gpa);
        if (exit_code == EXIT_FAIL_ARGS)
        {
            printf(M_ERR_STD_RNG);
            break;
        }

        rc = add_student(fd, id, argv[3], argv[4], gpa);
        if (rc < 0)
            exit_code = EXIT_FAIL_DB;

        break;

    case 'c':
        rc = count_db_records(fd);
        if (rc < 0)
            exit_code = EXIT_FAIL_DB;
        break;

    case 'd':
        if (argc != 3)
        {
            usage(argv[0]);
            exit_code = EXIT_FAIL_ARGS;
            break;
        }
        id = atoi(argv[2]);
        rc = del_student(fd, id);
        if (rc < 0)
            exit_code = EXIT_FAIL_DB;

        break;

    case 'f':
        if (argc != 3)
        {
            usage(argv[0]);
            exit_code = EXIT_FAIL_ARGS;
            break;
        }
        id = atoi(argv[2]);
        rc = get_student(fd, id, &student);

        switch (rc)
        {
        case NO_ERROR:
            print_student(&student);
            break;
        case SRCH_NOT_FOUND:
            printf(M_STD_NOT_FND_MSG, id);
            exit_code = EXIT_FAIL_DB;
            break;
        default:
            printf(M_ERR_DB_READ);
            exit_code = EXIT_FAIL_DB;
            break;
        }
        break;

    case 'p':
        //    arv[0] arv[1]
        // prog_name     -p
        //-----------------
        // example:  prog_name -p
        rc = print_db(fd);
        if (rc < 0)
            exit_code = EXIT_FAIL_DB;
        break;

    case 'z':
        //    arv[0] arv[1]
        // prog_name     -x
        //-----------------
        // example:  prog_name -x
        // HINT:  close the db file, we already have fd
        //       and reopen db indicating truncate=true
        close(fd);
        fd = open_db(DB_FILE, true);
        if (fd < 0)
        {
            exit_code = EXIT_FAIL_DB;
            break;
        }
        printf(M_DB_ZERO_OK);
        exit_code = EXIT_OK;
        break;
    default:
        usage(argv[0]);
        exit_code = EXIT_FAIL_ARGS;
    }

    close(fd);
    exit(exit_code);
}
