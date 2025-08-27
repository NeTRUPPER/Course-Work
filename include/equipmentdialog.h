#ifndef EQUIPMENTDIALOG_H
#define EQUIPMENTDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QLabel>
#include <QMessageBox>
#include "equipment.h"

class EquipmentDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EquipmentDialog(QWidget *parent = nullptr);
    explicit EquipmentDialog(Equipment* equipment, QWidget *parent = nullptr);
    
    Equipment* getEquipment() const { return m_equipment; }

private slots:
    void accept() override;
    void validateInput();

private:
    void setupUI();
    void loadEquipmentData();
    
    Equipment* m_equipment;
    bool m_isEditMode;
    
    // UI Components
    QLineEdit* m_nameEdit;
    QLineEdit* m_categoryEdit;
    QDoubleSpinBox* m_priceSpinBox;
    QDoubleSpinBox* m_additionalPriceSpinBox; // declared but created in cpp
    QDoubleSpinBox* m_depositSpinBox;
    QSpinBox* m_quantitySpinBox;
    QTextEdit* m_descriptionEdit;
    QDialogButtonBox* m_buttonBox;
    QLabel* m_statusLabel;
};

#endif // EQUIPMENTDIALOG_H 