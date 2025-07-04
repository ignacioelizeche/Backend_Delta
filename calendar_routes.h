#ifndef CALENDAR_ROUTES_H
#define CALENDAR_ROUTES_H

#include <QDateTime>
#include <QDebug>
#include <QHttpServer>
#include <QHttpServerRequest>
#include <QHttpServerResponse>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlError>
#include <QSqlQuery>
#include "database_manager.h" // Si usas singleton, sino elimina
#include "jwt_helper.h"       // Para JWTHelper::generateJWT y validateJWT
#include "response_utils.h"

class CalendarRoutes
{
public:
    static void setupRoutes(QHttpServer *server);

private:
    // GET /calendar/events/start/end
    static QHttpServerResponse getEvents(const QHttpServerRequest &request,
                                         const QString &start,
                                         const QString &end);

    // POST /calendar/events
    static QHttpServerResponse createEvent(const QHttpServerRequest &request);

    // PUT /calendar/events/id
    static QHttpServerResponse updateEvent(const QHttpServerRequest &request, const QString &id);

    // DELETE /calendar/events/id
    static QHttpServerResponse deleteEvent(const QHttpServerRequest &request, const QString &id);
};

#endif // CALENDAR_ROUTES_H
