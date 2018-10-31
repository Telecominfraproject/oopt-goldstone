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

static const char CONFIG_FILE[] = "SAI_INIT_CONFIG_FILE";
static const char CONFIG_FILE_PATH[] = "config.bcm";
static int g_profile_index;

// sai functions
const char *test_profile_get_value(_In_ sai_switch_profile_id_t profile_id,
                                   _In_ const char *variable) {
  return NULL;
}
int test_profile_get_next_value(_In_ sai_switch_profile_id_t profile_id,
                                _Out_ const char **variable,
                                _Out_ const char **value) {
  if ( g_profile_index > 0 ) {
      g_profile_index = 0;
      return -1;
  }
  if( value != NULL ) {
      *variable = CONFIG_FILE;
      *value = CONFIG_FILE_PATH;
      g_profile_index = 1;
      return 0;
  }
  return -1;
}
const sai_service_method_table_t test_services = {test_profile_get_value,
                                              test_profile_get_next_value};

int
main(int argc, char* argv)
{
    g_profile_index = 0;
    // sai init ports
    sai_hostif_api_t *hostif_api = NULL;
    sai_switch_api_t *switch_api = NULL;
    sai_port_api_t   *port_api   = NULL;

#define TRY(_expr)                              \
    do {                                        \
        int _rv = _expr;                        \
        printf("%s = %d\n", #_expr, _rv);       \
    } while(0)

    TRY(sai_api_initialize(0, &test_services));
    TRY(sai_api_query(SAI_API_HOSTIF, (void **)&hostif_api));
    TRY(sai_api_query(SAI_API_SWITCH, (void **)&switch_api));
    TRY(sai_api_query(SAI_API_PORT,   (void **)&port_api));

    sai_log_set(SAI_API_HOSTIF, SAI_LOG_LEVEL_DEBUG);

    sai_mac_t mac = {0xa8, 0x2b, 0xb5, 0x15, 0x26, 0x08};

    sai_object_id_t switch_id;
    sai_attribute_t attrs[] =
        {
            {
                .id = SAI_SWITCH_ATTR_INIT_SWITCH,
                .value.booldata = 1,
            },
            {
                .id = SAI_SWITCH_ATTR_SRC_MAC_ADDRESS,
                .value.mac = mac,
            },
        };

    TRY(switch_api->create_switch(&switch_id, 1, attrs));

    attrs[0].id = SAI_SWITCH_ATTR_NUMBER_OF_ACTIVE_PORTS;
    TRY(switch_api->get_switch_attribute(switch_id, 1, attrs));

    uint32_t num_port = attrs[0].value.u32;

    printf("num port: %d\n", num_port);

    sai_object_id_t l[128];

    memset(l, 0, sizeof(sai_object_id_t) * 128);

    sai_object_list_t objlist = {
        .count = 128,
        .list = l,
    };

    attrs[0].id = SAI_SWITCH_ATTR_PORT_LIST;
    attrs[0].value.objlist = objlist;

    TRY(switch_api->get_switch_attribute(switch_id, 1, attrs));

    for ( i = 0; i < num_port; i++ ) {
        printf("%d: %lx\n", i, l[i]);
    }

    sai_object_id_t hostif_id;
    sai_attribute_t hostif_attrs[] =
        {
            {
                .id = SAI_HOSTIF_ATTR_TYPE,
                .value.s32 = SAI_HOSTIF_TYPE_NETDEV,
            },
            {
                .id = SAI_HOSTIF_ATTR_OBJ_ID,
                .value.oid = l[0],
            },
            {
                .id = SAI_HOSTIF_ATTR_NAME,
                .value.chardata = "Ethernet1",
            },
        };

    TRY(hostif_api->create_hostif(&hostif_id, switch_id, 3, hostif_attrs));
    return 0;
}
