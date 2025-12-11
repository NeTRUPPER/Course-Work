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
#include <QFile>
#include <QFileInfo>
#include <openssl/evp.h>
#include <openssl/rand.h>

Security* Security::m_instance = nullptr;
int Security::s_failedAttempts = 0;
QDateTime Security::s_lastFailure;

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

    static const int IV_LEN = 12;   // рекомендовано для GCM
    static const int TAG_LEN = 16;  // 128-bit tag
    const QByteArray key = m_instance->m_encryptionKey;
    if (key.size() < 32) return QByteArray();

    QByteArray iv(IV_LEN, 0);
    if (RAND_bytes(reinterpret_cast<unsigned char*>(iv.data()), IV_LEN) != 1) {
        qWarning() << "Не удалось сгенерировать IV";
        return QByteArray();
    }

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return QByteArray();

    QByteArray ciphertext(data.size() + EVP_MAX_BLOCK_LENGTH, 0);
    int outLen = 0, totalLen = 0;
    QByteArray tag(TAG_LEN, 0);

    bool ok = true;
    ok = ok && EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) == 1;
    ok = ok && EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, IV_LEN, nullptr) == 1;
    ok = ok && EVP_EncryptInit_ex(ctx, nullptr, nullptr,
                                  reinterpret_cast<const unsigned char*>(key.constData()),
                                  reinterpret_cast<const unsigned char*>(iv.constData())) == 1;
    if (ok) {
        ok = EVP_EncryptUpdate(ctx,
                               reinterpret_cast<unsigned char*>(ciphertext.data()), &outLen,
                               reinterpret_cast<const unsigned char*>(data.constData()),
                               data.size()) == 1;
        totalLen = outLen;
    }
    if (ok) {
        ok = EVP_EncryptFinal_ex(ctx,
                                 reinterpret_cast<unsigned char*>(ciphertext.data()) + totalLen,
                                 &outLen) == 1;
        totalLen += outLen;
    }
    if (ok) {
        ok = EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, TAG_LEN, tag.data()) == 1;
    }

    EVP_CIPHER_CTX_free(ctx);

    if (!ok) {
        qWarning() << "Ошибка AES-GCM при шифровании";
        return QByteArray();
    }

    ciphertext.resize(totalLen);

    // Формат: "GCM1" + IV + TAG + CIPHERTEXT
    QByteArray out;
    out.reserve(4 + IV_LEN + TAG_LEN + ciphertext.size());
    out.append("GCM1", 4);
    out.append(iv);
    out.append(tag);
    out.append(ciphertext);
    return out;
}

QByteArray Security::decryptData(const QByteArray& encryptedData)
{
    if (!m_instance || !m_instance->m_isAuthenticated) {
        return QByteArray();
    }

    static const int IV_LEN = 12;
    static const int TAG_LEN = 16;
    const QByteArray key = m_instance->m_encryptionKey;
    if (key.size() < 32) return QByteArray();

    if (encryptedData.size() < 4 + IV_LEN + TAG_LEN) return QByteArray();
    if (encryptedData.left(4) != QByteArray("GCM1", 4)) return QByteArray();

    const QByteArray iv = encryptedData.mid(4, IV_LEN);
    const QByteArray tag = encryptedData.mid(4 + IV_LEN, TAG_LEN);
    const QByteArray cipher = encryptedData.mid(4 + IV_LEN + TAG_LEN);

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return QByteArray();

    QByteArray plain(cipher.size(), 0);
    int outLen = 0, totalLen = 0;
    bool ok = true;

    ok = ok && EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) == 1;
    ok = ok && EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, IV_LEN, nullptr) == 1;
    ok = ok && EVP_DecryptInit_ex(ctx, nullptr, nullptr,
                                  reinterpret_cast<const unsigned char*>(key.constData()),
                                  reinterpret_cast<const unsigned char*>(iv.constData())) == 1;
    if (ok) {
        ok = EVP_DecryptUpdate(ctx,
                               reinterpret_cast<unsigned char*>(plain.data()), &outLen,
                               reinterpret_cast<const unsigned char*>(cipher.constData()),
                               cipher.size()) == 1;
        totalLen = outLen;
    }
    if (ok) {
        ok = EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, TAG_LEN,
                                 const_cast<char*>(tag.constData())) == 1;
    }
    if (ok) {
        ok = EVP_DecryptFinal_ex(ctx,
                                 reinterpret_cast<unsigned char*>(plain.data()) + totalLen,
                                 &outLen) == 1;
        totalLen += outLen;
    }

    EVP_CIPHER_CTX_free(ctx);

    if (!ok) {
        qWarning() << "Ошибка AES-GCM: верификация тега не пройдена";
        return QByteArray();
    }

    plain.resize(totalLen);
    return plain;
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
    
    QSettings settings = makeSecureSettings();
    settings.setValue("master_password_hash", hash);
    settings.setValue("salt", salt);
    ensureSecurePermissions(settings.fileName());
    // Обновляем ключ шифрования сразу после установки пароля
    m_instance->m_encryptionKey = deriveKeyFromPassword(password, salt.toUtf8(), 200000);
    
    return true;
}

bool Security::verifyMasterPassword(const QString& password)
{
    if (!m_instance) {
        return false;
    }
    
    QString storedHash = getMasterPasswordHash();
    QSettings s = makeSecureSettings();
    QString salt = s.value("salt").toString();
    
    if (storedHash.isEmpty() || salt.isEmpty()) {
        return false;
    }
    
    QString hash = hashPassword(password, salt);
    if (hash == storedHash) {
        resetFailures();
        m_instance->m_encryptionKey = deriveKeyFromPassword(password, salt.toUtf8(), 200000);
        return true;
    }

    recordFailure();
    const int delay = backoffMs();
    qWarning() << "Неверный мастер-пароль. Попытка" << s_failedAttempts << ", задержка" << delay << "мс";
    QThread::msleep(delay);
    return false;
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
        if (Security::m_instance) {
            Security::m_instance->endSession();
            Security::m_instance->m_isAuthenticated = false;
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
    QSettings s = makeSecureSettings();
    QString hash = s.value("master_password_hash").toString();
    // миграция из старого места
    if (hash.isEmpty()) {
        QSettings legacy;
        hash = legacy.value("security/master_password_hash").toString();
        if (!hash.isEmpty()) {
            s.setValue("master_password_hash", hash);
            ensureSecurePermissions(s.fileName());
            const QString legacySalt = legacy.value("security/salt").toString();
            if (!legacySalt.isEmpty()) s.setValue("salt", legacySalt);
        }
    }
    return hash;
}

void Security::setMasterPasswordHash(const QString& hash)
{
    QSettings s = makeSecureSettings();
    s.setValue("master_password_hash", hash);
    ensureSecurePermissions(s.fileName());
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

QSettings Security::makeSecureSettings()
{
    const QString baseDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/secure";
    QDir().mkpath(baseDir);
    ensureSecurePermissions(baseDir);
    const QString path = baseDir + "/secure.ini";
    return QSettings(path, QSettings::IniFormat);
}

void Security::ensureSecurePermissions(const QString& path)
{
    QFileInfo info(path);
    if (!info.exists()) return;

    QFile f(path);
    if (info.isDir()) {
        f.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);
    } else {
        f.setPermissions(QFile::ReadOwner | QFile::WriteOwner);
    }
}

void Security::recordFailure()
{
    ++s_failedAttempts;
    s_lastFailure = QDateTime::currentDateTimeUtc();
}

void Security::resetFailures()
{
    s_failedAttempts = 0;
    s_lastFailure = QDateTime();
}

int Security::backoffMs()
{
    // Экспоненциальная задержка: 500, 1000, 2000, 4000, 8000 (макс)
    int exponent = qMin(4, s_failedAttempts - 1); // первая ошибка — 500 мс
    int delay = 500 * (1 << exponent);
    return qMin(delay, 8000);
}

QByteArray Security::currentSaltBytes()
{
    QSettings s = makeSecureSettings();
    return s.value("salt").toString().toUtf8();
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
