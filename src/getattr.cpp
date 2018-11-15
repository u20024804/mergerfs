/*
  Copyright (c) 2016, Antonio SJ Musumeci <trapexit@spawn.link>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include "config.hpp"
#include "errno.hpp"
#include "fileinfo.hpp"
#include "fs_base_stat.hpp"
#include "fs_inode.hpp"
#include "fs_path.hpp"
#include "rwlock.hpp"
#include "symlinkify.hpp"
#include "ugid.hpp"

#include <fuse.h>

#include <string>
#include <vector>

using std::string;
using std::vector;
using mergerfs::Policy;
using mergerfs::Config;
using mergerfs::rwlock::ReadGuard;

namespace local
{
  static
  int
  getattr_controlfile(struct stat *st_)
  {
    static const uid_t  uid = ::getuid();
    static const gid_t  gid = ::getgid();
    static const time_t now = ::time(NULL);

    st_->st_dev     = 0;
    st_->st_ino     = fs::inode::MAGIC;
    st_->st_mode    = (S_IFREG|S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
    st_->st_nlink   = 1;
    st_->st_uid     = uid;
    st_->st_gid     = gid;
    st_->st_rdev    = 0;
    st_->st_size    = 0;
    st_->st_blksize = 512;
    st_->st_blocks  = 0;
    st_->st_atime   = now;
    st_->st_mtime   = now;
    st_->st_ctime   = now;

    return 0;
  }

  static
  int
  getattr(Policy::Func::Search  searchFunc,
          const Branches       &branches_,
          const uint64_t        minfreespace,
          const char           *fusepath,
          struct stat          *st,
          const bool            symlinkify,
          const time_t          symlinkify_timeout)
  {
    int rv;
    string fullpath;
    vector<const string*> basepaths;

    rv = searchFunc(branches_,fusepath,minfreespace,basepaths);
    if(rv == -1)
      return -errno;

    fs::path::make(basepaths[0],fusepath,fullpath);

    rv = fs::lstat(fullpath,*st);
    if(rv == -1)
      return -errno;

    if(symlinkify && symlinkify::can_be_symlink(*st,symlinkify_timeout))
      st->st_mode = symlinkify::convert(st->st_mode);

    fs::inode::recompute(*st);

    return 0;
  }

  static
  int
  getattr(const uid_t   uid_,
          const gid_t   gid_,
          const Config *config_,
          const char   *fusepath_,
          struct stat  *st_)
  {
    const ugid::Set ugid(uid_,gid_);
    const ReadGuard readlock(&config_->branches_lock);

    return local::getattr(config_->getattr,
                          config_->branches,
                          config_->minfreespace,
                          fusepath_,
                          st_,
                          config_->symlinkify,
                          config_->symlinkify_timeout);
  }

  static
  int
  fgetattr(const fuse_file_info *ffi_,
           struct stat          *st_)
  {
    int rv;
    FileInfo *fi;

    fi = reinterpret_cast<FileInfo*>(ffi_->fh);

    rv = fs::fstat(fi->fd,*st_);
    if(rv == -1)
      return -errno;

    fs::inode::recompute(*st_);

    return 0;
  }

  static
  int
  getattr(const char  *fusepath_,
          struct stat *st_)
  {
    const fuse_context *fc     = fuse_get_context();
    const Config       &config = Config::get(fc);

    if(fusepath_ == config.controlfile)
      return local::getattr_controlfile(st_);

    return local::getattr(fc->uid,
                          fc->gid,
                          &config,
                          fusepath_,
                          st_);
  }
}

namespace mergerfs
{
  namespace fuse
  {
    int
    getattr(const char     *fusepath,
            struct stat    *st,
            fuse_file_info *ffi)
    {
      if((fusepath == NULL) && (ffi != NULL))
        return local::fgetattr(ffi,st);

      return local::getattr(fusepath,st);
    }
  }
}
