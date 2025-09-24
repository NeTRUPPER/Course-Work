#include "AdminAuthDialog.h"
#include "ui_AdminAuthDialog.h"
#include <QCheckBox>
#include <QLineEdit>

AdminAuthDialog::AdminAuthDialog(QWidget* parent)
    : QDialog(parent), ui(new Ui::AdminAuthDialog)
{
    ui->setupUi(this);
    setWindowTitle(tr("Подтверждение прав"));

    // На случай, если в ui_ уже есть лямбда — дубль не критичен,
    // но этот слот точно удовлетворит moc.
    connect(ui->checkBoxShow, &QCheckBox::toggled,
            this, &AdminAuthDialog::onShowToggled);
}

AdminAuthDialog::~AdminAuthDialog() {
    delete ui;
}

QString AdminAuthDialog::password() const {
    return ui->lineEditPwd->text();
}

void AdminAuthDialog::onShowToggled(bool on) {
    ui->lineEditPwd->setEchoMode(on ? QLineEdit::Normal : QLineEdit::Password);
}