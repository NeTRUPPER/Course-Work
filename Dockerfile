# Multi-stage build для Qt приложения с SQLCipher
FROM ubuntu:22.04 as builder

# Установка зависимостей для сборки
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    pkg-config \
    qt6-base-dev \
    qt6-tools-dev \
    qt6-tools-dev-tools \
    libqt6sql6-sqlite \
    libsqlcipher-dev \
    sqlcipher \
    zip \
    unzip \
    curl \
    wget \
    && rm -rf /var/lib/apt/lists/*

# Создание рабочей директории
WORKDIR /app

# Копирование исходного кода
COPY . .

# Сборка приложения с SQLCipher
RUN BUILD_SQLCIPHER_PLUGIN=1 ./build.sh

# Финальный образ для запуска
FROM ubuntu:22.04 as runtime

# Установка только runtime зависимостей
RUN apt-get update && apt-get install -y \
    qt6-base-dev \
    libqt6sql6-sqlite \
    libsqlcipher0 \
    sqlcipher \
    zip \
    unzip \
    libxcb-xinerama0 \
    libxcb-cursor0 \
    libxcb-icccm4 \
    libxcb-image0 \
    libxcb-keysyms1 \
    libxcb-randr0 \
    libxcb-render-util0 \
    libxcb-shape0 \
    libxcb-xfixes0 \
    libxcb-xkb1 \
    libxkbcommon-x11-0 \
    libxkbcommon0 \
    libxrender1 \
    libgl1-mesa-glx \
    libglib2.0-0 \
    libx11-6 \
    libxext6 \
    libxss1 \
    libasound2 \
    libgstreamer1.0-0 \
    libgstreamer-plugins-base1.0-0 \
    && rm -rf /var/lib/apt/lists/*

# Создание пользователя для безопасности
RUN useradd -m -s /bin/bash appuser

# Создание директорий для данных
RUN mkdir -p /app/data /app/templates && \
    chown -R appuser:appuser /app

# Копирование собранного приложения
COPY --from=builder /app/build/TouristRentalApp /app/
COPY --from=builder /app/build/sqldrivers /app/sqldrivers/
COPY --from=builder /app/resources /app/resources/

# Копирование шаблонов (если есть)
COPY --chown=appuser:appuser templates/ /app/templates/ 2>/dev/null || true

# Установка прав
RUN chmod +x /app/TouristRentalApp && \
    chown -R appuser:appuser /app

# Переключение на пользователя
USER appuser

# Рабочая директория
WORKDIR /app

# Переменные окружения
ENV QT_QPA_PLATFORM=xcb
ENV QT_PLUGIN_PATH=/app/sqldrivers
ENV QT_DEBUG_PLUGINS=0
ENV XDG_DATA_HOME=/app/data

# Точка входа
ENTRYPOINT ["./TouristRentalApp"]

# Папка с БД и сохранением данных
VOLUME ["/app/data"]
