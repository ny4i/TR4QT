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

1. **High**: US States Map
   - Most requested feature
   - Data readily available
   - Many states already in hand from ARRL sections
   - Simpler than DXCC (no prefix mapping needed)

2. **Medium**: DXCC Entities Map
   - More complex (multi-entity countries)
   - Requires DXCC prefix mapping
   - Larger dataset (world coverage)
   - Higher value for DX contesters

3. **Low**: Contest-specific auto-switching
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

## Estimated Implementation Time

- US States Map: ~2-3 hours (download data, convert, implement)
- DXCC Map: ~4-6 hours (download data, mapping logic, implement)
- Contest auto-switching: ~1 hour (simple routing logic)

**Total**: ~7-10 hours of development work
