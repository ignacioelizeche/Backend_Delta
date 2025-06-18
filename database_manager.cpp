#include "database_manager.h"
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>

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
    query.exec("CREATE TABLE IF NOT EXISTS problems ("
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




}
