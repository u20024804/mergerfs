#include "ctrlfs.hpp"

int
CtrlFSObject::from_symlink(const std::string &value_)
{
  return 0;
}

int
CtrlFSObject::to_symlink(std::string &value_)
{
  return 0;
}

void
CtrlFS::add(const std::string &key_,
            CtrlFSObject      *val_)
{
  _map[key_] = val_;
}

int
CtrlFS::get(const std::string &key_,
            std::string       &val_)
{
  return _map[key_]->to_symlink(val_);
}

int
CtrlFS::set(const std::string &key_,
            std::string       &val_)
{
  return _map[key_]->from_symlink(val_);
}
