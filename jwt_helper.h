#ifndef JWT_HELPER_H
#define JWT_HELPER_H

#include <QCryptographicHash>
#include <QDateTime>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRandomGenerator>
#include <QTimer>

class jwt_helper
{
public:
    jwt_helper();

    static QString generateJWT(int userId, const QString &email, const QString &role);
    static QJsonObject validateJWT(const QString &token);
};

#endif // JWT_HELPER_H
