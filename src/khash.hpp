#pragma once

#include <stdint.h>
#include <string.h>

static
inline
uint64_t
FLAGS_SIZE(const uint64_t m_)
{
  return ((m_ < 16) ? 1 : (m_ >> 4));
}

template<typename Key,typename Value>
class KHash
{
public:
  typedef uint32_t khint32_t;
  typedef uint64_t khint64_t;
  typedef uint32_t khint_t;
  typedef khint_t khiter_t;

public:
  KHash();
  ~KHash();

public:
  void clear(void);
  khiter_t get(const Key &key_) const;
  khiter_t put(Key &key_,
               int *rv_);
  Value&  val(const khiter_t i_);
  void    del(khiter_t i_);
  khint_t size(void) const;

private:
  int resize(const khint_t new_n_buckets_);

private:
  khint_t    _n_buckets;
  khint_t    _size;
  khint_t    _n_occupied;
  khint_t    _upper_bound;
  khint32_t *_flags;
  Key       *_keys;
  Value     *_vals;
};

template<typename KEY, typename VALUE>
KHash<KEY,VALUE>::KHash()
{

}

template<typename KEY, typename VALUE>
KHash<KEY,VALUE>::~KHash()
{
  if(_flags)
    delete[] _flags;
  if(_keys)
    delete[] _keys;
  if(_vals)
    delete[] _vals;
}

template<typename KEY, typename VALUE>
typename KHash<KEY,VALUE>::khiter_t
KHash<KEY,VALUE>::put(KEY &key_,
                      int *rv_)
{
  return 0;
}

template<typename KEY, typename VALUE>
typename KHash<KEY,VALUE>::khiter_t
KHash<KEY,VALUE>::get(const KEY &key_) const
{
  return 0;
}

template<typename KEY, typename VALUE>
void
KHash<KEY,VALUE>::clear(void)
{
  uint64_t size;

  if(_flags == NULL)
    return;

  size = (FLAGS_SIZE(_n_buckets) * sizeof(khint32_t));
  ::memset(_flags,0xAA,size);
  _size       = 0;
  _n_occupied = 0;
}

template<typename KEY, typename VALUE>
int
KHash<KEY,VALUE>::resize(const khint_t new_n_buckets_)
{
  return 0;
}
