#include "AdminPasswordManager.h"
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QByteArray>
#include <QSettings>

static QByteArray stretch(const QByteArray& input, const QByteArray& salt, int iterations) {
    // Простое «растяжение»: H = SHA256(salt || pwd), затем итерации H = SHA256(H || salt)
    QByteArray h = QCryptographicHash::hash(salt + input, QCryptographicHash::Sha256);
    for (int i = 0; i < iterations; ++i) {
        h = QCryptographicHash::hash(h + salt, QCryptographicHash::Sha256);
    }
    return h;
}

static QString makeRecord(const QByteArray& salt, const QByteArray& hash, int iterations) {
    return QStringLiteral("sha256$%1$%2$%3")
        .arg(iterations)
        .arg(QString::fromLatin1(salt.toBase64()))
        .arg(QString::fromLatin1(hash.toBase64()));
}

static bool parseRecord(const QString& record, int& iterOut, QByteArray& saltOut, QByteArray& hashOut) {
    const auto parts = record.split('$');
    if (parts.size() != 4 || parts[0] != QLatin1String("sha256")) return false;
    bool ok = false;
    int it = parts[1].toInt(&ok);
    if (!ok || it <= 0) return false;
    QByteArray salt = QByteArray::fromBase64(parts[2].toLatin1());
    QByteArray hash = QByteArray::fromBase64(parts[3].toLatin1());
    if (salt.isEmpty() || hash.size() != 32) return false;
    iterOut = it; saltOut = salt; hashOut = hash;
    return true;
}

AdminPasswordManager::AdminPasswordManager(QObject* p) : QObject(p) {}

QString AdminPasswordManager::loadRecord() const {
    if (!m_cache.isEmpty()) return m_cache;
    QSettings s; // org/app задайте в main()
    const QString rec = s.value("security/admin_password_record").toString();
    const_cast<AdminPasswordManager*>(this)->m_cache = rec;
    return rec;
}

void AdminPasswordManager::saveRecord(const QString& record) const {
    QSettings s;
    s.setValue("security/admin_password_record", record);
    const_cast<AdminPasswordManager*>(this)->m_cache = record;
}

bool AdminPasswordManager::hasPassword() const {
    return !loadRecord().isEmpty();
}

QString AdminPasswordManager::hashPassword(const QString& pwd, int iterations) {
    QByteArray salt(24, Qt::Uninitialized);
    auto *rng = QRandomGenerator::system();
    for (int i = 0; i < salt.size(); i += 4) {
        quint32 r = rng->generate();
        const int chunk = qMin(4, salt.size() - i);
        std::memcpy(salt.data() + i, &r, chunk);
    }
    QByteArray hash = stretch(pwd.toUtf8(), salt, iterations);
    return makeRecord(salt, hash, iterations);
}

bool AdminPasswordManager::setNewPassword(const QString& pwd) {
    const QString rec = hashPassword(pwd);
    saveRecord(rec);
    return true;
}

bool AdminPasswordManager::verifyAgainstRecord(const QString& pwd, const QString& record) {
    int it = 0; QByteArray salt, stored;
    if (!parseRecord(record, it, salt, stored)) return false;
    QByteArray calc = stretch(pwd.toUtf8(), salt, it);
    return calc == stored;
}

bool AdminPasswordManager::verifyPassword(const QString& pwd) const {
    const QString rec = loadRecord();
    if (rec.isEmpty()) return false;
    return verifyAgainstRecord(pwd, rec);
}

QString AdminPasswordManager::storedRecord() const { return loadRecord(); }
bool AdminPasswordManager::setStoredRecord(const QString& r) {
    int it=0; QByteArray salt, hash;
    if (!parseRecord(r, it, salt, hash)) return false;
    saveRecord(r);
    return true;
}