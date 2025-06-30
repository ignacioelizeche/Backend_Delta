#include "ranking_routes.h"
#include "response_utils.h"
#include "jwt_helper.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QDebug>
#include <QUrlQuery>

void RankingRoutes::setupRoutes(QHttpServer* server) {
    // CORS preflight OPTIONS
    server->route("/leaderboard", QHttpServerRequest::Method::Options, [](const QHttpServerRequest &req) {
        Q_UNUSED(req)
        return createCorsResponse("", QHttpServerResponse::StatusCode::Ok);
    });

    // GET /leaderboard
    server->route("/leaderboard", QHttpServerRequest::Method::Get, [](const QHttpServerRequest &req) {
        return RankingRoutes::getLeaderboard(req);
    });
}

QHttpServerResponse RankingRoutes::getLeaderboard(const QHttpServerRequest &req) {
    // Extract and validate JWT token
    QString authHeader = req.value("Authorization");
    if (authHeader.isEmpty()) {
        authHeader = req.value("authorization");
    }

    QString token;
    if (authHeader.startsWith("Bearer ")) {
        token = authHeader.mid(7);
    } else {
        token = req.value("token");
    }

    int currentUserId = 0;
    if (!token.isEmpty()) {
        QJsonObject authorize = jwt_helper::validateJWT(token);
        currentUserId = authorize.value("userId").toInt();
    }

    // Parse query parameters
    QUrl requestUrl = req.url();
    QUrlQuery query(requestUrl);

    QString timeframe = query.queryItemValue("timeframe", QUrl::FullyDecoded);
    if (timeframe.isEmpty()) {
        timeframe = "all"; // Default timeframe
    }

    int limit = query.queryItemValue("limit", QUrl::FullyDecoded).toInt();
    if (limit <= 0 || limit > 100) {
        limit = 50; // Default limit
    }

    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery q(db);

    // Calculate points based on timeframe
    QString whereClause = "";
    QString pointsCalculation = "xpPoints";

    if (timeframe == "weekly") {
        whereClause = "WHERE lastLoginDate >= datetime('now', '-7 days')";
        pointsCalculation = "(SELECT COALESCE(SUM(xp_earned), 0) FROM user_activity_log WHERE user_id = u.id AND created_at >= datetime('now', '-7 days'))";
    } else if (timeframe == "monthly") {
        whereClause = "WHERE lastLoginDate >= datetime('now', '-30 days')";
        pointsCalculation = "(SELECT COALESCE(SUM(xp_earned), 0) FROM user_activity_log WHERE user_id = u.id AND created_at >= datetime('now', '-30 days'))";
    }

    // Main leaderboard query
    QString queryStr = QString(
                           "SELECT u.id, u.username, u.coinBalance, u.level, u.xpPoints, u.totalProblemsCompleted, "
                           "u.streak, u.joinDate, u.lastLoginDate, "
                           "%1 as calculatedPoints, "
                           "(SELECT COUNT(*) FROM user_activity_log WHERE user_id = u.id AND activity_type = 'forum_post') as forumContributions "
                           "FROM users u "
                           "%2 "
                           "ORDER BY calculatedPoints DESC, u.totalProblemsCompleted DESC "
                           "LIMIT ?"
                           ).arg(pointsCalculation, whereClause);

    q.prepare(queryStr);
    q.addBindValue(limit);

    if (!q.exec()) {
        qDebug() << "Leaderboard query error:" << q.lastError().text();
        return createCorsResponse("Database error", QHttpServerResponse::StatusCode::InternalServerError);
    }

    QJsonArray usersArray;
    int rank = 1;
    int currentUserRank = 0;

    while (q.next()) {
        int userId = q.value("id").toInt();
        QString username = q.value("username").toString();
        int coins = q.value("coinBalance").toInt();
        int level = q.value("level").toInt();
        int xpPoints = q.value("xpPoints").toInt();
        int problemsSolved = q.value("totalProblemsCompleted").toInt();
        int streak = q.value("streak").toInt();
        QDateTime joinDate = q.value("joinDate").toDateTime();
        QDateTime lastActive = q.value("lastLoginDate").toDateTime();
        int calculatedPoints = q.value("calculatedPoints").toInt();
        int forumContributions = q.value("forumContributions").toInt();

        // Determine badge and color based on level/points
        QString badge;
        QString badgeColor;
        QJsonArray achievements;

        if (level >= 10) {
            badge = "Gold";
            badgeColor = "#ffd700";
            achievements.append("Elite Player");
        } else if (level >= 7) {
            badge = "Silver";
            badgeColor = "#c0c0c0";
            achievements.append("Advanced Player");
        } else if (level >= 4) {
            badge = "Bronze";
            badgeColor = "#cd7f32";
            achievements.append("Rising Star");
        } else {
            badge = "Beginner";
            badgeColor = "#8b8b8b";
        }

        // Add streak achievements
        if (streak >= 30) {
            achievements.append("Month Warrior");
        } else if (streak >= 7) {
            achievements.append("Week Warrior");
        }

        // Add problem solving achievements
        if (problemsSolved >= 100) {
            achievements.append("Problem Master");
        } else if (problemsSolved >= 50) {
            achievements.append("Problem Solver");
        }

        // Add math-specific achievement (example)
        if (problemsSolved >= 25) {
            achievements.append("Math Master");
        }

        QJsonObject userObj;
        userObj["rank"] = rank;
        userObj["id"] = userId;
        userObj["username"] = username;
        userObj["coins"] = coins;
        userObj["level"] = level;
        userObj["badge"] = badge;
        userObj["badgeColor"] = badgeColor;
        userObj["totalPoints"] = xpPoints;
        userObj["weeklyPoints"] = (timeframe == "weekly") ? calculatedPoints : 0;
        userObj["monthlyPoints"] = (timeframe == "monthly") ? calculatedPoints : 0;
        userObj["problemsSolved"] = problemsSolved;
        userObj["forumContributions"] = forumContributions;
        userObj["streak"] = streak;
        userObj["achievements"] = achievements;
        userObj["joinDate"] = joinDate.toString(Qt::ISODate);
        userObj["lastActive"] = lastActive.toString(Qt::ISODate);
        userObj["isCurrentUser"] = (userId == currentUserId);

        if (userId == currentUserId) {
            currentUserRank = rank;
        }

        usersArray.append(userObj);
        rank++;
    }

    // Get statistics
    QSqlQuery statsQuery(db);

    // Total users count
    statsQuery.exec("SELECT COUNT(*) as total FROM users");
    int totalUsers = 0;
    if (statsQuery.next()) {
        totalUsers = statsQuery.value("total").toInt();
    }

    // Average points
    statsQuery.exec("SELECT AVG(xpPoints) as avgPoints FROM users");
    double averagePoints = 0.0;
    if (statsQuery.next()) {
        averagePoints = statsQuery.value("avgPoints").toDouble();
    }

    // Top performer
    QString topPerformerUsername = "";
    int topPerformerPoints = 0;
    if (!usersArray.isEmpty()) {
        QJsonObject topUser = usersArray[0].toObject();
        topPerformerUsername = topUser["username"].toString();
        topPerformerPoints = topUser["totalPoints"].toInt();
    }

    // Weekly growth calculation (simplified)
    statsQuery.exec("SELECT COUNT(*) as newUsers FROM users WHERE joinDate >= datetime('now', '-7 days')");
    int newUsersThisWeek = 0;
    if (statsQuery.next()) {
        newUsersThisWeek = statsQuery.value("newUsers").toInt();
    }
    double weeklyGrowth = totalUsers > 0 ? (double(newUsersThisWeek) / totalUsers) * 100.0 : 0.0;

    // If current user is not in top results, get their rank
    if (currentUserRank == 0 && currentUserId > 0) {
        QString rankQuery = QString(
                                "SELECT COUNT(*) + 1 as userRank FROM users u2 "
                                "WHERE (%1) > (SELECT %1 FROM users WHERE id = ?)"
                                ).arg(pointsCalculation);

        QSqlQuery userRankQuery(db);
        userRankQuery.prepare(rankQuery);
        userRankQuery.addBindValue(currentUserId);

        if (userRankQuery.exec() && userRankQuery.next()) {
            currentUserRank = userRankQuery.value("userRank").toInt();
        }
    }

    // Build response
    QJsonObject response;
    response["users"] = usersArray;

    QJsonObject stats;
    stats["totalUsers"] = totalUsers;
    stats["averagePoints"] = qRound(averagePoints);

    QJsonObject topPerformer;
    topPerformer["username"] = topPerformerUsername;
    topPerformer["points"] = topPerformerPoints;
    stats["topPerformer"] = topPerformer;
    stats["weeklyGrowth"] = qRound(weeklyGrowth * 10) / 10.0; // Round to 1 decimal place

    response["stats"] = stats;
    response["currentUserRank"] = currentUserRank;

    return createCorsResponse(response, QHttpServerResponse::StatusCode::Ok);
}
