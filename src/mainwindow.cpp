#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_rentalManager(nullptr)
    , m_database(nullptr)
    , m_selectedCustomerId(-1)
    , m_selectedEquipmentId(-1)
    , m_selectedRentalId(-1)
{
    setupUI();
    setupMenuBar();
    setupToolBar();
    setupStatusBar();
    setupCentralWidget();
    setupConnections();
    
    setupSecurityMenu();                // добавим меню и действия
    m_adminSession.clear();             // на старте режим админа выключен
    statusBar()->showMessage(tr("Admin: OFF"), 1500);

    runFirstTimePasswordWizard();

    // Инициализация менеджеров
    m_database = &Database::getInstance();
    m_rentalManager = new RentalManager(this);
    
    // Открываем базу данных
    if (!m_database->openDatabase("")) {
        QMessageBox::critical(this, "Ошибка", "Не удалось открыть базу данных");
        return;
    }
    
    // Инициализация аудит-логгера на текущем соединении (default)
    AuditLogger::instance().init(/*если есть имя соединения, передай его сюда*/);

    // Провайдер "кто"
    AuditLogger::instance().setActorProvider([this](){
        return m_adminSession.isElevated() ? QStringLiteral("admin") : QStringLiteral("user");
    });

    // Запишем старт приложения
    AuditLogger::instance().log("App started", QSysInfo::prettyProductName(),
                                AuditSeverity::Info);

    // Загружаем данные в таблицы
    refreshCustomerTable();
    refreshEquipmentTable();
    refreshRentalTable();
    
    // Загружаем единственный светлый стиль
    loadStyleSheet("light");
    
    // Обновление статуса
    updateStatus();
    
    // Таймер для проверки просроченных аренд
    QTimer *overdueTimer = new QTimer(this);
    connect(overdueTimer, &QTimer::timeout, this, [this]() {
        m_rentalManager->checkOverdueRentals();
    });
    overdueTimer->start(300000); // Проверка каждые 5 минут
    
    // Загрузка настроек
    loadSettings();
}

MainWindow::~MainWindow()
{
    saveSettings();
}

void MainWindow::setupUI()
{
    setWindowTitle("Система проката туристического оборудования");
    setMinimumSize(1200, 800);
    
    // Центрируем окно на экране
    QScreen *screen = QApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();
    int x = (screenGeometry.width() - width()) / 2;
    int y = (screenGeometry.height() - height()) / 2;
    move(x, y);
}

void MainWindow::setupMenuBar()
{
    QMenuBar *menuBar = this->menuBar();
    
    // Меню Файл
    QMenu *fileMenu = menuBar->addMenu("&Файл");
    m_newCustomerAction = fileMenu->addAction("&Новый клиент");
    m_newEquipmentAction = fileMenu->addAction("&Новое оборудование");
    m_newRentalAction = fileMenu->addAction("&Новая аренда");
    fileMenu->addSeparator();
    m_exitAction = fileMenu->addAction("&Выход");
    
    // Меню Поиск
    QMenu *searchMenu = menuBar->addMenu("&Поиск");
    m_searchAction = searchMenu->addAction("&Поиск клиентов");
    m_searchEquipmentAction = searchMenu->addAction("Поиск оборудования");
    m_searchRentalAction = searchMenu->addAction("Поиск аренд");
    
    // Меню Отчеты
    QMenu *reportsMenu = menuBar->addMenu("&Отчеты");
    m_reportsAction = reportsMenu->addAction("&Генерировать отчет");
    m_reportRentalsAction = reportsMenu->addAction("Отчет по арендам");
    m_reportEquipmentAction = reportsMenu->addAction("Отчет по оборудованию");
    m_reportFinanceAction = reportsMenu->addAction("Финансовый отчет");
    
    // Меню Настройки
    QMenu *settingsMenu = menuBar->addMenu("&Настройки");
    m_settingsAction = settingsMenu->addAction("&Параметры");
    m_settingsBackupAction = settingsMenu->addAction("Резервное копирование");
    m_settingsRestoreAction = settingsMenu->addAction("Восстановление");
    settingsMenu->addSeparator();
    m_settingsChangePwdAction = settingsMenu->addAction("Сменить пароль");
    
    // Меню Справка
    QMenu *helpMenu = menuBar->addMenu("&Справка");
    m_aboutAction = helpMenu->addAction("&О программе");
}

void MainWindow::setupToolBar()
{
    QToolBar *toolBar = addToolBar("Основная панель");
    toolBar->setObjectName("mainToolbar");
    toolBar->setMovable(false);
    
    toolBar->addAction(m_newCustomerAction);
    toolBar->addAction(m_newEquipmentAction);
    toolBar->addAction(m_newRentalAction);
    toolBar->addSeparator();
    toolBar->addAction(m_searchAction);
    toolBar->addAction(m_reportsAction);
    toolBar->addSeparator();
    toolBar->addAction(m_settingsAction);
}

void MainWindow::setupStatusBar()
{
    QStatusBar *statusBar = this->statusBar();
    
    m_statusLabel = new QLabel("Готово");
    m_userLabel = new QLabel("Пользователь: Администратор");
    m_dateLabel = new QLabel(QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm"));
    
    statusBar->addWidget(m_statusLabel);
    statusBar->addPermanentWidget(m_userLabel);
    statusBar->addPermanentWidget(m_dateLabel);
    
    // Обновление времени
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this]() {
        m_dateLabel->setText(QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm"));
    });
    timer->start(60000); // Обновление каждую минуту
}

void MainWindow::setupCentralWidget()
{
    m_tabWidget = new QTabWidget(this);
    setCentralWidget(m_tabWidget);
    
    createCustomerTab();
    createEquipmentTab();
    createRentalTab();
    createReportsTab();
}

void MainWindow::createCustomerTab()
{
    m_customerTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(m_customerTab);
    
    // Панель поиска
    QHBoxLayout *searchLayout = new QHBoxLayout();
    m_customerSearchEdit = new QLineEdit();
    m_customerSearchEdit->setPlaceholderText("Поиск клиентов...");
    m_customerSearchEdit->setProperty("search", true);
    searchLayout->addWidget(m_customerSearchEdit);
    layout->addLayout(searchLayout);
    
    // Таблица клиентов
    m_customerTable = new QTableWidget();
    m_customerTable->setColumnCount(6);
    m_customerTable->setHorizontalHeaderLabels({"ID", "Имя", "Телефон", "Email", "Паспорт", "Адрес"});
    m_customerTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_customerTable->setAlternatingRowColors(true);
    layout->addWidget(m_customerTable);
    
    // Кнопки управления
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    m_addCustomerBtn = new QPushButton("Добавить клиента");
    m_addCustomerBtn->setObjectName("addCustomerBtn");
    m_editCustomerBtn = new QPushButton("Редактировать");
    m_editCustomerBtn->setObjectName("editCustomerBtn");
    m_deleteCustomerBtn = new QPushButton("Удалить");
    m_deleteCustomerBtn->setObjectName("deleteCustomerBtn");
    m_createRentalFromCustomerBtn = new QPushButton("Создать аренду");
    
    // Инициализируем состояние кнопок
    m_editCustomerBtn->setEnabled(false);
    m_deleteCustomerBtn->setEnabled(false);
    
    buttonLayout->addWidget(m_addCustomerBtn);
    buttonLayout->addWidget(m_editCustomerBtn);
    buttonLayout->addWidget(m_deleteCustomerBtn);
    buttonLayout->addWidget(m_createRentalFromCustomerBtn);
    buttonLayout->addStretch();
    layout->addLayout(buttonLayout);
    
    m_tabWidget->addTab(m_customerTab, "Клиенты");
}

void MainWindow::createEquipmentTab()
{
    m_equipmentTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(m_equipmentTab);
    
    // Панель поиска
    QHBoxLayout *searchLayout = new QHBoxLayout();
    m_equipmentSearchEdit = new QLineEdit();
    m_equipmentSearchEdit->setPlaceholderText("Поиск оборудования...");
    m_equipmentSearchEdit->setProperty("search", true);
    searchLayout->addWidget(m_equipmentSearchEdit);
    layout->addLayout(searchLayout);
    
    // Таблица оборудования
    m_equipmentTable = new QTableWidget();
    m_equipmentTable->setColumnCount(7);
            m_equipmentTable->setHorizontalHeaderLabels({"ID", "Название", "Категория", "Цена/день (₽)", "Залог (₽)", "Количество", "Доступно"});
    m_equipmentTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_equipmentTable->setAlternatingRowColors(true);
    layout->addWidget(m_equipmentTable);
    
    // Кнопки управления
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    m_addEquipmentBtn = new QPushButton("Добавить оборудование");
    m_addEquipmentBtn->setObjectName("addEquipmentBtn");
    m_editEquipmentBtn = new QPushButton("Редактировать");
    m_editEquipmentBtn->setObjectName("editEquipmentBtn");
    m_deleteEquipmentBtn = new QPushButton("Удалить");
    m_deleteEquipmentBtn->setObjectName("deleteEquipmentBtn");
    
    // Инициализируем состояние кнопок
    m_editEquipmentBtn->setEnabled(false);
    m_deleteEquipmentBtn->setEnabled(false);
    
    buttonLayout->addWidget(m_addEquipmentBtn);
    buttonLayout->addWidget(m_editEquipmentBtn);
    buttonLayout->addWidget(m_deleteEquipmentBtn);
    buttonLayout->addStretch();
    layout->addLayout(buttonLayout);
    
    m_tabWidget->addTab(m_equipmentTab, "Оборудование");
}

void MainWindow::connectAdminSensitiveActions() {
    
}

void MainWindow::onSearchRental()
{
    m_statusLabel->setText("Поиск аренд...");
    const QString term = QInputDialog::getText(this, "Поиск аренд",
                                               "Введите клиента, оборудование, статус или ID:");
    if (term.isEmpty()) return;

    const QString t = term.trimmed().toLower();
    QList<Rental*> rentals = Rental::getAll();
    QList<Rental*> filtered;
    filtered.reserve(rentals.size());

    for (Rental* r : rentals) {
        const QString id = QString::number(r->getId());
        const QString cust = r->getCustomer() ? r->getCustomer()->getName() : QString();
        const QString eq   = r->getEquipment() ? r->getEquipment()->getName() : QString();
        const QString st   = r->getStatusText();

        if (id.contains(t, Qt::CaseInsensitive) ||
            cust.toLower().contains(t) ||
            eq.toLower().contains(t) ||
            st.toLower().contains(t))
        {
            filtered.append(r);
        }
    }

    m_tabWidget->setCurrentWidget(m_rentalTab);
    displayRentals(filtered);
    statusBar()->showMessage(QString("Найдено аренд: %1").arg(filtered.size()), 3000);
}

// Печать текущего отчёта из QTextEdit
void MainWindow::onReportPrint()
{
    if (!m_reportsText) return;
    QTextDocument doc;
    doc.setHtml(m_reportsText->toHtml());

    QPrinter printer(QPrinter::HighResolution);
    printer.setPageSize(QPageSize(QPageSize::A4));
    QPrintDialog pd(&printer, this);
    pd.setWindowTitle("Печать отчёта");
    if (pd.exec() == QDialog::Accepted) {
        doc.print(&printer);
    }

    AuditLogger::instance().log("Report printed", m_reportTypeCombo ? m_reportTypeCombo->currentText() : "");
}

// Экспорт текущего отчёта: PDF или HTML
void MainWindow::onReportExport()
{
    if (!m_reportsText) return;

    const QString suggestedBase =
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) +
        "/report_" + QDateTime::currentDateTime().toString("yyyyMMdd_HHmm");

    const QString path = QFileDialog::getSaveFileName(
        this, "Экспорт отчёта", suggestedBase,
        "PDF (*.pdf);;HTML (*.html)");

    if (path.isEmpty()) return;

    if (path.endsWith(".pdf", Qt::CaseInsensitive)) {
        QTextDocument doc;
        doc.setHtml(m_reportsText->toHtml());
        QPrinter printer(QPrinter::HighResolution);
        printer.setOutputFormat(QPrinter::PdfFormat);
        printer.setOutputFileName(path);
        printer.setPageSize(QPageSize(QPageSize::A4));
        doc.print(&printer);
        QMessageBox::information(this, "Готово", "Отчёт сохранён в PDF.");
    } else {
        QFile f(path.endsWith(".html", Qt::CaseInsensitive) ? path : (path + ".html"));
        if (f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
            f.write(m_reportsText->toHtml().toUtf8());
            f.close();
            QMessageBox::information(this, "Готово", "Отчёт сохранён в HTML.");
        } else {
            QMessageBox::warning(this, "Ошибка", "Не удалось сохранить отчёт.");
        }
    }

    AuditLogger::instance().log("Report exported", m_reportTypeCombo ? m_reportTypeCombo->currentText() : "");
}

// Меню «Настройки» Резервное копирование
void MainWindow::onSettingsBackup()
{
    if (!AdminGuard::ensureAdmin(this, &m_adminSession, &m_adminMgr)) return;

    const QString suggested =
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) +
        "/rental_backup_" + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".db";

    const QString fileName = QFileDialog::getSaveFileName(
        this, "Сохранить резервную копию", suggested,
        "База данных (*.db);;Все файлы (*)");
    if (fileName.isEmpty()) return;

    if (m_database && m_database->backupDatabase(fileName)) {
        QMessageBox::information(this, "Успех", "Резервная копия создана успешно!");
        AuditLogger::instance().log("Backup created", fileName, AuditSeverity::Security);
    } else {
        QMessageBox::warning(this, "Ошибка", "Не удалось создать резервную копию.");
        AuditLogger::instance().log("Backup failed", fileName, AuditSeverity::Error);
    }
}

// Меню «Настройки» Восстановление 
void MainWindow::onSettingsRestore()
{
    if (!AdminGuard::ensureAdmin(this, &m_adminSession, &m_adminMgr)) return;

    const QString fileName = QFileDialog::getOpenFileName(
        this, "Выбрать файл для восстановления",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        "База данных (*.db);;Все файлы (*)");
    if (fileName.isEmpty()) return;

    if (QMessageBox::question(this, "Подтверждение",
         "Восстановление базы данных приведёт к потере текущих данных. Продолжить?",
         QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;

    if (m_database && m_database->restoreDatabase(fileName)) {
        // Обновить данные в UI
        refreshCustomerTable();
        refreshEquipmentTable();
        refreshRentalTable();
        QMessageBox::information(this, "Готово", "База данных успешно восстановлена.");
        AuditLogger::instance().log("Database restored", fileName, AuditSeverity::Security);
    } else {
        QMessageBox::warning(this, "Ошибка", "Не удалось восстановить базу данных.");
        AuditLogger::instance().log("Restore failed", fileName, AuditSeverity::Error);
    }
}

// Меню «Настройки» Сменить пароль
void MainWindow::onChangeAdminPassword()
{
    // Только админ может менять
    if (!AdminGuard::ensureAdmin(this, &m_adminSession, &m_adminMgr)) return;
    setAdminPassword();
}

void MainWindow::setupSecurityMenu() {
    // Создаём/находим меню «Безопасность»
    QMenu* sec = nullptr;
    for (auto* m : menuBar()->findChildren<QMenu*>()) {
        if (m->title() == tr("Безопасность")) { sec = m; break; }
    }
    if (!sec) sec = menuBar()->addMenu(tr("Безопасность"));

    // Действие: Установить/Сменить админ-пароль
    QAction* actSetPwd = new QAction(tr("Установить админ-пароль…"), this);
    connect(actSetPwd, &QAction::triggered, this, &MainWindow::setAdminPassword);
    sec->addAction(actSetPwd);

    // (необязательно) шорткаты
    actSetPwd->setShortcut(QKeySequence("Ctrl+Shift+P"));
}

void MainWindow::runFirstTimePasswordWizard() {
    if (m_adminMgr.hasPassword()) return;

    const auto btn = QMessageBox::question(
        this, tr("Админ-пароль"),
        tr("Админ-пароль ещё не установлен. Установить сейчас?"),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);

    if (btn == QMessageBox::Yes) {
        setAdminPassword();
    } else {
        statusBar()->showMessage(tr("Админ-пароль не установлен"), 3000);
    }
}

void MainWindow::setAdminPassword() {
    bool ok1=false, ok2=false;
    const QString p1 = QInputDialog::getText(this, tr("Новый админ-пароль"),
                                             tr("Введите новый пароль (мин. 10 символов):"),
                                             QLineEdit::Password, {}, &ok1);
    if (!ok1) return;

    const QString p2 = QInputDialog::getText(this, tr("Подтверждение пароля"),
                                             tr("Повторите пароль:"),
                                             QLineEdit::Password, {}, &ok2);
    if (!ok2) return;

    if (p1 != p2) {
        QMessageBox::warning(this, tr("Ошибка"), tr("Пароли не совпадают."));
        return;
    }
    if (p1.size() < 10) {
        QMessageBox::warning(this, tr("Ошибка"),
                             tr("Слишком короткий пароль. Минимум 10 символов."));
        return;
    }

    if (m_adminMgr.setNewPassword(p1)) {
        m_adminSession.clear(); // сбросить возможную старую сессию
        statusBar()->showMessage(tr("Админ-пароль установлен"), 3000);
        QMessageBox::information(this, tr("Готово"), tr("Админ-пароль успешно установлен."));
        AuditLogger::instance().log("Admin password changed", "", AuditSeverity::Security);
    } else {
        QMessageBox::warning(this, tr("Ошибка"), tr("Не удалось сохранить админ-пароль."));
        AuditLogger::instance().log("Admin password change failed", "", AuditSeverity::Warning);
    }
}

void MainWindow::createRentalTab()
{
    m_rentalTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(m_rentalTab);
    
    // Таблица аренд
            m_rentalTable = new QTableWidget();
        m_rentalTable->setColumnCount(9);
            m_rentalTable->setHorizontalHeaderLabels({"ID", "Клиент", "Оборудование", "Количество", "Дата начала", "Дата окончания", "Статус", "Стоимость аренды (₽)", "Залог (₽)"});
    m_rentalTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_rentalTable->setAlternatingRowColors(true);
    layout->addWidget(m_rentalTable);
    
    // Кнопки управления
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    m_newRentalBtn = new QPushButton("Новая аренда");
    m_newRentalBtn->setObjectName("newRentalBtn");
    m_completeRentalBtn = new QPushButton("Завершить аренду");
    m_completeRentalBtn->setObjectName("completeRentalBtn");
    m_viewRentalBtn = new QPushButton("Просмотр");
    m_viewRentalBtn->setObjectName("viewRentalBtn");
    m_printRentalBtn = new QPushButton("Печать договора");
    
    // Инициализируем состояние кнопок
    m_completeRentalBtn->setEnabled(false);
    m_viewRentalBtn->setEnabled(false);
    
    buttonLayout->addWidget(m_newRentalBtn);
    buttonLayout->addWidget(m_completeRentalBtn);
    buttonLayout->addWidget(m_viewRentalBtn);
    buttonLayout->addWidget(m_printRentalBtn);
    buttonLayout->addStretch();
    layout->addLayout(buttonLayout);
    
    m_tabWidget->addTab(m_rentalTab, "Аренды");
}

void MainWindow::createReportsTab()
{
    m_reportsTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(m_reportsTab);
    
    // Панель настроек отчета
    QGroupBox *reportGroup = new QGroupBox("Параметры отчета");
    QFormLayout *formLayout = new QFormLayout(reportGroup);
    
    m_reportTypeCombo = new QComboBox();
    m_reportTypeCombo->addItems({"Отчет по арендам", "Отчет по оборудованию", "Финансовый отчет", "Отчет по клиентам"});
    
    m_reportStartDate = new QDateEdit();
    m_reportStartDate->setDate(QDate::currentDate().addDays(-30));
    m_reportStartDate->setCalendarPopup(true);
    
    m_reportEndDate = new QDateEdit();
    m_reportEndDate->setDate(QDate::currentDate());
    m_reportEndDate->setCalendarPopup(true);
    
    formLayout->addRow("Тип отчета:", m_reportTypeCombo);
    formLayout->addRow("Дата начала:", m_reportStartDate);
    formLayout->addRow("Дата окончания:", m_reportEndDate);
    
    layout->addWidget(reportGroup);
    
    // Кнопки
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    m_generateReportBtn = new QPushButton("Сгенерировать отчет");
    m_reportPrintBtn   = new QPushButton("Печать");
    m_reportExportBtn  = new QPushButton("Экспорт");
    buttonLayout->addWidget(m_generateReportBtn);
    buttonLayout->addWidget(m_reportPrintBtn);
    buttonLayout->addWidget(m_reportExportBtn);
    buttonLayout->addStretch();
    layout->addLayout(buttonLayout);
    
    // Область отчета
    m_reportsText = new QTextEdit();
    m_reportsText->setObjectName("reportsText");
    m_reportsText->setReadOnly(true);
    layout->addWidget(m_reportsText);
    
    m_tabWidget->addTab(m_reportsTab, "Отчеты");
}

void MainWindow::setupConnections()
{
    // Соединения меню
    connect(m_newCustomerAction, &QAction::triggered, this, &MainWindow::onNewCustomer);
    connect(m_newEquipmentAction, &QAction::triggered, this, &MainWindow::onNewEquipment);
    connect(m_newRentalAction, &QAction::triggered, this, &MainWindow::onNewRental);
    connect(m_searchAction, &QAction::triggered, this, &MainWindow::onSearchCustomer);
    connect(m_searchEquipmentAction, &QAction::triggered, this, &MainWindow::onSearchEquipment);
    connect(m_searchRentalAction,    &QAction::triggered, this, &MainWindow::onSearchRental);
    connect(m_reportsAction, &QAction::triggered, this, &MainWindow::onReports);
    connect(m_reportRentalsAction, &QAction::triggered, this, [this]{
        m_tabWidget->setCurrentWidget(m_reportsTab);
        m_reportTypeCombo->setCurrentText("Отчет по арендам");
        onReports();
    });
    connect(m_reportEquipmentAction, &QAction::triggered, this, [this]{
        m_tabWidget->setCurrentWidget(m_reportsTab);
        m_reportTypeCombo->setCurrentText("Отчет по оборудованию");
        onReports();
    });
    connect(m_reportFinanceAction, &QAction::triggered, this, [this]{
        m_tabWidget->setCurrentWidget(m_reportsTab);
        m_reportTypeCombo->setCurrentText("Финансовый отчет");
        onReports();
    });
    connect(m_settingsAction, &QAction::triggered, this, &MainWindow::onSettings);
    connect(m_aboutAction, &QAction::triggered, this, &MainWindow::onAbout);
    connect(m_exitAction, &QAction::triggered, this, &MainWindow::onExit);
    connect(m_settingsBackupAction,  &QAction::triggered, this, &MainWindow::onSettingsBackup);
    connect(m_settingsRestoreAction, &QAction::triggered, this, &MainWindow::onSettingsRestore);
    connect(m_settingsChangePwdAction, &QAction::triggered, this, &MainWindow::onChangeAdminPassword);
    // Вкладка «Отчёты»: печать и экспорт
    connect(m_reportPrintBtn,  &QPushButton::clicked, this, &MainWindow::onReportPrint);
    connect(m_reportExportBtn, &QPushButton::clicked, this, &MainWindow::onReportExport);
    
    // Соединения кнопок
    connect(m_addCustomerBtn, &QPushButton::clicked, this, &MainWindow::onNewCustomer);
    connect(m_editCustomerBtn, &QPushButton::clicked, this, &MainWindow::onEditCustomer);
    connect(m_deleteCustomerBtn, &QPushButton::clicked, this, &MainWindow::onDeleteCustomer);
    connect(m_createRentalFromCustomerBtn, &QPushButton::clicked, this, [this]() {
        if (m_selectedCustomerId == -1) {
            QMessageBox::information(this, "Информация", "Выберите клиента в таблице");
            return;
        }
        Customer* customer = Customer::loadById(m_selectedCustomerId);
        if (!customer) {
            QMessageBox::warning(this, "Ошибка", "Не удалось загрузить клиента");
            return;
        }
        RentalDialog dialog(this);
        // Предзаполним поле клиента
        // RentalDialog теперь использует typeahead, поэтому проставим имя клиента
        // через публичный API: используем setProperty как простой канал
        dialog.setProperty("prefillCustomerName", customer->getDisplayName());
        if (dialog.exec() == QDialog::Accepted) {
            Rental* rental = dialog.getRental();
            if (rental->save()) {
                refreshRentalTable();
                statusBar()->showMessage("Аренда успешно создана", 3000);
            } else {
                QMessageBox::warning(this, "Ошибка", "Не удалось создать аренду");
            }
            delete rental;
        }
        delete customer;
    });
    
    connect(m_addEquipmentBtn, &QPushButton::clicked, this, &MainWindow::onNewEquipment);
    connect(m_editEquipmentBtn, &QPushButton::clicked, this, &MainWindow::onEditEquipment);
    connect(m_deleteEquipmentBtn, &QPushButton::clicked, this, &MainWindow::onDeleteEquipment);
    
    connect(m_newRentalBtn, &QPushButton::clicked, this, &MainWindow::onNewRental);
    connect(m_completeRentalBtn, &QPushButton::clicked, this, &MainWindow::onCompleteRental);
    connect(m_viewRentalBtn, &QPushButton::clicked, this, &MainWindow::onViewRental);
    connect(m_printRentalBtn, &QPushButton::clicked, this, &MainWindow::onPrintRental);
    
    connect(m_generateReportBtn, &QPushButton::clicked, this, &MainWindow::onReports);
    
    // Соединения поиска
    connect(m_customerSearchEdit, &QLineEdit::textChanged, this, &MainWindow::onCustomerSearch);
    connect(m_equipmentSearchEdit, &QLineEdit::textChanged, this, &MainWindow::onEquipmentSearch);
    
    // Соединения таблиц
    connect(m_customerTable, &QTableWidget::itemSelectionChanged, this, &MainWindow::onCustomerSelectionChanged);
    connect(m_equipmentTable, &QTableWidget::itemSelectionChanged, this, &MainWindow::onEquipmentSelectionChanged);
    connect(m_rentalTable, &QTableWidget::itemSelectionChanged, this, &MainWindow::onRentalSelectionChanged);
    
    // Двойной клик для редактирования
    connect(m_customerTable, &QTableWidget::itemDoubleClicked, this, &MainWindow::onCustomerDoubleClicked);
    connect(m_equipmentTable, &QTableWidget::itemDoubleClicked, this, &MainWindow::onEquipmentDoubleClicked);
    connect(m_rentalTable, &QTableWidget::itemDoubleClicked, this, &MainWindow::onRentalDoubleClicked);
}

void MainWindow::updateStatus()
{
    m_statusLabel->setText("Система готова к работе");
}

void MainWindow::loadSettings()
{
    QSettings settings;
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
}

void MainWindow::saveSettings()
{
    QSettings settings;
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
}

// Слоты для обработки событий
void MainWindow::onNewCustomer()
{
    m_statusLabel->setText("Создание нового клиента...");
    CustomerDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        Customer* customer = dialog.getCustomer();
        if (customer->save()) {
            refreshCustomerTable();
            statusBar()->showMessage("Клиент успешно создан", 3000);
            AuditLogger::instance().log("Customer created", QString("id=%1 name=%2").arg(customer->getId()).arg(customer->getName()));
        } else {
            QMessageBox::warning(this, "Ошибка", "Не удалось создать клиента");
            AuditLogger::instance().log("Customer create failed", "", AuditSeverity::Error);
        }
        delete customer;
    }
    m_statusLabel->setText("Готово");
}

void MainWindow::onNewEquipment()
{
    m_statusLabel->setText("Создание нового оборудования...");
    EquipmentDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        Equipment* equipment = dialog.getEquipment();
        if (equipment->save()) {
            refreshEquipmentTable();
            statusBar()->showMessage("Оборудование успешно создано", 3000);
            AuditLogger::instance().log("Equipment created", QString("id=%1 name=%2").arg(equipment->getId()).arg(equipment->getName()));
        } else {
            QMessageBox::warning(this, "Ошибка", "Не удалось создать оборудование");
            AuditLogger::instance().log("Equipment create failed", "", AuditSeverity::Error);
        }
        delete equipment;
    }
    m_statusLabel->setText("Готово");
}

void MainWindow::onNewRental()
{
    m_statusLabel->setText("Создание новой аренды...");
    RentalDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        Rental* rental = dialog.getRental();
        if (rental->save()) {
            refreshRentalTable();
            statusBar()->showMessage("Аренда успешно создана", 3000);
            AuditLogger::instance().log("Rental created",
                QString("id=%1 cust=%2 eq=%3 qty=%4")
                    .arg(rental->getId())
                    .arg(rental->getCustomer()?rental->getCustomer()->getName():"")
                    .arg(rental->getEquipment()?rental->getEquipment()->getName():"")
                    .arg(rental->getQuantity()));
        } else {
            QMessageBox::warning(this, "Ошибка", "Не удалось создать аренду");
            AuditLogger::instance().log("Rental create failed", "", AuditSeverity::Error);
        }
        delete rental;
    }
    m_statusLabel->setText("Готово");
}

void MainWindow::onSearchCustomer()
{
    m_statusLabel->setText("Поиск клиентов...");
    QString searchTerm = QInputDialog::getText(this, "Поиск клиентов", 
                                             "Введите имя, телефон или email:");
    if (!searchTerm.isEmpty()) {
        QList<Customer*> customers = Customer::search(searchTerm);
        displayCustomers(customers);
        statusBar()->showMessage(QString("Найдено клиентов: %1").arg(customers.size()), 3000);
    }
}

void MainWindow::onSearchEquipment()
{
    m_statusLabel->setText("Поиск оборудования...");
    QString searchTerm = QInputDialog::getText(this, "Поиск оборудования", 
                                             "Введите название или категорию:");
    if (!searchTerm.isEmpty()) {
        QList<Equipment*> equipment = Equipment::search(searchTerm);
        displayEquipment(equipment);
        statusBar()->showMessage(QString("Найдено оборудования: %1").arg(equipment.size()), 3000);
    }
}

void MainWindow::onViewRentals()
{
    m_statusLabel->setText("Просмотр аренд...");
    refreshRentalTable();
    m_tabWidget->setCurrentIndex(2); // Переключаемся на вкладку аренд
    statusBar()->showMessage("Таблица аренд обновлена", 2000);
}

void MainWindow::onReports()
{
    m_statusLabel->setText("Генерация отчета...");
    
    QString reportType = m_reportTypeCombo->currentText();
    QDate startDate = m_reportStartDate->date();
    QDate endDate = m_reportEndDate->date();
    
    QString report = QString("<h2>%1</h2>").arg(reportType);
    report += QString("<p><b>Период:</b> %1 - %2</p>").arg(startDate.toString("dd.MM.yyyy")).arg(endDate.toString("dd.MM.yyyy"));
    report += QString("<p><b>Дата генерации:</b> %1</p>").arg(QDateTime::currentDateTime().toString("dd.MM.yyyy HH:mm"));
    report += "<hr>";
    
    // Генерируем реальные отчеты
    if (reportType == "Отчет по арендам") {
        QList<Rental*> rentals = Rental::getAll();
        double totalRevenue = 0.0;
        double totalDeposits = 0.0;
        int totalRentals = 0;
        int activeRentals = 0;
        int completedRentals = 0;
        int overdueRentals = 0;
        
        report += "<h3>Статистика аренд</h3>";
        report += "<table border='1' cellpadding='5' cellspacing='0' style='border-collapse: collapse; width: 100%;'>";
        report += "<tr style='background-color: #1976d2; color: white;'>";
        report += "<th>ID</th><th>Клиент</th><th>Оборудование</th><th>Количество</th><th>Дата начала</th><th>Дата окончания</th><th>Статус</th><th>Стоимость аренды</th><th>Залог</th>";
        report += "</tr>";
        
        for (Rental* rental : rentals) {
            if (rental->getStartDate().date() >= startDate && 
                rental->getStartDate().date() <= endDate) {
                
                QString status = rental->getStatus();
                if (status == "active") {
                    if (rental->isOverdue()) {
                        status = "Просрочено";
                        overdueRentals++;
                    } else {
                        status = "Активна";
                        activeRentals++;
                    }
                } else if (status == "completed") {
                    completedRentals++;
                }
                
                totalRevenue += rental->getTotalPrice();
                totalDeposits += rental->getDeposit();
                totalRentals++;
                
                report += "<tr>";
                report += QString("<td>%1</td>").arg(rental->getId());
                report += QString("<td>%1</td>").arg(rental->getCustomer()->getName());
                report += QString("<td>%1</td>").arg(rental->getEquipment()->getName());
                report += QString("<td>%1</td>").arg(rental->getQuantity());
                report += QString("<td>%1</td>").arg(rental->getStartDate().toString("dd.MM.yyyy HH:mm"));
                report += QString("<td>%1</td>").arg(rental->getEndDate().toString("dd.MM.yyyy HH:mm"));
                report += QString("<td>%1</td>").arg(status);
                report += QString("<td>%1 ₽</td>").arg(QString::number(rental->getTotalPrice(), 'f', 2));
                report += QString("<td>%1 ₽</td>").arg(QString::number(rental->getDeposit(), 'f', 2));
                report += "</tr>";
            }
        }
        
        report += "</table>";
        report += "<br><h3>Итоговая статистика</h3>";
        report += QString("<p><b>Всего аренд за период:</b> %1</p>").arg(totalRentals);
        report += QString("<p><b>Активных аренд:</b> %1</p>").arg(activeRentals);
        report += QString("<p><b>Завершенных аренд:</b> %1</p>").arg(completedRentals);
        report += QString("<p><b>Просроченных аренд:</b> %1</p>").arg(overdueRentals);
        report += QString("<p><b>Общая выручка:</b> <span style='color: green; font-weight: bold;'>%1 ₽</span></p>").arg(QString::number(totalRevenue, 'f', 2));
        report += QString("<p><b>Общая сумма залогов:</b> <span style='color: blue; font-weight: bold;'>%1 ₽</span></p>").arg(QString::number(totalDeposits, 'f', 2));
        
    } else if (reportType == "Отчет по оборудованию") {
        QList<Equipment*> equipment = Equipment::getAll();
        report += QString("<h3>Всего оборудования: %1</h3>").arg(equipment.size());
        
        QMap<QString, int> categoryCount;
        QMap<QString, double> categoryRevenue;
        
        for (Equipment* item : equipment) {
            categoryCount[item->getCategory()]++;
            // Примерная выручка (можно улучшить, добавив реальные данные)
            categoryRevenue[item->getCategory()] += item->getPrice() * 30; // 30 дней
        }
        
        report += "<h3>По категориям:</h3>";
        report += "<table border='1' cellpadding='5' cellspacing='0' style='border-collapse: collapse; width: 100%;'>";
        report += "<tr style='background-color: #1976d2; color: white;'>";
        report += "<th>Категория</th><th>Количество</th><th>Примерная месячная выручка (₽)</th>";
        report += "</tr>";
        
        for (auto it = categoryCount.begin(); it != categoryCount.end(); ++it) {
            report += "<tr>";
            report += QString("<td>%1</td>").arg(it.key());
            report += QString("<td>%1 шт.</td>").arg(it.value());
            report += QString("<td>%1 ₽</td>").arg(QString::number(categoryRevenue[it.key()], 'f', 2));
            report += "</tr>";
        }
        
        report += "</table>";
        
    } else if (reportType == "Отчет по клиентам") {
        QList<Customer*> customers = Customer::getAll();
        report += QString("<h3>Всего клиентов: %1</h3>").arg(customers.size());
        
        // Статистика по новым клиентам за период
        int newCustomers = 0;
        QList<Customer*> newCustomersList;
        
        for (Customer* customer : customers) {
            if (customer->getCreatedAt().date() >= startDate && 
                customer->getCreatedAt().date() <= endDate) {
                newCustomers++;
                newCustomersList.append(customer);
            }
        }
        
        report += QString("<h3>Новых клиентов за период: %1</h3>").arg(newCustomers);
        
        if (!newCustomersList.isEmpty()) {
            report += "<table border='1' cellpadding='5' cellspacing='0' style='border-collapse: collapse; width: 100%;'>";
            report += "<tr style='background-color: #1976d2; color: white;'>";
            report += "<th>Имя</th><th>Телефон</th><th>Email</th><th>Дата регистрации</th>";
            report += "</tr>";
            
            for (Customer* customer : newCustomersList) {
                report += "<tr>";
                report += QString("<td>%1</td>").arg(customer->getName());
                report += QString("<td>%1</td>").arg(customer->getPhone());
                report += QString("<td>%1</td>").arg(customer->getEmail());
                report += QString("<td>%1</td>").arg(customer->getCreatedAt().toString("dd.MM.yyyy"));
                report += "</tr>";
            }
            
            report += "</table>";
        }
        
    } else if (reportType == "Финансовый отчет") {
        QList<Rental*> rentals = Rental::getAll();
        double totalRevenue = 0.0;
        double totalDeposits = 0.0;
        double totalDamage = 0.0;
        double totalCleaning = 0.0;
        int totalRentals = 0;
        int completedRentals = 0;
        int activeRentals = 0;
        
        for (Rental* rental : rentals) {
            if (rental->getStartDate().date() >= startDate && 
                rental->getStartDate().date() <= endDate) {
                totalRevenue += rental->getTotalPrice();
                totalDeposits += rental->getDeposit();
                totalDamage += rental->getDamageCost();
                totalCleaning += rental->getCleaningCost();
                totalRentals++;
                
                if (rental->getStatus() == "completed") {
                    completedRentals++;
                } else if (rental->getStatus() == "active") {
                    activeRentals++;
                }
            }
        }
        
        report += "<h3>Финансовая статистика</h3>";
        report += "<table border='1' cellpadding='5' cellspacing='0' style='border-collapse: collapse; width: 100%;'>";
        report += "<tr style='background-color: #1976d2; color: white;'>";
        report += "<th>Показатель</th><th>Значение</th>";
        report += "</tr>";
        report += QString("<tr><td>Общая выручка</td><td style='color: green; font-weight: bold;'>%1 ₽</td></tr>").arg(QString::number(totalRevenue, 'f', 2));
        report += QString("<tr><td>Общие залоги</td><td style='color: blue; font-weight: bold;'>%1 ₽</td></tr>").arg(QString::number(totalDeposits, 'f', 2));
        report += QString("<tr><td>Стоимость повреждений</td><td style='color: red; font-weight: bold;'>%1 ₽</td></tr>").arg(QString::number(totalDamage, 'f', 2));
        report += QString("<tr><td>Стоимость уборки</td><td style='color: orange; font-weight: bold;'>%1 ₽</td></tr>").arg(QString::number(totalCleaning, 'f', 2));
        report += QString("<tr><td>Чистая прибыль</td><td style='color: green; font-weight: bold;'>%1 ₽</td></tr>").arg(QString::number(totalRevenue - totalDamage - totalCleaning, 'f', 2));
        report += "</table>";
        
        report += "<br><h3>Статистика аренд</h3>";
        report += QString("<p><b>Всего аренд:</b> %1</p>").arg(totalRentals);
        report += QString("<p><b>Завершенных:</b> %1</p>").arg(completedRentals);
        report += QString("<p><b>Активных:</b> %1</p>").arg(activeRentals);
    }
    
    m_reportsText->setHtml(report);
    m_statusLabel->setText("Отчет сгенерирован");

    AuditLogger::instance().log("Report generated",
        QString("type=%1 period=%2..%3")
            .arg(reportType)
            .arg(startDate.toString("yyyy-MM-dd"))
            .arg(endDate.toString("yyyy-MM-dd")));
}

void MainWindow::onSettings()
{
    m_statusLabel->setText("Открытие настроек");
    
    // Улучшенный диалог настроек
    QDialog settingsDialog(this);
    settingsDialog.setWindowTitle("Настройки приложения");
    settingsDialog.setModal(true);
    settingsDialog.setMinimumWidth(500);
    settingsDialog.setMinimumHeight(400);
    
    QVBoxLayout* layout = new QVBoxLayout(&settingsDialog);
    
    // Внешний вид (упрощено): оставляем только параметры обновления
    QGroupBox* uiGroup = new QGroupBox("Интерфейс", &settingsDialog);
    QFormLayout* uiLayout = new QFormLayout(uiGroup);
    QCheckBox* autoRefreshCheck = new QCheckBox("Автообновление таблиц", &settingsDialog);
    autoRefreshCheck->setChecked(true);
    uiLayout->addRow("", autoRefreshCheck);
    QSpinBox* refreshIntervalSpin = new QSpinBox(&settingsDialog);
    refreshIntervalSpin->setRange(30, 300);
    refreshIntervalSpin->setValue(60);
    refreshIntervalSpin->setSuffix(" сек");
    uiLayout->addRow("Интервал обновления:", refreshIntervalSpin);
    layout->addWidget(uiGroup);
    
    // Настройки базы данных
    QGroupBox* dbGroup = new QGroupBox("База данных", &settingsDialog);
    QFormLayout* dbLayout = new QFormLayout(dbGroup);
    
    QPushButton* backupBtn = new QPushButton("Создать резервную копию", dbGroup);
    QPushButton* restoreBtn = new QPushButton("Восстановить из копии", dbGroup);
    QPushButton* auditBtn = new QPushButton("Журнал событий", dbGroup);
    
    dbLayout->addRow("", auditBtn);
    dbLayout->addRow("", backupBtn);
    dbLayout->addRow("", restoreBtn);
    
    layout->addWidget(dbGroup);
    
    // Настройки уведомлений
    QGroupBox* notifyGroup = new QGroupBox("Уведомления", &settingsDialog);
    QFormLayout* notifyLayout = new QFormLayout(notifyGroup);
    
    QCheckBox* overdueNotifyCheck = new QCheckBox("Уведомления о просроченных арендах", notifyGroup);
    overdueNotifyCheck->setChecked(true);
    notifyLayout->addRow("", overdueNotifyCheck);
    
    QCheckBox* lowStockNotifyCheck = new QCheckBox("Уведомления о низком количестве оборудования", notifyGroup);
    lowStockNotifyCheck->setChecked(true);
    notifyLayout->addRow("", lowStockNotifyCheck);
    
    layout->addWidget(notifyGroup);
    
    // Кнопки
    QDialogButtonBox* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply, &settingsDialog);
    layout->addWidget(buttonBox);
    
    // Подключения
    connect(buttonBox->button(QDialogButtonBox::Apply), &QPushButton::clicked, [&]() {
        loadStyleSheet("light");
        statusBar()->showMessage("Настройки применены", 2000);
    });
    
    connect(auditBtn, &QPushButton::clicked, [&, this](){
        if (!AdminGuard::ensureAdmin(this, &m_adminSession, &m_adminMgr)) return;
        AuditLogger::instance().log("Open audit log", "", AuditSeverity::Security);
        AuditLogDialog dlg(this);
        dlg.exec();
    });

    connect(backupBtn, &QPushButton::clicked, [&, this]() {
        if (!AdminGuard::ensureAdmin(this, &m_adminSession, &m_adminMgr)) return;

        const QString suggested = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                                + "/rental_backup_" + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".db";

        const QString fileName = QFileDialog::getSaveFileName(&settingsDialog,
            "Сохранить резервную копию", suggested, "База данных (*.db);;Все файлы (*)");
        if (fileName.isEmpty()) return;

        if (m_database->backupDatabase(fileName)) {
            QMessageBox::information(&settingsDialog, "Успех", "Резервная копия создана успешно!");
        } else {
            QMessageBox::warning(&settingsDialog, "Ошибка", "Не удалось создать резервную копию.");
        }
    });

    connect(restoreBtn, &QPushButton::clicked, [&, this]() {
        if (!AdminGuard::ensureAdmin(this, &m_adminSession, &m_adminMgr)) return;

        const QString fileName = QFileDialog::getOpenFileName(&settingsDialog,
            "Выбрать файл для восстановления",
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
            "База данных (*.db);;Все файлы (*)");
        if (fileName.isEmpty()) return;

        if (QMessageBox::question(&settingsDialog, "Подтверждение",
            "Восстановление базы данных приведёт к потере текущих данных. Продолжить?",
            QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;

        if (m_database->restoreDatabase(fileName)) {
            refreshCustomerTable();
            refreshEquipmentTable();
            refreshRentalTable();
            QMessageBox::information(&settingsDialog, "Готово", "База данных успешно восстановлена.");
        } else {
            QMessageBox::warning(&settingsDialog, "Ошибка", "Не удалось восстановить базу данных.");
        }
    });
    
    connect(buttonBox, &QDialogButtonBox::accepted, &settingsDialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &settingsDialog, &QDialog::reject);
    
    if (settingsDialog.exec() == QDialog::Accepted) {
        loadStyleSheet("light");
        statusBar()->showMessage("Настройки сохранены и применены", 2000);
    }
}

void MainWindow::onAbout()
{
    QMessageBox::about(this, "О программе", 
                      "Система проката оборудования\n"
                      "Версия 1.0.0\n\n"
                      "Разработано для управления прокатом оборудования\n"
                      "с защищенным хранением данных.");
}

void MainWindow::onExit()
{
    close();
}

// Дополнительные функции для редактирования и удаления
void MainWindow::onEditCustomer()
{
    if (m_selectedCustomerId == -1) {
        QMessageBox::information(this, "Информация", "Выберите клиента для редактирования");
        return;
    }
    
    Customer* customer = Customer::loadById(m_selectedCustomerId);
    if (!customer) {
        QMessageBox::warning(this, "Ошибка", "Не удалось загрузить данные клиента");
        return;
    }
    
    CustomerDialog dialog(customer, this);
    if (dialog.exec() == QDialog::Accepted) {
        if (customer->update()) {
            refreshCustomerTable();
            statusBar()->showMessage("Клиент успешно обновлен", 3000);
            AuditLogger::instance().log("Customer updated", QString("id=%1 name=%2").arg(customer->getId()).arg(customer->getName()));

        } else {
            QMessageBox::warning(this, "Ошибка", "Не удалось обновить клиента");
            AuditLogger::instance().log("Customer update failed", QString("id=%1").arg(customer->getId()), AuditSeverity::Error);
        }
    }
    delete customer;
}

void MainWindow::onEditEquipment()
{
    if (!AdminGuard::ensureAdmin(this, &m_adminSession, &m_adminMgr)) return;

    if (m_selectedEquipmentId == -1) {
        QMessageBox::information(this, "Информация", "Выберите оборудование для редактирования");
        return;
    }
    
    Equipment* equipment = Equipment::loadById(m_selectedEquipmentId);
    if (!equipment) {
        QMessageBox::warning(this, "Ошибка", "Не удалось загрузить данные оборудования");
        return;
    }
    
    EquipmentDialog dialog(equipment, this);
    if (dialog.exec() == QDialog::Accepted) {
        if (equipment->update()) {
            refreshEquipmentTable();
            statusBar()->showMessage("Оборудование успешно обновлено", 3000);
            AuditLogger::instance().log("Equipment updated", QString("id=%1 name=%2").arg(equipment->getId()).arg(equipment->getName()));
        } else {
            QMessageBox::warning(this, "Ошибка", "Не удалось обновить оборудование");
            AuditLogger::instance().log("Equipment update failed", QString("id=%1").arg(equipment->getId()), AuditSeverity::Error);
        }
    }
    delete equipment;
}

void MainWindow::onDeleteCustomer()
{
    if (m_selectedCustomerId == -1) {
        QMessageBox::information(this, "Информация", "Выберите клиента для удаления");
        return;
    }
    
    QMessageBox::StandardButton reply = QMessageBox::question(this, "Подтверждение", 
        "Вы уверены, что хотите удалить этого клиента? Это действие нельзя отменить.",
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        Customer* customer = Customer::loadById(m_selectedCustomerId);
        if (customer && customer->remove()) {
            refreshCustomerTable();
            m_selectedCustomerId = -1;
            statusBar()->showMessage("Клиент успешно удален", 3000);
            AuditLogger::instance().log("Customer deleted", QString("id=%1").arg(m_selectedCustomerId), AuditSeverity::Security);
        } else {
            QMessageBox::warning(this, "Ошибка", "Не удалось удалить клиента");
            AuditLogger::instance().log("Customer delete failed", QString("id=%1").arg(m_selectedCustomerId), AuditSeverity::Error);
        }
        delete customer;
    }
}

void MainWindow::onDeleteEquipment()
{
    

    if (!AdminGuard::ensureAdmin(this, &m_adminSession, &m_adminMgr)) return;

    if (m_selectedEquipmentId == -1) {
        QMessageBox::information(this, "Информация", "Выберите оборудование для удаления");
        return;
    }
    
    QMessageBox::StandardButton reply = QMessageBox::question(this, "Подтверждение", 
        "Вы уверены, что хотите удалить это оборудование? Это действие нельзя отменить.",
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        Equipment* equipment = Equipment::loadById(m_selectedEquipmentId);
        if (equipment && equipment->remove()) {
            refreshEquipmentTable();
            m_selectedEquipmentId = -1;
            statusBar()->showMessage("Оборудование успешно удалено", 3000);
            AuditLogger::instance().log("Equipment deleted", QString("id=%1").arg(m_selectedEquipmentId), AuditSeverity::Security);
        } else {
            QMessageBox::warning(this, "Ошибка", "Не удалось удалить оборудование");
            AuditLogger::instance().log("Equipment delete failed", QString("id=%1").arg(m_selectedEquipmentId), AuditSeverity::Error);
        }
        delete equipment;
    }
}

void MainWindow::onCompleteRental()
{
    if (m_selectedRentalId == -1) {
        QMessageBox::information(this, "Информация", "Выберите аренду для завершения");
        return;
    }
    
    Rental* rental = Rental::loadById(m_selectedRentalId);
    if (!rental) {
        QMessageBox::warning(this, "Ошибка", "Не удалось загрузить данные аренды");
        return;
    }
    
    // Диалог для завершения аренды
    QDialog dialog(this);
    dialog.setWindowTitle("Завершение аренды");
    dialog.setModal(true);
    dialog.setMinimumWidth(400);
    
    QVBoxLayout* layout = new QVBoxLayout(&dialog);
    
    QFormLayout* formLayout = new QFormLayout();
    
    QDoubleSpinBox* damageCostSpin = new QDoubleSpinBox(&dialog);
    damageCostSpin->setRange(0, 10000);
    damageCostSpin->setSuffix(" ₽");
    formLayout->addRow("Стоимость повреждений:", damageCostSpin);
    
    QDoubleSpinBox* cleaningCostSpin = new QDoubleSpinBox(&dialog);
    cleaningCostSpin->setRange(0, 1000);
    cleaningCostSpin->setSuffix(" ₽");
    formLayout->addRow("Стоимость уборки:", cleaningCostSpin);
    
    QDoubleSpinBox* finalDepositSpin = new QDoubleSpinBox(&dialog);
    finalDepositSpin->setRange(0, rental->getDeposit());
    finalDepositSpin->setValue(rental->getDeposit());
    finalDepositSpin->setSuffix(" ₽");
    formLayout->addRow("Возврат залога:", finalDepositSpin);
    
    QTextEdit* notesEdit = new QTextEdit(&dialog);
    notesEdit->setMaximumHeight(100);
    notesEdit->setPlaceholderText("Дополнительные заметки...");
    formLayout->addRow("Заметки:", notesEdit);
    
    layout->addLayout(formLayout);
    
    QDialogButtonBox* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    layout->addWidget(buttonBox);
    
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    
    if (dialog.exec() == QDialog::Accepted) {
        if (rental->complete(damageCostSpin->value(), cleaningCostSpin->value(), finalDepositSpin->value(), notesEdit->toPlainText())) {
            refreshRentalTable();
            statusBar()->showMessage("Аренда успешно завершена", 3000);
            AuditLogger::instance().log("Rental completed", QString("id=%1").arg(rental->getId()));
        } else {
            QMessageBox::warning(this, "Ошибка", "Не удалось завершить аренду");
            AuditLogger::instance().log("Rental complete failed", QString("id=%1").arg(rental->getId()), AuditSeverity::Error);
        }
    }
    delete rental;
}

void MainWindow::onViewRental()
{
    if (m_selectedRentalId == -1) {
        QMessageBox::information(this, "Информация", "Выберите аренду для просмотра");
        return;
    }
    
    Rental* rental = Rental::loadById(m_selectedRentalId);
    if (!rental) {
        QMessageBox::warning(this, "Ошибка", "Не удалось загрузить данные аренды");
        return;
    }
    
    // Диалог для просмотра деталей аренды
    QDialog dialog(this);
    dialog.setWindowTitle("Детали аренды");
    dialog.setModal(true);
    dialog.setMinimumWidth(500);
    
    QVBoxLayout* layout = new QVBoxLayout(&dialog);
    
    QFormLayout* formLayout = new QFormLayout();
    
    QLabel* customerLabel = new QLabel(rental->getCustomer()->getName(), &dialog);
    formLayout->addRow("Клиент:", customerLabel);
    
    QLabel* equipmentLabel = new QLabel(rental->getEquipment()->getName(), &dialog);
    formLayout->addRow("Оборудование:", equipmentLabel);
    
    QLabel* quantityLabel = new QLabel(QString::number(rental->getQuantity()), &dialog);
    formLayout->addRow("Количество:", quantityLabel);
    
    QLabel* startDateLabel = new QLabel(rental->getStartDate().toString("dd.MM.yyyy HH:mm"), &dialog);
    formLayout->addRow("Дата начала:", startDateLabel);
    
    QLabel* endDateLabel = new QLabel(rental->getEndDate().toString("dd.MM.yyyy HH:mm"), &dialog);
    formLayout->addRow("Дата окончания:", endDateLabel);
    
    QLabel* totalPriceLabel = new QLabel(QString::number(rental->getTotalPrice(), 'f', 2) + " ₽", &dialog);
    formLayout->addRow("Стоимость аренды:", totalPriceLabel);
    
    QLabel* depositLabel = new QLabel(QString::number(rental->getDeposit(), 'f', 2) + " ₽", &dialog);
    formLayout->addRow("Залог:", depositLabel);
    
    QLabel* statusLabel = new QLabel(rental->getStatusText(), &dialog);
    formLayout->addRow("Статус:", statusLabel);
    
    QLabel* notesLabel = new QLabel(rental->getNotes(), &dialog);
    formLayout->addRow("Заметки:", notesLabel);
    
    layout->addLayout(formLayout);
    
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    layout->addWidget(buttonBox);
    
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    
    dialog.exec();
    delete rental;
}

void MainWindow::onPrintRental()
{
    if (m_selectedRentalId == -1) {
        QMessageBox::information(this, "Информация", "Выберите аренду для печати");
        return;
    }
    Rental* rental = Rental::loadById(m_selectedRentalId);
    if (!rental) {
        QMessageBox::warning(this, "Ошибка", "Не удалось загрузить данные аренды");
        return;
    }

    Customer* c = rental->getCustomer();
    Equipment* e = rental->getEquipment();
    const QString customerName = c ? c->getName() : "";
    const QString customerPhone = c ? c->getPhone() : "";
    const QString customerEmail = c ? c->getEmail() : "";
    const QString customerPassport = c ? c->getPassport() : "";
    const QString customerAddress = c ? c->getAddress() : "";

    const QString equipmentName = e ? e->getName() : "";
    const QString equipmentCategory = e ? e->getCategory() : "";
    const QString equipmentPrice = e ? QString::number(e->getPrice(), 'f', 2) : "0.00";

    const QString startDt = rental->getStartDate().toString("dd.MM.yyyy HH:mm");
    const QString endDt = rental->getEndDate().toString("dd.MM.yyyy HH:mm");
    const QString quantity = QString::number(rental->getQuantity());
    const QString totalPrice = QString::number(rental->getTotalPrice(), 'f', 2);
    const QString deposit = QString::number(rental->getDeposit(), 'f', 2);

    QString html;
    html += "<html><head><meta charset='utf-8'><style>";
    html += "body{font-family:'Segoe UI',Arial,sans-serif;color:#111;} h1{font-size:18px;margin:0 0 12px;}";
    html += "table{width:100%;border-collapse:collapse;margin-top:10px;} th,td{border:1px solid #444;padding:6px;text-align:left;font-size:12px;}";
    html += "section{margin-bottom:12px;} .muted{color:#666;font-size:12px;}";
    html += "</style></head><body>";
    html += "<h1>Договор аренды оборудования</h1>";
    html += QString("<div class='muted'>Дата печати: %1</div>").arg(QDateTime::currentDateTime().toString("dd.MM.yyyy HH:mm"));

    html += "<section><h3>Стороны</h3>";
    html += "<table>";
    html += QString("<tr><th>Клиент</th><td>%1</td></tr>").arg(customerName.toHtmlEscaped());
    html += QString("<tr><th>Телефон</th><td>%1</td></tr>").arg(customerPhone.toHtmlEscaped());
    html += QString("<tr><th>Email</th><td>%1</td></tr>").arg(customerEmail.toHtmlEscaped());
    html += QString("<tr><th>Паспорт</th><td>%1</td></tr>").arg(customerPassport.toHtmlEscaped());
    html += QString("<tr><th>Адрес регистрации</th><td>%1</td></tr>").arg(customerAddress.toHtmlEscaped());
    html += "</table></section>";

    html += "<section><h3>Предмет договора</h3>";
    html += "<table>";
    html += QString("<tr><th>Оборудование</th><td>%1</td></tr>").arg(equipmentName.toHtmlEscaped());
    html += QString("<tr><th>Категория</th><td>%1</td></tr>").arg(equipmentCategory.toHtmlEscaped());
    html += QString("<tr><th>Количество</th><td>%1</td></tr>").arg(quantity.toHtmlEscaped());
    html += QString("<tr><th>Период аренды</th><td>%1 — %2</td></tr>").arg(startDt, endDt);
    html += "</table></section>";

    html += "<section><h3>Стоимость</h3>";
    html += "<table>";
    html += QString("<tr><th>Цена за 1-й день</th><td>%1 ₽</td></tr>").arg(equipmentPrice);
    html += QString("<tr><th>Итоговая стоимость</th><td><b>%1 ₽</b></td></tr>").arg(totalPrice);
    html += QString("<tr><th>Залог</th><td>%1 ₽</td></tr>").arg(deposit);
    html += "</table></section>";

    if (!rental->getNotes().isEmpty()) {
        html += QString("<section><h3>Примечания</h3><div>%1</div></section>").arg(rental->getNotes().toHtmlEscaped());
    }

    html += "<section><h3>Подписи</h3>";
    html += "<table><tr><th style='width:40%'>Арендодатель</th><th style='width:40%'>Арендатор</th></tr>";
    html += "<tr><td style='height:60px;vertical-align:bottom'>____________________</td><td style='vertical-align:bottom'>____________________</td></tr></table>";
    html += "</section>";

    html += "</body></html>";

    // Option A: Fill user-provided DOCX template if present
    QString templatePath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/Договор Прокат Всего  2025.docx";
    if (!QFile::exists(templatePath)) {
        templatePath = QFileDialog::getOpenFileName(this,
            "Выберите DOCX шаблон договора",
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
            "Шаблон договора (*.docx)");
    }
    QMap<QString, QString> placeholders = {
        {"{{DATE}}", QDate::currentDate().toString("dd.MM.yyyy")},
        {"{{CUSTOMER_NAME}}", customerName},
        {"{{CUSTOMER_PHONE}}", customerPhone},
        {"{{CUSTOMER_EMAIL}}", customerEmail},
        {"{{CUSTOMER_PASSPORT}}", customerPassport},
        {"{{CUSTOMER_ADDRESS}}", customerAddress},
        {"{{EQUIPMENT_NAME}}", equipmentName},
        {"{{EQUIPMENT_CATEGORY}}", equipmentCategory},
        {"{{QUANTITY}}", quantity},
        {"{{START}}", startDt},
        {"{{END}}", endDt},
        {"{{TOTAL}}", totalPrice},
        {"{{DEPOSIT}}", deposit},
        {"{{NOTES}}", rental->getNotes()}
    };
    QString filledDocxPath;
    bool docxOk = false;
    if (QFile::exists(templatePath)) {
        docxOk = generateDocxFromTemplate(templatePath, placeholders, filledDocxPath);
    }
    if (docxOk) {
        QMessageBox::information(this, "Готово", QString("Заполненный договор сохранен: %1").arg(filledDocxPath));
        QDesktopServices::openUrl(QUrl::fromLocalFile(filledDocxPath));
    } else {
        if (!templatePath.isEmpty()) {
            QMessageBox::warning(this, "Шаблон не заполнен",
                                 "Не удалось заполнить шаблон .docx. Будет напечатана HTML-версия из приложения.");
        }
        // Fallback: print HTML
        QTextDocument doc;
        doc.setHtml(html);
        QPrinter printer(QPrinter::HighResolution);
        printer.setPageSize(QPageSize(QPageSize::A4));
        QPrintDialog dialog(&printer, this);
        dialog.setWindowTitle("Печать договора аренды");
        if (dialog.exec() == QDialog::Accepted) {
            doc.print(&printer);
        }
    }

    AuditLogger::instance().log("Contract printed", QString("rental_id=%1").arg(m_selectedRentalId));

    delete rental;
}

// Simple DOCX templating via unzip/zip (works on Linux and Windows with 7zip/zip installed)
bool MainWindow::generateDocxFromTemplate(const QString& templatePath, const QMap<QString, QString>& values, QString& outputDocxPath)
{
    if (!QFile::exists(templatePath)) return false;

    QTemporaryDir tempDir;
    if (!tempDir.isValid()) return false;
    QString tempPath = tempDir.path();

    // Unzip DOCX (ZIP) into temp directory
#if defined(Q_OS_WIN)
    // Try PowerShell Expand-Archive if available
    {
        QProcess p;
        QStringList args;
        args << "-NoLogo" << "-NoProfile" << "-Command"
             << QString("Expand-Archive -Path \"%1\" -DestinationPath \"%2\" -Force").arg(templatePath, tempPath);
        p.start("powershell", args);
        p.waitForFinished(-1);
        if (p.exitCode() != 0) return false;
    }
#else
    // Ensure unzip exists
    if (QProcess::execute("which", QStringList() << "unzip") != 0) return false;
    if (QProcess::execute("unzip", QStringList() << "-o" << templatePath << "-d" << tempPath) != 0) return false;
#endif

    // Replace placeholders in word/document.xml
    QString docXmlPath = tempPath + "/word/document.xml";
    QFile f(docXmlPath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return false;
    QString xml = QString::fromUtf8(f.readAll());
    f.close();
    for (auto it = values.constBegin(); it != values.constEnd(); ++it) {
        xml.replace(it.key(), it.value().toHtmlEscaped());
    }
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) return false;
    f.write(xml.toUtf8());
    f.close();

    // Zip back to DOCX
    QString outName = QString("Договор_аренды_%1.docx").arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmm"));
    outputDocxPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/" + outName;
#if defined(Q_OS_WIN)
    {
        QProcess p;
        QString t = tempPath; t.replace("\\", "/");
        QString o = outputDocxPath; o.replace("\\", "/");
        QStringList args;
        args << "-NoLogo" << "-NoProfile" << "-Command"
             << QString("Compress-Archive -Path \"%1/*\" -DestinationPath \"%2\" -Force").arg(t, o);
        p.start("powershell", args);
        p.waitForFinished(-1);
        if (p.exitCode() != 0) return false;
    }
#else
    // Ensure zip exists
    if (QProcess::execute("which", QStringList() << "zip") != 0) return false;
    {
        QProcess p;
        p.setWorkingDirectory(tempPath);
        QStringList args;
        args << "-r" << outputDocxPath << ".";
        p.start("zip", args);
        p.waitForFinished(-1);
        if (p.exitCode() != 0) return false;
    }
#endif
    return QFile::exists(outputDocxPath);
}

void MainWindow::onCustomerSearch()
{
    QString searchTerm = m_customerSearchEdit->text().trimmed();
    if (searchTerm.isEmpty()) {
        refreshCustomerTable();
    } else {
        QList<Customer*> customers = Customer::search(searchTerm);
        displayCustomers(customers);
    }
}

void MainWindow::onEquipmentSearch()
{
    QString searchTerm = m_equipmentSearchEdit->text().trimmed();
    if (searchTerm.isEmpty()) {
        refreshEquipmentTable();
    } else {
        QList<Equipment*> equipment = Equipment::search(searchTerm);
        displayEquipment(equipment);
    }
}

void MainWindow::onCustomerSelectionChanged()
{
    QList<QTableWidgetItem*> selectedItems = m_customerTable->selectedItems();
    if (selectedItems.isEmpty()) {
        m_selectedCustomerId = -1;
        m_editCustomerBtn->setEnabled(false);
        m_deleteCustomerBtn->setEnabled(false);
    } else {
        int row = selectedItems.first()->row();
        m_selectedCustomerId = m_customerTable->item(row, 0)->text().toInt();
        m_editCustomerBtn->setEnabled(true);
        m_deleteCustomerBtn->setEnabled(true);
    }
}

void MainWindow::onEquipmentSelectionChanged()
{
    QList<QTableWidgetItem*> selectedItems = m_equipmentTable->selectedItems();
    if (selectedItems.isEmpty()) {
        m_selectedEquipmentId = -1;
        m_editEquipmentBtn->setEnabled(false);
        m_deleteEquipmentBtn->setEnabled(false);
    } else {
        int row = selectedItems.first()->row();
        m_selectedEquipmentId = m_equipmentTable->item(row, 0)->text().toInt();
        m_editEquipmentBtn->setEnabled(true);
        m_deleteEquipmentBtn->setEnabled(true);
    }
}

void MainWindow::onRentalSelectionChanged()
{
    QList<QTableWidgetItem*> selectedItems = m_rentalTable->selectedItems();
    if (selectedItems.isEmpty()) {
        m_selectedRentalId = -1;
        m_completeRentalBtn->setEnabled(false);
        m_viewRentalBtn->setEnabled(false);
    } else {
        int row = selectedItems.first()->row();
        m_selectedRentalId = m_rentalTable->item(row, 0)->text().toInt();
        m_completeRentalBtn->setEnabled(true);
        m_viewRentalBtn->setEnabled(true);
    }
}

void MainWindow::onCustomerDoubleClicked()
{
    onEditCustomer();
}

void MainWindow::onEquipmentDoubleClicked()
{
    onEditEquipment();
}

void MainWindow::onRentalDoubleClicked()
{
    onViewRental();
}

// Table refresh methods
void MainWindow::refreshCustomerTable()
{
    QList<Customer*> customers = Customer::getAll();
    displayCustomers(customers);
}

void MainWindow::refreshEquipmentTable()
{
    QList<Equipment*> equipment = Equipment::getAll();
    displayEquipment(equipment);
}

void MainWindow::refreshRentalTable()
{
    QList<Rental*> rentals = Rental::getAll();
    displayRentals(rentals);
}

// Display methods
void MainWindow::displayCustomers(const QList<Customer*>& customers)
{
    m_customerTable->setRowCount(0);
    
    for (Customer* customer : customers) {
        int row = m_customerTable->rowCount();
        m_customerTable->insertRow(row);
        
        m_customerTable->setItem(row, 0, new QTableWidgetItem(QString::number(customer->getId())));
        m_customerTable->setItem(row, 1, new QTableWidgetItem(customer->getName()));
        m_customerTable->setItem(row, 2, new QTableWidgetItem(customer->getPhone()));
        m_customerTable->setItem(row, 3, new QTableWidgetItem(customer->getEmail()));
        m_customerTable->setItem(row, 4, new QTableWidgetItem(customer->getPassport()));
        m_customerTable->setItem(row, 5, new QTableWidgetItem(customer->getAddress()));
    }
}

void MainWindow::displayEquipment(const QList<Equipment*>& equipment)
{
    m_equipmentTable->setRowCount(0);
    
    for (Equipment* item : equipment) {
        int row = m_equipmentTable->rowCount();
        m_equipmentTable->insertRow(row);
        
        m_equipmentTable->setItem(row, 0, new QTableWidgetItem(QString::number(item->getId())));
        m_equipmentTable->setItem(row, 1, new QTableWidgetItem(item->getName()));
        m_equipmentTable->setItem(row, 2, new QTableWidgetItem(item->getCategory()));
        m_equipmentTable->setItem(row, 3, new QTableWidgetItem(QString::number(item->getPrice(), 'f', 2) + " ₽"));
        m_equipmentTable->setItem(row, 4, new QTableWidgetItem(QString::number(item->getDeposit(), 'f', 2) + " ₽"));
        m_equipmentTable->setItem(row, 5, new QTableWidgetItem(QString::number(item->getQuantity())));
        m_equipmentTable->setItem(row, 6, new QTableWidgetItem(item->isAvailable() ? "Доступно" : "Недоступно"));
    }
}

void MainWindow::displayRentals(const QList<Rental*>& rentals)
{
    m_rentalTable->setRowCount(0);
    
    for (Rental* rental : rentals) {
        int row = m_rentalTable->rowCount();
        m_rentalTable->insertRow(row);
        
        m_rentalTable->setItem(row, 0, new QTableWidgetItem(QString::number(rental->getId())));
        m_rentalTable->setItem(row, 1, new QTableWidgetItem(rental->getCustomer()->getName()));
        m_rentalTable->setItem(row, 2, new QTableWidgetItem(rental->getEquipment()->getName()));
        m_rentalTable->setItem(row, 3, new QTableWidgetItem(QString::number(rental->getQuantity())));
        m_rentalTable->setItem(row, 4, new QTableWidgetItem(rental->getStartDate().toString("dd.MM.yyyy HH:mm")));
        m_rentalTable->setItem(row, 5, new QTableWidgetItem(rental->getEndDate().toString("dd.MM.yyyy HH:mm")));
        m_rentalTable->setItem(row, 6, new QTableWidgetItem(rental->getStatusText()));
        m_rentalTable->setItem(row, 7, new QTableWidgetItem(QString::number(rental->getTotalPrice(), 'f', 2) + " ₽"));
        m_rentalTable->setItem(row, 8, new QTableWidgetItem(QString::number(rental->getDeposit(), 'f', 2) + " ₽"));
    }
}

// Style methods
void MainWindow::loadStyleSheet(const QString& theme)
{
    QFile file(QString(":/styles/%1.qss").arg(theme));
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        QString style = file.readAll();
        qApp->setStyleSheet(style);
        file.close();
    }
} 