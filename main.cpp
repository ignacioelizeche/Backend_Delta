#include <QCoreApplication>
#include "api.h"
#include "database_manager.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    database_manager::instance(); // Esto inicializa la DB
    API api;                      // Arranca el servidor al crear la instancia
    return app.exec();
}
