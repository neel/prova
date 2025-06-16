#ifndef LINUX_OPEN_FLAGS_H
#define LINUX_OPEN_FLAGS_H

/*  Linux open(2) flag values for x86-64 / aarch64.
    Define them on Windows so audit-log parsing code compiles.            */

#if !defined(__linux__)   // only inject on Windows or Mac builds
/* Values are octal so they look exactly like the kernel header. */

#ifndef O_RDONLY
# define O_RDONLY        00000000
#endif
#ifndef O_WRONLY
# define O_WRONLY        00000001
#endif
#ifndef O_RDWR
# define O_RDWR          00000002
#endif
#ifndef O_ACCMODE
# define O_ACCMODE       00000003
#endif

#ifndef O_CREAT
# define O_CREAT         00000100
#endif
#ifndef O_EXCL
# define O_EXCL          00000200
#endif
#ifndef O_NOCTTY
# define O_NOCTTY        00000400
#endif
#ifndef O_TRUNC
# define O_TRUNC         00001000
#endif
#ifndef O_APPEND
# define O_APPEND        00002000
#endif
#ifndef O_NONBLOCK
# define O_NONBLOCK      00004000
#endif
/* ---- Aliases -------------------------------------------------------- */
#ifndef O_NDELAY
# define O_NDELAY        O_NONBLOCK        /* historical synonym        */
#endif

#ifndef O_DSYNC
# define O_DSYNC         00010000
#endif
#ifndef O_ASYNC
# define O_ASYNC         00020000          /* a.k.a. FASYNC             */
#endif
#ifndef O_DIRECT
# define O_DIRECT        00040000
#endif
#ifndef O_LARGEFILE
# define O_LARGEFILE     00100000
#endif
#ifndef O_DIRECTORY
# define O_DIRECTORY     00200000
#endif
#ifndef O_NOFOLLOW
# define O_NOFOLLOW      00400000
#endif
#ifndef O_NOATIME
# define O_NOATIME       01000000
#endif
#ifndef O_CLOEXEC
# define O_CLOEXEC       02000000
#endif

/*  Internal sync bit used by the kernel; O_SYNC = __O_SYNC | O_DSYNC     */
#ifndef __O_SYNC
# define __O_SYNC        04000000
#endif
#ifndef O_SYNC
# define O_SYNC          (__O_SYNC | O_DSYNC)   /* 04010000               */
#endif
/* ---- More aliases --------------------------------------------------- */
#ifndef O_FSYNC
# define O_FSYNC         O_SYNC            /* kept for BSD compatibility */
#endif
#ifndef O_RSYNC
# define O_RSYNC         O_SYNC            /* read sync = full sync      */
#endif

#ifndef O_PATH
# define O_PATH          010000000
#endif
#ifndef __O_TMPFILE
# define __O_TMPFILE     020000000
#endif
#ifndef O_TMPFILE
# define O_TMPFILE       (__O_TMPFILE | O_DIRECTORY)
#endif

/* ---- glibc “internal” names (double-underscore) --------------------- */
#ifndef __O_LARGEFILE
# define __O_LARGEFILE   O_LARGEFILE
#endif
#ifndef __O_DIRECTORY
# define __O_DIRECTORY   O_DIRECTORY
#endif
#ifndef __O_NOFOLLOW
# define __O_NOFOLLOW    O_NOFOLLOW
#endif
#ifndef __O_CLOEXEC
# define __O_CLOEXEC     O_CLOEXEC
#endif
#ifndef __O_DIRECT
# define __O_DIRECT      O_DIRECT
#endif
#ifndef __O_NOATIME
# define __O_NOATIME     O_NOATIME
#endif
#ifndef __O_PATH
# define __O_PATH        O_PATH
#endif
#ifndef __O_DSYNC
# define __O_DSYNC       O_DSYNC
#endif
#ifndef __O_RSYNC
# define __O_RSYNC       O_RSYNC
#endif
#ifndef __O_TMPFILE
/* already defined above */
#endif
#endif /* _WIN32 */

#endif // LINUX_OPEN_FLAGS_H
