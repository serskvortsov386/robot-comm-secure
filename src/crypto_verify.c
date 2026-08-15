#include "robot_secure_comm.h"
#include "esp_rom_crc32.h"
#include "mbedtls/md.h"
#include <string.h>

int robot_packet_validate(const robot_sos_packet_t* packet, const uint8_t* secret_key, size_t key_len) {
    if (packet == NULL || secret_key == NULL) return -3;

    // Уровень 1: Проверка версии протокола
    if (packet->version != PROTOCOL_VERSION) return -1;

    // Уровень 2: Проверка контрольной суммы (Защита от физических помех в эфире)
    uint32_t calculated_crc = esp_rom_crc32_le(0, (uint8_t*)packet, offsetof(robot_sos_packet_t, crc32));
    if (packet->crc32 != calculated_crc) return -2;

    // Уровень 3: Криптографическая проверка подлинности реестра (HMAC-SHA256)
    uint8_t calculated_hmac[32];
    mbedtls_md_context_t ctx;
    mbedtls_md_init(&ctx);
    
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1);
    mbedtls_md_hmac_starts(&ctx, secret_key, key_len);
    // Подписываем всё тело пакета, исключая само поле подписи
    mbedtls_md_hmac_update(&ctx, (uint8_t*)packet, offsetof(robot_sos_packet_t, signature));
    mbedtls_md_hmac_finish(&ctx, calculated_hmac);
    mbedtls_md_free(&ctx);

    // Сравниваем первые 32 байта поля signature с расчитанным HMAC
    if (memcmp(packet->signature, calculated_hmac, 32) != 0) {
        return -4; // Защита сработала: пакет подделан или отправлен несертифицированным устройством
    }

    return 0; // Пакет полностью безопасен
}

int robot_packet_sign(robot_sos_packet_t* packet, const uint8_t* secret_key, size_t key_len) {
    if (packet == NULL || secret_key == NULL) return -1;

    mbedtls_md_context_t ctx;
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1);
    mbedtls_md_hmac_starts(&ctx, secret_key, key_len);
    mbedtls_md_hmac_update(&ctx, (uint8_t*)packet, offsetof(robot_sos_packet_t, signature));
    mbedtls_md_hmac_finish(&ctx, packet->signature); // Записываем хэш в пакет
    mbedtls_md_free(&ctx);

    return 0;
}
