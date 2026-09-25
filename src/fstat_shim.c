#define _GNU_SOURCE
#include <sys/stat.h>
#include <sys/types.h>

/*
 * glibc 2.31 auf ARM32 (EABI):
 *   - __fxstat, __fxstat64 sind in der dynamischen libc.so.6 vorhanden.
 *   - fstat, fstat64 sind NUR als hidden symbols in libc_nonshared.a
 *     vorhanden, NICHT in der dynamischen libc.so.6.
 *
 * Android-Bionic-Bibliotheken (libapp.so) erwarten fstat als
 * dynamisches Symbol. Der Shim schliesst diese Luecke.
 *
 * _STAT_VER ist auf ARM32 gleich 3 (siehe glibc/sysdeps/unix/sysv/linux/arm/).
 */

extern int __fxstat(int ver, int fd, struct stat *buf);
extern int __fxstat64(int ver, int fd, struct stat64 *buf);

int fstat(int fd, struct stat *buf) {
    return __fxstat(3, fd, buf);
}

int fstat64(int fd, struct stat64 *buf) {
    return __fxstat64(3, fd, buf);
}
