#pragma once

#include <string>
#include <map>

class CtrlFSObject
{
public:
  virtual int from_symlink(const std::string &value_);
  virtual int to_symlink(std::string &value_);
};


class CtrlFS
{
public:
  void add(const std::string &key_,
           CtrlFSObject      *val_);
  int get(const std::string &key_,
          std::string       &val_);
  int set(const std::string &key_,
          std::string       &val_);

private:
  std::map<std::string,CtrlFSObject*> _map;
};
