#ifndef DATABASE_H
#define DATABASE_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QString>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QStandardPaths>
#include <QMessageBox>
#include <QApplication>

class Database : public QObject
{
    Q_OBJECT

public:
    static Database& getInstance();
    static void initialize(const QString& dbPath);
    
    bool openDatabase(const QString& password);
    void closeDatabase();
    bool isOpen() const;
    
    // Customer operations
    bool addCustomer(const QString& name, const QString& phone, const QString& email, 
                    const QString& passport, const QString& address, const QDate& passportIssueDate = QDate());
    bool updateCustomer(int id, const QString& name, const QString& phone, 
                       const QString& email, const QString& passport, const QString& address, const QDate& passportIssueDate = QDate());
    bool deleteCustomer(int id);
    QSqlQuery getCustomers();
    QSqlQuery getCustomerById(int id);
    QSqlQuery searchCustomers(const QString& searchTerm);
    
    // Equipment operations
    bool addEquipment(const QString& name, const QString& category, double price, 
                     double deposit, int quantity, const QString& description, double additionalPrice);
    bool updateEquipment(int id, const QString& name, const QString& category, 
                        double price, double deposit, int quantity, const QString& description, double additionalPrice);
    bool deleteEquipment(int id);
    QSqlQuery getEquipment();
    QSqlQuery getEquipmentById(int id);
    QSqlQuery searchEquipment(const QString& searchTerm);
    bool updateEquipmentQuantity(int id, int newQuantity);
    
    // Rental operations
    bool addRental(int customerId, int equipmentId, int quantity, 
                   const QDateTime& startDate, const QDateTime& endDate,
                   double totalPrice, double deposit, const QString& notes);
    bool updateRental(int id, const QDateTime& endDate, double finalPrice, 
                      const QString& status, const QString& notes);
    bool completeRental(int id, double damageCost, double cleaningCost, 
                       double finalDeposit, const QString& notes);
    bool deleteRental(int id);
    QSqlQuery getRentals();
    QSqlQuery getRentalById(int id);
    QSqlQuery getActiveRentals();
    QSqlQuery getRentalsByCustomer(int customerId);
    QSqlQuery getRentalsByDateRange(const QDateTime& start, const QDateTime& end);
    
    // Reports
    QSqlQuery getRentalReport(const QDateTime& start, const QDateTime& end);
    QSqlQuery getEquipmentReport();
    QSqlQuery getCustomerReport();
    QSqlQuery getFinancialReport(const QDateTime& start, const QDateTime& end);

    // Utility methods
    QString getDatabasePath() const { return m_dbPath; }
    QSqlDatabase& getDatabase() { return m_db; }
    bool backupDatabase(const QString& backupPath);
    bool restoreDatabase(const QString& backupPath);

private:
    explicit Database(QObject *parent = nullptr);
    ~Database();
    
    bool createTables();
    bool createCustomersTable();
    bool createEquipmentTable();
    bool createRentalsTable();
    bool createSettingsTable();
    
    QSqlDatabase m_db;
    QString m_dbPath;
    bool m_isOpen;
    
    // Security
    QString m_encryptionKey;
    bool setupEncryption();
    
    // Singleton instance
    static Database* m_instance;
};

#endif // DATABASE_H 