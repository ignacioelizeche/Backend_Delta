#ifndef RANKING_ROUTES_H
#define RANKING_ROUTES_H

#include <QHttpServer>
#include <QHttpServerRequest>
#include <QHttpServerResponse>

class RankingRoutes
{
public:
    static void setupRoutes(QHttpServer *server);
    static QHttpServerResponse getLeaderboard(const QHttpServerRequest &req);
};

#endif // RANKING_ROUTES_H
