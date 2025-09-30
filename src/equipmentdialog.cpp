#include "equipmentdialog.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QMessageBox>

EquipmentDialog::EquipmentDialog(QWidget *parent)
    : QDialog(parent)
    , m_equipment(new Equipment(this))
    , m_isEditMode(false)
{
    setupUI();
    setWindowTitle("Новое оборудование");
}

EquipmentDialog::EquipmentDialog(Equipment* equipment, QWidget *parent)
    : QDialog(parent)
    , m_equipment(equipment)
    , m_isEditMode(true)
{
    setupUI();
    loadEquipmentData();
    setWindowTitle("Редактирование оборудования");
}

void EquipmentDialog::setupUI()
{
    setModal(true);
    setMinimumWidth(450);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Форма
    QFormLayout* formLayout = new QFormLayout();
    
    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setPlaceholderText("Введите название оборудования");
    formLayout->addRow("Название:", m_nameEdit);
    
    m_categoryEdit = new QLineEdit(this);
    m_categoryEdit->setPlaceholderText("Введите категорию (необязательно)");
    formLayout->addRow("Категория:", m_categoryEdit);
    
    m_priceSpinBox = new QDoubleSpinBox(this);
    m_priceSpinBox->setRange(0.0, 10000.0);
    m_priceSpinBox->setSuffix(" ₽/день");
    m_priceSpinBox->setDecimals(2);
    formLayout->addRow("Цена за день:", m_priceSpinBox);

    QDoubleSpinBox* m_additionalPriceSpinBox = new QDoubleSpinBox(this);
    m_additionalPriceSpinBox->setObjectName("additionalPriceSpin");
    m_additionalPriceSpinBox->setRange(0.0, 10000.0);
    m_additionalPriceSpinBox->setSuffix(" ₽/доп.день");
    m_additionalPriceSpinBox->setDecimals(2);
    formLayout->addRow("Цена со 2-го дня:", m_additionalPriceSpinBox);
    
    m_depositSpinBox = new QDoubleSpinBox(this);
    m_depositSpinBox->setRange(0.0, 10000.0);
    m_depositSpinBox->setSuffix(" ₽");
    m_depositSpinBox->setDecimals(2);
    formLayout->addRow("Залог:", m_depositSpinBox);
    
    m_quantitySpinBox = new QSpinBox(this);
    m_quantitySpinBox->setRange(1, 100);
    m_quantitySpinBox->setSuffix(" шт.");
    formLayout->addRow("Количество:", m_quantitySpinBox);
    
    m_descriptionEdit = new QTextEdit(this);
    m_descriptionEdit->setMaximumHeight(100);
    m_descriptionEdit->setPlaceholderText("Описание оборудования...");
    formLayout->addRow("Описание:", m_descriptionEdit);
    
    mainLayout->addLayout(formLayout);
    
    // Статус
    m_statusLabel = new QLabel(this);
    m_statusLabel->setStyleSheet("color: gray; font-style: italic;");
    mainLayout->addWidget(m_statusLabel);
    
    // Кнопки
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mainLayout->addWidget(m_buttonBox);
    
    // Подключения
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &EquipmentDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    
    connect(m_nameEdit, &QLineEdit::textChanged, this, &EquipmentDialog::validateInput);
    connect(m_categoryEdit, &QLineEdit::textChanged, this, &EquipmentDialog::validateInput);
    connect(m_priceSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), 
            this, &EquipmentDialog::validateInput);
    connect(m_depositSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), 
            this, &EquipmentDialog::validateInput);
    connect(m_additionalPriceSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &EquipmentDialog::validateInput);
    connect(m_quantitySpinBox, QOverload<int>::of(&QSpinBox::valueChanged), 
            this, &EquipmentDialog::validateInput);
    
    validateInput();
}

void EquipmentDialog::loadEquipmentData()
{
    if (!m_equipment) return;
    
    m_nameEdit->setText(m_equipment->getName());
    m_categoryEdit->setText(m_equipment->getCategory());
    m_priceSpinBox->setValue(m_equipment->getPrice());
    findChild<QDoubleSpinBox*>("additionalPriceSpin")->setValue(m_equipment->getAdditionalDayPrice());
    m_depositSpinBox->setValue(m_equipment->getDeposit());
    m_quantitySpinBox->setValue(m_equipment->getQuantity());
    m_descriptionEdit->setText(m_equipment->getDescription());
}

void EquipmentDialog::validateInput()
{
    bool isValid = !m_nameEdit->text().trimmed().isEmpty() &&
                  !m_categoryEdit->text().trimmed().isEmpty() &&
                  m_priceSpinBox->value() > 0.0 &&
                  m_quantitySpinBox->value() > 0;
    
    if (isValid) {
        m_statusLabel->setText("✓ Форма заполнена корректно");
        m_statusLabel->setStyleSheet("color: green; font-style: italic;");
        m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    } else {
        m_statusLabel->setText("⚠ Заполните обязательные поля корректно");
        m_statusLabel->setStyleSheet("color: orange; font-style: italic;");
        m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    }
}

void EquipmentDialog::accept()
{
    if (!m_equipment) return;
    
    // Обновляем данные оборудования
    m_equipment->setName(m_nameEdit->text().trimmed());
    m_equipment->setCategory(m_categoryEdit->text().trimmed());
    m_equipment->setPrice(m_priceSpinBox->value());
    m_equipment->setDeposit(m_depositSpinBox->value());
    m_equipment->setAdditionalDayPrice(findChild<QDoubleSpinBox*>("additionalPriceSpin")->value());
    m_equipment->setQuantity(m_quantitySpinBox->value());
    m_equipment->setDescription(m_descriptionEdit->toPlainText().trimmed());
    
    // Проверяем валидность
    if (!m_equipment->isValid()) {
        QStringList errors = m_equipment->getValidationErrors();
        QMessageBox::warning(this, "Ошибка валидации", 
                           "Пожалуйста, исправьте следующие ошибки:\n\n" + errors.join("\n"));
        return;
    }
    
    QDialog::accept();
} 