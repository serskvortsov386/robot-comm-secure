#include "robot_secure_comm.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "ROBOT_MAIN";

// Секретный ключ реестра, выданный при сертификации робота (32 байта)
// Вместо старого вызова:
// static const uint8_t REGISTRY_SECRET_KEY[32] = "ROBOT_REGISTRY_TRUST_TOKEN_2026";

// Напишите новый вызов:
robot_packet_sign(&my_sos_packet, (const uint8_t*)CONFIG_ROBOT_MASTER_KEY, strlen(CONFIG_ROBOT_MASTER_KEY));


void app_main(void) {
    // 1. Инициализация памяти NVS (требуется для работы Bluetooth)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "Запуск системы безопасности робота...");

    // 2. Симуляция аварии: Робот застрял колесом
    robot_sos_packet_t my_sos_packet;
    uint64_t my_robot_id = 987654321; // Уникальный ID в реестре
    uint32_t current_time = 1771122334; // Текущий Timestamp

    robot_packet_pack(&my_sos_packet, my_robot_id, ROBOT_EVENT_MECHANICAL_MESS, current_time, 0);
    
    // Криптуем пакет подписью Реестра
    robot_packet_sign(&my_sos_packet, REGISTRY_SECRET_KEY, sizeof(REGISTRY_SECRET_KEY));
    ESP_LOGI(TAG, "Безопасный SOS пакет успешно сформирован и подписан!");

    // 3. Инициализация аппаратных каналов связи
    // ИК-Порт на UART1 (Пины TX: 17, RX: 16)
    transport_ir_init(1, 17, 16, 115200);

    // Отправляем SOS по ИК-каналу
    transport_ir_send(1, &my_sos_packet);
    ESP_LOGI(TAG, "Сигнал помощи отправлен через ИК-порт.");

    // 4. Запускаем постоянный крик о помощи по Bluetooth BLE
    transport_ble_init(&my_sos_packet);
    ESP_LOGI(TAG, "Включен режим BLE-маяка. Робот ожидает помощь.");

    // Фоновый цикл работы
    while(1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
