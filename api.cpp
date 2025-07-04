#include "api.h"
#include "auth_routes.h"
#include "calendar_routes.h"
#include "dashboard_routes.h"
#include "exams_routes.h"
#include "forum_routes.h"
#include "notebooks_routes.h"
#include "problems_routes.h"
#include "ranking_routes.h"
#include "response_utils.h"
#include "visualizations_routes.h"

#include <QDebug>
#include <QHostAddress>
#include <QHttpServerResponse>

// Constructor con la configuración y arranque del servidor
API::API(QObject *parent)
    : QObject(parent)
{
    m_httpServer = new QHttpServer(this);
    m_tcpServer = new QTcpServer(this);

    // Ruta OPTIONS para CORS general
    m_httpServer
        ->route("/<QString>", QHttpServerRequest::Method::Options, [](const QHttpServerRequest &) {
            QHttpServerResponse response(QHttpServerResponse::StatusCode::Ok);
            QHttpHeaders headers;
            headers.append("Access-Control-Allow-Origin", "*");
            headers.append("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
            headers.append("Access-Control-Allow-Headers", "Content-Type, Authorization");

            response.setHeaders(headers);
            return response;
        });

    // Ruta /health
    m_httpServer->route("/health", QHttpServerRequest::Method::Get, []() {
        QJsonObject json;
        json["status"] = "OK";
        json["message"] = "Server is running";

        // Create the CORS response and send it using the responder
        return createCorsResponse(json);
    });

    // Configurar rutas de módulos
    AuthRoutes::setupRoutes(m_httpServer);
    DashboardRoutes::setupRoutes(m_httpServer);
    CalendarRoutes::setupRoutes(m_httpServer);
    ProblemsRoutes::setupRoutes(m_httpServer);
    ForumRoutes::setupRoutes(m_httpServer);
    VisualizationsRoutes::setupRoutes(m_httpServer);
    ExamsRoutes::setupRoutes(m_httpServer);
    RankingRoutes::setupRoutes(m_httpServer);
    NotebooksRoutes::setupRoutes(m_httpServer);

    // Iniciar TCP server en puerto 8080
    if (!m_tcpServer->listen(QHostAddress::Any, 8080)) {
        qDebug() << "Failed to start server on port 8080, trying random port...";
        if (!m_tcpServer->listen()) {
            qDebug() << "Failed to start TCP server completely";
            return;
        }
    }

    // Bind HTTP server a TCP server
    if (!m_httpServer->bind(m_tcpServer)) {
        qDebug() << "Failed to bind HTTP server to TCP server";
        return;
    }

    qDebug() << "Server listening on port" << m_tcpServer->serverPort();
}
