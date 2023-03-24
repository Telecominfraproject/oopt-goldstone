#!/usr/bin/env python3

"""
    swssconfig_xcvrd.py

    Execute the operation equivalent to swssconfig which sets 
    PORT field/value pairs in CONFIG_DB to PORT_TABLE in APPL_DB.
"""

import swsssdk

PORT_TABLE_FIELD_LIST = ["speed", "index", "lanes"]


# connect redis db
config_db = swsssdk.ConfigDBConnector()
appl_db = swsssdk.SonicV2Connector()
config_db.connect()
appl_db.connect("APPL_DB")

port_list = config_db.keys("CONFIG_DB", "PORT|*")

for port in port_list:
    fv_dic = config_db.get_all("CONFIG_DB", port)

    # set PORT_TABLE to APPL_DB
    if_name = port.lstrip("PORT|")
    for key in PORT_TABLE_FIELD_LIST:
        appl_db.set("APPL_DB", f"PORT_TABLE:{if_name}", key, fv_dic[key])

    print(
        f"SET PORT_TABLE:{if_name}=",
        appl_db.get_all("APPL_DB", f"PORT_TABLE:{if_name}"),
    )


config_db.close("CONFIG_DB")
appl_db.close("APPL_DB")
