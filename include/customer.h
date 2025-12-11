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
    Customer(int id, const QString& lastName, const QString& firstName, const QString& middleName,
            const QString& phone, const QString& email,
            const QString& passport, const QString& address, QObject *parent = nullptr);
    
    // Getters
    int getId() const { return m_id; }
    QString getLastName() const { return m_lastName; }
    QString getFirstName() const { return m_firstName; }
    QString getMiddleName() const { return m_middleName; }
    QString getName() const { return getDisplayName(); }
    QString getPhone() const { return m_phone; }
    QString getEmail() const { return m_email; }
    QString getPassport() const { return m_passport; }
    QString getAddress() const { return m_address; }
    QDate getPassportIssueDate() const { return m_passportIssueDate; }
    QDateTime getCreatedAt() const { return m_createdAt; }
    QDateTime getUpdatedAt() const { return m_updatedAt; }
    
    // Setters
    void setId(int id) { m_id = id; }
    void setLastName(const QString& v) { m_lastName = v; }
    void setFirstName(const QString& v) { m_firstName = v; }
    void setMiddleName(const QString& v) { m_middleName = v; }
    void setName(const QString& fullName); // разбор ФИО по пробелам
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
    QString m_lastName;
    QString m_firstName;
    QString m_middleName;
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
