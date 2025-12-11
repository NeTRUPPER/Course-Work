#include "customer.h"
#include "database.h"
#include "security.h"
#include <QDebug>
#include <QRegularExpression>
#include <QRegularExpressionValidator>

namespace {
// Расшифровка с автоподстановкой старых открытых значений
QString decodeField(const QVariant& v) {
    const QString raw = v.toString();
    if (raw.isEmpty()) return QString();

    // Попытка понять, похоже ли на наш формат: base64, который начинается с "GCM1"
    QByteArray candidate = QByteArray::fromBase64(raw.toLatin1(), QByteArray::Base64Option::AbortOnBase64DecodingErrors);
    bool looksEncrypted = !candidate.isEmpty() && candidate.startsWith("GCM1");

    QString decrypted;
    if (looksEncrypted) {
        decrypted = Security::decryptString(raw);
    }
    // Если не похоже или не удалось — считаем, что это legacy открытый текст
    return decrypted.isEmpty() ? raw : decrypted;
}

QDate decodeDate(const QVariant& v) {
    const QString raw = decodeField(v);
    const QDate d = QDate::fromString(raw, Qt::ISODate);
    return d.isValid() ? d : QDate();
}
} // namespace

Customer::Customer(QObject *parent)
    : QObject(parent)
    , m_id(0)
    , m_lastName()
    , m_firstName()
    , m_middleName()
    , m_createdAt(QDateTime::currentDateTime())
    , m_updatedAt(QDateTime::currentDateTime())
{
}

Customer::Customer(int id, const QString& lastName, const QString& firstName, const QString& middleName,
                  const QString& phone, const QString& email,
                  const QString& passport, const QString& address, QObject *parent)
    : QObject(parent)
    , m_id(id)
    , m_lastName(lastName)
    , m_firstName(firstName)
    , m_middleName(middleName)
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
        errors << "Фамилия и имя обязательны";
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
    const QString fullName = getDisplayName();
    
    Database& db = Database::getInstance();
    
    if (m_id == 0) {
        // Новый клиент
        if (db.addCustomer(m_lastName, m_firstName, m_middleName,
                           fullName, m_phone, m_email, m_passport, m_address, m_passportIssueDate)) {
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
        if (db.updateCustomer(m_id, m_lastName, m_firstName, m_middleName,
                              fullName, m_phone, m_email, m_passport, m_address, m_passportIssueDate)) {
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
        customer->m_lastName = decodeField(query.value("last_name"));
        customer->m_firstName = decodeField(query.value("first_name"));
        customer->m_middleName = decodeField(query.value("patronymic"));
        const QString legacyName = decodeField(query.value("name"));
        if (customer->m_lastName.isEmpty() && customer->m_firstName.isEmpty() && !legacyName.isEmpty()) {
            customer->setName(legacyName);
        }
        customer->m_phone = decodeField(query.value("phone"));
        customer->m_email = decodeField(query.value("email"));
        customer->m_passport = decodeField(query.value("passport"));
        customer->m_address = decodeField(query.value("address"));
        customer->m_passportIssueDate = decodeDate(query.value("passport_issue_date"));
        customer->m_createdAt = query.value("created_at").toDateTime();
        customer->m_updatedAt = query.value("updated_at").toDateTime();
        return customer;
    }
    
    return nullptr;
}

QList<Customer*> Customer::search(const QString& searchTerm)
{
    const QString needle = searchTerm.trimmed().toLower();
    QList<Customer*> all = getAll();
    if (needle.isEmpty()) return all;

    QList<Customer*> filtered;
    for (Customer* c : all) {
        const QString haystack = QStringList{
            c->getLastName(),
            c->getFirstName(),
            c->getMiddleName(),
            c->getPhone(),
            c->getEmail(),
            c->getPassport(),
            c->getAddress()
        }.join(' ').toLower();
        if (haystack.contains(needle)) {
            filtered.append(c);
        } else {
            delete c;
        }
    }
    return filtered;
}

QList<Customer*> Customer::getAll()
{
    Database& db = Database::getInstance();
    QSqlQuery query = db.getCustomers();
    
    QList<Customer*> customers;
    while (query.next()) {
        Customer* customer = new Customer();
        customer->m_id = query.value("id").toInt();
        customer->m_lastName = decodeField(query.value("last_name"));
        customer->m_firstName = decodeField(query.value("first_name"));
        customer->m_middleName = decodeField(query.value("patronymic"));
        const QString legacyName = decodeField(query.value("name"));
        if (customer->m_lastName.isEmpty() && customer->m_firstName.isEmpty() && !legacyName.isEmpty()) {
            customer->setName(legacyName);
        }
        customer->m_phone = decodeField(query.value("phone"));
        customer->m_email = decodeField(query.value("email"));
        customer->m_passport = decodeField(query.value("passport"));
        customer->m_address = decodeField(query.value("address"));
        customer->m_passportIssueDate = decodeDate(query.value("passport_issue_date"));
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
           .arg(getDisplayName())
           .arg(m_phone)
           .arg(m_email);
}

QString Customer::getDisplayName() const
{
    QStringList parts;
    if (!m_lastName.trimmed().isEmpty()) parts << m_lastName.trimmed();
    if (!m_firstName.trimmed().isEmpty()) parts << m_firstName.trimmed();
    if (!m_middleName.trimmed().isEmpty()) parts << m_middleName.trimmed();
    const QString full = parts.join(" ");
    return full.isEmpty() ? QString("Клиент #%1").arg(m_id) : full;
}

bool Customer::validateName() const
{
    return !m_lastName.trimmed().isEmpty() && !m_firstName.trimmed().isEmpty();
}

void Customer::setName(const QString& fullName)
{
    const QStringList parts = fullName.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    if (parts.size() > 0) m_lastName = parts.at(0);
    if (parts.size() > 1) m_firstName = parts.at(1);
    if (parts.size() > 2) m_middleName = parts.mid(2).join(" ");
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
