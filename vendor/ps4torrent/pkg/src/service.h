#pragma once
#include <string>

// Служба ps4torrentd (payload) живёт отдельно от приложения: приложение запускает её отправкой ELF в доступный BinLoader и разговаривает с ней по HTTP. Автозапуск и системные конфиги без отдельного согласия пользователя не меняются.

// Отправляет ELF из PKG в BinLoader GoldHEN (127.0.0.1:9090). Если старая копия службы ещё работает,
// новая сама попросит её завершиться. connectFailed (необязателен): true, если не удалось именно подключиться
// к порту 9090 (BinLoader выключен: GoldHEN - Servers settings - Enable Payloader Server).
bool service_send_payload(std::string& err, bool* connectFailed = nullptr);
