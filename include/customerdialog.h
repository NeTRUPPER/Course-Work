#ifndef CUSTOMERDIALOG_H
#define CUSTOMERDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QDateEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QLabel>
#include <QMessageBox>
#include "customer.h"

class CustomerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CustomerDialog(QWidget *parent = nullptr);
    explicit CustomerDialog(Customer* customer, QWidget *parent = nullptr);
    
    Customer* getCustomer() const { return m_customer; }

private slots:
    void accept() override;
    void validateInput();

private:
    void setupUI();
    void loadCustomerData();
    
    Customer* m_customer;
    bool m_isEditMode;
    
    // UI Components
    QLineEdit* m_nameEdit;
    QLineEdit* m_phoneEdit;
    QLineEdit* m_emailEdit;
    QLineEdit* m_passportEdit;
    QLineEdit* m_addressEdit;
    QDateEdit* m_passportIssueDateEdit;
    QDialogButtonBox* m_buttonBox;
    QLabel* m_statusLabel;
};

#endif // CUSTOMERDIALOG_H 