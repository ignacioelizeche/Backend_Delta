#ifndef DASHBOARD_ROUTES_H
#define DASHBOARD_ROUTES_H

#include <QHttpServer>
#include <QHttpServerRequest>
#include <QHttpServerResponse>
#include <QJsonDocument>
#include <QJsonObject>
#include "response_utils.h"

class DashboardRoutes
{
public:
    static void setupRoutes(QHttpServer *server);

private:
    // GET /dashboard/stats
    static QHttpServerResponse getStats(const QHttpServerRequest &request);
};

#endif // DASHBOARD_ROUTES_H
