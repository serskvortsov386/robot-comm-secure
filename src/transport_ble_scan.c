#include "robot_secure_comm.h"
#include "esp_log.h"
#include "nimble/nimble_port.h"
#include "host/ble_hs.h"

static const char* TAG = "SECURE_BLE_SCAN";
static const uint8_t REGISTRY_KEY = "ROBOT_REGISTRY_TRUST_TOKEN_2026"; // Ключ для проверки

// Функция обратного вызова при обнаружении BLE устройств
static int ble_scan_cb(struct ble_gap_event *event, void *arg) {
    if (event->type == BLE_GAP_EVENT_DISC) {
        struct ble_hs_adv_fields fields;
        int rc = ble_hs_adv_parse_fields(&fields, event->disc.data, event->disc.length_data);
        if (rc != 0) return 0;

        // Ищем наши данные в Manufacturer Specific Data
        if (fields.mfg_data_len == sizeof(robot_sos_packet_t)) {
            robot_sos_packet_t incoming_packet;
            
            // Распаковываем пакет
            if (robot_packet_unpack(fields.mfg_data, fields.mfg_data_len, &incoming_packet) == 0) {
                // Уровень безопасности: Валидация пакета
                int res = robot_packet_validate(&incoming_packet, REGISTRY_KEY, sizeof(REGISTRY_KEY));
                
                if (res == 0) {
                    ESP_LOGI(TAG, "🟢 ОБНАРУЖЕН СЕРТИФИЦИРОВАННЫЙ РОБОТ В БЕДЕ!");
                    ESP_LOGI(TAG, "ID Робота: %llu | Код ЧП: 0x%04X | Данные: %lu", 
                             incoming_packet.robot_id, incoming_packet.event_code, incoming_packet.payload_data);
                    // Здесь вызывается логика ИИ или навигации робота для оказания помощи
                } else {
                    ESP_LOGW(TAG, "⚠️ Заблокирован подозрительный пакет. Код ошибки валидации: %d", res);
                }
            }
        }
    }
    return 0;
}

void transport_ble_scan_init(void) {
    struct ble_gap_disc_params disc_params;
    memset(&disc_params, 0, sizeof(disc_params));
    
    disc_params.filter_duplicates = 1; // Игнорировать дубликаты для разгрузки процессора
    disc_params.passive = 1;             // Пассивное сканирование (просто слушаем эфир)

    // Запускаем бесконечное сканирование
    int rc = ble_gap_disc(BLE_OWN_ADDR_PUBLIC, BLE_HS_FOREVER, &disc_params, ble_scan_cb, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "Ошибка запуска сканирования BLE: %d", rc);
    } else {
        ESP_LOGI(TAG, "BLE сканирование SOS-сигналов успешно запущено.");
    }
}
