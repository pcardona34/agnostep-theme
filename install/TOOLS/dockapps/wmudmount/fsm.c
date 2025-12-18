/* wmudmount
 * Copyright © 2010-2014  Brad Jorsch <anomie@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <sys/unistd.h>

#include "die.h"
#include "fsm.h"

#if defined(HAVE_STATVFS)

# ifdef HAVE_SYS_STATVFS_H
#  include <sys/statvfs.h>
# endif

int get_fs_usage(const char *path, guint64 *blocks, guint64 *bfree){
    struct statvfs s;
    if(statvfs(path, &s)<0){
        warn(DEBUG_WARN, "Could not call statvfs on %s: %s", path, strerror(errno));
        *blocks = 0;
        *bfree = 0;
        return 0;
    }
    *blocks = (guint64)s.f_blocks * s.f_bsize;
    *bfree = (guint64)s.f_bfree * s.f_bsize;
    return 1;
}

#elif defined(HAVE_STATFS) && (STATFS_ARGUMENTS == 2 || STATFS_ARGUMENTS == 4)

# ifdef HAVE_SYS_PARAM_H
#  include <sys/param.h>
# endif
# ifdef HAVE_SYS_VFS_H
#  include <sys/vfs.h>
# endif
# ifdef HAVE_SYS_MOUNT_H
#  include <sys/mount.h>
# endif
# ifdef HAVE_SYS_STATFS_H
#  include <sys/statfs.h>
# endif

int get_fs_usage(const char *path, guint64 *blocks, guint64 *bfree){
    struct statfs s;
#if STATFS_ARGUMENTS == 2
    if(statfs(path, &s)<0)
#else
    if(statfs(path, &s, sizeof(struct statfs), 0)<0)
#endif
    {
        warn(DEBUG_WARN, "Could not call statfs on %s: %s", path, strerror(errno));
        *blocks = 0;
        *bfree = 0;
        return 0;
    }
    *blocks = (guint64)s.f_blocks * s.f_bsize;
    *bfree = (guint64)s.f_bfree * s.f_bsize;
    return 1;
}

#endif
