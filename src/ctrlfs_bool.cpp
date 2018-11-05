#include "ctrlfs_bool.hpp"
#include "errno.hpp"

CtrlFS_bool::CtrlFS_bool(bool *bool_)
{
  _bool = bool_;
}

int
CtrlFS_bool::from_symlink(const std::string &value_)
{
  if(value_ == "true")
    *_bool = true;
  else if(value_ == "false")
    *_bool = false;
  else
    return -EINVAL;

  return 0;
}

int
CtrlFS_bool::to_symlink(std::string &value_)
{
  if(*_bool == true)
    value_ = "true";
  else
    value_ = "false";

  return 0;
}
