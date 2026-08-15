#include "robot_secure_comm.h"
#include "driver/uart.h"
#include "esp_log.h"

static const char* TAG = "SECURE_IR";
#define BUF_SIZE (1024)

void transport_ir_init(int uart_num, int tx_pin, int rx_pin, int baud_rate) {
    uart_config_t uart_config = {
        .baud_rate = baud_rate,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    ESP_ERROR_CHECK(uart_driver_install(uart_num, BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(uart_num, tx_pin, rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_LOGI(TAG, "ИК-транспорт успешно запущен на UART %d", uart_num);
}

int transport_ir_send(int uart_num, const robot_sos_packet_t* packet) {
    return uart_write_bytes(uart_num, (const char*)packet, sizeof(robot_sos_packet_t));
}

int transport_ir_receive(int uart_num, robot_sos_packet_t* out_packet, TickType_t wait_ticks) {
    uint8_t buffer[sizeof(robot_sos_packet_t)];
    int len = uart_read_bytes(uart_num, buffer, sizeof(robot_sos_packet_t), wait_ticks);
    if (len == sizeof(robot_sos_packet_t)) {
        return robot_packet_unpack(buffer, len, out_packet);
    }
    return -1; // Пакет не получен или получен не полностью
}
