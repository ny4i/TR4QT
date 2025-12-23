-- TR4QT Database Schema
-- SQLite database for storing contest logs

-- Contest sessions
-- Each contest gets its own log/session
CREATE TABLE IF NOT EXISTS contests (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    contest_id TEXT NOT NULL,           -- Contest type (e.g., "CQWW_CW")
    contest_name TEXT NOT NULL,         -- Display name
    start_time INTEGER,                 -- Unix timestamp
    end_time INTEGER,                   -- Unix timestamp
    my_call TEXT NOT NULL,              -- Operating callsign
    my_grid TEXT,                       -- Maidenhead grid
    my_country TEXT,                    -- My country name
    my_continent TEXT,                  -- My continent
    my_cq_zone INTEGER,                 -- My CQ zone
    my_itu_zone INTEGER,                -- My ITU zone
    my_state TEXT,                      -- My state/province
    exchange_sent TEXT,                 -- My exchange template
    current_serial INTEGER DEFAULT 1,   -- Current serial number
    created_at INTEGER NOT NULL,        -- When log was created
    notes TEXT                          -- Contest notes
);

CREATE INDEX IF NOT EXISTS idx_contests_id ON contests(contest_id);

-- QSO log entries
CREATE TABLE IF NOT EXISTS qsos (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    contest_id INTEGER NOT NULL,        -- FK to contests table

    -- QSO timing
    timestamp INTEGER NOT NULL,         -- Unix timestamp (UTC)

    -- Station worked
    callsign TEXT NOT NULL,             -- Callsign (normalized uppercase)

    -- Frequency/Mode/Band
    frequency INTEGER NOT NULL,         -- Frequency in Hz
    mode TEXT NOT NULL,                 -- Mode (CW, SSB, etc.)
    band TEXT NOT NULL,                 -- Band (160M, 80M, etc.)

    -- Exchange
    rst_sent TEXT DEFAULT '599',
    rst_received TEXT DEFAULT '599',
    exchange_sent TEXT,                 -- What we sent
    exchange_received TEXT,             -- What we received

    -- DXCC/Geographic (from cty.dat lookup)
    dxcc_entity TEXT,                   -- Country name
    dxcc_prefix TEXT,                   -- DXCC prefix
    cq_zone INTEGER,                    -- CQ Zone
    itu_zone INTEGER,                   -- ITU Zone
    continent TEXT,                     -- Continent code
    state TEXT,                         -- US/VE state/province

    -- Scoring
    qso_points INTEGER DEFAULT 0,       -- Points for this QSO
    is_dupe BOOLEAN DEFAULT 0,          -- Duplicate?
    is_multiplier BOOLEAN DEFAULT 0,    -- Provides multiplier?
    multipliers TEXT,                   -- JSON array of mult values

    -- Metadata
    serial_number INTEGER,              -- Our serial number sent
    operator_call TEXT,                 -- Operator who made this QSO
    deleted BOOLEAN DEFAULT 0,          -- Soft delete
    notes TEXT,                         -- Optional notes

    FOREIGN KEY (contest_id) REFERENCES contests(id) ON DELETE CASCADE
);

CREATE INDEX IF NOT EXISTS idx_qsos_contest ON qsos(contest_id);
CREATE INDEX IF NOT EXISTS idx_qsos_callsign ON qsos(callsign);
CREATE INDEX IF NOT EXISTS idx_qsos_timestamp ON qsos(timestamp);
CREATE INDEX IF NOT EXISTS idx_qsos_band_mode ON qsos(band, mode);
CREATE INDEX IF NOT EXISTS idx_qsos_dupe_check ON qsos(contest_id, callsign, band, mode);

-- Multipliers worked
-- Tracks which multipliers have been worked (for checking new mults)
CREATE TABLE IF NOT EXISTS multipliers (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    contest_id INTEGER NOT NULL,        -- FK to contests table
    mult_type TEXT NOT NULL,            -- Multiplier type (Country, CQZone, etc.)
    mult_value TEXT NOT NULL,           -- Multiplier value ("K", "5", "W1", etc.)
    band TEXT,                          -- Band if per-band, NULL if all-band
    first_qso_id INTEGER,               -- First QSO that worked this mult
    qso_count INTEGER DEFAULT 1,        -- How many times worked

    FOREIGN KEY (contest_id) REFERENCES contests(id) ON DELETE CASCADE,
    FOREIGN KEY (first_qso_id) REFERENCES qsos(id) ON DELETE SET NULL,

    UNIQUE(contest_id, mult_type, mult_value, band)
);

CREATE INDEX IF NOT EXISTS idx_mults_contest ON multipliers(contest_id);
CREATE INDEX IF NOT EXISTS idx_mults_type ON multipliers(mult_type);
CREATE INDEX IF NOT EXISTS idx_mults_lookup ON multipliers(contest_id, mult_type, band);

-- Application settings (for single-value config)
CREATE TABLE IF NOT EXISTS settings (
    key TEXT PRIMARY KEY,
    value TEXT
);
