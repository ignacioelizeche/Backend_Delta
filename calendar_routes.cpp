#include "calendar_routes.h"

void CalendarRoutes::setupRoutes(QHttpServer *server)
{
    server->route("/calendar/events/<arg>/<arg>",
                  QHttpServerRequest::Method::Options,
                  [](const QString &start, const QString &end, const QHttpServerRequest &req) {
                      Q_UNUSED(req)
                      return createCorsResponse("", QHttpServerResponse::StatusCode::Ok);
                  });
    server->route("/calendar/events/<arg>/<arg>",
                  QHttpServerRequest::Method::Get,
                  [](const QString &start, const QString &end, const QHttpServerRequest &req) {
                      return CalendarRoutes::getEvents(req, start, end);
                  });

    server->route("/calendar/events",
                  QHttpServerRequest::Method::Options,
                  [](const QHttpServerRequest &req) {
                      Q_UNUSED(req)
                      return createCorsResponse("", QHttpServerResponse::StatusCode::Ok);
                  });
    server->route("/calendar/events",
                  QHttpServerRequest::Method::Post,
                  [](const QHttpServerRequest &req) { return CalendarRoutes::createEvent(req); });

    server->route("/calendar/events/<arg>",
                  QHttpServerRequest::Method::Options,
                  [](const QString &id, const QHttpServerRequest &req) {
                      Q_UNUSED(req)
                      return createCorsResponse("", QHttpServerResponse::StatusCode::Ok);
                  });
    server->route("/calendar/events/<arg>",
                  QHttpServerRequest::Method::Put,
                  [](const QString &id, const QHttpServerRequest &req) {
                      return CalendarRoutes::updateEvent(req, id);
                  });
    server->route("/calendar/events/<arg>",
                  QHttpServerRequest::Method::Delete,
                  [](const QString &id, const QHttpServerRequest &req) {
                      return CalendarRoutes::deleteEvent(req, id);
                  });
}

// Las implementaciones originales se mantienen si las vas a usar internamente
QHttpServerResponse CalendarRoutes::getEvents(const QHttpServerRequest &request,
                                              const QString &start,
                                              const QString &end)
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

    if (authorize.value("role") == "Student") {
        return createCorsResponse("Only Teachers can add events",
                                  QHttpServerResponse::StatusCode::InternalServerError);
    }

    // Parse and validate date parameters
    QDateTime startDate = QDateTime::fromString(start, Qt::ISODate);
    QDateTime endDate = QDateTime::fromString(end, Qt::ISODate);

    //validate format
    if (!startDate.isValid() || !endDate.isValid()) {
        return createCorsResponse("Invalid date format. Use ISO 8601 format (YYYY-MM-DDTHH:mm:ssZ)",
                                  QHttpServerResponse::StatusCode::BadRequest);
    }

    // Validate date range
    if (startDate >= endDate) {
        return createCorsResponse("Start date must be before end date",
                                  QHttpServerResponse::StatusCode::BadRequest);
    }

    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery q(db);
    q.prepare(
        "SELECT id, title, description, eventType, startTime, endTime, attendees, priority, color, "
        "createdBy, createdAt, updatedAt FROM calendar WHERE startTime < ? AND endTime > ?");
    q.addBindValue(endDate);
    q.addBindValue(startDate);

    if (!q.exec()) {
        qDebug() << "Query error:" << q.lastError().text();
        return createCorsResponse("Database error",
                                  QHttpServerResponse::StatusCode::InternalServerError);
    }

    QJsonArray eventsArr;
    while (q.next()) {
        QJsonObject eventJson;
        eventJson["id"] = q.value("id").toInt();
        eventJson["title"] = q.value("title").toString();
        eventJson["description"] = q.value("description").toString();
        eventJson["eventType"] = q.value("eventType").toString();
        eventJson["startTime"] = q.value("startTime").toDateTime().toString(Qt::ISODate);
        eventJson["endTime"] = q.value("endTime").toDateTime().toString(Qt::ISODate);

        // Convert comma-separated attendees string back to JSON array
        QString attendeesStr = q.value("attendees").toString();
        QJsonArray attendeesArray;
        if (!attendeesStr.isEmpty()) {
            QStringList attendeesList = attendeesStr.split(",", Qt::SkipEmptyParts);
            for (const QString &attendee : attendeesList) {
                attendeesArray.append(attendee.trimmed());
            }
        }
        eventJson["attendees"] = attendeesArray;

        eventJson["priority"] = q.value("priority").toString();
        eventJson["color"] = q.value("color").toString();
        eventJson["createdBy"] = q.value("createdBy").toInt();
        eventJson["createdAt"] = q.value("createdAt").toDateTime().toString(Qt::ISODate);
        eventJson["updatedAt"] = q.value("updatedAt").toDateTime().toString(Qt::ISODate);

        eventsArr.append(eventJson);
    }

    QJsonDocument responseJson(eventsArr);
    return createCorsResponse(responseJson.toJson(), QHttpServerResponse::StatusCode::Ok);
}

QHttpServerResponse CalendarRoutes::createEvent(const QHttpServerRequest &request)
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
        return createCorsResponse("Token requestuired", QHttpServerResponse::StatusCode::BadRequest);
    }
    QJsonObject authorize = jwt_helper::validateJWT(token);
    qDebug() << "authorization:" << authorize;

    // Parse and validate input
    auto json = QJsonDocument::fromJson(request.body()).object();
    QString title = json["title"].toString();
    QString description = json["description"].toString();
    QString eventType = json["eventType"].toString();
    QString priority = json["priority"].toString();
    QString color = json["color"].toString();

    QDateTime startDateTime = QDateTime::fromString(json["startTime"].toString(), Qt::ISODate);
    QDateTime endDateTime = QDateTime::fromString(json["endTime"].toString(), Qt::ISODate);
    if (!startDateTime.isValid() || !endDateTime.isValid()) {
        return createCorsResponse("Invalid date format",
                                  QHttpServerResponse::StatusCode::BadRequest);
    }

    // Handle attendees array
    QJsonArray attendeesArray = json["attendees"].toArray();
    QStringList attendeesList;
    for (const QJsonValue &value : attendeesArray) {
        attendeesList.append(value.toString());
    }
    QString attendees = attendeesList.join(","); // Store as comma-separated string

    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery q(db);
    q.prepare("INSERT INTO calendar (title, description, eventType, startTime, endTime, attendees, "
              "priority, color) VALUES (?, ?, ?, ?, ?, ?, ?, ?)");
    q.addBindValue(title);
    q.addBindValue(description);
    q.addBindValue(eventType);
    q.addBindValue(startDateTime.toString(Qt::ISODate));
    q.addBindValue(endDateTime.toString(Qt::ISODate));
    q.addBindValue(attendees);
    q.addBindValue(priority);
    q.addBindValue(color);

    if (!q.exec()) {
        qDebug() << "Insert error:" << q.lastError().text();
        // Check for unique constraint violations
        if (q.lastError().text().contains("UNIQUE constraint failed")) {
            return createCorsResponse("Something went wrong",
                                      QHttpServerResponse::StatusCode::Conflict);
        }
        return createCorsResponse("New event registration failed",
                                  QHttpServerResponse::StatusCode::InternalServerError);
    }

    // Get the inserted ID
    int insertedId = q.lastInsertId().toInt();

    // Create successful JSON response
    QJsonObject responseJson;
    responseJson["id"] = insertedId;
    responseJson["title"] = title;
    responseJson["description"] = description;
    responseJson["eventType"] = eventType;
    responseJson["startTime"] = startDateTime.toString(Qt::ISODate);
    responseJson["endTime"] = endDateTime.toString(Qt::ISODate);
    responseJson["attendees"] = attendeesArray; // Return as array
    responseJson["priority"] = priority;
    responseJson["color"] = color;

    return createCorsResponse(responseJson, QHttpServerResponse::StatusCode::Created);
}

QHttpServerResponse CalendarRoutes::updateEvent(const QHttpServerRequest &request, const QString &Id)
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

    // Parse and validate input
    auto json = QJsonDocument::fromJson(request.body()).object();

    // Check if event exists first
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery checkQuery(db);
    checkQuery.prepare("SELECT * FROM calendar WHERE id = ?");
    checkQuery.addBindValue(Id);
    if (!checkQuery.exec() || !checkQuery.next()) {
        return createCorsResponse("Event not found", QHttpServerResponse::StatusCode::NotFound);
    }

    // Get current values as defaults
    QString title = json.contains("title") ? json["title"].toString()
                                           : checkQuery.value("title").toString();
    QString description = json.contains("description") ? json["description"].toString()
                                                       : checkQuery.value("description").toString();
    QString eventType = json.contains("eventType") ? json["eventType"].toString()
                                                   : checkQuery.value("eventType").toString();
    QString priority = json.contains("priority") ? json["priority"].toString()
                                                 : checkQuery.value("priority").toString();
    QString color = json.contains("color") ? json["color"].toString()
                                           : checkQuery.value("color").toString();

    // Handle datetime fields
    QDateTime startDateTime, endDateTime;
    if (json.contains("startTime")) {
        startDateTime = QDateTime::fromString(json["startTime"].toString(), Qt::ISODate);
        if (!startDateTime.isValid()) {
            return createCorsResponse("Invalid start date format",
                                      QHttpServerResponse::StatusCode::BadRequest);
        }
    } else {
        startDateTime = QDateTime::fromString(checkQuery.value("startTime").toString(), Qt::ISODate);
    }

    if (json.contains("endTime")) {
        endDateTime = QDateTime::fromString(json["endTime"].toString(), Qt::ISODate);
        if (!endDateTime.isValid()) {
            return createCorsResponse("Invalid end date format",
                                      QHttpServerResponse::StatusCode::BadRequest);
        }
    } else {
        endDateTime = QDateTime::fromString(checkQuery.value("endTime").toString(), Qt::ISODate);
    }

    // Handle attendees array
    QString attendees;
    QJsonArray attendeesArray;
    if (json.contains("attendees")) {
        attendeesArray = json["attendees"].toArray();
        QStringList attendeesList;
        for (const QJsonValue &value : attendeesArray) {
            attendeesList.append(value.toString());
        }
        attendees = attendeesList.join(",");
    } else {
        attendees = checkQuery.value("attendees").toString();
        // Convert back to array for response
        QStringList attendeesList = attendees.split(",", Qt::SkipEmptyParts);
        for (const QString &attendee : attendeesList) {
            attendeesArray.append(attendee);
        }
    }

    // Update the event
    QSqlQuery updateQuery(db);
    updateQuery.prepare("UPDATE calendar SET title = ?, description = ?, eventType = ?, startTime "
                        "= ?, endTime = ?, attendees = ?, priority = ?, color = ? WHERE id = ?");
    updateQuery.addBindValue(title);
    updateQuery.addBindValue(description);
    updateQuery.addBindValue(eventType);
    updateQuery.addBindValue(startDateTime.toString(Qt::ISODate));
    updateQuery.addBindValue(endDateTime.toString(Qt::ISODate));
    updateQuery.addBindValue(attendees);
    updateQuery.addBindValue(priority);
    updateQuery.addBindValue(color);
    updateQuery.addBindValue(Id);

    if (!updateQuery.exec()) {
        qDebug() << "Update error:" << updateQuery.lastError().text();
        // Check for unique constraint violations
        if (updateQuery.lastError().text().contains("UNIQUE constraint failed")) {
            return createCorsResponse("Something went wrong",
                                      QHttpServerResponse::StatusCode::Conflict);
        }
        return createCorsResponse("Event update failed",
                                  QHttpServerResponse::StatusCode::InternalServerError);
    }

    // Check if any rows were affected
    if (updateQuery.numRowsAffected() == 0) {
        return createCorsResponse("Event not found", QHttpServerResponse::StatusCode::NotFound);
    }

    // Create successful JSON response
    QJsonObject responseJson;
    responseJson["id"] = Id;
    responseJson["title"] = title;
    responseJson["description"] = description;
    responseJson["eventType"] = eventType;
    responseJson["startTime"] = startDateTime.toString(Qt::ISODate);
    responseJson["endTime"] = endDateTime.toString(Qt::ISODate);
    responseJson["attendees"] = attendeesArray;
    responseJson["priority"] = priority;
    responseJson["color"] = color;

    return createCorsResponse(responseJson, QHttpServerResponse::StatusCode::Ok);
}

QHttpServerResponse CalendarRoutes::deleteEvent(const QHttpServerRequest &request, const QString &Id)
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

    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery deleteQuery(db);
    deleteQuery.prepare("DELETE FROM calendar WHERE id = ?");
    deleteQuery.addBindValue(Id);

    if (!deleteQuery.exec()) {
        qDebug() << "Delete error:" << deleteQuery.lastError().text();
        return createCorsResponse("Event deletion failed",
                                  QHttpServerResponse::StatusCode::InternalServerError);
    }

    // Check if any rows were affected (event existed and was deleted)
    if (deleteQuery.numRowsAffected() == 0) {
        return createCorsResponse("Event not found", QHttpServerResponse::StatusCode::NotFound);
    }

    // Create successful JSON response
    QJsonObject responseJson;
    responseJson["success"] = true;
    responseJson["message"] = "Event deleted successfully";

    return createCorsResponse(responseJson, QHttpServerResponse::StatusCode::Ok);
}
