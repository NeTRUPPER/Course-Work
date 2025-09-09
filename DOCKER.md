# Docker развертывание Tourist Rental System

## Быстрый старт

### Linux (Ubuntu/Debian)

1. **Установка Docker:**
```bash
sudo apt update
sudo apt install -y docker.io docker-compose
sudo usermod -aG docker $USER
# Перелогиньтесь или выполните: newgrp docker
```

2. **Запуск приложения:**
```bash
# Сборка и запуск
./docker-run.sh build
./docker-run.sh run

# Или интерактивный режим
./docker-run.sh interactive
```

### Windows (WSL2 + Docker Desktop)

1. **Установка Docker Desktop для Windows**
2. **В WSL2:**
```bash
# Установка Docker в WSL2
curl -fsSL https://get.docker.com -o get-docker.sh
sudo sh get-docker.sh
sudo usermod -aG docker $USER

# Запуск приложения
./docker-run.sh build
./docker-run.sh interactive
```

## Команды управления

```bash
# Сборка образа
./docker-run.sh build

# Запуск в фоне
./docker-run.sh run

# Интерактивный режим (для отладки)
./docker-run.sh interactive

# Просмотр логов
./docker-run.sh logs

# Остановка
./docker-run.sh stop

# Перезапуск
./docker-run.sh restart

# Очистка
./docker-run.sh clean
```

## Структура данных

```
Course-Work/
├── data/           # База данных и настройки (монтируется в контейнер)
├── templates/      # Шаблоны документов
├── Dockerfile      # Конфигурация Docker образа
├── docker-compose.yml
└── docker-run.sh   # Скрипт управления
```

## Особенности

### Безопасность
- Приложение запускается под пользователем `appuser` (не root)
- Данные изолированы в volume
- SQLCipher шифрование базы данных

### Кроссплатформенность
- Linux: нативный X11 через Docker
- Windows: через WSL2 + Docker Desktop
- Автоматическая настройка GUI окружения

### Персистентность данных
- База данных сохраняется в `./data/`
- Настройки Qt в `~/.config/`
- Шаблоны документов в `./templates/`

## Устранение проблем

### GUI не отображается (Linux)
```bash
# Разрешить доступ к X11
xhost +local:docker

# Проверить DISPLAY
echo $DISPLAY
```

### Ошибки сборки
```bash
# Очистить кэш Docker
./docker-run.sh clean

# Пересобрать
./docker-run.sh build
```

### Проблемы с правами
```bash
# Исправить права на директории
sudo chown -R $USER:$USER data templates
```

## Производственное развертывание

### С systemd (Linux)
```bash
# Создать systemd service
sudo tee /etc/systemd/system/tourist-rental.service > /dev/null <<EOF
[Unit]
Description=Tourist Rental System
After=docker.service
Requires=docker.service

[Service]
Type=oneshot
RemainAfterExit=yes
WorkingDirectory=/path/to/Course-Work
ExecStart=/path/to/Course-Work/docker-run.sh run
ExecStop=/path/to/Course-Work/docker-run.sh stop
User=$USER

[Install]
WantedBy=multi-user.target
EOF

# Включить и запустить
sudo systemctl enable tourist-rental.service
sudo systemctl start tourist-rental.service
```

### С Docker Swarm
```bash
# Инициализировать swarm
docker swarm init

# Развернуть stack
docker stack deploy -c docker-compose.yml tourist-rental
```

