#ifndef RANKING_ROUTES_H
#define RANKING_ROUTES_H

#include <QHttpServer>
#include <QHttpServerRequest>
#include <QHttpServerResponse>
#include <QJsonDocument>
#include <QJsonObject>
#include "response_utils.h"

class RankingRoutes {
public:
    static void setupRoutes(QHttpServer* server);

private:
    // GET /leaderboard
    static QHttpServerResponse getLeaderboard(const QHttpServerRequest &request);
};

#endif // RANKING_ROUTES_H
