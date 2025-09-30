#include "AuditLogDialog.h"

AuditLogDialog::AuditLogDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle(tr("Журнал событий (только для администратора)"));
    setModal(true);
    setMinimumSize(800, 500);

    auto* v = new QVBoxLayout(this);

    // Фильтры
    auto* form = new QFormLayout();
    m_filterEdit = new QLineEdit(this);
    m_fromEdit = new QDateEdit(this);
    m_toEdit   = new QDateEdit(this);
    m_fromEdit->setCalendarPopup(true);
    m_toEdit->setCalendarPopup(true);
    m_fromEdit->setDate(QDate::currentDate().addDays(-7));
    m_toEdit->setDate(QDate::currentDate());
    m_sevCombo = new QComboBox(this);
    m_sevCombo->addItem("Любая", -1);
    m_sevCombo->addItem("Info", (int)AuditSeverity::Info);
    m_sevCombo->addItem("Warning", (int)AuditSeverity::Warning);
    m_sevCombo->addItem("Error", (int)AuditSeverity::Error);
    m_sevCombo->addItem("Security", (int)AuditSeverity::Security);

    form->addRow(tr("Поиск:"), m_filterEdit);
    form->addRow(tr("С:"), m_fromEdit);
    form->addRow(tr("По:"), m_toEdit);
    form->addRow(tr("Уровень:"), m_sevCombo);
    v->addLayout(form);

    // Кнопки строкой
    auto* h = new QHBoxLayout();
    auto* btnReload = new QPushButton(tr("Обновить"), this);
    m_btnExport = new QPushButton(tr("Экспорт CSV"), this);
    m_btnClear  = new QPushButton(tr("Очистить старше 90 дней"), this);
    h->addWidget(btnReload);
    h->addWidget(m_btnExport);
    h->addWidget(m_btnClear);
    h->addStretch();
    v->addLayout(h);

    // Таблица
    m_table = new QTableWidget(this);
    m_table->setColumnCount(5);
    m_table->setHorizontalHeaderLabels({tr("Время"), tr("Пользователь"), tr("Событие"), tr("Детали"), tr("Уровень")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    v->addWidget(m_table);

    // Соединения
    connect(btnReload,   &QPushButton::clicked, this, &AuditLogDialog::reload);
    connect(m_btnExport, &QPushButton::clicked, this, &AuditLogDialog::exportCsv);
    connect(m_btnClear,  &QPushButton::clicked, this, &AuditLogDialog::clearOld);
    connect(m_filterEdit,&QLineEdit::textChanged, this, &AuditLogDialog::reload);
    connect(m_fromEdit,  &QDateEdit::dateChanged, this, &AuditLogDialog::reload);
    connect(m_toEdit,    &QDateEdit::dateChanged, this, &AuditLogDialog::reload);
    connect(m_sevCombo,  &QComboBox::currentIndexChanged, this, &AuditLogDialog::reload);

    reload();
}

void AuditLogDialog::reload() {
    QDateTime from(m_fromEdit->date(), QTime(0,0,0));
    QDateTime to(m_toEdit->date(), QTime(23,59,59, 999));
    const QString filter = m_filterEdit->text();
    const int sev = m_sevCombo->currentData().toInt();

    const auto rows = AuditLogger::instance().fetch(1000, from, to, filter, sev);

    m_table->setRowCount(0);
    for (const auto& m : rows) {
        const int r = m_table->rowCount();
        m_table->insertRow(r);
        m_table->setItem(r, 0, new QTableWidgetItem(m.value("ts").toString()));
        m_table->setItem(r, 1, new QTableWidgetItem(m.value("actor").toString()));
        m_table->setItem(r, 2, new QTableWidgetItem(m.value("event").toString()));
        m_table->setItem(r, 3, new QTableWidgetItem(m.value("details").toString()));
        const int sev = m.value("severity").toInt();
        QString sevText = "Info";
        if (sev == (int)AuditSeverity::Warning) sevText = "Warning";
        else if (sev == (int)AuditSeverity::Error) sevText = "Error";
        else if (sev == (int)AuditSeverity::Security) sevText = "Security";
        m_table->setItem(r, 4, new QTableWidgetItem(sevText));
    }
}

void AuditLogDialog::exportCsv() {
    const QString suggested =
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) +
        "/audit_" + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".csv";
    const QString path = QFileDialog::getSaveFileName(this, tr("Сохранить CSV"),
                                                      suggested, "CSV (*.csv)");
    if (path.isEmpty()) return;

    QDateTime from(m_fromEdit->date(), QTime(0,0,0));
    QDateTime to(m_toEdit->date(), QTime(23,59,59, 999));
    const QString filter = m_filterEdit->text();
    const int sev = m_sevCombo->currentData().toInt();

    const auto rows = AuditLogger::instance().fetch(0, from, to, filter, sev); // без лимита
    if (AuditLogger::exportCsv(path, rows)) {
        QMessageBox::information(this, tr("Готово"), tr("CSV сохранён."));
    } else {
        QMessageBox::warning(this, tr("Ошибка"), tr("Не удалось сохранить CSV."));
    }
}

void AuditLogDialog::clearOld() {
    if (QMessageBox::question(this, tr("Подтверждение"),
        tr("Удалить записи старше 90 дней?")) != QMessageBox::Yes) return;

    const QDateTime before = QDateTime::currentDateTime().addDays(-90);
    if (AuditLogger::instance().clearOlderThan(before)) {
        reload();
        QMessageBox::information(this, tr("Готово"), tr("Старые записи удалены."));
    } else {
        QMessageBox::warning(this, tr("Ошибка"), tr("Не удалось очистить журнал."));
    }
}
