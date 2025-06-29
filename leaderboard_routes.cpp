#include "leaderboard_routes.h"
#include <QSqlRecord>  // Agregado para solucionar el error

void LeaderboardRoutes::setupRoutes(QHttpServer *server)
{
    // GET /leaderboard - General leaderboard
    server->route("/leaderboard",
                  QHttpServerRequest::Method::Options,
                  [](const QHttpServerRequest &req) {
                      Q_UNUSED(req)
                      return createCorsResponse("", QHttpServerResponse::StatusCode::Ok);
                  });

    server->route("/leaderboard",
                  QHttpServerRequest::Method::Get,
                  [](const QHttpServerRequest &req) {
                      return LeaderboardRoutes::getLeaderboard(req);
                  });

    // GET /leaderboard/weekly
    server->route("/leaderboard/weekly",
                  QHttpServerRequest::Method::Options,
                  [](const QHttpServerRequest &req) {
                      Q_UNUSED(req)
                      return createCorsResponse("", QHttpServerResponse::StatusCode::Ok);
                  });

    server->route("/leaderboard/weekly",
                  QHttpServerRequest::Method::Get,
                  [](const QHttpServerRequest &req) {
                      return LeaderboardRoutes::getWeeklyLeaderboard(req);
                  });

    // GET /leaderboard/monthly
    server->route("/leaderboard/monthly",
                  QHttpServerRequest::Method::Options,
                  [](const QHttpServerRequest &req) {
                      Q_UNUSED(req)
                      return createCorsResponse("", QHttpServerResponse::StatusCode::Ok);
                  });

    server->route("/leaderboard/monthly",
                  QHttpServerRequest::Method::Get,
                  [](const QHttpServerRequest &req) {
                      return LeaderboardRoutes::getMonthlyLeaderboard(req);
                  });

    // GET /leaderboard/user/{userId}
    server->route("/leaderboard/user/<arg>",
                  QHttpServerRequest::Method::Options,
                  [](const QString &userId, const QHttpServerRequest &req) {
                      Q_UNUSED(req)
                      return createCorsResponse("", QHttpServerResponse::StatusCode::Ok);
                  });

    server->route("/leaderboard/user/<arg>",
                  QHttpServerRequest::Method::Get,
                  [](const QString &userId, const QHttpServerRequest &req) {
                      return LeaderboardRoutes::getUserRanking(req, userId);
                  });
}

QHttpServerResponse LeaderboardRoutes::getLeaderboard(const QHttpServerRequest &request)
{
    // Authentication
    QString authHeader = request.value("Authorization");
    if (authHeader.isEmpty()) {
        authHeader = request.value("authorization");
    }
    QString token;
    if (authHeader.startsWith("Bearer ")) {
        token = authHeader.mid(7);
    } else {
        token = request.value("token");
    }
    if (token.isEmpty()) {
        return createCorsResponse("Token required", QHttpServerResponse::StatusCode::BadRequest);
    }

    QJsonObject authorize = jwt_helper::validateJWT(token);
    if (!authorize.contains("userId") || authorize.value("userId").toInt() <= 0) {
        return createCorsResponse("Invalid token", QHttpServerResponse::StatusCode::Unauthorized);
    }

    // Get query parameters for pagination
    QString limitStr = request.query().queryItemValue("limit");
    QString offsetStr = request.query().queryItemValue("offset");

    int limit = limitStr.isEmpty() ? 50 : qBound(1, limitStr.toInt(), 100);
    int offset = offsetStr.isEmpty() ? 0 : qMax(0, offsetStr.toInt());

    QJsonArray leaderboardData = buildLeaderboardData("all");

    // Apply pagination
    QJsonArray paginatedData;
    for (int i = offset; i < qMin(offset + limit, leaderboardData.size()); ++i) {
        paginatedData.append(leaderboardData[i]);
    }

    QJsonObject responseJson;
    responseJson["success"] = true;
    responseJson["leaderboard"] = paginatedData;
    responseJson["total"] = leaderboardData.size();
    responseJson["limit"] = limit;
    responseJson["offset"] = offset;
    responseJson["timeframe"] = "all_time";

    return createCorsResponse(responseJson, QHttpServerResponse::StatusCode::Ok);
}

QHttpServerResponse LeaderboardRoutes::getWeeklyLeaderboard(const QHttpServerRequest &request)
{
    // Same authentication as above...
    QString authHeader = request.value("Authorization");
    if (authHeader.isEmpty()) {
        authHeader = request.value("authorization");
    }
    QString token;
    if (authHeader.startsWith("Bearer ")) {
        token = authHeader.mid(7);
    } else {
        token = request.value("token");
    }
    if (token.isEmpty()) {
        return createCorsResponse("Token required", QHttpServerResponse::StatusCode::BadRequest);
    }

    QJsonObject authorize = jwt_helper::validateJWT(token);
    if (!authorize.contains("userId") || authorize.value("userId").toInt() <= 0) {
        return createCorsResponse("Invalid token", QHttpServerResponse::StatusCode::Unauthorized);
    }

    QJsonArray leaderboardData = buildLeaderboardData("weekly");

    QJsonObject responseJson;
    responseJson["success"] = true;
    responseJson["leaderboard"] = leaderboardData;
    responseJson["timeframe"] = "weekly";

    return createCorsResponse(responseJson, QHttpServerResponse::StatusCode::Ok);
}

QHttpServerResponse LeaderboardRoutes::getMonthlyLeaderboard(const QHttpServerRequest &request)
{
    // Same authentication as above...
    QString authHeader = request.value("Authorization");
    if (authHeader.isEmpty()) {
        authHeader = request.value("authorization");
    }
    QString token;
    if (authHeader.startsWith("Bearer ")) {
        token = authHeader.mid(7);
    } else {
        token = request.value("token");
    }
    if (token.isEmpty()) {
        return createCorsResponse("Token required", QHttpServerResponse::StatusCode::BadRequest);
    }

    QJsonObject authorize = jwt_helper::validateJWT(token);
    if (!authorize.contains("userId") || authorize.value("userId").toInt() <= 0) {
        return createCorsResponse("Invalid token", QHttpServerResponse::StatusCode::Unauthorized);
    }

    QJsonArray leaderboardData = buildLeaderboardData("monthly");

    QJsonObject responseJson;
    responseJson["success"] = true;
    responseJson["leaderboard"] = leaderboardData;
    responseJson["timeframe"] = "monthly";

    return createCorsResponse(responseJson, QHttpServerResponse::StatusCode::Ok);
}

QHttpServerResponse LeaderboardRoutes::getUserRanking(const QHttpServerRequest &request, const QString &userId)
{
    // Authentication
    QString authHeader = request.value("Authorization");
    if (authHeader.isEmpty()) {
        authHeader = request.value("authorization");
    }
    QString token;
    if (authHeader.startsWith("Bearer ")) {
        token = authHeader.mid(7);
    } else {
        token = request.value("token");
    }
    if (token.isEmpty()) {
        return createCorsResponse("Token required", QHttpServerResponse::StatusCode::BadRequest);
    }

    QJsonObject authorize = jwt_helper::validateJWT(token);
    if (!authorize.contains("userId") || authorize.value("userId").toInt() <= 0) {
        return createCorsResponse("Invalid token", QHttpServerResponse::StatusCode::Unauthorized);
    }

    int targetUserId = userId.toInt();
    if (targetUserId <= 0) {
        return createCorsResponse("Invalid user ID", QHttpServerResponse::StatusCode::BadRequest);
    }

    QSqlDatabase db = QSqlDatabase::database();

    // Get user information and ranking - Now uses user_activity_log for more details
    QSqlQuery userQuery(db);
    userQuery.prepare("SELECT u.id, u.username, u.email, u.xpPoints, u.coinBalance, u.level, "
                      "u.streak, u.totalProblemsCompleted, u.joinDate, "
                      "COUNT(DISTINCT ual.id) as totalActivities "
                      "FROM users u "
                      "LEFT JOIN user_activity_log ual ON u.id = ual.user_id "
                      "WHERE u.id = ? "
                      "GROUP BY u.id");
    userQuery.addBindValue(targetUserId);

    if (!userQuery.exec() || !userQuery.next()) {
        return createCorsResponse("User not found", QHttpServerResponse::StatusCode::NotFound);
    }

    // Get user's current rank
    QSqlQuery rankQuery(db);
    rankQuery.prepare("SELECT COUNT(*) + 1 as user_rank FROM users "
                      "WHERE (xpPoints > (SELECT xpPoints FROM users WHERE id = ?)) "
                      "OR (xpPoints = (SELECT xpPoints FROM users WHERE id = ?) AND id < ?)");
    rankQuery.addBindValue(targetUserId);
    rankQuery.addBindValue(targetUserId);
    rankQuery.addBindValue(targetUserId);
    rankQuery.exec();

    int userRank = 1;
    if (rankQuery.next()) {
        userRank = rankQuery.value("user_rank").toInt();
    }

    // Get solved problems for this user
    QJsonArray solvedProblems = getUserSolvedProblems(targetUserId);

    // Build response
    QJsonObject userInfo;
    userInfo["id"] = userQuery.value("id").toInt();
    userInfo["username"] = userQuery.value("username").toString();
    userInfo["email"] = userQuery.value("email").toString();
    userInfo["xpPoints"] = userQuery.value("xpPoints").toInt();
    userInfo["coinBalance"] = userQuery.value("coinBalance").toInt();
    userInfo["level"] = userQuery.value("level").toInt();
    userInfo["streak"] = userQuery.value("streak").toInt();
    userInfo["totalProblemsCompleted"] = userQuery.value("totalProblemsCompleted").toInt();
    userInfo["totalActivities"] = userQuery.value("totalActivities").toInt();
    userInfo["joinDate"] = userQuery.value("joinDate").toDateTime().toString(Qt::ISODate);
    userInfo["currentRank"] = userRank;
    userInfo["solvedProblems"] = solvedProblems;

    QJsonObject responseJson;
    responseJson["success"] = true;
    responseJson["user"] = userInfo;

    return createCorsResponse(responseJson, QHttpServerResponse::StatusCode::Ok);
}

QJsonArray LeaderboardRoutes::buildLeaderboardData(const QString &timeframe)
{
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query(db);

    QString sql;
    if (timeframe == "weekly") {
        sql = "SELECT u.id, u.username, u.xpPoints, u.coinBalance, u.level, u.streak, "
              "u.totalProblemsCompleted, u.joinDate, "
              "COUNT(DISTINCT ual.id) as weeklyActivities, "
              "COALESCE(SUM(CASE WHEN ual.created_at >= datetime('now', '-7 days') THEN ual.xp_earned ELSE 0 END), 0) as weeklyXP, "
              "COALESCE(SUM(CASE WHEN ual.created_at >= datetime('now', '-7 days') THEN ual.points_earned ELSE 0 END), 0) as weeklyPoints "
              "FROM users u "
              "LEFT JOIN user_activity_log ual ON u.id = ual.user_id "
              "WHERE u.role = 'student' "
              "GROUP BY u.id "
              "ORDER BY weeklyXP DESC, u.xpPoints DESC, u.totalProblemsCompleted DESC "
              "LIMIT 100";
    } else if (timeframe == "monthly") {
        sql = "SELECT u.id, u.username, u.xpPoints, u.coinBalance, u.level, u.streak, "
              "u.totalProblemsCompleted, u.joinDate, "
              "COUNT(DISTINCT ual.id) as monthlyActivities, "
              "COALESCE(SUM(CASE WHEN ual.created_at >= datetime('now', '-30 days') THEN ual.xp_earned ELSE 0 END), 0) as monthlyXP, "
              "COALESCE(SUM(CASE WHEN ual.created_at >= datetime('now', '-30 days') THEN ual.points_earned ELSE 0 END), 0) as monthlyPoints "
              "FROM users u "
              "LEFT JOIN user_activity_log ual ON u.id = ual.user_id "
              "WHERE u.role = 'student' "
              "GROUP BY u.id "
              "ORDER BY monthlyXP DESC, u.xpPoints DESC, u.totalProblemsCompleted DESC "
              "LIMIT 100";
    } else {
        // All time leaderboard
        sql = "SELECT u.id, u.username, u.xpPoints, u.coinBalance, u.level, u.streak, "
              "u.totalProblemsCompleted, u.joinDate, "
              "COUNT(DISTINCT ual.id) as totalActivities "
              "FROM users u "
              "LEFT JOIN user_activity_log ual ON u.id = ual.user_id "
              "WHERE u.role = 'student' "
              "GROUP BY u.id "
              "ORDER BY u.xpPoints DESC, u.totalProblemsCompleted DESC, u.coinBalance DESC "
              "LIMIT 100";
    }

    query.prepare(sql);

    if (!query.exec()) {
        qDebug() << "Leaderboard query error:" << query.lastError().text();
        return QJsonArray();
    }

    QJsonArray leaderboardArray;
    int rank = 1;

    while (query.next()) {
        QJsonObject userObj;
        int userId = query.value("id").toInt();

        userObj["rank"] = rank++;
        userObj["id"] = userId;
        userObj["username"] = query.value("username").toString();
        userObj["xpPoints"] = query.value("xpPoints").toInt();
        userObj["coinBalance"] = query.value("coinBalance").toInt();
        userObj["level"] = query.value("level").toInt();
        userObj["streak"] = query.value("streak").toInt();
        userObj["totalProblemsCompleted"] = query.value("totalProblemsCompleted").toInt();
        userObj["joinDate"] = query.value("joinDate").toDateTime().toString(Qt::ISODate);

        if (timeframe == "weekly") {
            userObj["weeklyActivities"] = query.value("weeklyActivities").toInt();
            userObj["weeklyXP"] = query.value("weeklyXP").toInt();
            userObj["weeklyPoints"] = query.value("weeklyPoints").toInt();
        } else if (timeframe == "monthly") {
            userObj["monthlyActivities"] = query.value("monthlyActivities").toInt();
            userObj["monthlyXP"] = query.value("monthlyXP").toInt();
            userObj["monthlyPoints"] = query.value("monthlyPoints").toInt();
        } else {
            userObj["totalActivities"] = query.value("totalActivities").toInt();
        }

        // Get solved problems for this user
        userObj["solvedProblems"] = getUserSolvedProblems(userId);

        leaderboardArray.append(userObj);
    }

    return leaderboardArray;
}

// Función completa que usa las nuevas tablas
QJsonArray LeaderboardRoutes::getUserSolvedProblems(int userId)
{
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query(db);

    // Get solved problems from problem_attempts table
    query.prepare("SELECT DISTINCT p.id, p.title, p.difficulty, p.topic, p.pointValue, p.xpValue, "
                  "pa.timestamp as solvedAt, pa.xpEarned, pa.coinsEarned, pa.timeSpent "
                  "FROM problems p "
                  "INNER JOIN problem_attempts pa ON p.id = pa.problemId "
                  "WHERE pa.userId = ? AND pa.correct = 1 "
                  "ORDER BY pa.timestamp DESC");
    query.addBindValue(userId);

    if (!query.exec()) {
        qDebug() << "Error getting solved problems:" << query.lastError().text();
        return QJsonArray();
    }

    QJsonArray solvedProblemsArray;
    while (query.next()) {
        QJsonObject problemObj;
        problemObj["id"] = query.value("id").toInt();
        problemObj["title"] = query.value("title").toString();
        problemObj["difficulty"] = query.value("difficulty").toString();
        problemObj["topic"] = query.value("topic").toString();
        problemObj["pointValue"] = query.value("pointValue").toInt();
        problemObj["xpValue"] = query.value("xpValue").toInt();
        problemObj["solvedAt"] = query.value("solvedAt").toDateTime().toString(Qt::ISODate);
        problemObj["xpEarned"] = query.value("xpEarned").toInt();
        problemObj["coinsEarned"] = query.value("coinsEarned").toInt();
        problemObj["timeSpent"] = query.value("timeSpent").toInt();

        solvedProblemsArray.append(problemObj);
    }

    return solvedProblemsArray;
}

void LeaderboardRoutes::updateLeaderboardHistory(const QString &timeframe)
{
    QJsonArray leaderboardData = buildLeaderboardData(timeframe);
    QSqlDatabase db = QSqlDatabase::database();

    // Clear old entries for this timeframe (optional - keep last 30 days of history)
    QSqlQuery clearQuery(db);
    clearQuery.prepare("DELETE FROM leaderboard_history "
                       "WHERE timeframe = ? AND recorded_at < datetime('now', '-30 days')");
    clearQuery.addBindValue(timeframe);
    clearQuery.exec();

    // Insert current leaderboard snapshot
    for (int i = 0; i < leaderboardData.size(); ++i) {
        QJsonObject userObj = leaderboardData[i].toObject();

        QSqlQuery insertQuery(db);
        insertQuery.prepare("INSERT INTO leaderboard_history "
                            "(user_id, rank_position, points, xp_points, problems_completed, timeframe) "
                            "VALUES (?, ?, ?, ?, ?, ?)");
        insertQuery.addBindValue(userObj["id"].toInt());
        insertQuery.addBindValue(userObj["rank"].toInt());

        // Use appropriate points based on timeframe
        int points = 0;
        if (timeframe == "weekly" && userObj.contains("weeklyPoints")) {
            points = userObj["weeklyPoints"].toInt();
        } else if (timeframe == "monthly" && userObj.contains("monthlyPoints")) {
            points = userObj["monthlyPoints"].toInt();
        } else {
            points = userObj["xpPoints"].toInt();
        }

        insertQuery.addBindValue(points);
        insertQuery.addBindValue(userObj["xpPoints"].toInt());
        insertQuery.addBindValue(userObj["totalProblemsCompleted"].toInt());
        insertQuery.addBindValue(timeframe);

        if (!insertQuery.exec()) {
            qDebug() << "Error inserting leaderboard history:" << insertQuery.lastError().text();
        }
    }

    qDebug() << "Leaderboard history updated for timeframe:" << timeframe;
}
