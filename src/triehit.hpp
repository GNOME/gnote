

#ifndef __TRIEHIT_HPP_
#define __TRIEHIT_HPP_

#include <list>
#include <string>
#include <tr1/memory>

namespace gnote {

  template<class value_t>
  class TrieHitList
    : public std::list<value_t*>
  {
  public:
    ~TrieHitList()
      {
        typename TrieHitList::iterator iter;
        for(iter = this->begin(); iter != this->end(); ++iter) {
          delete *iter;
        }
      }
  };


  template<class value_t>
  class TrieHit
  {
  public:
    typedef TrieHitList<TrieHit<value_t> > List;
    typedef std::tr1::shared_ptr<List>      ListPtr;
    
    int         start;
    int         end;
    std::string key;
    value_t     value;

    TrieHit(int s, int e, const std::string & k, const value_t & v)
      : start(s), end(e), key(k), value(v)
      {
      }
  };


}

#endif
