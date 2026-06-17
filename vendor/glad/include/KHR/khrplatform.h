#ifndef __khrplatform_h_
#define __khrplatform_h_

#ifdef _WIN32
#   ifdef KHRONOS_DLL_IMPORTS
#       define KHRONOS_APICALL __declspec(dllimport)
#   else
#       define KHRONOS_APICALL
#   endif
#   define KHRONOS_APIENTRY  __stdcall
#   define KHRONOS_APIENTRYP KHRONOS_APIENTRY *
    typedef signed   char          khronos_int8_t;
    typedef unsigned char          khronos_uint8_t;
    typedef signed   short int     khronos_int16_t;
    typedef unsigned short int     khronos_uint16_t;
    typedef signed   int           khronos_int32_t;
    typedef unsigned int           khronos_uint32_t;
    typedef signed   long long int khronos_int64_t;
    typedef unsigned long long int khronos_uint64_t;
#   ifdef _WIN64
    typedef signed   long long int khronos_intptr_t;
    typedef unsigned long long int khronos_uintptr_t;
    typedef signed   long long int khronos_ssize_t;
    typedef unsigned long long int khronos_usize_t;
#   else
    typedef signed   long  int     khronos_intptr_t;
    typedef unsigned long  int     khronos_uintptr_t;
    typedef signed   long  int     khronos_ssize_t;
    typedef unsigned long  int     khronos_usize_t;
#   endif
#else
#   include <stdint.h>
#   include <stddef.h>
#   define KHRONOS_APICALL
#   define KHRONOS_APIENTRY
#   define KHRONOS_APIENTRYP *
    typedef int8_t    khronos_int8_t;
    typedef uint8_t   khronos_uint8_t;
    typedef int16_t   khronos_int16_t;
    typedef uint16_t  khronos_uint16_t;
    typedef int32_t   khronos_int32_t;
    typedef uint32_t  khronos_uint32_t;
    typedef int64_t   khronos_int64_t;
    typedef uint64_t  khronos_uint64_t;
    typedef intptr_t  khronos_intptr_t;
    typedef uintptr_t khronos_uintptr_t;
    typedef ptrdiff_t khronos_ssize_t;
    typedef size_t    khronos_usize_t;
#endif

typedef float khronos_float_t;

#endif /* __khrplatform_h_ */
