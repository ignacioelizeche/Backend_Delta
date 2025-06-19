#include "leaderboard_services.h"
#include "database_manager.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>

LeaderboardService::LeaderboardService(QObject *parent) : QObject(parent) {
    // Initialize point values
    m_pointValues["problem_easy"] = 10;
    m_pointValues["problem_medium"] = 20;
    m_pointValues["problem_hard"] = 30;
    m_pointValues["forum_post"] = 5;
    m_pointValues["forum_helpful"] = 15;
    m_pointValues["streak_bonus"] = 5;
    m_pointValues["daily_login"] = 2;
}

bool LeaderboardService::awardPoints(int userId, const QString& activityType,
                                     int customPoints, const QString& details) {
    QSqlDatabase& db = database_manager::instance().database();

    if (!db.isOpen()) {
        qDebug() << "Database is not open";
        return false;
    }

    QSqlQuery query(db);

    // Determine points to award
    int points = customPoints > 0 ? customPoints : m_pointValues.value(activityType, 0);

    if (points <= 0) {
        qDebug() << "No points defined for activity type:" << activityType;
        return false;
    }

    // Start transaction
    db.transaction();

    // Update user XP points and coins
    if (!updateUserPoints(userId, points)) {
        db.rollback();
        return false;
    }

    // Check for level up
    checkLevelUp(userId);

    // Update streak if it's a daily activity
    if (activityType == "daily_login" || activityType.startsWith("problem")) {
        updateUserStreak(userId);
    }

    db.commit();
    return true;
}

bool LeaderboardService::updateUserPoints(int userId, int points) {
    QSqlDatabase& db = database_manager::instance().database();
    QSqlQuery query(db);

    // Update XP points and coin balance
    query.prepare(R"(
        UPDATE users
        SET xpPoints = xpPoints + ?,
            coinBalance = coinBalance + ?
        WHERE id = ?
    )");
    query.addBindValue(points);
    query.addBindValue(points / 2); // Coins = half of points
    query.addBindValue(userId);

    if (!query.exec()) {
        qDebug() << "Failed to update user points:" << query.lastError().text();
        return false;
    }

    return true;
}

void LeaderboardService::checkLevelUp(int userId) {
    QSqlDatabase& db = database_manager::instance().database();
    QSqlQuery query(db);

    // Get current user stats
    query.prepare("SELECT xpPoints, level FROM users WHERE id = ?");
    query.addBindValue(userId);

    if (!query.exec() || !query.next()) {
        return;
    }

    int currentXP = query.value("xpPoints").toInt();
    int currentLevel = query.value("level").toInt();

    // Calculate new level (100 XP points per level)
    int newLevel = (currentXP / 100) + 1;

    if (newLevel > currentLevel) {
        // Update level
        QSqlQuery updateQuery(db);
        updateQuery.prepare("UPDATE users SET level = ? WHERE id = ?");
        updateQuery.addBindValue(newLevel);
        updateQuery.addBindValue(userId);
        updateQuery.exec();

        qDebug() << "User" << userId << "leveled up to level" << newLevel;

        // Award bonus coins for leveling up
        updateUserCoins(userId, newLevel * 10); // 10 coins per level reached
    }
}

bool LeaderboardService::updateUserCoins(int userId, int coins) {
    QSqlDatabase& db = database_manager::instance().database();
    QSqlQuery query(db);

    query.prepare("UPDATE users SET coinBalance = coinBalance + ? WHERE id = ?");
    query.addBindValue(coins);
    query.addBindValue(userId);

    if (!query.exec()) {
        qDebug() << "Failed to update user coins:" << query.lastError().text();
        return false;
    }

    return true;
}

void LeaderboardService::updateUserStreak(int userId) {
    QSqlDatabase& db = database_manager::instance().database();
    QSqlQuery query(db);

    // Get user's last login date
    query.prepare("SELECT lastLoginDate, streak FROM users WHERE id = ?");
    query.addBindValue(userId);

    if (!query.exec() || !query.next()) {
        return;
    }

    QDateTime lastLogin = query.value("lastLoginDate").toDateTime();
    int currentStreak = query.value("streak").toInt();
    QDateTime now = QDateTime::currentDateTime();

    // Check if last login was yesterday (streak continues) or today (no change needed)
    QDate lastLoginDate = lastLogin.date();
    QDate today = now.date();

    int daysDifference = lastLoginDate.daysTo(today);

    if (daysDifference == 1) {
        // Consecutive day - increase streak
        currentStreak++;

        // Award streak bonus every 5 days
        if (currentStreak % 5 == 0) {
            awardPoints(userId, "streak_bonus", 0, QString("Streak milestone: %1 days").arg(currentStreak));
        }
    } else if (daysDifference > 1) {
        // Streak broken - reset to 1
        currentStreak = 1;
    }
    // If daysDifference == 0, it's the same day, no streak change needed

    // Update streak and last login date
    QSqlQuery updateQuery(db);
    updateQuery.prepare("UPDATE users SET streak = ?, lastLoginDate = ? WHERE id = ?");
    updateQuery.addBindValue(currentStreak);
    updateQuery.addBindValue(now);
    updateQuery.addBindValue(userId);
    updateQuery.exec();
}

bool LeaderboardService::awardProblemPoints(int userId, int problemId, const QString& difficulty) {
    QSqlDatabase& db = database_manager::instance().database();

    if (!db.isOpen()) {
        qDebug() << "Database is not open";
        return false;
    }

    db.transaction();

    // Check if this problem was already solved by this user
    QSqlQuery checkQuery(db);
    checkQuery.prepare("SELECT id FROM user_problem_solutions WHERE user_id = ? AND problem_id = ?");
    checkQuery.addBindValue(userId);
    checkQuery.addBindValue(problemId);

    if (checkQuery.exec() && checkQuery.next()) {
        qDebug() << "Problem already solved by user" << userId;
        db.rollback();
        return false;
    }

    // Check if this is the first solver (count existing solutions for this problem)
    QSqlQuery firstSolverQuery(db);
    firstSolverQuery.prepare("SELECT COUNT(*) as solver_count FROM user_problem_solutions WHERE problem_id = ? AND is_correct = 1");
    firstSolverQuery.addBindValue(problemId);

    bool isFirstSolver = false;
    if (firstSolverQuery.exec() && firstSolverQuery.next()) {
        int solverCount = firstSolverQuery.value("solver_count").toInt();
        isFirstSolver = (solverCount == 0);
    }

    // Get base points for this difficulty
    QString activityType = "problem_" + difficulty.toLower();
    int basePoints = m_pointValues.value(activityType, 0);

    if (basePoints <= 0) {
        qDebug() << "No points defined for difficulty:" << difficulty;
        db.rollback();
        return false;
    }

    // Calculate final points (10% bonus for first solver)
    int finalPoints = basePoints;
    if (isFirstSolver) {
        finalPoints = basePoints + (basePoints * 0.1); // 10% bonus
    }

    // Record the solution in user_problem_solutions table
    QSqlQuery solutionQuery(db);
    solutionQuery.prepare(R"(
        INSERT INTO user_problem_solutions (user_id, problem_id, is_correct, solved_at, points_earned)
        VALUES (?, ?, 1, ?, ?)
    )");
    solutionQuery.addBindValue(userId);
    solutionQuery.addBindValue(problemId);
    solutionQuery.addBindValue(QDateTime::currentDateTime());
    solutionQuery.addBindValue(finalPoints);

    if (!solutionQuery.exec()) {
        qDebug() << "Failed to record solution:" << solutionQuery.lastError().text();
        db.rollback();
        return false;
    }

    // Update user's total problems completed
    QSqlQuery updateStatsQuery(db);
    updateStatsQuery.prepare("UPDATE users SET totalProblemsCompleted = totalProblemsCompleted + 1 WHERE id = ?");
    updateStatsQuery.addBindValue(userId);

    if (!updateStatsQuery.exec()) {
        qDebug() << "Failed to update problem stats:" << updateStatsQuery.lastError().text();
        db.rollback();
        return false;
    }

    // Update user points
    if (!updateUserPoints(userId, finalPoints)) {
        db.rollback();
        return false;
    }

    // Check for level up
    checkLevelUp(userId);

    // Update streak
    updateUserStreak(userId);

    db.commit();

    qDebug() << "Points awarded to user" << userId << ":" << finalPoints
             << (isFirstSolver ? "(with first solver bonus)" : "");

    return true;
}

QJsonObject LeaderboardService::getUserStats(int userId) {
    QSqlDatabase& db = database_manager::instance().database();
    QSqlQuery query(db);

    query.prepare(R"(
        SELECT username, xpPoints, level, streak, coinBalance, totalProblemsCompleted,
               joinDate, lastLoginDate
        FROM users
        WHERE id = ?
    )");
    query.addBindValue(userId);

    QJsonObject stats;
    if (query.exec() && query.next()) {
        stats["username"] = query.value("username").toString();
        stats["xpPoints"] = query.value("xpPoints").toInt();
        stats["level"] = query.value("level").toInt();
        stats["streak"] = query.value("streak").toInt();
        stats["coinBalance"] = query.value("coinBalance").toInt();
        stats["totalProblemsCompleted"] = query.value("totalProblemsCompleted").toInt();
        stats["joinDate"] = query.value("joinDate").toString();
        stats["lastLoginDate"] = query.value("lastLoginDate").toString();

        // Calculate progress to next level
        int currentXP = query.value("xpPoints").toInt();
        int currentLevel = query.value("level").toInt();
        int xpForCurrentLevel = (currentLevel - 1) * 100;
        int xpForNextLevel = currentLevel * 100;
        int progressToNextLevel = currentXP - xpForCurrentLevel;

        stats["progressToNextLevel"] = progressToNextLevel;
        stats["xpNeededForNextLevel"] = 100 - progressToNextLevel;
    }

    return stats;
}

QJsonArray LeaderboardService::getLeaderboard(const QString& sortBy, int limit) {
    QSqlDatabase& db = database_manager::instance().database();
    QSqlQuery query(db);

    QString orderColumn = "xpPoints"; // Default sorting
    if (sortBy == "level") {
        orderColumn = "level";
    } else if (sortBy == "streak") {
        orderColumn = "streak";
    } else if (sortBy == "problems") {
        orderColumn = "totalProblemsCompleted";
    }

    QString queryStr = QString(R"(
        SELECT username, xpPoints, level, streak, totalProblemsCompleted, coinBalance,
               ROW_NUMBER() OVER (ORDER BY %1 DESC, xpPoints DESC) as rank
        FROM users
        ORDER BY %1 DESC, xpPoints DESC
        LIMIT ?
    )").arg(orderColumn);

    query.prepare(queryStr);
    query.addBindValue(limit > 0 ? limit : 100);

    QJsonArray leaderboard;
    if (query.exec()) {
        while (query.next()) {
            QJsonObject user;
            user["rank"] = query.value("rank").toInt();
            user["username"] = query.value("username").toString();
            user["xpPoints"] = query.value("xpPoints").toInt();
            user["level"] = query.value("level").toInt();
            user["streak"] = query.value("streak").toInt();
            user["totalProblemsCompleted"] = query.value("totalProblemsCompleted").toInt();
            user["coinBalance"] = query.value("coinBalance").toInt();

            leaderboard.append(user);
        }
    }

    return leaderboard;
}
