#!/bin/bash

# Скрипт сборки для TouristRentalApp
# Автор: AI Assistant
# Дата: $(date)

echo "=== Сборка TouristRentalApp ==="

# Создаем директорию для сборки
if [ ! -d "build" ]; then
    mkdir build
    echo "Создана директория build/"
fi

# Переходим в директорию сборки
cd build

# Очищаем предыдущую сборку
echo "Очистка предыдущей сборки..."
rm -rf *

# Запускаем CMake
echo "Запуск CMake..."
cmake ..

# Проверяем успешность CMake
if [ $? -ne 0 ]; then
    echo "Ошибка: CMake завершился с ошибкой"
    exit 1
fi

# Компилируем проект
echo "Компиляция проекта..."
make -j$(nproc)

# Проверяем успешность компиляции
if [ $? -ne 0 ]; then
    echo "Ошибка: Компиляция завершилась с ошибкой"
    exit 1
fi

echo "=== Сборка завершена успешно! ==="
echo "Исполняемый файл: build/TouristRentalApp"
echo ""
echo "Для запуска выполните:"
echo "cd build && ./TouristRentalApp" 