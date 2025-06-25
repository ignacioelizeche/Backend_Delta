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
