#include "AuditLogger.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QFile>
#include <QTextStream>

AuditLogger::AuditLogger(QObject* parent) : QObject(parent) {}
AuditLogger& AuditLogger::instance() { static AuditLogger g; return g; }

void AuditLogger::init(const QString& connectionName) {
    m_connectionName = connectionName;
    ensureTable();
}

void AuditLogger::setActorProvider(std::function<QString()> provider) {
    m_actorProvider = std::move(provider);
}

void AuditLogger::ensureTable() {
    QSqlDatabase db = m_connectionName.isEmpty()
            ? QSqlDatabase::database()
            : QSqlDatabase::database(m_connectionName);
    if (!db.isValid()) return;
    if (!db.isOpen())  db.open();

    QSqlQuery q(db);
    q.exec(R"SQL(
        CREATE TABLE IF NOT EXISTS audit_log (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            ts TEXT NOT NULL,
            actor TEXT,
            event TEXT NOT NULL,
            details TEXT,
            severity INTEGER NOT NULL DEFAULT 0
        );
    )SQL");
    q.exec("CREATE INDEX IF NOT EXISTS idx_audit_ts ON audit_log(ts DESC);");
    q.exec("CREATE INDEX IF NOT EXISTS idx_audit_sev ON audit_log(severity);");
}

void AuditLogger::log(const QString& event, const QString& details, AuditSeverity severity) {
    QSqlDatabase db = m_connectionName.isEmpty()
            ? QSqlDatabase::database()
            : QSqlDatabase::database(m_connectionName);
    if (!db.isValid()) return;
    if (!db.isOpen())  db.open();

    const QString actor = m_actorProvider ? m_actorProvider() : QStringLiteral("user");
    const QString ts = QDateTime::currentDateTime().toString(Qt::ISODateWithMs);

    QSqlQuery q(db);
    q.prepare("INSERT INTO audit_log(ts, actor, event, details, severity) VALUES(?,?,?,?,?);");
    q.addBindValue(ts);
    q.addBindValue(actor);
    q.addBindValue(event);
    q.addBindValue(details);
    q.addBindValue(static_cast<int>(severity));
    q.exec(); // если упадёт — просто пропустим, логирование вспомогательное
}

QList<QVariantMap> AuditLogger::fetch(int limit, const QDateTime& from, const QDateTime& to,
                                      const QString& textFilter, int severityFilter) {
    QList<QVariantMap> res;
    QSqlDatabase db = m_connectionName.isEmpty()
            ? QSqlDatabase::database()
            : QSqlDatabase::database(m_connectionName);
    if (!db.isValid()) return res;
    if (!db.isOpen())  db.open();

    QString sql = "SELECT id, ts, actor, event, details, severity FROM audit_log WHERE 1=1";
    QList<QVariant> binds;

    if (from.isValid()) { sql += " AND ts >= ?"; binds << from.toString(Qt::ISODateWithMs); }
    if (to.isValid())   { sql += " AND ts <= ?"; binds << to.toString(Qt::ISODateWithMs); }
    if (!textFilter.trimmed().isEmpty()) {
        sql += " AND (LOWER(actor) LIKE ? OR LOWER(event) LIKE ? OR LOWER(details) LIKE ?)";
        const QString f = "%" + textFilter.trimmed().toLower() + "%";
        binds << f << f << f;
    }
    if (severityFilter >= 0) {
        sql += " AND severity = ?";
        binds << severityFilter;
    }
    sql += " ORDER BY ts DESC";
    if (limit > 0) sql += QString(" LIMIT %1").arg(limit);

    QSqlQuery q(db);
    q.prepare(sql);
    for (const auto& v : binds) q.addBindValue(v);
    q.exec();
    while (q.next()) {
        QVariantMap m;
        m["id"] = q.value(0);
        m["ts"] = q.value(1);
        m["actor"] = q.value(2);
        m["event"] = q.value(3);
        m["details"] = q.value(4);
        m["severity"] = q.value(5);
        res << m;
    }
    return res;
}

bool AuditLogger::clearOlderThan(const QDateTime& before) {
    if (!before.isValid()) return false;
    QSqlDatabase db = m_connectionName.isEmpty()
            ? QSqlDatabase::database()
            : QSqlDatabase::database(m_connectionName);
    if (!db.isValid()) return false;
    if (!db.isOpen())  db.open();

    QSqlQuery q(db);
    q.prepare("DELETE FROM audit_log WHERE ts < ?;");
    q.addBindValue(before.toString(Qt::ISODateWithMs));
    return q.exec();
}

bool AuditLogger::exportCsv(const QString& path, const QList<QVariantMap>& rows) {
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) return false;
    QTextStream out(&f);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    out.setEncoding(QStringConverter::Utf8);
#else
    out.setCodec("UTF-8");
#endif
    out << "id;ts;actor;event;details;severity\n";
    for (const auto& m : rows) {
        auto esc = [](const QString& s){ QString x=s; x.replace("\"","\"\""); return "\"" + x + "\""; };
        out << m.value("id").toString() << ";"
            << esc(m.value("ts").toString()) << ";"
            << esc(m.value("actor").toString()) << ";"
            << esc(m.value("event").toString()) << ";"
            << esc(m.value("details").toString()) << ";"
            << m.value("severity").toString() << "\n";
    }
    f.close();
    return true;
}
