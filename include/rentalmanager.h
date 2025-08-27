#ifndef RENTALMANAGER_H
#define RENTALMANAGER_H

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QList>
#include <QDebug>
#include "customer.h"
#include "equipment.h"
#include "rental.h"

class RentalManager : public QObject
{
    Q_OBJECT

public:
    explicit RentalManager(QObject *parent = nullptr);
    ~RentalManager();
    
    // Rental operations
    Rental* createRental(Customer* customer, Equipment* equipment, int quantity,
                        const QDateTime& startDate, const QDateTime& endDate,
                        const QString& notes = "");
    bool completeRental(Rental* rental, double damageCost = 0.0, 
                       double cleaningCost = 0.0, double finalDeposit = 0.0,
                       const QString& notes = "");
    bool cancelRental(Rental* rental, const QString& reason = "");
    
    // Price calculations
    double calculateRentalPrice(Equipment* equipment, int days) const;
    double calculateDeposit(Equipment* equipment, int quantity) const;
    double calculateFinalPrice(Rental* rental, double damageCost, double cleaningCost) const;
    double calculateDamageCost(Equipment* equipment, double damagePercentage) const;
    double calculateCleaningCost(Equipment* equipment, bool isDirty) const;
    
    // Business logic
    bool canRentEquipment(Equipment* equipment, int quantity) const;
    bool isEquipmentAvailable(Equipment* equipment, const QDateTime& startDate, 
                            const QDateTime& endDate, int quantity) const;
    QList<Rental*> getOverdueRentals() const;
    QList<Rental*> getActiveRentals() const;
    QList<Rental*> getRentalsByCustomer(Customer* customer) const;
    QList<Rental*> getRentalsByEquipment(Equipment* equipment) const;
    
    // Reports
    QList<Rental*> getRentalsByDateRange(const QDateTime& start, const QDateTime& end) const;
    double calculateTotalRevenue(const QDateTime& start, const QDateTime& end) const;
    double calculateTotalDeposits(const QDateTime& start, const QDateTime& end) const;
    QMap<QString, int> getEquipmentUsageStats(const QDateTime& start, const QDateTime& end) const;
    QMap<QString, double> getCustomerRevenueStats(const QDateTime& start, const QDateTime& end) const;
    
    // Validation
    bool validateRentalRequest(Customer* customer, Equipment* equipment, int quantity,
                              const QDateTime& startDate, const QDateTime& endDate) const;
    QStringList getRentalValidationErrors(Customer* customer, Equipment* equipment, int quantity,
                                         const QDateTime& startDate, const QDateTime& endDate) const;
    
    // Notifications
    void checkOverdueRentals();
    void sendOverdueNotifications();
    void sendReturnReminders();

signals:
    void rentalCreated(Rental* rental);
    void rentalCompleted(Rental* rental);
    void rentalCancelled(Rental* rental);
    void equipmentReserved(Equipment* equipment, int quantity);
    void equipmentReleased(Equipment* equipment, int quantity);
    void overdueRentalDetected(Rental* rental);
    void returnReminderNeeded(Rental* rental);

private:
    bool reserveEquipment(Equipment* equipment, int quantity);
    void releaseEquipment(Equipment* equipment, int quantity);
    bool updateEquipmentAvailability(Equipment* equipment, int quantity, bool reserve);
    QDateTime calculateDefaultEndDate(const QDateTime& startDate, int days = 1) const;
    int calculateRentalDays(const QDateTime& startDate, const QDateTime& endDate) const;
    bool isDateRangeValid(const QDateTime& startDate, const QDateTime& endDate) const;
    bool isCustomerValid(Customer* customer) const;
    bool isEquipmentValid(Equipment* equipment) const;
};

#endif // RENTALMANAGER_H 