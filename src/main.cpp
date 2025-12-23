#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include "core/Constants.h"
#include "utils/CountryFile.h"
#include "utils/CountryFileDownloader.h"
#include "utils/AppSettings.h"
#include "radio/RadioInterface.h"
#include "ui/MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // Register custom types for Qt meta-object system (required for queued connections)
    qRegisterMetaType<TR4QT::RadioConfig>("RadioConfig");
    qRegisterMetaType<TR4QT::RadioState>("RadioState");
    qRegisterMetaType<TR4QT::ModeType>("ModeType");
    qRegisterMetaType<TR4QT::VFO>("VFO");

    app.setOrganizationName(TR4QT::APP_ORG);
    app.setApplicationName(TR4QT::APP_NAME);
    app.setApplicationVersion(TR4QT::APP_VERSION);

    qDebug() << "TR4QT Version" << TR4QT::APP_VERSION;

    // Initialize country file on first run
    TR4QT::CountryFile countryFile;
    QString ctyPath = TR4QT::AppSettings::instance().getCountryFilePath();

    if (QFile::exists(ctyPath)) {
        qDebug() << "Loading cty.dat from" << ctyPath;
        if (countryFile.loadFromFile(ctyPath)) {
            qDebug() << "Loaded" << countryFile.getAllCountries().size() << "countries";
        }
    } else {
        qDebug() << "No cty.dat found. Use Tools → Download Country File when implemented.";
    }

    // Create and show main window
    TR4QT::MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}
