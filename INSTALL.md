# Инструкции по установке

## Системные требования

- Linux (Ubuntu 20.04+, Debian 11+, Arch Linux)
- CMake 3.16+
- C++17 компилятор (GCC 7+, Clang 6+)
- Qt6 (Core, Widgets, Sql)
- SQLCipher

## Установка зависимостей

### Ubuntu/Debian

```bash
# Обновление пакетов
sudo apt update

# Установка инструментов разработки
sudo apt install build-essential cmake pkg-config

# Установка Qt6
sudo apt install qt6-base-dev qt6-tools-dev qt6-tools-dev-tools

# Установка SQLCipher
sudo apt install libsqlcipher-dev

# Дополнительные зависимости
sudo apt install libssl-dev
```

### Arch Linux

```bash
# Установка инструментов разработки
sudo pacman -S base-devel cmake pkg-config

# Установка Qt6
sudo pacman -S qt6-base qt6-tools

# Установка SQLCipher
sudo pacman -S sqlcipher

# Дополнительные зависимости
sudo pacman -S openssl
```

### Fedora/RHEL/CentOS

```bash
# Установка инструментов разработки
sudo dnf install gcc-c++ cmake pkg-config

# Установка Qt6
sudo dnf install qt6-qtbase-devel qt6-qttools-devel

# Установка SQLCipher
sudo dnf install sqlcipher-devel

# Дополнительные зависимости
sudo dnf install openssl-devel
```

## Проверка установки

После установки зависимостей проверьте их наличие:

```bash
# Проверка CMake
cmake --version

# Проверка Qt6
pkg-config --modversion Qt6Core

# Проверка SQLCipher
pkg-config --modversion sqlcipher
```

## Сборка проекта

1. Клонируйте репозиторий:
```bash
git clone <repository-url>
cd TouristRentalApp
```

2. Запустите скрипт сборки:
```bash
./build.sh
```

Или соберите вручную:
```bash
mkdir build
cd build
cmake ..
make -j$(nproc)
```

## Запуск приложения

После успешной сборки:

```bash
cd build
./TouristRentalApp
```

При первом запуске система попросит установить мастер-пароль для защиты данных.

## Устранение проблем

### Ошибка "Qt6 not found"
```bash
# Ubuntu/Debian
sudo apt install qt6-base-dev qt6-tools-dev

# Arch Linux
sudo pacman -S qt6-base qt6-tools
```

### Ошибка "SQLCipher not found"
```bash
# Ubuntu/Debian
sudo apt install libsqlcipher-dev

# Arch Linux
sudo pacman -S sqlcipher
```

### Ошибка компиляции
Убедитесь, что у вас установлен компилятор C++17:
```bash
g++ --version
```

### Ошибка линковки
Проверьте, что все библиотеки установлены:
```bash
pkg-config --list-all | grep -E "(Qt6|sqlcipher)"
```

## Разработка

Для разработки рекомендуется установить дополнительные инструменты:

```bash
# Ubuntu/Debian
sudo apt install qt6-tools-dev-tools qtcreator

# Arch Linux
sudo pacman -S qt6-tools qtcreator
```

## Отладка

Для отладки соберите проект с отладочной информацией:

```bash
mkdir build-debug
cd build-debug
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)
``` 