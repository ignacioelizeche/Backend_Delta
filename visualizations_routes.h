#ifndef VISUALIZATIONS_ROUTES_H
#define VISUALIZATIONS_ROUTES_H

#include <QHttpServer>
#include <QHttpServerRequest>
#include <QHttpServerResponse>
#include <QJsonDocument>
#include <QJsonObject>
#include "response_utils.h"

class VisualizationsRoutes
{
public:
    static void setupRoutes(QHttpServer *server);

private:
    // GET /visualizations
    static QHttpServerResponse getVisualizations(const QHttpServerRequest &request);

    // GET /visualizations/id
    static QHttpServerResponse getVisualization(const QHttpServerRequest &request,
                                                const QString &id);
};

#endif // VISUALIZATIONS_ROUTES_H
