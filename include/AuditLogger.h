#pragma once
#include <QObject>
#include <QDateTime>
#include <QSqlDatabase>
#include <QVariantMap>
#include <functional>

enum class AuditSeverity : int {
    Info     = 0,
    Warning  = 1,
    Error    = 2,
    Security = 3
};

class AuditLogger : public QObject {
    Q_OBJECT
public:
    static AuditLogger& instance();

    // Использовать конкретное имя соединения БД (по умолчанию — default)
    void init(const QString& connectionName = QString());

    // Позволяет прокинуть «кто» выполняет действие (админ/пользователь)
    void setActorProvider(std::function<QString()> provider);

    // Простая запись события
    void log(const QString& event,
             const QString& details = QString(),
             AuditSeverity severity = AuditSeverity::Info);

    // Чтение лога с фильтрами (limit <= 0 значит без лимита)
    QList<QVariantMap> fetch(int limit = 1000,
                             const QDateTime& from = QDateTime(),
                             const QDateTime& to = QDateTime(),
                             const QString& textFilter = QString(),
                             int severityFilter = -1);

    // Удалить старше даты
    bool clearOlderThan(const QDateTime& before);

    // Экспорт в CSV
    static bool exportCsv(const QString& path, const QList<QVariantMap>& rows);

private:
    explicit AuditLogger(QObject* parent = nullptr);
    void ensureTable();

    QString m_connectionName; // имя соединения QSqlDatabase
    std::function<QString()> m_actorProvider;
};
