#pragma once
#include "AdminSession.h"
#include "AdminPasswordManager.h"
#include "AdminAuthDialog.h"
#include "AuditLogger.h"
#include <QMessageBox>
#include <QObject>

class QWidget;
class AdminSession;
class AdminPasswordManager;

class AdminGuard {
public:
    // Возвращает true, если доступ разрешён (уже был высокий уровень или пароль введён верно)
    static bool ensureAdmin(QWidget* parent,
                            AdminSession* session,
                            AdminPasswordManager* mgr,
                            int minutes = 15); // срок действия сессии
};