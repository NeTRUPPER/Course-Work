# TouristRentalApp — Система проката туристического оборудования (единая документация)

Минималистичное кроссплатформенное Qt‑приложение для управления прокатом: клиенты, оборудование, аренды, печать договора, аутентификация. Документация объединена в один файл.

## Возможности
- Управление клиентами, оборудованием и арендами
- Поиск с подсказками (typeahead) по клиентам и оборудованию
- Печать договора: HTML напрямую или по .docx‑шаблону (подстановка плейсхолдеров)
- Аутентификация мастер‑паролем (сессия 1 час). Шифрование БД отключено по требованию
- Резервное копирование базы (копирование файла)
- Темы: светлая, тёмная, минималистичная
- Docker для одинакового запуска на Linux/Windows

## Установка и запуск (Linux, Ubuntu/Debian)
1) Зависимости:
```bash
sudo apt update && sudo apt install -y \
  build-essential cmake ninja-build \
  qt6-base-dev qt6-base-dev-tools \
  qt6-tools-dev qt6-tools-dev-tools \
  libqt6sql6-sqlite zip unzip
```
2) Сборка и запуск:
```bash
chmod +x ./build.sh
./build.sh
cd build && ./TouristRentalApp
```

## Установка и запуск (Windows)
Вариант A (рекомендуется): Docker Desktop + WSL2 — см. раздел Docker ниже.

Вариант B (нативно, кратко):
- Установите Qt 6 (MSVC), Visual Studio (MSVC) / Build Tools, CMake, Ninja
- Сборка:
```powershell
cmake -S . -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release
ninja -C build
build/TouristRentalApp.exe
```

## Docker (Linux/Windows)
1) Требуется Docker (на Windows — Docker Desktop + WSL2)
2) Сборка и запуск:
```bash
./docker-run.sh build
./docker-run.sh run
# интерактивно: ./docker-run.sh interactive
```
- Данные: ./data (монтируется в контейнер)
- Шаблоны договоров: ./templates
- GUI (Linux): при необходимости выполните `xhost +local:docker`

## Печать договора
- HTML‑печать из приложения — всегда доступна
- DOCX‑шаблон (рекомендуется): положите `Договор Прокат Всего  2025.docx` в “Документы” или выберите вручную при печати. Подставляются плейсхолдеры:
  `{{DATE}} {{CUSTOMER_NAME}} {{CUSTOMER_PHONE}} {{CUSTOMER_EMAIL}} {{CUSTOMER_PASSPORT}} {{CUSTOMER_ADDRESS}} {{EQUIPMENT_NAME}} {{EQUIPMENT_CATEGORY}} {{QUANTITY}} {{START}} {{END}} {{TOTAL}} {{DEPOSIT}} {{NOTES}}`

## Аутентификация и безопасность
- При первом запуске задаётся мастер‑пароль (однократно). При последующих запусках — запрос пароля (до 5 попыток). Сессия активна 1 час
- По вашему требованию шифрование БД отключено (PRAGMA key не применяется). Для включения возможно собрать driver SQLite с SQLCipher и вернуть соответствующий код

## Горячие клавиши
- Ctrl+N — Новый клиент; Ctrl+E — Новое оборудование; Ctrl+R — Новая аренда; Ctrl+F — Поиск; Ctrl+Q — Выход

## Структура
```
include/   # Заголовки
src/       # Исходники (C++)
resources/ # Ресурсы (иконки, стили QSS)
ui/        # Формы Qt Designer
build.sh   # Сборка (CMake + Make)
Dockerfile, docker-compose.yml, docker-run.sh
README.md  # Этот файл
```

## Частые проблемы
- Нет GUI в Docker (Linux): `xhost +local:docker`, проверьте `$DISPLAY`
- Qt не найден: установите зависимости (см. выше)
- DOCX не заполнился: убедитесь, что unzip/zip установлены, и плейсхолдеры совпадают

## Лицензия и назначение
Проект для учебных целей. Используйте на свой риск.