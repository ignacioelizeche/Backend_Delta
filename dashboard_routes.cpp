#include "dashboard_routes.h"

void DashboardRoutes::setupRoutes(QHttpServer *server)
{
    server->route("/dashboard/stats",
                  QHttpServerRequest::Method::Options,
                  [](const QHttpServerRequest &req) {
                      Q_UNUSED(req)
                      return createCorsResponse("", QHttpServerResponse::StatusCode::Ok);
                  });
}

// Implementación original conservada
QHttpServerResponse DashboardRoutes::getStats(const QHttpServerRequest &request)
{
    QJsonObject response;
    response["message"] = "Dashboard stats endpoint";
    return QHttpServerResponse(QJsonDocument(response).toJson(),
                               QHttpServerResponse::StatusCode::Ok);
}
