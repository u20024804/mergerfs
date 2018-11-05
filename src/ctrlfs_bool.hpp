#pragma once

#include "ctrlfs.hpp"

class CtrlFS_bool : public CtrlFSObject
{
public:
  CtrlFS_bool(bool *bool_);
  
public:
  int from_symlink(const std::string &value_);
  int to_symlink(std::string &value_);

private:
  bool *_bool;
};
