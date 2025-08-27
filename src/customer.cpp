#include "customer.h"
#include "database.h"
#include <QDebug>
#include <QRegularExpression>
#include <QRegularExpressionValidator>

Customer::Customer(QObject *parent)
    : QObject(parent)
    , m_id(0)
    , m_createdAt(QDateTime::currentDateTime())
    , m_updatedAt(QDateTime::currentDateTime())
{
}

Customer::Customer(int id, const QString& name, const QString& phone, const QString& email,
                  const QString& passport, const QString& address, QObject *parent)
    : QObject(parent)
    , m_id(id)
    , m_name(name)
    , m_phone(phone)
    , m_email(email)
    , m_passport(passport)
    , m_address(address)
    , m_passportIssueDate()
    , m_createdAt(QDateTime::currentDateTime())
    , m_updatedAt(QDateTime::currentDateTime())
{
}

bool Customer::isValid() const
{
    return validateName() && validatePhone() && validateEmail() && validatePassport();
}

QStringList Customer::getValidationErrors() const
{
    QStringList errors;
    
    if (!validateName()) {
        errors << "Имя не может быть пустым";
    }
    
    if (!validatePhone()) {
        errors << "Некорректный номер телефона";
    }
    
    if (!validateEmail()) {
        errors << "Некорректный email адрес";
    }
    
    if (!validatePassport()) {
        errors << "Некорректные данные паспорта";
    }
    
    return errors;
}

bool Customer::save()
{
    if (!isValid()) {
        qDebug() << "Ошибка валидации клиента:" << getValidationErrors();
        return false;
    }
    
    Database& db = Database::getInstance();
    
    if (m_id == 0) {
        // Новый клиент
        if (db.addCustomer(m_name, m_phone, m_email, m_passport, m_address, m_passportIssueDate)) {
            // Получаем ID нового клиента через lastInsertId
            QSqlQuery query(db.getDatabase());
            if (query.exec("SELECT last_insert_rowid()")) {
                if (query.next()) {
                    m_id = query.value(0).toInt();
                }
            }
            m_createdAt = QDateTime::currentDateTime();
            m_updatedAt = m_createdAt;
            return true;
        }
    } else {
        // Обновление существующего клиента
        if (db.updateCustomer(m_id, m_name, m_phone, m_email, m_passport, m_address, m_passportIssueDate)) {
            m_updatedAt = QDateTime::currentDateTime();
            return true;
        }
    }
    
    return false;
}

bool Customer::update()
{
    return save();
}

bool Customer::remove()
{
    if (m_id == 0) {
        return false;
    }
    
    Database& db = Database::getInstance();
    if (db.deleteCustomer(m_id)) {
        m_id = 0;
        return true;
    }
    
    return false;
}

Customer* Customer::loadById(int id)
{
    Database& db = Database::getInstance();
    QSqlQuery query = db.getCustomerById(id);
    
    if (query.next()) {
        Customer* customer = new Customer();
        customer->m_id = query.value("id").toInt();
        customer->m_name = query.value("name").toString();
        customer->m_phone = query.value("phone").toString();
        customer->m_email = query.value("email").toString();
        customer->m_passport = query.value("passport").toString();
        customer->m_address = query.value("address").toString();
        customer->m_passportIssueDate = query.value("passport_issue_date").toDate();
        customer->m_createdAt = query.value("created_at").toDateTime();
        customer->m_updatedAt = query.value("updated_at").toDateTime();
        return customer;
    }
    
    return nullptr;
}

QList<Customer*> Customer::search(const QString& searchTerm)
{
    Database& db = Database::getInstance();
    QSqlQuery query = db.searchCustomers(searchTerm);
    
    QList<Customer*> customers;
    while (query.next()) {
        Customer* customer = new Customer();
        customer->m_id = query.value("id").toInt();
        customer->m_name = query.value("name").toString();
        customer->m_phone = query.value("phone").toString();
        customer->m_email = query.value("email").toString();
        customer->m_passport = query.value("passport").toString();
        customer->m_address = query.value("address").toString();
        customer->m_passportIssueDate = query.value("passport_issue_date").toDate();
        customer->m_createdAt = query.value("created_at").toDateTime();
        customer->m_updatedAt = query.value("updated_at").toDateTime();
        customers.append(customer);
    }
    
    return customers;
}

QList<Customer*> Customer::getAll()
{
    Database& db = Database::getInstance();
    QSqlQuery query = db.getCustomers();
    
    QList<Customer*> customers;
    while (query.next()) {
        Customer* customer = new Customer();
        customer->m_id = query.value("id").toInt();
        customer->m_name = query.value("name").toString();
        customer->m_phone = query.value("phone").toString();
        customer->m_email = query.value("email").toString();
        customer->m_passport = query.value("passport").toString();
        customer->m_address = query.value("address").toString();
        customer->m_passportIssueDate = query.value("passport_issue_date").toDate();
        customer->m_createdAt = query.value("created_at").toDateTime();
        customer->m_updatedAt = query.value("updated_at").toDateTime();
        customers.append(customer);
    }
    
    return customers;
}

QString Customer::toString() const
{
    return QString("Customer(id=%1, name='%2', phone='%3', email='%4')")
           .arg(m_id)
           .arg(m_name)
           .arg(m_phone)
           .arg(m_email);
}

QString Customer::getDisplayName() const
{
    if (m_name.isEmpty()) {
        return QString("Клиент #%1").arg(m_id);
    }
    return m_name;
}

bool Customer::validateName() const
{
    return !m_name.trimmed().isEmpty();
}

bool Customer::validatePhone() const
{
    if (m_phone.isEmpty()) {
        return true; // Телефон не обязателен
    }
    
    // Простая валидация российского номера телефона
    QRegularExpression phoneRegex(R"(\+7\s?\(?(\d{3})\)?\s?(\d{3})-?(\d{2})-?(\d{2}))");
    return phoneRegex.match(m_phone).hasMatch();
}

bool Customer::validateEmail() const
{
    if (m_email.isEmpty()) {
        return true; // Email не обязателен
    }
    
    QRegularExpression emailRegex(R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})");
    return emailRegex.match(m_email).hasMatch();
}

bool Customer::validatePassport() const
{
    if (m_passport.isEmpty()) {
        return true; // Паспорт не обязателен
    }
    
    // Валидация российского паспорта (серия и номер)
    QRegularExpression passportRegex(R"(\d{4}\s?\d{6})");
    return passportRegex.match(m_passport).hasMatch();
} 