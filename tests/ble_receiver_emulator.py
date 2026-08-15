import asyncio
import hmac
import hashlib
import struct
from bleak import BleakScanner

# Секретный ключ общего реестра (должен строго совпадать с ключом в ESP32!)
REGISTRY_SECRET_KEY = b"ROBOT_REGISTRY_TRUST_TOKEN_2026"

# Константы протокола
PROTOCOL_VERSION = 0x02
EXPECTED_PACKET_SIZE = 64  # Итоговый размер структуры

# Словарь человекочитаемых ошибок
EVENT_CODES = {
    0x0000: "🟢 Робот в порядке (OK)",
    0x0001: "⚠️ Механическая блокировка (Застрял)",
    0x0002: "🪫 Критический разряд батареи",
    0x0003: "🛑 Отказ навигационных сенсоров",
    0x0004: "🚨 Аварийная остановка оператором"
}

def validate_robot_packet(raw_data: bytes) -> dict | None:
    """Парсинг и криптографическая проверка бинарного SOS-пакета"""
    if len(raw_data) < EXPECTED_PACKET_SIZE:
        return None

    # Структура пакета в Си: pack(1) -> < B(1) Q(8) H(2) I(4) I(4) I(4) 32s(32)
    # Поля: version, robot_id, event_code, timestamp, payload_data, crc32, signature
    packet_format = "<B Q H I I I 32s"
    
    try:
        unpacked = struct.unpack(packet_format, raw_data[:EXPECTED_PACKET_SIZE])
        version, robot_id, event_code, timestamp, payload, crc32, signature = unpacked
        
        # 1. Проверка версии
        if version != PROTOCOL_VERSION:
            return {"error": f"Неверная версия протокола: {version}"}

        # 2. Проверка сигнатуры HMAC-SHA256
        # Подписываем часть данных ДО поля signature (первые 23 байта)
        data_to_sign = raw_data[:23]
        expected_hmac = hmac.new(REGISTRY_SECRET_KEY, data_to_sign, hashlib.sha256).digest()

        if not hmac.compare_digest(signature, expected_hmac):
            return {"error": "КРИПТОГРАФИЧЕСКАЯ ОШИБКА: Подпись не совпадает! Пакет подделан!"}

        # Если проверка пройдена, возвращаем распарсенные данные
        return {
            "success": True,
            "robot_id": robot_id,
            "event": EVENT_CODES.get(event_code, f"Неизвестный код 0x{event_code:04X}"),
            "timestamp": timestamp,
            "payload": payload
        }
    except Exception as e:
        return {"error": f"Ошибка парсинга структуры: {e}"}

def detection_callback(device, advertisement_data):
    """Вызывается при обнаружении любого BLE-устройства"""
    # Проверяем наличие Manufacturer Specific Data
    if advertisement_data.manufacturer_data:
        # Извлекаем сырые бинарные данные вещания
        for mfg_id, raw_bytes in advertisement_data.manufacturer_data.items():
            # Наш пакет должен быть ровно 64 байта
            if len(raw_bytes) == EXPECTED_PACKET_SIZE:
                result = validate_robot_packet(raw_bytes)
                
                if result:
                    print("-" * 60)
                    if "error" in result:
                        print(f"❌ Заблокирована угроза от устройства [{device.address}]:")
                        print(f"   {result['error']}")
                    else:
                        print(f"📡 СЧИТАН СИГНАЛ ВЗАИМОПОМОЩИ ОТ РЕЕСТРОВОГО РОБОТА!")
                        print(f"   Адрес устройства: {device.address}")
                        print(f"   ID Робота в базе: {result['robot_id']}")
                        print(f"   Статус/Проблема:  {result['event']}")
                        print(f"   Метка времени:    {result['timestamp']}")
                        print(f"   Доп. данные:      {result['payload']}")
                    print("-" * 60)

async def main():
    print("🚀 Эмулятор робота-помощника запущен.")
    print("Слушаю BLE эфир на предмет защищенных SOS-сигналов из реестра...")
    
    # Инициализируем сканер Bleak с привязкой к нашему коллбэку
    scanner = BleakScanner(detection_callback)
    await scanner.start()
    
    # Сканируем бесконечно
    while True:
        await asyncio.sleep(1.0)

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\n🛑 Эмулятор остановлен.")
