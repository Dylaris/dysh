#ifndef _DYSH_ERR_H_
#define _DYSH_ERR_H_

/* If exp is false, then print some message */
#define ERR_PRINT_IFF(exp, fmt, ...) \
    do { \
        if (!(exp)) { \
            fprintf(stdout, fmt "\n", ##__VA_ARGS__); \
        } \
    } while (0)

/* If exp is false, then print some message and exit */
#define ERR_EXIT_IFF(exp, fmt, ...) \
    do { \
        if (!(exp)) { \
            fprintf(stderr, fmt " in %s at line %d\n", ##__VA_ARGS__, __FILE__, __LINE__); \
            exit(1); \
        } \
    } while (0)

#endif /* _DYSH_ERR_H_ */