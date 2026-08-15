idf_component_register(SRCS "src/protocol_core.c"
                            "src/crypto_verify.c"
                            "src/transport_ir.c"
                            "src/transport_ble.c"
                       INCLUDE_DIRS "include"
                       PRIV_INCLUDE_DIRS "src"
                       REQUIRES driver mbedtls nimble)
