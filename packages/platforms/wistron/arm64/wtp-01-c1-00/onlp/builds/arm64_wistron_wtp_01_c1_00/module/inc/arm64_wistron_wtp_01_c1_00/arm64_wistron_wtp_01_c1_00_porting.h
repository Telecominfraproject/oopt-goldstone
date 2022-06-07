/**************************************************************************//**
 *
 * @file
 * @brief arm64_wistron_wtp_01_c1_00 Porting Macros.
 *
 * @addtogroup arm64_wistron_wtp_01_c1_00-porting
 * @{
 *
 *****************************************************************************/
#ifndef __ARM64_WISTRON_WTP_01_C1_00_PORTING_H__
#define __ARM64_WISTRON_WTP_01_C1_00_PORTING_H__


/* <auto.start.portingmacro(ALL).define> */
#if ARM64_WISTRON_WTP_01_C1_00_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS == 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <memory.h>
#endif

#ifndef ARM64_WISTRON_WTP_01_C1_00_MALLOC
    #if defined(GLOBAL_MALLOC)
        #define ARM64_WISTRON_WTP_01_C1_00_MALLOC GLOBAL_MALLOC
    #elif ARM64_WISTRON_WTP_01_C1_00_CONFIG_PORTING_STDLIB == 1
        #define ARM64_WISTRON_WTP_01_C1_00_MALLOC malloc
    #else
        #error The macro ARM64_WISTRON_WTP_01_C1_00_MALLOC is required but cannot be defined.
    #endif
#endif

#ifndef ARM64_WISTRON_WTP_01_C1_00_FREE
    #if defined(GLOBAL_FREE)
        #define ARM64_WISTRON_WTP_01_C1_00_FREE GLOBAL_FREE
    #elif ARM64_WISTRON_WTP_01_C1_00_CONFIG_PORTING_STDLIB == 1
        #define ARM64_WISTRON_WTP_01_C1_00_FREE free
    #else
        #error The macro ARM64_WISTRON_WTP_01_C1_00_FREE is required but cannot be defined.
    #endif
#endif

#ifndef ARM64_WISTRON_WTP_01_C1_00_MEMSET
    #if defined(GLOBAL_MEMSET)
        #define ARM64_WISTRON_WTP_01_C1_00_MEMSET GLOBAL_MEMSET
    #elif ARM64_WISTRON_WTP_01_C1_00_CONFIG_PORTING_STDLIB == 1
        #define ARM64_WISTRON_WTP_01_C1_00_MEMSET memset
    #else
        #error The macro ARM64_WISTRON_WTP_01_C1_00_MEMSET is required but cannot be defined.
    #endif
#endif

#ifndef ARM64_WISTRON_WTP_01_C1_00_MEMCPY
    #if defined(GLOBAL_MEMCPY)
        #define ARM64_WISTRON_WTP_01_C1_00_MEMCPY GLOBAL_MEMCPY
    #elif ARM64_WISTRON_WTP_01_C1_00_CONFIG_PORTING_STDLIB == 1
        #define ARM64_WISTRON_WTP_01_C1_00_MEMCPY memcpy
    #else
        #error The macro ARM64_WISTRON_WTP_01_C1_00_MEMCPY is required but cannot be defined.
    #endif
#endif

#ifndef ARM64_WISTRON_WTP_01_C1_00_STRNCPY
    #if defined(GLOBAL_STRNCPY)
        #define ARM64_WISTRON_WTP_01_C1_00_STRNCPY GLOBAL_STRNCPY
    #elif ARM64_WISTRON_WTP_01_C1_00_CONFIG_PORTING_STDLIB == 1
        #define ARM64_WISTRON_WTP_01_C1_00_STRNCPY strncpy
    #else
        #error The macro ARM64_WISTRON_WTP_01_C1_00_STRNCPY is required but cannot be defined.
    #endif
#endif

#ifndef ARM64_WISTRON_WTP_01_C1_00_VSNPRINTF
    #if defined(GLOBAL_VSNPRINTF)
        #define ARM64_WISTRON_WTP_01_C1_00_VSNPRINTF GLOBAL_VSNPRINTF
    #elif ARM64_WISTRON_WTP_01_C1_00_CONFIG_PORTING_STDLIB == 1
        #define ARM64_WISTRON_WTP_01_C1_00_VSNPRINTF vsnprintf
    #else
        #error The macro ARM64_WISTRON_WTP_01_C1_00_VSNPRINTF is required but cannot be defined.
    #endif
#endif

#ifndef ARM64_WISTRON_WTP_01_C1_00_SNPRINTF
    #if defined(GLOBAL_SNPRINTF)
        #define ARM64_WISTRON_WTP_01_C1_00_SNPRINTF GLOBAL_SNPRINTF
    #elif ARM64_WISTRON_WTP_01_C1_00_CONFIG_PORTING_STDLIB == 1
        #define ARM64_WISTRON_WTP_01_C1_00_SNPRINTF snprintf
    #else
        #error The macro ARM64_WISTRON_WTP_01_C1_00_SNPRINTF is required but cannot be defined.
    #endif
#endif

#ifndef ARM64_WISTRON_WTP_01_C1_00_STRLEN
    #if defined(GLOBAL_STRLEN)
        #define ARM64_WISTRON_WTP_01_C1_00_STRLEN GLOBAL_STRLEN
    #elif ARM64_WISTRON_WTP_01_C1_00_CONFIG_PORTING_STDLIB == 1
        #define ARM64_WISTRON_WTP_01_C1_00_STRLEN strlen
    #else
        #error The macro ARM64_WISTRON_WTP_01_C1_00_STRLEN is required but cannot be defined.
    #endif
#endif

/* <auto.end.portingmacro(ALL).define> */


#endif /* __ARM64_WISTRON_WTP_01_C1_00_PORTING_H__ */
/* @} */
