#include "problems_routes.h"

void ProblemsRoutes::setupRoutes(QHttpServer *server)
{
    // GET /problems/difficulty/{Difficulty}/{topic}
    server->route("/problems/difficulty/<arg>/<arg>",
                  QHttpServerRequest::Method::Options,
                  [](const QString &difficulty, const QString &topic, const QHttpServerRequest &req) {
                      Q_UNUSED(req)
                      return createCorsResponse("", QHttpServerResponse::StatusCode::Ok);
                  });
    server->route("/problems/difficulty/<arg>/<arg>",
                  QHttpServerRequest::Method::Get,
                  [](const QString &difficulty, const QString &topic, const QHttpServerRequest &req) {
                      return ProblemsRoutes::getProblems(difficulty, topic, req);
                  });

    // GET /problems/{id}
    server->route("/problems/<arg>",
                  QHttpServerRequest::Method::Options,
                  [](const QString &id, const QHttpServerRequest &req) {
                      Q_UNUSED(req)
                      return createCorsResponse("", QHttpServerResponse::StatusCode::Ok);
                  });
    server->route("/problems/<arg>",
                  QHttpServerRequest::Method::Get,
                  [](const QString &id, const QHttpServerRequest &req) {
                      return ProblemsRoutes::getProblem(req, id);
                  });

    // POST /problems/{id}/submit
    server->route("/problems/<arg>/submit",
                  QHttpServerRequest::Method::Options,
                  [](const QString &id, const QHttpServerRequest &req) {
                      Q_UNUSED(req)
                      return createCorsResponse("", QHttpServerResponse::StatusCode::Ok);
                  });
    server->route("/problems/<arg>/submit",
                  QHttpServerRequest::Method::Post,
                  [](const QString &id, const QHttpServerRequest &req) {
                      return ProblemsRoutes::submitProblem(req, id);
                  });

    // GET /problems/{id}/attempts
    server->route("/problems/<arg>/attempts",
                  QHttpServerRequest::Method::Options,
                  [](const QString &id, const QHttpServerRequest &req) {
                      Q_UNUSED(req)
                      return createCorsResponse("", QHttpServerResponse::StatusCode::Ok);
                  });
    server->route("/problems/<arg>/attempts",
                  QHttpServerRequest::Method::Get,
                  [](const QString &id, const QHttpServerRequest &req) {
                      return ProblemsRoutes::getProblemAttempts(req, id);
                  });

    // GET /problems/recommendations/{userId}
    server->route("/problems/recommendations/<arg>",
                  QHttpServerRequest::Method::Options,
                  [](const QString &userId, const QHttpServerRequest &req) {
                      Q_UNUSED(req)
                      return createCorsResponse("", QHttpServerResponse::StatusCode::Ok);
                  });
    server->route("/problems/recommendations/<arg>",
                  QHttpServerRequest::Method::Get,
                  [](const QString &userId, const QHttpServerRequest &req) {
                      return ProblemsRoutes::getRecommendations(req, userId);
                  });
}

QHttpServerResponse ProblemsRoutes::getProblems(const QString &difficulty, const QString &topic, const QHttpServerRequest &request)
{
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
    qDebug() << "authorization:" << authorize;

    if (!authorize.contains("userId") || authorize.value("userId").toInt() <= 0) {
        return createCorsResponse("Invalid token", QHttpServerResponse::StatusCode::Unauthorized);
    }

    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery q(db);
    q.prepare("SELECT id, title, description, difficulty, topic, pointValue, xpValue, "
              "estimatedTime, tags, concepts, type, timeLimit, createdAt "
              "FROM problems WHERE difficulty = ? AND topic = ?");
    q.addBindValue(difficulty);
    q.addBindValue(topic);

    if (!q.exec()) {
        qDebug() << "Query error:" << q.lastError().text();
        return createCorsResponse("Database error",
                                                 QHttpServerResponse::StatusCode::InternalServerError);
    }

    QJsonArray problemsArr;
    while (q.next()) {
        QJsonObject problemJson;
        problemJson["id"] = q.value("id").toInt();
        problemJson["title"] = q.value("title").toString();
        problemJson["description"] = q.value("description").toString();
        problemJson["difficulty"] = q.value("difficulty").toString();
        problemJson["topic"] = q.value("topic").toString();
        problemJson["pointValue"] = q.value("pointValue").toInt();
        problemJson["xpValue"] = q.value("xpValue").toInt();
        problemJson["estimatedTime"] = q.value("estimatedTime").toInt();

        // Convert comma-separated tags and concepts to arrays
        QString tagsStr = q.value("tags").toString();
        QJsonArray tagsArray;
        if (!tagsStr.isEmpty()) {
            QStringList tagsList = tagsStr.split(",", Qt::SkipEmptyParts);
            for (const QString &tag : tagsList) {
                tagsArray.append(tag.trimmed());
            }
        }
        problemJson["tags"] = tagsArray;

        QString conceptsStr = q.value("concepts").toString();
        QJsonArray conceptsArray;
        if (!conceptsStr.isEmpty()) {
            QStringList conceptsList = conceptsStr.split(",", Qt::SkipEmptyParts);
            for (const QString &concept : conceptsList) {
                conceptsArray.append(concept.trimmed());
            }
        }
        problemJson["concepts"] = conceptsArray;

        problemJson["type"] = q.value("type").toString();
        problemJson["timeLimit"] = q.value("timeLimit").toInt();
        problemJson["createdAt"] = q.value("createdAt").toDateTime().toString(Qt::ISODate);

        problemsArr.append(problemJson);
    }

    QJsonDocument responseJson(problemsArr);
    return createCorsResponse(responseJson.toJson(), QHttpServerResponse::StatusCode::Ok);
}

QHttpServerResponse ProblemsRoutes::getProblem(const QHttpServerRequest &request, const QString &id)
{
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
    qDebug() << "authorization:" << authorize;

    if (!authorize.contains("userId") || authorize.value("userId").toInt() <= 0) {
        return createCorsResponse("Invalid token", QHttpServerResponse::StatusCode::Unauthorized);
    }

    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery q(db);
    q.prepare("SELECT id, title, description, difficulty, topic, pointValue, xpValue, "
              "estimatedTime, tags, concepts, type, timeLimit, explanation, createdAt "
              "FROM problems WHERE id = ?");
    q.addBindValue(id);

    if (!q.exec()) {
        qDebug() << "Query error:" << q.lastError().text();
        return createCorsResponse("Database error",
                                                 QHttpServerResponse::StatusCode::InternalServerError);
    }

    if (!q.next()) {
        return createCorsResponse("Problem not found", QHttpServerResponse::StatusCode::NotFound);
    }

    QJsonObject problemJson;
    problemJson["id"] = q.value("id").toInt();
    problemJson["title"] = q.value("title").toString();
    problemJson["description"] = q.value("description").toString();
    problemJson["difficulty"] = q.value("difficulty").toString();
    problemJson["topic"] = q.value("topic").toString();
    problemJson["pointValue"] = q.value("pointValue").toInt();
    problemJson["xpValue"] = q.value("xpValue").toInt();
    problemJson["estimatedTime"] = q.value("estimatedTime").toInt();

    // Convert comma-separated tags and concepts to arrays
    QString tagsStr = q.value("tags").toString();
    QJsonArray tagsArray;
    if (!tagsStr.isEmpty()) {
        QStringList tagsList = tagsStr.split(",", Qt::SkipEmptyParts);
        for (const QString &tag : tagsList) {
            tagsArray.append(tag.trimmed());
        }
    }
    problemJson["tags"] = tagsArray;

    QString conceptsStr = q.value("concepts").toString();
    QJsonArray conceptsArray;
    if (!conceptsStr.isEmpty()) {
        QStringList conceptsList = conceptsStr.split(",", Qt::SkipEmptyParts);
        for (const QString &concept : conceptsList) {
            conceptsArray.append(concept.trimmed());
        }
    }
    problemJson["concepts"] = conceptsArray;

    problemJson["type"] = q.value("type").toString();
    problemJson["timeLimit"] = q.value("timeLimit").toInt();
    problemJson["explanation"] = q.value("explanation").toString();
    problemJson["createdAt"] = q.value("createdAt").toDateTime().toString(Qt::ISODate);

    return createCorsResponse(problemJson, QHttpServerResponse::StatusCode::Ok);
}

QHttpServerResponse ProblemsRoutes::submitProblem(const QHttpServerRequest &request, const QString &id)
{
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
    qDebug() << "authorization:" << authorize;

    if (!authorize.contains("userId") || authorize.value("userId").toInt() <= 0) {
        return createCorsResponse("Invalid token", QHttpServerResponse::StatusCode::Unauthorized);
    }

    int userId = authorize.value("userId").toInt();

    // Parse and validate input
    auto json = QJsonDocument::fromJson(request.body()).object();
    QString answer = json["answer"].toString();

    if (answer.isEmpty()) {
        return createCorsResponse("Answer is required", QHttpServerResponse::StatusCode::BadRequest);
    }

    // Get problem details first
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery problemQuery(db);
    problemQuery.prepare("SELECT title, difficulty, xpValue, pointValue, correctAnswer, explanation "
                         "FROM problems WHERE id = ?");
    problemQuery.addBindValue(id);

    if (!problemQuery.exec() || !problemQuery.next()) {
        return createCorsResponse("Problem not found", QHttpServerResponse::StatusCode::NotFound);
    }

    QString correctAnswer = problemQuery.value("correctAnswer").toString();
    bool isCorrect = (answer.trimmed().toLower() == correctAnswer.trimmed().toLower());

    int xpEarned = isCorrect ? problemQuery.value("xpValue").toInt() : 0;
    int coinsEarned = isCorrect ? problemQuery.value("pointValue").toInt() : 0;
    int pointsEarned = isCorrect ? problemQuery.value("pointValue").toInt() : 0;

    // Record the attempt
    QSqlQuery attemptQuery(db);
    attemptQuery.prepare("INSERT INTO problem_attempts (userId, problemId, answer, correct, "
                         "xpEarned, coinsEarned, timestamp) VALUES (?, ?, ?, ?, ?, ?, ?)");
    attemptQuery.addBindValue(userId);
    attemptQuery.addBindValue(id);
    attemptQuery.addBindValue(answer);
    attemptQuery.addBindValue(isCorrect);
    attemptQuery.addBindValue(xpEarned);
    attemptQuery.addBindValue(coinsEarned);
    attemptQuery.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));

    if (!attemptQuery.exec()) {
        qDebug() << "Insert attempt error:" << attemptQuery.lastError().text();
        return createCorsResponse("Failed to record attempt",
                                                 QHttpServerResponse::StatusCode::InternalServerError);
    }

    // Update user stats if correct
    int newLevel = 0;
    if (isCorrect) {
        QSqlQuery updateUserQuery(db);
        updateUserQuery.prepare("UPDATE users SET xp = xp + ?, coins = coins + ?, points = points + ? "
                                "WHERE id = ?");
        updateUserQuery.addBindValue(xpEarned);
        updateUserQuery.addBindValue(coinsEarned);
        updateUserQuery.addBindValue(pointsEarned);
        updateUserQuery.addBindValue(userId);
        updateUserQuery.exec();

        // Calculate new level (simplified - assuming 100 XP per level)
        QSqlQuery levelQuery(db);
        levelQuery.prepare("SELECT xp FROM users WHERE id = ?");
        levelQuery.addBindValue(userId);
        if (levelQuery.exec() && levelQuery.next()) {
            int totalXp = levelQuery.value("xp").toInt();
            newLevel = (totalXp / 100) + 1;
        }
    }

    // Check for achievements (simplified example)
    QJsonArray achievements;
    if (isCorrect) {
        QSqlQuery achievementQuery(db);
        achievementQuery.prepare("SELECT COUNT(*) FROM problem_attempts WHERE userId = ? AND correct = 1 "
                                 "AND problemId IN (SELECT id FROM problems WHERE topic = ?)");
        achievementQuery.addBindValue(userId);
        achievementQuery.addBindValue(problemQuery.value("title").toString().contains("Calculus") ? "Calculus" : "Math");

        if (achievementQuery.exec() && achievementQuery.next()) {
            int correctCount = achievementQuery.value(0).toInt();
            if (correctCount == 10) {
                QJsonObject achievement;
                achievement["id"] = 2;
                achievement["title"] = "Calculus Master";
                achievement["description"] = "Solved 10 calculus problems";
                achievement["icon"] = "📐";
                achievements.append(achievement);
            }
        }
    }

    // Get next suggested problem (simplified)
    int nextSuggestedProblem = 0;
    QSqlQuery nextQuery(db);
    nextQuery.prepare("SELECT id FROM problems WHERE id > ? ORDER BY id LIMIT 1");
    nextQuery.addBindValue(id);
    if (nextQuery.exec() && nextQuery.next()) {
        nextSuggestedProblem = nextQuery.value("id").toInt();
    }

    // Create response
    QJsonObject responseJson;
    responseJson["success"] = true;
    responseJson["correct"] = isCorrect;
    responseJson["message"] = isCorrect ? "Correct! Well done." : "Incorrect. Try again!";
    responseJson["xpEarned"] = xpEarned;
    responseJson["coinsEarned"] = coinsEarned;
    responseJson["pointsEarned"] = pointsEarned;
    responseJson["newLevel"] = newLevel;
    responseJson["achievements"] = achievements;
    responseJson["explanation"] = problemQuery.value("explanation").toString();
    responseJson["nextSuggestedProblem"] = nextSuggestedProblem;

    return createCorsResponse(responseJson, QHttpServerResponse::StatusCode::Ok);
}

QHttpServerResponse ProblemsRoutes::getProblemAttempts(const QHttpServerRequest &request, const QString &problemId)
{
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
    qDebug() << "authorization:" << authorize;

    if (!authorize.contains("userId") || authorize.value("userId").toInt() <= 0) {
        return createCorsResponse("Invalid token", QHttpServerResponse::StatusCode::Unauthorized);
    }

    int userId = authorize.value("userId").toInt();

    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery q(db);
    q.prepare("SELECT pa.id, pa.problemId, p.title, p.difficulty, pa.correct, pa.timestamp, "
              "pa.timeSpent, pa.xpEarned, pa.coinsEarned "
              "FROM problem_attempts pa "
              "JOIN problems p ON pa.problemId = p.id "
              "WHERE pa.problemId = ? AND pa.userId = ? "
              "ORDER BY pa.timestamp DESC");
    q.addBindValue(problemId);
    q.addBindValue(userId);

    if (!q.exec()) {
        qDebug() << "Query error:" << q.lastError().text();
        return createCorsResponse("Database error",
                                                 QHttpServerResponse::StatusCode::InternalServerError);
    }

    QJsonArray attemptsArr;
    while (q.next()) {
        QJsonObject attemptJson;
        attemptJson["id"] = q.value("id").toInt();
        attemptJson["problemId"] = q.value("problemId").toInt();
        attemptJson["title"] = q.value("title").toString();
        attemptJson["difficulty"] = q.value("difficulty").toString();
        attemptJson["correct"] = q.value("correct").toBool();
        attemptJson["timestamp"] = q.value("timestamp").toDateTime().toString(Qt::ISODate);
        attemptJson["timeSpent"] = q.value("timeSpent").toInt();
        attemptJson["xpEarned"] = q.value("xpEarned").toInt();
        attemptJson["coinsEarned"] = q.value("coinsEarned").toInt();

        attemptsArr.append(attemptJson);
    }

    QJsonDocument responseJson(attemptsArr);
    return createCorsResponse(responseJson.toJson(), QHttpServerResponse::StatusCode::Ok);
}

QHttpServerResponse ProblemsRoutes::getRecommendations(const QHttpServerRequest &request, const QString &targetUserId)
{
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
    qDebug() << "authorization:" << authorize;

    if (!authorize.contains("userId") || authorize.value("userId").toInt() <= 0) {
        return createCorsResponse("Invalid token", QHttpServerResponse::StatusCode::Unauthorized);
    }

    // Simple recommendation logic: get problems not yet attempted by the user
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery q(db);
    q.prepare("SELECT p.id, p.title, p.difficulty, p.topic, p.pointValue, p.xpValue, p.estimatedTime "
              "FROM problems p "
              "LEFT JOIN problem_attempts pa ON p.id = pa.problemId AND pa.userId = ? "
              "WHERE pa.id IS NULL "
              "ORDER BY p.difficulty, p.xpValue DESC "
              "LIMIT 10");
    q.addBindValue(targetUserId);

    if (!q.exec()) {
        qDebug() << "Query error:" << q.lastError().text();
        return createCorsResponse("Database error",
                                                 QHttpServerResponse::StatusCode::InternalServerError);
    }

    QJsonArray recommendationsArr;
    while (q.next()) {
        QJsonObject recommendationJson;
        recommendationJson["id"] = q.value("id").toInt();
        recommendationJson["title"] = q.value("title").toString();
        recommendationJson["difficulty"] = q.value("difficulty").toString();
        recommendationJson["topic"] = q.value("topic").toString();
        recommendationJson["pointValue"] = q.value("pointValue").toInt();
        recommendationJson["xpValue"] = q.value("xpValue").toInt();
        recommendationJson["estimatedTime"] = q.value("estimatedTime").toInt();

        recommendationsArr.append(recommendationJson);
    }

    QJsonDocument responseJson(recommendationsArr);
    return createCorsResponse(responseJson.toJson(), QHttpServerResponse::StatusCode::Ok);
}
