#include "ranking_routes.h"
#include "response_utils.h"
#include "jwt_helper.h"
#include <QDateTime>
#include <QJsonDocument>
#include <QUrlQuery>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

void RankingRoutes::setupRoutes(QHttpServer* server) {
    // CORS preflight OPTIONS
    server->route("/leaderboard", QHttpServerRequest::Method::Options, [](const QHttpServerRequest &req) {
        Q_UNUSED(req)
        return createCorsResponse("", QHttpServerResponse::StatusCode::Ok);
    });

    // GET /leaderboard with enhanced timeframe support
    server->route("/leaderboard", QHttpServerRequest::Method::Get, [](const QHttpServerRequest &req) {
        return RankingRoutes::getLeaderboard(req);
    });
}

QHttpServerResponse RankingRoutes::getLeaderboard(const QHttpServerRequest &request) {
    // Parse query parameters
    QUrlQuery query(request.url().query());
    int limit = query.queryItemValue("limit").toInt();
    QString timeframe = query.queryItemValue("timeframe");

    // Set defaults
    if (limit <= 0) limit = 50;
    if (timeframe.isEmpty()) timeframe = "all-time";

    // Get current user info from Authorization header
    QString authHeader = request.value("Authorization");
    if (authHeader.isEmpty()) {
        authHeader = request.value("authorization");
    }

    int currentUserId = -1;
    if (!authHeader.isEmpty()) {
        QString token;
        if (authHeader.startsWith("Bearer ")) {
            token = authHeader.mid(7);
        }
        if (!token.isEmpty()) {
            QJsonObject userInfo = jwt_helper::validateJWT(token);
            currentUserId = userInfo.value("userId").toInt();
        }
    }

    QSqlDatabase db = QSqlDatabase::database();

    // Build the appropriate query based on timeframe
    QString leaderboardQuery = buildLeaderboardQuery(timeframe);

    QSqlQuery q(db);
    q.prepare(leaderboardQuery);
    q.addBindValue(limit);

    if (!q.exec()) {
        qDebug() << "Leaderboard query error:" << q.lastError().text();
        return createCorsResponse("Failed to fetch leaderboard", QHttpServerResponse::StatusCode::InternalServerError);
    }

    QJsonArray usersArray;
    QJsonObject currentUserEntry;
    bool foundCurrentUser = false;
    int rank = 1;

    while (q.next()) {
        QJsonObject user = buildUserEntry(q, currentUserId, rank);
        usersArray.append(user);

        if (user["id"].toInt() == currentUserId) {
            currentUserEntry = user;
            foundCurrentUser = true;
        }
        rank++;
    }

    // Get current user's rank if not in top results
    int currentUserRank = getCurrentUserRank(currentUserId, timeframe, foundCurrentUser, currentUserEntry);

    // Get leaderboard stats
    QJsonObject stats = getLeaderboardStats(timeframe);

    // Build final response
    QJsonObject response;
    response["users"] = usersArray;
    response["stats"] = stats;
    response["currentUserRank"] = currentUserRank;

    return createCorsResponse(response, QHttpServerResponse::StatusCode::Ok);
}

QString RankingRoutes::buildLeaderboardQuery(const QString& timeframe) {
    // Base query using actual database schema
    QString baseSelect = R"(
        SELECT
            u.id, u.username, u.coinBalance as coins, u.level,
            u.xpPoints as totalPoints, u.streak, u.joinDate, u.lastLoginDate as lastActive,
            u.totalProblemsCompleted as problemsSolved,
            GROUP_CONCAT(DISTINCT a.name) as achievements
        FROM users u
        LEFT JOIN user_achievements ua ON u.id = ua.user_id
        LEFT JOIN achievements a ON ua.achievement_id = a.id
    )";

    QString groupBy = " GROUP BY u.id ";
    QString orderBy;

    // Since we don't have weekly/monthly points in the schema,
    // we'll use totalPoints for all timeframes for now
    if (timeframe == "weekly") {
        // You might want to calculate weekly points from user_activity_log
        orderBy = " ORDER BY u.xpPoints DESC, u.totalProblemsCompleted DESC ";
    } else if (timeframe == "monthly") {
        // You might want to calculate monthly points from user_activity_log
        orderBy = " ORDER BY u.xpPoints DESC, u.totalProblemsCompleted DESC ";
    } else {
        orderBy = " ORDER BY u.xpPoints DESC, u.totalProblemsCompleted DESC ";
    }

    return baseSelect + groupBy + orderBy + " LIMIT ? ";
}

QDateTime RankingRoutes::getTimeframeCutoff(const QString& timeframe) {
    QDateTime now = QDateTime::currentDateTime();

    if (timeframe == "weekly") {
        return now.addDays(-7);
    } else if (timeframe == "monthly") {
        return now.addDays(-30);
    }

    return QDateTime(); // For all-time, no cutoff needed
}

QJsonObject RankingRoutes::buildUserEntry(const QSqlQuery& query, int currentUserId, int rank) {
    QJsonObject user;
    int userId = query.value("id").toInt();

    user["rank"] = rank;
    user["id"] = userId;
    user["username"] = query.value("username").toString();
    user["coins"] = query.value("coins").toInt();
    user["level"] = query.value("level").toInt();

    // Set default badge based on level or points
    QString badge = "Bronze";
    QString badgeColor = "#cd7f32";
    int totalPoints = query.value("totalPoints").toInt();

    if (totalPoints >= 3000) {
        badge = "Gold";
        badgeColor = "#ffd700";
    } else if (totalPoints >= 1500) {
        badge = "Silver";
        badgeColor = "#c0c0c0";
    }

    user["badge"] = badge;
    user["badgeColor"] = badgeColor;
    user["totalPoints"] = totalPoints;

    // For now, set weekly/monthly points same as total points
    // You should implement proper calculation later
    user["weeklyPoints"] = totalPoints;
    user["monthlyPoints"] = totalPoints;

    user["problemsSolved"] = query.value("problemsSolved").toInt();
    user["forumContributions"] = 0; // Not in current schema
    user["streak"] = query.value("streak").toInt();
    user["joinDate"] = query.value("joinDate").toDateTime().toString(Qt::ISODate);
    user["lastActive"] = query.value("lastActive").toDateTime().toString(Qt::ISODate);
    user["isCurrentUser"] = (userId == currentUserId);

    // Parse achievements
    QString achievementsString = query.value("achievements").toString();
    QJsonArray achievements;
    if (!achievementsString.isEmpty()) {
        QStringList achievementsList = achievementsString.split(",");
        for (const QString& achievement : achievementsList) {
            achievements.append(achievement.trimmed());
        }
    }
    user["achievements"] = achievements;

    return user;
}

int RankingRoutes::getCurrentUserRank(int currentUserId, const QString& timeframe,
                                      bool foundInResults, const QJsonObject& currentUserEntry) {
    if (currentUserId == -1) return -1;

    if (foundInResults) {
        return currentUserEntry["rank"].toInt();
    }

    // Query for user's specific rank using actual schema
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery rankQuery(db);

    QString rankQueryStr = R"(
        SELECT COUNT(*) + 1 as rank FROM users
        WHERE xpPoints > (SELECT xpPoints FROM users WHERE id = ?)
    )";

    rankQuery.prepare(rankQueryStr);
    rankQuery.addBindValue(currentUserId);

    if (rankQuery.exec() && rankQuery.next()) {
        return rankQuery.value("rank").toInt();
    }

    return -1;
}

QJsonObject RankingRoutes::getLeaderboardStats(const QString& timeframe) {
    QSqlDatabase db = QSqlDatabase::database();
    QJsonObject stats;

    // Get total users
    QSqlQuery totalQuery(db);
    totalQuery.prepare("SELECT COUNT(*) as total FROM users");
    if (totalQuery.exec() && totalQuery.next()) {
        stats["totalUsers"] = totalQuery.value("total").toInt();
    }

    // Get average points
    QSqlQuery avgQuery(db);
    avgQuery.prepare("SELECT AVG(xpPoints) as avg FROM users WHERE xpPoints > 0");
    if (avgQuery.exec() && avgQuery.next()) {
        stats["averagePoints"] = avgQuery.value("avg").toInt();
    }

    // Get top performer
    QSqlQuery topQuery(db);
    topQuery.prepare("SELECT username, xpPoints as points FROM users ORDER BY xpPoints DESC LIMIT 1");
    if (topQuery.exec() && topQuery.next()) {
        QJsonObject topPerformer;
        topPerformer["username"] = topQuery.value("username").toString();
        topPerformer["points"] = topQuery.value("points").toInt();
        stats["topPerformer"] = topPerformer;
    }

    // Calculate weekly growth (mock calculation for now)
    stats["weeklyGrowth"] = 12.5;

    return stats;
}
