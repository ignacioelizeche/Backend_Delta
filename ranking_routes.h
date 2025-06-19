#ifndef RANKING_ROUTES_H
#define RANKING_ROUTES_H

#include <QHttpServer>
#include <QHttpServerRequest>
#include <QHttpServerResponse>
#include <QUrlQuery>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QDebug>

class RankingRoutes {
public:
    // Main setup method
    static void setupRoutes(QHttpServer* server);

    // Route handlers
    static QHttpServerResponse getLeaderboard(const QHttpServerRequest &request);

private:
    // Helper methods
    static QString buildLeaderboardQuery(const QString& timeframe);
    static QDateTime getTimeframeCutoff(const QString& timeframe);
    static QJsonObject buildUserEntry(const QSqlQuery& query, int currentUserId, int rank);
    static int getCurrentUserRank(int currentUserId, const QString& timeframe,
                                  bool foundInResults, const QJsonObject& currentUserEntry);
    static QJsonObject getLeaderboardStats(const QString& timeframe);

    // Private constructor for utility class
    RankingRoutes() = default;
};

#endif // RANKING_ROUTES_H
