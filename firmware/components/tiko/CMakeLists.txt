idf_component_register(SRCS "tiko.cpp" "webfs.c" "policy.cpp"
    INCLUDE_DIRS "." "../../.." "../../../src"
    REQUIRES furi gui storage input wifi btshim toolbox esp_wifi esp_http_server
        esp_netif esp_event esp_system esp_timer nvs_flash mbedtls
    EMBED_TXTFILES "../../../sdcard/TikoOS/assets/index.html")
target_compile_features(${COMPONENT_LIB} PRIVATE cxx_std_17)

set_target_properties(${COMPONENT_LIB} PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED YES)
