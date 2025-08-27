#include "equipment.h"
#include "database.h"
#include <QDebug>

Equipment::Equipment(QObject *parent)
    : QObject(parent)
    , m_id(0)
    , m_price(0.0)
    , m_additionalDayPrice(0.0)
    , m_deposit(0.0)
    , m_quantity(1)
    , m_availableQuantity(1)
    , m_createdAt(QDateTime::currentDateTime())
    , m_updatedAt(QDateTime::currentDateTime())
{
}

Equipment::Equipment(int id, const QString& name, const QString& category, double price,
                    double deposit, int quantity, const QString& description, QObject *parent)
    : QObject(parent)
    , m_id(id)
    , m_name(name)
    , m_category(category)
    , m_price(price)
    , m_additionalDayPrice(0.0)
    , m_deposit(deposit)
    , m_quantity(quantity)
    , m_availableQuantity(quantity)
    , m_description(description)
    , m_createdAt(QDateTime::currentDateTime())
    , m_updatedAt(QDateTime::currentDateTime())
{
}

bool Equipment::isAvailable() const
{
    return m_availableQuantity > 0;
}

bool Equipment::canRent(int quantity) const
{
    return m_availableQuantity >= quantity && quantity > 0;
}

double Equipment::calculateRentalPrice(int days) const
{
    if (days <= 0) return 0.0;
    if (days == 1) return m_price;
    double additional = (m_additionalDayPrice > 0.0 ? m_additionalDayPrice : m_price);
    return m_price + additional * (days - 1);
}

double Equipment::calculateDeposit() const
{
    return m_deposit;
}

void Equipment::reserveQuantity(int quantity)
{
    if (m_availableQuantity >= quantity) {
        m_availableQuantity -= quantity;
    }
}

void Equipment::releaseQuantity(int quantity)
{
    m_availableQuantity = qMin(m_availableQuantity + quantity, m_quantity);
}

bool Equipment::isValid() const
{
    return validateName() && validateCategory() && validatePrice() && 
           validateDeposit() && validateQuantity();
}

QStringList Equipment::getValidationErrors() const
{
    QStringList errors;
    
    if (!validateName()) {
        errors << "Название не может быть пустым";
    }
    
    if (!validateCategory()) {
        errors << "Категория не может быть пустой";
    }
    
    if (!validatePrice()) {
        errors << "Цена должна быть положительной";
    }
    
    if (!validateDeposit()) {
        errors << "Залог должен быть положительным";
    }
    
    if (!validateQuantity()) {
        errors << "Количество должно быть положительным";
    }
    
    return errors;
}

bool Equipment::save()
{
    if (!isValid()) {
        qDebug() << "Ошибка валидации оборудования:" << getValidationErrors();
        return false;
    }
    
    Database& db = Database::getInstance();
    
    if (m_id == 0) {
        // Новое оборудование
        if (db.addEquipment(m_name, m_category, m_price, m_deposit, m_quantity, m_description, m_additionalDayPrice)) {
            // Получаем ID нового оборудования
            QSqlQuery query = db.getEquipment();
            while (query.next()) {
                if (query.value("name").toString() == m_name &&
                    query.value("category").toString() == m_category) {
                    m_id = query.value("id").toInt();
                    break;
                }
            }
            m_createdAt = QDateTime::currentDateTime();
            m_updatedAt = m_createdAt;
            return true;
        }
    } else {
        // Обновление существующего оборудования
        if (db.updateEquipment(m_id, m_name, m_category, m_price, m_deposit, m_quantity, m_description, m_additionalDayPrice)) {
            m_updatedAt = QDateTime::currentDateTime();
            return true;
        }
    }
    
    return false;
}

bool Equipment::update()
{
    return save();
}

bool Equipment::remove()
{
    if (m_id == 0) {
        return false;
    }
    
    Database& db = Database::getInstance();
    if (db.deleteEquipment(m_id)) {
        m_id = 0;
        return true;
    }
    
    return false;
}

Equipment* Equipment::loadById(int id)
{
    Database& db = Database::getInstance();
    QSqlQuery query = db.getEquipmentById(id);
    
    if (query.next()) {
        Equipment* equipment = new Equipment();
        equipment->m_id = query.value("id").toInt();
        equipment->m_name = query.value("name").toString();
        equipment->m_category = query.value("category").toString();
        equipment->m_price = query.value("price").toDouble();
        equipment->m_deposit = query.value("deposit").toDouble();
        equipment->m_additionalDayPrice = query.value("additional_day_price").toDouble();
        equipment->m_quantity = query.value("quantity").toInt();
        equipment->m_availableQuantity = query.value("available_quantity").toInt();
        equipment->m_description = query.value("description").toString();
        equipment->m_createdAt = query.value("created_at").toDateTime();
        equipment->m_updatedAt = query.value("updated_at").toDateTime();
        return equipment;
    }
    
    return nullptr;
}

QList<Equipment*> Equipment::search(const QString& searchTerm)
{
    Database& db = Database::getInstance();
    QSqlQuery query = db.searchEquipment(searchTerm);
    
    QList<Equipment*> equipment;
    while (query.next()) {
        Equipment* item = new Equipment();
        item->m_id = query.value("id").toInt();
        item->m_name = query.value("name").toString();
        item->m_category = query.value("category").toString();
        item->m_price = query.value("price").toDouble();
        item->m_deposit = query.value("deposit").toDouble();
        item->m_additionalDayPrice = query.value("additional_day_price").toDouble();
        item->m_quantity = query.value("quantity").toInt();
        item->m_availableQuantity = query.value("available_quantity").toInt();
        item->m_description = query.value("description").toString();
        item->m_createdAt = query.value("created_at").toDateTime();
        item->m_updatedAt = query.value("updated_at").toDateTime();
        equipment.append(item);
    }
    
    return equipment;
}

QList<Equipment*> Equipment::getByCategory(const QString& category)
{
    Database& db = Database::getInstance();
    QSqlQuery query = db.getEquipment();
    
    QList<Equipment*> equipment;
    while (query.next()) {
        if (query.value("category").toString() == category) {
            Equipment* item = new Equipment();
            item->m_id = query.value("id").toInt();
            item->m_name = query.value("name").toString();
            item->m_category = query.value("category").toString();
            item->m_price = query.value("price").toDouble();
            item->m_deposit = query.value("deposit").toDouble();
            item->m_additionalDayPrice = query.value("additional_day_price").toDouble();
            item->m_quantity = query.value("quantity").toInt();
            item->m_availableQuantity = query.value("available_quantity").toInt();
            item->m_description = query.value("description").toString();
            item->m_createdAt = query.value("created_at").toDateTime();
            item->m_updatedAt = query.value("updated_at").toDateTime();
            equipment.append(item);
        }
    }
    
    return equipment;
}

QList<Equipment*> Equipment::getAll()
{
    Database& db = Database::getInstance();
    QSqlQuery query = db.getEquipment();
    
    QList<Equipment*> equipment;
    while (query.next()) {
        Equipment* item = new Equipment();
        item->m_id = query.value("id").toInt();
        item->m_name = query.value("name").toString();
        item->m_category = query.value("category").toString();
        item->m_price = query.value("price").toDouble();
        item->m_deposit = query.value("deposit").toDouble();
        item->m_additionalDayPrice = query.value("additional_day_price").toDouble();
        item->m_quantity = query.value("quantity").toInt();
        item->m_availableQuantity = query.value("available_quantity").toInt();
        item->m_description = query.value("description").toString();
        item->m_createdAt = query.value("created_at").toDateTime();
        item->m_updatedAt = query.value("updated_at").toDateTime();
        equipment.append(item);
    }
    
    return equipment;
}

QString Equipment::toString() const
{
    return QString("Equipment(id=%1, name='%2', category='%3', price=%4, deposit=%5)")
           .arg(m_id)
           .arg(m_name)
           .arg(m_category)
           .arg(m_price)
           .arg(m_deposit);
}

QString Equipment::getDisplayName() const
{
    if (m_name.isEmpty()) {
        return QString("Оборудование #%1").arg(m_id);
    }
    return m_name;
}

QString Equipment::getStatusText() const
{
    if (m_availableQuantity == 0) {
        return "Недоступно";
    } else if (m_availableQuantity == m_quantity) {
        return "Доступно";
    } else {
        return QString("Доступно %1 из %2").arg(m_availableQuantity).arg(m_quantity);
    }
}

bool Equipment::validateName() const
{
    return !m_name.trimmed().isEmpty();
}

bool Equipment::validateCategory() const
{
    return !m_category.trimmed().isEmpty();
}

bool Equipment::validatePrice() const
{
    return m_price > 0.0;
}

bool Equipment::validateDeposit() const
{
    return m_deposit >= 0.0;
}

bool Equipment::validateQuantity() const
{
    return m_quantity > 0;
} 