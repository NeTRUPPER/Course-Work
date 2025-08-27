#ifndef EQUIPMENT_H
#define EQUIPMENT_H

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QDebug>

class Equipment : public QObject
{
    Q_OBJECT

public:
    explicit Equipment(QObject *parent = nullptr);
    Equipment(int id, const QString& name, const QString& category, double price,
              double deposit, int quantity, const QString& description, QObject *parent = nullptr);
    
    // Getters
    int getId() const { return m_id; }
    QString getName() const { return m_name; }
    QString getCategory() const { return m_category; }
    double getPrice() const { return m_price; }
    double getAdditionalDayPrice() const { return m_additionalDayPrice; }
    double getDeposit() const { return m_deposit; }
    int getQuantity() const { return m_quantity; }
    int getAvailableQuantity() const { return m_availableQuantity; }
    QString getDescription() const { return m_description; }
    QDateTime getCreatedAt() const { return m_createdAt; }
    QDateTime getUpdatedAt() const { return m_updatedAt; }
    
    // Setters
    void setId(int id) { m_id = id; }
    void setName(const QString& name) { m_name = name; }
    void setCategory(const QString& category) { m_category = category; }
    void setPrice(double price) { m_price = price; }
    void setAdditionalDayPrice(double price) { m_additionalDayPrice = price; }
    void setDeposit(double deposit) { m_deposit = deposit; }
    void setQuantity(int quantity) { m_quantity = quantity; }
    void setAvailableQuantity(int quantity) { m_availableQuantity = quantity; }
    void setDescription(const QString& description) { m_description = description; }
    
    // Business logic
    bool isAvailable() const;
    bool canRent(int quantity) const;
    double calculateRentalPrice(int days) const;
    double calculateDeposit() const;
    void reserveQuantity(int quantity);
    void releaseQuantity(int quantity);
    
    // Validation
    bool isValid() const;
    QStringList getValidationErrors() const;
    
    // Database operations
    bool save();
    bool update();
    bool remove();
    static Equipment* loadById(int id);
    static QList<Equipment*> search(const QString& searchTerm);
    static QList<Equipment*> getByCategory(const QString& category);
    static QList<Equipment*> getAll();
    
    // Utility
    QString toString() const;
    QString getDisplayName() const;
    QString getStatusText() const;

private:
    int m_id;
    QString m_name;
    QString m_category;
    double m_price;
    double m_additionalDayPrice;
    double m_deposit;
    int m_quantity;
    int m_availableQuantity;
    QString m_description;
    QDateTime m_createdAt;
    QDateTime m_updatedAt;
    
    bool validateName() const;
    bool validateCategory() const;
    bool validatePrice() const;
    bool validateDeposit() const;
    bool validateQuantity() const;
};

#endif // EQUIPMENT_H 