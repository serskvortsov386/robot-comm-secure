#include "robot_secure_comm.h"
#include "esp_log.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"
#include <string.h>

static const char* TAG = "SECURE_BLE";
static robot_sos_packet_t ble_packet_storage;
static const uint8_t REGISTRY_KEY[] = "ROBOT_REGISTRY_TRUST_TOKEN_2026";

// ==========================================
// ЧАСТЬ 1: ОТПРАВКА СИГНАЛА (ВЕЩАНИЕ/BROADCAST)
// ==========================================

static void ble_app_advertise(void) {
    struct ble_gap_adv_params adv_params;
    struct ble_hs_adv_fields fields;
    int rc;

    memset(&fields, 0, sizeof fields);
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    
    // Упаковываем наш бинарный SOS-пакет в поле Manufacturer Specific Data
    fields.mfg_data = (uint8_t*)&ble_packet_storage;
    fields.mfg_data_len = sizeof(robot_sos_packet_t);

    rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        ESP_LOGE(TAG, "Ошибка установки полей BLE рекламы: %d", rc);
        return;
    }

    memset(&adv_params, 0, sizeof adv_params);
    adv_params.conn_mode = BLE_GAP_CONN_MODE_NON; // Режим без установки соединения
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    rc = ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, BLE_HS_FOREVER, &adv_params, NULL, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "Ошибка запуска BLE вещания: %d", rc);
    }
}

// ==========================================
// ЧАСТЬ 2: ПРИЕМ СИГНАЛА (СКАНЕР ДЛЯ ПОМОЩНИКА)
// ==========================================

static int ble_scan_cb(struct ble_gap_event *event, void *arg) {
    if (event->type == BLE_GAP_EVENT_DISC) {
        struct ble_hs_adv_fields fields;
        int rc = ble_hs_adv_parse_fields(&fields, event->disc.data, event->disc.length_data);
        if (rc != 0) return 0;

        // Если размер Manufacturer Data совпадает с размером нашего SOS-пакета
        if (fields.mfg_data_len == sizeof(robot_sos_packet_t)) {
            robot_sos_packet_t incoming_packet;
            
            if (robot_packet_unpack(fields.mfg_data, fields.mfg_data_len, &incoming_packet) == 0) {
                // Криптографическая валидация пакета
                int res = robot_packet_validate(&incoming_packet, REGISTRY_KEY, sizeof(REGISTRY_KEY));
                
                if (res == 0) {
                    ESP_LOGI(TAG, "🟢 ОБНАРУЖЕН СЕРТИФИЦИРОВАННЫЙ РОБОТ В БЕДЕ!");
                    ESP_LOGI(TAG, "ID Робота: %llu | Код ЧП: 0x%04X | Инфо-поле: %lu", 
                             incoming_packet.robot_id, incoming_packet.event_code, incoming_packet.payload_data);
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
    
    disc_params.filter_duplicates = 1; // Убираем дубликаты пакетов для экономии процессора
    disc_params.passive = 1;             // Пассивное прослушивание эфира

    int rc = ble_gap_disc(BLE_OWN_ADDR_PUBLIC, BLE_HS_FOREVER, &disc_params, ble_scan_cb, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "Ошибка запуска сканирования BLE: %d", rc);
    } else {
        ESP_LOGI(TAG, "BLE сканирование SOS-сигналов успешно запущено.");
    }
}

// ==========================================
// ЧАСТЬ 3: ИНИЦИАЛИЗАЦИЯ И СИНХРОНИЗАЦИЯ СТЕКА
// ==========================================

static void ble_app_on_sync(void) {
    ESP_LOGI(TAG, "NimBLE стек синхронизирован.");
    // Если в буфере лежит готовый пакет — запускаем вещание «Я в беде»
    if (ble_packet_storage.version == PROTOCOL_VERSION) {
        ble_app_advertise();
    } else {
        // Иначе этот робот здоров и просто включает режим сканирования (помощник)
        transport_ble_scan_init();
    }
}

void transport_ble_init(const robot_sos_packet_t* packet_to_broadcast) {
    if (packet_to_broadcast != NULL) {
        memcpy(&ble_packet_storage, packet_to_broadcast, sizeof(robot_sos_packet_t));
    } else {
        memset(&ble_packet_storage, 0, sizeof(robot_sos_packet_t));
    }

    ESP_ERROR_CHECK(nimble_port_init());
    ble_hs_cfg.sync_cb = ble_app_on_sync;
    
    nimble_port_freertos_init(NULL);
    ESP_LOGI(TAG, "BLE транспорт успешно инициализирован.");
}
