#ifndef RESPONSE_UTILS_H
#define RESPONSE_UTILS_H

#include <QString>
#include <QHttpServerResponse>
#include <QJsonObject>

// Solo prototipos
QHttpServerResponse createCorsResponse(const QString &content, QHttpServerResponse::StatusCode status = QHttpServerResponse::StatusCode::Ok);
QHttpServerResponse createCorsResponse(const QJsonObject &content, QHttpServerResponse::StatusCode status = QHttpServerResponse::StatusCode::Ok);

#endif // RESPONSE_UTILS_H
