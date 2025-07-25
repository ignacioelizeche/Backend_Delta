#ifndef PROBLEMS_ROUTES_H
#define PROBLEMS_ROUTES_H
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

class ProblemsRoutes
{
public:
    static void setupRoutes(QHttpServer *server);

private:
    // GET /problems/difficulty/{difficulty}/{topic}
    static QHttpServerResponse getProblems(const QString &difficulty,
                                           const QString &topic,
                                           const QHttpServerRequest &request);
    // GET /problems/{id}
    static QHttpServerResponse getProblem(const QHttpServerRequest &request, const QString &id);
    // POST /problems/{id}/submit
    static QHttpServerResponse submitProblem(const QHttpServerRequest &request, const QString &id);
    // GET /problems/{id}/attempts
    static QHttpServerResponse getProblemAttempts(const QHttpServerRequest &request,
                                                  const QString &id);
    // GET /problems/recommendations/{userId}
    static QHttpServerResponse getRecommendations(const QHttpServerRequest &request,
                                                  const QString &userId);
    static QHttpServerResponse createProblem(const QHttpServerRequest &request);
    static QHttpServerResponse getAllProblems(const QHttpServerRequest &request);
};
#endif // PROBLEMS_ROUTES_H
