#include "notebooks_routes.h"

void NotebooksRoutes::setupRoutes(QHttpServer *server)
{
    server->route("/notebooks",
                  QHttpServerRequest::Method::Options,
                  [](const QHttpServerRequest &req) {
                      Q_UNUSED(req)
                      return createCorsResponse("", QHttpServerResponse::StatusCode::Ok);
                  });

    server->route("/resources",
                  QHttpServerRequest::Method::Options,
                  [](const QHttpServerRequest &req) {
                      Q_UNUSED(req)
                      return createCorsResponse("", QHttpServerResponse::StatusCode::Ok);
                  });
}

QHttpServerResponse NotebooksRoutes::getNotebooks(const QHttpServerRequest &request)
{
    // Implementar lógica para obtener notebooks
    QJsonObject response;
    response["message"] = "Get notebooks endpoint";
    return QHttpServerResponse(QJsonDocument(response).toJson(),
                               QHttpServerResponse::StatusCode::Ok);
}

QHttpServerResponse NotebooksRoutes::createNotebook(const QHttpServerRequest &request)
{
    // Implementar lógica para crear notebook
    QJsonObject response;
    response["message"] = "Create notebook endpoint";
    return QHttpServerResponse(QJsonDocument(response).toJson(),
                               QHttpServerResponse::StatusCode::Created);
}

QHttpServerResponse NotebooksRoutes::getResources(const QHttpServerRequest &request)
{
    // Implementar lógica para obtener recursos
    QJsonObject response;
    response["message"] = "Get resources endpoint";
    return QHttpServerResponse(QJsonDocument(response).toJson(),
                               QHttpServerResponse::StatusCode::Ok);
}

QHttpServerResponse NotebooksRoutes::createResource(const QHttpServerRequest &request)
{
    // Implementar lógica para crear recurso
    QJsonObject response;
    response["message"] = "Create resource endpoint";
    return QHttpServerResponse(QJsonDocument(response).toJson(),
                               QHttpServerResponse::StatusCode::Created);
}
