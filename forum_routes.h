#ifndef FORUM_ROUTES_H
#define FORUM_ROUTES_H

#include <QHttpServer>
#include <QHttpServerRequest>
#include <QHttpServerResponse>
#include <QJsonDocument>
#include <QJsonObject>
#include "response_utils.h"

class ForumRoutes {
public:
    static void setupRoutes(QHttpServer* server);

private:
    // GET /forum/posts
    static QHttpServerResponse getPosts(const QHttpServerRequest &request);

    // GET /forum/posts/id
    static QHttpServerResponse getPost(const QHttpServerRequest &request,
                                       const QString &id);

    // POST /forum/posts
    static QHttpServerResponse createPost(const QHttpServerRequest &request);

    // GET /forum/posts/id/comments
    static QHttpServerResponse getPostComments(const QHttpServerRequest &request,
                                               const QString &id);

    // POST /forum/posts/id/comments
    static QHttpServerResponse createComment(const QHttpServerRequest &request,
                                             const QString &id);

    // GET /forum/categories
    static QHttpServerResponse getCategories(const QHttpServerRequest &request);

    // POST /forum/posts/id/vote
    static QHttpServerResponse votePost(const QHttpServerRequest &request,
                                        const QString &id);

    // GET /forum/search
    static QHttpServerResponse searchForum(const QHttpServerRequest &request);

    // GET /forum/stats
    static QHttpServerResponse getForumStats(const QHttpServerRequest &request);
};

#endif // FORUM_ROUTES_H
