#ifndef CUSTOMER_H
#define CUSTOMER_H

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QDate>
#include <QDebug>

class Customer : public QObject
{
    Q_OBJECT

public:
    explicit Customer(QObject *parent = nullptr);
    Customer(int id, const QString& name, const QString& phone, const QString& email,
            const QString& passport, const QString& address, QObject *parent = nullptr);
    
    // Getters
    int getId() const { return m_id; }
    QString getName() const { return m_name; }
    QString getPhone() const { return m_phone; }
    QString getEmail() const { return m_email; }
    QString getPassport() const { return m_passport; }
    QString getAddress() const { return m_address; }
    QDate getPassportIssueDate() const { return m_passportIssueDate; }
    QDateTime getCreatedAt() const { return m_createdAt; }
    QDateTime getUpdatedAt() const { return m_updatedAt; }
    
    // Setters
    void setId(int id) { m_id = id; }
    void setName(const QString& name) { m_name = name; }
    void setPhone(const QString& phone) { m_phone = phone; }
    void setEmail(const QString& email) { m_email = email; }
    void setPassport(const QString& passport) { m_passport = passport; }
    void setAddress(const QString& address) { m_address = address; }
    void setPassportIssueDate(const QDate& date) { m_passportIssueDate = date; }
    
    // Validation
    bool isValid() const;
    QStringList getValidationErrors() const;
    
    // Database operations
    bool save();
    bool update();
    bool remove();
    static Customer* loadById(int id);
    static QList<Customer*> search(const QString& searchTerm);
    static QList<Customer*> getAll();
    
    // Utility
    QString toString() const;
    QString getDisplayName() const;

private:
    int m_id;
    QString m_name;
    QString m_phone;
    QString m_email;
    QString m_passport;
    QString m_address;
    QDate m_passportIssueDate;
    QDateTime m_createdAt;
    QDateTime m_updatedAt;
    
    bool validateName() const;
    bool validatePhone() const;
    bool validateEmail() const;
    bool validatePassport() const;
};

#endif // CUSTOMER_H 