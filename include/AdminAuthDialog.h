#pragma once
#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui { class AdminAuthDialog; }
QT_END_NAMESPACE

class AdminAuthDialog : public QDialog {
    Q_OBJECT
public:
    explicit AdminAuthDialog(QWidget* parent = nullptr);
    ~AdminAuthDialog();
    QString password() const;

private slots:
    void onShowToggled(bool on); // слот, на который ссылается moc

private:
    Ui::AdminAuthDialog* ui;
};