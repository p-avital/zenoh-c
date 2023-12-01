#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
#define VALID_PLATFORM 0
#else  // def windows
#define VALID_PLATFORM 1

#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#undef NDEBUG
#include <assert.h>

typedef int (*func_ptr_t)();

int run_timeouted_test(func_ptr_t functions[], int num_functions, int timeout_seconds) {
    pid_t child_pids[num_functions];
    int status;
    int result = 0;

    // Fork a child process for each function
    for (int i = 0; i < num_functions; ++i) {
        child_pids[i] = fork();
        if (child_pids[i] == 0) {
            // Child process
            exit(functions[i]());
        } else if (child_pids[i] < 0) {
            // Error forking
            perror("Error forking");
            exit(EXIT_FAILURE);
        }
    }

    // Wait for all processes to finish or timeout
    int remaining_time = timeout_seconds * 10;
    int remaining_children = num_functions;

    while (remaining_time > 0 && remaining_children > 0 && result == 0) {
        pid_t finished_pid = waitpid(-1, &status, WNOHANG);
        if (finished_pid > 0) {
            for (int i = 0; i < num_functions; ++i) {
                if (finished_pid == child_pids[i]) {
                    child_pids[i] = 0;
                    --remaining_children;
                    if ((WIFEXITED(status) && WEXITSTATUS(status) != 0)) {
                        result = -1;
                        break;
                    }
                }
            }
        } else if (finished_pid == 0) {
            // No process finished yet
            usleep(100000);  // 0.1 sec
            remaining_time--;
        } else {
            // Error in waitpid
            perror("Error waiting for child process");
            break;
        }
    }

    if (remaining_time <= 0) {
        fprintf(stderr, "Timeout reached\n");
    }

    // Check if the processes are still running and kill them if necessary
    if (remaining_children != 0) {
        for (int i = 0; i < num_functions; ++i) {
            if (child_pids[i]) {
                kill(child_pids[i], SIGKILL);
                waitpid(child_pids[i], &status, 0);
            }
        }
        result = -1;
    }

    return result;
};

#define SEM_INIT(sem, name)                              \
    do {                                                 \
        sem_unlink(name);                                \
        sem = sem_open(name, O_CREAT | O_EXCL, 0666, 0); \
        if (sem == SEM_FAILED) {                         \
            perror("sem_open");                          \
            exit(-1);                                    \
        }                                                \
    } while (0)

#define SEM_DROP(sem, name)      \
    do {                         \
        int r;                   \
        r = sem_close(sem);      \
        if (r) {                 \
            perror("sem_close"); \
            exit(-1);            \
        };                       \
        r = sem_unlink(name);    \
        if (r) {                 \
            perror("sem_close"); \
            exit(-1);            \
        }                        \
    } while (0)

#define SEM_WAIT(sem)                        \
    do {                                     \
        int r;                               \
        do {                                 \
            r = sem_wait(sem);               \
            if (r == -1 && errno != EINTR) { \
                perror("sem_wait");          \
                exit(-1);                    \
            }                                \
        } while (r != 0);                    \
    } while (0)

#define SEM_POST(sem)           \
    do {                        \
        int r;                  \
        r = sem_post(sem);      \
        if (r) {                \
            perror("sem_post"); \
            exit(-1);           \
        };                      \
    } while (0)

#endif  // def windows
