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
#include "fs_glob.hpp"
#include "fs_statvfs_cache.hpp"
#include "num.hpp"
#include "policy.hpp"
#include "str.hpp"
#include "version.hpp"

#include <fuse.h>

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

using std::string;
using std::vector;

enum
  {
    MERGERFS_OPT_HELP,
    MERGERFS_OPT_VERSION
  };

static
void
set_option(fuse_args         *args,
           const std::string &option_)
{

  fuse_opt_add_arg(args,"-o");
  fuse_opt_add_arg(args,option_.c_str());
}

static
void
set_kv_option(fuse_args         *args,
              const std::string &key,
              const std::string &value)
{
  std::string option;

  option = key + '=' + value;

  set_option(args,option);
}

static
void
set_fsname(fuse_args      *args,
           const Branches &branches_)
{
  vector<string> branches;

  branches_.to_paths(branches);

  if(branches.size() > 0)
    {
      std::string fsname;

      fsname = str::remove_common_prefix_and_join(branches,':');

      set_kv_option(args,"fsname",fsname);
    }
}

static
void
set_subtype(fuse_args *args)
{
  set_kv_option(args,"subtype","mergerfs");
}

static
void
set_default_options(fuse_args *args)
{
  set_option(args,"atomic_o_trunc");
  set_option(args,"big_writes");
  set_option(args,"default_permissions");
}

static
int
parse_and_process(const std::string &value_,
                  uint64_t          &int_)
{
  int rv;

  rv = num::to_uint64_t(value_,int_);
  if(rv == -1)
    return 1;

  return 0;
}

static
int
parse_and_process(const std::string &value,
                  time_t            &time)
{
  int rv;

  rv = num::to_time_t(value,time);
  if(rv == -1)
    return 1;

  return 0;
}

static
int
parse_and_process(const std::string &value,
                  bool              &boolean)
{
  if(value == "false")
    boolean = false;
  else if(value == "true")
    boolean = true;
  else
    return 1;

  return 0;
}

static
int
parse_and_process_errno(const std::string &value_,
                        int               &errno_)
{
  if(value_ == "passthrough")
    errno_ = 0;
  else if(value_ == "nosys")
    errno_ = ENOSYS;
  else if(value_ == "noattr")
    errno_ = ENOATTR;
  else
    return 1;

  return 0;
}

static
int
parse_and_process_statfs(const std::string    &value_,
                         Config::StatFS::Enum &enum_)
{
  if(value_ == "base")
    enum_ = Config::StatFS::BASE;
  else if(value_ == "full")
    enum_ = Config::StatFS::FULL;
  else
    return 1;

  return 0;
}

static
int
parse_and_process_statfsignore(const std::string          &value_,
                               Config::StatFSIgnore::Enum &enum_)
{
  if(value_ == "none")
    enum_ = Config::StatFSIgnore::NONE;
  else if(value_ == "ro")
    enum_ = Config::StatFSIgnore::RO;
  else if(value_ == "nc")
    enum_ = Config::StatFSIgnore::NC;
  else
    return 1;

  return 0;
}

static
int
parse_and_process_statfs_cache(const std::string &value_)
{
  int rv;
  uint64_t timeout;

  rv = num::to_uint64_t(value_,timeout);
  if(rv == -1)
    return 1;

  fs::statvfs_cache_timeout(timeout);

  return 0;
}

static
int
parse_and_process_cache(Config       &config_,
                        const string &func_,
                        const string &value_,
                        fuse_args    *outargs)
{
  if(func_ == "open")
    return parse_and_process(value_,config_.open_cache.timeout);
  else if(func_ == "statfs")
    return parse_and_process_statfs_cache(value_);
  else if(func_ == "entry")
    return (set_kv_option(outargs,"entry_timeout",value_),0);
  else if(func_ == "negative_entry")
    return (set_kv_option(outargs,"negative_timeout",value_),0);
  else if(func_ == "attr")
    return (set_kv_option(outargs,"attr_timeout",value_),0);

  return 1;
}

static
int
parse_and_process_arg(Config            &config,
                      const std::string &arg)
{
  if(arg == "defaults")
    return 0;
  else if(arg == "direct_io")
    return (config.direct_io=true,0);

  return 1;
}

static
int
parse_and_process_kv_arg(Config            &config,
                         const std::string &key,
                         const std::string &value,
                         fuse_args         *outargs)
{
  int rv;
  std::vector<std::string> keypart;

  rv = -1;
  str::split(keypart,key,'.');
  if(keypart.size() == 2)
    {
      if(keypart[0] == "func")
        rv = config.set_func_policy(keypart[1],value);
      else if(keypart[0] == "category")
        rv = config.set_category_policy(keypart[1],value);
      else if(keypart[0] == "cache")
        rv = parse_and_process_cache(config,keypart[1],value,outargs);
    }
  else
    {
      if(key == "minfreespace")
        rv = parse_and_process(value,config.minfreespace);
      else if(key == "moveonenospc")
        rv = parse_and_process(value,config.moveonenospc);
      else if(key == "dropcacheonclose")
        rv = parse_and_process(value,config.dropcacheonclose);
      else if(key == "symlinkify")
        rv = parse_and_process(value,config.symlinkify);
      else if(key == "symlinkify_timeout")
        rv = parse_and_process(value,config.symlinkify_timeout);
      else if(key == "nullrw")
        rv = parse_and_process(value,config.nullrw);
      else if(key == "ignorepponrename")
        rv = parse_and_process(value,config.ignorepponrename);
      else if(key == "security_capability")
        rv = parse_and_process(value,config.security_capability);
      else if(key == "link_cow")
        rv = parse_and_process(value,config.link_cow);
      else if(key == "xattr")
        rv = parse_and_process_errno(value,config.xattr);
      else if(key == "statfs")
        rv = parse_and_process_statfs(value,config.statfs);
      else if(key == "statfs_ignore")
        rv = parse_and_process_statfsignore(value,config.statfs_ignore);
    }

  if(rv == -1)
    rv = 1;

  return rv;
}

static
int
process_opt(Config            &config,
            const std::string &arg,
            fuse_args         *outargs)
{
  int rv;
  std::vector<std::string> argvalue;

  str::split(argvalue,arg,'=');
  switch(argvalue.size())
    {
    case 1:
      rv = parse_and_process_arg(config,argvalue[0]);
      break;

    case 2:
      rv = parse_and_process_kv_arg(config,argvalue[0],argvalue[1],outargs);
      break;

    default:
      rv = 1;
      break;
    };

  return rv;
}

static
int
process_branches(const char *arg,
                 Config     &config)
{
  config.branches.set(arg);

  return 0;
}

static
int
process_destmounts(const char *arg,
                   Config     &config)
{
  config.destmount = arg;

  return 1;
}

static
void
usage(void)
{
  std::cout <<
    "Usage: mergerfs [options] <srcpaths> <destpath>\n"
    "\n"
    "    -o [opt,...]           mount options\n"
    "    -h --help              print help\n"
    "    -v --version           print version\n"
    "\n"
    "mergerfs options:\n"
    "    <srcpaths>             ':' delimited list of directories. Supports\n"
    "                           shell globbing (must be escaped in shell)\n"
    "    -o func.<f>=<p>        Set function <f> to policy <p>\n"
    "    -o category.<c>=<p>    Set functions in category <c> to <p>\n"
    "    -o cache.open=<int>    'open' policy cache timeout in seconds.\n"
    "                           default = 0 (disabled)\n"
    "    -o cache.statfs=<int>  'statfs' cache timeout in seconds. Used by\n"
    "                           policies. default = 0 (disabled)\n"
    "    -o cache.attr=<int>    file attribute cache timeout in seconds.\n"
    "                           default = 1\n"
    "    -o cache.entry=<int>   file name lookup cache timeout in seconds.\n"
    "                           default = 1\n"
    "    -o cache.negative_entry=<int>\n"
    "                           negative file name lookup cache timeout in\n"
    "                           seconds. default = 0\n"
    "    -o direct_io           Bypass page caching, may increase write\n"
    "                           speeds at the cost of reads. Please read docs\n"
    "                           for more details as there are tradeoffs.\n"
    "    -o use_ino             Have mergerfs generate inode values rather than\n"
    "                           autogenerated by libfuse. Suggested.\n"
    "    -o minfreespace=<int>  minimum free space needed for certain policies.\n"
    "                           default = 4G\n"
    "    -o moveonenospc=<bool> Try to move file to another drive when ENOSPC\n"
    "                           on write. default = false\n"
    "    -o dropcacheonclose=<bool>\n"
    "                           When a file is closed suggest to OS it drop\n"
    "                           the file's cache. This is useful when direct_io\n"
    "                           is disabled. default = false\n"
    "    -o symlinkify=<bool>   Read-only files, after a timeout, will be turned\n"
    "                           into symlinks. Read docs for limitations and\n"
    "                           possible issues. default = false\n"
    "    -o symlinkify_timeout=<int>\n"
    "                           timeout in seconds before will turn to symlinks.\n"
    "                           default = 3600\n"
    "    -o nullrw=<bool>       Disables reads and writes. For benchmarking.\n"
    "                           default = false\n"
    "    -o ignorepponrename=<bool>\n"
    "                           Ignore path preserving when performing renames\n"
    "                           and links. default = false\n"
    "    -o link_cow=<bool>     delink/clone file on open to simulate CoW.\n"
    "                           default = false\n"
    "    -o security_capability=<bool>\n"
    "                           When disabled return ENOATTR when the xattr\n"
    "                           security.capability is queried. default = true\n"
    "    -o xattr=passthrough|noattr|nosys\n"
    "                           Runtime control of xattrs. By default xattr\n"
    "                           requests will pass through to the underlying\n"
    "                           filesystems. notattr will short circuit as if\n"
    "                           nothing exists. nosys will respond as if not\n"
    "                           supported or disabled. default = passthrough\n"
    "    -o statfs=base|full    When set to 'base' statfs will use all branches\n"
    "                           when performing statfs calculations. 'full' will\n"
    "                           only include branches on which that path is\n"
    "                           available. default = base\n"
    "    -o statfs_ignore=none|ro|nc\n"
    "                           'ro' will cause statfs calculations to ignore\n"
    "                           available space for branches mounted or tagged\n"
    "                           as 'read only' or 'no create'. 'nc' will ignore\n"
    "                           available space for branches tagged as\n"
    "                           'no create'. default = none\n"
            << std::endl;
}

static
int
option_processor(void       *data,
                 const char *arg,
                 int         key,
                 fuse_args  *outargs)
{
  int     rv     = 0;
  Config &config = *(Config*)data;

  switch(key)
    {
    case FUSE_OPT_KEY_OPT:
      rv = process_opt(config,arg,outargs);
      break;

    case FUSE_OPT_KEY_NONOPT:
      rv = config.branches.empty() ?
        process_branches(arg,config) :
        process_destmounts(arg,config);
      break;

    case MERGERFS_OPT_HELP:
      usage();
      close(2);
      dup(1);
      fuse_opt_add_arg(outargs,"-ho");
      break;

    case MERGERFS_OPT_VERSION:
      std::cout << "mergerfs version: "
                << (MERGERFS_VERSION[0] ? MERGERFS_VERSION : "unknown")
                << std::endl;
      fuse_opt_add_arg(outargs,"--version");
      break;

    default:
      break;
    }

  return rv;
}

namespace options
{
  void
  parse(fuse_args *args,
        Config    *config)
  {
    const struct fuse_opt opts[] =
      {
        FUSE_OPT_KEY("-h",MERGERFS_OPT_HELP),
        FUSE_OPT_KEY("--help",MERGERFS_OPT_HELP),
        FUSE_OPT_KEY("-v",MERGERFS_OPT_VERSION),
        FUSE_OPT_KEY("-V",MERGERFS_OPT_VERSION),
        FUSE_OPT_KEY("--version",MERGERFS_OPT_VERSION),
        {NULL,-1U,0}
      };

    fuse_opt_parse(args,
                   config,
                   opts,
                   ::option_processor);

    set_default_options(args);
    set_fsname(args,config->branches);
    set_subtype(args);
  }
}
