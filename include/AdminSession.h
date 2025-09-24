#pragma once
#include <QObject>
#include <QDateTime>

class AdminSession : public QObject {
    Q_OBJECT
public:
    explicit AdminSession(QObject* parent = nullptr);
    bool isElevated() const;
    void elevateForMinutes(int minutes);
    void clear();
private:
    QDateTime m_expires;
};