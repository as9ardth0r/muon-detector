/**
 * syscalls.c — implémentations minimales des appels système que newlib
 * réclame au link dès qu'une fonction stdio (ici sscanf dans nmea.c)
 * est utilisée, même sans OS derrière. Boilerplate standard pour tout
 * projet bare-metal ARM+newlib — pas de comportement réel derrière
 * (pas de vrai système de fichiers sur cette cible).
 */
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>

extern char _ebss; /* défini par le linker script : fin de .bss, début du tas */
static char *heap_end = &_ebss;

void _exit(int status) {
    (void)status;
    while (1) { }
}

int _close(int file) { (void)file; return -1; }
int _fstat(int file, struct stat *st) { (void)file; st->st_mode = S_IFCHR; return 0; }
int _isatty(int file) { (void)file; return 1; }
int _lseek(int file, int ptr, int dir) { (void)file; (void)ptr; (void)dir; return 0; }
int _read(int file, char *ptr, int len) { (void)file; (void)ptr; (void)len; return 0; }
int _write(int file, const char *ptr, int len) { (void)file; (void)ptr; return len; }
int _kill(int pid, int sig) { (void)pid; (void)sig; errno = EINVAL; return -1; }
int _getpid(void) { return 1; }

void *_sbrk(int incr) {
    extern char _estack; /* sommet de la RAM, défini par le linker script */
    char *prev_heap_end = heap_end;
    if (heap_end + incr > &_estack) {
        errno = ENOMEM;
        return (void *)-1;
    }
    heap_end += incr;
    return prev_heap_end;
}
