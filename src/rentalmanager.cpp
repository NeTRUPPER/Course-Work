#include "rentalmanager.h"
#include "database.h"
#include <QDebug>
#include <QMessageBox>

RentalManager::RentalManager(QObject *parent)
    : QObject(parent)
{
}

RentalManager::~RentalManager()
{
}

Rental* RentalManager::createRental(Customer* customer, Equipment* equipment, int quantity,
                                   const QDateTime& startDate, const QDateTime& endDate,
                                   const QString& notes)
{
    if (!validateRentalRequest(customer, equipment, quantity, startDate, endDate)) {
        QStringList errors = getRentalValidationErrors(customer, equipment, quantity, startDate, endDate);
        qDebug() << "Ошибка валидации аренды:" << errors;
        return nullptr;
    }
    
    // Проверяем доступность оборудования
    if (!isEquipmentAvailable(equipment, startDate, endDate, quantity)) {
        qDebug() << "Оборудование недоступно в указанный период";
        return nullptr;
    }
    
    // Рассчитываем стоимость
    double totalPrice = calculateRentalPrice(equipment, calculateRentalDays(startDate, endDate));
    double deposit = calculateDeposit(equipment, quantity);
    
    // Создаем аренду
    Rental* rental = new Rental(0, customer, equipment, quantity, startDate, endDate,
                               totalPrice, deposit, notes, this);
    
    if (rental->save()) {
        // Резервируем оборудование
        reserveEquipment(equipment, quantity);
        
        emit rentalCreated(rental);
        return rental;
    } else {
        delete rental;
        return nullptr;
    }
}

bool RentalManager::completeRental(Rental* rental, double damageCost, double cleaningCost,
                                  double finalDeposit, const QString& notes)
{
    if (!rental || !rental->isActive()) {
        return false;
    }
    
    if (rental->complete(damageCost, cleaningCost, finalDeposit, notes)) {
        // Освобождаем оборудование
        releaseEquipment(rental->getEquipment(), rental->getQuantity());
        
        emit rentalCompleted(rental);
        return true;
    }
    
    return false;
}

bool RentalManager::cancelRental(Rental* rental, const QString& reason)
{
    if (!rental || !rental->isActive()) {
        return false;
    }
    
    // Освобождаем оборудование
    releaseEquipment(rental->getEquipment(), rental->getQuantity());
    
    // Обновляем статус
    rental->setStatus("cancelled");
    rental->setNotes(rental->getNotes() + "\nОтменено: " + reason);
    
    if (rental->update()) {
        emit rentalCancelled(rental);
        return true;
    }
    
    return false;
}

double RentalManager::calculateRentalPrice(Equipment* equipment, int days) const
{
    if (!equipment) {
        return 0.0;
    }
    
    return equipment->calculateRentalPrice(days);
}

double RentalManager::calculateDeposit(Equipment* equipment, int quantity) const
{
    if (!equipment) {
        return 0.0;
    }
    
    return equipment->calculateDeposit() * quantity;
}

double RentalManager::calculateFinalPrice(Rental* rental, double damageCost, double cleaningCost) const
{
    if (!rental) {
        return 0.0;
    }
    
    return rental->getTotalPrice() + damageCost + cleaningCost;
}

double RentalManager::calculateDamageCost(Equipment* equipment, double damagePercentage) const
{
    if (!equipment) {
        return 0.0;
    }
    
    return equipment->getPrice() * (damagePercentage / 100.0);
}

double RentalManager::calculateCleaningCost(Equipment* equipment, bool isDirty) const
{
    if (!equipment || !isDirty) {
        return 0.0;
    }
    
    // Фиксированная стоимость уборки
    return 500.0;
}

bool RentalManager::canRentEquipment(Equipment* equipment, int quantity) const
{
    if (!equipment) {
        return false;
    }
    
    return equipment->canRent(quantity);
}

bool RentalManager::isEquipmentAvailable(Equipment* equipment, const QDateTime& startDate,
                                        const QDateTime& endDate, int quantity) const
{
    if (!equipment || !isDateRangeValid(startDate, endDate)) {
        return false;
    }
    
    // Проверяем базовую доступность
    if (!equipment->canRent(quantity)) {
        return false;
    }
    
    // Проверяем конфликты с существующими арендами
    QList<Rental*> activeRentals = Rental::getActive();
    
    for (Rental* rental : activeRentals) {
        if (rental->getEquipment()->getId() == equipment->getId()) {
            // Проверяем пересечение периодов
            if ((startDate >= rental->getStartDate() && startDate < rental->getEndDate()) ||
                (endDate > rental->getStartDate() && endDate <= rental->getEndDate()) ||
                (startDate <= rental->getStartDate() && endDate >= rental->getEndDate())) {
                
                // Проверяем доступное количество
                int reservedQuantity = rental->getQuantity();
                int availableQuantity = equipment->getAvailableQuantity();
                
                if (availableQuantity - reservedQuantity < quantity) {
                    return false;
                }
            }
        }
    }
    
    return true;
}

QList<Rental*> RentalManager::getOverdueRentals() const
{
    return Rental::getOverdue();
}

QList<Rental*> RentalManager::getActiveRentals() const
{
    return Rental::getActive();
}

QList<Rental*> RentalManager::getRentalsByCustomer(Customer* customer) const
{
    if (!customer) {
        return QList<Rental*>();
    }
    
    return Rental::getByCustomer(customer->getId());
}

QList<Rental*> RentalManager::getRentalsByEquipment(Equipment* equipment) const
{
    if (!equipment) {
        return QList<Rental*>();
    }
    
    return Rental::getByEquipment(equipment->getId());
}

QList<Rental*> RentalManager::getRentalsByDateRange(const QDateTime& start, const QDateTime& end) const
{
    Database& db = Database::getInstance();
    QSqlQuery query = db.getRentalsByDateRange(start, end);
    
    QList<Rental*> rentals;
    while (query.next()) {
        Rental* rental = new Rental();
        rental->setId(query.value("id").toInt());
        rental->setQuantity(query.value("quantity").toInt());
        rental->setStartDate(query.value("start_date").toDateTime());
        rental->setEndDate(query.value("end_date").toDateTime());
        rental->setTotalPrice(query.value("total_price").toDouble());
        rental->setDeposit(query.value("deposit").toDouble());
        rental->setFinalPrice(query.value("final_price").toDouble());
        rental->setDamageCost(query.value("damage_cost").toDouble());
        rental->setCleaningCost(query.value("cleaning_cost").toDouble());
        rental->setFinalDeposit(query.value("final_deposit").toDouble());
        rental->setNotes(query.value("notes").toString());
        rental->setStatus(query.value("status").toString());
        rental->setCreatedAt(query.value("created_at").toDateTime());
        rental->setUpdatedAt(query.value("updated_at").toDateTime());
        
        // Загружаем связанные объекты
        int customerId = query.value("customer_id").toInt();
        int equipmentId = query.value("equipment_id").toInt();
        rental->setCustomer(Customer::loadById(customerId));
        rental->setEquipment(Equipment::loadById(equipmentId));
        
        rentals.append(rental);
    }
    
    return rentals;
}

double RentalManager::calculateTotalRevenue(const QDateTime& start, const QDateTime& end) const
{
    QList<Rental*> rentals = getRentalsByDateRange(start, end);
    double totalRevenue = 0.0;
    
    for (Rental* rental : rentals) {
        if (rental->isCompleted()) {
            totalRevenue += rental->getFinalPrice();
        } else {
            totalRevenue += rental->getTotalPrice();
        }
    }
    
    return totalRevenue;
}

double RentalManager::calculateTotalDeposits(const QDateTime& start, const QDateTime& end) const
{
    QList<Rental*> rentals = getRentalsByDateRange(start, end);
    double totalDeposits = 0.0;
    
    for (Rental* rental : rentals) {
        totalDeposits += rental->getDeposit();
    }
    
    return totalDeposits;
}

QMap<QString, int> RentalManager::getEquipmentUsageStats(const QDateTime& start, const QDateTime& end) const
{
    QList<Rental*> rentals = getRentalsByDateRange(start, end);
    QMap<QString, int> stats;
    
    for (Rental* rental : rentals) {
        if (rental->getEquipment()) {
            QString category = rental->getEquipment()->getCategory();
            stats[category] = stats.value(category, 0) + rental->getQuantity();
        }
    }
    
    return stats;
}

QMap<QString, double> RentalManager::getCustomerRevenueStats(const QDateTime& start, const QDateTime& end) const
{
    QList<Rental*> rentals = getRentalsByDateRange(start, end);
    QMap<QString, double> stats;
    
    for (Rental* rental : rentals) {
        if (rental->getCustomer()) {
            QString customerName = rental->getCustomer()->getName();
            double revenue = rental->isCompleted() ? rental->getFinalPrice() : rental->getTotalPrice();
            stats[customerName] = stats.value(customerName, 0.0) + revenue;
        }
    }
    
    return stats;
}

bool RentalManager::validateRentalRequest(Customer* customer, Equipment* equipment, int quantity,
                                         const QDateTime& startDate, const QDateTime& endDate) const
{
    return isCustomerValid(customer) && isEquipmentValid(equipment) && 
           quantity > 0 && isDateRangeValid(startDate, endDate);
}

QStringList RentalManager::getRentalValidationErrors(Customer* customer, Equipment* equipment, int quantity,
                                                    const QDateTime& startDate, const QDateTime& endDate) const
{
    QStringList errors;
    
    if (!isCustomerValid(customer)) {
        errors << "Некорректные данные клиента";
    }
    
    if (!isEquipmentValid(equipment)) {
        errors << "Некорректные данные оборудования";
    }
    
    if (quantity <= 0) {
        errors << "Количество должно быть положительным";
    }
    
    if (!isDateRangeValid(startDate, endDate)) {
        errors << "Некорректный период аренды";
    }
    
    return errors;
}

void RentalManager::checkOverdueRentals()
{
    QList<Rental*> overdueRentals = getOverdueRentals();
    
    for (Rental* rental : overdueRentals) {
        emit overdueRentalDetected(rental);
    }
}

void RentalManager::sendOverdueNotifications()
{
    QList<Rental*> overdueRentals = getOverdueRentals();
    
    for (Rental* rental : overdueRentals) {
        if (rental->getCustomer()) {
            QString message = QString("Аренда #%1 просрочена! Клиент: %2, Оборудование: %3")
                            .arg(rental->getId())
                            .arg(rental->getCustomer()->getName())
                            .arg(rental->getEquipment() ? rental->getEquipment()->getName() : "Unknown");
            
            QMessageBox::warning(nullptr, "Просроченная аренда", message);
        }
    }
}

void RentalManager::sendReturnReminders()
{
    QList<Rental*> activeRentals = getActiveRentals();
    QDateTime now = QDateTime::currentDateTime();
    
    for (Rental* rental : activeRentals) {
        // Напоминаем за день до окончания аренды
        if (rental->getEndDate().addDays(-1) <= now && rental->getEndDate() > now) {
            emit returnReminderNeeded(rental);
        }
    }
}

bool RentalManager::reserveEquipment(Equipment* equipment, int quantity)
{
    if (!equipment || quantity <= 0) {
        return false;
    }
    
    if (equipment->canRent(quantity)) {
        equipment->reserveQuantity(quantity);
        equipment->save();
        emit equipmentReserved(equipment, quantity);
        return true;
    }
    
    return false;
}

void RentalManager::releaseEquipment(Equipment* equipment, int quantity)
{
    if (!equipment || quantity <= 0) {
        return;
    }
    
    equipment->releaseQuantity(quantity);
    equipment->save();
    emit equipmentReleased(equipment, quantity);
}

bool RentalManager::updateEquipmentAvailability(Equipment* equipment, int quantity, bool reserve)
{
    if (reserve) {
        return reserveEquipment(equipment, quantity);
    } else {
        releaseEquipment(equipment, quantity);
        return true;
    }
}

QDateTime RentalManager::calculateDefaultEndDate(const QDateTime& startDate, int days) const
{
    return startDate.addDays(days);
}

int RentalManager::calculateRentalDays(const QDateTime& startDate, const QDateTime& endDate) const
{
    return startDate.daysTo(endDate);
}

bool RentalManager::isDateRangeValid(const QDateTime& startDate, const QDateTime& endDate) const
{
    return startDate.isValid() && endDate.isValid() && startDate < endDate;
}

bool RentalManager::isCustomerValid(Customer* customer) const
{
    return customer != nullptr && customer->isValid();
}

bool RentalManager::isEquipmentValid(Equipment* equipment) const
{
    return equipment != nullptr && equipment->isValid();
} 