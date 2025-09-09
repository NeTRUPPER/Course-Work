#ifndef SECURITY_H
#define SECURITY_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QCryptographicHash>
#include <QSettings>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>
#include <QMessageBox>
#include <QInputDialog>
#include <QApplication>
#include <QDateTime>
#include <QTimer>
#include <QRandomGenerator>

class Security : public QObject
{
    Q_OBJECT

public:
    static void initialize();
    static bool authenticate();
    static QString authenticateAndGetPassword(int maxAttempts = 5);
    static bool changePassword();
    static bool isAuthenticated();
    static void logout();
    
    // Encryption/Decryption
    static QByteArray encryptData(const QByteArray& data);
    static QByteArray decryptData(const QByteArray& encryptedData);
    static QString encryptString(const QString& text);
    static QString decryptString(const QString& encryptedText);
    
    // Password management
    static bool setMasterPassword(const QString& password);
    static bool verifyMasterPassword(const QString& password);
    static bool hasMasterPassword();
    
    // Session management
    static void startSession();
    static void endSession();
    static bool isSessionValid();
    
    // Access control
    static bool hasPermission(const QString& permission);
    static void grantPermission(const QString& permission);
    static void revokePermission(const QString& permission);
    
    // Encryption key generation (public for Database access)
    static QByteArray generateEncryptionKey();
    static QByteArray deriveKeyFromPassword(const QString& password, const QByteArray& salt, int iterations = 200000);

private:
    static Security* m_instance;
    explicit Security(QObject *parent = nullptr);
    ~Security();
    
    static QString getMasterPasswordHash();
    static void setMasterPasswordHash(const QString& hash);
    static QString generateSalt();
    static QString hashPassword(const QString& password, const QString& salt);
    static QString generateSessionToken();
    
    QString m_sessionToken;
    QDateTime m_sessionStart;
    QStringList m_permissions;
    bool m_isAuthenticated;
    
    // Encryption key derived from master password
    QByteArray m_encryptionKey;
    bool m_isInitialized;
};

#endif // SECURITY_H 