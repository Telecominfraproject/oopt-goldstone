/**************************************************************************//**
 *
 * Simple SAI Sanity Test
 *
 *****************************************************************************/
#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <sai.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string.h>
#include <sys/select.h>

// sai functions
const char *test_profile_get_value(_In_ sai_switch_profile_id_t profile_id,
                                   _In_ const char *variable) {
  return NULL;
}
int test_profile_get_next_value(_In_ sai_switch_profile_id_t profile_id,
                                _Out_ const char **variable,
                                _Out_ const char **value) {
  return -1;
}
const sai_service_method_table_t test_services = {test_profile_get_value,
                                              test_profile_get_next_value};

int
main(int argc, char* argv)
{
    // sai init ports
    sai_hostif_api_t *hostif_api = NULL;
    sai_switch_api_t *switch_api = NULL;

#define TRY(_expr)                              \
    do {                                        \
        int _rv = _expr;                        \
        printf("%s = %d\n", #_expr, _rv);       \
    } while(0)

    TRY(sai_api_initialize(0, &test_services));
    TRY(sai_api_query(SAI_API_SWITCH, (void **)&switch_api));

    sai_object_id_t switch_id;
    sai_attribute_t attrs[] =
        {
            {
                .id = SAI_SWITCH_ATTR_INIT_SWITCH,
                .value.booldata = 1,
            },
        };

    TRY(switch_api->create_switch(&switch_id, 1, attrs));
    return 0;
}
