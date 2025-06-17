#ifndef PROBLEMS_ROUTES_H
#define PROBLEMS_ROUTES_H

#include <QHttpServer>
#include <QHttpServerRequest>
#include <QHttpServerResponse>
#include <QJsonDocument>
#include <QJsonObject>
#include "response_utils.h"

class ProblemsRoutes {
public:
    static void setupRoutes(QHttpServer* server);

private:
    // GET /problems
    static QHttpServerResponse getProblems(const QHttpServerRequest &request);

    // GET /problems/id
    static QHttpServerResponse getProblem(const QHttpServerRequest &request,
                                          const QString &id);

    // POST /problems/id/submit
    static QHttpServerResponse submitProblem(const QHttpServerRequest &request,
                                             const QString &id);

    // GET /problems/id/attempts
    static QHttpServerResponse getProblemAttempts(const QHttpServerRequest &request,
                                                  const QString &id);

    // GET /problems/recommendations/userId
    static QHttpServerResponse getRecommendations(const QHttpServerRequest &request,
                                                  const QString &userId);
};

#endif // PROBLEMS_ROUTES_H
