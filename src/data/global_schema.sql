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
