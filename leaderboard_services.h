#ifndef LEADERBOARD_SERVICE_H
#define LEADERBOARD_SERVICE_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QMap>
#include <QString>
#include <QDebug>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>

class LeaderboardService : public QObject
{
    Q_OBJECT

public:
    explicit LeaderboardService(QObject *parent = nullptr);

    // Main point awarding function
    bool awardPoints(int userId, const QString& activityType,
                     int customPoints = 0, const QString& details = QString());

    // Point and level management
    bool updateUserPoints(int userId, int points);
    bool updateUserCoins(int userId, int coins);
    void checkLevelUp(int userId);
    void updateUserStreak(int userId);

    // Problem-specific point awarding with first solver bonus
    bool awardProblemPoints(int userId, int problemId, const QString& difficulty);

    // User statistics and leaderboard
    QJsonObject getUserStats(int userId);
    QJsonArray getLeaderboard(const QString& sortBy = "xpPoints", int limit = 100);

private:
    QMap<QString, int> m_pointValues;
};

#endif // LEADERBOARD_SERVICE_H
