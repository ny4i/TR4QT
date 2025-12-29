# Geographic Map Support - TODO

## Current Status (v2.98.1)
- ✅ ARRL/RAC Sections Map - Chloropleth with GeoJSON polygons
- ✅ 91 sections with full geographic boundaries
- ✅ Color scheme: Blue (1 QSO) → Red (2) → Green gradients (3-500+)

## Future Enhancements

### 1. US States Map
**Purpose**: Track WAS (Worked All States) progress with visual map

**What We Already Have**:
From ARRL section shapefiles, we have these 39 single-state sections:
- AL, AK, AZ, CO, CT, DE, GA, ID, IL, IN, IA, KS, KY, LA, ME, MI, MN, MS, MO, MT, NE, NV, NH, NM, NC, ND, OH, OK, OR, RI, SC, SD, TN, UT, VT, VA, WI, WV, WY

**Need to Add** (11 states + DC):
- Multi-section states: CA, FL, MA, NJ, NY, PA, TX, WA
- District of Columbia (DC)
- Territories: AS, GU, MP, PR, VI (already have as sections)

**Data Sources**:
1. **Natural Earth Data** (recommended)
   - URL: https://www.naturalearthdata.com/
   - 1:10m Cultural Vectors → Admin 1 – States, Provinces
   - Free, public domain
   - High quality, well-maintained

2. **US Census Bureau TIGER**
   - URL: https://www.census.gov/geographies/mapping-files/time-series/geo/tiger-line-file.html
   - States shapefile
   - Authoritative source
   - More detailed (larger files)

**Implementation**:
- Convert state shapefiles to GeoJSON
- Create `/api/states-geojson` endpoint
- Create `/states-map` page with chloropleth
- Track QSOs per state from QSO.state field
- Color scheme: Same as sections (1=Blue, 2=Red, 3+=Green)

---

### 2. DXCC Entities Map
**Purpose**: Track DXCC progress with world map visualization

**What We Need**:
- World country boundaries for ~340 DXCC entities
- Mapping from countries to DXCC prefixes/entities
- Handle special cases (e.g., Alaska/Hawaii separate from US mainland)

**Data Sources**:
1. **Natural Earth Data** (recommended)
   - URL: https://www.naturalearthdata.com/
   - 1:50m Cultural Vectors → Admin 0 – Countries
   - Includes ~250 countries/territories
   - Free, public domain

2. **DXCC to Country Mapping**:
   - Use existing cty.dat (already in TR4QT)
   - Map DXCC prefix → country code → shapefile
   - Handle multi-DXCC countries (US = K, KH6, KL7, KP2, KP4, etc.)

**Special Cases**:
- United States: Multiple DXCC entities (K, KH6, KL7, KP2, KP4)
- Russia: European Russia vs. Asiatic Russia
- Antarctica: BY1, CE9, KC4, R1AN, etc.
- Oceanic zones: Pacific islands, Caribbean

**Implementation**:
- Convert country shapefiles to GeoJSON
- Create DXCC entity → GeoJSON mapping file
- Create `/api/dxcc-geojson` endpoint
- Create `/dxcc-map` page with chloropleth
- Track QSOs per DXCC from QSO.dxccEntity field
- Color scheme: Same as sections
- Handle zoom levels for different regions (world, continent, country)

---

### 3. Contest-Specific Maps
**Purpose**: Show different maps based on active contest type

**Contest Multiplier Maps**:
- **State QSO Party**: US States map
- **ARRL Sweepstakes**: ARRL Sections map (already done!)
- **CQ WW**: DXCC + CQ Zones overlay
- **ARRL DX**: DXCC entities map
- **NAQP**: US/Canada states/provinces
- **Field Day**: ARRL Sections map (already done!)

**Implementation**:
- Auto-select appropriate map based on contest type
- Add map link to dashboard for active contest
- Show relevant multipliers on map
- Filter by contest when loading worked data

---

## Download Shapefile Data

### Natural Earth Quick Links:
```bash
# Download US States (1:10m)
wget https://www.naturalearthdata.com/http//www.naturalearthdata.com/download/10m/cultural/ne_10m_admin_1_states_provinces.zip

# Download World Countries (1:50m - good for DXCC)
wget https://www.naturalearthdata.com/http//www.naturalearthdata.com/download/50m/cultural/ne_50m_admin_0_countries.zip

# Extract
unzip ne_10m_admin_1_states_provinces.zip -d shapes/states/
unzip ne_50m_admin_0_countries.zip -d shapes/dxcc/
```

### Convert to GeoJSON:
```bash
# Use existing conversion script
python3 scripts/convert_shp_to_geojson.py --input shapes/states/ --output resources/us_states.geojson
python3 scripts/convert_shp_to_geojson.py --input shapes/dxcc/ --output resources/dxcc_entities.geojson
```

---

## Priority

1. **High**: ✅ US States Map (COMPLETED v3.4.0)
   - Most requested feature
   - Data readily available
   - Many states already in hand from ARRL sections
   - Simpler than DXCC (no prefix mapping needed)
   - **Status**: Implemented and committed

2. **High**: In-App Map Viewer Window
   - Create standalone Qt widget to view maps inside TR4QT
   - **Does NOT depend on web server** - works even if server disabled
   - Better UX than opening external browser
   - Two implementation approaches:
     - **Option A**: QWebEngineView loading HTML from Qt resources (qrc://)
     - **Option B**: Native QGraphicsView rendering GeoJSON polygons
   - Add menu items: View → Sections Map, View → States Map
   - Could be dockable window or separate dialog
   - Fetches QSO data directly from QSOTableModel, not via HTTP API

3. **Medium**: DXCC Entities Map
   - More complex (multi-entity countries)
   - Requires DXCC prefix mapping
   - Larger dataset (world coverage)
   - Higher value for DX contesters

4. **Medium**: Static Map Export (JPG/PNG)
   - Generate static image files of maps for sharing/reports
   - Export current state of sections/states/DXCC maps
   - Useful for contest reports, presentations, social media
   - Could use headless browser rendering or server-side map generation
   - File → Export Map as Image...

5. **Low**: Contest-specific auto-switching
   - Nice-to-have feature
   - Depends on having all maps implemented first

---

## Technical Notes

- Keep same chloropleth color scheme across all maps for consistency
- Reuse existing GeoJSON loading/rendering code from sections map
- Store all GeoJSON files in Qt resources for embedded serving
- Consider file size limits (sections GeoJSON is 6.23 MB)
- May need simplified versions of detailed shapefiles to reduce size
- Natural Earth Data 1:50m is good balance of detail vs. file size

---

## Related Files

- `scripts/convert_shp_to_geojson.py` - Already exists, can reuse
- `src/network/WebServer.cpp` - Add new endpoints and map pages
- `resources/resources.qrc` - Add new GeoJSON resources
- `src/models/QSO.h` - Already has state and dxccEntity fields

---

## In-App Map Viewer Implementation Details

**Goal**: Embed maps directly in TR4QT without requiring web server or external browser

**Critical Requirement**: Must work independently of web server (user may have it disabled)

---

### Option A: QWebEngineView with Embedded HTML (Recommended)

**Approach**: Load self-contained HTML from Qt resources

**How It Works**:
1. Create standalone HTML files with embedded CSS/JS (no external URLs)
2. Embed Leaflet JS library in Qt resources
3. Load HTML via `qrc://` URLs instead of `http://`
4. Use Qt/JavaScript bridge to pass QSO data from C++ to JavaScript
5. JavaScript renders map with data from Qt, not HTTP API

**Implementation Steps**:
1. Create `resources/maps/sections_map_standalone.html` (self-contained)
2. Embed Leaflet library in `resources/maps/leaflet.js`
3. Add to `resources.qrc`
4. Create `MapViewerDialog` class with QWebEngineView
5. Load via `view->load(QUrl("qrc:/maps/sections_map_standalone.html"))`
6. Use `QWebChannel` to pass QSO data from C++ to JavaScript
7. Add menu items: View → Sections Map, View → States Map

**Advantages**:
- Reuses existing Leaflet/web map code (already written!)
- No web server dependency
- Self-contained
- Beautiful, interactive maps
- Professional quality (mature Leaflet library)
- Zoom, pan, tooltips all work perfectly
- ~50MB for Qt WebEngine is negligible (standard Qt module)

**Architecture** (Avoid God Class Anti-Pattern):

**CRITICAL**: Do NOT add map logic to MainWindow! Create proper separation:

```cpp
// NEW: Self-contained dialog class
class MapViewerDialog : public QDialog {
    enum MapType { Sections, States, DXCC };
    MapViewerDialog(MapType type, QSOTableModel* qsoModel, QWidget* parent);
    // Owns QWebEngineView, QWebChannel, handles all map logic
};

// NEW: Pure utility class for data transformation
class MapDataProvider {
    static QJsonObject getWorkedSections(QSOTableModel* model);
    static QJsonObject getWorkedStates(QSOTableModel* model);
    // No state, just transforms QSO data to JSON
};

// MainWindow: ONLY creates and shows dialog (2 lines!)
void MainWindow::onShowSectionsMap() {
    auto* dialog = new MapViewerDialog(MapType::Sections, m_qsoModel, this);
    dialog->show();
}
```

**Benefits**:
- ✅ Reduces MainWindow coupling
- ✅ MapViewerDialog is self-contained and reusable
- ✅ MapDataProvider is testable in isolation
- ✅ Clear single responsibility for each class
- ✅ Example of good architecture for future features

**Files to Create/Modify**:
- `resources/maps/sections_map_standalone.html` - Embedded HTML/CSS/JS
- `resources/maps/states_map_standalone.html` - States version
- `resources/maps/leaflet.js` - Leaflet library embedded
- `resources/maps/leaflet.css` - Leaflet styles
- `resources/resources.qrc` - Add map resources
- `src/ui/MapViewerDialog.h` - NEW: Self-contained dialog class
- `src/ui/MapViewerDialog.cpp` - NEW: QWebEngineView + QWebChannel bridge
- `src/utils/MapDataProvider.h` - NEW: Data transformation utility
- `src/utils/MapDataProvider.cpp` - NEW: QSO data → JSON conversion
- `src/ui/MainWindow.cpp` - Add menu actions ONLY (minimal changes)
- `CMakeLists.txt` - Add Qt6::WebEngineWidgets dependency

---

### Option B: Native Qt Graphics (Pure Qt, No WebEngine)

**Approach**: Render GeoJSON polygons directly using Qt graphics

**How It Works**:
1. Parse GeoJSON files using QJsonDocument
2. Convert lat/lon coordinates to screen coordinates (Mercator projection)
3. Create QGraphicsPolygonItem for each state/section
4. Color based on QSO count (chloropleth logic in C++)
5. Use QGraphicsView to display and interact

**Implementation Steps**:
1. Create `MapRenderer` class to parse GeoJSON and convert coordinates
2. Create `MapWidget` inheriting QGraphicsView
3. Render polygons as QGraphicsPolygonItems with appropriate colors
4. Implement hover/click interactions with QGraphicsItem events
5. Add legend and statistics as QGraphicsTextItems
6. Add menu items to show MapWidget

**Pros**:
- No Qt WebEngine dependency (smaller distribution)
- Truly native Qt - faster, lighter weight
- Complete control over rendering
- No JavaScript needed

**Cons**:
- More C++ code to write (projection math, rendering logic)
- Less polished visuals compared to Leaflet
- More work to implement zoom/pan/interactions
- Need to implement chloropleth coloring in C++

**Files to Create/Modify**:
- `src/ui/MapWidget.h` - QGraphicsView-based map widget
- `src/ui/MapWidget.cpp` - GeoJSON parsing and rendering
- `src/utils/MapRenderer.h` - Coordinate projection utilities
- `src/utils/MapRenderer.cpp` - Mercator projection, polygon rendering
- `src/ui/MainWindow.cpp` - Menu actions
- No CMake changes needed (uses existing Qt modules)

---

### Recommendation: Use Option A

Option A is the clear choice - it has no real downsides:
- ✅ Faster to implement (reuse existing map HTML/JS)
- ✅ Professional results (Leaflet is battle-tested)
- ✅ All features work (zoom, pan, tooltips, colors)
- ✅ No web server dependency (works offline)
- ✅ Qt WebEngine is a standard Qt module

Option B would only be needed if targeting embedded systems with severe size constraints, which doesn't apply to TR4QT.

**Implementation Priority**: High - users will love having maps integrated directly in the app

---

## Static Map Export Implementation Details

**Goal**: Generate PNG/JPG images of maps for sharing, reports, or archiving

**Use Cases**:
- Contest reports (include map showing worked states/sections)
- Social media posts (share WAS progress)
- Presentations
- Print-friendly format
- Archive snapshots of progress over time

**Implementation Options**:

### Option 1: Browser Screenshot (Easiest)
- Use QWebEngineView to render map
- Call `QWebEngineView::grab()` to capture as QPixmap
- Save as PNG/JPG
- **Pros**: Simple, reuses existing rendering
- **Cons**: Requires Qt WebEngine

### Option 2: Headless Browser (Puppeteer/Playwright)
- Use Node.js with Puppeteer to render and screenshot
- Call from Qt via QProcess
- **Pros**: High quality, supports all web features
- **Cons**: External dependency

### Option 3: Server-Side Rendering
- Python library like `matplotlib` + `geopandas` or `folium`
- Generate map image directly from GeoJSON + worked data
- **Pros**: No browser needed, fast, scriptable
- **Cons**: Separate codebase from web maps

### Option 4: Canvas Export from Web Map
- Add JavaScript button to existing maps: "Export as PNG"
- Use `html2canvas` or Leaflet's `.toDataURL()` method
- Download directly from browser
- **Pros**: Zero Qt code needed
- **Cons**: User must use browser, not integrated

**Recommended Approach**: Option 1 (QWebEngineView screenshot)
- Most integrated with existing architecture
- Leverages already-working web maps
- Simple Qt implementation

**Implementation Steps**:
1. Create `MapExporter` class with `exportMapImage()` method
2. Render map in hidden QWebEngineView
3. Wait for map to fully load (detect via JS callback)
4. Call `grab()` to capture as QPixmap
5. Save to user-selected file path (PNG/JPG)
6. Add menu action: File → Export Map as Image...
7. Show file dialog with format selection

**Files to Create/Modify**:
- `src/utils/MapExporter.h` - New exporter class
- `src/utils/MapExporter.cpp` - Screenshot implementation
- `src/ui/MainWindow.cpp` - Add export menu action
- May reuse `MapViewerDialog` if it exists

**Export Options Dialog**:
- Map type: Sections / States / DXCC
- Image size: 1920x1080, 2560x1440, 3840x2160, Custom
- Format: PNG (lossless) / JPG (smaller file)
- Include legend: Yes/No
- Include statistics overlay: Yes/No

---

## Estimated Implementation Time

- ✅ US States Map: ~2-3 hours (download data, convert, implement) - **COMPLETED**
- In-App Map Viewer: ~2-3 hours (QWebEngineView widget, menu items, window management)
- Static Map Export: ~2-3 hours (QWebEngineView screenshot, export dialog, file handling)
- DXCC Map: ~4-6 hours (download data, mapping logic, implement)
- Contest auto-switching: ~1 hour (simple routing logic)

**Total Completed**: 2-3 hours
**Total Remaining**: ~9-15 hours of development work
