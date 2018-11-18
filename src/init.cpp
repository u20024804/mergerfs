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

#include <fuse.h>

#include "config.hpp"
#include "ugid.hpp"

namespace mergerfs
{
  namespace fuse
  {
    void *
    init(fuse_conn_info *conn,
         fuse_config    *config)
    {
      const Config *cfg;

      cfg = &Config::get();

      ugid::init();

      conn->want |= FUSE_CAP_ASYNC_DIO;
      conn->want |= FUSE_CAP_ASYNC_READ;
      conn->want |= FUSE_CAP_ATOMIC_O_TRUNC;
      conn->want |= FUSE_CAP_AUTO_INVAL_DATA;
      conn->want |= FUSE_CAP_DONT_MASK;
      conn->want |= FUSE_CAP_IOCTL_DIR;
      conn->want |= FUSE_CAP_PARALLEL_DIROPS;
      conn->want |= FUSE_CAP_READDIRPLUS;
      conn->want |= FUSE_CAP_READDIRPLUS_AUTO;
      conn->want |= FUSE_CAP_SPLICE_MOVE;
      conn->want |= FUSE_CAP_SPLICE_READ;
      conn->want |= FUSE_CAP_SPLICE_WRITE;
      if(cfg->writeback_cache == true)
        conn->want |= FUSE_CAP_WRITEBACK_CACHE;

      conn->max_write     = (1024 * 1024);
      conn->max_read      = 0;
      conn->max_readahead = (1024 * 1024 * 16);

      config->use_ino             = true;
      config->nullpath_ok         = true;
      config->set_uid             = false;
      config->set_gid             = false;
      config->set_mode            = false;
      config->intr                = false;
      config->entry_timeout       = cfg->entry_timeout;
      config->negative_timeout    = cfg->negative_timeout;
      config->attr_timeout        = cfg->attr_timeout;
      config->remember            = cfg->remember;
      config->hard_remove         = cfg->hard_remove;
      config->direct_io           = cfg->direct_io;
      config->kernel_cache        = cfg->kernel_cache;
      config->auto_cache          = cfg->auto_cache;
      config->ac_attr_timeout_set = (config->ac_attr_timeout > 0.0);
      config->ac_attr_timeout     = cfg->ac_attr_timeout;

      return &Config::get_writable();
    }
  }
}
