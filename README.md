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
Вариант A — Docker (рекомендуется):
```bash
# из корня репозитория
docker compose -f /home/vlad/Documents/VCode/Course-Work/docker-compose.yml build
docker compose -f /home/vlad/Documents/VCode/Course-Work/docker-compose.yml up -d
# логи при необходимости
docker compose -f /home/vlad/Documents/VCode/Course-Work/docker-compose.yml logs -f
```
- Данные БД сохраняются в именованном томе `sqlite_data` (монтируется в `/app/data`).
- Если нужен bind‑mount, создайте `docker-compose.override.yml` рядом и укажите путь (пример ниже в разделе Docker).

Вариант B — Нативно:
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
./build/TouristRentalApp
```

## Установка и запуск (Windows)
Вариант A — Docker Desktop (WSL2, рекомендуется):
1) Установите Docker Desktop и включите WSL2 Backend.
2) В PowerShell/CMD из корня проекта:
```powershell
docker compose -f C:\\path\\to\\Course-Work\\docker-compose.yml build
docker compose -f C:\\path\\to\\Course-Work\\docker-compose.yml up -d
docker compose -f C:\\path\\to\\Course-Work\\docker-compose.yml logs -f
```
- Данные сохраняются в именованном томе `sqlite_data` (кроссплатформенно).
- Для bind‑mount используйте абсолютный путь Windows (пример ниже).

Вариант B — Нативно (кратко):
- Установите: Qt 6 (MSVC), Visual Studio Build Tools, CMake, Ninja.
- Сборка:
```powershell
cmake -S . -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release
ninja -C build
build\\TouristRentalApp.exe
```

## Docker (Linux/Windows)
1) Требуется Docker (на Windows — Docker Desktop + WSL2).
2) Быстрый старт командами compose (см. разделы выше) или helper‑скриптом:
```bash
./docker-run.sh build
./docker-run.sh run
# интерактивно (для отладки GUI на Linux): ./docker-run.sh interactive
```
- Персистентность БД: именованный том `sqlite_data` → `/app/data`. Внутри приложения путь к БД определяется через `QStandardPaths::AppDataLocation`, а `XDG_DATA_HOME=/app/data` направляет его в том.
- Шаблоны договоров: каталог `templates/` копируется в образ в `/app/templates`.
- GUI (Linux): при необходимости выполните `xhost +local:docker` перед запуском, и экспортируйте `DISPLAY` в контейнере при интерактивном режиме.

Bind‑mount (по желанию, чтобы видеть `rental.db` на хосте):
- Linux пример `docker-compose.override.yml`:
```yaml
services:
  app:
    volumes:
      - /home/USER/Course-Work/.data:/app/data
```
- Windows пример `docker-compose.override.yml` (путь Windows):
```yaml
services:
  app:
    volumes:
      - C:\\Users\\YourName\\Course-Work\\.data:/app/data
```
- Windows пример через WSL2‑путь:
```yaml
services:
  app:
    volumes:
      - /mnt/c/Users/YourName/Course-Work/.data:/app/data
```

Проверка сохранности данных:
```bash
docker compose down
docker compose up -d
# данные (клиенты/оборудование/аренды) должны сохраниться
```

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

## Верстка и UI (Qt6)
- **Архитектура UI**: интерфейс построен на Qt Widgets. Основное окно — `ui/mainwindow.ui`, формы ввода — `ui/customerform.ui`, `ui/equipmentform.ui`, `ui/rentalform.ui`. Верстка выполнена через Qt Designer (Layouts, Spacers), логика — в `src/*`.
  - **Главное окно** (`mainwindow.ui` + `src/mainwindow.cpp`): меню, тулбар с иконками, таблицы клиентов/оборудования/аренд, панель действий (добавить/изменить/удалить/поиск/печать/экспорт/отчет).
  - **Диалоги**: `customerdialog`, `equipmentdialog`, `rentaldialog` — отвечают за создание/редактирование сущностей, включают валидацию полей и подсказки (typeahead) по связанным данным.

- **Ресурсы и оформление**:
  - **Иконки**: каталог `resources/icons/` и регистрация в `resources/resources.qrc`. Иконки назначаются в Designer или программно. Чтобы добавить иконку: положите файл в `resources/icons/`, добавьте запись в `resources.qrc`, пересоберите.
  - **Стили (QSS)**: базовая тема — `resources/styles/light.qss`. Тема подключается при старте приложения (см. `src/main.cpp`/`src/mainwindow.cpp`). QSS влияет на виджет‑уровень: цвета, отступы, состояния `:hover/:pressed`.
  - **Шрифты и DPI**: используется системный шрифт Qt. Включена поддержка High‑DPI (Qt автоматически масштабирует; проверьте переменные окружения `QT_SCALE_FACTOR`, `QT_AUTO_SCREEN_SCALE_FACTOR=1` при необходимости).

- **Навигация и UX‑практики**:
  - Единый тулбар с часто используемыми действиями и хоткеями (см. раздел «Горячие клавиши»).
  - Поиск по таблицам с инкрементальным фильтром, результаты подсвечиваются, пустые состояния отображают понятные сообщения.
  - Диалоги не блокируют основное окно дольше, чем нужно: валидация выполняется до сохранения, ошибки показываются через `QMessageBox`.

- **Таблицы и формы**:
  - Таблицы клиентов, оборудования и аренд — на базе `QTableView` с моделью SQL/кастомной моделью. Колонки настроены на авто‑resize по содержимому, для длинных текстов используется elide.
  - Формы диалогов содержат группировки полей, маски ввода и проверки (`QValidator`) для телефонов, дат, числовых значений, сумм.

- **Печать и экспорт**:
  - Печать договора из главного окна: HTML‑рендер по текущим данным или заполнение DOCX‑шаблона (см. раздел «Печать договора»). Экспорт таблиц — в CSV/HTML при необходимости.

- **Темизация и кастомизация**:
  - Чтобы поменять цвета/отступы — правьте `resources/styles/light.qss`. Пример подключения темы программно:
    1) Загрузите файл через `QFile(":/styles/light.qss")` (путь из `resources.qrc`).
    2) Примените через `qApp->setStyleSheet(...)`.
  - Можно добавить другие темы, например `dark.qss`, и переключатель темы в настройках приложения.

- **Добавление новых элементов UI**:
  1) Создайте `.ui` в Qt Designer и положите в `ui/`.
  2) Сгенерируйте соответствующий класс через `uic` (CMake делает это автоматически) — заголовок появится в `include/ui_*.h` (автогенерируется).
  3) Подключите виджет в соответствующем `*.cpp`, свяжите сигналы/слоты.
  4) При необходимости добавьте иконки в `resources/icons/` и обновите `resources.qrc`.

- **Лучшие практики, применённые в верстке**:
  - Везде используются `Layouts` для адаптивности вместо жёстких размеров.
  - Контрастные элементы управления, подсказки placeholder‑текстом, валидация до сохранения.
  - Минимум модальных диалогов, подтверждения только на деструктивные действия.
  - Единый набор отступов/радиусов через QSS, единая палитра (светлая тема).

- **Где искать код UI**:
  - Формы: каталог `ui/` (`*.ui`).
  - Логика окон/диалогов: `src/mainwindow.cpp`, `src/customerdialog.cpp`, `src/equipmentdialog.cpp`, `src/rentaldialog.cpp` и соответствующие заголовки в `include/`.
  - Ресурсы: `resources/` + `resources/resources.qrc`.

## Частые проблемы
- Нет GUI в Docker (Linux): `xhost +local:docker`, проверьте `$DISPLAY`
- Qt не найден: установите зависимости (см. выше)
- DOCX не заполнился: убедитесь, что unzip/zip установлены, и плейсхолдеры совпадают

## Лицензия и назначение
Проект для учебных целей. Используйте на свой риск.