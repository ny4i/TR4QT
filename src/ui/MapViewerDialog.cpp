#include "MapViewerDialog.h"
#include "../ui/models/QSOTableModel.h"
#include "../utils/MapDataProvider.h"
#include "../utils/AppSettings.h"
#include <QWebEngineView>
#include <QWebChannel>
#include <QVBoxLayout>
#include <QJsonDocument>
#include <QFile>

using TR4QT::QSOTableModel;
using TR4QT::AppSettings;

// MapViewerBridge implementation

MapViewerBridge::MapViewerBridge(QSOTableModel* qsoModel, MapViewerDialog::MapType mapType, QObject* parent)
    : QObject(parent)
    , m_qsoModel(qsoModel)
    , m_mapType(mapType)
{
    AppSettings& settings = AppSettings::instance();
    m_callsign = settings.getMyCallsign();
    m_operatorName = settings.getCurrentOperator();
    m_contestName = "General Logging";  // TODO: Get from active contest if available

    // Handle empty values
    if (m_callsign.isEmpty()) m_callsign = "N0CALL";
    if (m_operatorName.isEmpty()) m_operatorName = "Unknown";

    // Connect to model changes for auto-refresh
    if (m_qsoModel) {
        connect(m_qsoModel, &QAbstractItemModel::rowsInserted,
                this, &MapViewerBridge::onModelDataChanged);
        connect(m_qsoModel, &QAbstractItemModel::dataChanged,
                this, &MapViewerBridge::onModelDataChanged);
    }
}

void MapViewerBridge::requestSectionsData() {
    QJsonObject data = MapDataProvider::getWorkedSections(m_qsoModel);
    QJsonDocument doc(data);
    QString jsonString = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
    emit sectionsDataChanged(jsonString);
}

void MapViewerBridge::requestStatesData() {
    QJsonObject data = MapDataProvider::getWorkedStates(m_qsoModel);
    QJsonDocument doc(data);
    QString jsonString = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
    emit statesDataChanged(jsonString);
}

QString MapViewerBridge::getSectionsGeoJSON() {
    QFile file(":/data/arrl_sections.geojson");
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open arrl_sections.geojson";
        return "{}";
    }
    return QString::fromUtf8(file.readAll());
}

QString MapViewerBridge::getStatesGeoJSON() {
    QFile file(":/data/us_states.geojson");
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open us_states.geojson";
        return "{}";
    }
    return QString::fromUtf8(file.readAll());
}

void MapViewerBridge::onModelDataChanged() {
    // Auto-refresh map data when QSO model changes
    switch (m_mapType) {
        case MapViewerDialog::Sections:
            requestSectionsData();
            break;
        case MapViewerDialog::States:
            requestStatesData();
            break;
        case MapViewerDialog::DXCC:
            // Future implementation
            break;
    }
}

// MapViewerDialog implementation

MapViewerDialog::MapViewerDialog(MapType type, QSOTableModel* qsoModel, QWidget* parent)
    : QDialog(parent)
    , m_mapType(type)
    , m_qsoModel(qsoModel)
    , m_webView(nullptr)
    , m_channel(nullptr)
    , m_bridge(nullptr)
{
    setupUi();
    loadMap();
}

MapViewerDialog::~MapViewerDialog() {
    // QObject parent-child relationship handles cleanup
}

void MapViewerDialog::setupUi() {
    // Set dialog size and title
    resize(1200, 800);

    QString title;
    switch (m_mapType) {
        case Sections:
            title = "ARRL Sections Map";
            break;
        case States:
            title = "US States Map (WAS)";
            break;
        case DXCC:
            title = "DXCC Entities Map";
            break;
    }
    setWindowTitle(title);

    // Create layout
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    // Create web view
    m_webView = new QWebEngineView(this);
    layout->addWidget(m_webView);

    // Create QWebChannel
    m_channel = new QWebChannel(this);
    m_webView->page()->setWebChannel(m_channel);

    // Create and register bridge object (pass map type for auto-refresh)
    m_bridge = new MapViewerBridge(m_qsoModel, m_mapType, this);
    m_channel->registerObject("bridge", m_bridge);
}

void MapViewerDialog::loadMap() {
    QString htmlPath;

    switch (m_mapType) {
        case Sections:
            htmlPath = ":/maps/sections_map_standalone.html";
            break;
        case States:
            htmlPath = ":/maps/states_map_standalone.html";
            break;
        case DXCC:
            // Future: DXCC map
            htmlPath = ":/maps/sections_map_standalone.html";
            break;
    }

    // Load HTML content from Qt resources
    QFile htmlFile(htmlPath);
    if (!htmlFile.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open HTML file:" << htmlPath;
        return;
    }

    QString htmlContent = QString::fromUtf8(htmlFile.readAll());
    htmlFile.close();

    // Use setHtml() with base URL to allow loading resources
    // Base URL of qrc:/maps/ allows the HTML to load leaflet.js, etc.
    m_webView->setHtml(htmlContent, QUrl("qrc:/maps/"));
}
