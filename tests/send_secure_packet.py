import struct
import binascii
import argparse

def create_robot_packet(robot_id, event_code, timestamp, payload):
    version = 0x01
    # Формируем тело пакета без CRC и подписи: B (1 байт), Q (8 байт), H (2 байт), I (4 байт), I (4 байт)
    header_format = "<B Q H I I"
    packed_header = struct.pack(header_format, version, robot_id, event_code, timestamp, payload)
    
    # Считаем контрольную сумму CRC32 (как это сделает ESP32)
    crc32 = binascii.crc32(packed_header) & 0xffffffff
    
    # Имитируем подпись Ed25519 (64 байта нулей для теста или реальная подпись)
    mock_signature = b'\x00' * 64
    
    # Полный пакет: заголовок + CRC32 (I) + Подпись (64s)
    full_packet_format = f"<B Q H I I I 64s"
    return struct.pack(full_packet_format, version, robot_id, event_code, timestamp, payload, crc32, mock_signature)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Симулятор безопасного реестра роботов")
    parser.add_argument("--event", type=int, default=1, help="Код события (например, 1 - застрял)")
    args = parser.parse_args()

    # Собираем пакет от робота №54321
    packet = create_robot_packet(robot_id=54321, event_code=args.event, timestamp=1770000000, payload=0)
    
    print(f" Сгенерирован безопасный бинарный пакет (Размер: {len(packet)} байт):")
    print(f"Хекс-дамп: {binascii.hexlify(packet).decode()}")
    print(" Направлен в транспортный слой (ИК/BLE)...")
