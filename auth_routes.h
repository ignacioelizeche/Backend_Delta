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
#include <QRegularExpression>
#include <QString>
#include <QCryptographicHash>
#include <QRandomGenerator>

class AuthRoutes {
public:
    static void setupRoutes(QHttpServer* server);
    static QHttpServerResponse login(const QHttpServerRequest &request);
    static QHttpServerResponse registerUser(const QHttpServerRequest &request);
    static QHttpServerResponse logout(const QHttpServerRequest &request);
    static QHttpServerResponse getCurrentUser(const QHttpServerRequest &request);

private:
    // Función para validar formato de email
    static bool isValidEmail(const QString& email);

    // Funciones para manejo de contraseñas
    static QString generateSalt();
    static QString hashPassword(const QString& password, const QString& salt);
    static bool verifyPassword(const QString& password, const QString& hashedPassword, const QString& salt);
};

// Implementación inline de las funciones de seguridad
inline bool AuthRoutes::isValidEmail(const QString& email) {
    // Patrón regex para validar emails
    QRegularExpression emailRegex(
        R"(^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$)"
        );

    return emailRegex.match(email).hasMatch();
}

inline QString AuthRoutes::generateSalt() {
    // Generar un salt aleatorio de 16 bytes
    QByteArray saltBytes;
    for (int i = 0; i < 16; ++i) {
        saltBytes.append(static_cast<char>(QRandomGenerator::global()->bounded(256)));
    }
    return saltBytes.toHex();
}

inline QString AuthRoutes::hashPassword(const QString& password, const QString& salt) {
    // Combinar password + salt y hacer hash SHA-256
    QString combined = password + salt;
    QByteArray hash = QCryptographicHash::hash(combined.toUtf8(), QCryptographicHash::Sha256);
    return hash.toHex();
}

inline bool AuthRoutes::verifyPassword(const QString& password, const QString& hashedPassword, const QString& salt) {
    // Verificar si la contraseña coincide
    QString testHash = hashPassword(password, salt);
    return testHash == hashedPassword;
}

#endif // AUTH_ROUTES_H
