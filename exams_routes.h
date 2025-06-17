#ifndef EXAMS_ROUTES_H
#define EXAMS_ROUTES_H

#include <QHttpServer>
#include <QHttpServerRequest>
#include <QHttpServerResponse>
#include <QJsonDocument>
#include <QJsonObject>
#include "response_utils.h"

class ExamsRoutes {
public:
    static void setupRoutes(QHttpServer* server);

private:
    // GET /exams
    static QHttpServerResponse getExams(const QHttpServerRequest &request);

    // GET /exams/id
    static QHttpServerResponse getExam(const QHttpServerRequest &request,
                                       const QString &id);

    // POST /exams/id/start
    static QHttpServerResponse startExam(const QHttpServerRequest &request,
                                         const QString &id);

    // POST /exam-sessions/sessionId/submit
    static QHttpServerResponse submitExamSession(const QHttpServerRequest &request,
                                                 const QString &sessionId);

    // GET /exam-attempts
    static QHttpServerResponse getExamAttempts(const QHttpServerRequest &request);

    // POST /exam-attempts/attemptId/finish
    static QHttpServerResponse finishExamAttempt(const QHttpServerRequest &request,
                                                 const QString &attemptId);

    // POST /exam-attempts/attemptId/answers
    static QHttpServerResponse submitExamAnswers(const QHttpServerRequest &request,
                                                 const QString &attemptId);

    // GET /exams/stats
    static QHttpServerResponse getExamStats(const QHttpServerRequest &request);
};

#endif // EXAMS_ROUTES_H
