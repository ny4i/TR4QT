# TR4QT Feature Roadmap - Market Differentiators

**Generated**: 2026-01-09
**Focus**: Features that differentiate TR4QT from competing contest loggers
**Source**: GitHub issues with "prompts" label
**Philosophy**: Preserve TR4W minimalist efficiency while leveraging modern Qt capabilities

---

## Executive Summary

This roadmap focuses on features that will make TR4QT a market leader in contest logging software. Each feature addresses a gap in competing loggers while maintaining the TR4W philosophy of keyboard-driven, efficient operation.

**Strategic Goals**:
1. **Digital Mode Integration** - Best-in-class WSJT-X integration
2. **Intelligence Layer** - Smart SCP, historical analysis, predictive band changes
3. **Modern UX** - Command palette, keyboard discovery, visual analytics
4. **Competitive Analysis** - Multi-year comparisons, performance trends

---

## 🎯 Market Differentiator Analysis

### Competitor Gap Analysis

**N1MM Logger+**:
- ✅ Excellent multi-op support
- ❌ Dated UI, steep learning curve
- ❌ Limited historical analysis
- ❌ Basic digital mode integration

**Win-Test**:
- ✅ Strong SO2R support
- ❌ Windows-only
- ❌ No modern visual analytics
- ❌ Limited digital mode workflows

**Logger32**:
- ✅ Good general logging
- ❌ Not contest-optimized
- ❌ No intelligent callsign prediction
- ❌ Basic digital integration

**TR4QT Opportunity**:
- ✅ Cross-platform (macOS, Windows, Linux)
- ✅ Modern Qt UI with TR4W keyboard efficiency
- 🎯 **Add**: Intelligent SCP with live spot integration
- 🎯 **Add**: Unified digital mode view (WSJT-X native)
- 🎯 **Add**: Multi-year contest analysis
- 🎯 **Add**: Command palette for discoverability

---

## 📊 Priority Matrix

### P0: Foundation (Infrastructure for other features)

| Feature | Effort | Impact | Strategic Value |
|---------|--------|--------|-----------------|
| **Flexible Keyboard Command System** | Medium | High | Enables all UX improvements |
| **Roadmap Generation** | Low | Medium | Documentation/planning tool |

### P1: High Impact Differentiators

| Feature | Effort | Impact | Strategic Value |
|---------|--------|--------|-----------------|
| **Progressive SCP Engine** | High | Very High | Unique competitive advantage |
| **Digital Interface to WSJT-X** | Very High | Very High | Critical for FT8/FT4 contests |
| **Command Bar (Help Overlay)** | Medium | High | Improves discoverability |

### P2: Advanced Features

| Feature | Effort | Impact | Strategic Value |
|---------|--------|--------|-----------------|
| **Multi-Year Contest Comparison** | Very High | Medium | Appeals to serious contesters |
| **SH5-like Contest Analysis** | High | Medium | Post-contest optimization |

---

## 🚀 Feature Details

### P0-1: Flexible Keyboard Command System
**GitHub Issue**: #50
**Status**: Foundation - Must complete first
**Effort**: 2-3 weeks
**Strategic Value**: HIGH - Enables all keyboard-driven features

#### Description
Create a centralized keyboard command system that replaces scattered shortcut handling. Enables dynamic rebinding, command discovery, and shared keyboard handling across multiple views.

#### Requirements
- CommandID enum for all application commands
- KeyBindingManager for centralized shortcut management
- JSON configuration file for user customization
- Support for context-sensitive bindings (main window vs dialogs)
- Reverse query: "What does Alt+B do?"

#### Architecture
```cpp
// commandid.h
enum class CommandID {
    LogQSO,
    BandUp,
    BandDown,
    ShowBandMap,
    ShowDXCluster,
    EditCWMessages,
    // ... 100+ commands
};

// keybindingmanager.h
class KeyBindingManager {
public:
    void registerCommand(CommandID cmd, const QString& name,
                        const QKeySequence& defaultBinding);
    QKeySequence getBinding(CommandID cmd) const;
    CommandID getCommandForKey(const QKeySequence& key) const;
    void saveBindings(const QString& path);
    void loadBindings(const QString& path);

signals:
    void commandTriggered(CommandID cmd);
};
```

#### Implementation Plan
1. Create CommandID enum (all existing shortcuts)
2. Implement KeyBindingManager
3. Create JSON config schema
4. Migrate MainWindow shortcuts to use manager
5. Add settings UI for rebinding
6. Document all commands

#### Market Differentiator
- **Unique**: No other contest logger has discoverable, rebindable keyboard shortcuts
- **Value**: Reduces learning curve, enables power users to customize workflow

---

### P0-2: Roadmap Generation
**GitHub Issue**: #52
**Status**: Documentation/Planning Tool
**Effort**: 1 week
**Strategic Value**: MEDIUM - Improves project visibility

#### Description
Generate comprehensive ROADMAP.md by analyzing all *.md files in project. Creates architectural overview, platform-specific notes, and grouped TODO list.

#### Requirements
- Parse all markdown files recursively
- Extract TODOs, action items, architectural notes
- Generate structured ROADMAP.md with:
  - Application purpose and scope
  - Main workflows
  - Architecture overview (managers, contest system, etc.)
  - Platform-specific considerations (macOS vs Windows)
  - Grouped implementation tasks

#### Implementation Approach
Use Task agent to:
1. Read all *.md files
2. Build mental model of application
3. Extract action items from TODO.md, REFACTORING_STATUS.md, etc.
4. Generate ROADMAP.md with clear sections

#### Market Differentiator
- **Unique**: Auto-generated living roadmap from documentation
- **Value**: Improved transparency for contributors, clear project direction

---

### P1-1: Progressive "Super Check Partial" (SCP) Engine
**GitHub Issue**: #55
**Status**: High Impact - Top Priority
**Effort**: 6-8 weeks
**Strategic Value**: VERY HIGH - Unique competitive advantage

#### Description
Advanced SCP engine that goes beyond simple callsign matching. Integrates historical activity data, live spots, and multiplier status to provide intelligent predictions.

#### Requirements

**Core Functionality**:
- Load MASTER.SCP database (100k+ callsigns)
- Real-time partial matching as user types
- Display top 5-10 matches in check window

**Progressive Intelligence** (Market Differentiator):
1. **Band/Mode Weighting**
   - Weight matches based on current band
   - Adjust for mode (CW/SSB/RTTY/FT8)
   - Example: If on 20M CW, prioritize callsigns active on 20M CW

2. **Time-of-Day Analysis**
   - Analyze historical logs for activity patterns
   - Prioritize callsigns likely to be active now
   - Example: EU stations more likely 10-14 UTC

3. **Live Spot Overlay**
   - Integrate DX Cluster spots (last 15 minutes)
   - Highlight SCP matches that have been spotted recently
   - Visual indicator: "🔴 K3LR (spotted 5m ago on 14.025)"

4. **Multiplier Integration**
   - Color-code matches by multiplier status
   - **NEW MULT**: Bright green
   - **WORKED**: Gray/dimmed
   - **DUPE**: Red

5. **Contest-Specific Learning**
   - Build activity database per contest
   - Remember who works which contests
   - Example: CQ WW regulars vs casual participants

#### Architecture
```cpp
// src/scp/SCPEngine.h
class SCPEngine {
public:
    struct Match {
        QString callsign;
        float score;              // 0.0-1.0 confidence
        bool isNewMultiplier;
        bool isRecentlySpotted;
        QDateTime lastSpotTime;
        BandType suggestedBand;
    };

    QList<Match> getMatches(const QString& partial,
                           const Context& context);

private:
    SCPDatabase* m_database;
    ActivityAnalyzer* m_analyzer;
    SpotIntegrator* m_spotIntegrator;
    MultiplierTracker* m_multTracker;
};

// Context includes: current band, mode, time, contest type
struct Context {
    BandType band;
    ModeType mode;
    QDateTime timestamp;
    QString contestType;
    StationInfo myStation;
};
```

#### Data Sources
1. **MASTER.SCP** - Master callsign database
2. **Historical Logs** - User's past contest logs
3. **DX Cluster** - Real-time spots (existing integration)
4. **Contest Instance** - Current multiplier status

#### UI Integration
- Display in existing SCP label (Column 3)
- Add color coding for multiplier status
- Tooltip shows: "Last spotted: 5m ago, 14.025 MHz, New Mult!"
- Keyboard navigation: Ctrl+↓ to cycle through matches

#### Implementation Plan
1. **Phase 1** (2 weeks): Enhanced basic SCP
   - Improve existing SCPMatcher
   - Add score weighting algorithm
   - Integrate with CountryFile for DXCC info

2. **Phase 2** (2 weeks): Band/Mode weighting
   - Create ActivityAnalyzer
   - Parse historical logs for patterns
   - Implement time-of-day analysis

3. **Phase 3** (2 weeks): Live spot integration
   - Connect to DXClusterWindow
   - Add 15-minute spot cache
   - Update SCP matches with spot info

4. **Phase 4** (2 weeks): Multiplier integration
   - Connect to MultiplierWidget
   - Add color coding to UI
   - Implement visual indicators

#### Market Differentiator
- **Unique**: No other logger combines SCP + spots + multipliers + time analysis
- **Value**: Significantly increases rate by predicting which stations to call
- **Target**: Serious contesters chasing high scores

---

### P1-2: Digital Interface to WSJT-X
**GitHub Issue**: #53
**Status**: Critical for FT8/FT4 Contests
**Effort**: 10-12 weeks
**Strategic Value**: VERY HIGH - Essential for modern contesting

#### Description
Unified "Contest Digital View" that integrates WSJT-X directly into TR4QT. Provides seamless FT8/FT4 operation with keyboard-driven workflow matching TR4W efficiency.

#### Background
WSJT-X is the de-facto standard for FT8/FT4 digital modes. Current workflow requires:
1. Run WSJT-X separately
2. Manually enter QSOs into logger
3. No integration with bandmap/multipliers
4. Poor keyboard workflow

**TR4QT Opportunity**: Create best-in-class digital integration, similar to how TR4W revolutionized CW contesting.

#### Requirements

**Core Integration**:
- Bi-directional UDP communication with WSJT-X
- Sync CAT control (frequency, mode)
- Sync PTT (audio/serial)
- Auto-import QSOs from WSJT-X

**Contest Digital View** (Market Differentiator):
- Single unified window combining:
  - WSJT-X decode list (live)
  - Band map (from spots + decodes)
  - Multiplier targets
  - QSO entry form
- Keyboard-driven workflow:
  - Arrow keys navigate decodes
  - Enter to log QSO
  - F1-F12 for messages (like CW)
  - Tab/Shift+Tab for CQ/S&P mode

**Smart Features**:
- Auto-highlight new multipliers in decode list
- Color-code dupes vs needed
- "Call Priority Queue" - rank decodes by mult value
- One-click "Work All New Mults" mode

#### Architecture
```cpp
// src/digital/WSJTXInterface.h
class WSJTXInterface : public QObject {
    Q_OBJECT
public:
    void connectToWSJTX();
    void sendMessage(const QString& message);
    void setFrequency(freq_t freq);

signals:
    void decodeReceived(const WSJTXDecode& decode);
    void qsoLogged(const QSO& qso);
    void statusChanged(const WSJTXStatus& status);
};

struct WSJTXDecode {
    QDateTime timestamp;
    int snr;
    float deltaTime;
    freq_t frequency;
    QString callsign;
    QString grid;
    bool isCQ;
    bool isNewMultiplier;  // TR4QT enhancement
    bool isDupe;           // TR4QT enhancement
};

// src/digital/DigitalContestView.h
class DigitalContestView : public QWidget {
    Q_OBJECT
public:
    // Unified view: decodes + bandmap + mults + logging
    void setContest(ContestBase* contest);
    void updateDecodes(const QList<WSJTXDecode>& decodes);

private:
    QTableView* m_decodeList;     // Live WSJT-X decodes
    BandMapWidget* m_bandMap;     // Visual frequency display
    MultiplierWidget* m_multView;  // Needed mults
    QLineEdit* m_callsignEntry;
    QLineEdit* m_exchangeEntry;
};
```

#### Protocol
Use existing WSJT-X UDP protocol (well-documented):
- Port 2237 (default)
- Messages: Heartbeat, Status, Decode, QSO Logged, etc.
- Bidirectional: TR4QT can send commands to WSJT-X

#### UI Design Philosophy
**TR4W Principles Applied to Digital**:
- No mouse required
- Keyboard shortcuts for all actions
- Minimal visual clutter
- Focus on rate (QSOs/hour)

**Layout**:
```
┌─────────────────────────────────────────────────┐
│ [20M FT8]  [Auto Seq: ON]  [CQ Mode]           │
├─────────────────────────────────────────────────┤
│ DECODES (Live from WSJT-X)                      │
│ 🟢 K3LR   EN91  +10dB  [New Mult - CA]         │ <- Green = new mult
│ 🔵 W1AW   FN31  +05dB  [Worked]                │ <- Blue = worked
│ 🔴 K1TTT  FN42  +12dB  [DUPE]                  │ <- Red = dupe
│ 🟢 VE3XYZ FN03  +08dB  [New Mult - ON]         │
│ ... (scrolling list)                            │
├─────────────────────────────────────────────────┤
│ BAND MAP                                        │
│ [Visual frequency display with decodes]         │
├─────────────────────────────────────────────────┤
│ CALLSIGN: [K3LR____]  EXCH: [CA______]         │
│ NEEDED MULTS: CA, OR, WA, MT (4 remaining)     │
└─────────────────────────────────────────────────┘
```

**Keyboard Shortcuts**:
- `↑`/`↓`: Navigate decode list
- `Enter`: Log selected QSO
- `Tab`: Switch to CQ mode
- `Shift+Tab`: Switch to S&P mode
- `F1-F12`: Send macros to WSJT-X
- `Ctrl+M`: Toggle multiplier-only filter
- `Escape`: Clear selection

#### Implementation Plan
1. **Phase 1** (3 weeks): UDP integration
   - Implement WSJT-X protocol
   - Receive decodes, status updates
   - Send frequency changes, messages

2. **Phase 2** (3 weeks): Decode list UI
   - Display live decodes in table
   - Color-code by mult/dupe status
   - Keyboard navigation

3. **Phase 3** (2 weeks): QSO logging integration
   - Auto-populate callsign from decode
   - Auto-fill exchange (grid, RST)
   - One-key logging

4. **Phase 4** (2 weeks): Band map integration
   - Merge WSJT-X decodes with DX spots
   - Visual frequency display
   - Click-to-tune

5. **Phase 5** (2 weeks): Smart features
   - Multiplier-only filter
   - Call priority queue
   - Auto-sequencing

#### Reference Implementation
TR4W has `uWSJTX` unit - study for protocol details. However, TR4QT will significantly improve the UX by unifying the views.

#### Market Differentiator
- **Unique**: Only logger with fully unified digital contest view
- **Value**: Makes FT8/FT4 contesting as efficient as CW with TR4W
- **Target**: RTTY Roundup, FT8 Roundup, ARRL Digital contests

---

### P1-3: Advanced "Command Bar" (Help Overlay)
**GitHub Issue**: #56
**Status**: UX Enhancement
**Effort**: 3-4 weeks
**Strategic Value**: HIGH - Reduces learning curve

#### Description
Interactive command palette overlay (like VS Code's Ctrl+Shift+P) showing all available commands. Includes reverse query to discover what keys do.

#### Requirements

**Command Palette**:
- Hotkey to activate (Alt+H or ?)
- Fuzzy search through all commands
- Show current key binding for each command
- Execute command from palette

**Reverse Query**:
- Press hotkey + any key combination
- Shows: "Alt+B → Band Up"
- Displays command description

**UI Design**:
- Semi-transparent overlay (doesn't close main window)
- Searchable list (QCompleter)
- Keyboard navigation (↑/↓/Enter)
- Escape to close

#### Architecture
```cpp
// src/ui/dialogs/CommandPalette.h
class CommandPalette : public QDialog {
    Q_OBJECT
public:
    void show() override;
    void setCommands(const QList<CommandInfo>& commands);

private:
    QLineEdit* m_searchBox;
    QListView* m_resultsList;
    KeyBindingManager* m_keyManager;
};

struct CommandInfo {
    CommandID id;
    QString name;
    QString description;
    QKeySequence binding;
    QString category;  // "Logging", "Band Control", "Windows", etc.
};
```

#### Implementation Plan
1. **Week 1**: Command palette UI
   - Create dialog with search box + list
   - Implement fuzzy search
   - Wire to KeyBindingManager

2. **Week 2**: Reverse query mode
   - Detect when user presses unknown key
   - Display binding information
   - Add help text

3. **Week 3**: Polish
   - Add command categories
   - Icons for commands
   - Dark theme support

4. **Week 4**: Documentation
   - Generate command reference from code
   - Update user guide

#### Market Differentiator
- **Unique**: No other logger has command discovery
- **Value**: New users can find features, power users can customize
- **Inspiration**: VS Code, Sublime Text command palettes

---

### P2-1: Multi-Year & Cross-Contest Comparison
**GitHub Issue**: #54
**Status**: Advanced Analytics
**Effort**: 8-10 weeks
**Strategic Value**: MEDIUM - Appeals to serious contesters

#### Description
Load multiple years of the same contest to visualize performance trends and optimize strategy.

#### Requirements

**Data Loading**:
- Import multiple contest databases
- Normalize QSO data across years
- Handle scoring rule changes

**Visualizations**:
1. **Rate Chart Comparison**
   - Side-by-side rate graphs (QSOs/hour)
   - Overlay multiple years on same chart
   - Identify peak hours vs slow periods

2. **Band/Mode Heatmap**
   - Visual grid: Band × Hour
   - Color intensity = QSO count
   - Compare 2024 vs 2025 heatmaps

3. **Run Potential Indicator**
   - Analyze historical runs
   - Suggest best times to change bands
   - "Based on past logs, 20M is productive 14-18 UTC"

4. **Performance Metrics**
   - Year-over-year score comparison
   - Multiplier efficiency (% worked vs available)
   - Rate trend analysis

#### Architecture
```cpp
// src/analysis/MultiYearComparison.h
class MultiYearComparison {
public:
    void loadContest(int year, const QString& dbPath);
    QList<RateData> getRateComparison();
    QImage generateHeatmap(int year);
    QString getRunPotentialAdvice(BandType band, QDateTime time);

private:
    QMap<int, ContestData> m_yearlyData;
    StatisticsEngine* m_statsEngine;
};

struct ContestData {
    int year;
    QList<QSO> qsos;
    QMap<BandType, QList<Run>> runs;
    QMap<int, int> hourlyRates;  // Hour → QSO count
};
```

#### UI Design
```
┌─────────────────────────────────────────────┐
│ Multi-Year Comparison: CQ WW CW             │
├─────────────────────────────────────────────┤
│ Years: [x] 2024  [x] 2025  [ ] 2026         │
├─────────────────────────────────────────────┤
│ [RATE CHART]                                │
│  QSO/Hr                                     │
│  200 ┤     ╱╲     2025                      │
│  150 ┤    ╱  ╲   ╱╲  2024                   │
│  100 ┤   ╱    ╲ ╱  ╲                        │
│   50 ┤  ╱      ╲    ╲                       │
│    0 └──────────────────────→ Hour          │
│      00 06 12 18 24 30 36 42 48             │
├─────────────────────────────────────────────┤
│ [BAND HEATMAP - 2024]                       │
│ Band  00 06 12 18 24 30 36 42 48           │
│ 160M  ░░ ░░ ░░ ░░ ██ ██ ░░ ░░ ░░           │
│  80M  ░░ ░░ ██ ██ ██ ░░ ░░ ░░ ░░           │
│  40M  ██ ██ ██ ░░ ░░ ░░ ██ ██ ██           │
│  20M  ░░ ██ ██ ██ ░░ ░░ ██ ██ ░░           │
│  15M  ░░ ░░ ██ ██ ██ ░░ ░░ ░░ ░░           │
│  10M  ░░ ░░ ░░ ██ ██ ██ ░░ ░░ ░░           │
├─────────────────────────────────────────────┤
│ RUN POTENTIAL ADVICE:                       │
│ • 20M is most productive 12-18 UTC          │
│ • Consider earlier start on 40M (04 UTC)    │
│ • 2025 had 15% more EU mults on 15M         │
└─────────────────────────────────────────────┘
```

#### Implementation Plan
1. **Phase 1** (3 weeks): Data loading
   - Import multiple contest DBs
   - Normalize data structures
   - Build comparison queries

2. **Phase 2** (3 weeks): Rate charts
   - Use QCustomPlot for visualization
   - Overlay multiple years
   - Interactive tooltips

3. **Phase 3** (2 weeks): Heatmaps
   - Generate band/time heatmaps
   - Color coding (red=high, blue=low)
   - Export as PNG

4. **Phase 4** (2 weeks): Run analysis
   - Detect runs in historical logs
   - Statistical analysis of productivity
   - Generate advice text

#### Market Differentiator
- **Unique**: No other logger has multi-year visualization
- **Value**: Helps operators optimize strategy for next year
- **Target**: Serious contesters, club competition teams

---

### P2-2: SH5-like Contest Log Analysis
**GitHub Issue**: #51
**Status**: Post-Contest Tool
**Effort**: 6-8 weeks
**Strategic Value**: MEDIUM - Post-contest optimization

#### Description
SH5-class analysis module that reads logs (Cabrillo/ADIF) and produces rich statistics and visualizations. Think "log analysis on steroids."

#### Background
SH5 was a famous post-contest analysis tool for TR that provided deep insights into operating strategy. Modern version should leverage Qt's visualization capabilities.

#### Requirements

**Import**:
- Read Cabrillo files
- Read ADIF files
- Import multiple logs for comparison

**Analysis Modules**:
1. **Rate Analysis**
   - QSO rate over time
   - Run detection and scoring
   - Off-time analysis (required breaks)

2. **Band/Mode Analysis**
   - Time spent per band
   - QSO efficiency per band
   - Optimal band change detection

3. **Multiplier Analysis**
   - Multiplier timeline (when worked)
   - Missing multipliers (available but not worked)
   - Multiplier efficiency (% worked)

4. **Error Detection**
   - Potential dupes
   - Invalid exchanges
   - Clock errors (timestamp anomalies)

5. **Comparison Analysis**
   - Compare to previous years
   - Compare to other operators
   - Identify improvement areas

**Visualizations**:
- Interactive rate charts (QCustomPlot)
- Band usage pie charts
- Multiplier progress timeline
- Heatmaps (band × time)

#### Architecture
```cpp
// src/analysis/LogAnalyzer.h
class LogAnalyzer {
public:
    void loadLog(const QString& filePath, LogFormat format);

    // Analysis engines
    RateAnalysis analyzeRate();
    BandAnalysis analyzeBands();
    MultiplierAnalysis analyzeMultipliers();
    QList<LogError> detectErrors();

    // Reports
    QString generateTextReport();
    void exportPDF(const QString& path);
    void exportHTML(const QString& path);
};

// src/ui/dialogs/AnalysisWindow.h
class AnalysisWindow : public QMainWindow {
    Q_OBJECT
public:
    void setLog(const LogData& log);

private:
    QTabWidget* m_tabs;
    RateChartWidget* m_rateChart;
    BandUsageWidget* m_bandUsage;
    MultiplierTimelineWidget* m_multTimeline;
};
```

#### UI Design
```
┌─────────────────────────────────────────────┐
│ Log Analysis: CQ WW CW 2024 - K3LR          │
├─────────────────────────────────────────────┤
│ [Rate] [Bands] [Mults] [Errors] [Compare]  │
├─────────────────────────────────────────────┤
│ RATE ANALYSIS                               │
│                                             │
│  QSO/Hr                                     │
│  200 ┤     ╱╲                               │
│  150 ┤    ╱  ╲   ╱╲                         │
│  100 ┤   ╱    ╲ ╱  ╲                        │
│   50 ┤  ╱      ╲    ╲                       │
│    0 └──────────────────────→ Hour          │
│      00 06 12 18 24 30 36 42 48             │
│                                             │
│  Avg Rate: 156 QSO/hour                     │
│  Peak Rate: 218 QSO/hour (14 UTC, 20M)     │
│  Best Run: 147 QSOs (12-14 UTC, 20M CW)    │
│  Off Time: 3.2 hours (required: 3.0)       │
├─────────────────────────────────────────────┤
│ IMPROVEMENT SUGGESTIONS:                    │
│ • Start 40M run 2 hours earlier            │
│ • Missed 12 multipliers on 15M (list...)   │
│ • Rate dropped during band changes         │
└─────────────────────────────────────────────┘
```

#### Implementation Plan
1. **Phase 1** (2 weeks): Log parsing
   - Cabrillo parser
   - ADIF parser
   - Unified data model

2. **Phase 2** (2 weeks): Rate analysis
   - Calculate rates
   - Detect runs
   - Off-time calculation

3. **Phase 3** (2 weeks): Band/mult analysis
   - Band time tracking
   - Multiplier timeline
   - Missing mult detection

4. **Phase 4** (2 weeks): Visualizations
   - Rate charts (QCustomPlot)
   - Band usage pie charts
   - Interactive timeline

#### Market Differentiator
- **Unique**: Built-in log analysis (competitors require separate tools)
- **Value**: Immediate post-contest insights for improvement
- **Target**: Serious contesters wanting to improve scores

---

## 📈 Implementation Roadmap

### Phase 1: Foundation (Q1 2026) - 4 weeks

**Goal**: Infrastructure for advanced features

1. ✅ Complete remaining refactoring (6 hours)
   - Fix DialogHelper violations
   - Extract constants
   - Band logic consolidation

2. **Flexible Keyboard Command System** (3 weeks)
   - CommandID enum
   - KeyBindingManager
   - JSON configuration
   - Settings UI for rebinding

3. **Roadmap Generation** (1 week)
   - Parse markdown files
   - Generate ROADMAP.md

**Deliverable**: Solid foundation for keyboard-driven features

---

### Phase 2: Market Differentiators (Q2 2026) - 12 weeks

**Goal**: Unique features that set TR4QT apart

1. **Progressive SCP Engine** (8 weeks)
   - Enhanced matching
   - Band/mode weighting
   - Live spot integration
   - Multiplier color-coding

2. **Command Bar (Help Overlay)** (4 weeks)
   - Command palette UI
   - Reverse query mode
   - Documentation generation

**Deliverable**: Intelligent SCP and discoverability

---

### Phase 3: Digital Mode Revolution (Q3 2026) - 12 weeks

**Goal**: Best-in-class FT8/FT4 integration

1. **Digital Interface to WSJT-X** (12 weeks)
   - UDP protocol integration
   - Unified Contest Digital View
   - Keyboard-driven workflow
   - Smart features (mult filter, priority queue)

**Deliverable**: Seamless digital mode contesting

---

### Phase 4: Analytics & Optimization (Q4 2026) - 14 weeks

**Goal**: Advanced analysis and strategy tools

1. **Multi-Year Contest Comparison** (8 weeks)
   - Data loading and normalization
   - Rate charts and heatmaps
   - Run potential analysis
   - Performance metrics

2. **SH5-like Contest Analysis** (6 weeks)
   - Log parsing (Cabrillo/ADIF)
   - Rate, band, multiplier analysis
   - Visualization suite
   - Report generation

**Deliverable**: Comprehensive post-contest analysis

---

## 🎯 Success Metrics

### User Adoption
- **Target**: 1,000 active users by end of 2026
- **Metric**: Downloads from GitHub Releases
- **Benchmark**: N1MM (~10k users), Win-Test (~2k users)

### Feature Differentiation
- **SCP Intelligence**: Measure accuracy vs basic SCP
- **Digital Mode Usage**: Track FT8/FT4 QSOs logged
- **Analysis Usage**: Post-contest analysis runs

### User Satisfaction
- **GitHub Stars**: Target 500+ (currently ~50)
- **Issue Response Time**: < 48 hours
- **User Testimonials**: Collect on website

### Contest Performance
- **Score Improvements**: Track users' year-over-year scores
- **Rate Improvements**: Compare to previous contest software
- **Testimonials**: "Switched from N1MM, rate increased 15%"

---

## 🔧 Technical Debt Management

While adding features, maintain code quality:

1. **Code Review**: All new features go through code-refactoring-reviewer agent
2. **Testing**: 90%+ test coverage for new code
3. **Documentation**: Update docs with each feature
4. **Refactoring**: Address tech debt before adding dependent features

---

## 🎓 Development Philosophy

### TR4W Heritage
- **Keyboard-first**: No mouse required for any operation
- **Minimalist UI**: Clean, functional, fast
- **Contest-focused**: Every feature serves the goal of higher QSO rates

### Modern Qt Capabilities
- **Cross-platform**: macOS, Windows, Linux
- **Visual analytics**: Leverage Qt charts and graphics
- **Network integration**: WebServer, UDP, TCP
- **Plugin system**: Contest modules, radio interfaces

### Market Positioning
- **Quality over quantity**: Better to have 10 excellent features than 50 mediocre ones
- **Innovation**: Don't copy competitors, leapfrog them
- **Community**: Open source, transparent development

---

## 📊 Competitive Analysis Summary

| Feature | N1MM+ | Win-Test | Logger32 | **TR4QT** |
|---------|-------|----------|----------|-----------|
| **Cross-platform** | ❌ | ❌ | ❌ | ✅ |
| **Intelligent SCP** | ❌ | ❌ | ❌ | 🎯 P1 |
| **WSJT-X Integration** | Basic | Basic | Basic | 🎯 P1 (Unified) |
| **Multi-year Analysis** | ❌ | ❌ | ❌ | 🎯 P2 |
| **Command Palette** | ❌ | ❌ | ❌ | 🎯 P1 |
| **Built-in Log Analysis** | ❌ | ❌ | ❌ | 🎯 P2 |
| **Keyboard Rebinding** | Limited | Limited | Limited | 🎯 P0 |
| **Open Source** | ❌ | ❌ | ❌ | ✅ |

---

## 🚀 Call to Action

This roadmap defines TR4QT's path to becoming the premier contest logging software. Each feature has been chosen for its strategic value and market differentiation.

**Next Steps**:
1. Review and approve this roadmap
2. Close GitHub issues as features are incorporated
3. Begin Phase 1: Foundation (Keyboard Command System)
4. Track progress in quarterly reviews

**Long-term Vision**: By end of 2026, TR4QT should be recognized as the most innovative, user-friendly, and powerful contest logger available.

---

**Last Updated**: 2026-01-09
**Next Review**: 2026-04-01 (Quarterly)
