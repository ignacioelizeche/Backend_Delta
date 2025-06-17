#ifndef NOTEBOOKS_ROUTES_H
#define NOTEBOOKS_ROUTES_H

#include <QHttpServer>
#include <QHttpServerRequest>
#include <QHttpServerResponse>
#include <QJsonDocument>
#include <QJsonObject>
#include "response_utils.h"

class NotebooksRoutes {
public:
    static void setupRoutes(QHttpServer* server);

private:
    // GET /notebooks
    static QHttpServerResponse getNotebooks(const QHttpServerRequest &request);

    // POST /notebooks
    static QHttpServerResponse createNotebook(const QHttpServerRequest &request);

    // GET /resources
    static QHttpServerResponse getResources(const QHttpServerRequest &request);

    // POST /resources
    static QHttpServerResponse createResource(const QHttpServerRequest &request);
};

#endif // NOTEBOOKS_ROUTES_H
