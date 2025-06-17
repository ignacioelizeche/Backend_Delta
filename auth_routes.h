#ifndef AUTH_ROUTES_H
#define AUTH_ROUTES_H

#include <QHttpServer>
#include <QDebug>
#include "response_utils.h"
#include "database_manager.h"  // Si usas singleton, sino elimina
#include "jwt_helper.h"        // Para JWTHelper::generateJWT y validateJWT
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QDebug>


class AuthRoutes {
public:
    static void setupRoutes(QHttpServer* server);

    static QHttpServerResponse login(const QHttpServerRequest &request);
    static QHttpServerResponse registerUser(const QHttpServerRequest &request);
    static QHttpServerResponse logout(const QHttpServerRequest &request);
    static QHttpServerResponse getCurrentUser(const QHttpServerRequest &request);

};

#endif // AUTH_ROUTES_H
