// terminal.c
// Shared-memory terminal bridge:
// - Read from shared VM->Host ring buffer and print to stdout
// - Read from stdin and write to shared Host->VM ring buffer

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>

// Shared memory layout must match simulator
#define SHM_NAME "/asoc_shm"
#define IO_BUF_SIZE 4096
struct shared_io {
    volatile unsigned int vth_head, vth_tail; // vm -> host (GPU output)
    volatile unsigned int htv_head, htv_tail; // host -> vm (keyboard input)
    char vth_buf[IO_BUF_SIZE];
    char htv_buf[IO_BUF_SIZE];
};

static struct shared_io *g_shm = NULL;
static struct termios oldt;

static void restore_terminal(void) {
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
}

static void handle_signal(int sig) {
    (void)sig;
    restore_terminal();
    _exit(0);
}

int main(void) {
    struct termios newt;
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    signal(SIGQUIT, handle_signal);

    // Open shared memory
    int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        perror("shm_open");
        return 1;
    }
    if (ftruncate(fd, sizeof(struct shared_io)) == -1) {
        // It's ok if the simulator already sized it; ignore error
    }
    g_shm = (struct shared_io*)mmap(NULL, sizeof(struct shared_io), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (g_shm == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }
    close(fd);

    // Set terminal to raw, non-blocking stdin
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    atexit(restore_terminal);
    int fl = fcntl(STDIN_FILENO, F_GETFL, 0);
    if (fl != -1) fcntl(STDIN_FILENO, F_SETFL, fl | O_NONBLOCK);

    char buf;
    while (1) {
        // Drain VM->Host buffer
        if (g_shm) {
            while (g_shm->vth_head != g_shm->vth_tail) {
                unsigned int tail = g_shm->vth_tail;
                char c = g_shm->vth_buf[tail];
                g_shm->vth_tail = (tail + 1) % IO_BUF_SIZE;
                if (write(STDOUT_FILENO, &c, 1) == -1) {
                    perror("write");
                    break;
                }
            }
        }

        // Read from stdin
        ssize_t n = read(STDIN_FILENO, &buf, 1);
        if (n > 0 && g_shm) {
            unsigned int head = g_shm->htv_head;
            unsigned int next = (head + 1) % IO_BUF_SIZE;
            if (next != g_shm->htv_tail) {
                g_shm->htv_buf[head] = buf;
                g_shm->htv_head = next;
            }
        }

        usleep(10000);
    }

    return 0;
}