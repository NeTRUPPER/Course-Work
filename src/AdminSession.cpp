#include "AdminSession.h"
AdminSession::AdminSession(QObject* p): QObject(p) {}
bool AdminSession::isElevated() const {
    return !m_expires.isNull() && QDateTime::currentDateTimeUtc() < m_expires;
}
void AdminSession::elevateForMinutes(int minutes) {
    m_expires = QDateTime::currentDateTimeUtc().addSecs(minutes*60);
}
void AdminSession::clear(){ m_expires = {}; }