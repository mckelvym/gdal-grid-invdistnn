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

/*
 * php4.swg
 *
 * PHP4 runtime library
 *
 */

#ifdef __cplusplus
extern "C" {
#endif
#include "zend.h"
#include "zend_API.h"
#include "php.h"

/* These TSRMLS_ stuff should already be defined now, but with older php under
   redhat are not... */
#ifndef TSRMLS_D
#define TSRMLS_D
#endif
#ifndef TSRMLS_DC
#define TSRMLS_DC
#endif
#ifndef TSRMLS_C
#define TSRMLS_C
#endif
#ifndef TSRMLS_CC
#define TSRMLS_CC
#endif

#ifdef __cplusplus
}
#endif

#define SWIG_fail goto fail

static char *default_error_msg = "Unknown error occurred";
static int default_error_code = E_ERROR;

#define SWIG_PHP_Arg_Error_Msg(argnum,extramsg) "Error in argument " #argnum " "#extramsg

#define SWIG_PHP_Error(code,msg) ErrorCode() = code; ErrorMsg() = msg; SWIG_fail;

#define SWIG_contract_assert(expr,msg) \
  if (!(expr) ) { zend_printf("Contract Assert Failed %s\n",msg ); } else

/* Standard SWIG API */
#define SWIG_GetModule(clientdata) SWIG_Php4_GetModule()
#define SWIG_SetModule(clientdata, pointer) SWIG_Php4_SetModule(pointer)

/* used to wrap returned objects in so we know whether they are newobject
   and need freeing, or not */
typedef struct _swig_object_wrapper {
  void * ptr;
  int newobject;
} swig_object_wrapper;

/* empty zend destructor for types without one */
static ZEND_RSRC_DTOR_FUNC(SWIG_landfill) {};

#define SWIG_SetPointerZval(a,b,c,d) SWIG_ZTS_SetPointerZval(a,b,c,d, SWIG_module_entry TSRMLS_CC)

static void
SWIG_ZTS_SetPointerZval(zval *z, void *ptr, swig_type_info *type, int newobject, zend_module_entry* module_entry TSRMLS_DC) {
  swig_object_wrapper *value=NULL;
  /*
   * First test for Null pointers.  Return those as PHP native NULL
   */
  if (!ptr ) {
    ZVAL_NULL(z);
    return;
  }
  if (type->clientdata) {
    if (! (*(int *)(type->clientdata)))
      zend_error(E_ERROR, "Type: %s failed to register with zend",type->name);
    value=(swig_object_wrapper *)emalloc(sizeof(swig_object_wrapper));
    value->ptr=ptr;
    value->newobject=newobject;
    ZEND_REGISTER_RESOURCE(z, value, *(int *)(type->clientdata));
    return;
  } else { /* have to deal with old fashioned string pointer?
              but this should not get this far */
    zend_error(E_ERROR, "Type: %s not registered with zend",type->name);
  }
}

/* This old-style routine converts an old string-pointer c into a real pointer
   ptr calling making appropriate casting functions according to ty
   We don't use this any more */
static int
SWIG_ConvertPtr_(char *c, void **ptr, swig_type_info *ty) {
   register int d;
   unsigned long p;
   swig_cast_info *tc;

   if(c == NULL) {
   	*ptr = 0;
	return 0;
   }

   p = 0;
   if (*c != '_') {
    *ptr = (void *) 0;
    if (strcmp(c,"NULL") == 0) {
	return 0;
    } else {
	goto type_error;
    }
  }

    c++;
    /* Extract hex value from pointer */
    while ((d = *c)) {
      if ((d >= '0') && (d <= '9'))
        p = (p << 4) + (d - '0');
      else if ((d >= 'a') && (d <= 'f'))
        p = (p << 4) + (d - ('a'-10));
      else
        break;
      c++;
    }
    *ptr = (void *) p;
	
    if(ty) {
	tc = SWIG_TypeCheck(c,ty);
	if(!tc) goto type_error;
	*ptr = SWIG_TypeCast(tc, (void*)p);
    }
    return 0;

type_error:

    return -1;
}

/* This is a new pointer conversion routine
   Taking the native pointer p (which would have been converted from the old
   string pointer) and it's php type id, and it's type name (which also would
   have come from the old string pointer) it converts it to ptr calling 
   appropriate casting functions according to ty
   Sadly PHP has no API to find a type name from a type id, only from an instance
   of a resource of the type id, so we have to pass type_name as well.
   The two functions which might call this are:
   SWIG_ZTS_ConvertResourcePtr which gets the type name from the resource
   and the registered zend destructors for which we have one per type each
   with the type name hard wired in. */
static int
SWIG_ZTS_ConvertResourceData(void * p, int type, const char *type_name, void **ptr, swig_type_info *ty TSRMLS_DC) {
  swig_cast_info *tc;

  if (ty) {
    if (! type_name) {  
      /* can't convert p to ptr type ty if we don't know what type p is */
      return -1;
    } else {
      /* convert and cast p from type_name to ptr as ty
         Need to sort out const-ness, can SWIG_TypeCast really not take a const? */
      tc = SWIG_TypeCheck((char *)type_name,ty);
      if (!tc) return -1;
      *ptr = SWIG_TypeCast(tc, (void*)p);
    }
  } else {
    /* They don't care about the target type, so just pass on the pointer! */
    *ptr = (void *) p;
  }
  return 0;
}

/* This function fills ptr with a pointer of type ty by extracting the pointer
   and type info from the resource in z.  z must be a resource
   It uses SWIG_ZTS_ConvertResourceData to do the real work. */
static int
SWIG_ZTS_ConvertResourcePtr(zval *z, void **ptr, swig_type_info *ty TSRMLS_DC) {
  swig_object_wrapper *value;
  void *p;
  int type;
  char *type_name;

  value = (swig_object_wrapper *) zend_list_find(z->value.lval,&type);
  p = value->ptr;
  if (type==-1) return -1;

  type_name=zend_rsrc_list_get_rsrc_type(z->value.lval TSRMLS_CC);

  return SWIG_ZTS_ConvertResourceData(p,type,type_name,ptr,ty TSRMLS_CC);
}

/* But in fact SWIG_ConvertPtr is the native interface for getting typed
   pointer values out of zvals.  We need the TSRMLS_ macros for when we
   make PHP type calls later as we handle php resources */
#define SWIG_ConvertPtr(a,b,c) SWIG_ZTS_ConvertPtr(a,b,c TSRMLS_CC)

/* We allow passing of a STRING or RESOURCE pointing to the object
   or an OBJECT whose _cPtr is a string or resource pointing to the object
   STRING pointers are very depracated */
static int
SWIG_ZTS_ConvertPtr(zval *z, void **ptr, swig_type_info *ty TSRMLS_DC) {
   char *c;
   zval *val;
   
   if(z == NULL) {
	*ptr = 0;
	return 0;
   }

   if (z->type==IS_OBJECT) {
     zval ** _cPtr;
     if (zend_hash_find(HASH_OF(z),"_cPtr",sizeof("_cPtr"),(void**)&_cPtr)==SUCCESS) {
       /* Don't co-erce to string if it isn't */
       if ((*_cPtr)->type==IS_STRING) c = Z_STRVAL_PP(_cPtr);
       else if ((*_cPtr)->type==IS_RESOURCE) {
         return SWIG_ZTS_ConvertResourcePtr(*_cPtr,ptr,ty TSRMLS_CC);
       } else goto type_error; /* _cPtr was not string or resource property */
     } else goto type_error; /* can't find property _cPtr */
   } else if (z->type==IS_RESOURCE) {
     return SWIG_ZTS_ConvertResourcePtr(z,ptr,ty TSRMLS_CC);
   } else if (z->type==IS_STRING) {
     c = Z_STRVAL_P(z); 
     return SWIG_ConvertPtr_(c,ptr,ty);
   } else goto type_error;

type_error:

    return -1;
}

static char const_name[] = "swig_runtime_data_type_pointer";
static swig_module_info *SWIG_Php4_GetModule() {
  zval *pointer;
  swig_module_info *ret = 0;

  MAKE_STD_ZVAL(pointer);

  if (zend_get_constant(const_name, sizeof(const_name), pointer)) {
    if (pointer->type == IS_LONG) {
      ret = (swig_module_info *) pointer->value.lval;
    }
  } 
  return 0;
}

static void SWIG_Php4_SetModule(swig_module_info *pointer) {
  REGISTER_MAIN_LONG_CONSTANT(const_name, (long) pointer, 0);
}


/* -------- TYPES TABLE (BEGIN) -------- */

#define SWIGTYPE_int swig_types[0]
#define SWIGTYPE_p_char swig_types[1]
static swig_type_info *swig_types[2];
static swig_module_info swig_module = {swig_types, 2, 0, 0, 0, 0};
#define SWIG_TypeQuery(name) SWIG_TypeQueryModule(&swig_module, &swig_module, name)
#define SWIG_MangledTypeQuery(name) SWIG_MangledTypeQueryModule(&swig_module, &swig_module, name)

/* -------- TYPES TABLE (END) -------- */

/* header section */
/*
  +----------------------------------------------------------------------+
  | PHP version 4.0                                                      |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997, 1998, 1999, 2000, 2001 The PHP Group             |
  +----------------------------------------------------------------------+
  | This source file is subject to version 2.02 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available at through the world-wide-web at                           |
  | http://www.php.net/license/2_02.txt.                                 |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Authors:                                                             |
  |                                                                      |
  +----------------------------------------------------------------------+
 */
ZEND_BEGIN_MODULE_GLOBALS(gdalconst)
char *error_msg;
int error_code;
ZEND_END_MODULE_GLOBALS(gdalconst)
ZEND_DECLARE_MODULE_GLOBALS(gdalconst)
#ifdef ZTS
#define ErrorMsg() TSRMG(gdalconst_globals_id, zend_gdalconst_globals *, error_msg );
#define ErrorCode() TSRMG(gdalconst_globals_id, zend_gdalconst_globals *, error_code );
#else
#define ErrorMsg() (gdalconst_globals.error_msg)
#define ErrorCode() (gdalconst_globals.error_code)
#endif

static void gdalconst_init_globals(zend_gdalconst_globals *gdalconst_globals ) {
  gdalconst_globals->error_msg = default_error_msg;
  gdalconst_globals->error_code = default_error_code;
}
static void gdalconst_destroy_globals(zend_gdalconst_globals *gdalconst_globals) { }

void SWIG_ResetError() {
  ErrorMsg() = default_error_msg;
  ErrorCode() = default_error_code;
}
#define SWIG_name	"gdalconst"
#ifdef __cplusplus
extern "C" {
#endif
#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_gdalconst.h"
#ifdef __cplusplus
}
#endif


#include "gdal.h"
#include "gdalwarper.h"
#include "cpl_string.h"
#include "cpl_minixml.h"

/* class entry subsection */


/* entry subsection */
/* Every non-class user visible function must have an entry here */
function_entry gdalconst_functions[] = {
 {NULL, NULL, NULL}
};

zend_module_entry gdalconst_module_entry = {
#if ZEND_MODULE_API_NO > 20010900
    STANDARD_MODULE_HEADER,
#endif
    "gdalconst",
    gdalconst_functions,
    PHP_MINIT(gdalconst),
    PHP_MSHUTDOWN(gdalconst),
    PHP_RINIT(gdalconst),
    PHP_RSHUTDOWN(gdalconst),
    PHP_MINFO(gdalconst),
#if ZEND_MODULE_API_NO > 20010900
    NO_VERSION_YET,
#endif
    STANDARD_MODULE_PROPERTIES
};
zend_module_entry* SWIG_module_entry = &gdalconst_module_entry;


/* -------- TYPE CONVERSION AND EQUIVALENCE RULES (BEGIN) -------- */

static swig_type_info _swigt__int = {"_int", "int", 0, 0, 0};
static swig_type_info _swigt__p_char = {"_p_char", "char *", 0, 0, 0};

static swig_type_info *swig_type_initial[] = {
  &_swigt__int,
  &_swigt__p_char,
};

static swig_cast_info _swigc__int[] = {  {&_swigt__int, 0, 0, 0},{0, 0, 0, 0}};
static swig_cast_info _swigc__p_char[] = {  {&_swigt__p_char, 0, 0, 0},{0, 0, 0, 0}};

static swig_cast_info *swig_cast_initial[] = {
  _swigc__int,
  _swigc__p_char,
};


/* -------- TYPE CONVERSION AND EQUIVALENCE RULES (END) -------- */

/* vdecl subsection */
static int le_swig__int=0; /* handle for  */
static int le_swig__p_char=0; /* handle for  */
/* end vdecl subsection */
/* wrapper section */
/* NEW Destructor style */
static ZEND_RSRC_DTOR_FUNC(_wrap_destroy_int) {
  /* bah! No destructor for this simple type!! */
}
/* NEW Destructor style */
static ZEND_RSRC_DTOR_FUNC(_wrap_destroy_p_char) {
  /* bah! No destructor for this simple type!! */
}
/* end wrapper section */
/* init section */
#ifdef __cplusplus
extern "C" {
#endif
ZEND_GET_MODULE(gdalconst)
#ifdef __cplusplus
}
#endif

PHP_MSHUTDOWN_FUNCTION(gdalconst)
{
    return SUCCESS;
}
#define SWIG_php_minit PHP_MINIT_FUNCTION(gdalconst)
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


  SWIG_php_minit {
    SWIG_InitializeModule(0);

/* oinit subsection */
ZEND_INIT_MODULE_GLOBALS(gdalconst, gdalconst_init_globals, gdalconst_destroy_globals);

/* Register resource destructors for pointer types */
le_swig__int=zend_register_list_destructors_ex(_wrap_destroy_int,NULL,(char *)(SWIGTYPE_int->name),module_number);
SWIG_TypeClientData(SWIGTYPE_int,&le_swig__int);
le_swig__p_char=zend_register_list_destructors_ex(_wrap_destroy_p_char,NULL,(char *)(SWIGTYPE_p_char->name),module_number);
SWIG_TypeClientData(SWIGTYPE_p_char,&le_swig__p_char);
CG(active_class_entry) = NULL;
/* end oinit subsection */

    return SUCCESS;
}
PHP_RINIT_FUNCTION(gdalconst)
{
/* cinit subsection */
REGISTER_LONG_CONSTANT( "GDT_Unknown", GDT_Unknown, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT( "GDT_Byte", GDT_Byte, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT( "GDT_UInt16", GDT_UInt16, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT( "GDT_Int16", GDT_Int16, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT( "GDT_UInt32", GDT_UInt32, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT( "GDT_Int32", GDT_Int32, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT( "GDT_Float32", GDT_Float32, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT( "GDT_Float64", GDT_Float64, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT( "GDT_CInt16", GDT_CInt16, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT( "GDT_CInt32", GDT_CInt32, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT( "GDT_CFloat32", GDT_CFloat32, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT( "GDT_CFloat64", GDT_CFloat64, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT( "GDT_TypeCount", GDT_TypeCount, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT( "GA_ReadOnly", GA_ReadOnly, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT( "GA_Update", GA_Update, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT( "GF_Read", GF_Read, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT( "GF_Write", GF_Write, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT( "GCI_Undefined", GCI_Undefined, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT( "GCI_GrayIndex", GCI_GrayIndex, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT( "GCI_PaletteIndex", GCI_PaletteIndex, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT( "GCI_RedBand", GCI_RedBand, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT( "GCI_GreenBand", GCI_GreenBand, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT( "GCI_BlueBand", GCI_BlueBand, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT( "GCI_AlphaBand", GCI_AlphaBand, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT( "GCI_HueBand", GCI_HueBand, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT( "GCI_SaturationBand", GCI_SaturationBand, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT( "GCI_LightnessBand", GCI_LightnessBand, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT( "GCI_CyanBand", GCI_CyanBand, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT( "GCI_MagentaBand", GCI_MagentaBand, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT( "GCI_YellowBand", GCI_YellowBand, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT( "GCI_BlackBand", GCI_BlackBand, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT( "GRA_NearestNeighbour", GRA_NearestNeighbour, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT( "GRA_Bilinear", GRA_Bilinear, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT( "GRA_Cubic", GRA_Cubic, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT( "GRA_CubicSpline", GRA_CubicSpline, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT( "GPI_Gray", GPI_Gray, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT( "GPI_RGB", GPI_RGB, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT( "GPI_CMYK", GPI_CMYK, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT( "GPI_HLS", GPI_HLS, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT( "CXT_Element", CXT_Element, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT( "CXT_Text", CXT_Text, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT( "CXT_Attribute", CXT_Attribute, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT( "CXT_Comment", CXT_Comment, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT( "CXT_Literal", CXT_Literal, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT( "CE_None", CE_None, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT( "CE_Debug", CE_Debug, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT( "CE_Warning", CE_Warning, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT( "CE_Failure", CE_Failure, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT( "CE_Fatal", CE_Fatal, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT( "CPLE_None", CPLE_None, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT( "CPLE_AppDefined", CPLE_AppDefined, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT( "CPLE_OutOfMemory", CPLE_OutOfMemory, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT( "CPLE_FileIO", CPLE_FileIO, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT( "CPLE_OpenFailed", CPLE_OpenFailed, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT( "CPLE_IllegalArg", CPLE_IllegalArg, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT( "CPLE_NotSupported", CPLE_NotSupported, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT( "CPLE_AssertionFailed", CPLE_AssertionFailed, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT( "CPLE_NoWriteAccess", CPLE_NoWriteAccess, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT( "CPLE_UserInterrupt", CPLE_UserInterrupt, CONST_CS | CONST_PERSISTENT);
REGISTER_STRING_CONSTANT("DMD_LONGNAME", GDAL_DMD_LONGNAME, CONST_CS | CONST_PERSISTENT);
REGISTER_STRING_CONSTANT("DMD_HELPTOPIC", GDAL_DMD_HELPTOPIC, CONST_CS | CONST_PERSISTENT);
REGISTER_STRING_CONSTANT("DMD_MIMETYPE", GDAL_DMD_MIMETYPE, CONST_CS | CONST_PERSISTENT);
REGISTER_STRING_CONSTANT("DMD_EXTENSION", GDAL_DMD_EXTENSION, CONST_CS | CONST_PERSISTENT);
REGISTER_STRING_CONSTANT("DMD_CREATIONOPTIONLIST", GDAL_DMD_CREATIONOPTIONLIST, CONST_CS | CONST_PERSISTENT);
REGISTER_STRING_CONSTANT("DMD_CREATIONDATATYPES", GDAL_DMD_CREATIONDATATYPES, CONST_CS | CONST_PERSISTENT);
REGISTER_STRING_CONSTANT("DCAP_CREATE", GDAL_DCAP_CREATE, CONST_CS | CONST_PERSISTENT);
REGISTER_STRING_CONSTANT("DCAP_CREATECOPY", GDAL_DCAP_CREATECOPY, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT( "CPLES_BackslashQuotable", CPLES_BackslashQuotable, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT( "CPLES_XML", CPLES_XML, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT( "CPLES_URL", CPLES_URL, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT( "CPLES_SQL", CPLES_SQL, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT( "CPLES_CSV", CPLES_CSV, CONST_CS | CONST_PERSISTENT);
/* end cinit subsection */

/* vinit subsection */
/* end vinit subsection */

    return SUCCESS;
}
PHP_RSHUTDOWN_FUNCTION(gdalconst)
{
    return SUCCESS;
}
PHP_MINFO_FUNCTION(gdalconst)
{
}
/* end init section */
