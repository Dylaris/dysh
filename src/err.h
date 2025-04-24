#ifndef _DYSH_ERR_H_
#define _DYSH_ERR_H_

#define TRUE   1
#define FALSE  0
#define UNKNW -1

#define FILE_MODE_E (1 << 0) /* exist */
#define FILE_MODE_R (1 << 1) /* readble */
#define FILE_MODE_W (1 << 2) /* writeable */
#define FILE_MODE_X (1 << 3) /* executable */

#define FILE_TYPE_R (1 << 0) /* regular file */
#define FILE_TYPE_D (1 << 1) /* directory */

/* Exit if exp is true */
#define EXIT_IF(exp, fmt, ...) do { \
        if (exp) { \
            fprintf(stderr, fmt, ##__VA_ARGS__); \
            exit(EXIT_FAILURE); \
        } \
    } while (0)

#define CHECK_FILE_MODE(filename, mode) \
    ({ \
        int res = FALSE; \
        for (int i = 0; i < 32; i++) { \
            if (!((mode) & (1 << i))) continue; \
            switch (1 << i) { \
            case FILE_MODE_E: \
                res = (access(filename, F_OK) == 0) ? TRUE : FALSE; \
                break; \
            case FILE_MODE_R: \
                res = (access(filename, R_OK) == 0) ? TRUE : FALSE; \
                break; \
            case FILE_MODE_W: \
                res = (access(filename, W_OK) == 0) ? TRUE : FALSE; \
                break; \
            case FILE_MODE_X: \
                res = (access(filename, X_OK) == 0) ? TRUE : FALSE; \
                break; \
            default: \
                fprintf(stderr, "CHECK_FILE_MODE: unknown mode\n"); \
                res = UNKNW; \
                break; \
            } \
            if (!res) break; \
        } \
        res; \
     })

#define CHECK_FILE_TYPE(filename, type) \
    ({ \
        int res = CHECK_FILE_MODE(filename, FILE_MODE_E); \
        if (!res) { \
            fprintf(stderr, "CHECK_FILE_TYPE: file not exist\n"); \
            res = UNKNW; \
        } else { \
            struct stat statbuf; \
            if (stat(filename, &statbuf) == 0) { \
                switch (type) { \
                case FILE_TYPE_R: \
                    res = S_ISREG(statbuf.st_mode) ? TRUE : FALSE; \
                    break; \
                case FILE_TYPE_D: \
                    res = S_ISDIR(statbuf.st_mode) ? TRUE : FALSE; \
                    break; \
                default: \
                    res = UNKNW; \
                    break; \
                } \
            } else { \
                fprintf(stderr, "CHECK_FILE_TYPE: file not exist\n"); \
                res = UNKNW; \
            } \
        } \
        res; \
     })

#define PASS(case_id) \
    do { \
        printf("case %d: ok\n", case_id); \
        fflush(stdout); \
    } while (0)

#define FAIL(case_id) \
    do { \
        printf("case %d: fail\n", case_id); \
        fflush(stdout); \
    } while (0)

#define TEST(exp, case_id) \
    do { \
        if (exp) PASS(case_id); \
        else     FAIL(case_id); \
    } while (0)

#endif /* _DYSH_ERR_H_ */
