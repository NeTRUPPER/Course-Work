#ifndef RENTALDIALOG_H
#define RENTALDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QCompleter>
#include <QSpinBox>
#include <QDateEdit>
#include <QTimeEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QLabel>
#include <QMessageBox>
#include <QGroupBox>
#include "rental.h"
#include "customer.h"
#include "equipment.h"

class RentalDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RentalDialog(QWidget *parent = nullptr);
    explicit RentalDialog(Rental* rental, QWidget *parent = nullptr);
    
    Rental* getRental() const { return m_rental; }

private slots:
    void accept() override;
    void validateInput();
    void onCustomerChanged(int index);
    void onEquipmentChanged(int index);
    void onQuantityChanged();
    void onDateChanged();
    void calculatePrice();

private:
    void setupUI();
    void loadRentalData();
    void loadCustomers();
    void loadEquipment();
    
    Rental* m_rental;
    bool m_isEditMode;
    
    // UI Components
    QLineEdit* m_customerEdit;
    QLineEdit* m_equipmentEdit;
    QCompleter* m_customerCompleter;
    QCompleter* m_equipmentCompleter;
    QSpinBox* m_quantitySpinBox;
    QDateEdit* m_startDateEdit;
    QTimeEdit* m_startTimeEdit;
    QDateEdit* m_endDateEdit;
    QTimeEdit* m_endTimeEdit;
    QTextEdit* m_notesEdit;
    QLabel* m_priceLabel;
    QLabel* m_depositLabel;
    QDialogButtonBox* m_buttonBox;
    QLabel* m_statusLabel;
    
    // Data
    QList<Customer*> m_customers;
    QList<Equipment*> m_equipment;
};

#endif // RENTALDIALOG_H 