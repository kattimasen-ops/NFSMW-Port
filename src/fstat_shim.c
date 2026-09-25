#define _GNU_SOURCE
#include <sys/stat.h>
#include <sys/types.h>

/*
 * glibc 2.31 auf ARM32 exportiert fstat nur als weak alias fuer __fxstat,
 * aber nicht als eigenstaendiges dynamisches Symbol. Der eapx-Relokations-
 * loader sucht jedoch genau fstat und verweigert sonst den Start.
 *
 * _STAT_VER ist auf ARM32 gleich 3.
 */
extern int __fxstat(int ver, int fd, struct stat *buf);
extern int __fxstat64(int ver, int fd, struct stat64 *buf);

int fstat(int fd, struct stat *buf) {
    return __fxstat(3, fd, buf);
}

int fstat64(int fd, struct stat64 *buf) {
    return __fxstat64(3, fd, buf);
}

/* Fallback fuer den unwahrscheinlichen Fall, dass lstat/stat ebenfalls
   fehlen. Bewusst nur aktiviert, wenn der Loader sie meldet. */
int lstat(const char *path, struct stat *buf) {
    return __lxstat(3, path, buf);
}

int stat(const char *path, struct stat *buf) {
    return __xstat(3, path, buf);
}
