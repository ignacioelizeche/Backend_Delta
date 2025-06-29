#ifndef EXAMS_ROUTES_H
#define EXAMS_ROUTES_H

#include <QHttpServer>
#include <QHttpServerRequest>
#include <QHttpServerResponse>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSql>
#include <QSqlDatabase>
#include "response_utils.h"
#include <QSqlQuery>
#include <QJsonArray>
#include "jwt_helper.h"
#include <QSqlError>
#include <QDateTime>
#include <QDebug>
#include <QBuffer>
#include <QByteArray>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QMimeType>
#include <QUuid>
#include <QCryptographicHash>
#include <QVariant>
#include <QCoreApplication>
#include <QDir>


class ExamsRoutes {
public:
    static void setupRoutes(QHttpServer* server);
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
