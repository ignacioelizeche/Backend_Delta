#ifndef LEADERBOARD_ROUTES_H
#define LEADERBOARD_ROUTES_H

#include <QHttpServer>
#include <QHttpServerRequest>
#include <QHttpServerResponse>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QDebug>
#include "response_utils.h"
#include "jwt_helper.h"

class LeaderboardRoutes
{
public:
    static void setupRoutes(QHttpServer *server);

    // Main leaderboard endpoints
    static QHttpServerResponse getLeaderboard(const QHttpServerRequest &request);
    static QHttpServerResponse getWeeklyLeaderboard(const QHttpServerRequest &request);
    static QHttpServerResponse getMonthlyLeaderboard(const QHttpServerRequest &request);
    static QHttpServerResponse getUserRanking(const QHttpServerRequest &request, const QString &userId);

    // Utility functions
    static QJsonArray buildLeaderboardData(const QString &timeframe);
    static QJsonArray getUserSolvedProblems(int userId);
    static void updateLeaderboardHistory(const QString &timeframe);
};

#endif // LEADERBOARD_ROUTES_H
