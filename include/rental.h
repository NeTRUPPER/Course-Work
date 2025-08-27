#ifndef RENTAL_H
#define RENTAL_H

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QDebug>
#include <QList>

class Customer;
class Equipment;

class Rental : public QObject
{
    Q_OBJECT

public:
    enum Status {
        Active,
        Completed,
        Cancelled,
        Overdue
    };
    
    explicit Rental(QObject *parent = nullptr);
    Rental(int id, Customer* customer, Equipment* equipment, int quantity,
           const QDateTime& startDate, const QDateTime& endDate,
           double totalPrice, double deposit, const QString& notes, QObject *parent = nullptr);
    
    // Getters
    int getId() const { return m_id; }
    Customer* getCustomer() const { return m_customer; }
    Equipment* getEquipment() const { return m_equipment; }
    int getQuantity() const { return m_quantity; }
    QDateTime getStartDate() const { return m_startDate; }
    QDateTime getEndDate() const { return m_endDate; }
    double getTotalPrice() const { return m_totalPrice; }
    double getDeposit() const { return m_deposit; }
    double getFinalPrice() const { return m_finalPrice; }
    double getDamageCost() const { return m_damageCost; }
    double getCleaningCost() const { return m_cleaningCost; }
    double getFinalDeposit() const { return m_finalDeposit; }
    QString getNotes() const { return m_notes; }
    QString getStatus() const { return m_status; }
    QDateTime getCreatedAt() const { return m_createdAt; }
    QDateTime getUpdatedAt() const { return m_updatedAt; }
    
    // Setters
    void setId(int id) { m_id = id; }
    void setCustomer(Customer* customer) { m_customer = customer; }
    void setEquipment(Equipment* equipment) { m_equipment = equipment; }
    void setQuantity(int quantity) { m_quantity = quantity; }
    void setStartDate(const QDateTime& date) { m_startDate = date; }
    void setEndDate(const QDateTime& date) { m_endDate = date; }
    void setTotalPrice(double price) { m_totalPrice = price; }
    void setDeposit(double deposit) { m_deposit = deposit; }
    void setFinalPrice(double price) { m_finalPrice = price; }
    void setDamageCost(double cost) { m_damageCost = cost; }
    void setCleaningCost(double cost) { m_cleaningCost = cost; }
    void setFinalDeposit(double deposit) { m_finalDeposit = deposit; }
    void setNotes(const QString& notes) { m_notes = notes; }
    void setStatus(const QString& status) { m_status = status; }
    void setCreatedAt(const QDateTime& date) { m_createdAt = date; }
    void setUpdatedAt(const QDateTime& date) { m_updatedAt = date; }
    
    // Business logic
    int getRentalDays() const;
    bool isOverdue() const;
    bool isActive() const;
    bool isCompleted() const;
    double calculateTotalPrice() const;
    double calculateDeposit() const;
    double calculateFinalPrice() const;
    double calculateDamageCost() const;
    double calculateCleaningCost() const;
    double calculateFinalDeposit() const;
    QString getStatusText() const;
    
    // Validation
    bool isValid() const;
    QStringList getValidationErrors() const;
    
    // Database operations
    bool save();
    bool update();
    bool remove();
    bool complete(double damageCost, double cleaningCost, double finalDeposit, const QString& notes);
    static Rental* loadById(int id);
    static QList<Rental*> getByCustomer(int customerId);
    static QList<Rental*> getByEquipment(int equipmentId);
    static QList<Rental*> getActive();
    static QList<Rental*> getOverdue();
    static QList<Rental*> getAll();
    
    // Utility
    QString toString() const;
    QString getDisplayName() const;

private:
    int m_id;
    Customer* m_customer;
    Equipment* m_equipment;
    int m_quantity;
    QDateTime m_startDate;
    QDateTime m_endDate;
    double m_totalPrice;
    double m_deposit;
    double m_finalPrice;
    double m_damageCost;
    double m_cleaningCost;
    double m_finalDeposit;
    QString m_notes;
    QString m_status;
    QDateTime m_createdAt;
    QDateTime m_updatedAt;
    
    bool validateCustomer() const;
    bool validateEquipment() const;
    bool validateQuantity() const;
    bool validateDates() const;
    bool validatePrices() const;
};

#endif // RENTAL_H 