#ifndef EXAMS_ROUTES_H
#define EXAMS_ROUTES_H

#include <QBuffer>
#include <QByteArray>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QHttpServer>
#include <QHttpServerRequest>
#include <QHttpServerResponse>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMimeDatabase>
#include <QMimeType>
#include <QSql>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>
#include <QVariant>
#include "jwt_helper.h"
#include "response_utils.h"

class ExamsRoutes
{
public:
    static void setupRoutes(QHttpServer *server);

private:
    static QHttpServerResponse getExams(const QHttpServerRequest &request);
    static QHttpServerResponse getExam(const QHttpServerRequest &request, const QString &id);
    static QHttpServerResponse uploadExam(const QHttpServerRequest &request);
    static QHttpServerResponse viewExam(const QHttpServerRequest &request, const QString &id);

    static QJsonArray parseStringToJsonArray(const QString &str);
    static QString calculateFileHash(const QByteArray &data);
    static int estimatePDFPageCount(const QByteArray &pdfData);
};

#endif // EXAMS_ROUTES_H
