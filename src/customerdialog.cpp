#include "customerdialog.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QMessageBox>

CustomerDialog::CustomerDialog(QWidget *parent)
    : QDialog(parent)
    , m_customer(new Customer(this))
    , m_isEditMode(false)
{
    setupUI();
    setWindowTitle("Новый клиент");
}

CustomerDialog::CustomerDialog(Customer* customer, QWidget *parent)
    : QDialog(parent)
    , m_customer(customer)
    , m_isEditMode(true)
{
    setupUI();
    loadCustomerData();
    setWindowTitle("Редактирование клиента");
}

void CustomerDialog::setupUI()
{
    setModal(true);
    setMinimumWidth(400);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Форма
    QFormLayout* formLayout = new QFormLayout();
    
    m_lastNameEdit = new QLineEdit(this);
    m_lastNameEdit->setPlaceholderText("Фамилия");
    formLayout->addRow("Фамилия:", m_lastNameEdit);

    m_firstNameEdit = new QLineEdit(this);
    m_firstNameEdit->setPlaceholderText("Имя");
    formLayout->addRow("Имя:", m_firstNameEdit);

    m_middleNameEdit = new QLineEdit(this);
    m_middleNameEdit->setPlaceholderText("Отчество");
    formLayout->addRow("Отчество:", m_middleNameEdit);
    
    m_phoneEdit = new QLineEdit(this);
    m_phoneEdit->setPlaceholderText("+7 (XXX) XXX-XX-XX");
    formLayout->addRow("Телефон:", m_phoneEdit);
    
    m_emailEdit = new QLineEdit(this);
    m_emailEdit->setPlaceholderText("example@email.com");
    formLayout->addRow("Email:", m_emailEdit);
    
    m_passportEdit = new QLineEdit(this);
    m_passportEdit->setPlaceholderText("XXXX XXXXXX");
    formLayout->addRow("Паспорт:", m_passportEdit);

    m_passportIssueDateEdit = new QDateEdit(this);
    m_passportIssueDateEdit->setCalendarPopup(true);
    m_passportIssueDateEdit->setDisplayFormat("dd.MM.yyyy");
    formLayout->addRow("Дата выдачи паспорта:", m_passportIssueDateEdit);
    
    m_addressEdit = new QLineEdit(this);
    m_addressEdit->setPlaceholderText("Адрес регистрации");
    formLayout->addRow("Адрес регистрации:", m_addressEdit);
    
    mainLayout->addLayout(formLayout);
    
    // Статус
    m_statusLabel = new QLabel(this);
    m_statusLabel->setStyleSheet("color: #2c3e50; font-style: italic;");
    mainLayout->addWidget(m_statusLabel);
    
    // Кнопки
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mainLayout->addWidget(m_buttonBox);
    
    // Подключения
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &CustomerDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    
    connect(m_lastNameEdit, &QLineEdit::textChanged, this, &CustomerDialog::validateInput);
    connect(m_firstNameEdit, &QLineEdit::textChanged, this, &CustomerDialog::validateInput);
    connect(m_middleNameEdit, &QLineEdit::textChanged, this, &CustomerDialog::validateInput);
    connect(m_phoneEdit, &QLineEdit::textChanged, this, &CustomerDialog::validateInput);
    connect(m_emailEdit, &QLineEdit::textChanged, this, &CustomerDialog::validateInput);
    connect(m_passportEdit, &QLineEdit::textChanged, this, &CustomerDialog::validateInput);
    connect(m_addressEdit, &QLineEdit::textChanged, this, &CustomerDialog::validateInput);
    connect(m_passportIssueDateEdit, &QDateEdit::dateChanged, this, &CustomerDialog::validateInput);
    
    validateInput();
}

void CustomerDialog::loadCustomerData()
{
    if (!m_customer) return;
    
    m_lastNameEdit->setText(m_customer->getLastName());
    m_firstNameEdit->setText(m_customer->getFirstName());
    m_middleNameEdit->setText(m_customer->getMiddleName());
    m_phoneEdit->setText(m_customer->getPhone());
    m_emailEdit->setText(m_customer->getEmail());
    m_passportEdit->setText(m_customer->getPassport());
    m_addressEdit->setText(m_customer->getAddress());
    if (m_customer->getPassportIssueDate().isValid())
        m_passportIssueDateEdit->setDate(m_customer->getPassportIssueDate());
}

void CustomerDialog::validateInput()
{
    bool isValid = !m_lastNameEdit->text().trimmed().isEmpty() && !m_firstNameEdit->text().trimmed().isEmpty();
    
    if (isValid) {
        m_statusLabel->setText("✓ Форма заполнена корректно");
        m_statusLabel->setStyleSheet("color: green; font-style: italic;");
        m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    } else {
        m_statusLabel->setText("⚠ Заполните обязательные поля");
        m_statusLabel->setStyleSheet("color: orange; font-style: italic;");
        m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    }
}

void CustomerDialog::accept()
{
    if (!m_customer) return;
    
    // Обновляем данные клиента
    m_customer->setLastName(m_lastNameEdit->text().trimmed());
    m_customer->setFirstName(m_firstNameEdit->text().trimmed());
    m_customer->setMiddleName(m_middleNameEdit->text().trimmed());
    m_customer->setPhone(m_phoneEdit->text().trimmed());
    m_customer->setEmail(m_emailEdit->text().trimmed());
    m_customer->setPassport(m_passportEdit->text().trimmed());
    m_customer->setAddress(m_addressEdit->text().trimmed());
    m_customer->setPassportIssueDate(m_passportIssueDateEdit->date());
    
    // Проверяем валидность
    if (!m_customer->isValid()) {
        QStringList errors = m_customer->getValidationErrors();
        QMessageBox::warning(this, "Ошибка валидации", 
                           "Пожалуйста, исправьте следующие ошибки:\n\n" + errors.join("\n"));
        return;
    }
    
    // Отвязываем объект, чтобы он не удалился вместе с диалогом
    if (!m_isEditMode) {
        m_customer->setParent(nullptr);
    }
    
    QDialog::accept();
} 
