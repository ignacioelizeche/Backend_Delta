#include "problems_routes.h"

void ProblemsRoutes::setupRoutes(QHttpServer* server) {
    server->route("/problems", QHttpServerRequest::Method::Options, [](const QHttpServerRequest &req) {
        Q_UNUSED(req)
        return createCorsResponse("", QHttpServerResponse::StatusCode::Ok);
    });

    server->route("/problems/<arg>", QHttpServerRequest::Method::Options, [](const QHttpServerRequest &req) {
        Q_UNUSED(req)
        return createCorsResponse("", QHttpServerResponse::StatusCode::Ok);
    });

    server->route("/problems/<arg>/submit", QHttpServerRequest::Method::Options, [](const QHttpServerRequest &req) {
        Q_UNUSED(req)
        return createCorsResponse("", QHttpServerResponse::StatusCode::Ok);
    });

    server->route("/problems/<arg>/attempts", QHttpServerRequest::Method::Options, [](const QHttpServerRequest &req) {
        Q_UNUSED(req)
        return createCorsResponse("", QHttpServerResponse::StatusCode::Ok);
    });

    server->route("/problems/recommendations/<arg>", QHttpServerRequest::Method::Options, [](const QHttpServerRequest &req) {
        Q_UNUSED(req)
        return createCorsResponse("", QHttpServerResponse::StatusCode::Ok);
    });
}


// Ahora las funciones devuelven directamente QHttpServerResponse con CORS y JSON

QHttpServerResponse ProblemsRoutes::getProblems(const QHttpServerRequest &request) {
    QJsonObject response;
    response["message"] = "Get problems endpoint";
    return createCorsResponse(response, QHttpServerResponse::StatusCode::Ok);
}

QHttpServerResponse ProblemsRoutes::getProblem(const QHttpServerRequest &request, const QString &id) {
    QJsonObject response;
    response["message"] = "Get problem endpoint";
    response["id"] = id;
    return createCorsResponse(response, QHttpServerResponse::StatusCode::Ok);
}

QHttpServerResponse ProblemsRoutes::submitProblem(const QHttpServerRequest &request, const QString &id) {
    QJsonObject response;
    response["message"] = "Submit problem endpoint";
    response["id"] = id;
    return createCorsResponse(response, QHttpServerResponse::StatusCode::Ok);
}

QHttpServerResponse ProblemsRoutes::getProblemAttempts(const QHttpServerRequest &request, const QString &id) {
    QJsonObject response;
    response["message"] = "Get problem attempts endpoint";
    response["id"] = id;
    return createCorsResponse(response, QHttpServerResponse::StatusCode::Ok);
}

QHttpServerResponse ProblemsRoutes::getRecommendations(const QHttpServerRequest &request, const QString &userId) {
    QJsonObject response;
    response["message"] = "Get problem recommendations endpoint";
    response["userId"] = userId;
    return createCorsResponse(response, QHttpServerResponse::StatusCode::Ok);
}
