#ifndef ROBOT_SECURE_COMM_H
#define ROBOT_SECURE_COMM_H

#include <stdint.h>
#include "freertos/FreeRTOS.h"

#define PROTOCOL_VERSION 0x02

#pragma pack(push, 1)
typedef struct {
    uint8_t  version;          
    uint64_t robot_id;         
    uint16_t event_code;       
    uint32_t timestamp;        
    uint32_t payload_data;     
    uint32_t crc32;            
    uint8_t  signature[32]; // 32 байта для HMAC-SHA256 подписи
} robot_sos_packet_t;
#pragma pack(pop)

typedef enum {
    ROBOT_EVENT_OK             = 0x0000,
    ROBOT_EVENT_MECHANICAL_MESS= 0x0001, 
    ROBOT_EVENT_LOW_BATTERY    = 0x0002, 
    ROBOT_EVENT_SENSOR_FAILURE = 0x0003, 
    ROBOT_EVENT_EMERGENCY_STOP = 0x0004  
} robot_event_code_t;

// Функции ядра
int robot_packet_pack(robot_sos_packet_t* packet, uint64_t robot_id, uint16_t event_code, uint32_t timestamp, uint32_t payload);
int robot_packet_unpack(const uint8_t* raw_buffer, size_t buffer_len, robot_sos_packet_t* out_packet);

// Функции безопасности
int robot_packet_validate(const robot_sos_packet_t* packet, const uint8_t* secret_key, size_t key_len);
int robot_packet_sign(robot_sos_packet_t* packet, const uint8_t* secret_key, size_t key_len);

// Транспорт
void transport_ir_init(int uart_num, int tx_pin, int rx_pin, int baud_rate);
int transport_ir_send(int uart_num, const robot_sos_packet_t* packet);
int transport_ir_receive(int uart_num, robot_sos_packet_t* out_packet, TickType_t wait_ticks);
void transport_ble_init(const robot_sos_packet_t* packet_to_broadcast);

#endif
