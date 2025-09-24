#include "AdminGuard.h"

bool AdminGuard::ensureAdmin(QWidget* parent, AdminSession* session, AdminPasswordManager* mgr, int minutes)
{
    if (session->isElevated()){
        AuditLogger::instance().log("Admin auth success", "", AuditSeverity::Security);
        return true;
    }
    if (!mgr->hasPassword()) {
        QMessageBox::warning(parent, QObject::tr("Админ-пароль не настроен"),
                             QObject::tr("Админ-пароль ещё не установлен. "
                                         "Установите его в «Настройки → Безопасность»."));
        return false;
    }

    AdminAuthDialog dlg(parent);
    if (dlg.exec() != QDialog::Accepted) return false;

    if (!mgr->verifyPassword(dlg.password())) {
        QMessageBox::critical(parent, QObject::tr("Отказано"), QObject::tr("Неверный админ-пароль."));
        AuditLogger::instance().log("Admin auth failed", "", AuditSeverity::Security);
        return false;
    }

    session->elevateForMinutes(minutes);
    return true;
}