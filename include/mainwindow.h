#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QTableWidget>
#include <QLineEdit>
#include <QDateEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QGroupBox>
#include <QFormLayout>
#include <QMessageBox>
#include <QStatusBar>
#include <QMenuBar>
#include <QToolBar>
#include <QAction>
#include <QDialog>
#include <QCalendarWidget>
#include <QTextEdit>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QInputDialog>
#include <QDialogButtonBox>
#include <QFile>
#include <QApplication>

// Forward declarations
class CustomerForm;
class EquipmentForm;
class RentalForm;
class Database;
class RentalManager;
class Customer;
class Equipment;
class Rental;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onNewCustomer();
    void onNewEquipment();
    void onNewRental();
    void onEditCustomer();
    void onEditEquipment();
    void onDeleteCustomer();
    void onDeleteEquipment();
    void onCompleteRental();
    void onViewRental();
    void onSearchCustomer();
    void onSearchEquipment();
    void onCustomerSearch();
    void onEquipmentSearch();
    void onViewRentals();
    void onReports();
    void onSettings();
    void onAbout();
    void onExit();
    void onCustomerSelectionChanged();
    void onEquipmentSelectionChanged();
    void onRentalSelectionChanged();
    void onCustomerDoubleClicked();
    void onEquipmentDoubleClicked();
    void onRentalDoubleClicked();

private:
    void setupUI();
    void setupMenuBar();
    void setupToolBar();
    void setupStatusBar();
    void setupCentralWidget();
    void createCustomerTab();
    void createEquipmentTab();
    void createRentalTab();
    void createReportsTab();
    void setupConnections();
    void updateStatus();
    void loadSettings();
    void saveSettings();
    
    // Table refresh methods
    void refreshCustomerTable();
    void refreshEquipmentTable();
    void refreshRentalTable();
    
    // Display methods
    void displayCustomers(const QList<Customer*>& customers);
    void displayEquipment(const QList<Equipment*>& equipment);
    void displayRentals(const QList<Rental*>& rentals);
    
    // Style methods
    void loadStyleSheet(const QString& theme);

    // UI Components
    QTabWidget *m_tabWidget;
    QWidget *m_customerTab;
    QWidget *m_equipmentTab;
    QWidget *m_rentalTab;
    QWidget *m_reportsTab;
    
    // Customer Tab Components
    QTableWidget *m_customerTable;
    QPushButton *m_addCustomerBtn;
    QPushButton *m_editCustomerBtn;
    QPushButton *m_deleteCustomerBtn;
    QLineEdit *m_customerSearchEdit;
    
    // Equipment Tab Components
    QTableWidget *m_equipmentTable;
    QPushButton *m_addEquipmentBtn;
    QPushButton *m_editEquipmentBtn;
    QPushButton *m_deleteEquipmentBtn;
    QLineEdit *m_equipmentSearchEdit;
    
    // Rental Tab Components
    QTableWidget *m_rentalTable;
    QPushButton *m_newRentalBtn;
    QPushButton *m_completeRentalBtn;
    QPushButton *m_viewRentalBtn;
    
    // Reports Tab Components
    QTextEdit *m_reportsText;
    QPushButton *m_generateReportBtn;
    QComboBox *m_reportTypeCombo;
    QDateEdit *m_reportStartDate;
    QDateEdit *m_reportEndDate;
    
    // Menu Actions
    QAction *m_newCustomerAction;
    QAction *m_newEquipmentAction;
    QAction *m_newRentalAction;
    QAction *m_searchAction;
    QAction *m_reportsAction;
    QAction *m_settingsAction;
    QAction *m_aboutAction;
    QAction *m_exitAction;
    
    // Managers
    RentalManager *m_rentalManager;
    Database *m_database;
    
    // Status
    QLabel *m_statusLabel;
    QLabel *m_userLabel;
    QLabel *m_dateLabel;
    
    // Selection tracking
    int m_selectedCustomerId;
    int m_selectedEquipmentId;
    int m_selectedRentalId;
};

#endif // MAINWINDOW_H 