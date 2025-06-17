#ifndef JWT_HELPER_H
#define JWT_HELPER_H

#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QDateTime>
#include <QTimer>


class jwt_helper
{
public:
    jwt_helper();


    static QString generateJWT(int userId, const QString &email, const QString &role);
    static QJsonObject validateJWT(const QString &token);
};

#endif // JWT_HELPER_H
