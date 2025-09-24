#pragma once
#include <QObject>
#include <QString>

class AdminPasswordManager : public QObject {
    Q_OBJECT
public:
    explicit AdminPasswordManager(QObject* parent = nullptr);

    bool hasPassword() const;               // установлен ли пароль
    bool setNewPassword(const QString& pwd);// установить/сменить (перезапишет)
    bool verifyPassword(const QString& pwd) const;

    // для миграций/бэкапов можно получить/положить строку формата
    QString storedRecord() const;
    bool setStoredRecord(const QString& record);

    static QString hashPassword(const QString& pwd, int iterations = 500000);
    static bool verifyAgainstRecord(const QString& pwd, const QString& record);

private:
    QString loadRecord() const;
    void saveRecord(const QString& record) const;

    // Выберите реализацию хранения: QSettings или БД
    QString m_cache; // lazy cache, чтобы не читать каждый раз
};