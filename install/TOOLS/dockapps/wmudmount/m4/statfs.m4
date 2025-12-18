# FUNC_STATFS
AC_DEFUN([FUNC_STATFS],
[AC_CHECK_HEADERS(sys/param.h sys/vfs.h sys/mount.h sys/statfs.h)
 AC_CHECK_FUNCS(statfs,[
  AC_MSG_CHECKING([number of arguments to statfs])
  AC_TRY_COMPILE([
#include <sys/unistd.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_SYS_VFS_H
#include <sys/vfs.h>
#endif
#ifdef HAVE_SYS_MOUNT_H
#include <sys/mount.h>
#endif
#ifdef HAVE_SYS_STATFS_H
#include <sys/statfs.h>
#endif
],[struct statfs s; statfs("/", &s);],
   [AC_MSG_RESULT(2)
    AC_DEFINE(STATFS_ARGUMENTS,2,[Number of arguments to statfs()])
    $1
   ],
   [AC_TRY_COMPILE([
#include <sys/unistd.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_SYS_VFS_H
#include <sys/vfs.h>
#endif
#ifdef HAVE_SYS_MOUNT_H
#include <sys/mount.h>
#endif
#ifdef HAVE_SYS_STATFS_H
#include <sys/statfs.h>
#endif
],[struct statfs s; statfs("/", &s, sizeof(struct statfs), 0);],
    [AC_MSG_RESULT(4)
     AC_DEFINE(STATFS_ARGUMENTS,4,[Number of arguments to statfs()])
     $1
    ],
    [AC_MSG_RESULT(Unknown)
     AC_MSG_ERROR([Unable to determine number of arguments to statfs])]
   )]
  )
 ],[$2]
)])
