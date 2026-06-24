
/**
 * \file      hashmap.h
 * \details   fixed size hashmaps with reference counting
 * \copyright GPLv3
 * \author    Joris van der Hoeven
 * \date      1999
 */

#ifndef HASHMAP_H
#define HASHMAP_H
#include "list.hpp"

template <class T> class list;
template <class T, class U> class hashmap;
template <class T, class U> class rel_hashmap;
template <class T, class U> class rel_hashmap_rep;
template <class T, class U> class hashmap_iterator_rep;

template <class T, class U> int N (hashmap<T, U> a);
template <class T, class U>
tm_ostream& operator<< (tm_ostream& out, hashmap<T, U> h);
template <class T, class U> hashmap<T, U> copy (hashmap<T, U> h);
template <class T, class U>
hashmap<T, U> changes (hashmap<T, U> p, hashmap<T, U> b);
template <class T, class U>
hashmap<T, U> invert (hashmap<T, U> p, hashmap<T, U> b);
template <class T, class U>
bool operator== (hashmap<T, U> h1, hashmap<T, U> h2);
template <class T, class U>
bool operator!= (hashmap<T, U> h1, hashmap<T, U> h2);

/**
 * @brief Hash entry template for key-value pairs.
 *
 * @tparam T Type of the key.
 * @tparam U Type of the value.
 */
template <class T, class U> struct hashentry {
  int code;
  T   key;
  U   im;
  hashentry<T, U> () {}
  hashentry<T, U> (int code, T key2, U im2);
};

template <class T, class U> class hashmap_rep : concrete_struct {
  int                    size; // size of hashmap (nr of entries)
  int                    n;    // nr of keys (a power of two)
  int                    max;  // mean number of entries per key
  U                      init; // default entry
  list<hashentry<T, U>>* a;    // the array of entries

public:
  /**
   * @brief Constructor for the hashmap_rep class.
   *
   * @tparam T Type of the keys in the hash map.
   * @tparam U Type of the values in the hash map.
   * @param init2 Initial value for hash entries. * @param n2 Initial size of
   * the hash table array.
   * @param max2 Maximum allowable size for dynamic resizing.
   */
  inline hashmap_rep<T, U> (U init2, int n2= 1, int max2= 1)
      : size (0), n (n2), max (max2), init (init2),
        a (tm_new_array<list<hashentry<T, U>>> (n)) {}

  /**
   * @brief Destructor for the hashmap_rep class.
   *
   * @tparam T Type of the keys in the hash map.
   * @tparam U Type of the values in the hash map.
   */
  inline ~hashmap_rep<T, U> () { tm_delete_array (a); }

  /**
   * @brief Resizes the hashmap and rehashes all existing keys.
   *
   * @param n The new size of the hashmap.
   * @note This operation could be costly as it rehashes all keys.
   */
  void resize (int n);

  /**
   * @brief Remove a specific key from the hashmap, if it exists.
   *
   * @tparam T The type of the key in the hashmap.
   * @param x The key to be removed from the hashmap.
   */
  void reset (T x);

  /**
   * @brief Applies a given routine to each key in the hashmap.
   *
   * @tparam T The type of the key in the hashmap.
   * @param routine The function pointer to the routine to be applied to each
   * key in the hashmap.
   */
  void generate (void (*routine) (T));

  bool contains (T x);
  bool empty ();
  U    bracket_ro (T x);
  U&   bracket_rw (T x);
  U&   bracket_rw_debug (T x);

  /**
   * @brief Joins another hashmap into the current hashmap.
   *
   * @tparam T The type of the key in the hashmap.
   * @tparam U The type of the value in the hashmap.
   * @param h The hashmap to be joined into the current hashmap.
   */
  void join (hashmap<T, U> H);

  friend class hashmap<T, U>;
  friend class rel_hashmap<T, U>;
  friend class rel_hashmap_rep<T, U>;
  friend class hashmap_iterator_rep<T, U>;
  friend int N LESSGTR (hashmap<T, U> h);
  friend tm_ostream& operator<< LESSGTR (tm_ostream & out, hashmap<T, U> h);

  // only for hashmap<string,tree>
  /**
   * @brief Writes back a key-value pair into the current hashmap using a base
   * hashmap as a reference.
   *
   * @tparam T The type of the key in the hashmap.
   * @tparam U The type of the value in the hashmap.
   * @param x The key to be inserted or updated in the current hashmap.
   * @param base The base hashmap used as a reference for the value of the key.
   */
  void write_back (T x, hashmap<T, U> base);

  /**
   * @brief Applies a patch to the current hashmap using another hashmap as a
   * base reference.
   *
   * @tparam T The type of the key in the hashmap.
   * @tparam U The type of the value in the hashmap.
   * @param patch The hashmap containing the key-value pairs that serve as the
   * patch.
   * @param base The base hashmap used as a reference for the patching process.
   */
  void pre_patch (hashmap<T, U> patch, hashmap<T, U> base);

  /**
   * @brief Applies a post-patch to the current hashmap using another hashmap as
   * a base reference.
   *
   * @tparam T The type of the key in the hashmap.
   * @tparam U The type of the value in the hashmap.
   * @param patch The hashmap containing the key-value pairs that serve as the
   * post-patch.
   * @param base The base hashmap used as a reference for the post-patching
   * process.
   */
  void post_patch (hashmap<T, U> patch, hashmap<T, U> base);

  friend hashmap<T, U> copy    LESSGTR (hashmap<T, U> h);
  friend hashmap<T, U> changes LESSGTR (hashmap<T, U> patch,
                                        hashmap<T, U> base);
  friend hashmap<T, U> invert LESSGTR (hashmap<T, U> patch, hashmap<T, U> base);
  friend class edit_env_rep; // FIXME: broken encapsulation
  // end only for hashmap<string,tree>

  friend bool operator== LESSGTR (hashmap<T, U> h1, hashmap<T, U> h2);
  friend bool operator!= LESSGTR (hashmap<T, U> h1, hashmap<T, U> h2);
};

/**
 * @brief A simple hashmap class implementation.
 * @tparam T The type of the key in the hashmap.
 * @tparam U The type of the value in the hashmap.
 */
template <class T, class U> class hashmap {
  CONCRETE_TEMPLATE_2 (hashmap, T, U);
  static hashmap<T, U> init;

  /**
   * @brief Default constructor that initializes the hashmap with default
   * parameters.
   */
  inline hashmap ()
      : rep (tm_new<hashmap_rep<T, U>> (type_helper<U>::init_val (), 1, 1)) {}

  /**
   * @brief Constructor that allows custom initial value, size, and maximum load
   * factor.
   *
   * @param init The initial value for the type U.
   * @param n The initial size of the hashmap.
   * @param max The maximum load factor of the hashmap.
   */
  inline hashmap (U init, int n= 1, int max= 1)
      : rep (tm_new<hashmap_rep<T, U>> (init, n, max)) {}

  /**
   * @brief Read-only access operator.
   *
   * @param x The key whose value is to be returned.
   * @return The value corresponding to the provided key.
   */
  inline U operator[] (T x) { return rep->bracket_ro (x); }

  /**
   * @brief Read-write access to the value mapped by the key x.
   *
   * @param x The key to look up in the hashmap.
   * @return A reference to the value associated with the key x.
   */
  inline U& operator() (T x) { return rep->bracket_rw (x); }
};
CONCRETE_TEMPLATE_2_CODE (hashmap, class, T, class, U);

#define TMPL template <class T, class U>

/**
 * @brief Returns the size of the given hashmap.
 *
 * @tparam T The type of the key in the hashmap.
 * @tparam U The type of the value in the hashmap.
 * @param h The hashmap whose size is to be determined.
 * @return The size of the hashmap.
 */
TMPL inline int
N (hashmap<T, U> h) {
  return h->size;
}

/**
 * @brief Creates a new hashmap containing entries that have changed in the
 * 'patch' compared to 'base'.
 *
 * @tparam T The type of the key in the hashmap.
 * @tparam U The type of the value in the hashmap.
 * @param patch The hashmap containing updated key-value pairs.
 * @param base The original hashmap that serves as the reference.
 * @return A new hashmap containing only the entries that have changed from
 * 'base' to 'patch'.
 */
TMPL hashmap<T, U> changes (hashmap<T, U> patch, hashmap<T, U> base);

/**
 * @brief Creates a new hashmap containing entries that are different in the
 * 'patch' compared to 'base', but with values taken from 'base'.
 *
 * @tparam T The type of the key in the hashmap.
 * @tparam U The type of the value in the hashmap.
 * @param patch The hashmap containing potentially updated key-value pairs.
 * @param base The original hashmap that serves as the reference.
 * @return A new hashmap containing only the entries that are different in
 * 'patch' but using the values from 'base'.
 */
TMPL hashmap<T, U> invert (hashmap<T, U> patch, hashmap<T, U> base);
#undef TMPL

#include "hashmap.ipp"
#include "hashmap_extra.ipp"

#endif // defined HASHMAP_H
