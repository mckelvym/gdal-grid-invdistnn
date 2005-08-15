/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 1.3.25
 * 
 * This file is not intended to be easily readable and contains a number of 
 * coding conventions designed to improve portability and efficiency. Do not make
 * changes to this file unless you know what you are doing--modify the SWIG 
 * interface file instead. 
 * ----------------------------------------------------------------------------- */

/***********************************************************************
 *
 *  This section contains generic SWIG labels for method/variable
 *  declarations/attributes, and other compiler dependent labels.
 *
 ************************************************************************/

/* template workaround for compilers that cannot correctly implement the C++ standard */
#ifndef SWIGTEMPLATEDISAMBIGUATOR
#  if defined(__SUNPRO_CC) && (__SUNPRO_CC <= 0x560)
#    define SWIGTEMPLATEDISAMBIGUATOR template
#  else
#    define SWIGTEMPLATEDISAMBIGUATOR 
#  endif
#endif

/* inline attribute */
#ifndef SWIGINLINE
# if defined(__cplusplus) || (defined(__GNUC__) && !defined(__STRICT_ANSI__))
#   define SWIGINLINE inline
# else
#   define SWIGINLINE
# endif
#endif

/* attribute recognised by some compilers to avoid 'unused' warnings */
#ifndef SWIGUNUSED
# if defined(__GNUC__) || defined(__ICC)
#   define SWIGUNUSED __attribute__ ((unused)) 
# else
#   define SWIGUNUSED 
# endif
#endif

/* internal SWIG method */
#ifndef SWIGINTERN
# define SWIGINTERN static SWIGUNUSED
#endif

/* internal inline SWIG method */
#ifndef SWIGINTERNINLINE
# define SWIGINTERNINLINE SWIGINTERN SWIGINLINE
#endif

/* exporting methods for Windows DLLs */
#ifndef SWIGEXPORT
# if defined(_WIN32) || defined(__WIN32__) || defined(__CYGWIN__)
#   if defined(STATIC_LINKED)
#     define SWIGEXPORT
#   else
#     define SWIGEXPORT __declspec(dllexport)
#   endif
# else
#   define SWIGEXPORT
# endif
#endif

/* calling conventions for Windows */
#ifndef SWIGSTDCALL
# if defined(_WIN32) || defined(__WIN32__) || defined(__CYGWIN__)
#   define SWIGSTDCALL __stdcall
# else
#   define SWIGSTDCALL
# endif 
#endif


/* ruby.swg */
/* Implementation : RUBY */
#define SWIGRUBY 1

#include "ruby.h"

/* Flags for pointer conversion */
#define SWIG_POINTER_EXCEPTION     0x1
#define SWIG_POINTER_DISOWN        0x2

#define NUM2USHRT(n) (\
    (0 <= NUM2UINT(n) && NUM2UINT(n) <= USHRT_MAX)\
    ? (unsigned short) NUM2UINT(n) \
    : (rb_raise(rb_eArgError, "integer %d out of range of `unsigned short'",\
               NUM2UINT(n)), (short)0)\
)

#define NUM2SHRT(n) (\
    (SHRT_MIN <= NUM2INT(n) && NUM2INT(n) <= SHRT_MAX)\
    ? (short)NUM2INT(n)\
    : (rb_raise(rb_eArgError, "integer %d out of range of `short'",\
               NUM2INT(n)), (short)0)\
)

/* Ruby 1.7 defines NUM2LL(), LL2NUM() and ULL2NUM() macros */
#ifndef NUM2LL
#define NUM2LL(x) NUM2LONG((x))
#endif
#ifndef LL2NUM
#define LL2NUM(x) INT2NUM((long) (x))
#endif
#ifndef ULL2NUM
#define ULL2NUM(x) UINT2NUM((unsigned long) (x))
#endif

/* Ruby 1.7 doesn't (yet) define NUM2ULL() */
#ifndef NUM2ULL
#ifdef HAVE_LONG_LONG
#define NUM2ULL(x) rb_num2ull((x))
#else
#define NUM2ULL(x) NUM2ULONG(x)
#endif
#endif

/*
 * Need to be very careful about how these macros are defined, especially
 * when compiling C++ code or C code with an ANSI C compiler.
 *
 * VALUEFUNC(f) is a macro used to typecast a C function that implements
 * a Ruby method so that it can be passed as an argument to API functions
 * like rb_define_method() and rb_define_singleton_method().
 *
 * VOIDFUNC(f) is a macro used to typecast a C function that implements
 * either the "mark" or "free" stuff for a Ruby Data object, so that it
 * can be passed as an argument to API functions like Data_Wrap_Struct()
 * and Data_Make_Struct().
 */
 
#ifdef __cplusplus
#  ifndef RUBY_METHOD_FUNC /* These definitions should work for Ruby 1.4.6 */
#    define PROTECTFUNC(f) ((VALUE (*)()) f)
#    define VALUEFUNC(f) ((VALUE (*)()) f)
#    define VOIDFUNC(f)  ((void (*)()) f)
#  else
#    ifndef ANYARGS /* These definitions should work for Ruby 1.6 */
#      define PROTECTFUNC(f) ((VALUE (*)()) f)
#      define VALUEFUNC(f) ((VALUE (*)()) f)
#      define VOIDFUNC(f)  ((RUBY_DATA_FUNC) f)
#    else /* These definitions should work for Ruby 1.7+ */
#      define PROTECTFUNC(f) ((VALUE (*)(VALUE)) f)
#      define VALUEFUNC(f) ((VALUE (*)(ANYARGS)) f)
#      define VOIDFUNC(f)  ((RUBY_DATA_FUNC) f)
#    endif
#  endif
#else
#  define VALUEFUNC(f) (f)
#  define VOIDFUNC(f) (f)
#endif

typedef struct {
  VALUE klass;
  VALUE mImpl;
  void  (*mark)(void *);
  void  (*destroy)(void *);
} swig_class;

/* Don't use for expressions have side effect */
#ifndef RB_STRING_VALUE
#define RB_STRING_VALUE(s) (TYPE(s) == T_STRING ? (s) : (*(volatile VALUE *)&(s) = rb_str_to_str(s)))
#endif
#ifndef StringValue
#define StringValue(s) RB_STRING_VALUE(s)
#endif
#ifndef StringValuePtr
#define StringValuePtr(s) RSTRING(RB_STRING_VALUE(s))->ptr
#endif
#ifndef StringValueLen
#define StringValueLen(s) RSTRING(RB_STRING_VALUE(s))->len
#endif
#ifndef SafeStringValue
#define SafeStringValue(v) do {\
    StringValue(v);\
    rb_check_safe_str(v);\
} while (0)
#endif

#ifndef HAVE_RB_DEFINE_ALLOC_FUNC
#define rb_define_alloc_func(klass, func) rb_define_singleton_method((klass), "new", VALUEFUNC((func)), -1)
#define rb_undef_alloc_func(klass) rb_undef_method(CLASS_OF((klass)), "new")
#endif

/* Contract support */

#define SWIG_contract_assert(expr, msg) if (!(expr)) { rb_raise(rb_eRuntimeError, (char *) msg ); } else


/***********************************************************************
 * swigrun.swg
 *
 *     This file contains generic CAPI SWIG runtime support for pointer
 *     type checking.
 *
 ************************************************************************/

/* This should only be incremented when either the layout of swig_type_info changes,
   or for whatever reason, the runtime changes incompatibly */
#define SWIG_RUNTIME_VERSION "2"

/* define SWIG_TYPE_TABLE_NAME as "SWIG_TYPE_TABLE" */
#ifdef SWIG_TYPE_TABLE
# define SWIG_QUOTE_STRING(x) #x
# define SWIG_EXPAND_AND_QUOTE_STRING(x) SWIG_QUOTE_STRING(x)
# define SWIG_TYPE_TABLE_NAME SWIG_EXPAND_AND_QUOTE_STRING(SWIG_TYPE_TABLE)
#else
# define SWIG_TYPE_TABLE_NAME
#endif

/*
  You can use the SWIGRUNTIME and SWIGRUNTIMEINLINE macros for
  creating a static or dynamic library from the swig runtime code.
  In 99.9% of the cases, swig just needs to declare them as 'static'.
  
  But only do this if is strictly necessary, ie, if you have problems
  with your compiler or so.
*/

#ifndef SWIGRUNTIME
# define SWIGRUNTIME SWIGINTERN
#endif

#ifndef SWIGRUNTIMEINLINE
# define SWIGRUNTIMEINLINE SWIGRUNTIME SWIGINLINE
#endif

#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void *(*swig_converter_func)(void *);
typedef struct swig_type_info *(*swig_dycast_func)(void **);

/* Structure to store inforomation on one type */
typedef struct swig_type_info {
  const char             *name;			/* mangled name of this type */
  const char             *str;			/* human readable name of this type */
  swig_dycast_func        dcast;		/* dynamic cast function down a hierarchy */
  struct swig_cast_info  *cast;			/* linked list of types that can cast into this type */
  void                   *clientdata;		/* language specific type data */
} swig_type_info;

/* Structure to store a type and conversion function used for casting */
typedef struct swig_cast_info {
  swig_type_info         *type;			/* pointer to type that is equivalent to this type */
  swig_converter_func     converter;		/* function to cast the void pointers */
  struct swig_cast_info  *next;			/* pointer to next cast in linked list */
  struct swig_cast_info  *prev;			/* pointer to the previous cast */
} swig_cast_info;

/* Structure used to store module information
 * Each module generates one structure like this, and the runtime collects
 * all of these structures and stores them in a circularly linked list.*/
typedef struct swig_module_info {
  swig_type_info         **types;		/* Array of pointers to swig_type_info structures that are in this module */
  size_t                 size;		        /* Number of types in this module */
  struct swig_module_info *next;		/* Pointer to next element in circularly linked list */
  swig_type_info         **type_initial;	/* Array of initially generated type structures */
  swig_cast_info         **cast_initial;	/* Array of initially generated casting structures */
  void                    *clientdata;		/* Language specific module data */
} swig_module_info;


/* 
  Compare two type names skipping the space characters, therefore
  "char*" == "char *" and "Class<int>" == "Class<int >", etc.

  Return 0 when the two name types are equivalent, as in
  strncmp, but skipping ' '.
*/
SWIGRUNTIME int
SWIG_TypeNameComp(const char *f1, const char *l1,
		  const char *f2, const char *l2) {
  for (;(f1 != l1) && (f2 != l2); ++f1, ++f2) {
    while ((*f1 == ' ') && (f1 != l1)) ++f1;
    while ((*f2 == ' ') && (f2 != l2)) ++f2;
    if (*f1 != *f2) return (int)(*f1 - *f2);
  }
  return (l1 - f1) - (l2 - f2);
}

/*
  Check type equivalence in a name list like <name1>|<name2>|...
  Return 0 if not equal, 1 if equal
*/
SWIGRUNTIME int
SWIG_TypeEquiv(const char *nb, const char *tb) {
  int equiv = 0;
  const char* te = tb + strlen(tb);
  const char* ne = nb;
  while (!equiv && *ne) {
    for (nb = ne; *ne; ++ne) {
      if (*ne == '|') break;
    }
    equiv = (SWIG_TypeNameComp(nb, ne, tb, te) == 0) ? 1 : 0;
    if (*ne) ++ne;
  }
  return equiv;
}

/*
  Check type equivalence in a name list like <name1>|<name2>|...
  Return 0 if equal, -1 if nb < tb, 1 if nb > tb
*/
SWIGRUNTIME int
SWIG_TypeCompare(const char *nb, const char *tb) {
  int equiv = 0;
  const char* te = tb + strlen(tb);
  const char* ne = nb;
  while (!equiv && *ne) {
    for (nb = ne; *ne; ++ne) {
      if (*ne == '|') break;
    }
    equiv = (SWIG_TypeNameComp(nb, ne, tb, te) == 0) ? 1 : 0;
    if (*ne) ++ne;
  }
  return equiv;
}


/* think of this as a c++ template<> or a scheme macro */
#define SWIG_TypeCheck_Template(comparison, ty)         \
  if (ty) {                                             \
    swig_cast_info *iter = ty->cast;                    \
    while (iter) {                                      \
      if (comparison) {                                 \
        if (iter == ty->cast) return iter;              \
        /* Move iter to the top of the linked list */   \
        iter->prev->next = iter->next;                  \
        if (iter->next)                                 \
          iter->next->prev = iter->prev;                \
        iter->next = ty->cast;                          \
        iter->prev = 0;                                 \
        if (ty->cast) ty->cast->prev = iter;            \
        ty->cast = iter;                                \
        return iter;                                    \
      }                                                 \
      iter = iter->next;                                \
    }                                                   \
  }                                                     \
  return 0

/*
  Check the typename
*/
SWIGRUNTIME swig_cast_info *
SWIG_TypeCheck(const char *c, swig_type_info *ty) {
  SWIG_TypeCheck_Template(strcmp(iter->type->name, c) == 0, ty);
}

/* Same as previous function, except strcmp is replaced with a pointer comparison */
SWIGRUNTIME swig_cast_info *
SWIG_TypeCheckStruct(swig_type_info *from, swig_type_info *into) {
  SWIG_TypeCheck_Template(iter->type == from, into);
}

/*
  Cast a pointer up an inheritance hierarchy
*/
SWIGRUNTIMEINLINE void *
SWIG_TypeCast(swig_cast_info *ty, void *ptr) {
  return ((!ty) || (!ty->converter)) ? ptr : (*ty->converter)(ptr);
}

/* 
   Dynamic pointer casting. Down an inheritance hierarchy
*/
SWIGRUNTIME swig_type_info *
SWIG_TypeDynamicCast(swig_type_info *ty, void **ptr) {
  swig_type_info *lastty = ty;
  if (!ty || !ty->dcast) return ty;
  while (ty && (ty->dcast)) {
    ty = (*ty->dcast)(ptr);
    if (ty) lastty = ty;
  }
  return lastty;
}

/*
  Return the name associated with this type
*/
SWIGRUNTIMEINLINE const char *
SWIG_TypeName(const swig_type_info *ty) {
  return ty->name;
}

/*
  Return the pretty name associated with this type,
  that is an unmangled type name in a form presentable to the user.
*/
SWIGRUNTIME const char *
SWIG_TypePrettyName(const swig_type_info *type) {
  /* The "str" field contains the equivalent pretty names of the
     type, separated by vertical-bar characters.  We choose
     to print the last name, as it is often (?) the most
     specific. */
  if (type->str != NULL) {
    const char *last_name = type->str;
    const char *s;
    for (s = type->str; *s; s++)
      if (*s == '|') last_name = s+1;
    return last_name;
  }
  else
    return type->name;
}

/* 
   Set the clientdata field for a type
*/
SWIGRUNTIME void
SWIG_TypeClientData(swig_type_info *ti, void *clientdata) {
  if (!ti->clientdata) {
    swig_cast_info *cast = ti->cast;
    /* if (ti->clientdata == clientdata) return; */
    ti->clientdata = clientdata;
    
    while (cast) {
      if (!cast->converter)
	SWIG_TypeClientData(cast->type, clientdata);
      cast = cast->next;
    }
  }
}

/*
  Search for a swig_type_info structure only by mangled name
  Search is a O(log #types)
  
  We start searching at module start, and finish searching when start == end.  
  Note: if start == end at the beginning of the function, we go all the way around
  the circular list.
*/
SWIGRUNTIME swig_type_info *
SWIG_MangledTypeQueryModule(swig_module_info *start, 
                            swig_module_info *end, 
		            const char *name) {
  swig_module_info *iter = start;
  do {
    if (iter->size) {
      register size_t l = 0;
      register size_t r = iter->size - 1;
      do {
	/* since l+r >= 0, we can (>> 1) instead (/ 2) */
	register size_t i = (l + r) >> 1; 
	const char *iname = iter->types[i]->name;
	if (iname) {
	  register int compare = strcmp(name, iname);
	  if (compare == 0) {	    
	    return iter->types[i];
	  } else if (compare < 0) {
	    if (i) {
	      r = i - 1;
	    } else {
	      break;
	    }
	  } else if (compare > 0) {
	    l = i + 1;
	  }
	} else {
	  break; /* should never happen */
	}
      } while (l <= r);
    }
    iter = iter->next;
  } while (iter != end);
  return 0;
}

/*
  Search for a swig_type_info structure for either a mangled name or a human readable name.
  It first searches the mangled names of the types, which is a O(log #types)
  If a type is not found it then searches the human readable names, which is O(#types).
  
  We start searching at module start, and finish searching when start == end.  
  Note: if start == end at the beginning of the function, we go all the way around
  the circular list.
*/
SWIGRUNTIME swig_type_info *
SWIG_TypeQueryModule(swig_module_info *start, 
                     swig_module_info *end, 
		     const char *name) {
  /* STEP 1: Search the name field using binary search */
  swig_type_info *ret = SWIG_MangledTypeQueryModule(start, end, name);
  if (ret) {
    return ret;
  } else {
    /* STEP 2: If the type hasn't been found, do a complete search
       of the str field (the human readable name) */
    swig_module_info *iter = start;
    do {
      register size_t i = 0;
      for (; i < iter->size; ++i) {
	if (iter->types[i]->str && (SWIG_TypeEquiv(iter->types[i]->str, name)))
	  return iter->types[i];
      }
      iter = iter->next;
    } while (iter != end);
  }
  
  /* neither found a match */
  return 0;
}


/* 
   Pack binary data into a string
*/
SWIGRUNTIME char *
SWIG_PackData(char *c, void *ptr, size_t sz) {
  static const char hex[17] = "0123456789abcdef";
  register const unsigned char *u = (unsigned char *) ptr;
  register const unsigned char *eu =  u + sz;
  for (; u != eu; ++u) {
    register unsigned char uu = *u;
    *(c++) = hex[(uu & 0xf0) >> 4];
    *(c++) = hex[uu & 0xf];
  }
  return c;
}

/* 
   Unpack binary data from a string
*/
SWIGRUNTIME const char *
SWIG_UnpackData(const char *c, void *ptr, size_t sz) {
  register unsigned char *u = (unsigned char *) ptr;
  register const unsigned char *eu = u + sz;
  for (; u != eu; ++u) {
    register char d = *(c++);
    register unsigned char uu = 0;
    if ((d >= '0') && (d <= '9'))
      uu = ((d - '0') << 4);
    else if ((d >= 'a') && (d <= 'f'))
      uu = ((d - ('a'-10)) << 4);
    else 
      return (char *) 0;
    d = *(c++);
    if ((d >= '0') && (d <= '9'))
      uu |= (d - '0');
    else if ((d >= 'a') && (d <= 'f'))
      uu |= (d - ('a'-10));
    else 
      return (char *) 0;
    *u = uu;
  }
  return c;
}

/* 
   Pack 'void *' into a string buffer.
*/
SWIGRUNTIME char *
SWIG_PackVoidPtr(char *buff, void *ptr, const char *name, size_t bsz) {
  char *r = buff;
  if ((2*sizeof(void *) + 2) > bsz) return 0;
  *(r++) = '_';
  r = SWIG_PackData(r,&ptr,sizeof(void *));
  if (strlen(name) + 1 > (bsz - (r - buff))) return 0;
  strcpy(r,name);
  return buff;
}

SWIGRUNTIME const char *
SWIG_UnpackVoidPtr(const char *c, void **ptr, const char *name) {
  if (*c != '_') {
    if (strcmp(c,"NULL") == 0) {
      *ptr = (void *) 0;
      return name;
    } else {
      return 0;
    }
  }
  return SWIG_UnpackData(++c,ptr,sizeof(void *));
}

SWIGRUNTIME char *
SWIG_PackDataName(char *buff, void *ptr, size_t sz, const char *name, size_t bsz) {
  char *r = buff;
  size_t lname = (name ? strlen(name) : 0);
  if ((2*sz + 2 + lname) > bsz) return 0;
  *(r++) = '_';
  r = SWIG_PackData(r,ptr,sz);
  if (lname) {
    strncpy(r,name,lname+1);
  } else {
    *r = 0;
  }
  return buff;
}

SWIGRUNTIME const char *
SWIG_UnpackDataName(const char *c, void *ptr, size_t sz, const char *name) {
  if (*c != '_') {
    if (strcmp(c,"NULL") == 0) {
      memset(ptr,0,sz);
      return name;
    } else {
      return 0;
    }
  }
  return SWIG_UnpackData(++c,ptr,sz);
}

#ifdef __cplusplus
}
#endif

/* Common SWIG API */
#define SWIG_ConvertPtr(obj, pp, type, flags) \
  SWIG_Ruby_ConvertPtr(obj, pp, type, flags)
#define SWIG_NewPointerObj(p, type, flags) \
  SWIG_Ruby_NewPointerObj(p, type, flags)
#define SWIG_MustGetPtr(p, type, argnum, flags) \
  SWIG_Ruby_MustGetPtr(p, type, argnum, flags)
#define SWIG_GetModule(clientdata) \
  SWIG_Ruby_GetModule()
#define SWIG_SetModule(clientdata, pointer) \
  SWIG_Ruby_SetModule(pointer)

/* Ruby-specific SWIG API */

#define SWIG_InitRuntime() \
  SWIG_Ruby_InitRuntime()
#define SWIG_define_class(ty) \
  SWIG_Ruby_define_class(ty)
#define SWIG_NewClassInstance(value, ty) \
  SWIG_Ruby_NewClassInstance(value, ty)
#define SWIG_MangleStr(value) \
  SWIG_Ruby_MangleStr(value)
#define SWIG_CheckConvert(value, ty) \
  SWIG_Ruby_CheckConvert(value, ty)
#define SWIG_NewPackedObj(ptr, sz, ty) \
  SWIG_Ruby_NewPackedObj(ptr, sz, ty)
#define SWIG_ConvertPacked(obj, ptr, sz, ty, flags) \
  SWIG_Ruby_ConvertPacked(obj, ptr, sz, ty, flags)

/* rubydef.swg */
#ifdef __cplusplus
extern "C" {
#endif

static VALUE _mSWIG = Qnil;
static VALUE _cSWIG_Pointer = Qnil;
static VALUE swig_runtime_data_type_pointer = Qnil;

/* Initialize Ruby runtime support */
static void
SWIG_Ruby_InitRuntime(void)
{
    if (_mSWIG == Qnil) {
        _mSWIG = rb_define_module("SWIG");
    }
}

/* Define Ruby class for C type */
static void
SWIG_Ruby_define_class(swig_type_info *type)
{
    VALUE klass;
    char *klass_name = (char *) malloc(4 + strlen(type->name) + 1);
    sprintf(klass_name, "TYPE%s", type->name);
    if (NIL_P(_cSWIG_Pointer)) {
	_cSWIG_Pointer = rb_define_class_under(_mSWIG, "Pointer", rb_cObject);
	rb_undef_method(CLASS_OF(_cSWIG_Pointer), "new");
    }
    klass = rb_define_class_under(_mSWIG, klass_name, _cSWIG_Pointer);
    free((void *) klass_name);
}

/* Create a new pointer object */
static VALUE
SWIG_Ruby_NewPointerObj(void *ptr, swig_type_info *type, int own)
{
    char *klass_name;
    swig_class *sklass;
    VALUE klass;
    VALUE obj;
    
    if (!ptr)
	return Qnil;
    
    if (type->clientdata) {
      sklass = (swig_class *) type->clientdata;
      obj = Data_Wrap_Struct(sklass->klass, VOIDFUNC(sklass->mark), (own ? VOIDFUNC(sklass->destroy) : 0), ptr);
    } else {
      klass_name = (char *) malloc(4 + strlen(type->name) + 1);
      sprintf(klass_name, "TYPE%s", type->name);
      klass = rb_const_get(_mSWIG, rb_intern(klass_name));
      free((void *) klass_name);
      obj = Data_Wrap_Struct(klass, 0, 0, ptr);
    }
    rb_iv_set(obj, "__swigtype__", rb_str_new2(type->name));
    return obj;
}

/* Create a new class instance (always owned) */
static VALUE
SWIG_Ruby_NewClassInstance(VALUE klass, swig_type_info *type)
{
    VALUE obj;
    swig_class *sklass = (swig_class *) type->clientdata;
    obj = Data_Wrap_Struct(klass, VOIDFUNC(sklass->mark), VOIDFUNC(sklass->destroy), 0);
    rb_iv_set(obj, "__swigtype__", rb_str_new2(type->name));
    return obj;
}

/* Get type mangle from class name */
static SWIGINLINE char *
SWIG_Ruby_MangleStr(VALUE obj)
{
  VALUE stype = rb_iv_get(obj, "__swigtype__");
  return StringValuePtr(stype);
}

/* Convert a pointer value */
static int
SWIG_Ruby_ConvertPtr(VALUE obj, void **ptr, swig_type_info *ty, int flags)
{
  char *c;
  swig_cast_info *tc;

  /* Grab the pointer */
  if (NIL_P(obj)) {
    *ptr = 0;
    return 0;
  } else {
    Data_Get_Struct(obj, void, *ptr);
  }
  
  /* Do type-checking if type info was provided */
  if (ty) {
    if (ty->clientdata) {
        if (rb_obj_is_kind_of(obj, ((swig_class *) (ty->clientdata))->klass)) {
          if (*ptr == 0)
            rb_raise(rb_eRuntimeError, "This %s already released", ty->str);
          return 0;
        }
    }
    if ((c = SWIG_MangleStr(obj)) == NULL) {
      if (flags & SWIG_POINTER_EXCEPTION)
        rb_raise(rb_eTypeError, "Expected %s", ty->str);
      else
        return -1;
    }
    tc = SWIG_TypeCheck(c, ty);
    if (!tc) {
      if (flags & SWIG_POINTER_EXCEPTION)
        rb_raise(rb_eTypeError, "Expected %s", ty->str);
      else
        return -1;
    }
    *ptr = SWIG_TypeCast(tc, *ptr);
  }
  return 0;
}

/* Convert a pointer value, signal an exception on a type mismatch */
static SWIGINLINE void *
SWIG_Ruby_MustGetPtr(VALUE obj, swig_type_info *ty, int argnum, int flags)
{
  void *result;
  SWIG_ConvertPtr(obj, &result, ty, flags | SWIG_POINTER_EXCEPTION);
  return result;
}

/* Check convert */
static SWIGINLINE int
SWIG_Ruby_CheckConvert(VALUE obj, swig_type_info *ty)
{
  char *c = SWIG_MangleStr(obj);
  if (!c)
    return 0;
  return SWIG_TypeCheck(c,ty) != 0;
}

static VALUE
SWIG_Ruby_NewPackedObj(void *ptr, int sz, swig_type_info *type) {
  char result[1024];
  char *r = result;
  if ((2*sz + 1 + strlen(type->name)) > 1000) return 0;
  *(r++) = '_';
  r = SWIG_PackData(r, ptr, sz);
  strcpy(r, type->name);
  return rb_str_new2(result);
}

/* Convert a packed value value */
static void
SWIG_Ruby_ConvertPacked(VALUE obj, void *ptr, int sz, swig_type_info *ty, int flags) {
  swig_cast_info *tc;
  const char  *c;

  if (TYPE(obj) != T_STRING) goto type_error;
  c = StringValuePtr(obj);
  /* Pointer values must start with leading underscore */
  if (*c != '_') goto type_error;
  c++;
  c = SWIG_UnpackData(c, ptr, sz);
  if (ty) {
    tc = SWIG_TypeCheck(c, ty);
    if (!tc) goto type_error;
  }
  return;

type_error:

  if (flags) {
    if (ty) {
      rb_raise(rb_eTypeError, "Type error. Expected %s", ty->name);
    } else {
      rb_raise(rb_eTypeError, "Expected a pointer");
    }
  }
}

static swig_module_info *SWIG_Ruby_GetModule() {
    VALUE pointer;
    swig_module_info *ret = 0;

   /* first check if pointer already created */
    pointer = rb_gv_get("$swig_runtime_data_type_pointer" SWIG_RUNTIME_VERSION SWIG_TYPE_TABLE_NAME);
    if (pointer != Qnil) {
      Data_Get_Struct(pointer, swig_module_info, ret);
    }
    return ret;
}

static void SWIG_Ruby_SetModule(swig_module_info *pointer) {
      /* register a new class */
      VALUE cl = rb_define_class("swig_runtime_data", rb_cObject);
      /* create and store the structure pointer to a global variable */
      swig_runtime_data_type_pointer = Data_Wrap_Struct(cl, 0, 0, pointer);
      rb_define_readonly_variable("$swig_runtime_data_type_pointer" SWIG_RUNTIME_VERSION SWIG_TYPE_TABLE_NAME, &swig_runtime_data_type_pointer);
}

#ifdef __cplusplus
}
#endif



/* -------- TYPES TABLE (BEGIN) -------- */

#define SWIGTYPE_p_unsigned_long swig_types[0]
static swig_type_info *swig_types[1];
static swig_module_info swig_module = {swig_types, 1, 0, 0, 0, 0};
#define SWIG_TypeQuery(name) SWIG_TypeQueryModule(&swig_module, &swig_module, name)
#define SWIG_MangledTypeQuery(name) SWIG_MangledTypeQueryModule(&swig_module, &swig_module, name)

/* -------- TYPES TABLE (END) -------- */

#define SWIG_init    Init_gdalconst
#define SWIG_name    "Gdalconst"

static VALUE mGdalconst;

static void SWIG_AsVal(VALUE obj, int *val)
{
    *val = (int) NUM2INT(obj);
}


#include "gdal.h"
#include "gdalwarper.h"
#include "cpl_string.h"
#include "cpl_minixml.h"


/* -------- TYPE CONVERSION AND EQUIVALENCE RULES (BEGIN) -------- */

static swig_type_info _swigt__p_unsigned_long = {"_p_unsigned_long", "unsigned long *|VALUE *", 0, 0, 0};

static swig_type_info *swig_type_initial[] = {
  &_swigt__p_unsigned_long,
};

static swig_cast_info _swigc__p_unsigned_long[] = {  {&_swigt__p_unsigned_long, 0, 0, 0},{0, 0, 0, 0}};

static swig_cast_info *swig_cast_initial[] = {
  _swigc__p_unsigned_long,
};


/* -------- TYPE CONVERSION AND EQUIVALENCE RULES (END) -------- */

/*************************************************************************
 * Type initialization:
 * This problem is tough by the requirement that no dynamic 
 * memory is used. Also, since swig_type_info structures store pointers to 
 * swig_cast_info structures and swig_cast_info structures store pointers back
 * to swig_type_info structures, we need some lookup code at initialization. 
 * The idea is that swig generates all the structures that are needed. 
 * The runtime then collects these partially filled structures. 
 * The SWIG_InitializeModule function takes these initial arrays out of 
 * swig_module, and does all the lookup, filling in the swig_module.types
 * array with the correct data and linking the correct swig_cast_info
 * structures together.

 * The generated swig_type_info structures are assigned staticly to an initial 
 * array. We just loop though that array, and handle each type individually.
 * First we lookup if this type has been already loaded, and if so, use the
 * loaded structure instead of the generated one. Then we have to fill in the
 * cast linked list. The cast data is initially stored in something like a
 * two-dimensional array. Each row corresponds to a type (there are the same
 * number of rows as there are in the swig_type_initial array). Each entry in
 * a column is one of the swig_cast_info structures for that type.
 * The cast_initial array is actually an array of arrays, because each row has
 * a variable number of columns. So to actually build the cast linked list,
 * we find the array of casts associated with the type, and loop through it 
 * adding the casts to the list. The one last trick we need to do is making
 * sure the type pointer in the swig_cast_info struct is correct.

 * First off, we lookup the cast->type name to see if it is already loaded. 
 * There are three cases to handle:
 *  1) If the cast->type has already been loaded AND the type we are adding
 *     casting info to has not been loaded (it is in this module), THEN we
 *     replace the cast->type pointer with the type pointer that has already
 *     been loaded.
 *  2) If BOTH types (the one we are adding casting info to, and the 
 *     cast->type) are loaded, THEN the cast info has already been loaded by
 *     the previous module so we just ignore it.
 *  3) Finally, if cast->type has not already been loaded, then we add that
 *     swig_cast_info to the linked list (because the cast->type) pointer will
 *     be correct.
**/

#ifdef __cplusplus
extern "C" {
#endif

SWIGRUNTIME void
SWIG_InitializeModule(void *clientdata) {
  swig_type_info *type, *ret;
  swig_cast_info *cast;
  size_t i;
  swig_module_info *module_head;
  static int init_run = 0;

  clientdata = clientdata;

  if (init_run) return;
  init_run = 1;

  /* Initialize the swig_module */
  swig_module.type_initial = swig_type_initial;
  swig_module.cast_initial = swig_cast_initial;

  /* Try and load any already created modules */
  module_head = SWIG_GetModule(clientdata);
  if (module_head) {
    swig_module.next = module_head->next;
    module_head->next = &swig_module;
  } else {
    /* This is the first module loaded */
    swig_module.next = &swig_module;
    SWIG_SetModule(clientdata, &swig_module);
  }
		 
  /* Now work on filling in swig_module.types */
  for (i = 0; i < swig_module.size; ++i) {
    type = 0;

    /* if there is another module already loaded */
    if (swig_module.next != &swig_module) {
      type = SWIG_MangledTypeQueryModule(swig_module.next, &swig_module, swig_module.type_initial[i]->name);
    }
    if (type) {
      /* Overwrite clientdata field */
      if (swig_module.type_initial[i]->clientdata) type->clientdata = swig_module.type_initial[i]->clientdata;
    } else {
      type = swig_module.type_initial[i];
    }

    /* Insert casting types */
    cast = swig_module.cast_initial[i];
    while (cast->type) {
    
      /* Don't need to add information already in the list */
      ret = 0;
      if (swig_module.next != &swig_module) {
        ret = SWIG_MangledTypeQueryModule(swig_module.next, &swig_module, cast->type->name);
      }
      if (ret && type == swig_module.type_initial[i]) {
        cast->type = ret;
        ret = 0;
      }
      
      if (!ret) {
        if (type->cast) {
          type->cast->prev = cast;
          cast->next = type->cast;
        }
        type->cast = cast;
      }

      cast++;
    }

    /* Set entry in modules->types array equal to the type */
    swig_module.types[i] = type;
  }
}

/* This function will propagate the clientdata field of type to
* any new swig_type_info structures that have been added into the list
* of equivalent types.  It is like calling
* SWIG_TypeClientData(type, clientdata) a second time.
*/
SWIGRUNTIME void
SWIG_PropagateClientData(void) {
  size_t i;
  swig_cast_info *equiv;
  static int init_run = 0;

  if (init_run) return;
  init_run = 1;

  for (i = 0; i < swig_module.size; i++) {
    if (swig_module.types[i]->clientdata) {
      equiv = swig_module.types[i]->cast;
      while (equiv) {
        if (!equiv->converter) {
          if (equiv->type && !equiv->type->clientdata)
            SWIG_TypeClientData(equiv->type, swig_module.types[i]->clientdata);
        }
        equiv = equiv->next;
      }
    }
  }
}

#ifdef __cplusplus
}
#endif


#ifdef __cplusplus
extern "C"
#endif
SWIGEXPORT void Init_gdalconst(void) {
    int i;
    
    SWIG_InitRuntime();
    mGdalconst = rb_define_module("Gdalconst");
    
    SWIG_InitializeModule(0);
    for (i = 0; i < swig_module.size; i++) {
        SWIG_define_class(swig_module.types[i]);
    }
    
    rb_define_const(mGdalconst,"GDT_Unknown", INT2NUM(GDT_Unknown));
    rb_define_const(mGdalconst,"GDT_Byte", INT2NUM(GDT_Byte));
    rb_define_const(mGdalconst,"GDT_UInt16", INT2NUM(GDT_UInt16));
    rb_define_const(mGdalconst,"GDT_Int16", INT2NUM(GDT_Int16));
    rb_define_const(mGdalconst,"GDT_UInt32", INT2NUM(GDT_UInt32));
    rb_define_const(mGdalconst,"GDT_Int32", INT2NUM(GDT_Int32));
    rb_define_const(mGdalconst,"GDT_Float32", INT2NUM(GDT_Float32));
    rb_define_const(mGdalconst,"GDT_Float64", INT2NUM(GDT_Float64));
    rb_define_const(mGdalconst,"GDT_CInt16", INT2NUM(GDT_CInt16));
    rb_define_const(mGdalconst,"GDT_CInt32", INT2NUM(GDT_CInt32));
    rb_define_const(mGdalconst,"GDT_CFloat32", INT2NUM(GDT_CFloat32));
    rb_define_const(mGdalconst,"GDT_CFloat64", INT2NUM(GDT_CFloat64));
    rb_define_const(mGdalconst,"GDT_TypeCount", INT2NUM(GDT_TypeCount));
    rb_define_const(mGdalconst,"GA_ReadOnly", INT2NUM(GA_ReadOnly));
    rb_define_const(mGdalconst,"GA_Update", INT2NUM(GA_Update));
    rb_define_const(mGdalconst,"GF_Read", INT2NUM(GF_Read));
    rb_define_const(mGdalconst,"GF_Write", INT2NUM(GF_Write));
    rb_define_const(mGdalconst,"GCI_Undefined", INT2NUM(GCI_Undefined));
    rb_define_const(mGdalconst,"GCI_GrayIndex", INT2NUM(GCI_GrayIndex));
    rb_define_const(mGdalconst,"GCI_PaletteIndex", INT2NUM(GCI_PaletteIndex));
    rb_define_const(mGdalconst,"GCI_RedBand", INT2NUM(GCI_RedBand));
    rb_define_const(mGdalconst,"GCI_GreenBand", INT2NUM(GCI_GreenBand));
    rb_define_const(mGdalconst,"GCI_BlueBand", INT2NUM(GCI_BlueBand));
    rb_define_const(mGdalconst,"GCI_AlphaBand", INT2NUM(GCI_AlphaBand));
    rb_define_const(mGdalconst,"GCI_HueBand", INT2NUM(GCI_HueBand));
    rb_define_const(mGdalconst,"GCI_SaturationBand", INT2NUM(GCI_SaturationBand));
    rb_define_const(mGdalconst,"GCI_LightnessBand", INT2NUM(GCI_LightnessBand));
    rb_define_const(mGdalconst,"GCI_CyanBand", INT2NUM(GCI_CyanBand));
    rb_define_const(mGdalconst,"GCI_MagentaBand", INT2NUM(GCI_MagentaBand));
    rb_define_const(mGdalconst,"GCI_YellowBand", INT2NUM(GCI_YellowBand));
    rb_define_const(mGdalconst,"GCI_BlackBand", INT2NUM(GCI_BlackBand));
    rb_define_const(mGdalconst,"GRA_NearestNeighbour", INT2NUM(GRA_NearestNeighbour));
    rb_define_const(mGdalconst,"GRA_Bilinear", INT2NUM(GRA_Bilinear));
    rb_define_const(mGdalconst,"GRA_Cubic", INT2NUM(GRA_Cubic));
    rb_define_const(mGdalconst,"GRA_CubicSpline", INT2NUM(GRA_CubicSpline));
    rb_define_const(mGdalconst,"GPI_Gray", INT2NUM(GPI_Gray));
    rb_define_const(mGdalconst,"GPI_RGB", INT2NUM(GPI_RGB));
    rb_define_const(mGdalconst,"GPI_CMYK", INT2NUM(GPI_CMYK));
    rb_define_const(mGdalconst,"GPI_HLS", INT2NUM(GPI_HLS));
    rb_define_const(mGdalconst,"CXT_Element", INT2NUM(CXT_Element));
    rb_define_const(mGdalconst,"CXT_Text", INT2NUM(CXT_Text));
    rb_define_const(mGdalconst,"CXT_Attribute", INT2NUM(CXT_Attribute));
    rb_define_const(mGdalconst,"CXT_Comment", INT2NUM(CXT_Comment));
    rb_define_const(mGdalconst,"CXT_Literal", INT2NUM(CXT_Literal));
    rb_define_const(mGdalconst,"CE_None", INT2NUM(CE_None));
    rb_define_const(mGdalconst,"CE_Debug", INT2NUM(CE_Debug));
    rb_define_const(mGdalconst,"CE_Warning", INT2NUM(CE_Warning));
    rb_define_const(mGdalconst,"CE_Failure", INT2NUM(CE_Failure));
    rb_define_const(mGdalconst,"CE_Fatal", INT2NUM(CE_Fatal));
    rb_define_const(mGdalconst,"CPLE_None", INT2NUM(CPLE_None));
    rb_define_const(mGdalconst,"CPLE_AppDefined", INT2NUM(CPLE_AppDefined));
    rb_define_const(mGdalconst,"CPLE_OutOfMemory", INT2NUM(CPLE_OutOfMemory));
    rb_define_const(mGdalconst,"CPLE_FileIO", INT2NUM(CPLE_FileIO));
    rb_define_const(mGdalconst,"CPLE_OpenFailed", INT2NUM(CPLE_OpenFailed));
    rb_define_const(mGdalconst,"CPLE_IllegalArg", INT2NUM(CPLE_IllegalArg));
    rb_define_const(mGdalconst,"CPLE_NotSupported", INT2NUM(CPLE_NotSupported));
    rb_define_const(mGdalconst,"CPLE_AssertionFailed", INT2NUM(CPLE_AssertionFailed));
    rb_define_const(mGdalconst,"CPLE_NoWriteAccess", INT2NUM(CPLE_NoWriteAccess));
    rb_define_const(mGdalconst,"CPLE_UserInterrupt", INT2NUM(CPLE_UserInterrupt));
    rb_define_const(mGdalconst,"DMD_LONGNAME", rb_str_new2("GDAL_DMD_LONGNAME"));
    rb_define_const(mGdalconst,"DMD_HELPTOPIC", rb_str_new2("GDAL_DMD_HELPTOPIC"));
    rb_define_const(mGdalconst,"DMD_MIMETYPE", rb_str_new2("GDAL_DMD_MIMETYPE"));
    rb_define_const(mGdalconst,"DMD_EXTENSION", rb_str_new2("GDAL_DMD_EXTENSION"));
    rb_define_const(mGdalconst,"DMD_CREATIONOPTIONLIST", rb_str_new2("GDAL_DMD_CREATIONOPTIONLIST"));
    rb_define_const(mGdalconst,"DMD_CREATIONDATATYPES", rb_str_new2("GDAL_DMD_CREATIONDATATYPES"));
    rb_define_const(mGdalconst,"DCAP_CREATE", rb_str_new2("GDAL_DCAP_CREATE"));
    rb_define_const(mGdalconst,"DCAP_CREATECOPY", rb_str_new2("GDAL_DCAP_CREATECOPY"));
    rb_define_const(mGdalconst,"CPLES_BackslashQuotable", INT2NUM(CPLES_BackslashQuotable));
    rb_define_const(mGdalconst,"CPLES_XML", INT2NUM(CPLES_XML));
    rb_define_const(mGdalconst,"CPLES_URL", INT2NUM(CPLES_URL));
    rb_define_const(mGdalconst,"CPLES_SQL", INT2NUM(CPLES_SQL));
    rb_define_const(mGdalconst,"CPLES_CSV", INT2NUM(CPLES_CSV));
}

