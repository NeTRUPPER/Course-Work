#include "rentaldialog.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QDateTime>

RentalDialog::RentalDialog(QWidget *parent)
    : QDialog(parent)
    , m_rental(new Rental(this))
    , m_isEditMode(false)
{
    setupUI();
    loadCustomers();
    loadEquipment();
    setWindowTitle("Новая аренда");
    // Prefill from property if provided
    QVariant prefill = property("prefillCustomerName");
    if (prefill.isValid()) {
        m_customerEdit->setText(prefill.toString());
        validateInput();
    }
}

RentalDialog::RentalDialog(Rental* rental, QWidget *parent)
    : QDialog(parent)
    , m_rental(rental)
    , m_isEditMode(true)
{
    setupUI();
    loadCustomers();
    loadEquipment();
    loadRentalData();
    setWindowTitle("Редактирование аренды");
}

void RentalDialog::setupUI()
{
    setModal(true);
    setMinimumWidth(500);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Форма
    QFormLayout* formLayout = new QFormLayout();
    
    m_customerEdit = new QLineEdit(this);
    m_customerEdit->setPlaceholderText("Начните вводить имя клиента...");
    formLayout->addRow("Клиент:", m_customerEdit);
    
    m_equipmentEdit = new QLineEdit(this);
    m_equipmentEdit->setPlaceholderText("Начните вводить название оборудования...");
    formLayout->addRow("Оборудование:", m_equipmentEdit);
    
    m_quantitySpinBox = new QSpinBox(this);
    m_quantitySpinBox->setRange(1, 100);
    m_quantitySpinBox->setSuffix(" шт.");
    formLayout->addRow("Количество:", m_quantitySpinBox);
    
    // Даты
    QGroupBox* dateGroup = new QGroupBox("Период аренды", this);
    QFormLayout* dateLayout = new QFormLayout(dateGroup);
    
    m_startDateEdit = new QDateEdit(this);
    m_startDateEdit->setDate(QDate::currentDate());
    m_startDateEdit->setCalendarPopup(true);
    dateLayout->addRow("Дата начала:", m_startDateEdit);
    
    m_startTimeEdit = new QTimeEdit(this);
    m_startTimeEdit->setTime(QTime::currentTime());
    dateLayout->addRow("Время начала:", m_startTimeEdit);
    
    m_endDateEdit = new QDateEdit(this);
    m_endDateEdit->setDate(QDate::currentDate().addDays(1));
    m_endDateEdit->setCalendarPopup(true);
    dateLayout->addRow("Дата окончания:", m_endDateEdit);
    
    m_endTimeEdit = new QTimeEdit(this);
    m_endTimeEdit->setTime(QTime::currentTime());
    dateLayout->addRow("Время окончания:", m_endTimeEdit);
    
    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(dateGroup);
    
    // Расчеты
    QGroupBox* calcGroup = new QGroupBox("Расчет стоимости", this);
    QFormLayout* calcLayout = new QFormLayout(calcGroup);
    
    m_priceLabel = new QLabel("0.00 ₽", this);
    m_priceLabel->setStyleSheet("font-weight: bold; color: blue;");
    calcLayout->addRow("Стоимость аренды:", m_priceLabel);
    
    m_depositLabel = new QLabel("0.00 ₽", this);
    m_depositLabel->setStyleSheet("font-weight: bold; color: green;");
    calcLayout->addRow("Залог:", m_depositLabel);
    
    mainLayout->addWidget(calcGroup);
    
    // Заметки
    QFormLayout* notesLayout = new QFormLayout();
    m_notesEdit = new QTextEdit(this);
    m_notesEdit->setMaximumHeight(80);
    m_notesEdit->setPlaceholderText("Дополнительные заметки...");
    notesLayout->addRow("Заметки:", m_notesEdit);
    
    mainLayout->addLayout(notesLayout);
    
    // Статус
    m_statusLabel = new QLabel(this);
    m_statusLabel->setStyleSheet("color: gray; font-style: italic;");
    mainLayout->addWidget(m_statusLabel);
    
    // Кнопки
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mainLayout->addWidget(m_buttonBox);
    
    // Подключения
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &RentalDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    
    connect(m_customerEdit, &QLineEdit::textChanged, this, &RentalDialog::validateInput);
    connect(m_equipmentEdit, &QLineEdit::textChanged, this, &RentalDialog::validateInput);
    connect(m_quantitySpinBox, QOverload<int>::of(&QSpinBox::valueChanged), 
            this, &RentalDialog::onQuantityChanged);
    connect(m_startDateEdit, &QDateEdit::dateChanged, this, &RentalDialog::onDateChanged);
    connect(m_startTimeEdit, &QTimeEdit::timeChanged, this, &RentalDialog::onDateChanged);
    connect(m_endDateEdit, &QDateEdit::dateChanged, this, &RentalDialog::onDateChanged);
    connect(m_endTimeEdit, &QTimeEdit::timeChanged, this, &RentalDialog::onDateChanged);
    
    validateInput();
    
    // Инициализируем расчет стоимости (покажет 0.00 ₽, так как оборудование не выбрано)
    calculatePrice();
}

void RentalDialog::loadCustomers()
{
    m_customers = Customer::getAll();
    QStringList names;
    for (Customer* customer : m_customers) {
        names << customer->getDisplayName();
    }
    m_customerCompleter = new QCompleter(names, this);
    m_customerCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    m_customerCompleter->setFilterMode(Qt::MatchContains);
    m_customerEdit->setCompleter(m_customerCompleter);
}

void RentalDialog::loadEquipment()
{
    m_equipment = Equipment::getAll();
    QStringList items;
    for (Equipment* item : m_equipment) {
        if (item->isAvailable()) items << item->getDisplayName();
    }
    m_equipmentCompleter = new QCompleter(items, this);
    m_equipmentCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    m_equipmentCompleter->setFilterMode(Qt::MatchContains);
    m_equipmentEdit->setCompleter(m_equipmentCompleter);
}

void RentalDialog::loadRentalData()
{
    if (!m_rental) return;
    
    // Устанавливаем клиента
    if (m_rental->getCustomer()) {
        m_customerEdit->setText(m_rental->getCustomer()->getDisplayName());
    }
    
    // Устанавливаем оборудование
    if (m_rental->getEquipment()) {
        m_equipmentEdit->setText(m_rental->getEquipment()->getDisplayName());
    }
    
    m_quantitySpinBox->setValue(m_rental->getQuantity());
    m_startDateEdit->setDate(m_rental->getStartDate().date());
    m_startTimeEdit->setTime(m_rental->getStartDate().time());
    m_endDateEdit->setDate(m_rental->getEndDate().date());
    m_endTimeEdit->setTime(m_rental->getEndDate().time());
    m_notesEdit->setText(m_rental->getNotes());
}

void RentalDialog::onCustomerChanged(int)
{
    // unused with typeahead; left for compatibility
}

void RentalDialog::onEquipmentChanged(int)
{
    // unused with typeahead; left for compatibility
}

void RentalDialog::onQuantityChanged()
{
    if (m_rental->getEquipment()) {
        calculatePrice();
    }
    validateInput();
}

void RentalDialog::onDateChanged()
{
    if (m_rental->getEquipment()) {
        calculatePrice();
    }
    validateInput();
}

void RentalDialog::calculatePrice()
{
    if (!m_rental->getEquipment()) {
        m_priceLabel->setText("0.00 ₽");
        m_depositLabel->setText("0.00 ₽");
        return;
    }
    
    QDateTime startDateTime(m_startDateEdit->date(), m_startTimeEdit->time());
    QDateTime endDateTime(m_endDateEdit->date(), m_endTimeEdit->time());
    
    m_rental->setStartDate(startDateTime);
    m_rental->setEndDate(endDateTime);
    m_rental->setQuantity(m_quantitySpinBox->value());
    
    double totalPrice = m_rental->calculateTotalPrice();
    double deposit = m_rental->calculateDeposit();
    
    m_priceLabel->setText(QString::number(totalPrice, 'f', 2) + " ₽");
    m_depositLabel->setText(QString::number(deposit, 'f', 2) + " ₽");
}

void RentalDialog::validateInput()
{
    // Resolve customer by name
    QString custName = m_customerEdit->text().trimmed();
    Customer* resolvedCustomer = nullptr;
    for (Customer* c : m_customers) {
        if (c->getDisplayName().compare(custName, Qt::CaseInsensitive) == 0) { resolvedCustomer = c; break; }
    }
    if (resolvedCustomer) m_rental->setCustomer(resolvedCustomer);

    // Resolve equipment by name
    QString equipName = m_equipmentEdit->text().trimmed();
    Equipment* resolvedEquipment = nullptr;
    for (Equipment* e : m_equipment) {
        if (e->getDisplayName().compare(equipName, Qt::CaseInsensitive) == 0) { resolvedEquipment = e; break; }
    }
    if (resolvedEquipment) m_rental->setEquipment(resolvedEquipment);

    bool isValid = (resolvedCustomer != nullptr) &&
                  (resolvedEquipment != nullptr) &&
                  m_quantitySpinBox->value() > 0;
    
    if (isValid) {
        QDateTime startDateTime(m_startDateEdit->date(), m_startTimeEdit->time());
        QDateTime endDateTime(m_endDateEdit->date(), m_endTimeEdit->time());
        
        if (startDateTime >= endDateTime) {
            m_statusLabel->setText("⚠ Дата окончания должна быть позже даты начала");
            m_statusLabel->setStyleSheet("color: red; font-style: italic;");
            m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
            return;
        }
        
        m_statusLabel->setText("✓ Форма заполнена корректно");
        m_statusLabel->setStyleSheet("color: green; font-style: italic;");
        m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    } else {
        m_statusLabel->setText("⚠ Заполните обязательные поля");
        m_statusLabel->setStyleSheet("color: orange; font-style: italic;");
        m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    }
}

void RentalDialog::accept()
{
    if (!m_rental) return;
    
    // Обновляем данные аренды
    QDateTime startDateTime(m_startDateEdit->date(), m_startTimeEdit->time());
    QDateTime endDateTime(m_endDateEdit->date(), m_endTimeEdit->time());
    
    m_rental->setStartDate(startDateTime);
    m_rental->setEndDate(endDateTime);
    m_rental->setQuantity(m_quantitySpinBox->value());
    m_rental->setNotes(m_notesEdit->toPlainText().trimmed());
    
    // Устанавливаем рассчитанные цены
    m_rental->setTotalPrice(m_rental->calculateTotalPrice());
    m_rental->setDeposit(m_rental->calculateDeposit());
    
    // Проверяем валидность
    if (!m_rental->isValid()) {
        QStringList errors = m_rental->getValidationErrors();
        QMessageBox::warning(this, "Ошибка валидации", 
                           "Пожалуйста, исправьте следующие ошибки:\n\n" + errors.join("\n"));
        return;
    }
    
    QDialog::accept();
} 