#!/bin/bash

# Скрипт для сборки и запуска Docker контейнера
# Поддерживает Linux и Windows (WSL2)

set -e

# Цвета для вывода
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Функция для вывода сообщений
log() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

# Проверка наличия Docker
check_docker() {
    if ! command -v docker &> /dev/null; then
        error "Docker не установлен. Установите Docker и попробуйте снова."
        exit 1
    fi
    
    if ! command -v docker-compose &> /dev/null; then
        warn "docker-compose не найден. Попробуем использовать 'docker compose'"
        COMPOSE_CMD="docker compose"
    else
        COMPOSE_CMD="docker-compose"
    fi
}

# Создание необходимых директорий
create_directories() {
    log "Создание необходимых директорий..."
    mkdir -p data templates
    
    # Создание .gitignore для данных
    if [ ! -f data/.gitignore ]; then
        echo "*.db" > data/.gitignore
        echo "*.db-journal" >> data/.gitignore
        echo "*.db-wal" >> data/.gitignore
        echo "*.db-shm" >> data/.gitignore
    fi
}

# Настройка X11 для Linux
setup_x11() {
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        log "Настройка X11 для Linux..."
        
        # Проверка DISPLAY
        if [ -z "$DISPLAY" ]; then
            warn "DISPLAY не установлен. Устанавливаем :0"
            export DISPLAY=:0
        fi
        
        # Разрешение доступа к X11
        xhost +local:docker 2>/dev/null || warn "Не удалось настроить xhost. GUI может не работать."
    fi
}

# Сборка образа
build_image() {
    log "Сборка Docker образа..."
    $COMPOSE_CMD build --no-cache
    
    if [ $? -eq 0 ]; then
        success "Образ успешно собран"
    else
        error "Ошибка при сборке образа"
        exit 1
    fi
}

# Запуск контейнера
run_container() {
    log "Запуск контейнера..."
    
    # Остановка существующего контейнера
    $COMPOSE_CMD down 2>/dev/null || true
    
    # Запуск нового контейнера
    $COMPOSE_CMD up -d
    
    if [ $? -eq 0 ]; then
        success "Контейнер запущен"
        log "Приложение доступно в контейнере 'tourist-rental-system'"
        log "Для просмотра логов: $COMPOSE_CMD logs -f"
        log "Для остановки: $COMPOSE_CMD down"
    else
        error "Ошибка при запуске контейнера"
        exit 1
    fi
}

# Запуск в интерактивном режиме
run_interactive() {
    log "Запуск в интерактивном режиме..."
    
    setup_x11
    
    docker run -it --rm \
        -e DISPLAY=$DISPLAY \
        -e QT_QPA_PLATFORM=xcb \
        -e QT_PLUGIN_PATH=/app/sqldrivers \
        -v /tmp/.X11-unix:/tmp/.X11-unix:rw \
        -v "$(pwd)/data:/app/data" \
        -v "$(pwd)/templates:/app/templates" \
        -v "$HOME/.config:/home/appuser/.config:rw" \
        --name tourist-rental-app \
        tourist-rental-app:latest
}

# Очистка
cleanup() {
    log "Очистка Docker ресурсов..."
    $COMPOSE_CMD down --volumes --remove-orphans
    docker system prune -f
    success "Очистка завершена"
}

# Показать помощь
show_help() {
    echo "Использование: $0 [КОМАНДА]"
    echo ""
    echo "Команды:"
    echo "  build     - Собрать Docker образ"
    echo "  run       - Запустить контейнер в фоне"
    echo "  interactive - Запустить в интерактивном режиме"
    echo "  logs      - Показать логи контейнера"
    echo "  stop      - Остановить контейнер"
    echo "  restart   - Перезапустить контейнер"
    echo "  clean     - Очистить Docker ресурсы"
    echo "  help      - Показать эту справку"
    echo ""
    echo "Примеры:"
    echo "  $0 build && $0 run"
    echo "  $0 interactive"
    echo "  $0 logs"
}

# Основная логика
main() {
    case "${1:-help}" in
        "build")
            check_docker
            create_directories
            build_image
            ;;
        "run")
            check_docker
            create_directories
            setup_x11
            run_container
            ;;
        "interactive")
            check_docker
            create_directories
            run_interactive
            ;;
        "logs")
            check_docker
            $COMPOSE_CMD logs -f
            ;;
        "stop")
            check_docker
            $COMPOSE_CMD down
            success "Контейнер остановлен"
            ;;
        "restart")
            check_docker
            $COMPOSE_CMD restart
            success "Контейнер перезапущен"
            ;;
        "clean")
            check_docker
            cleanup
            ;;
        "help"|*)
            show_help
            ;;
    esac
}

# Запуск
main "$@"
