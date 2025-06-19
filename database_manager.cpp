#include "database_manager.h"
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>

database_manager& database_manager::instance() {
    static database_manager instance;
    return instance;
}

QSqlDatabase& database_manager::database() {
    return m_db;
}

database_manager::database_manager() {
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName("delta.db");
    if (!m_db.open()) {
        qDebug() << "Failed to open database:" << m_db.lastError().text();
    } else {
        qDebug() << "Database opened successfully.";
    }

    QSqlQuery query(m_db);
    query.exec("CREATE TABLE IF NOT EXISTS users ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT, "
               "username TEXT UNIQUE, "
               "email TEXT UNIQUE, "
               "password TEXT, "
               "role TEXT DEFAULT 'student', "
               "coinBalance INTEGER DEFAULT 0, "
               "xpPoints INTEGER DEFAULT 0, "
               "level INTEGER DEFAULT 1, "
               "streak INTEGER DEFAULT 0, "
               "totalProblemsCompleted INTEGER DEFAULT 0, "
               "lastLoginDate DATETIME DEFAULT CURRENT_TIMESTAMP, "
               "joinDate DATETIME DEFAULT CURRENT_TIMESTAMP"
               ")");
    query.exec("CREATE TABLE IF NOT EXISTS calendar ("
                "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                "title VARCHAR(200) NOT NULL,"
                "description TEXT,"
                "eventType VARCHAR(50) NOT NULL,"
                "startTime TIMESTAMP NOT NULL,"
                "endTime TIMESTAMP NOT NULL,"
                "attendees TEXT DEFAULT '',"
                "priority VARCHAR(20) DEFAULT 'medium',"
                "color VARCHAR(7) DEFAULT '#3b82f6',"
                "createdBy INTEGER REFERENCES users(id),"
                "createdAt TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
                "updatedAt TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
                ")");
    query.exec("CREATE TABLE IF NOT EXISTS user_problem_solutions ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT, "
               "user_id INTEGER NOT NULL, "
               "problem_id INTEGER NOT NULL, "
               "is_correct BOOLEAN NOT NULL DEFAULT 0, "
               "solved_at DATETIME NOT NULL, "
               "points_earned INTEGER NOT NULL DEFAULT 0, "
               "UNIQUE(user_id, problem_id), "
               "FOREIGN KEY (user_id) REFERENCES users(id)"
               ")");
    // Tables for the full leaderboard system:
    query.exec("CREATE TABLE IF NOT EXISTS user_activity_log ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT, "
               "user_id INTEGER NOT NULL, "
               "activity_type TEXT NOT NULL, "
               "points_earned INTEGER NOT NULL, "
               "details TEXT, "
               "created_at DATETIME DEFAULT CURRENT_TIMESTAMP, "
               "FOREIGN KEY (user_id) REFERENCES users(id)"
               ")");

    query.exec("CREATE TABLE IF NOT EXISTS achievements ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT, "
               "name TEXT UNIQUE NOT NULL, "
               "description TEXT, "
               "points INTEGER DEFAULT 0, "
               "icon TEXT"
               ")");

    query.exec("CREATE TABLE IF NOT EXISTS user_achievements ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT, "
               "user_id INTEGER NOT NULL, "
               "achievement_id INTEGER NOT NULL, "
               "earned_at DATETIME DEFAULT CURRENT_TIMESTAMP, "
               "UNIQUE(user_id, achievement_id), "
               "FOREIGN KEY (user_id) REFERENCES users(id), "
               "FOREIGN KEY (achievement_id) REFERENCES achievements(id)"
               ")");

    query.exec("CREATE TABLE IF NOT EXISTS leaderboard_history ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT, "
               "user_id INTEGER NOT NULL, "
               "rank_position INTEGER NOT NULL, "
               "points INTEGER NOT NULL, "
               "timeframe TEXT NOT NULL, "
               "recorded_at DATETIME DEFAULT CURRENT_TIMESTAMP, "
               "FOREIGN KEY (user_id) REFERENCES users(id)"
               ")");
    /*
     UNDER REVISION
If it's neceserary it would be put later as QSql code
ALTER TABLE users ADD COLUMN weeklyPoints INTEGER DEFAULT 0;
ALTER TABLE users ADD COLUMN monthlyPoints INTEGER DEFAULT 0;
ALTER TABLE users ADD COLUMN badge TEXT DEFAULT 'Bronze';
ALTER TABLE users ADD COLUMN badgeColor TEXT DEFAULT '#cd7f32';
ALTER TABLE users ADD COLUMN forumContributions INTEGER DEFAULT 0;
    */

}
