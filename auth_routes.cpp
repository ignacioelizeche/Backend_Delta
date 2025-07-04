#include "auth_routes.h"

void AuthRoutes::setupRoutes(QHttpServer* server) {
    // CORS preflight OPTIONS
    server->route("/auth/register", QHttpServerRequest::Method::Options, [](const QHttpServerRequest &req) {
        Q_UNUSED(req)
        return createCorsResponse("", QHttpServerResponse::StatusCode::Ok);
    });
    server->route("/auth/login", QHttpServerRequest::Method::Options, [](const QHttpServerRequest &req) {
        Q_UNUSED(req)
        return createCorsResponse("", QHttpServerResponse::StatusCode::Ok);
    });
    server->route("/auth/logout", QHttpServerRequest::Method::Options, [](const QHttpServerRequest &req) {
        Q_UNUSED(req)
        return createCorsResponse("", QHttpServerResponse::StatusCode::Ok);
    });
    server->route("/users/me", QHttpServerRequest::Method::Options, [](const QHttpServerRequest &req) {
        Q_UNUSED(req)
        return createCorsResponse("", QHttpServerResponse::StatusCode::Ok);
    });

    // POST /auth/register
    server->route("/auth/register", QHttpServerRequest::Method::Post, [](const QHttpServerRequest &req) {
        return AuthRoutes::registerUser(req);
    });

    // POST /auth/login
    server->route("/auth/login", QHttpServerRequest::Method::Post, [](const QHttpServerRequest &req) {
        return AuthRoutes::login(req);
    });

    // POST /auth/logout
    server->route("/auth/logout", QHttpServerRequest::Method::Post, [](const QHttpServerRequest &req) {
        return AuthRoutes::logout(req);
    });

    // GET /users/me
    server->route("/users/me", QHttpServerRequest::Method::Get, [](const QHttpServerRequest &req) {
        return AuthRoutes::getCurrentUser(req);
    });
}

QHttpServerResponse AuthRoutes::registerUser(const QHttpServerRequest &req) {
    auto json = QJsonDocument::fromJson(req.body()).object();
    QString username = json["username"].toString();
    QString email = json["email"].toString();
    QString password = json["password"].toString();
    QString role = json["role"].toString();
    QDateTime time = QDateTime::currentDateTimeUtc();

    if (username.isEmpty() || password.isEmpty() || email.isEmpty()) {
        return createCorsResponse("Username, email and password required", QHttpServerResponse::StatusCode::BadRequest);
    }

    // Validar formato del email
    if (!isValidEmail(email)) {
        return createCorsResponse("Invalid email format", QHttpServerResponse::StatusCode::BadRequest);
    }

    // Generar salt y hash de la contraseña
    QString salt = generateSalt();
    QString hashedPassword = hashPassword(password, salt);

    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery q(db);

    // CORREGIDO: Agregar 'salt' en la lista de columnas (12 columnas, 12 placeholders)
    q.prepare("INSERT INTO users (username, email, password, salt, role, coinBalance, xpPoints, level, streak, totalProblemsCompleted, lastLoginDate, joinDate) "
              "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
    q.addBindValue(username);           // 1
    q.addBindValue(email);              // 2
    q.addBindValue(hashedPassword);     // 3
    q.addBindValue(salt);               // 4 - NUEVO
    q.addBindValue(role);               // 5
    q.addBindValue(500);                // 6
    q.addBindValue(0);                  // 7
    q.addBindValue(1);                  // 8
    q.addBindValue(0);                  // 9
    q.addBindValue(0);                  // 10
    q.addBindValue(time);               // 11
    q.addBindValue(time);               // 12

    if (!q.exec()) {
        qDebug() << "Insert error:" << q.lastError().text();
        if (q.lastError().text().contains("UNIQUE constraint failed")) {
            return createCorsResponse("Username or email already exists", QHttpServerResponse::StatusCode::Conflict);
        }
        return createCorsResponse("Registration failed", QHttpServerResponse::StatusCode::InternalServerError);
    }

    QJsonObject responseJson;
    responseJson["message"] = "User registered successfully";
    responseJson["success"] = true;

    return createCorsResponse(responseJson, QHttpServerResponse::StatusCode::Created);
}

QHttpServerResponse AuthRoutes::login(const QHttpServerRequest &req) {
    auto json = QJsonDocument::fromJson(req.body()).object();
    QString email = json["email"].toString();
    QString password = json["password"].toString();

    if (email.isEmpty() || password.isEmpty()) {
        return createCorsResponse("Email and password required", QHttpServerResponse::StatusCode::BadRequest);
    }

    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery q(db);

    // CORREGIDO: Agregar 'salt' en el SELECT
    q.prepare("SELECT id, username, email, password, salt, role, coinBalance, xpPoints, level, streak, totalProblemsCompleted, lastLoginDate, joinDate "
              "FROM users WHERE email = ?");
    q.addBindValue(email);

    if (!q.exec()) {
        qDebug() << "Query error:" << q.lastError().text();
        return createCorsResponse("Login failed", QHttpServerResponse::StatusCode::InternalServerError);
    }

    if (q.next()) {
        QString storedHash = q.value("password").toString();
        QString salt = q.value("salt").toString();

        // Verificar la contraseña
        if (!verifyPassword(password, storedHash, salt)) {
            QJsonObject errorJson;
            errorJson["message"] = "Invalid credentials";
            errorJson["success"] = false;
            return createCorsResponse(errorJson, QHttpServerResponse::StatusCode::Unauthorized);
        }

        // Resto del código permanece igual...
        int userId = q.value("id").toInt();
        QString username = q.value("username").toString();
        QString userEmail = q.value("email").toString();
        QString role = q.value("role").toString();
        int coinBalance = q.value("coinBalance").toInt();
        int xpPoints = q.value("xpPoints").toInt();
        int level = q.value("level").toInt();
        int streak = q.value("streak").toInt();
        int totalProblemsCompleted = q.value("totalProblemsCompleted").toInt();
        QDateTime previousLastLogin = q.value("lastLoginDate").toDateTime();
        QDateTime joinDate = q.value("joinDate").toDateTime();

        // Actualizar última fecha de login
        QSqlQuery updateQuery(db);
        updateQuery.prepare("UPDATE users SET lastLoginDate = ? WHERE id = ?");
        updateQuery.addBindValue(QDateTime::currentDateTime());
        updateQuery.addBindValue(userId);
        updateQuery.exec();

        QString token = jwt_helper::generateJWT(userId, userEmail, role);

        QJsonObject responseJson;
        responseJson["token"] = token;
        responseJson["success"] = true;

        QJsonObject userData;
        userData["id"] = userId;
        userData["username"] = username;
        userData["email"] = userEmail;
        userData["role"] = role;
        userData["coinBalance"] = coinBalance;
        userData["xpPoints"] = xpPoints;
        userData["level"] = level;
        userData["streak"] = streak;
        userData["totalProblemsCompleted"] = totalProblemsCompleted;
        userData["previousLastLogin"] = previousLastLogin.toString(Qt::ISODate);
        userData["joinDate"] = joinDate.toString(Qt::ISODate);

        responseJson["user"] = userData;

        return createCorsResponse(responseJson, QHttpServerResponse::StatusCode::Ok);
    }

    QJsonObject errorJson;
    errorJson["message"] = "Invalid credentials";
    errorJson["success"] = false;
    return createCorsResponse(errorJson, QHttpServerResponse::StatusCode::Unauthorized);
}

QHttpServerResponse AuthRoutes::logout(const QHttpServerRequest &req) {
    Q_UNUSED(req);
    QJsonObject responseJson;
    responseJson["success"] = true;
    responseJson["message"] = "Logged out successfully";
    return createCorsResponse(responseJson, QHttpServerResponse::StatusCode::Ok);
}

QHttpServerResponse AuthRoutes::getCurrentUser(const QHttpServerRequest &req) {
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

    if (token.isEmpty()) {
        return createCorsResponse("Token required", QHttpServerResponse::StatusCode::BadRequest);
    }

    QJsonObject authorize = jwt_helper::validateJWT(token);
    qDebug() << "authorization:" << authorize;
    int userId = authorize.value("userId").toInt();

    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery q(db);
    q.prepare("SELECT id, username, email, role, coinBalance, xpPoints, level, streak, totalProblemsCompleted, lastLoginDate FROM users WHERE id = ?");
    q.addBindValue(userId);

    if (!q.exec()) {
        qDebug() << "Query error:" << q.lastError().text();
        return createCorsResponse("Database error", QHttpServerResponse::StatusCode::InternalServerError);
    }

    if (q.next()) {
        QJsonObject responseJson;
        responseJson["id"] = q.value("id").toInt();
        responseJson["username"] = q.value("username").toString();
        responseJson["email"] = q.value("email").toString();
        responseJson["role"] = q.value("role").toString();
        responseJson["coinBalance"] = q.value("coinBalance").toInt();
        responseJson["xpPoints"] = q.value("xpPoints").toInt();
        responseJson["level"] = q.value("level").toInt();
        responseJson["streak"] = q.value("streak").toInt();
        responseJson["totalProblemsCompleted"] = q.value("totalProblemsCompleted").toInt();
        responseJson["lastActive"] = q.value("lastLoginDate").toDateTime().toString(Qt::ISODate);
        return createCorsResponse(responseJson, QHttpServerResponse::StatusCode::Ok);
    } else {
        return createCorsResponse("User not found", QHttpServerResponse::StatusCode::NotFound);
    }
}
