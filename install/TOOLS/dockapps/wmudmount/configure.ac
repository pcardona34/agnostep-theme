# Process this file with autoconf to produce a configure script.
AC_INIT([wmudmount], [3.0])
AM_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE()
AM_SILENT_RULES([yes])

DATE=`date '+%B %e, %Y'`
AC_SUBST(DATE)
AC_SUBST(VERSION)

# Checks for programs.
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CC_STDC
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_RANLIB

AX_CHECK_LINKER_FLAGS([-Wl,--as-needed],
 [LDFLAGS="$LDFLAGS -Wl,--as-needed"],
 [:]
)

# Checks for libraries.
AC_CHECK_LIB(m, round)
PKG_CHECK_MODULES(udisks2,udisks2)

AC_MSG_CHECKING([whether libnotify support is desired])
AC_ARG_WITH([libnotify],
 [AS_HELP_STRING([--without-libnotify],[do not compile in support for libnotify])],
 [],[with_libnotify=check])
AC_MSG_RESULT($with_libnotify)
if test "x$with_libnotify" = xyes; then
 PKG_CHECK_MODULES(libnotify,libnotify,[AC_DEFINE([HAVE_LIBNOTIFY],1,[Define if libnotify is available])])
elif test "x$with_libnotify" = xcheck; then
 PKG_CHECK_MODULES(libnotify,libnotify,
  [AC_DEFINE([HAVE_LIBNOTIFY],1,[Define if libnotify is available])],
  [:])
fi

AC_MSG_CHECKING([whether libsecret support is desired])
AC_ARG_WITH([secret],
 [AS_HELP_STRING([--without-secret],[do not compile in support for libsecret])],
 [],[with_secret=yes])
AC_MSG_RESULT($with_secret)
if test "x$with_secret" != xno; then
 PKG_CHECK_MODULES(secret,libsecret-1,
  [AC_DEFINE([HAVE_SECRET],1,[Define if libsecret-1 is available])],
  [AC_MSG_ERROR([Package requirements (libsecret-1) were not met:

$secret_PKG_ERRORS

Consider adjusting the PKG_CONFIG_PATH environment variable if you
installed software in a non-standard prefix.

If you want to compile without libsecret-1, pass --without-libsecret
to configure.])])
fi

AC_MSG_CHECKING([whether gcr (secure memory) support is desired])
AC_ARG_WITH([gcr],
 [AS_HELP_STRING([--without-gcr],[do not compile in support for gcr or secure memory])],
 [],[with_gcr=yes])
AC_MSG_RESULT($with_gcr)
if test "x$with_gcr" != xno; then
 PKG_CHECK_MODULES(gcr,gcr-3,
  [AC_DEFINE([HAVE_GCR],1,[Define if gcr-3 is available])],
  [AC_MSG_ERROR([Package requirements (gcr-3) were not met:

$gcr_PKG_ERRORS

Consider adjusting the PKG_CONFIG_PATH environment variable if you
installed software in a non-standard prefix.

If you want to compile without gcr-3 (meaning that
secure memory WILL NOT BE AVAILABLE), pass --without-gcr
to configure.])])
fi

# Checks for header files.
AC_PATH_XTRA
AM_PATH_GTK_3_0(3.16.0,,[AC_MSG_ERROR(cannot find libgtk3)])
PKG_CHECK_MODULES(gdk_pixbuf,gdk-pixbuf-2.0)
AC_HEADER_STDC
if test "$ac_cv_header_stdc" != "yes"; then AC_MSG_WARN(standard C headers not found); fi
AC_CHECK_HEADERS([string.h sys/statvfs.h],, AC_MSG_WARN($ac_header not found))
AC_HEADER_TIME

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
if test "$ac_cv_func_malloc_0_nonnull" != "yes"; then AC_MSG_WARN([malloc() doesn't seem to work]); fi #'
AC_REPLACE_FUNCS([pipe2])
AC_CHECK_FUNCS([memset strtol],, AC_MSG_WARN($ac_func not found))
AC_MSG_CHECKING([for prctl and PR_SET_DUMPABLE])
AC_COMPILE_IFELSE([AC_LANG_SOURCE([
#include <sys/prctl.h>
int main(void){
    prctl(PR_SET_DUMPABLE, 0);
    return 0;
}
])],[
   AC_DEFINE(HAVE_PRCTL,1,[Define if the Linux prctl is available and PR_SET_DUMPABLE is defined])
   AC_MSG_RESULT([yes])
],[AC_MSG_RESULT([no])])

AC_CHECK_FUNCS(statvfs,, [FUNC_STATFS(,[AC_MSG_ERROR([Neither statfs nor vstatfs exist])])])

AC_CHECK_HEADERS(mntent.h,[
 AC_MSG_CHECKING([for struct mntent and getmntent in mntent.h])
 AC_COMPILE_IFELSE([AC_LANG_SOURCE([
#include <stdio.h>
#include <mntent.h>
int main(){
        struct mntent *e;
        FILE *fp;
        fp=setmntent("/etc/fstab", "r");
        e = getmntent(fp);
        endmntent(fp);
        return 0;
}])],
  [AC_DEFINE([HAVE_MNTENT_FUNCS],[1],[Define if mntent.h is available and defines struct mntent, setmntent, getmntent, and endmntent])
   AC_MSG_RESULT(yes)],
  [AC_MSG_RESULT(no)])
])

AC_CHECK_HEADERS(fstab.h,[
 AC_MSG_CHECKING([for struct fstab and getfsent in fstab.h])
 AC_COMPILE_IFELSE([AC_LANG_SOURCE([
#include <fstab.h>
int main(){
        struct fstab *e;
        setfsent();
        e = getfsent();
        endfsent();
        return 0;
}])],
  [AC_DEFINE([HAVE_FSTAB_FUNCS],[1],[Define if fstab.h is available and defines struct fstab, setfsent, getfsent, and endfsent])
   AC_MSG_RESULT(yes)],
  [AC_MSG_RESULT(no)])
])

X_CFLAGS="$X_CFLAGS \$(GTK_CFLAGS) \$(gdk_pixbuf_CFLAGS) \$(udisks2_CFLAGS) \$(secret_CFLAGS) \$(gcr_CFLAGS) \$(libnotify_CFLAGS)"
X_LIBS="\$(X_PRE_LIBS) $X_LIBS -lX11 \$(gdk_pixbuf_LIBS) \$(X_EXTRA_LIBS) \$(GTK_LIBS) \$(udisks2_LIBS) \$(secret_LIBS) \$(gcr_LIBS) \$(libnotify_LIBS)"

AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([b0rken/Makefile])
AC_CONFIG_FILES([m4/Makefile])
AC_CONFIG_FILES([icons/Makefile])
AC_CONFIG_FILES([icons/scalable/Makefile])
AC_CONFIG_FILES([icons/16x16/Makefile])
AC_CONFIG_FILES([icons/22x22/Makefile])
AC_CONFIG_FILES([icons/48x48/Makefile])
AC_OUTPUT
