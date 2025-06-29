#include "response_utils.h"
#include <QHttpHeaders>

// Definición de la función para QString
QHttpServerResponse createCorsResponse(const QString &content, QHttpServerResponse::StatusCode status) {
    QHttpServerResponse response(content, status);

    QHttpHeaders headers;
    headers.append("Access-Control-Allow-Origin", "*");
    headers.append("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    headers.append("Access-Control-Allow-Headers", "Content-Type, Authorization");
    headers.append("Access-Control-Max-Age", "86400");

    response.setHeaders(headers);
    return response;
}

// Definición de la función para QJsonObject
QHttpServerResponse createCorsResponse(const QJsonObject &content, QHttpServerResponse::StatusCode status) {
    QHttpServerResponse response(content, status);

    QHttpHeaders headers;
    headers.append("Access-Control-Allow-Origin", "*");
    headers.append("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    headers.append("Access-Control-Allow-Headers", "Content-Type, Authorization");
    headers.append("Access-Control-Max-Age", "86400");

    response.setHeaders(headers);
    return response;
}

// Definición de la función para QJsonArray
QHttpServerResponse createCorsResponse(const QJsonArray &content,
                                       QHttpServerResponse::StatusCode status)
{
    QHttpServerResponse response(content, status);

    QHttpHeaders headers;
    headers.append("Access-Control-Allow-Origin", "*");
    headers.append("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    headers.append("Access-Control-Allow-Headers", "Content-Type, Authorization");
    headers.append("Access-Control-Max-Age", "86400");

    response.setHeaders(headers);
    return response;
}
