// ------------------------------------------------------------
// Файл: security.cpp
// Назначение: Аутентификация пользователя (мастер‑пароль),
// управление сессией и утилиты для хеширования/ключей.
// Шифрование БД отключено по требованию.
// ------------------------------------------------------------
#include "security.h"
#include <QSettings>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>
#include <QDateTime>
#include <QTimer>
#include <QCryptographicHash>
#include <QDataStream>
#include <QBuffer>
#include <QIODevice>
#include <QMessageAuthenticationCode>

Security* Security::m_instance = nullptr;

Security::Security(QObject *parent)
    : QObject(parent)
    , m_isAuthenticated(false)
    , m_isInitialized(false)
{
    m_sessionStart = QDateTime::currentDateTime();
    m_sessionToken = generateSessionToken();
}

Security::~Security()
{
    endSession();
}

void Security::initialize()
{
    if (!m_instance) {
        m_instance = new Security();
        m_instance->m_isInitialized = true;
    }
}

bool Security::authenticate()
{
    if (!m_instance) {
        qDebug() << "Ошибка: Security не инициализирован";
        return false;
    }
    
    // Проверяем, есть ли мастер-пароль
    if (!hasMasterPassword()) {
        // Запрашиваем установку мастер-пароля
        bool ok;
        QString password = QInputDialog::getText(nullptr, "Установка мастер-пароля",
                                               "Установите мастер-пароль для защиты данных:",
                                               QLineEdit::Password, "", &ok);
        if (!ok || password.isEmpty()) {
            return false;
        }
        
        QString confirmPassword = QInputDialog::getText(nullptr, "Подтверждение пароля",
                                                      "Подтвердите мастер-пароль:",
                                                      QLineEdit::Password, "", &ok);
        if (!ok || password != confirmPassword) {
            QMessageBox::warning(nullptr, "Ошибка", "Пароли не совпадают!");
            return false;
        }
        
        if (!setMasterPassword(password)) {
            QMessageBox::critical(nullptr, "Ошибка", "Не удалось установить мастер-пароль!");
            return false;
        }
        
        QMessageBox::information(nullptr, "Успех", "Мастер-пароль установлен!");
    } else {
        // Запрашиваем мастер-пароль
        bool ok;
        QString password = QInputDialog::getText(nullptr, "Аутентификация",
                                               "Введите мастер-пароль:",
                                               QLineEdit::Password, "", &ok);
        if (!ok || password.isEmpty()) {
            return false;
        }
        
        if (!verifyMasterPassword(password)) {
            QMessageBox::critical(nullptr, "Ошибка", "Неверный пароль!");
            return false;
        }
    }
    
    m_instance->m_isAuthenticated = true;
    startSession();
    return true;
}

QString Security::authenticateAndGetPassword(int maxAttempts)
{
    initialize();
    if (!hasMasterPassword()) {
        bool ok;
        QString pw1 = QInputDialog::getText(nullptr, "Установка мастер-пароля",
                                            "Установите мастер-пароль:", QLineEdit::Password, "", &ok);
        if (!ok || pw1.isEmpty()) return QString();
        QString pw2 = QInputDialog::getText(nullptr, "Подтверждение пароля",
                                            "Повторите мастер-пароль:", QLineEdit::Password, "", &ok);
        if (!ok || pw1 != pw2) {
            QMessageBox::warning(nullptr, "Ошибка", "Пароли не совпадают");
            return QString();
        }
        if (!setMasterPassword(pw1)) return QString();
        // После первичной установки сразу используем этот пароль без повторного запроса
        m_instance->m_isAuthenticated = true;
        startSession();
        return pw1;
    }
    for (int attempt = 0; attempt < maxAttempts; ++attempt) {
        bool ok;
        QString password = QInputDialog::getText(nullptr, "Аутентификация",
                                                 "Введите мастер-пароль:", QLineEdit::Password, "", &ok);
        if (!ok) return QString();
        if (verifyMasterPassword(password)) {
            m_instance->m_isAuthenticated = true;
            startSession();
            return password;
        }
        QMessageBox::critical(nullptr, "Ошибка", "Неверный пароль");
    }
    return QString();
}

bool Security::changePassword()
{
    if (!m_instance || !m_instance->m_isAuthenticated) {
        return false;
    }
    
    bool ok;
    QString oldPassword = QInputDialog::getText(nullptr, "Смена пароля",
                                               "Введите текущий пароль:",
                                               QLineEdit::Password, "", &ok);
    if (!ok || !verifyMasterPassword(oldPassword)) {
        QMessageBox::warning(nullptr, "Ошибка", "Неверный текущий пароль!");
        return false;
    }
    
    QString newPassword = QInputDialog::getText(nullptr, "Новый пароль",
                                               "Введите новый пароль:",
                                               QLineEdit::Password, "", &ok);
    if (!ok || newPassword.isEmpty()) {
        return false;
    }
    
    QString confirmPassword = QInputDialog::getText(nullptr, "Подтверждение",
                                                   "Подтвердите новый пароль:",
                                                   QLineEdit::Password, "", &ok);
    if (!ok || newPassword != confirmPassword) {
        QMessageBox::warning(nullptr, "Ошибка", "Пароли не совпадают!");
        return false;
    }
    
    if (setMasterPassword(newPassword)) {
        QMessageBox::information(nullptr, "Успех", "Пароль изменен!");
        return true;
    } else {
        QMessageBox::critical(nullptr, "Ошибка", "Не удалось изменить пароль!");
        return false;
    }
}

bool Security::isAuthenticated()
{
    return m_instance && m_instance->m_isAuthenticated && m_instance->isSessionValid();
}

void Security::logout()
{
    if (m_instance) {
        m_instance->endSession();
        m_instance->m_isAuthenticated = false;
    }
}

QByteArray Security::encryptData(const QByteArray& data)
{
    if (!m_instance || !m_instance->m_isAuthenticated) {
        return QByteArray();
    }
    
    // Простое XOR шифрование (в реальном проекте используйте более надежные алгоритмы)
    QByteArray encrypted = data;
    QByteArray key = m_instance->m_encryptionKey;
    
    for (int i = 0; i < encrypted.size(); ++i) {
        encrypted[i] = encrypted[i] ^ key[i % key.size()];
    }
    
    return encrypted;
}

QByteArray Security::decryptData(const QByteArray& encryptedData)
{
    if (!m_instance || !m_instance->m_isAuthenticated) {
        return QByteArray();
    }
    
    // XOR дешифрование
    QByteArray decrypted = encryptedData;
    QByteArray key = m_instance->m_encryptionKey;
    
    for (int i = 0; i < decrypted.size(); ++i) {
        decrypted[i] = decrypted[i] ^ key[i % key.size()];
    }
    
    return decrypted;
}

QString Security::encryptString(const QString& text)
{
    QByteArray data = text.toUtf8();
    QByteArray encrypted = encryptData(data);
    return QString::fromLatin1(encrypted.toBase64());
}

QString Security::decryptString(const QString& encryptedText)
{
    QByteArray encrypted = QByteArray::fromBase64(encryptedText.toLatin1());
    QByteArray decrypted = decryptData(encrypted);
    return QString::fromUtf8(decrypted);
}

bool Security::setMasterPassword(const QString& password)
{
    if (!m_instance) {
        return false;
    }
    
    QString salt = generateSalt();
    QString hash = hashPassword(password, salt);
    
    QSettings settings;
    settings.setValue("security/master_password_hash", hash);
    settings.setValue("security/salt", salt);
    // Ключ шифрования больше не запрашиваем отдельно
    m_instance->m_encryptionKey.clear();
    
    return true;
}

bool Security::verifyMasterPassword(const QString& password)
{
    if (!m_instance) {
        return false;
    }
    
    QString storedHash = getMasterPasswordHash();
    QString salt = QSettings().value("security/salt").toString();
    
    if (storedHash.isEmpty() || salt.isEmpty()) {
        return false;
    }
    
    QString hash = hashPassword(password, salt);
    return hash == storedHash;
}

bool Security::hasMasterPassword()
{
    return !getMasterPasswordHash().isEmpty();
}

void Security::startSession()
{
    if (!m_instance) {
        return;
    }
    
    m_instance->m_sessionStart = QDateTime::currentDateTime();
    m_instance->m_sessionToken = generateSessionToken();
    
    // Устанавливаем таймер для автоматического завершения сессии
    QTimer::singleShot(3600000, m_instance, []() { // 1 час
        if (Security::m_instance && Security::m_instance->isSessionValid()) {
            Security::m_instance->endSession();
            QMessageBox::warning(nullptr, "Сессия завершена", 
                               "Сессия автоматически завершена для безопасности.");
        }
    });
}

void Security::endSession()
{
    if (!m_instance) {
        return;
    }
    
    m_instance->m_sessionToken.clear();
    m_instance->m_sessionStart = QDateTime();
    m_instance->m_permissions.clear();
}

bool Security::isSessionValid()
{
    if (!m_instance) {
        return false;
    }
    
    // Проверяем, что сессия не старше 1 часа
    return m_instance->m_sessionStart.isValid() && 
           m_instance->m_sessionStart.secsTo(QDateTime::currentDateTime()) < 3600;
}

bool Security::hasPermission(const QString& permission)
{
    if (!m_instance || !m_instance->m_isAuthenticated) {
        return false;
    }
    
    return m_instance->m_permissions.contains(permission);
}

void Security::grantPermission(const QString& permission)
{
    if (m_instance && m_instance->m_isAuthenticated) {
        m_instance->m_permissions.append(permission);
    }
}

void Security::revokePermission(const QString& permission)
{
    if (m_instance) {
        m_instance->m_permissions.removeAll(permission);
    }
}

QString Security::getMasterPasswordHash()
{
    return QSettings().value("security/master_password_hash").toString();
}

void Security::setMasterPasswordHash(const QString& hash)
{
    QSettings().setValue("security/master_password_hash", hash);
}

QString Security::generateSalt()
{
    const QString chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    QString salt;
    for (int i = 0; i < 32; ++i) {
        salt += chars[QRandomGenerator::global()->bounded(chars.length())];
    }
    return salt;
}

QString Security::hashPassword(const QString& password, const QString& salt)
{
    QByteArray data = (password + salt).toUtf8();
    return QString(QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex());
}

QByteArray Security::generateEncryptionKey()
{
    // Устарело: ключ теперь выводится PBKDF2 при открытии БД
    return QByteArray();
}

QByteArray Security::deriveKeyFromPassword(const QString& password, const QByteArray& salt, int iterations)
{
    // PBKDF2-HMAC-SHA256
    QByteArray key(32, 0);
    int blocks = (key.size() + 31) / 32;
    for (int block = 1; block <= blocks; ++block) {
        QByteArray u;
        QByteArray t(32, 0);
        // U1 = HMAC(password, salt || INT(block))
        QByteArray data = salt;
        data.append(char((block >> 24) & 0xFF));
        data.append(char((block >> 16) & 0xFF));
        data.append(char((block >> 8) & 0xFF));
        data.append(char(block & 0xFF));
        u = QMessageAuthenticationCode::hash(data, password.toUtf8(), QCryptographicHash::Sha256);
        t = u;
        for (int i = 1; i < iterations; ++i) {
            u = QMessageAuthenticationCode::hash(u, password.toUtf8(), QCryptographicHash::Sha256);
            for (int j = 0; j < 32; ++j) t[j] = t[j] ^ u[j];
        }
        for (int k = 0; k < 32 && (k + (block - 1) * 32) < key.size(); ++k) {
            key[k + (block - 1) * 32] = t[k];
        }
    }
    return key;
}

QString Security::generateSessionToken()
{
    const QString chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    QString token;
    for (int i = 0; i < 32; ++i) {
        token += chars[QRandomGenerator::global()->bounded(chars.length())];
    }
    return token;
} 