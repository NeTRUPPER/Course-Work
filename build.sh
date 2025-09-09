#!/bin/bash

# Скрипт сборки для TouristRentalApp
# Автор: AI Assistant
# Дата: $(date)

echo "=== Сборка TouristRentalApp ==="

set -e

build_sqlcipher_qsqlite() {
  echo "--- Подготовка плагина Qt SQLite с SQLCipher ---"
  if ! command -v sqlcipher >/dev/null 2>&1; then
    echo "[!] Не найден sqlcipher. Установите пакет libsqlcipher-dev и sqlcipher." >&2
    return 1
  fi
  if ! dpkg -s qt6-base-dev >/dev/null 2>&1; then
    echo "[!] Не найден qt6-base-dev. Установите Qt6 dev-пакеты." >&2
    return 1
  fi

  workdir="$(pwd)"
  tmpdir="${workdir}/.qt_sqlcipher_build"
  rm -rf "$tmpdir" && mkdir -p "$tmpdir"
  pushd "$tmpdir" >/dev/null

  echo "Загрузка исходников qt6-base (apt source)..."
  apt source qt6-base-dev >/dev/null 2>&1 || true
  qtsrc_dir=$(ls -d qtbase-* 2>/dev/null | head -n1 || true)
  if [ -z "$qtsrc_dir" ]; then
    echo "[!] Не удалось получить исходники qtbase через apt source." >&2
    popd >/dev/null; return 1
  fi

  echo "Сборка плагина qsqlite с SQLCipher..."
  cd "$qtsrc_dir/src/plugins/sqldrivers/sqlite"
  rm -rf build && mkdir build && cd build
  cmake .. -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DQt6_DIR=/usr/lib/cmake/Qt6 \
    -DQT_FEATURE_system_sqlite=ON \
    -DSQLite3_INCLUDE_DIR=/usr/include \
    -DSQLite3_LIBRARY=/usr/lib/x86_64-linux-gnu/libsqlcipher.so \
    -DCMAKE_CXX_FLAGS="-DSQLITE_HAS_CODEC" || { echo "[!] CMake для плагина не прошел."; popd >/dev/null; return 1; }
  ninja || { echo "[!] Сборка плагина не удалась."; popd >/dev/null; return 1; }

  outdir="${workdir}/build/sqldrivers"
  mkdir -p "$outdir"
  cp -f libqsqlite.so "$outdir/" || true
  echo "Плагин скопирован в: $outdir/libqsqlite.so"
  echo "Проверка зависимостей:" && ldd "$outdir/libqsqlite.so" | grep -i sqlcipher || true

  popd >/dev/null
}

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
echo "--- Опционально: сборка плагина SQLCipher для шифрования БД ---"
if [ "${BUILD_SQLCIPHER_PLUGIN}" = "1" ]; then
  if command -v apt >/dev/null 2>&1; then
    echo "Установка зависимостей (может потребоваться sudo):"
    set +e
    sudo apt update && sudo apt install -y ninja-build libsqlcipher-dev sqlcipher qt6-base-dev qt6-base-dev-tools >/dev/null 2>&1
    set -e
  fi
  build_sqlcipher_qsqlite || echo "[!] Автосборка плагина не удалась. Соберите вручную по README."
fi
echo ""
echo "Для запуска выполните:"
echo "cd build && ./TouristRentalApp" 