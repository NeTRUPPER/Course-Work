// ------------------------------------------------------------
// Файл: main.cpp
// Назначение: Точка входа в приложение TouristRentalApp.
// Инициализирует Qt, настраивает окно и запускает цикл событий.
// ------------------------------------------------------------
#include <QApplication>
#include <QStyleFactory>
#include <QDir>
#include <QStandardPaths>
#include <QMessageBox>
#include <QDebug>
#include "mainwindow.h"
#include "database.h"
#include "security.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Устанавливаем современный стиль
    app.setStyle(QStyleFactory::create("Fusion"));
    
    // Настройка информации о приложении
    app.setApplicationName("Tourist Rental System");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("Tourist Rental");
    
    // Создаем директорию для данных приложения
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataPath);
    
    try {
        // Инициализируем базу данных
        Database::initialize(dataPath + "/rental.db");
        
        // Открываем базу данных (создаст таблицы при необходимости)
        if (!Database::getInstance().openDatabase("")) {
            QMessageBox::critical(nullptr, "Ошибка БД", "Не удалось открыть базу данных");
            return -1;
        }
        
        // Создаем и показываем главное окно
        MainWindow window;
        window.show();
        
        return app.exec();
        
    } catch (const std::exception& e) {
        QMessageBox::critical(nullptr, "Ошибка инициализации", 
                            QString("Не удалось инициализировать приложение: %1").arg(e.what()));
        return -1;
    }
} 