#pragma once

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>


class database_manager {
public:
    static database_manager& instance();

    QSqlDatabase& database();

private:
    database_manager();  // solo declaración

    QSqlDatabase m_db;

    // Prevent copies
    database_manager(const database_manager&) = delete;
    database_manager& operator=(const database_manager&) = delete;
};
