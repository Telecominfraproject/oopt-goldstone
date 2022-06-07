/**************************************************************************//**
 *
 *
 *
 *****************************************************************************/
#include <arm64_wistron_wtp_01_c1_00/arm64_wistron_wtp_01_c1_00_config.h>

#include "arm64_wistron_wtp_01_c1_00_log.h"

static int
datatypes_init__(void)
{
#define ARM64_WISTRON_WTP_01_C1_00_ENUMERATION_ENTRY(_enum_name, _desc)     AIM_DATATYPE_MAP_REGISTER(_enum_name, _enum_name##_map, _desc,                               AIM_LOG_INTERNAL);
#include <arm64_wistron_wtp_01_c1_00/arm64_wistron_wtp_01_c1_00.x>
    return 0;
}

void __arm64_wistron_wtp_01_c1_00_module_init__(void)
{
    AIM_LOG_STRUCT_REGISTER();
    datatypes_init__();
}

int __onlp_platform_version__ = 1;
