#include "exams_routes.h"

void ExamsRoutes::setupRoutes(QHttpServer* server) {
    // GET /exams
    server->route("/documents", QHttpServerRequest::Method::Options, [](const QHttpServerRequest &req) {
        Q_UNUSED(req)
        return createCorsResponse("", QHttpServerResponse::StatusCode::Ok);
    });
    server->route("/documents", QHttpServerRequest::Method::Get,
                  [](const QHttpServerRequest &req) {  // Debe ser const reference
                      return ExamsRoutes::getExams(req);
                  });

    // POST /exams/upload
    server->route("/documents/uploadExams", QHttpServerRequest::Method::Options, [](const QHttpServerRequest &req) {
        Q_UNUSED(req)
        return createCorsResponse("", QHttpServerResponse::StatusCode::Ok);
    });
    server->route("/documents/uploadExams", QHttpServerRequest::Method::Post,
                  [](const QHttpServerRequest &req) {
                      return ExamsRoutes::uploadExam(req);
                  });

    // GET /exams/<id>
    server->route("/documents/view/<arg>", QHttpServerRequest::Method::Options, [](const QString &id, const QHttpServerRequest &req) {
        Q_UNUSED(req)
        return createCorsResponse("", QHttpServerResponse::StatusCode::Ok);
    });
    server->route("/documents/view/<arg>", QHttpServerRequest::Method::Get,
                  [](const QString &id,const QHttpServerRequest &req) {
                      return ExamsRoutes::getExam(req, id);
                  });

    server->route("/api/exams/<arg>/pdf", QHttpServerRequest::Method::Options, [](const QString &id, const QHttpServerRequest &req) {
        Q_UNUSED(req)
        return createCorsResponse("", QHttpServerResponse::StatusCode::Ok);
    });

    server->route("/api/exams/<arg>/pdf", QHttpServerRequest::Method::Get,
                  [](const QString &id,const QHttpServerRequest &req) {
        return ExamsRoutes::viewExam(req, id);
    });
}

QHttpServerResponse ExamsRoutes::getExams(const QHttpServerRequest &request) {
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

    if (authorize.isEmpty() || !authorize.contains("role")) {
        return createCorsResponse("Invalid token", QHttpServerResponse::StatusCode::Unauthorized);
    }

    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery q(db);
    if (!q.prepare("SELECT * "
                   "FROM documents ORDER BY uploadedat DESC")) {
        qDebug() << "Prepare error:" << q.lastError().text();
        return createCorsResponse("Database query preparation failed", QHttpServerResponse::StatusCode::InternalServerError);
    }

    if (!q.exec()) {
        qDebug() << "Query error:" << q.lastError().text();
        return createCorsResponse("Database query execution failed", QHttpServerResponse::StatusCode::InternalServerError);
    }

    QJsonArray documentsArr;
    while (q.next()) {
        QJsonObject documentJson;
        documentJson["id"] = q.value("id").toInt();
        documentJson["title"] = q.value("title").toString();
        documentJson["description"] = q.value("description").toString();
        documentJson["filename"] = q.value("filename").toString();
        documentJson["filesize"] = q.value("filesize").toInt();
        documentJson["pdfContent"] = q.value("filedata").toString();
        documentJson["uploadedBy"] = q.value("uploadedby").toInt();
        documentJson["uploadedAt"] = q.value("uploadedat").toDateTime().toString(Qt::ISODate);
        documentJson["updatedAt"] = q.value("updatedat").toDateTime().toString(Qt::ISODate);
        documentJson["subject"] = q.value("subject").toString();
        documentJson["category"] = q.value("category").toString();
        documentJson["difficulty"] = q.value("difficulty").toString();
        documentJson["pageCount"] = q.value("pagecount").toInt();
        documentJson["isPublic"] = q.value("ispublic").toBool();
        documentJson["isActive"] = q.value("isactive").toBool();
        documentJson["totalDownloads"] = q.value("totaldownloads").toInt();
        documentJson["totalViews"] = q.value("totalviews").toInt();
        documentJson["averageRating"] = q.value("averagerating").toDouble();
        documentJson["ratingCount"] = q.value("ratingcount").toInt();
        documentJson["createdByName"] = "Unknown";  // Cambiar si tienes una tabla de usuarios
        documentJson["additionalFiles"] = QJsonArray(); // En futuro puedes usar otra tabla

        // Parsear topics
        QJsonArray topicsArray;
        QString topicsStr = q.value("topics").toString();
        if (topicsStr.trimmed().startsWith("[") && topicsStr.trimmed().endsWith("]")) {
            QJsonDocument doc = QJsonDocument::fromJson(topicsStr.toUtf8());
            if (doc.isArray()) {
                topicsArray = doc.array();
            }
        }
        documentJson["topics"] = topicsArray;

        // Parsear tags
        QJsonArray tagsArray;
        QString tagsStr = q.value("tags").toString();
        if (tagsStr.trimmed().startsWith("[") && tagsStr.trimmed().endsWith("]")) {
            QJsonDocument doc = QJsonDocument::fromJson(tagsStr.toUtf8());
            if (doc.isArray()) {
                tagsArray = doc.array();
            }
        }
        documentJson["tags"] = tagsArray;

        // Parsear prerequisites
        QJsonArray prereqArray;
        QString prereqStr = q.value("prerequisites").toString();
        if (prereqStr.trimmed().startsWith("[") && prereqStr.trimmed().endsWith("]")) {
            QJsonDocument doc = QJsonDocument::fromJson(prereqStr.toUtf8());
            if (doc.isArray()) {
                prereqArray = doc.array();
            }
        }
        documentJson["prerequisites"] = prereqArray;

        documentsArr.append(documentJson);
    }

    QJsonDocument responseJson(documentsArr);
    return createCorsResponse(responseJson.toJson(), QHttpServerResponse::StatusCode::Ok);
}

QHttpServerResponse ExamsRoutes::getExam(const QHttpServerRequest &request, const QString &id)
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

    // Verificar que el token sea válido
    if (authorize.isEmpty() || !authorize.contains("role")) {
        return createCorsResponse("Invalid token", QHttpServerResponse::StatusCode::Unauthorized);
    }

    // Validar que el ID sea un número válido
    bool ok;
    int documentId = id.toInt(&ok);  // Fixed: Pass the ok variable by reference
    if (!ok || documentId <= 0) {
        return createCorsResponse("Invalid document ID", QHttpServerResponse::StatusCode::BadRequest);
    }

    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery q(db);
    q.prepare("SELECT id, title, filename, filesize, mimetype, filedata, uploadedBy, uploadedAt, updatedAt FROM documents WHERE id = ?");
    q.addBindValue(documentId);

    if (!q.exec()) {
        qDebug() << "Query error:" << q.lastError().text();
        return createCorsResponse("Database error", QHttpServerResponse::StatusCode::InternalServerError);
    }

    if (!q.next()) {
        return createCorsResponse("Document not found", QHttpServerResponse::StatusCode::NotFound);
    }

    QJsonObject documentJson;
    documentJson["id"] = q.value("id").toInt();
    documentJson["title"] = q.value("title").toString();
    documentJson["filename"] = q.value("filename").toString();
    documentJson["filesize"] = q.value("filesize").toLongLong();
    documentJson["mimetype"] = q.value("mimetype").toString();
    documentJson["filedata"] = q.value("filedata").toString(); // Base64 data
    documentJson["uploadedBy"] = q.value("uploadedBy").toInt();
    documentJson["uploadedAt"] = q.value("uploadedAt").toDateTime().toString(Qt::ISODate);
    documentJson["updatedAt"] = q.value("updatedAt").toDateTime().toString(Qt::ISODate);

    QJsonDocument responseJson(documentJson);
    return createCorsResponse(responseJson.toJson(), QHttpServerResponse::StatusCode::Ok);
}

QHttpServerResponse ExamsRoutes::uploadExam(const QHttpServerRequest &request) {
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

    // Validate JWT token
    QJsonObject authorize = jwt_helper::validateJWT(token);
    qDebug() << "authorization:" << authorize;

    if (authorize.isEmpty() || !authorize.contains("role")) {
        return createCorsResponse("Invalid token", QHttpServerResponse::StatusCode::Unauthorized);
    }

    // Check if request is JSON
    QString contentType = request.value("Content-Type");
    if (!contentType.contains("application/json")) {
        return createCorsResponse("Content-Type must be application/json",
                                  QHttpServerResponse::StatusCode::BadRequest);
    }

    // Parse JSON request body
    QByteArray requestBody = request.body();
    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(requestBody, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        return createCorsResponse("Invalid JSON format", QHttpServerResponse::StatusCode::BadRequest);
    }

    if (!jsonDoc.isObject()) {
        return createCorsResponse("JSON must be an object", QHttpServerResponse::StatusCode::BadRequest);
    }

    QJsonObject requestJson = jsonDoc.object();

    // Extract documentData from request
    if (!requestJson.contains("documentData") || !requestJson["documentData"].isObject()) {
        return createCorsResponse("documentData is required", QHttpServerResponse::StatusCode::BadRequest);
    }

    QJsonObject documentData = requestJson["documentData"].toObject();

    // Extract fields from documentData
    QString title = documentData["title"].toString();
    QString description = documentData["description"].toString();
    QString subject = documentData["subject"].toString();
    QString category = documentData["category"].toString();
    QString difficulty = documentData["difficulty"].toString();
    QString fileName = documentData["fileName"].toString();
    qint64 fileSize = documentData["fileSize"].toVariant().toLongLong();
    QString pdfContent = documentData["pdfContent"].toString();
    int pageCount = documentData["pageCount"].toInt();
    bool isPublic = documentData["isPublic"].toBool();
    bool isActive = documentData["isActive"].toBool(true); // Default to true

    QJsonArray topicsArray = documentData["topics"].toArray();
    QJsonArray tagsArray = documentData["tags"].toArray();
    QJsonArray prerequisitesArray = documentData["prerequisites"].toArray();

    // Validate required fields
    if (title.isEmpty()) {
        return createCorsResponse("Title is required", QHttpServerResponse::StatusCode::BadRequest);
    }

    if (subject.isEmpty()) {
        return createCorsResponse("Subject is required", QHttpServerResponse::StatusCode::BadRequest);
    }

    if (category.isEmpty()) {
        return createCorsResponse("Category is required", QHttpServerResponse::StatusCode::BadRequest);
    }

    if (pdfContent.isEmpty()) {
        return createCorsResponse("PDF content is required", QHttpServerResponse::StatusCode::BadRequest);
    }

    if (fileName.isEmpty()) {
        return createCorsResponse("File name is required", QHttpServerResponse::StatusCode::BadRequest);
    }

    // Validate file type
    if (!fileName.toLower().endsWith(".pdf")) {
        return createCorsResponse("Only PDF files are allowed", QHttpServerResponse::StatusCode::BadRequest);
    }

    // Validate category values
    QStringList validCategories = {"exam", "study-guide", "notes", "assignment", "reference"};
    if (!validCategories.contains(category.toLower())) {
        return createCorsResponse("Invalid category", QHttpServerResponse::StatusCode::BadRequest);
    }

    // Validate difficulty values (optional field)
    QStringList validDifficulties = {"easy", "medium", "hard"};
    if (!difficulty.isEmpty() && !validDifficulties.contains(difficulty.toLower())) {
        return createCorsResponse("Invalid difficulty level", QHttpServerResponse::StatusCode::BadRequest);
    }

    // Decode base64 content for hash calculation
    QByteArray fileData = QByteArray::fromBase64(pdfContent.toUtf8());
    if (fileData.isEmpty()) {
        return createCorsResponse("Invalid PDF content (base64)", QHttpServerResponse::StatusCode::BadRequest);
    }

    // Calculate file hash for integrity
    QString fileHash = ExamsRoutes::calculateFileHash(fileData);

    // If page count is 0, estimate it
    if (pageCount == 0) {
        pageCount = ExamsRoutes::estimatePDFPageCount(fileData);
    }

    // Get user ID from JWT token
    int createdBy = authorize["user_id"].toInt();
    QString createdByName = authorize["username"].toString();

    // Generate unique ID
    int documentId = QDateTime::currentMSecsSinceEpoch() % 1000000; // Simple ID generation

    // Get current timestamp
    QDateTime now = QDateTime::currentDateTime();
    QString createdAt = now.toString(Qt::ISODate);

    // Insert into database
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery q(db);

    // Updated query to match response format
    q.prepare("INSERT INTO documents (id, title, description, subject, category, difficulty, "
              "topics, tags, prerequisites, filename, filesize, filedata, pagecount, ispublic, "
              "isactive, uploadedby, uploadedat, updatedat, totaldownloads, totalviews, "
              "averagerating, ratingcount, filehash) "
              "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");

    q.addBindValue(documentId);
    q.addBindValue(title);
    q.addBindValue(description);
    q.addBindValue(subject);
    q.addBindValue(category);
    q.addBindValue(difficulty.isEmpty() ? QVariant() : difficulty);
    q.addBindValue(QJsonDocument(topicsArray).toJson(QJsonDocument::Compact));
    q.addBindValue(QJsonDocument(tagsArray).toJson(QJsonDocument::Compact));
    q.addBindValue(QJsonDocument(prerequisitesArray).toJson(QJsonDocument::Compact));
    q.addBindValue(fileName);
    q.addBindValue(fileSize);
    q.addBindValue(pdfContent); // Store base64 content
    q.addBindValue(pageCount);
    q.addBindValue(isPublic);
    q.addBindValue(isActive);
    q.addBindValue(createdBy);
    q.addBindValue(createdAt);
    q.addBindValue(createdAt); // updatedAt
    q.addBindValue(0); // totalDownloads
    q.addBindValue(0); // totalViews
    q.addBindValue(0.0); // averageRating
    q.addBindValue(0); // ratingCount
    q.addBindValue(fileHash);

    if (!q.exec()) {
        qDebug() << "Insert error:" << q.lastError().text();
        return createCorsResponse("Failed to save document",
                                  QHttpServerResponse::StatusCode::InternalServerError);
    }

    // Create response object matching the expected format
    QJsonObject responseJson;
    responseJson["id"] = documentId;
    responseJson["title"] = title;
    responseJson["description"] = description;
    responseJson["subject"] = subject;
    responseJson["category"] = category;
    if (!difficulty.isEmpty()) {
        responseJson["difficulty"] = difficulty;
    }
    responseJson["topics"] = topicsArray;
    responseJson["tags"] = tagsArray;
    responseJson["prerequisites"] = prerequisitesArray;
    responseJson["fileName"] = fileName;
    responseJson["fileSize"] = fileSize;
    responseJson["pdfContent"] = pdfContent;
    responseJson["pageCount"] = pageCount;
    responseJson["isPublic"] = isPublic;
    responseJson["isActive"] = isActive;
    responseJson["createdBy"] = createdBy;
    responseJson["createdByName"] = createdByName;
    responseJson["createdAt"] = createdAt;
    responseJson["updatedAt"] = createdAt;
    responseJson["totalDownloads"] = 0;
    responseJson["totalViews"] = 0;
    responseJson["averageRating"] = 0;
    responseJson["ratingCount"] = 0;
    responseJson["additionalFiles"] = QJsonArray(); // Empty array

    return createCorsResponse(responseJson, QHttpServerResponse::StatusCode::Created);
}

QHttpServerResponse ExamsRoutes::viewExam(const QHttpServerRequest &request, const QString &id) {
    // Authentication check
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

    // Validate JWT token
    QJsonObject authorize = jwt_helper::validateJWT(token);
    if (authorize.isEmpty() || !authorize.contains("role")) {
        return createCorsResponse("Invalid token", QHttpServerResponse::StatusCode::Unauthorized);
    }

    // Validate and convert ID parameter
    bool ok;
    int documentId = id.toInt(&ok);
    if (!ok || documentId <= 0) {
        return createCorsResponse("Invalid document ID", QHttpServerResponse::StatusCode::BadRequest);
    }

    // Query database to get document info and check permissions
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery q(db);

    q.prepare("SELECT d.filepath, d.filename, d.ispublic, d.uploadedby, d.isactive "
              "FROM documents d "
              "WHERE d.id = ? AND d.isactive = 1");
    q.addBindValue(documentId);

    if (!q.exec()) {
        qDebug() << "Query error:" << q.lastError().text();
        return createCorsResponse("Database error", QHttpServerResponse::StatusCode::InternalServerError);
    }

    if (!q.next()) {
        return createCorsResponse("Document not found", QHttpServerResponse::StatusCode::NotFound);
    }

    // Check access permissions
    int currentUserId = authorize["id"].toInt();
    bool isDocumentPublic = q.value("ispublic").toBool();
    int documentOwnerId = q.value("uploadedby").toInt();

    if (!isDocumentPublic && currentUserId != documentOwnerId) {
        return createCorsResponse("Access denied", QHttpServerResponse::StatusCode::Forbidden);
    }

    // Get file information
    QString relativeFilePath = q.value("filepath").toString();
    QString originalFileName = q.value("filename").toString();

    // Build absolute file path
    QString absoluteFilePath = QCoreApplication::applicationDirPath() + "/" + relativeFilePath;

    // Check if file exists and open it
    QFile file(absoluteFilePath);
    if (!file.exists()) {
        qDebug() << "File not found on disk:" << absoluteFilePath;
        return createCorsResponse("PDF file not found on server", QHttpServerResponse::StatusCode::NotFound);
    }

    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Failed to open file:" << absoluteFilePath;
        return createCorsResponse("Failed to open PDF file", QHttpServerResponse::StatusCode::InternalServerError);
    }

    // Read file content
    QByteArray pdfData = file.readAll();
    file.close();

    if (pdfData.isEmpty()) {
        qDebug() << "Empty file or failed to read:" << absoluteFilePath;
        return createCorsResponse("Failed to read PDF file", QHttpServerResponse::StatusCode::InternalServerError);
    }

    // Create response with PDF content
    QHttpServerResponse response("application/pdf", pdfData);
    QHttpHeaders headers;

    // Set headers for PDF viewing
    headers.append("Content-Disposition", QString("inline; filename=\"%1\"").arg(originalFileName));
    headers.append("Content-Length", QString::number(pdfData.size()));
    headers.append("Cache-Control", "public, max-age=3600"); // Cache for 1 hour

    // Add CORS headers if needed
    headers.append("Access-Control-Allow-Origin", "*");
    headers.append("Access-Control-Allow-Methods", "GET, OPTIONS");
    headers.append("Access-Control-Allow-Headers", "Content-Type, Authorization");

    response.setHeaders(headers);

    // Optional: Update download/view count
    QSqlQuery updateQuery(db);
    updateQuery.prepare("UPDATE documents SET totalviews = totalviews + 1 WHERE id = ?");
    updateQuery.addBindValue(documentId);
    updateQuery.exec(); // Don't fail if this doesn't work

    qDebug() << "PDF served successfully:" << absoluteFilePath << "Size:" << pdfData.size() << "bytes";

    return response;
}

// EXTRA FUNCTIONS TO CALCULATE THINGS FOR THE UPLOAD GET AND GET EXAM/S FUNCTIONS
QJsonArray ExamsRoutes::parseStringToJsonArray(const QString &str)
{
    if (str.isEmpty()) {
        return QJsonArray();
    }

    QJsonDocument doc = QJsonDocument::fromJson(str.toUtf8());
    if (doc.isArray()) {
        return doc.array();
    }

    return QJsonArray();
}

QString ExamsRoutes::calculateFileHash(const QByteArray &data)
{
    return QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex();
}

int ExamsRoutes::estimatePDFPageCount(const QByteArray &pdfData)
{
    // Simple heuristic: count "/Page" occurrences in PDF
    // In production, use a proper PDF library like Poppler-Qt5
    QString pdfString = QString::fromLatin1(pdfData);
    return pdfString.count("/Page");
}
