#include "problems_routes.h"

void ProblemsRoutes::setupRoutes(QHttpServer *server)
{
    // GET /problems/difficulty/{Difficulty}/{topic}
    server->route("/problems/difficulty/undefined/undefined",
                  QHttpServerRequest::Method::Options,
                  [](const QString &difficulty, const QString &topic, const QHttpServerRequest &req) {
                      Q_UNUSED(req)
                      return createCorsResponse("", QHttpServerResponse::StatusCode::Ok);
                  });
    // GET /problems - get all problems
    server->route("/problems/difficulty/undefined/undefined",
                  QHttpServerRequest::Method::Get,
                  [](const QHttpServerRequest &req) {
                      return ProblemsRoutes::getAllProblems(req);
                  });
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
    // POST /problems/{id}/submit
    server->route("/problems",
                  QHttpServerRequest::Method::Options,
                  [](const QHttpServerRequest &req) {
                      Q_UNUSED(req)
                      return createCorsResponse("", QHttpServerResponse::StatusCode::Ok);
                  });
    server->route("/problems",
                  QHttpServerRequest::Method::Post,
                  [](const QHttpServerRequest &req) {
                      return ProblemsRoutes::createProblem(req);
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
        qDebug() << "Query error:" << q.lastError().text();
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

// In your problems_routes.cpp file
QHttpServerResponse ProblemsRoutes::submitProblem(const QHttpServerRequest &request, const QString &id)
{
    // Authorization code
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

    // Get database instance
    QSqlDatabase db = database_manager::instance().database();

    // Start transaction for data consistency
    if (!db.transaction()) {
        qDebug() << "Failed to start transaction:" << db.lastError().text();
        return createCorsResponse("Database error", QHttpServerResponse::StatusCode::InternalServerError);
    }

    // Get problem details first
    QSqlQuery problemQuery(db);
    problemQuery.prepare("SELECT title, difficulty, xpValue, pointValue, correctAnswer, explanation, topic "
                         "FROM problems WHERE id = ?");
    problemQuery.addBindValue(id.toInt());

    if (!problemQuery.exec() || !problemQuery.next()) {
        db.rollback();
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
    attemptQuery.addBindValue(id.toInt());
    attemptQuery.addBindValue(answer);
    attemptQuery.addBindValue(isCorrect);
    attemptQuery.addBindValue(xpEarned);
    attemptQuery.addBindValue(coinsEarned);
    attemptQuery.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));

    if (!attemptQuery.exec()) {
        qDebug() << "Insert attempt error:" << attemptQuery.lastError().text();
        db.rollback();
        return createCorsResponse("Failed to record attempt",
                                  QHttpServerResponse::StatusCode::InternalServerError);
    }

    // Update user stats if correct (matching your column names)
    int newLevel = 0;
    if (isCorrect) {
        QSqlQuery updateUserQuery(db);
        updateUserQuery.prepare("UPDATE users SET xpPoints = xpPoints + ?, coinBalance = coinBalance + ?, "
                                "totalProblemsCompleted = totalProblemsCompleted + 1 WHERE id = ?");
        updateUserQuery.addBindValue(xpEarned);
        updateUserQuery.addBindValue(coinsEarned);
        updateUserQuery.addBindValue(userId);

        if (!updateUserQuery.exec()) {
            qDebug() << "Update user error:" << updateUserQuery.lastError().text();
            db.rollback();
            return createCorsResponse("Failed to update user stats",
                                      QHttpServerResponse::StatusCode::InternalServerError);
        }

        // Calculate new level (simplified - assuming 100 XP per level)
        QSqlQuery levelQuery(db);
        levelQuery.prepare("SELECT xpPoints FROM users WHERE id = ?");
        levelQuery.addBindValue(userId);
        if (levelQuery.exec() && levelQuery.next()) {
            int totalXp = levelQuery.value("xpPoints").toInt();
            newLevel = (totalXp / 100) + 1;

            // Update level if it changed
            if (newLevel > 1) {
                QSqlQuery updateLevelQuery(db);
                updateLevelQuery.prepare("UPDATE users SET level = ? WHERE id = ?");
                updateLevelQuery.addBindValue(newLevel);
                updateLevelQuery.addBindValue(userId);
                updateLevelQuery.exec();
            }
        }
    }

    // Commit transaction
    if (!db.commit()) {
        qDebug() << "Failed to commit transaction:" << db.lastError().text();
        db.rollback();
        return createCorsResponse("Database error", QHttpServerResponse::StatusCode::InternalServerError);
    }

    // Check for achievements (simplified example)
    QJsonArray achievements;
    if (isCorrect) {
        QSqlQuery achievementQuery(db);
        achievementQuery.prepare("SELECT COUNT(*) FROM problem_attempts WHERE userId = ? AND correct = 1 "
                                 "AND problemId IN (SELECT id FROM problems WHERE topic = ?)");
        achievementQuery.addBindValue(userId);
        QString topic = problemQuery.value("topic").toString();
        achievementQuery.addBindValue(topic);

        if (achievementQuery.exec() && achievementQuery.next()) {
            int correctCount = achievementQuery.value(0).toInt();
            if (correctCount == 10) {
                QJsonObject achievement;
                achievement["id"] = 2;
                achievement["title"] = topic + " Master";
                achievement["description"] = "Solved 10 " + topic.toLower() + " problems";
                achievement["icon"] = "🏆";
                achievements.append(achievement);
            }
        }
    }

    // Get next suggested problem (simplified)
    int nextSuggestedProblem = 0;
    QSqlQuery nextQuery(db);
    nextQuery.prepare("SELECT id FROM problems WHERE id > ? ORDER BY id LIMIT 1");
    nextQuery.addBindValue(id.toInt());
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

QHttpServerResponse ProblemsRoutes::createProblem(const QHttpServerRequest &request)
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
    qDebug() << "authorization:" << authorize;

    if (!authorize.contains("userId") || authorize.value("userId").toInt() <= 0) {
        return createCorsResponse("Invalid token", QHttpServerResponse::StatusCode::Unauthorized);
    }

    int userId = authorize.value("userId").toInt();

    // Parse and validate input JSON
    QJsonDocument jsonDoc = QJsonDocument::fromJson(request.body());
    if (jsonDoc.isNull() || !jsonDoc.isObject()) {
        return createCorsResponse("Invalid JSON format", QHttpServerResponse::StatusCode::BadRequest);
    }

    QJsonObject json = jsonDoc.object();

    // Required fields validation
    QStringList requiredFields = {"title", "description", "difficulty", "topic", "correctAnswer", "type"};
    for (const QString &field : requiredFields) {
        if (!json.contains(field) || json[field].toString().trimmed().isEmpty()) {
            return createCorsResponse(QString("Field '%1' is required").arg(field),
                                      QHttpServerResponse::StatusCode::BadRequest);
        }
    }

    // Extract and validate data
    QString title = json["title"].toString().trimmed();
    QString description = json["description"].toString().trimmed();
    QString difficulty = json["difficulty"].toString().trimmed().toLower();
    QString topic = json["topic"].toString().trimmed();
    QString correctAnswer = json["correctAnswer"].toString().trimmed();
    QString type = json["type"].toString().trimmed().toLower();

    // Validate difficulty
    QStringList validDifficulties = {"easy", "medium", "hard", "expert"};
    if (!validDifficulties.contains(difficulty)) {
        return createCorsResponse("Invalid difficulty. Must be: easy, medium, hard, or expert",
                                  QHttpServerResponse::StatusCode::BadRequest);
    }

    // Validate type
    QStringList validTypes = {"multiple_choice", "short_answer", "essay", "code", "true_false"};
    if (!validTypes.contains(type)) {
        return createCorsResponse("Invalid type. Must be: multiple_choice, short_answer, essay, code, or true_false",
                                  QHttpServerResponse::StatusCode::BadRequest);
    }

    // Optional fields with defaults
    int pointValue = json.contains("pointValue") ? json["pointValue"].toInt() : 10;
    int xpValue = json.contains("xpValue") ? json["xpValue"].toInt() : 5;
    int estimatedTime = json.contains("estimatedTime") ? json["estimatedTime"].toInt() : 5; // minutes
    int timeLimit = json.contains("timeLimit") ? json["timeLimit"].toInt() : 30; // minutes
    QString explanation = json.contains("explanation") ? json["explanation"].toString().trimmed() : "";

    // Validate numeric values
    if (pointValue < 1 || pointValue > 1000) {
        return createCorsResponse("Point value must be between 1 and 1000",
                                  QHttpServerResponse::StatusCode::BadRequest);
    }
    if (xpValue < 1 || xpValue > 500) {
        return createCorsResponse("XP value must be between 1 and 500",
                                  QHttpServerResponse::StatusCode::BadRequest);
    }
    if (estimatedTime < 1 || estimatedTime > 180) {
        return createCorsResponse("Estimated time must be between 1 and 180 minutes",
                                  QHttpServerResponse::StatusCode::BadRequest);
    }
    if (timeLimit < 1 || timeLimit > 300) {
        return createCorsResponse("Time limit must be between 1 and 300 minutes",
                                  QHttpServerResponse::StatusCode::BadRequest);
    }

    // Process tags array
    QString tagsStr = "";
    if (json.contains("tags") && json["tags"].isArray()) {
        QJsonArray tagsArray = json["tags"].toArray();
        QStringList tagsList;
        for (const auto &tagValue : tagsArray) {
            QString tag = tagValue.toString().trimmed();
            if (!tag.isEmpty()) {
                tagsList.append(tag);
            }
        }
        tagsStr = tagsList.join(",");
    }

    // Process concepts array
    QString conceptsStr = "";
    if (json.contains("concepts") && json["concepts"].isArray()) {
        QJsonArray conceptsArray = json["concepts"].toArray();
        QStringList conceptsList;
        for (const auto &conceptValue : conceptsArray) {
            QString concept = conceptValue.toString().trimmed();
            if (!concept.isEmpty()) {
                conceptsList.append(concept);
            }
        }
        conceptsStr = conceptsList.join(",");
    }

    // Insert into database - SIMPLIFIED VERSION
    QSqlDatabase db = QSqlDatabase::database();

    // Check database connection
    if (!db.isOpen()) {
        qDebug() << "Database connection error: Database is not open";
        return createCorsResponse("Database connection error",
                                  QHttpServerResponse::StatusCode::InternalServerError);
    }

    QSqlQuery insertQuery(db);

    // SIMPLIFIED INSERT - only include fields that definitely exist
    insertQuery.prepare("INSERT INTO problems (title, description, difficulty, topic, pointValue, "
                        "xpValue, explanation, estimatedTime, tags, concepts, type, timeLimit, correctAnswer, "
                        "createdAt) "
                        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");

    insertQuery.addBindValue(title);
    insertQuery.addBindValue(description);
    insertQuery.addBindValue(difficulty);
    insertQuery.addBindValue(topic);
    insertQuery.addBindValue(pointValue);
    insertQuery.addBindValue(xpValue);
    insertQuery.addBindValue(explanation);
    insertQuery.addBindValue(estimatedTime);
    insertQuery.addBindValue(tagsStr);
    insertQuery.addBindValue(conceptsStr);
    insertQuery.addBindValue(type);
    insertQuery.addBindValue(timeLimit);
    insertQuery.addBindValue(correctAnswer);
    insertQuery.addBindValue(QDateTime::currentDateTime());

    if (!insertQuery.exec()) {
        QString errorMsg = insertQuery.lastError().text();
        qDebug() << "Insert problem error:" << errorMsg;
        qDebug() << "Query:" << insertQuery.lastQuery();
        return createCorsResponse("Failed to create problem: " + errorMsg,
                                  QHttpServerResponse::StatusCode::InternalServerError);
    }

    // Get the ID of the newly inserted problem
    int problemId = insertQuery.lastInsertId().toInt();

    if (problemId <= 0) {
        qDebug() << "Failed to get last insert ID";
        return createCorsResponse("Failed to get problem ID after creation",
                                  QHttpServerResponse::StatusCode::InternalServerError);
    }

    // Create success response with the new problem data
    QJsonObject responseJson;
    responseJson["success"] = true;
    responseJson["message"] = "Problem created successfully";
    responseJson["problemId"] = problemId;
    responseJson["title"] = title;
    responseJson["difficulty"] = difficulty;
    responseJson["topic"] = topic;
    responseJson["pointValue"] = pointValue;
    responseJson["xpValue"] = xpValue;
    responseJson["estimatedTime"] = estimatedTime;
    responseJson["timeLimit"] = timeLimit;
    responseJson["type"] = type;
    responseJson["createdAt"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    if (!tagsStr.isEmpty()) {
        QJsonArray tagsArray;
        QStringList tagsList = tagsStr.split(",", Qt::SkipEmptyParts);
        for (const QString &tag : tagsList) {
            tagsArray.append(tag.trimmed());
        }
        responseJson["tags"] = tagsArray;
    }

    if (!conceptsStr.isEmpty()) {
        QJsonArray conceptsArray;
        QStringList conceptsList = conceptsStr.split(",", Qt::SkipEmptyParts);
        for (const QString &concept : conceptsList) {
            conceptsArray.append(concept.trimmed());
        }
        responseJson["concepts"] = conceptsArray;
    }

    return createCorsResponse(responseJson, QHttpServerResponse::StatusCode::Created);
}

QHttpServerResponse ProblemsRoutes::getAllProblems(const QHttpServerRequest &request)
{
    // Same authentication logic as your other methods
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

    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery q(db);
    // Remove the WHERE clause to get all problems
    q.prepare("SELECT id, title, description, difficulty, topic, pointValue, xpValue, "
              "estimatedTime, tags, concepts, type, timeLimit, createdAt "
              "FROM problems ORDER BY createdAt DESC");

    if (!q.exec()) {
        qDebug() << "Query error:" << q.lastError().text();
        return createCorsResponse("Database error",
                                  QHttpServerResponse::StatusCode::InternalServerError);
    }

    // Rest of the code is the same as getProblems()...
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
