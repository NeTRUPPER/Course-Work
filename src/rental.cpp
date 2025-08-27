#include "rental.h"
#include "database.h"
#include "customer.h"
#include "equipment.h"
#include <QDebug>

Rental::Rental(QObject *parent)
    : QObject(parent)
    , m_id(0)
    , m_customer(nullptr)
    , m_equipment(nullptr)
    , m_quantity(1)
    , m_totalPrice(0.0)
    , m_deposit(0.0)
    , m_finalPrice(0.0)
    , m_damageCost(0.0)
    , m_cleaningCost(0.0)
    , m_finalDeposit(0.0)
    , m_status("active")
    , m_createdAt(QDateTime::currentDateTime())
    , m_updatedAt(QDateTime::currentDateTime())
{
}

Rental::Rental(int id, Customer* customer, Equipment* equipment, int quantity,
               const QDateTime& startDate, const QDateTime& endDate,
               double totalPrice, double deposit, const QString& notes, QObject *parent)
    : QObject(parent)
    , m_id(id)
    , m_customer(customer)
    , m_equipment(equipment)
    , m_quantity(quantity)
    , m_startDate(startDate)
    , m_endDate(endDate)
    , m_totalPrice(totalPrice)
    , m_deposit(deposit)
    , m_finalPrice(0.0)
    , m_damageCost(0.0)
    , m_cleaningCost(0.0)
    , m_finalDeposit(0.0)
    , m_notes(notes)
    , m_status("active")
    , m_createdAt(QDateTime::currentDateTime())
    , m_updatedAt(QDateTime::currentDateTime())
{
}

int Rental::getRentalDays() const
{
    int days = m_startDate.daysTo(m_endDate);
    int result = qMax(days, 1);
    return result;
}

bool Rental::isOverdue() const
{
    return m_status == "active" && QDateTime::currentDateTime() > m_endDate;
}

bool Rental::isActive() const
{
    return m_status == "active";
}

bool Rental::isCompleted() const
{
    return m_status == "completed";
}

double Rental::calculateTotalPrice() const
{
    if (!m_equipment) {
        return 0.0;
    }
    
    int days = getRentalDays();
    double equipmentPrice = m_equipment->calculateRentalPrice(days);
    double totalPrice = equipmentPrice * m_quantity;
    
    return totalPrice;
}

double Rental::calculateDeposit() const
{
    if (!m_equipment) {
        return 0.0;
    }
    
    double deposit = m_equipment->calculateDeposit() * m_quantity;
    
    return deposit;
}

double Rental::calculateFinalPrice() const
{
    return m_totalPrice + m_damageCost + m_cleaningCost;
}

double Rental::calculateDamageCost() const
{
    if (!m_equipment) {
        return 0.0;
    }
    
    // Расчет стоимости ущерба (процент от стоимости оборудования)
    return m_equipment->getPrice() * 0.1; // 10% от стоимости
}

double Rental::calculateCleaningCost() const
{
    if (!m_equipment) {
        return 0.0;
    }
    
    // Фиксированная стоимость уборки
    return 500.0;
}

double Rental::calculateFinalDeposit() const
{
    return m_deposit - m_damageCost - m_cleaningCost;
}

QString Rental::getStatusText() const
{
    if (m_status == "active") {
        if (isOverdue()) {
            return "Просрочено";
        } else {
            return "Активна";
        }
    } else if (m_status == "completed") {
        return "Завершена";
    } else if (m_status == "cancelled") {
        return "Отменена";
    }
    
    return "Неизвестно";
}

bool Rental::isValid() const
{
    return validateCustomer() && validateEquipment() && validateQuantity() && 
           validateDates() && validatePrices();
}

QStringList Rental::getValidationErrors() const
{
    QStringList errors;
    
    if (!validateCustomer()) {
        errors << "Клиент не выбран";
    }
    
    if (!validateEquipment()) {
        errors << "Оборудование не выбрано";
    }
    
    if (!validateQuantity()) {
        errors << "Количество должно быть положительным";
    }
    
    if (!validateDates()) {
        errors << "Некорректные даты аренды";
    }
    
    if (!validatePrices()) {
        errors << "Некорректные цены";
    }
    
    return errors;
}

bool Rental::save()
{
    if (!isValid()) {
        qDebug() << "Ошибка валидации аренды:" << getValidationErrors();
        return false;
    }
    
    Database& db = Database::getInstance();
    
    if (m_id == 0) {
        // Новая аренда
        if (db.addRental(m_customer->getId(), m_equipment->getId(), m_quantity,
                         m_startDate, m_endDate, m_totalPrice, m_deposit, m_notes)) {
            // Получаем ID новой аренды
            QSqlQuery query = db.getRentals();
            while (query.next()) {
                if (query.value("customer_id").toInt() == m_customer->getId() &&
                    query.value("equipment_id").toInt() == m_equipment->getId() &&
                    query.value("start_date").toDateTime() == m_startDate) {
                    m_id = query.value("id").toInt();
                    break;
                }
            }
            m_createdAt = QDateTime::currentDateTime();
            m_updatedAt = m_createdAt;
            
            // Резервируем оборудование
            m_equipment->reserveQuantity(m_quantity);
            m_equipment->save();
            
            return true;
        }
    } else {
        // Обновление существующей аренды
        if (db.updateRental(m_id, m_endDate, m_finalPrice, m_status, m_notes)) {
            m_updatedAt = QDateTime::currentDateTime();
            return true;
        }
    }
    
    return false;
}

bool Rental::update()
{
    return save();
}

bool Rental::remove()
{
    if (m_id == 0) {
        return false;
    }
    
    // Освобождаем оборудование
    if (m_equipment) {
        m_equipment->releaseQuantity(m_quantity);
        m_equipment->save();
    }
    
    Database& db = Database::getInstance();
    if (db.deleteRental(m_id)) {
        m_id = 0;
        return true;
    }
    
    return false;
}

bool Rental::complete(double damageCost, double cleaningCost, double finalDeposit, const QString& notes)
{
    if (m_id == 0) {
        return false;
    }
    
    Database& db = Database::getInstance();
    if (db.completeRental(m_id, damageCost, cleaningCost, finalDeposit, notes)) {
        m_damageCost = damageCost;
        m_cleaningCost = cleaningCost;
        m_finalDeposit = finalDeposit;
        m_finalPrice = calculateFinalPrice();
        m_status = "completed";
        m_notes = notes;
        m_updatedAt = QDateTime::currentDateTime();
        
        // Освобождаем оборудование
        if (m_equipment) {
            m_equipment->releaseQuantity(m_quantity);
            m_equipment->save();
        }
        
        return true;
    }
    
    return false;
}

Rental* Rental::loadById(int id)
{
    Database& db = Database::getInstance();
    QSqlQuery query = db.getRentalById(id);
    
    if (query.next()) {
        Rental* rental = new Rental();
        rental->m_id = query.value("id").toInt();
        rental->m_quantity = query.value("quantity").toInt();
        rental->m_startDate = query.value("start_date").toDateTime();
        rental->m_endDate = query.value("end_date").toDateTime();
        rental->m_totalPrice = query.value("total_price").toDouble();
        rental->m_deposit = query.value("deposit").toDouble();
        rental->m_finalPrice = query.value("final_price").toDouble();
        rental->m_damageCost = query.value("damage_cost").toDouble();
        rental->m_cleaningCost = query.value("cleaning_cost").toDouble();
        rental->m_finalDeposit = query.value("final_deposit").toDouble();
        rental->m_notes = query.value("notes").toString();
        rental->m_status = query.value("status").toString();
        rental->m_createdAt = query.value("created_at").toDateTime();
        rental->m_updatedAt = query.value("updated_at").toDateTime();
        
        // Загружаем связанные объекты
        int customerId = query.value("customer_id").toInt();
        int equipmentId = query.value("equipment_id").toInt();
        rental->m_customer = Customer::loadById(customerId);
        rental->m_equipment = Equipment::loadById(equipmentId);
        
        return rental;
    }
    
    return nullptr;
}

QList<Rental*> Rental::getByCustomer(int customerId)
{
    Database& db = Database::getInstance();
    QSqlQuery query = db.getRentalsByCustomer(customerId);
    
    QList<Rental*> rentals;
    while (query.next()) {
        Rental* rental = new Rental();
        rental->m_id = query.value("id").toInt();
        rental->m_quantity = query.value("quantity").toInt();
        rental->m_startDate = query.value("start_date").toDateTime();
        rental->m_endDate = query.value("end_date").toDateTime();
        rental->m_totalPrice = query.value("total_price").toDouble();
        rental->m_deposit = query.value("deposit").toDouble();
        rental->m_finalPrice = query.value("final_price").toDouble();
        rental->m_damageCost = query.value("damage_cost").toDouble();
        rental->m_cleaningCost = query.value("cleaning_cost").toDouble();
        rental->m_finalDeposit = query.value("final_deposit").toDouble();
        rental->m_notes = query.value("notes").toString();
        rental->m_status = query.value("status").toString();
        rental->m_createdAt = query.value("created_at").toDateTime();
        rental->m_updatedAt = query.value("updated_at").toDateTime();
        
        // Загружаем связанные объекты
        int equipmentId = query.value("equipment_id").toInt();
        rental->m_customer = Customer::loadById(customerId);
        rental->m_equipment = Equipment::loadById(equipmentId);
        
        rentals.append(rental);
    }
    
    return rentals;
}

QList<Rental*> Rental::getByEquipment(int equipmentId)
{
    Database& db = Database::getInstance();
    QSqlQuery query = db.getRentals();
    
    QList<Rental*> rentals;
    while (query.next()) {
        if (query.value("equipment_id").toInt() == equipmentId) {
            Rental* rental = new Rental();
            rental->m_id = query.value("id").toInt();
            rental->m_quantity = query.value("quantity").toInt();
            rental->m_startDate = query.value("start_date").toDateTime();
            rental->m_endDate = query.value("end_date").toDateTime();
            rental->m_totalPrice = query.value("total_price").toDouble();
            rental->m_deposit = query.value("deposit").toDouble();
            rental->m_finalPrice = query.value("final_price").toDouble();
            rental->m_damageCost = query.value("damage_cost").toDouble();
            rental->m_cleaningCost = query.value("cleaning_cost").toDouble();
            rental->m_finalDeposit = query.value("final_deposit").toDouble();
            rental->m_notes = query.value("notes").toString();
            rental->m_status = query.value("status").toString();
            rental->m_createdAt = query.value("created_at").toDateTime();
            rental->m_updatedAt = query.value("updated_at").toDateTime();
            
            // Загружаем связанные объекты
            int customerId = query.value("customer_id").toInt();
            rental->m_customer = Customer::loadById(customerId);
            rental->m_equipment = Equipment::loadById(equipmentId);
            
            rentals.append(rental);
        }
    }
    
    return rentals;
}

QList<Rental*> Rental::getActive()
{
    Database& db = Database::getInstance();
    QSqlQuery query = db.getActiveRentals();
    
    QList<Rental*> rentals;
    while (query.next()) {
        Rental* rental = new Rental();
        rental->m_id = query.value("id").toInt();
        rental->m_quantity = query.value("quantity").toInt();
        rental->m_startDate = query.value("start_date").toDateTime();
        rental->m_endDate = query.value("end_date").toDateTime();
        rental->m_totalPrice = query.value("total_price").toDouble();
        rental->m_deposit = query.value("deposit").toDouble();
        rental->m_finalPrice = query.value("final_price").toDouble();
        rental->m_damageCost = query.value("damage_cost").toDouble();
        rental->m_cleaningCost = query.value("cleaning_cost").toDouble();
        rental->m_finalDeposit = query.value("final_deposit").toDouble();
        rental->m_notes = query.value("notes").toString();
        rental->m_status = query.value("status").toString();
        rental->m_createdAt = query.value("created_at").toDateTime();
        rental->m_updatedAt = query.value("updated_at").toDateTime();
        
        // Загружаем связанные объекты
        int customerId = query.value("customer_id").toInt();
        int equipmentId = query.value("equipment_id").toInt();
        rental->m_customer = Customer::loadById(customerId);
        rental->m_equipment = Equipment::loadById(equipmentId);
        
        rentals.append(rental);
    }
    
    return rentals;
}

QList<Rental*> Rental::getOverdue()
{
    QList<Rental*> activeRentals = getActive();
    QList<Rental*> overdueRentals;
    
    for (Rental* rental : activeRentals) {
        if (rental->isOverdue()) {
            overdueRentals.append(rental);
        }
    }
    
    return overdueRentals;
}

QList<Rental*> Rental::getAll()
{
    Database& db = Database::getInstance();
    QSqlQuery query = db.getRentals();
    
    QList<Rental*> rentals;
    while (query.next()) {
        Rental* rental = new Rental();
        rental->m_id = query.value("id").toInt();
        rental->m_quantity = query.value("quantity").toInt();
        rental->m_startDate = query.value("start_date").toDateTime();
        rental->m_endDate = query.value("end_date").toDateTime();
        rental->m_totalPrice = query.value("total_price").toDouble();
        rental->m_deposit = query.value("deposit").toDouble();
        rental->m_finalPrice = query.value("final_price").toDouble();
        rental->m_damageCost = query.value("damage_cost").toDouble();
        rental->m_cleaningCost = query.value("cleaning_cost").toDouble();
        rental->m_finalDeposit = query.value("final_deposit").toDouble();
        rental->m_notes = query.value("notes").toString();
        rental->m_status = query.value("status").toString();
        rental->m_createdAt = query.value("created_at").toDateTime();
        rental->m_updatedAt = query.value("updated_at").toDateTime();
        
        // Загружаем связанные объекты
        int customerId = query.value("customer_id").toInt();
        int equipmentId = query.value("equipment_id").toInt();
        rental->m_customer = Customer::loadById(customerId);
        rental->m_equipment = Equipment::loadById(equipmentId);
        
        rentals.append(rental);
    }
    
    return rentals;
}

QString Rental::toString() const
{
    return QString("Rental(id=%1, customer=%2, equipment=%3, quantity=%4, status=%5)")
           .arg(m_id)
           .arg(m_customer ? m_customer->getName() : "Unknown")
           .arg(m_equipment ? m_equipment->getName() : "Unknown")
           .arg(m_quantity)
           .arg(m_status);
}

QString Rental::getDisplayName() const
{
    QString customerName = m_customer ? m_customer->getName() : "Unknown";
    QString equipmentName = m_equipment ? m_equipment->getName() : "Unknown";
    return QString("Аренда #%1: %2 - %3").arg(m_id).arg(customerName).arg(equipmentName);
}

bool Rental::validateCustomer() const
{
    return m_customer != nullptr && m_customer->isValid();
}

bool Rental::validateEquipment() const
{
    return m_equipment != nullptr && m_equipment->isValid();
}

bool Rental::validateQuantity() const
{
    return m_quantity > 0;
}

bool Rental::validateDates() const
{
    return m_startDate.isValid() && m_endDate.isValid() && m_startDate < m_endDate;
}

bool Rental::validatePrices() const
{
    return m_totalPrice >= 0.0 && m_deposit >= 0.0;
} 