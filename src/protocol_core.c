#include "robot_secure_comm.h"
#include "esp_rom_crc32.h"
#include <string.h>

// Сборка пакета (для отправки пострадавшим роботом)
int robot_packet_pack(robot_sos_packet_t* packet, uint64_t robot_id, uint16_t event_code, uint32_t timestamp, uint32_t payload) {
    if (packet == NULL) return -1;

    memset(packet, 0, sizeof(robot_sos_packet_t));
    packet->version = PROTOCOL_VERSION;
    packet->robot_id = robot_id;
    packet->event_code = event_code;
    packet->timestamp = timestamp;
    packet->payload_data = payload;

    // Считаем CRC32 от полей до поля crc32
    packet->crc32 = esp_rom_crc32_le(0, (uint8_t*)packet, offsetof(robot_sos_packet_t, crc32));
    
    // Поле signature заполняется отдельно в crypto_verify.c перед отправкой
    return 0;
}

// Первичная обработка пакета на стороне робота-помощника
int robot_packet_unpack(const uint8_t* raw_buffer, size_t buffer_len, robot_sos_packet_t* out_packet) {
    if (raw_buffer == NULL || out_packet == NULL || buffer_len < sizeof(robot_sos_packet_t)) {
        return -1; 
    }

    // Извлекаем структуру из сырого буфера связи
    memcpy(out_packet, raw_buffer, sizeof(robot_sos_packet_t));
    return 0;
}
