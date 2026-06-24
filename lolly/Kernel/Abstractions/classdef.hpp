/** \file classdef.hpp
 *  \copyright GPLv3
 *  \details defines concrete and abstract base structures.
 *  \author Joris van der Hoeven
 *  \date   1999
 */

#pragma once

/**
 * @brief Debugging macro used to disable debugging output.
 */
#define TM_DEBUG(x)

/**
 * @brief Global variable holding the number of concrete structures currently
 * active.
 */
extern int concrete_count;

/**
 * @brief Structure representing a concrete object with a reference count.
 */
struct concrete_struct {
  int ref_count; ///< The reference count for the concrete object.

  /**
   * @brief Default constructor for the concrete object. Increments the
   * reference count.
   */
  inline concrete_struct () : ref_count (1) { TM_DEBUG (concrete_count++); }

  /**
   * @brief Virtual destructor for the concrete object. Decrements the reference
   * count.
   */
  virtual inline ~concrete_struct () { TM_DEBUG (concrete_count--); }
};

/**
 * @brief Global variable holding the number of abstract structures currently
 * active.
 */
extern int abstract_count;

/**
 * @brief Structure representing an abstract object with a reference count.
 */
struct abstract_struct {
  int ref_count; ///< The reference count for the abstract object.

  /**
   * @brief Default constructor for the abstract object. Sets the reference
   * count to 0.
   */
  inline abstract_struct () : ref_count (0) { TM_DEBUG (abstract_count++); }

  /**
   * @brief Virtual destructor for the abstract object. Decrements the reference
   * count.
   */
  virtual inline ~abstract_struct () { TM_DEBUG (abstract_count--); }
};

/******************************************************************************
 * indirect structures
 ******************************************************************************/

/**
 * @brief Macro used to increment the reference count for a structure object.
 *
 * @param R The structure object to increment the reference count for.
 */
#define INC_COUNT(R)                                                           \
  {                                                                            \
    (R)->ref_count++;                                                          \
  }

/**
 * @brief Macro used to decrement the reference count for a structure object and
 * delete it if the count reaches 0.
 *
 * @param R The structure object to decrement the reference count for and delete
 * if necessary.
 */
#define DEC_COUNT(R)                                                           \
  {                                                                            \
    if (0 == --((R)->ref_count)) {                                             \
      tm_delete (R);                                                           \
    }                                                                          \
  }
// #define DEC_COUNT(R) { if(0==--((R)->ref_count)) { tm_delete (R); R=NULL;}}

/**
 * @brief Macro used to increment the reference count for a structure object,
 * only if the object is not NULL.
 *
 * @param R The structure object to increment the reference count for.
 */
#define INC_COUNT_NULL(R)                                                      \
  {                                                                            \
    if ((R) != NULL) (R)->ref_count++;                                         \
  }
/*#define DEC_COUNT_NULL(R) \
  { if ((R)!=NULL && 0==--((R)->ref_count)) { tm_delete (R); } } */

/**
 * @brief Macro used to decrement the reference count for a structure object and
 * delete it if the count reaches 0, only if the object is not NULL.
 *
 * @param R The structure object to decrement the reference count for and delete
 * if necessary.
 */
#define DEC_COUNT_NULL(R)                                                      \
  {                                                                            \
    if ((R) != NULL && 0 == --((R)->ref_count)) {                              \
      tm_delete (R);                                                           \
      R= NULL;                                                                 \
    }                                                                          \
  }

// concrete

/**
 * @brief Macro used to define a concrete smart pointer with reference counting.
 *
 * The PTR parameter should be a valid identifier that will be used as the name
 * of the smart pointer type.
 *
 * The macro defines the following:
 * - An opaque pointer to the implementation class, named PTR##_rep;
 * - A public copy constructor and destructor for the smart pointer, and a
 * pointer dereference operator;
 * - A public assignment operator that decrements the reference count for the
 * old object and increments the reference count for the new object.
 *
 * @param PTR The name of the concrete smart pointer type to be defined.
 */
#define CONCRETE(PTR)                                                          \
  PTR##_rep* rep;                                                              \
                                                                               \
public:                                                                        \
  inline PTR (const PTR&);                                                     \
  inline ~PTR ();                                                              \
  inline PTR##_rep* operator->();                                              \
  inline PTR&       operator= (PTR x)

/**
 * @brief Macro used to define the implementation of a concrete smart pointer.
 *
 * This macro defines the following:
 * - A copy constructor that increments the reference count for the new object;
 * - A destructor that decrements the reference count for the old object;
 * - A pointer dereference operator that returns the implementation pointer;
 * - An assignment operator that decrements the reference count for the old
 * object, increments the reference count for the new object, and updates the
 * implementation pointer.
 *
 * @param PTR The name of the concrete smart pointer type to be defined.
 */
#define CONCRETE_CODE(PTR)                                                     \
  inline PTR::PTR (const PTR& x) : rep (x.rep) { INC_COUNT (this->rep); }      \
  inline PTR::~PTR () { DEC_COUNT (this->rep); }                               \
  inline PTR##_rep* PTR::operator->() { return rep; }                          \
  inline PTR&       PTR::operator= (PTR x) {                                   \
    INC_COUNT (x.rep);                                                   \
    DEC_COUNT (this->rep);                                               \
    this->rep= x.rep;                                                    \
    return *this;                                                        \
  }

// definition for 1 parameter template classes

/**
 * @brief Macro used to define a concrete smart pointer with reference counting
 * for a single template parameter.
 *
 * The PTR parameter should be a valid identifier that will be used as the name
 * of the smart pointer type. The T parameter should be a valid template
 * parameter that will be used as the type of the managed object.
 *
 * The macro defines the following:
 * - An opaque pointer to the implementation class, named PTR##_rep<T>;
 * - A public copy constructor and destructor for the smart pointer, and a
 * pointer dereference operator;
 * - A public assignment operator that decrements the reference count for the
 * old object and increments the reference count for the new object.
 *
 * @param PTR The name of the concrete smart pointer type to be defined.
 * @param T The type of the managed object.
 */
#define CONCRETE_TEMPLATE(PTR, T)                                              \
  PTR##_rep<T>* rep;                                                           \
                                                                               \
public:                                                                        \
  inline PTR (const PTR<T>&);                                                  \
  inline ~PTR ();                                                              \
  inline PTR##_rep<T>* operator->();                                           \
  inline PTR<T>&       operator= (PTR<T> x)

/**
 * @brief Macro used to define the implementation of a concrete smart pointer
 * with reference counting for a single template parameter.
 *
 * This macro defines the following:
 * - A copy constructor that increments the reference count for the new object;
 * - A destructor that decrements the reference count for the old object;
 * - A pointer dereference operator that returns the implementation pointer;
 * - An assignment operator that decrements the reference count for the old
 * object, increments the reference count for the new object, and updates the
 * implementation pointer.
 *
 * @param PTR The name of the concrete smart pointer type to be defined.
 * @param TT The template parameter type.
 * @param T The type of the managed object.
 */
#define CONCRETE_TEMPLATE_CODE(PTR, TT, T)                                     \
  template <TT T> inline PTR<T>::PTR (const PTR<T>& x) : rep (x.rep) {         \
    INC_COUNT (this->rep);                                                     \
  }                                                                            \
  template <TT T> inline PTR<T>::~PTR () { DEC_COUNT (this->rep); }            \
  template <TT T> inline PTR##_rep<T>* PTR<T>::operator->() {                  \
    return this->rep;                                                          \
  }                                                                            \
  template <TT T> inline PTR<T>& PTR<T>::operator= (PTR<T> x) {                \
    INC_COUNT (x.rep);                                                         \
    DEC_COUNT (this->rep);                                                     \
    this->rep= x.rep;                                                          \
    return *this;                                                              \
  }

// definition for 2 parameter template classes

/**
 * @brief Macro used to define a concrete smart pointer with reference counting
 * for two template parameters.
 *
 * The PTR parameter should be a valid identifier that will be used as the name
 * of the smart pointer type. The T1 and T2 parameters should be valid template
 * parameters that will be used as the types of the managed objects.
 *
 * The macro defines the following:
 * - An opaque pointer to the implementation class, named PTR##_rep<T1, T2>;
 * - A public copy constructor and destructor for the smart pointer, and a
 * pointer dereference operator;
 * - A public assignment operator that decrements the reference count for the
 * old object and increments the reference count for the new object.
 *
 * @param PTR The name of the concrete smart pointer type to be defined.
 * @param T1 The first type of the managed object.
 * @param T2 The second type of the managed object.
 */
#define CONCRETE_TEMPLATE_2(PTR, T1, T2)                                       \
  PTR##_rep<T1, T2>* rep;                                                      \
                                                                               \
public:                                                                        \
  inline PTR (const PTR<T1, T2>&);                                             \
  inline ~PTR ();                                                              \
  inline PTR##_rep<T1, T2>* operator->();                                      \
  inline PTR<T1, T2>&       operator= (PTR<T1, T2> x)

/**
 * @brief Macro used to define the implementation of a concrete smart pointer
 * with reference counting for two template parameters.
 *
 * This macro defines the following:
 * - A copy constructor that increments the reference count for the new object;
 * - A destructor that decrements the reference count for the old object;
 * - A pointer dereference operator that returns the implementation pointer;
 * - An assignment operator that decrements the reference count for the old
 * object, increments the reference count for the new object, and updates the
 * implementation pointer.
 *
 * @param PTR The name of the concrete smart pointer type to be defined.
 * @param TT1 The first template parameter type.
 * @param T1 The first type of the managed object.
 * @param TT2 The second template parameter type.
 * @param T2 The second type of the managed object.
 */
#define CONCRETE_TEMPLATE_2_CODE(PTR, TT1, T1, TT2, T2)                        \
  template <TT1 T1, TT2 T2>                                                    \
  inline PTR<T1, T2>::PTR (const PTR<T1, T2>& x) : rep (x.rep) {               \
    INC_COUNT (this->rep);                                                     \
  }                                                                            \
  template <TT1 T1, TT2 T2> inline PTR<T1, T2>::~PTR () {                      \
    DEC_COUNT (this->rep);                                                     \
  }                                                                            \
  template <TT1 T1, TT2 T2>                                                    \
  inline PTR##_rep<T1, T2>* PTR<T1, T2>::operator->() {                        \
    return this->rep;                                                          \
  }                                                                            \
  template <TT1 T1, TT2 T2>                                                    \
  inline PTR<T1, T2>& PTR<T1, T2>::operator= (PTR<T1, T2> x) {                 \
    INC_COUNT (x.rep);                                                         \
    DEC_COUNT (this->rep);                                                     \
    this->rep= x.rep;                                                          \
    return *this;                                                              \
  }
// end concrete

// abstract

/**
 * @brief Macro to define an abstract pointer type.
 *
 * @param PTR The name of the abstract pointer type to define.
 */
#define ABSTRACT(PTR)                                                          \
  CONCRETE (PTR);                                                              \
  inline PTR (PTR##_rep*)

/**
 * @brief Macro to define an abstract pointer type with code.
 *
 * @param PTR The name of the abstract pointer type to define.
 */
#define ABSTRACT_CODE(PTR)                                                     \
  CONCRETE_CODE (PTR);                                                         \
  inline PTR::PTR (PTR##_rep* rep2) : rep (rep2) { INC_COUNT (this->rep); }

/**
 * @brief Macro to define a templated abstract pointer type.
 *
 * @param PTR The name of the abstract pointer type to define.
 * @param T The template parameter of the abstract pointer type.
 */
#define ABSTRACT_TEMPLATE(PTR, T)                                              \
  CONCRETE_TEMPLATE (PTR, T);                                                  \
  inline PTR (PTR##_rep<T>*)

/**
 * @brief Macro to define a templated abstract pointer type with code.
 *
 * @param PTR The name of the abstract pointer type to define.
 * @param TT The template parameter type of the concrete implementation.
 * @param T The template parameter of the abstract pointer type.
 */
#define ABSTRACT_TEMPLATE_CODE(PTR, TT, T)                                     \
  CONCRETE_TEMPLATE_CODE (PTR, TT, T);                                         \
  template <TT T> inline PTR<T>::PTR (PTR##_rep<T>* rep2) : rep (rep2) {       \
    INC_COUNT (this->rep);                                                     \
  }
// end abstract

/******************************************************************************
 * null indirect structures
 ******************************************************************************/

// concrete_null

/**
 * @brief Macro to define a concrete null pointer type.
 *
 * @param PTR The name of the concrete null pointer type to define.
 */
#define CONCRETE_NULL(PTR)                                                     \
  CONCRETE (PTR);                                                              \
  inline PTR ();                                                               \
  friend bool is_nil /*LESSGTR*/ (PTR x)

/**
 * @brief Code for the concrete null pointer type.
 *
 * @param PTR The name of the concrete null pointer type.
 */
#define CONCRETE_NULL_CODE(PTR)                                                \
  inline PTR::PTR () : rep (NULL) {}                                           \
  inline PTR::PTR (const PTR& x) : rep (x.rep) { INC_COUNT_NULL (this->rep); } \
  inline PTR::~PTR () { DEC_COUNT_NULL (this->rep); }                          \
  inline PTR##_rep* PTR::operator->() { return this->rep; }                    \
  inline PTR&       PTR::operator= (PTR x) {                                   \
    INC_COUNT_NULL (x.rep);                                              \
    DEC_COUNT_NULL (this->rep);                                          \
    this->rep= x.rep;                                                    \
    return *this;                                                        \
  }                                                                            \
  inline bool is_nil (PTR x) { return x.rep == NULL; }

/**
 * @brief Macro to define a templated concrete null pointer type.
 *
 * @param PTR The name of the templated concrete null pointer type to define.
 * @param T The template parameter type of the concrete null pointer type.
 */
#define CONCRETE_NULL_TEMPLATE(PTR, T)                                         \
  CONCRETE_TEMPLATE (PTR, T);                                                  \
  inline PTR ();                                                               \
  friend bool is_nil LESSGTR (PTR<T> x)

/**
 * @brief Code for the templated concrete null pointer type.
 *
 * @param PTR The name of the templated concrete null pointer type.
 * @param TT The template parameter type of the concrete implementation.
 * @param T The template parameter of the concrete null pointer type.
 */
#define CONCRETE_NULL_TEMPLATE_CODE(PTR, TT, T)                                \
  template <TT T> inline PTR<T>::PTR () : rep (NULL) {}                        \
  template <TT T> inline PTR<T>::PTR (const PTR<T>& x) : rep (x.rep) {         \
    INC_COUNT_NULL (this->rep);                                                \
  }                                                                            \
  template <TT T> inline PTR<T>::~PTR () { DEC_COUNT_NULL (this->rep); }       \
  template <TT T> inline PTR##_rep<T>* PTR<T>::operator->() {                  \
    return this->rep;                                                          \
  }                                                                            \
  template <TT T> inline PTR<T>& PTR<T>::operator= (PTR<T> x) {                \
    INC_COUNT_NULL (x.rep);                                                    \
    DEC_COUNT_NULL (this->rep);                                                \
    this->rep= x.rep;                                                          \
    return *this;                                                              \
  }                                                                            \
  template <TT T> inline bool is_nil (PTR<T> x) { return x.rep == NULL; }

/**
 * @brief Macro for concrete null indirect structure two-template-parameter
 * definition.
 * @param PTR Pointer type of the concrete null indirect structure.
 * @param T1 First template parameter of the concrete null indirect structure.
 * @param T2 Second template parameter of the concrete null indirect structure.
 */
#define CONCRETE_NULL_TEMPLATE_2(PTR, T1, T2)                                  \
  CONCRETE_TEMPLATE_2 (PTR, T1, T2);                                           \
  inline PTR ();                                                               \
  friend bool is_nil LESSGTR (PTR<T1, T2> x)

/**
 * @brief Macro for concrete null indirect structure two-template-parameter code
 * definition.
 * @param PTR Pointer type of the concrete null indirect structure.
 * @param TT1 Template type of the first template parameter of the concrete null
 * indirect structure.
 * @param T1 First template parameter of the concrete null indirect structure.
 * @param TT2 Template type of the second template parameter of the concrete
 * null indirect structure.
 * @param T2 Second template parameter of the concrete null indirect structure.
 */
#define CONCRETE_NULL_TEMPLATE_2_CODE(PTR, TT1, T1, TT2, T2)                   \
  template <TT1 T1, TT2 T2> inline PTR<T1, T2>::PTR () : rep (NULL) {}         \
  template <TT1 T1, TT2 T2>                                                    \
  inline PTR<T1, T2>::PTR (const PTR<T1, T2>& x) : rep (x.rep) {               \
    INC_COUNT_NULL (this->rep);                                                \
  }                                                                            \
  template <TT1 T1, TT2 T2> inline PTR<T1, T2>::~PTR () {                      \
    DEC_COUNT_NULL (this->rep);                                                \
  }                                                                            \
  template <TT1 T1, TT2 T2> PTR##_rep<T1, T2>* PTR<T1, T2>::operator->() {     \
    return this->rep;                                                          \
  }                                                                            \
  template <TT1 T1, TT2 T2>                                                    \
  inline PTR<T1, T2>& PTR<T1, T2>::operator= (PTR<T1, T2> x) {                 \
    INC_COUNT_NULL (x.rep);                                                    \
    DEC_COUNT_NULL (this->rep);                                                \
    this->rep= x.rep;                                                          \
    return *this;                                                              \
  }                                                                            \
  template <TT1 T1, TT2 T2> inline bool is_nil (PTR<T1, T2> x) {               \
    return x.rep == NULL;                                                      \
  }
// end concrete_null

// abstract_null

/**
 * @def ABSTRACT_NULL(PTR)
 * @brief Macro for abstract null indirect structure definition.
 * @param PTR Pointer type of the abstract null indirect structure.
 */
#define ABSTRACT_NULL(PTR)                                                     \
  CONCRETE_NULL (PTR);                                                         \
  inline PTR (PTR##_rep*)

/**
 * @brief Macro for abstract null indirect structure code definition.
 * @param PTR Pointer type of the abstract null indirect structure.
 */
#define ABSTRACT_NULL_CODE(PTR)                                                \
  CONCRETE_NULL_CODE (PTR);                                                    \
  inline PTR::PTR (PTR##_rep* rep2) : rep (rep2) { INC_COUNT_NULL (this->rep); }

/**
 * @brief Macro for abstract null indirect structure template definition.
 * @param PTR Pointer type of the abstract null indirect structure.
 * @param T Template parameter of the abstract null indirect structure.
 */
#define ABSTRACT_NULL_TEMPLATE(PTR, T)                                         \
  CONCRETE_NULL_TEMPLATE (PTR, T);                                             \
  inline PTR (PTR##_rep<T>*)

/**
 * @brief Macro for abstract null indirect structure template code definition.
 * @param PTR Pointer type of the abstract null indirect structure.
 * @param TT Template type of the abstract null indirect structure.
 * @param T Template parameter of the abstract null indirect structure.
 */
#define ABSTRACT_NULL_TEMPLATE_CODE(PTR, TT, T)                                \
  CONCRETE_NULL_TEMPLATE_CODE (PTR, TT, T);                                    \
  template <TT T> inline PTR<T>::PTR (PTR##_rep<T>* rep2) : rep (rep2) {       \
    INC_COUNT (this->rep);                                                     \
  }

/**
 * @brief Macro for abstract null indirect structure two-template-parameter
 * definition.
 * @param PTR Pointer type of the abstract null indirect structure.
 * @param T1 First template parameter of the abstract null indirect structure.
 * @param T2 Second template parameter of the abstract null indirect structure.
 */
#define ABSTRACT_NULL_TEMPLATE_2(PTR, T1, T2)                                  \
  CONCRETE_NULL_TEMPLATE_2 (PTR, T1, T2);                                      \
  inline PTR (PTR##_rep<T1, T2>*)

/**
 * @brief Macro for abstract null indirect structure two-template-parameter code
 * definition.
 * @param PTR Pointer type of the abstract null indirect structure.
 * @param TT1 Template type of the first template parameter of the abstract null
 * indirect structure.
 * @param T1 First template parameter of the abstract null indirect structure.
 * @param TT2 Template type of the second template parameter of the abstract
 * null indirect structure.
 * @param T2 Second template parameter of the abstract null indirect structure.
 */
#define ABSTRACT_NULL_TEMPLATE_2_CODE(PTR, TT1, T1, TT2, T2)                   \
  CONCRETE_NULL_TEMPLATE_2_CODE (PTR, TT1, T1, TT2, T2);                       \
  template <TT1 T1, TT2 T2>                                                    \
  inline PTR<T1, T2>::PTR (PTR##_rep<T1, T2>* rep2) : rep (rep2) {             \
    INC_COUNT (this->rep);                                                     \
  }
// end abstract_null

/******************************************************************************
 * extensions
 ******************************************************************************/

/**
 * @def EXTEND(BASE, PTR)
 * @brief Macro for extension of a base indirect structure with an abstract
 * indirect structure.
 * @param BASE Base indirect structure.
 * @param PTR Abstract indirect structure.
 */
#define EXTEND(BASE, PTR)                                                      \
  ABSTRACT (PTR);                                                              \
  inline PTR (BASE&);                                                          \
  inline operator BASE ()

/**
 * @def EXTEND_CODE(BASE, PTR)
 * @brief Macro for extension of a base indirect structure with an abstract
 * indirect structure code implementation.
 * @param BASE Base indirect structure.
 * @param PTR Abstract indirect structure.
 */
#define EXTEND_CODE(BASE, PTR)                                                 \
  ABSTRACT_CODE (PTR);                                                         \
  inline PTR::PTR (BASE& x) : rep (static_cast<PTR##_rep*> (x.rep)) {          \
    INC_COUNT (this->rep);                                                     \
  }                                                                            \
  inline PTR::operator BASE () { return BASE (this->rep); }
// end extend

// extend_null

/**
 * @def EXTEND_NULL(BASE, PTR)
 * @brief Macro for extension of a base indirect structure with a concrete null
 * indirect structure.
 * @param BASE Base indirect structure.
 * @param PTR Concrete null indirect structure.
 */
#define EXTEND_NULL(BASE, PTR)                                                 \
  ABSTRACT_NULL (PTR);                                                         \
  inline PTR (BASE&);                                                          \
  inline operator BASE ()

/**
 * @def EXTEND_NULL_CODE(BASE, PTR)
 * @brief Macro for extension of a base indirect structure with a concrete null
 * indirect structure code implementation.
 * @param BASE Base indirect structure.
 * @param PTR Concrete null indirect structure.
 */
#define EXTEND_NULL_CODE(BASE, PTR)                                            \
  ABSTRACT_NULL_CODE (PTR);                                                    \
  inline PTR::PTR (BASE& x) : rep (static_cast<PTR##_rep*> (x.rep)) {          \
    INC_COUNT_NULL (this->rep);                                                \
  }                                                                            \
  inline PTR::operator BASE () { return BASE (this->rep); }

/**
 * @def EXTEND_NULL_TEMPLATE(BASE, PTR, T)
 * @brief Macro for extension of a base indirect structure with a concrete null
 * indirect structure with a template parameter.
 * @param BASE Base indirect structure.
 * @param PTR Concrete null indirect structure.
 * @param T Template parameter of the concrete null indirect structure.
 */
#define EXTEND_NULL_TEMPLATE(BASE, PTR, T)                                     \
  ABSTRACT_NULL_TEMPLATE (PTR, T);                                             \
  inline PTR<T> (BASE&);                                                       \
  inline operator BASE ()

/**
 * @def EXTEND_NULL_TEMPLATE_CODE(BASE, PTR, TT, T)
 * @brief Macro for extension of a base indirect structure with a concrete null
 * indirect structure with a template parameter code implementation.
 * @param BASE Base indirect structure.
 * @param PTR Concrete null indirect structure.
 * @param TT Template type of the concrete null indirect structure.
 * @param T Template parameter of the concrete null indirect structure.
 */
#define EXTEND_NULL_TEMPLATE_CODE(BASE, PTR, TT, T)                            \
  ABSTRACT_NULL_TEMPLATE_CODE (PTR, TT, T);                                    \
  template <TT T>                                                              \
  inline PTR<T>::PTR (BASE& x) : rep (static_cast<PTR##_rep<T>*> (x.rep)) {    \
    INC_COUNT_NULL (this->rep);                                                \
  }                                                                            \
  template <TT T> inline PTR<T>::operator BASE () { return BASE (this->rep); }
// end extend_null