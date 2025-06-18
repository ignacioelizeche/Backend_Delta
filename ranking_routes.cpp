#include "ranking_routes.h"
//hello world
void RankingRoutes::setupRoutes(QHttpServer* server) {
    server->route("/leaderboard", QHttpServerRequest::Method::Options, [](const QHttpServerRequest &req) {
        Q_UNUSED(req)
        return createCorsResponse("", QHttpServerResponse::StatusCode::Ok);
    });
}


QHttpServerResponse RankingRoutes::getLeaderboard(const QHttpServerRequest &request) {
    // Implementar lógica para obtener tabla de clasificación
    QJsonObject response;
    response["message"] = "Get leaderboard endpoint";
    return QHttpServerResponse(QJsonDocument(response).toJson(),
                               QHttpServerResponse::StatusCode::Ok);
}
