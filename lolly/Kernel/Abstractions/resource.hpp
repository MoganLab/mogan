
/** \file resource.hpp
 *  \copyright GPLv3
 *  \details Utils to define a class that holding its instances globally.
 *           Instances is called "Resources", and can be looked up by a string.
 *           Weak referrence is provided to access member of resources.
 *  \author Joris van der Hoeven
 *  \date   1999
 *  \author jingkaimori
 *  \date   2024
 */

#ifndef RESOURCE_H
#define RESOURCE_H

#include "hashmap.hpp"
#include "string.hpp"

/**
 * \brief base class of resources
 * \note no referrence counting is applied on this structure
 */
template <class T> struct rep {
  string res_name;
  inline rep<T> (string res_name2) : res_name (res_name2) {
    T::instances (res_name)= static_cast<pointer> (this);
  }
  inline virtual ~rep<T> () { T::instances->reset (res_name); }
};

template <class R> class resource_ptr {
protected:
  ~resource_ptr () {};

public:
  R*                                     rep;
  inline static hashmap<string, pointer> instances=
      hashmap<string, pointer> (NULL);
  /* C++17 feature, use inline keyword here to pack definition along with
   declaration, instead of defining static member inside expansion
   of RESOURCE macro.*/
  inline R* operator->() { return rep; }
};

#define RESOURCE(PTR)                                                          \
  struct PTR##_rep;                                                            \
  struct PTR : public resource_ptr<PTR##_rep> {                                \
    inline PTR (PTR##_rep* rep2= NULL) { rep= rep2; }                          \
    inline PTR (string s) { rep= (PTR##_rep*) instances[s]; }                  \
    inline ~PTR () {}                                                          \
  }

template <class R>
inline bool
is_nil (const resource_ptr<R>& res) {
  return res.rep == NULL;
}

template <class R>
tm_ostream& operator<< (tm_ostream& out, const resource_ptr<R>& t);

#define make(T, s, im) ((T::instances->contains (s)) ? T (s) : T (im))

template <class T>
tm_ostream&
operator<< (tm_ostream& out, const resource_ptr<T>& t) {
  return out << t->res_name;
}

#endif // RESOURCE_H
