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

    // Validate JWT token
    QJsonObject authorize = jwt_helper::validateJWT(token);
    qDebug() << "authorization:" << authorize;

    if (authorize.isEmpty() || !authorize.contains("role")) {
        return createCorsResponse("Invalid token", QHttpServerResponse::StatusCode::Unauthorized);
    }

    // Get query parameters
    QUrlQuery urlQuery(request.url().query());
    QString category = urlQuery.queryItemValue("category");
    QString subject = urlQuery.queryItemValue("subject");
    QString difficulty = urlQuery.queryItemValue("difficulty");
    int limit = urlQuery.queryItemValue("limit").toInt();
    int offset = urlQuery.queryItemValue("offset").toInt();

    // Set default values
    if (limit <= 0) {
        limit = 50; // Default limit
    }
    if (offset < 0) {
        offset = 0; // Default offset
    }

    // Validate category if provided - UPDATED to match upload function categories
    if (!category.isEmpty()) {
        QStringList validCategories = {"exam", "study-guide", "notes", "assignment", "reference"};
        if (!validCategories.contains(category.toLower())) {
            return createCorsResponse("Invalid category", QHttpServerResponse::StatusCode::BadRequest);
        }
    }

    // Validate difficulty if provided
    if (!difficulty.isEmpty()) {
        QStringList validDifficulties = {"easy", "medium", "hard"};
        if (!validDifficulties.contains(difficulty.toLower())) {
            return createCorsResponse("Invalid difficulty level", QHttpServerResponse::StatusCode::BadRequest);
        }
    }

    // Build SQL query with filters - UPDATED to use filepath instead of filedata
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery q(db);

    QString queryStr = "SELECT d.id, d.title, d.description, d.subject, d.category, d.difficulty, "
                       "d.topics, d.tags, d.prerequisites, d.filename, d.filesize, d.filepath, "
                       "d.pagecount, d.ispublic, d.isactive, d.uploadedby, u.username as createdByName, "
                       "d.uploadedat, d.updatedat, d.totaldownloads, d.totalviews, "
                       "d.averagerating, d.ratingcount "
                       "FROM documents d "
                       "LEFT JOIN users u ON d.uploadedby = u.id "
                       "WHERE d.isactive = 1 AND d.ispublic = 1";

    QStringList whereClauses;
    QVariantList bindValues;

    // Add filters
    if (!category.isEmpty()) {
        whereClauses << "LOWER(d.category) = ?";
        bindValues << category.toLower();
    }

    if (!subject.isEmpty()) {
        whereClauses << "LOWER(d.subject) LIKE ?";
        bindValues << QString("%" + subject.toLower() + "%");
    }

    if (!difficulty.isEmpty()) {
        whereClauses << "LOWER(d.difficulty) = ?";
        bindValues << difficulty.toLower();
    }

    // Add WHERE clauses if any
    if (!whereClauses.isEmpty()) {
        queryStr += " AND " + whereClauses.join(" AND ");
    }

    // Add ordering and pagination
    queryStr += " ORDER BY d.uploadedat DESC LIMIT ? OFFSET ?";
    bindValues << limit << offset;

    // Prepare and execute query
    q.prepare(queryStr);
    for (int i = 0; i < bindValues.size(); ++i) {
        q.addBindValue(bindValues[i]);
    }

    if (!q.exec()) {
        qDebug() << "Query error:" << q.lastError().text();
        qDebug() << "Failed query:" << queryStr;
        return createCorsResponse("Failed to retrieve documents",
                                  QHttpServerResponse::StatusCode::InternalServerError);
    }

    // Build response array
    QJsonArray documentsArray;
    while (q.next()) {
        QJsonObject documentObj;

        int documentId = q.value("id").toInt();

        documentObj["id"] = documentId;
        documentObj["title"] = q.value("title").toString();
        documentObj["description"] = q.value("description").toString();
        documentObj["subject"] = q.value("subject").toString();
        documentObj["category"] = q.value("category").toString();

        QString difficultyValue = q.value("difficulty").toString();
        if (!difficultyValue.isEmpty()) {
            documentObj["difficulty"] = difficultyValue;
        }

        // Parse JSON arrays from database
        QString topicsJson = q.value("topics").toString();
        if (!topicsJson.isEmpty()) {
            QJsonDocument topicsDoc = QJsonDocument::fromJson(topicsJson.toUtf8());
            if (topicsDoc.isArray()) {
                documentObj["topics"] = topicsDoc.array();
            } else {
                documentObj["topics"] = QJsonArray();
            }
        } else {
            documentObj["topics"] = QJsonArray();
        }

        QString tagsJson = q.value("tags").toString();
        if (!tagsJson.isEmpty()) {
            QJsonDocument tagsDoc = QJsonDocument::fromJson(tagsJson.toUtf8());
            if (tagsDoc.isArray()) {
                documentObj["tags"] = tagsDoc.array();
            } else {
                documentObj["tags"] = QJsonArray();
            }
        } else {
            documentObj["tags"] = QJsonArray();
        }

        QString prerequisitesJson = q.value("prerequisites").toString();
        if (!prerequisitesJson.isEmpty()) {
            QJsonDocument prerequisitesDoc = QJsonDocument::fromJson(prerequisitesJson.toUtf8());
            if (prerequisitesDoc.isArray()) {
                documentObj["prerequisites"] = prerequisitesDoc.array();
            } else {
                documentObj["prerequisites"] = QJsonArray();
            }
        } else {
            documentObj["prerequisites"] = QJsonArray();
        }

        documentObj["fileName"] = q.value("filename").toString();
        documentObj["fileSize"] = q.value("filesize").toLongLong();
        QString filePath = q.value("filepath").toString();
        documentObj["filePath"] = filePath;
        documentObj["pdfUrl"] = QString("/api/exams/%1/pdf").arg(documentId);

        documentObj["pageCount"] = q.value("pagecount").toInt();
        documentObj["isPublic"] = q.value("ispublic").toBool();
        documentObj["isActive"] = q.value("isactive").toBool();
        documentObj["createdBy"] = q.value("uploadedby").toInt();
        documentObj["createdByName"] = q.value("createdByName").toString();
        documentObj["createdAt"] = q.value("uploadedat").toString();
        documentObj["updatedAt"] = q.value("updatedat").toString();
        documentObj["totalDownloads"] = q.value("totaldownloads").toInt();
        documentObj["totalViews"] = q.value("totalviews").toInt();
        documentObj["averageRating"] = q.value("averagerating").toDouble();
        documentObj["ratingCount"] = q.value("ratingcount").toInt();
        documentObj["additionalFiles"] = QJsonArray(); // Empty array for now

        documentsArray.append(documentObj);
    }

    return createCorsResponse(documentsArray, QHttpServerResponse::StatusCode::Ok);
}

QHttpServerResponse ExamsRoutes::getExam(const QHttpServerRequest &request, const QString &id) {
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

    // Validate and convert ID parameter
    bool ok;
    int documentId = id.toInt(&ok);
    if (!ok || documentId <= 0) {
        return createCorsResponse("Invalid document ID", QHttpServerResponse::StatusCode::BadRequest);
    }

    // Query database for specific document - UPDATED to use filepath instead of filedata
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery q(db);

    q.prepare("SELECT d.id, d.title, d.description, d.subject, d.category, d.difficulty, "
              "d.topics, d.tags, d.prerequisites, d.filename, d.filesize, d.filepath, "
              "d.pagecount, d.ispublic, d.isactive, d.uploadedby, u.username as createdByName, "
              "d.uploadedat, d.updatedat, d.totaldownloads, d.totalviews, "
              "d.averagerating, d.ratingcount "
              "FROM documents d "
              "LEFT JOIN users u ON d.uploadedby = u.id "
              "WHERE d.id = ? AND d.isactive = 1");

    q.addBindValue(documentId);

    if (!q.exec()) {
        qDebug() << "Query error:" << q.lastError().text();
        return createCorsResponse("Database error", QHttpServerResponse::StatusCode::InternalServerError);
    }

    // Check if document exists
    if (!q.next()) {
        return createCorsResponse("Document not found", QHttpServerResponse::StatusCode::NotFound);
    }

    // Get current user ID for access control - FIXED to use consistent field name
    int currentUserId = authorize["id"].toInt();  // Changed from "user_id" to "id"
    bool isDocumentPublic = q.value("ispublic").toBool();
    int documentOwnerId = q.value("uploadedby").toInt();

    // Check access permissions: document must be public OR user must be the owner
    if (!isDocumentPublic && currentUserId != documentOwnerId) {
        return createCorsResponse("Access denied", QHttpServerResponse::StatusCode::Forbidden);
    }

    // Build document object
    QJsonObject documentObj;

    documentObj["id"] = q.value("id").toInt();
    documentObj["title"] = q.value("title").toString();
    documentObj["description"] = q.value("description").toString();
    documentObj["subject"] = q.value("subject").toString();
    documentObj["category"] = q.value("category").toString();

    QString difficultyValue = q.value("difficulty").toString();
    if (!difficultyValue.isEmpty()) {
        documentObj["difficulty"] = difficultyValue;
    }

    // Parse JSON arrays from database
    QString topicsJson = q.value("topics").toString();
    if (!topicsJson.isEmpty()) {
        QJsonDocument topicsDoc = QJsonDocument::fromJson(topicsJson.toUtf8());
        if (topicsDoc.isArray()) {
            documentObj["topics"] = topicsDoc.array();
        } else {
            documentObj["topics"] = QJsonArray();
        }
    } else {
        documentObj["topics"] = QJsonArray();
    }

    QString tagsJson = q.value("tags").toString();
    if (!tagsJson.isEmpty()) {
        QJsonDocument tagsDoc = QJsonDocument::fromJson(tagsJson.toUtf8());
        if (tagsDoc.isArray()) {
            documentObj["tags"] = tagsDoc.array();
        } else {
            documentObj["tags"] = QJsonArray();
        }
    } else {
        documentObj["tags"] = QJsonArray();
    }

    QString prerequisitesJson = q.value("prerequisites").toString();
    if (!prerequisitesJson.isEmpty()) {
        QJsonDocument prerequisitesDoc = QJsonDocument::fromJson(prerequisitesJson.toUtf8());
        if (prerequisitesDoc.isArray()) {
            documentObj["prerequisites"] = prerequisitesDoc.array();
        } else {
            documentObj["prerequisites"] = QJsonArray();
        }
    } else {
        documentObj["prerequisites"] = QJsonArray();
    }

    documentObj["fileName"] = q.value("filename").toString();
    documentObj["fileSize"] = q.value("filesize").toLongLong();

    // REMOVED: No longer include PDF content in response for security and performance
    // documentObj["pdfContent"] = q.value("filedata").toString();

    // ADDED: Include file path and PDF URL instead
    QString filePath = q.value("filepath").toString();
    documentObj["filePath"] = filePath;
    documentObj["pdfUrl"] = QString("/api/exams/%1/pdf").arg(documentId);

    documentObj["pageCount"] = q.value("pagecount").toInt();
    documentObj["isPublic"] = q.value("ispublic").toBool();
    documentObj["isActive"] = q.value("isactive").toBool();
    documentObj["createdBy"] = q.value("uploadedby").toInt();
    documentObj["createdByName"] = q.value("createdByName").toString();
    documentObj["createdAt"] = q.value("uploadedat").toString();
    documentObj["updatedAt"] = q.value("updatedat").toString();
    documentObj["totalDownloads"] = q.value("totaldownloads").toInt();
    documentObj["totalViews"] = q.value("totalviews").toInt();
    documentObj["averageRating"] = q.value("averagerating").toDouble();
    documentObj["ratingCount"] = q.value("ratingcount").toInt();
    documentObj["additionalFiles"] = QJsonArray(); // Empty array for now

    // Increment view count (optional - you might want to do this in a separate endpoint)
    QSqlQuery updateViewsQuery(db);
    updateViewsQuery.prepare("UPDATE documents SET totalviews = totalviews + 1 WHERE id = ?");
    updateViewsQuery.addBindValue(documentId);
    updateViewsQuery.exec(); // Don't fail if this doesn't work

    return createCorsResponse(documentObj, QHttpServerResponse::StatusCode::Ok);
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
    bool isActive = documentData["isActive"].toBool(true);

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

    // Decode base64 content
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
    int createdById = authorize["id"].toInt();
    QString createdBy = authorize["email"].toString();

    // Generate unique ID
    int documentId = QDateTime::currentMSecsSinceEpoch() % 1000000;

    // CREATE UPLOADS DIRECTORY AND SAVE FILE
    QString uploadsDir = QCoreApplication::applicationDirPath() + "/uploads/exams/";
    QDir dir;
    if (!dir.exists(uploadsDir)) {
        if (!dir.mkpath(uploadsDir)) {
            return createCorsResponse("Failed to create uploads directory",
                                      QHttpServerResponse::StatusCode::InternalServerError);
        }
    }

    // Generate unique filename
    QString timestamp = QString::number(QDateTime::currentMSecsSinceEpoch());
    QString sanitizedFileName = fileName;
    // Remove any path separators and special characters
    sanitizedFileName.replace(QRegularExpression("[/\\\\:*?\"<>|]"), "_");
    QString uniqueFileName = QString("exam_%1_%2_%3")
                                 .arg(documentId)
                                 .arg(timestamp)
                                 .arg(sanitizedFileName);

    QString filePath = uploadsDir + uniqueFileName;
    QString relativeFilePath = QString("uploads/exams/%1").arg(uniqueFileName);

    // Save file to disk
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qDebug() << "Failed to open file for writing:" << filePath;
        return createCorsResponse("Failed to save PDF file",
                                  QHttpServerResponse::StatusCode::InternalServerError);
    }

    qint64 bytesWritten = file.write(fileData);
    file.close();

    if (bytesWritten != fileData.size()) {
        // Clean up partial file
        QFile::remove(filePath);
        return createCorsResponse("Failed to write complete PDF file",
                                  QHttpServerResponse::StatusCode::InternalServerError);
    }

    qDebug() << "PDF file saved successfully:" << filePath;

    // Get current timestamp
    QDateTime now = QDateTime::currentDateTime();
    QString createdAt = now.toString(Qt::ISODate);

    // Insert into database - MODIFIED TO STORE FILE PATH INSTEAD OF BASE64
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery q(db);

    // Updated query to store file path instead of base64 content
    q.prepare("INSERT INTO documents (id, title, description, subject, category, difficulty, "
              "topics, tags, prerequisites, filename, filesize, filepath, pagecount, ispublic, "
              "isactive, uploadedby, uploadedat, updatedat, totaldownloads, totalviews, "
              "averagerating, ratingcount, filehash) "
              "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");

    q.addBindValue(documentId);
    q.addBindValue(title);
    q.addBindValue(description);
    q.addBindValue(subject);
    q.addBindValue(category);
    q.addBindValue(difficulty);
    q.addBindValue(QJsonDocument(topicsArray).toJson(QJsonDocument::Compact));
    q.addBindValue(QJsonDocument(tagsArray).toJson(QJsonDocument::Compact));
    q.addBindValue(QJsonDocument(prerequisitesArray).toJson(QJsonDocument::Compact));
    q.addBindValue(fileName);
    q.addBindValue(fileSize);
    q.addBindValue(relativeFilePath); // Store file path instead of base64
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
        // Clean up file if database insert fails
        QFile::remove(filePath);
        return createCorsResponse("Failed to save document",
                                  QHttpServerResponse::StatusCode::InternalServerError);
    }

    // Create response object - NO LONGER INCLUDE PDF CONTENT
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
    responseJson["filePath"] = relativeFilePath; // Include file path
    responseJson["pdfUrl"] = QString("/api/exams/%1/pdf").arg(documentId); // Include PDF URL
    responseJson["pageCount"] = pageCount;
    responseJson["isPublic"] = isPublic;
    responseJson["isActive"] = isActive;
    responseJson["createdBy"] = createdById;
    responseJson["createdByName"] = createdBy;
    responseJson["createdAt"] = createdAt;
    responseJson["updatedAt"] = createdAt;
    responseJson["totalDownloads"] = 0;
    responseJson["totalViews"] = 0;
    responseJson["averageRating"] = 0;
    responseJson["ratingCount"] = 0;
    responseJson["additionalFiles"] = QJsonArray();

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
