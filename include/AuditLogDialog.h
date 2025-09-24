#pragma once

#include "AuditLogger.h"
#include <QDialog>
#include <QTableWidget>
#include <QDateEdit>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QFileDialog>
#include <QStandardPaths>
#include <QMessageBox>
#include <QDateTime>
#include <QTextStream>
#include <QHeaderView>   
#include <QLabel> 

class AuditLogDialog : public QDialog {
    Q_OBJECT
public:
    explicit AuditLogDialog(QWidget* parent = nullptr);
private slots:
    void reload();
    void exportCsv();
    void clearOld();
private:
    QTableWidget* m_table = nullptr;
    QLineEdit*    m_filterEdit = nullptr;
    QDateEdit*    m_fromEdit = nullptr;
    QDateEdit*    m_toEdit = nullptr;
    QComboBox*    m_sevCombo = nullptr;
    QPushButton*  m_btnExport = nullptr;
    QPushButton*  m_btnClear = nullptr;
};
