-- TR4QT Global Database Schema
-- SQLite database for application-wide data (shared across all contests)
-- This database contains data that should persist and be shared between contest logs

-- LOTW users tracking
-- Stores callsigns that use LOTW and their last upload timestamp
-- Downloaded from https://lotw.arrl.org/lotw-user-activity.csv
CREATE TABLE IF NOT EXISTS lotw_users (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    callsign TEXT NOT NULL UNIQUE,         -- Callsign (normalized uppercase)
    last_upload_date TEXT NOT NULL,        -- Date in YYYY-MM-DD format
    last_upload_time TEXT NOT NULL,        -- Time in HH:MM:SS format (UTC)
    last_updated INTEGER NOT NULL          -- Unix timestamp when we downloaded this data
);

-- Index for fast lookups by callsign (primary use case)
CREATE INDEX IF NOT EXISTS idx_lotw_callsign ON lotw_users(callsign);

-- Index for cleanup queries (finding stale data)
CREATE INDEX IF NOT EXISTS idx_lotw_updated ON lotw_users(last_updated);

-- Global application settings (optional - can also use QSettings)
-- This table can store settings that need to be queryable or relational
CREATE TABLE IF NOT EXISTS global_settings (
    key TEXT PRIMARY KEY,
    value TEXT,
    updated_at INTEGER NOT NULL          -- Unix timestamp
);

-- DX Cluster spots persistence (shutdown snapshot only)
-- Stores spots in memory during operation, persisted on clean shutdown
-- Enables band switching to show previously received spots and aging indicators
CREATE TABLE IF NOT EXISTS dx_spots (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    callsign TEXT NOT NULL UNIQUE,         -- Callsign spotted (normalized uppercase)
    frequency INTEGER NOT NULL,            -- Transmit frequency in Hz
    qsx INTEGER DEFAULT 0,                 -- Split RX frequency in Hz (0 if not split)
    timestamp INTEGER NOT NULL,            -- Unix timestamp when spot was received
    comment TEXT,                          -- DX cluster comment
    source TEXT,                           -- Source (e.g., "DX Cluster (W1ABC)")
    is_multiplier BOOLEAN DEFAULT 0,       -- Is this a needed multiplier?
    is_worked BOOLEAN DEFAULT 0,           -- Already worked this station?
    is_lotw_user BOOLEAN DEFAULT 0,        -- Is this a LOTW user?
    azimuth REAL DEFAULT -1.0,             -- Bearing from user's location (degrees, 0-360, -1 if unknown)
    distance REAL DEFAULT -1.0             -- Distance from user's location (km, -1 if unknown)
);

-- Index for fast callsign lookup (duplicate checking)
CREATE INDEX IF NOT EXISTS idx_spots_callsign ON dx_spots(callsign);

-- Index for timestamp-based queries (finding old spots)
CREATE INDEX IF NOT EXISTS idx_spots_timestamp ON dx_spots(timestamp);

-- DXCC Entity mapping (ADIF specification)
-- Maps DXCC entity codes to entity names for ADIF exports
-- Based on ADIF specification: https://adif.org.uk/316/ADIF_316.htm
CREATE TABLE IF NOT EXISTS dxcc_entities (
    entity_code INTEGER PRIMARY KEY,       -- ADIF DXCC Entity Code (1-522)
    entity_name TEXT NOT NULL UNIQUE,      -- Official ADIF entity name
    is_deleted BOOLEAN DEFAULT 0,          -- True if entity was deleted from DXCC list
    notes TEXT                             -- Additional notes (e.g., "Deleted 1991")
);

-- Index for reverse lookup (entity name -> code)
CREATE INDEX IF NOT EXISTS idx_dxcc_name ON dxcc_entities(entity_name);

-- Super Check Partial (SCP) callsign database
-- Stores callsigns from MASTER.SCP download + local QSO history
-- Used for real-time callsign matching as operator types in callsign field
-- Downloaded from http://www.supercheckpartial.com/MASTER.SCP
CREATE TABLE IF NOT EXISTS scp_callsigns (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    callsign TEXT NOT NULL UNIQUE COLLATE NOCASE,  -- Normalized uppercase callsign
    source TEXT NOT NULL,                           -- 'master_scp' or 'local_log'
    contest_id TEXT,                                -- Contest ID if from local log (NULL for master)
    added_at INTEGER NOT NULL                       -- Unix timestamp when added
);

-- Index for prefix/suffix matching (critical for performance)
-- SQLite LIKE 'W1%' can use this index for prefix matching
CREATE INDEX IF NOT EXISTS idx_scp_callsign ON scp_callsigns(callsign);

-- Index for source filtering (to separate master vs local)
CREATE INDEX IF NOT EXISTS idx_scp_source ON scp_callsigns(source);

-- SCP metadata tracking (version, last update time, etc.)
-- Stores metadata about MASTER.SCP database (version, download date, etc.)
CREATE TABLE IF NOT EXISTS scp_metadata (
    key TEXT PRIMARY KEY,
    value TEXT,
    updated_at INTEGER NOT NULL          -- Unix timestamp
);
