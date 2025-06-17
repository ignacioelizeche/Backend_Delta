#include "visualizations_routes.h"

void VisualizationsRoutes::setupRoutes(QHttpServer* server) {
    server->route("/visualizations", QHttpServerRequest::Method::Options, [](const QHttpServerRequest &req) {
        Q_UNUSED(req)
        return createCorsResponse("", QHttpServerResponse::StatusCode::Ok);
    });

    server->route("/visualizations/<arg>", QHttpServerRequest::Method::Options, [](const QHttpServerRequest &req) {
        Q_UNUSED(req)
        return createCorsResponse("", QHttpServerResponse::StatusCode::Ok);
    });
}

QHttpServerResponse VisualizationsRoutes::getVisualizations(const QHttpServerRequest &request) {
    // Implementar lógica para obtener lista de visualizaciones
    QJsonObject response;
    response["message"] = "Get visualizations endpoint";
    return QHttpServerResponse(QJsonDocument(response).toJson(),
                               QHttpServerResponse::StatusCode::Ok);
}

QHttpServerResponse VisualizationsRoutes::getVisualization(const QHttpServerRequest &request,
                                                           const QString &id) {
    // Implementar lógica para obtener visualización específica
    QJsonObject response;
    response["message"] = "Get visualization endpoint";
    response["id"] = id;
    return QHttpServerResponse(QJsonDocument(response).toJson(),
                               QHttpServerResponse::StatusCode::Ok);
}
