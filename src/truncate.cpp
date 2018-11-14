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
#include "fs_base_truncate.hpp"
#include "fs_base_ftruncate.hpp"
#include "fs_path.hpp"
#include "rv.hpp"
#include "rwlock.hpp"
#include "ugid.hpp"

#include <fuse.h>

#include <string>
#include <vector>

#include <sys/types.h>
#include <unistd.h>

using std::string;
using std::vector;
using mergerfs::Policy;
using mergerfs::Config;
using mergerfs::rwlock::ReadGuard;

static
int
truncate_loop_core(const string *basepath_,
                   const char   *fusepath_,
                   const off_t   size_,
                   const int     error_)
{
  int rv;
  string fullpath;

  fullpath = fs::path::make(basepath_,fusepath_);

  rv = fs::truncate(fullpath,size_);

  return error::calc(rv,error_,errno);
}

static
int
truncate_loop(const vector<const string*> &basepaths,
              const char                  *fusepath,
              const off_t                  size)
{
  int error;

  error = -1;
  for(size_t i = 0, ei = basepaths.size(); i != ei; i++)
    {
      error = truncate_loop_core(basepaths[i],fusepath,size,error);
    }

  return -error;
}

static
int
truncate(Policy::Func::Action  actionFunc,
         const Branches       &branches_,
         const uint64_t        minfreespace,
         const char           *fusepath,
         const off_t           size)
{
  int rv;
  vector<const string*> basepaths;

  rv = actionFunc(branches_,fusepath,minfreespace,basepaths);
  if(rv == -1)
    return -errno;

  return truncate_loop(basepaths,fusepath,size);
}

static
int
truncate(const uid_t   uid_,
         const gid_t   gid_,
         const Config *config_,
         const char   *fusepath_,
         const off_t   size_)
{
  const ugid::Set ugid(uid_,gid_);
  const ReadGuard readlock(&config_->branches_lock);

  return truncate(config_->truncate,
                  config_->branches,
                  config_->minfreespace,
                  fusepath_,
                  size_);
}

static
int
ftruncate(const fuse_file_info *ffi_,
          const off_t           size_)
{
  int rv;
  FileInfo *fi;

  fi = reinterpret_cast<FileInfo*>(ffi_->fh);

  rv = fs::ftruncate(fi->fd,size_);

  return ((rv == -1) ? -errno : 0);
}

namespace mergerfs
{
  namespace fuse
  {
    int
    truncate(const char     *fusepath,
             off_t           size,
             fuse_file_info *ffi)
    {
      const fuse_context *fc     = fuse_get_context();
      const Config       &config = Config::get(fc);

      if((fusepath == NULL) && (ffi != NULL))
        return ::ftruncate(ffi,size);

      return ::truncate(fc->uid,
                        fc->gid,
                        &config,
                        fusepath,
                        size);
    }
  }
}
