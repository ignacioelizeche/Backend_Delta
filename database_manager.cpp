#include "database_manager.h"

database_manager &database_manager::instance()
{
    static database_manager instance;
    return instance;
}

QSqlDatabase &database_manager::database()
{
    return m_db;
}

database_manager::database_manager()
{
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName("delta.db");
    if (!m_db.open()) {
        qDebug() << "Failed to open database:" << m_db.lastError().text();
    } else {
        qDebug() << "Database opened successfully.";
    }

    QSqlQuery query(m_db);

    // Create users table
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

    // Create calendar table
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

    // Create problems table
    query.exec("CREATE TABLE IF NOT EXISTS problems ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT,"
               "title VARCHAR(50) NOT NULL,"
               "description TEXT,"
               "difficulty VARCHAR(20) NOT NULL,"
               "topic VARCHAR(50) NOT NULL,"
               "pointValue INTEGER DEFAULT 10,"
               "xpValue INTEGER DEFAULT 25,"
               "estimatedTime INTEGER DEFAULT 180,"
               "tags TEXT DEFAULT '',"
               "concepts TEXT DEFAULT '',"
               "type TEXT DEFAULT '',"
               "timeLimit INTEGER DEFAULT 240,"
               "correctAnswer VARCHAR(200) DEFAULT '',"
               "explanation VARCHAR(200) DEFAULT '',"
               "createdAt TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
               ")");


    // Create user_activity_log table for tracking all user activities
    if (!query.exec("CREATE TABLE IF NOT EXISTS user_activity_log ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                    "user_id INTEGER NOT NULL,"
                    "activity_type VARCHAR(50) NOT NULL,"
                    "details TEXT DEFAULT '',"
                    "points_earned INTEGER DEFAULT 0,"
                    "xp_earned INTEGER DEFAULT 0,"
                    "coins_earned INTEGER DEFAULT 0,"
                    "created_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
                    "FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE"
                    ")")) {
        qDebug() << "Error creating user_activity_log table:" << query.lastError().text();
    } else {
        qDebug() << "user_activity_log table created successfully.";
    }

    // Create leaderboard_history table for storing historical rankings
    if (!query.exec("CREATE TABLE IF NOT EXISTS leaderboard_history ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                    "user_id INTEGER NOT NULL,"
                    "rank_position INTEGER NOT NULL,"
                    "points INTEGER DEFAULT 0,"
                    "xp_points INTEGER DEFAULT 0,"
                    "problems_completed INTEGER DEFAULT 0,"
                    "timeframe VARCHAR(20) NOT NULL,"
                    "recorded_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
                    "FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE"
                    ")")) {
        qDebug() << "Error creating leaderboard_history table:" << query.lastError().text();
    } else {
        qDebug() << "leaderboard_history table created successfully.";
    }

    //Table for exams documents
    query.exec("DROP TABLE IF EXISTS documents");
    query.exec("CREATE TABLE IF NOT EXISTS documents ("
               "id INTEGER PRIMARY KEY,"
               "title TEXT NOT NULL,"
               "description TEXT,"
               "subject TEXT,"
               "category TEXT,"
               "difficulty TEXT,"
               "topics TEXT,"
               "tags TEXT,"
               "prerequisites TEXT,"
               "filename TEXT NOT NULL,"
               "filesize INTEGER NOT NULL,"
               "filepath TEXT NOT NULL,"
               "pagecount INTEGER,"
               "ispublic BOOLEAN DEFAULT 1,"
               "isactive BOOLEAN DEFAULT 1,"
               "uploadedby INTEGER NOT NULL,"
               "uploadedat DATETIME DEFAULT CURRENT_TIMESTAMP,"
               "updatedat DATETIME DEFAULT CURRENT_TIMESTAMP,"
               "totaldownloads INTEGER DEFAULT 0,"
               "totalviews INTEGER DEFAULT 0,"
               "averagerating DECIMAL(3,2) DEFAULT 0.00,"
               "ratingcount INTEGER DEFAULT 0,"
               "filehash TEXT,"
               "FOREIGN KEY (uploadedby) REFERENCES users(id)"
               ")");

    // Create problem_attempts table
    createProblemAttemptsTable();
}

void database_manager::createProblemAttemptsTable()
{
    QSqlQuery query(m_db);

    // Create the problem_attempts table
    bool success = query.exec("CREATE TABLE IF NOT EXISTS problem_attempts ("
                              "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                              "userId INTEGER NOT NULL,"
                              "problemId INTEGER NOT NULL,"
                              "answer TEXT NOT NULL,"
                              "correct BOOLEAN NOT NULL,"
                              "xpEarned INTEGER DEFAULT 0,"
                              "coinsEarned INTEGER DEFAULT 0,"
                              "timestamp DATETIME NOT NULL,"
                              "createdAt DATETIME DEFAULT CURRENT_TIMESTAMP,"
                              "FOREIGN KEY (userId) REFERENCES users(id) ON DELETE CASCADE,"
                              "FOREIGN KEY (problemId) REFERENCES problems(id) ON DELETE CASCADE"
                              ")");

    if (!success) {
        qDebug() << "Error creating problem_attempts table:" << query.lastError().text();
        return;
    }

    // Create indexes for better performance
    query.exec("CREATE INDEX IF NOT EXISTS idx_attempts_userId ON problem_attempts(userId)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_attempts_problemId ON problem_attempts(problemId)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_attempts_timestamp ON problem_attempts(timestamp)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_attempts_correct ON problem_attempts(correct)");

    qDebug() << "problem_attempts table created successfully";
}
