#include "database.h"

Database* Database::m_instance = nullptr;

Database::Database(QObject *parent)
    : QObject(parent)
    , m_isOpen(false)
{
    m_db = QSqlDatabase::addDatabase("QSQLITE");
}

Database::~Database()
{
    closeDatabase();
}

Database& Database::getInstance()
{
    if (!m_instance) {
        m_instance = new Database();
    }
    return *m_instance;
}

void Database::initialize(const QString& dbPath)
{
    if (!m_instance) {
        m_instance = new Database();
    }
    
    m_instance->m_dbPath = dbPath;
    
    // Создаем директорию для базы данных
    QDir().mkpath(QFileInfo(dbPath).absolutePath());
    
    m_instance->m_db.setDatabaseName(dbPath);
    
    // Настройка базы данных
    if (!m_instance->setupEncryption()) {
        throw std::runtime_error("Не удалось настроить базу данных");
    }
}

bool Database::setupEncryption()
{
    // Шифрование БД отключено. Используется только аутентификация приложения.
    return true;
}

bool Database::openDatabase(const QString& password)
{
    // Аутентификация перед открытием БД
    QString pw = password;
    if (pw.isEmpty()) {
        pw = Security::authenticateAndGetPassword();
        if (pw.isEmpty()) {
            qDebug() << "Аутентификация отклонена";
            return false;
        }
    }

    if (!m_db.open()) {
        qDebug() << "Ошибка открытия базы данных:" << m_db.lastError().text();
        return false;
    }
    
    // Гарантируем наличие таблиц (идемпотентно)
    if (!createTables()) {
        qDebug() << "Ошибка инициализации схемы БД";
        m_db.close();
        return false;
    }
    
    m_isOpen = true;
    return true;
}

void Database::closeDatabase()
{
    if (m_isOpen) {
        m_db.close();
        m_isOpen = false;
    }
}

bool Database::isOpen() const
{
    return m_isOpen;
}

bool Database::createTables()
{
    return createCustomersTable() &&
           createEquipmentTable() &&
           createRentalsTable() &&
           createSettingsTable();
}

bool Database::createCustomersTable()
{
    QSqlQuery query(m_db);
    QString sql = "CREATE TABLE IF NOT EXISTS customers ("
                  "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                  "name TEXT NOT NULL,"
                  "phone TEXT,"
                  "email TEXT,"
                  "passport TEXT,"
                  "address TEXT,"
                  "passport_issue_date DATE,"
                  "created_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
                  "updated_at DATETIME DEFAULT CURRENT_TIMESTAMP"
                  ")";
    
    if (!query.exec(sql)) {
        qDebug() << "Ошибка создания таблицы customers:" << query.lastError().text();
        return false;
    }
    
    // Try to add missing columns for existing DBs
    QSqlQuery pragma(m_db);
    if (pragma.exec("PRAGMA table_info(customers)")) {
        bool hasPassportIssue = false;
        while (pragma.next()) {
            if (pragma.value(1).toString() == "passport_issue_date") {
                hasPassportIssue = true;
            }
        }
        if (!hasPassportIssue) {
            QSqlQuery alter(m_db);
            alter.exec("ALTER TABLE customers ADD COLUMN passport_issue_date DATE");
        }
    }
    return true;
}

bool Database::createEquipmentTable()
{
    QSqlQuery query(m_db);
    QString sql = "CREATE TABLE IF NOT EXISTS equipment ("
                  "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                  "name TEXT NOT NULL,"
                  "category TEXT NOT NULL,"
                  "price REAL NOT NULL,"
                  "additional_day_price REAL NOT NULL DEFAULT 0,"
                  "deposit REAL NOT NULL,"
                  "quantity INTEGER NOT NULL DEFAULT 1,"
                  "available_quantity INTEGER NOT NULL DEFAULT 1,"
                  "description TEXT,"
                  "created_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
                  "updated_at DATETIME DEFAULT CURRENT_TIMESTAMP"
                  ")";
    
    if (!query.exec(sql)) {
        qDebug() << "Ошибка создания таблицы equipment:" << query.lastError().text();
        return false;
    }
    
    // Migration: add missing additional_day_price if needed
    QSqlQuery pragma(m_db);
    if (pragma.exec("PRAGMA table_info(equipment)")) {
        bool hasAdditional = false;
        while (pragma.next()) {
            if (pragma.value(1).toString() == "additional_day_price") {
                hasAdditional = true;
            }
        }
        if (!hasAdditional) {
            QSqlQuery alter(m_db);
            alter.exec("ALTER TABLE equipment ADD COLUMN additional_day_price REAL NOT NULL DEFAULT 0");
        }
    }
    return true;
}

bool Database::createRentalsTable()
{
    QSqlQuery query(m_db);
    QString sql = "CREATE TABLE IF NOT EXISTS rentals ("
                  "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                  "customer_id INTEGER NOT NULL,"
                  "equipment_id INTEGER NOT NULL,"
                  "quantity INTEGER NOT NULL,"
                  "start_date DATETIME NOT NULL,"
                  "end_date DATETIME NOT NULL,"
                  "total_price REAL NOT NULL,"
                  "deposit REAL NOT NULL,"
                  "final_price REAL DEFAULT 0,"
                  "damage_cost REAL DEFAULT 0,"
                  "cleaning_cost REAL DEFAULT 0,"
                  "final_deposit REAL DEFAULT 0,"
                  "notes TEXT,"
                  "status TEXT DEFAULT 'active',"
                  "created_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
                  "updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
                  "FOREIGN KEY (customer_id) REFERENCES customers (id),"
                  "FOREIGN KEY (equipment_id) REFERENCES equipment (id)"
                  ")";
    
    if (!query.exec(sql)) {
        qDebug() << "Ошибка создания таблицы rentals:" << query.lastError().text();
        return false;
    }
    
    return true;
}

bool Database::createSettingsTable()
{
    QSqlQuery query(m_db);
    QString sql = "CREATE TABLE IF NOT EXISTS settings ("
                  "key TEXT PRIMARY KEY,"
                  "value TEXT NOT NULL,"
                  "updated_at DATETIME DEFAULT CURRENT_TIMESTAMP"
                  ")";
    
    if (!query.exec(sql)) {
        qDebug() << "Ошибка создания таблицы settings:" << query.lastError().text();
        return false;
    }
    
    return true;
}

// Customer operations
bool Database::addCustomer(const QString& name, const QString& phone, const QString& email,
                          const QString& passport, const QString& address, const QDate& passportIssueDate)
{
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO customers (name, phone, email, passport, address, passport_issue_date) "
                  "VALUES (?, ?, ?, ?, ?, ?)");
    query.addBindValue(name);
    query.addBindValue(phone);
    query.addBindValue(email);
    query.addBindValue(passport);
    query.addBindValue(address);
    if (passportIssueDate.isValid()) query.addBindValue(passportIssueDate); else query.addBindValue(QVariant());

    if (!query.exec()) {
        qDebug() << "Ошибка добавления клиента:" << query.lastError().text()
                 << ", driver:" << query.lastError().driverText()
                 << ", db:" << query.lastError().databaseText()
                 << ", with params";
        return false;
    }
    return true;
}

bool Database::updateCustomer(int id, const QString& name, const QString& phone,
                             const QString& email, const QString& passport, const QString& address, const QDate& passportIssueDate)
{
    QSqlQuery query(m_db);
    query.prepare("UPDATE customers SET name = ?, phone = ?, email = ?, passport = ?, "
                  "address = ?, passport_issue_date = ?, updated_at = CURRENT_TIMESTAMP WHERE id = ?");
    query.addBindValue(name);
    query.addBindValue(phone);
    query.addBindValue(email);
    query.addBindValue(passport);
    query.addBindValue(address);
    if (passportIssueDate.isValid()) query.addBindValue(passportIssueDate); else query.addBindValue(QVariant());
    query.addBindValue(id);
    
    if (!query.exec()) {
        qDebug() << "Ошибка обновления клиента:" << query.lastError().text();
        return false;
    }
    
    return query.numRowsAffected() > 0;
}

bool Database::deleteCustomer(int id)
{
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM customers WHERE id = ?");
    query.addBindValue(id);
    
    if (!query.exec()) {
        qDebug() << "Ошибка удаления клиента:" << query.lastError().text();
        return false;
    }
    
    return query.numRowsAffected() > 0;
}

QSqlQuery Database::getCustomers()
{
    QSqlQuery query(m_db);
    query.exec("SELECT * FROM customers ORDER BY name");
    return query;
}

QSqlQuery Database::getCustomerById(int id)
{
    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM customers WHERE id = ?");
    query.addBindValue(id);
    query.exec();
    return query;
}

QSqlQuery Database::searchCustomers(const QString& searchTerm)
{
    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM customers WHERE name LIKE ? OR phone LIKE ? OR email LIKE ? "
                  "OR passport LIKE ? ORDER BY name");
    QString pattern = "%" + searchTerm + "%";
    query.addBindValue(pattern);
    query.addBindValue(pattern);
    query.addBindValue(pattern);
    query.addBindValue(pattern);
    query.exec();
    return query;
}

// Equipment operations
bool Database::addEquipment(const QString& name, const QString& category, double price,
                           double deposit, int quantity, const QString& description, double additionalPrice)
{
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO equipment (name, category, price, additional_day_price, deposit, quantity, available_quantity, description) "
                  "VALUES (?, ?, ?, ?, ?, ?, ?, ?)");
    query.addBindValue(name);
    query.addBindValue(category);
    query.addBindValue(price);
    query.addBindValue(additionalPrice);
    query.addBindValue(deposit);
    query.addBindValue(quantity);
    query.addBindValue(quantity);
    query.addBindValue(description);

    if (!query.exec()) {
        qDebug() << "Ошибка добавления оборудования:" << query.lastError().text()
                 << ", driver:" << query.lastError().driverText()
                 << ", db:" << query.lastError().databaseText();
        return false;
    }
    return true;
}

bool Database::updateEquipment(int id, const QString& name, const QString& category,
                              double price, double deposit, int quantity, const QString& description, double additionalPrice)
{
    QSqlQuery query(m_db);
    query.prepare("UPDATE equipment SET name = ?, category = ?, price = ?, additional_day_price = ?, deposit = ?, "
                  "quantity = ?, description = ?, updated_at = CURRENT_TIMESTAMP WHERE id = ?");
    query.addBindValue(name);
    query.addBindValue(category);
    query.addBindValue(price);
    query.addBindValue(additionalPrice);
    query.addBindValue(deposit);
    query.addBindValue(quantity);
    query.addBindValue(description);
    query.addBindValue(id);
    
    if (!query.exec()) {
        qDebug() << "Ошибка обновления оборудования:" << query.lastError().text();
        return false;
    }
    
    return query.numRowsAffected() > 0;
}

bool Database::deleteEquipment(int id)
{
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM equipment WHERE id = ?");
    query.addBindValue(id);
    
    if (!query.exec()) {
        qDebug() << "Ошибка удаления оборудования:" << query.lastError().text();
        return false;
    }
    
    return query.numRowsAffected() > 0;
}

QSqlQuery Database::getEquipment()
{
    QSqlQuery query(m_db);
    query.exec("SELECT * FROM equipment ORDER BY name");
    return query;
}

QSqlQuery Database::getEquipmentById(int id)
{
    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM equipment WHERE id = ?");
    query.addBindValue(id);
    query.exec();
    return query;
}

QSqlQuery Database::searchEquipment(const QString& searchTerm)
{
    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM equipment WHERE name LIKE ? OR category LIKE ? "
                  "OR description LIKE ? ORDER BY name");
    QString pattern = "%" + searchTerm + "%";
    query.addBindValue(pattern);
    query.addBindValue(pattern);
    query.addBindValue(pattern);
    query.exec();
    return query;
}

bool Database::updateEquipmentQuantity(int id, int newQuantity)
{
    QSqlQuery query(m_db);
    query.prepare("UPDATE equipment SET available_quantity = ?, updated_at = CURRENT_TIMESTAMP WHERE id = ?");
    query.addBindValue(newQuantity);
    query.addBindValue(id);
    
    if (!query.exec()) {
        qDebug() << "Ошибка обновления количества оборудования:" << query.lastError().text();
        return false;
    }
    
    return query.numRowsAffected() > 0;
}

// Rental operations
bool Database::addRental(int customerId, int equipmentId, int quantity,
                        const QDateTime& startDate, const QDateTime& endDate,
                        double totalPrice, double deposit, const QString& notes)
{
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO rentals (customer_id, equipment_id, quantity, start_date, "
                  "end_date, total_price, deposit, notes) VALUES (?, ?, ?, ?, ?, ?, ?, ?)");
    query.addBindValue(customerId);
    query.addBindValue(equipmentId);
    query.addBindValue(quantity);
    query.addBindValue(startDate);
    query.addBindValue(endDate);
    query.addBindValue(totalPrice);
    query.addBindValue(deposit);
    query.addBindValue(notes);
    
    if (!query.exec()) {
        qDebug() << "Ошибка добавления аренды:" << query.lastError().text();
        return false;
    }
    
    return true;
}

bool Database::updateRental(int id, const QDateTime& endDate, double finalPrice,
                           const QString& status, const QString& notes)
{
    QSqlQuery query(m_db);
    query.prepare("UPDATE rentals SET end_date = ?, final_price = ?, status = ?, notes = ?, "
                  "updated_at = CURRENT_TIMESTAMP WHERE id = ?");
    query.addBindValue(endDate);
    query.addBindValue(finalPrice);
    query.addBindValue(status);
    query.addBindValue(notes);
    query.addBindValue(id);
    
    if (!query.exec()) {
        qDebug() << "Ошибка обновления аренды:" << query.lastError().text();
        return false;
    }
    
    return query.numRowsAffected() > 0;
}

bool Database::completeRental(int id, double damageCost, double cleaningCost,
                             double finalDeposit, const QString& notes)
{
    QSqlQuery query(m_db);
    query.prepare("UPDATE rentals SET damage_cost = ?, cleaning_cost = ?, final_deposit = ?, "
                  "status = 'completed', notes = ?, updated_at = CURRENT_TIMESTAMP WHERE id = ?");
    query.addBindValue(damageCost);
    query.addBindValue(cleaningCost);
    query.addBindValue(finalDeposit);
    query.addBindValue(notes);
    query.addBindValue(id);
    
    if (!query.exec()) {
        qDebug() << "Ошибка завершения аренды:" << query.lastError().text();
        return false;
    }
    
    return query.numRowsAffected() > 0;
}

bool Database::deleteRental(int id)
{
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM rentals WHERE id = ?");
    query.addBindValue(id);
    
    if (!query.exec()) {
        qDebug() << "Ошибка удаления аренды:" << query.lastError().text();
        return false;
    }
    
    return query.numRowsAffected() > 0;
}

QSqlQuery Database::getRentals()
{
    QSqlQuery query(m_db);
    query.exec("SELECT r.*, c.name as customer_name, e.name as equipment_name "
               "FROM rentals r "
               "JOIN customers c ON r.customer_id = c.id "
               "JOIN equipment e ON r.equipment_id = e.id "
               "ORDER BY r.created_at DESC");
    return query;
}

QSqlQuery Database::getRentalById(int id)
{
    QSqlQuery query(m_db);
    query.prepare("SELECT r.*, c.name as customer_name, e.name as equipment_name "
                  "FROM rentals r "
                  "JOIN customers c ON r.customer_id = c.id "
                  "JOIN equipment e ON r.equipment_id = e.id "
                  "WHERE r.id = ?");
    query.addBindValue(id);
    query.exec();
    return query;
}

QSqlQuery Database::getActiveRentals()
{
    QSqlQuery query(m_db);
    query.exec("SELECT r.*, c.name as customer_name, e.name as equipment_name "
               "FROM rentals r "
               "JOIN customers c ON r.customer_id = c.id "
               "JOIN equipment e ON r.equipment_id = e.id "
               "WHERE r.status = 'active' "
               "ORDER BY r.end_date ASC");
    return query;
}

QSqlQuery Database::getRentalsByCustomer(int customerId)
{
    QSqlQuery query(m_db);
    query.prepare("SELECT r.*, c.name as customer_name, e.name as equipment_name "
                  "FROM rentals r "
                  "JOIN customers c ON r.customer_id = c.id "
                  "JOIN equipment e ON r.equipment_id = e.id "
                  "WHERE r.customer_id = ? "
                  "ORDER BY r.created_at DESC");
    query.addBindValue(customerId);
    query.exec();
    return query;
}

QSqlQuery Database::getRentalsByDateRange(const QDateTime& start, const QDateTime& end)
{
    QSqlQuery query(m_db);
    query.prepare("SELECT r.*, c.name as customer_name, e.name as equipment_name "
                  "FROM rentals r "
                  "JOIN customers c ON r.customer_id = c.id "
                  "JOIN equipment e ON r.equipment_id = e.id "
                  "WHERE r.start_date >= ? AND r.start_date <= ? "
                  "ORDER BY r.start_date DESC");
    query.addBindValue(start);
    query.addBindValue(end);
    query.exec();
    return query;
}

// Reports
QSqlQuery Database::getRentalReport(const QDateTime& start, const QDateTime& end)
{
    QSqlQuery query(m_db);
    query.prepare("SELECT r.*, c.name as customer_name, e.name as equipment_name, "
                  "e.category as equipment_category "
                  "FROM rentals r "
                  "JOIN customers c ON r.customer_id = c.id "
                  "JOIN equipment e ON r.equipment_id = e.id "
                  "WHERE r.start_date >= ? AND r.start_date <= ? "
                  "ORDER BY r.start_date DESC");
    query.addBindValue(start);
    query.addBindValue(end);
    query.exec();
    return query;
}

QSqlQuery Database::getEquipmentReport()
{
    QSqlQuery query(m_db);
    query.exec("SELECT e.*, "
               "COUNT(r.id) as rental_count, "
               "SUM(r.total_price) as total_revenue, "
               "AVG(r.total_price) as avg_revenue "
               "FROM equipment e "
               "LEFT JOIN rentals r ON e.id = r.equipment_id "
               "GROUP BY e.id "
               "ORDER BY rental_count DESC");
    return query;
}

QSqlQuery Database::getCustomerReport()
{
    QSqlQuery query(m_db);
    query.exec("SELECT c.*, "
               "COUNT(r.id) as rental_count, "
               "SUM(r.total_price) as total_spent, "
               "AVG(r.total_price) as avg_spent "
               "FROM customers c "
               "LEFT JOIN rentals r ON c.id = r.customer_id "
               "GROUP BY c.id "
               "ORDER BY total_spent DESC");
    return query;
}

QSqlQuery Database::getFinancialReport(const QDateTime& start, const QDateTime& end)
{
    QSqlQuery query(m_db);
    query.prepare("SELECT "
                  "SUM(total_price) as total_revenue, "
                  "SUM(deposit) as total_deposits, "
                  "SUM(damage_cost) as total_damage, "
                  "SUM(cleaning_cost) as total_cleaning, "
                  "COUNT(*) as rental_count, "
                  "COUNT(CASE WHEN status = 'completed' THEN 1 END) as completed_count, "
                  "COUNT(CASE WHEN status = 'active' THEN 1 END) as active_count "
                  "FROM rentals "
                  "WHERE start_date >= ? AND start_date <= ?");
    query.addBindValue(start);
    query.addBindValue(end);
    query.exec();
    return query;
} 

// Экранируем путь для SQL-литерала
static QString sqlQuotePath(const QString& p) {
    QString s = QDir::toNativeSeparators(p);
    s.replace('\'', "''");
    return "'" + s + "'";
}

bool Database::backupDatabase(const QString& backupPath)
{
    if (backupPath.isEmpty()) return false;
    if (!m_db.isValid()) return false;
    if (!m_db.isOpen() && !m_db.open()) return false;

    // Нельзя бэкапить в сам же файл
    if (QFileInfo(backupPath).canonicalFilePath() == QFileInfo(m_dbPath).canonicalFilePath())
        return false;

    // Гарантируем каталог и чистим возможные старые копии
    const QFileInfo dst(backupPath);
    QDir().mkpath(dst.absolutePath());
    QFile::remove(backupPath);
    QFile::remove(backupPath + "-wal");
    QFile::remove(backupPath + "-shm");

    // Попытка 1: VACUUM INTO (консистентная копия)
    {
        QSqlQuery pragma(m_db);
        pragma.exec("PRAGMA wal_checkpoint(TRUNCATE);"); // не помешает
        QSqlQuery q(m_db);
        const QString sql = "VACUUM INTO " + sqlQuotePath(backupPath) + ";";
        if (q.exec(sql)) {
            return true;
        }
        // если SQLite не поддерживает VACUUM INTO — пойдём в фолбэк
    }

    // Попытка 2 (фолбэк): блокируем записи и копируем файлы
    bool ok = true;
    {
        QSqlQuery q(m_db);
        q.exec("PRAGMA wal_checkpoint(TRUNCATE);");
        ok = q.exec("BEGIN IMMEDIATE;"); // эксклюзивная блокировка на запись

        ok = ok && QFile::copy(m_dbPath, backupPath);

        const QString wal = m_dbPath + "-wal";
        const QString shm = m_dbPath + "-shm";
        if (QFile::exists(wal))      ok = ok && QFile::copy(wal, backupPath + "-wal");
        if (QFile::exists(shm))      ok = ok && QFile::copy(shm, backupPath + "-shm");

        q.exec("COMMIT;");
    }
    return ok;
}

bool Database::restoreDatabase(const QString& backupPath)
{
    if (!QFile::exists(backupPath)) return false;

    // 1) Полностью разрываем текущее соединение
    const QString conn = m_db.connectionName();
    if (m_db.isOpen()) m_db.close();
    m_isOpen = false;

    // Сбрасываем хэндл и удаляем соединение из пула (важно для снятия файловых дескрипторов)
    m_db = QSqlDatabase();                // сброс ссылки
    QSqlDatabase::removeDatabase(conn);   // теперь файлы можно заменять

    // 2) Удаляем текущую БД и служебные файлы
    QFile::remove(m_dbPath);
    QFile::remove(m_dbPath + "-wal");
    QFile::remove(m_dbPath + "-shm");

    // 3) Копируем бэкап на место
    bool ok = QFile::copy(backupPath, m_dbPath);
    if (!ok) return false;

    // Если бэкап делали старым способом и рядом есть -wal/-shm — перенесём их тоже
    if (QFile::exists(backupPath + "-wal"))
        ok = ok && QFile::copy(backupPath + "-wal", m_dbPath + "-wal");
    if (QFile::exists(backupPath + "-shm"))
        ok = ok && QFile::copy(backupPath + "-shm", m_dbPath + "-shm");

    if (!ok) return false;

    // 4) Поднимаем соединение заново
    m_db = QSqlDatabase::addDatabase("QSQLITE", conn);
    m_db.setDatabaseName(m_dbPath);
    m_isOpen = m_db.open();
    if (!m_isOpen) return false;

    // 5) Базовые pragma
    QSqlQuery pq(m_db);
    pq.exec("PRAGMA foreign_keys=ON;");

    return true;
}