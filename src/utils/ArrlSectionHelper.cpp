#include "ArrlSectionHelper.h"
#include <QHash>
#include <QPair>
#include <QRegularExpression>
#include <QStringList>

namespace TR4QT {
namespace Arrl {

// Forward declarations
static QString normalizeCounty(const QString& county);
static QString normalizeState(const QString& state);

// ============================================================================
// SIMPLE STATE → SECTION MAPPINGS (1:1 states)
// ============================================================================
// States that have a single ARRL section for the entire state

static const QHash<QString, QString> simpleStateSections = {
    // Single-section states (alphabetical)
    {"AK", "AK"},   // Alaska
    {"AL", "AL"},   // Alabama
    {"AR", "AR"},   // Arkansas
    {"AZ", "AZ"},   // Arizona
    {"CO", "CO"},   // Colorado
    {"CT", "CT"},   // Connecticut
    {"DE", "DE"},   // Delaware
    {"GA", "GA"},   // Georgia
    {"HI", "HI"},   // Hawaii
    {"IA", "IA"},   // Iowa
    {"ID", "ID"},   // Idaho
    {"IL", "IL"},   // Illinois
    {"IN", "IN"},   // Indiana
    {"KS", "KS"},   // Kansas
    {"KY", "KY"},   // Kentucky
    {"LA", "LA"},   // Louisiana
    {"MDC", "MDC"}, // Maryland-DC (note: DC also maps here)
    {"MD", "MDC"},  // Maryland
    {"ME", "ME"},   // Maine
    {"MI", "MI"},   // Michigan
    {"MN", "MN"},   // Minnesota
    {"MO", "MO"},   // Missouri
    {"MS", "MS"},   // Mississippi
    {"MT", "MT"},   // Montana
    {"NC", "NC"},   // North Carolina
    {"ND", "ND"},   // North Dakota
    {"NE", "NE"},   // Nebraska
    {"NH", "NH"},   // New Hampshire
    {"NM", "NM"},   // New Mexico
    {"NV", "NV"},   // Nevada
    {"OH", "OH"},   // Ohio
    {"OK", "OK"},   // Oklahoma
    {"OR", "OR"},   // Oregon
    {"RI", "RI"},   // Rhode Island
    {"SC", "SC"},   // South Carolina
    {"SD", "SD"},   // South Dakota
    {"TN", "TN"},   // Tennessee
    {"UT", "UT"},   // Utah
    {"VA", "VA"},   // Virginia
    {"VI", "VI"},   // Virgin Islands
    {"VT", "VT"},   // Vermont
    {"WI", "WI"},   // Wisconsin
    {"WV", "WV"},   // West Virginia
    {"WY", "WY"}    // Wyoming
};

// ============================================================================
// FLORIDA (3 sections: NFL, WCF, SFL)
// ============================================================================
// Source: http://www.arrl.org/florida-section
// NFL = Northern Florida, WCF = West Central Florida, SFL = South Florida

static const QHash<QString, QString> floridaCounties = {
    // NFL - Northern Florida
    {"ALACHUA", "NFL"},
    {"BAKER", "NFL"},
    {"BAY", "NFL"},
    {"BRADFORD", "NFL"},
    {"CALHOUN", "NFL"},
    {"CLAY", "NFL"},
    {"COLUMBIA", "NFL"},
    {"DIXIE", "NFL"},
    {"DUVAL", "NFL"},
    {"ESCAMBIA", "NFL"},
    {"FLAGLER", "NFL"},
    {"FRANKLIN", "NFL"},
    {"GADSDEN", "NFL"},
    {"GILCHRIST", "NFL"},
    {"GULF", "NFL"},
    {"HAMILTON", "NFL"},
    {"HOLMES", "NFL"},
    {"JACKSON", "NFL"},
    {"JEFFERSON", "NFL"},
    {"LAFAYETTE", "NFL"},
    {"LEON", "NFL"},
    {"LEVY", "NFL"},
    {"LIBERTY", "NFL"},
    {"MADISON", "NFL"},
    {"NASSAU", "NFL"},
    {"OKALOOSA", "NFL"},
    {"PUTNAM", "NFL"},
    {"SANTA ROSA", "NFL"},
    {"ST. JOHNS", "NFL"},
    {"ST JOHNS", "NFL"},
    {"SAINT JOHNS", "NFL"},
    {"SUWANNEE", "NFL"},
    {"TAYLOR", "NFL"},
    {"UNION", "NFL"},
    {"WAKULLA", "NFL"},
    {"WALTON", "NFL"},
    {"WASHINGTON", "NFL"},

    // WCF - West Central Florida
    {"BREVARD", "WCF"},
    {"CITRUS", "WCF"},
    {"HARDEE", "WCF"},
    {"HERNANDO", "WCF"},
    {"HIGHLANDS", "WCF"},
    {"HILLSBOROUGH", "WCF"},
    {"LAKE", "WCF"},
    {"MANATEE", "WCF"},
    {"MARION", "WCF"},
    {"ORANGE", "WCF"},
    {"OSCEOLA", "WCF"},
    {"PASCO", "WCF"},
    {"PINELLAS", "WCF"},
    {"POLK", "WCF"},
    {"SEMINOLE", "WCF"},
    {"SUMTER", "WCF"},
    {"VOLUSIA", "WCF"},

    // SFL - South Florida
    {"BROWARD", "SFL"},
    {"CHARLOTTE", "SFL"},
    {"COLLIER", "SFL"},
    {"DESOTO", "SFL"},
    {"GLADES", "SFL"},
    {"HENDRY", "SFL"},
    {"INDIAN RIVER", "SFL"},
    {"LEE", "SFL"},
    {"MARTIN", "SFL"},
    {"MIAMI-DADE", "SFL"},
    {"MIAMI DADE", "SFL"},
    {"DADE", "SFL"},
    {"MONROE", "SFL"},
    {"OKEECHOBEE", "SFL"},
    {"PALM BEACH", "SFL"},
    {"SARASOTA", "SFL"},
    {"ST. LUCIE", "SFL"},
    {"ST LUCIE", "SFL"},
    {"SAINT LUCIE", "SFL"}
};

// ============================================================================
// CALIFORNIA (10 sections: EB, LAX, ORG, SB, SCV, SDG, SF, SJV, SV, PAC)
// ============================================================================
// Source: http://www.arrl.org/california-section

static const QHash<QString, QString> californiaCounties = {
    // EB - East Bay
    {"ALAMEDA", "EB"},
    {"CONTRA COSTA", "EB"},

    // LAX - Los Angeles
    {"LOS ANGELES", "LAX"},

    // ORG - Orange
    {"ORANGE", "ORG"},

    // SB - Santa Barbara
    {"SAN LUIS OBISPO", "SB"},
    {"SANTA BARBARA", "SB"},
    {"VENTURA", "SB"},

    // SCV - Santa Clara Valley
    {"SANTA CLARA", "SCV"},

    // SDG - San Diego
    {"IMPERIAL", "SDG"},
    {"SAN DIEGO", "SDG"},

    // SF - San Francisco
    {"MARIN", "SF"},
    {"SAN FRANCISCO", "SF"},
    {"SAN MATEO", "SF"},

    // SJV - San Joaquin Valley
    {"FRESNO", "SJV"},
    {"KERN", "SJV"},
    {"KINGS", "SJV"},
    {"MADERA", "SJV"},
    {"MERCED", "SJV"},
    {"SAN BENITO", "SJV"},
    {"SAN JOAQUIN", "SJV"},
    {"STANISLAUS", "SJV"},
    {"TULARE", "SJV"},

    // SV - Sacramento Valley
    {"ALPINE", "SV"},
    {"AMADOR", "SV"},
    {"BUTTE", "SV"},
    {"CALAVERAS", "SV"},
    {"COLUSA", "SV"},
    {"DEL NORTE", "SV"},
    {"EL DORADO", "SV"},
    {"GLENN", "SV"},
    {"HUMBOLDT", "SV"},
    {"INYO", "SV"},
    {"LAKE", "SV"},
    {"LASSEN", "SV"},
    {"MENDOCINO", "SV"},
    {"MODOC", "SV"},
    {"MONO", "SV"},
    {"MONTEREY", "SV"},
    {"NAPA", "SV"},
    {"NEVADA", "SV"},
    {"PLACER", "SV"},
    {"PLUMAS", "SV"},
    {"RIVERSIDE", "SV"},
    {"SACRAMENTO", "SV"},
    {"SAN BERNARDINO", "SV"},
    {"SHASTA", "SV"},
    {"SIERRA", "SV"},
    {"SISKIYOU", "SV"},
    {"SOLANO", "SV"},
    {"SONOMA", "SV"},
    {"SUTTER", "SV"},
    {"TEHAMA", "SV"},
    {"TRINITY", "SV"},
    {"TUOLUMNE", "SV"},
    {"YOLO", "SV"},
    {"YUBA", "SV"},

    // PAC - Pacific (Santa Cruz area)
    {"SANTA CRUZ", "PAC"}
};

// ============================================================================
// TEXAS (3 sections: NTX, STX, WTX)
// ============================================================================
// Source: http://www.arrl.org/texas-section

static const QHash<QString, QString> texasCounties = {
    // NTX - North Texas
    {"ARCHER", "NTX"},
    {"BAYLOR", "NTX"},
    {"BOWIE", "NTX"},
    {"CLAY", "NTX"},
    {"COLLIN", "NTX"},
    {"COOKE", "NTX"},
    {"DALLAS", "NTX"},
    {"DELTA", "NTX"},
    {"DENTON", "NTX"},
    {"ELLIS", "NTX"},
    {"ERATH", "NTX"},
    {"FANNIN", "NTX"},
    {"GRAYSON", "NTX"},
    {"HARDEMAN", "NTX"},
    {"HOOD", "NTX"},
    {"HUNT", "NTX"},
    {"JACK", "NTX"},
    {"JOHNSON", "NTX"},
    {"KAUFMAN", "NTX"},
    {"LAMAR", "NTX"},
    {"MONTAGUE", "NTX"},
    {"NAVARRO", "NTX"},
    {"PALO PINTO", "NTX"},
    {"PARKER", "NTX"},
    {"RAINS", "NTX"},
    {"RED RIVER", "NTX"},
    {"ROCKWALL", "NTX"},
    {"SOMERVELL", "NTX"},
    {"TARRANT", "NTX"},
    {"THROCKMORTON", "NTX"},
    {"VAN ZANDT", "NTX"},
    {"WICHITA", "NTX"},
    {"WILBARGER", "NTX"},
    {"WISE", "NTX"},
    {"YOUNG", "NTX"},

    // STX - South Texas (all other counties not in NTX or WTX)
    {"ANDERSON", "STX"},
    {"ANGELINA", "STX"},
    {"ARANSAS", "STX"},
    {"ATASCOSA", "STX"},
    {"AUSTIN", "STX"},
    {"BANDERA", "STX"},
    {"BASTROP", "STX"},
    {"BEE", "STX"},
    {"BELL", "STX"},
    {"BEXAR", "STX"},
    {"BLANCO", "STX"},
    {"BOSQUE", "STX"},
    {"BRAZORIA", "STX"},
    {"BRAZOS", "STX"},
    {"BROOKS", "STX"},
    {"BROWN", "STX"},
    {"BURLESON", "STX"},
    {"BURNET", "STX"},
    {"CALDWELL", "STX"},
    {"CALHOUN", "STX"},
    {"CAMERON", "STX"},
    {"CAMP", "STX"},
    {"CASS", "STX"},
    {"CHAMBERS", "STX"},
    {"CHEROKEE", "STX"},
    {"COLORADO", "STX"},
    {"COMAL", "STX"},
    {"COMANCHE", "STX"},
    {"CORYELL", "STX"},
    {"DEWITT", "STX"},
    {"DUVAL", "STX"},
    {"EASTLAND", "STX"},
    {"FALLS", "STX"},
    {"FAYETTE", "STX"},
    {"FORT BEND", "STX"},
    {"FRANKLIN", "STX"},
    {"FREESTONE", "STX"},
    {"FRIO", "STX"},
    {"GALVESTON", "STX"},
    {"GILLESPIE", "STX"},
    {"GOLIAD", "STX"},
    {"GONZALES", "STX"},
    {"GRIMES", "STX"},
    {"GUADALUPE", "STX"},
    {"HAMILTON", "STX"},
    {"HARDIN", "STX"},
    {"HARRIS", "STX"},
    {"HARRISON", "STX"},
    {"HAYS", "STX"},
    {"HENDERSON", "STX"},
    {"HIDALGO", "STX"},
    {"HILL", "STX"},
    {"HOPKINS", "STX"},
    {"HOUSTON", "STX"},
    {"JACKSON", "STX"},
    {"JASPER", "STX"},
    {"JEFFERSON", "STX"},
    {"JIM HOGG", "STX"},
    {"JIM WELLS", "STX"},
    {"KARNES", "STX"},
    {"KENEDY", "STX"},
    {"KERR", "STX"},
    {"KIMBLE", "STX"},
    {"KLEBERG", "STX"},
    {"LAMPASAS", "STX"},
    {"LAVACA", "STX"},
    {"LEE", "STX"},
    {"LEON", "STX"},
    {"LIBERTY", "STX"},
    {"LIMESTONE", "STX"},
    {"LIVE OAK", "STX"},
    {"LLANO", "STX"},
    {"MADISON", "STX"},
    {"MARION", "STX"},
    {"MASON", "STX"},
    {"MATAGORDA", "STX"},
    {"MAVERICK", "STX"},
    {"MCLENNAN", "STX"},
    {"MCMULLEN", "STX"},
    {"MILAM", "STX"},
    {"MILLS", "STX"},
    {"MONTGOMERY", "STX"},
    {"MORRIS", "STX"},
    {"NACOGDOCHES", "STX"},
    {"NEWTON", "STX"},
    {"NUECES", "STX"},
    {"ORANGE", "STX"},
    {"PANOLA", "STX"},
    {"POLK", "STX"},
    {"REFUGIO", "STX"},
    {"ROBERTSON", "STX"},
    {"RUSK", "STX"},
    {"SABINE", "STX"},
    {"SAN AUGUSTINE", "STX"},
    {"SAN JACINTO", "STX"},
    {"SAN PATRICIO", "STX"},
    {"SAN SABA", "STX"},
    {"SHELBY", "STX"},
    {"SMITH", "STX"},
    {"STARR", "STX"},
    {"STEPHENS", "STX"},
    {"TEXAS", "STX"},
    {"TITUS", "STX"},
    {"TRAVIS", "STX"},
    {"TRINITY", "STX"},
    {"TYLER", "STX"},
    {"UPSHUR", "STX"},
    {"UVALDE", "STX"},
    {"VAL VERDE", "STX"},
    {"VICTORIA", "STX"},
    {"WALKER", "STX"},
    {"WALLER", "STX"},
    {"WASHINGTON", "STX"},
    {"WEBB", "STX"},
    {"WHARTON", "STX"},
    {"WILLIAMSON", "STX"},
    {"WILSON", "STX"},
    {"WOOD", "STX"},
    {"ZAPATA", "STX"},

    // WTX - West Texas
    {"ANDREWS", "WTX"},
    {"BAILEY", "WTX"},
    {"BORDEN", "WTX"},
    {"BREWSTER", "WTX"},
    {"BRISCOE", "WTX"},
    {"CALLAHAN", "WTX"},
    {"CARSON", "WTX"},
    {"CASTRO", "WTX"},
    {"CHILDRESS", "WTX"},
    {"COCHRAN", "WTX"},
    {"COKE", "WTX"},
    {"COLEMAN", "WTX"},
    {"COLLINGSWORTH", "WTX"},
    {"CONCHO", "WTX"},
    {"COTTLE", "WTX"},
    {"CRANE", "WTX"},
    {"CROCKETT", "WTX"},
    {"CROSBY", "WTX"},
    {"CULBERSON", "WTX"},
    {"DALLAM", "WTX"},
    {"DAWSON", "WTX"},
    {"DEAF SMITH", "WTX"},
    {"DICKENS", "WTX"},
    {"DONLEY", "WTX"},
    {"ECTOR", "WTX"},
    {"EDWARDS", "WTX"},
    {"EL PASO", "WTX"},
    {"FISHER", "WTX"},
    {"FLOYD", "WTX"},
    {"FOARD", "WTX"},
    {"GAINES", "WTX"},
    {"GARZA", "WTX"},
    {"GLASSCOCK", "WTX"},
    {"GRAY", "WTX"},
    {"HALE", "WTX"},
    {"HALL", "WTX"},
    {"HANSFORD", "WTX"},
    {"HARTLEY", "WTX"},
    {"HASKELL", "WTX"},
    {"HEMPHILL", "WTX"},
    {"HOCKLEY", "WTX"},
    {"HOWARD", "WTX"},
    {"HUDSPETH", "WTX"},
    {"HUTCHINSON", "WTX"},
    {"IRION", "WTX"},
    {"JEFF DAVIS", "WTX"},
    {"JONES", "WTX"},
    {"KENT", "WTX"},
    {"KING", "WTX"},
    {"KINNEY", "WTX"},
    {"KNOX", "WTX"},
    {"LAMB", "WTX"},
    {"LIPSCOMB", "WTX"},
    {"LOVING", "WTX"},
    {"LUBBOCK", "WTX"},
    {"LYNN", "WTX"},
    {"MARTIN", "WTX"},
    {"MCCULLOCH", "WTX"},
    {"MENARD", "WTX"},
    {"MIDLAND", "WTX"},
    {"MITCHELL", "WTX"},
    {"MOORE", "WTX"},
    {"MOTLEY", "WTX"},
    {"NOLAN", "WTX"},
    {"OCHILTREE", "WTX"},
    {"OLDHAM", "WTX"},
    {"PARMER", "WTX"},
    {"PECOS", "WTX"},
    {"POTTER", "WTX"},
    {"PRESIDIO", "WTX"},
    {"RANDALL", "WTX"},
    {"REAGAN", "WTX"},
    {"REAL", "WTX"},
    {"REEVES", "WTX"},
    {"ROBERTS", "WTX"},
    {"RUNNELS", "WTX"},
    {"SCHLEICHER", "WTX"},
    {"SCURRY", "WTX"},
    {"SHACKELFORD", "WTX"},
    {"SHERMAN", "WTX"},
    {"STERLING", "WTX"},
    {"STONEWALL", "WTX"},
    {"SUTTON", "WTX"},
    {"SWISHER", "WTX"},
    {"TAYLOR", "WTX"},
    {"TERRELL", "WTX"},
    {"TERRY", "WTX"},
    {"TOM GREEN", "WTX"},
    {"UPTON", "WTX"},
    {"WARD", "WTX"},
    {"WHEELER", "WTX"},
    {"WINKLER", "WTX"},
    {"YOAKUM", "WTX"}
};

// ============================================================================
// NEW YORK (4 sections: NLI, NNY, WNY, ENY)
// ============================================================================
// Source: http://www.arrl.org/new-york-section

static const QHash<QString, QString> newYorkCounties = {
    // NLI - New York City-Long Island
    {"BRONX", "NLI"},
    {"KINGS", "NLI"},
    {"NASSAU", "NLI"},
    {"NEW YORK", "NLI"},
    {"QUEENS", "NLI"},
    {"RICHMOND", "NLI"},
    {"SUFFOLK", "NLI"},

    // NNY - Northern New York
    {"CLINTON", "NNY"},
    {"ESSEX", "NNY"},
    {"FRANKLIN", "NNY"},
    {"FULTON", "NNY"},
    {"HAMILTON", "NNY"},
    {"JEFFERSON", "NNY"},
    {"LEWIS", "NNY"},
    {"SARATOGA", "NNY"},
    {"ST. LAWRENCE", "NNY"},
    {"ST LAWRENCE", "NNY"},
    {"SAINT LAWRENCE", "NNY"},
    {"WARREN", "NNY"},
    {"WASHINGTON", "NNY"},

    // WNY - Western New York
    {"ALLEGANY", "WNY"},
    {"CATTARAUGUS", "WNY"},
    {"CHAUTAUQUA", "WNY"},
    {"ERIE", "WNY"},
    {"GENESEE", "WNY"},
    {"LIVINGSTON", "WNY"},
    {"MONROE", "WNY"},
    {"NIAGARA", "WNY"},
    {"ONTARIO", "WNY"},
    {"ORLEANS", "WNY"},
    {"SENECA", "WNY"},
    {"STEUBEN", "WNY"},
    {"WAYNE", "WNY"},
    {"WYOMING", "WNY"},
    {"YATES", "WNY"},

    // ENY - Eastern New York
    {"ALBANY", "ENY"},
    {"BROOME", "ENY"},
    {"CAYUGA", "ENY"},
    {"CHEMUNG", "ENY"},
    {"CHENANGO", "ENY"},
    {"COLUMBIA", "ENY"},
    {"CORTLAND", "ENY"},
    {"DELAWARE", "ENY"},
    {"DUTCHESS", "ENY"},
    {"GREENE", "ENY"},
    {"HERKIMER", "ENY"},
    {"MADISON", "ENY"},
    {"MONTGOMERY", "ENY"},
    {"ONEIDA", "ENY"},
    {"ONONDAGA", "ENY"},
    {"ORANGE", "ENY"},
    {"OSWEGO", "ENY"},
    {"OTSEGO", "ENY"},
    {"PUTNAM", "ENY"},
    {"RENSSELAER", "ENY"},
    {"ROCKLAND", "ENY"},
    {"SCHENECTADY", "ENY"},
    {"SCHOHARIE", "ENY"},
    {"SCHUYLER", "ENY"},
    {"SULLIVAN", "ENY"},
    {"TIOGA", "ENY"},
    {"TOMPKINS", "ENY"},
    {"ULSTER", "ENY"},
    {"WESTCHESTER", "ENY"}
};

// ============================================================================
// NEW JERSEY (2 sections: NNJ, SNJ)
// ============================================================================
// Source: http://www.arrl.org/new-jersey-section

static const QHash<QString, QString> newJerseyCounties = {
    // NNJ - Northern New Jersey
    {"BERGEN", "NNJ"},
    {"ESSEX", "NNJ"},
    {"HUDSON", "NNJ"},
    {"HUNTERDON", "NNJ"},
    {"MIDDLESEX", "NNJ"},
    {"MORRIS", "NNJ"},
    {"PASSAIC", "NNJ"},
    {"SOMERSET", "NNJ"},
    {"SUSSEX", "NNJ"},
    {"UNION", "NNJ"},
    {"WARREN", "NNJ"},

    // SNJ - Southern New Jersey
    {"ATLANTIC", "SNJ"},
    {"BURLINGTON", "SNJ"},
    {"CAMDEN", "SNJ"},
    {"CAPE MAY", "SNJ"},
    {"CUMBERLAND", "SNJ"},
    {"GLOUCESTER", "SNJ"},
    {"MERCER", "SNJ"},
    {"MONMOUTH", "SNJ"},
    {"OCEAN", "SNJ"},
    {"SALEM", "SNJ"}
};

// ============================================================================
// MASSACHUSETTS (2 sections: EMA, WMA)
// ============================================================================
// Source: http://www.arrl.org/massachusetts-section

static const QHash<QString, QString> massachusettsCounties = {
    // EMA - Eastern Massachusetts
    {"BARNSTABLE", "EMA"},
    {"BRISTOL", "EMA"},
    {"DUKES", "EMA"},
    {"ESSEX", "EMA"},
    {"MIDDLESEX", "EMA"},
    {"NANTUCKET", "EMA"},
    {"NORFOLK", "EMA"},
    {"PLYMOUTH", "EMA"},
    {"SUFFOLK", "EMA"},

    // WMA - Western Massachusetts
    {"BERKSHIRE", "WMA"},
    {"FRANKLIN", "WMA"},
    {"HAMPDEN", "WMA"},
    {"HAMPSHIRE", "WMA"},
    {"WORCESTER", "WMA"}
};

// ============================================================================
// PENNSYLVANIA (2 sections: EPA, WPA)
// ============================================================================
// Source: http://www.arrl.org/pennsylvania-section

static const QHash<QString, QString> pennsylvaniaCounties = {
    // EPA - Eastern Pennsylvania
    {"BERKS", "EPA"},
    {"BUCKS", "EPA"},
    {"CARBON", "EPA"},
    {"CHESTER", "EPA"},
    {"DELAWARE", "EPA"},
    {"LACKAWANNA", "EPA"},
    {"LEHIGH", "EPA"},
    {"LUZERNE", "EPA"},
    {"MONROE", "EPA"},
    {"MONTGOMERY", "EPA"},
    {"NORTHAMPTON", "EPA"},
    {"PHILADELPHIA", "EPA"},
    {"PIKE", "EPA"},
    {"SCHUYLKILL", "EPA"},
    {"SUSQUEHANNA", "EPA"},
    {"WAYNE", "EPA"},
    {"WYOMING", "EPA"},

    // WPA - Western Pennsylvania
    {"ADAMS", "WPA"},
    {"ALLEGHENY", "WPA"},
    {"ARMSTRONG", "WPA"},
    {"BEAVER", "WPA"},
    {"BEDFORD", "WPA"},
    {"BLAIR", "WPA"},
    {"BRADFORD", "WPA"},
    {"BUTLER", "WPA"},
    {"CAMBRIA", "WPA"},
    {"CAMERON", "WPA"},
    {"CENTRE", "WPA"},
    {"CLARION", "WPA"},
    {"CLEARFIELD", "WPA"},
    {"CLINTON", "WPA"},
    {"COLUMBIA", "WPA"},
    {"CRAWFORD", "WPA"},
    {"CUMBERLAND", "WPA"},
    {"DAUPHIN", "WPA"},
    {"ELK", "WPA"},
    {"ERIE", "WPA"},
    {"FAYETTE", "WPA"},
    {"FOREST", "WPA"},
    {"FRANKLIN", "WPA"},
    {"FULTON", "WPA"},
    {"GREENE", "WPA"},
    {"HUNTINGDON", "WPA"},
    {"INDIANA", "WPA"},
    {"JEFFERSON", "WPA"},
    {"JUNIATA", "WPA"},
    {"LANCASTER", "WPA"},
    {"LAWRENCE", "WPA"},
    {"LEBANON", "WPA"},
    {"LYCOMING", "WPA"},
    {"MCKEAN", "WPA"},
    {"MERCER", "WPA"},
    {"MIFFLIN", "WPA"},
    {"MONTOUR", "WPA"},
    {"NORTHUMBERLAND", "WPA"},
    {"PERRY", "WPA"},
    {"POTTER", "WPA"},
    {"SNYDER", "WPA"},
    {"SOMERSET", "WPA"},
    {"SULLIVAN", "WPA"},
    {"TIOGA", "WPA"},
    {"UNION", "WPA"},
    {"VENANGO", "WPA"},
    {"WARREN", "WPA"},
    {"WASHINGTON", "WPA"},
    {"WESTMORELAND", "WPA"},
    {"YORK", "WPA"}
};

// ============================================================================
// WASHINGTON (2 sections: EWA, WWA)
// ============================================================================
// Source: http://www.arrl.org/washington-section
// Division: Cascade Mountains

static const QHash<QString, QString> washingtonCounties = {
    // EWA - Eastern Washington (east of Cascades)
    {"ADAMS", "EWA"},
    {"ASOTIN", "EWA"},
    {"BENTON", "EWA"},
    {"CHELAN", "EWA"},
    {"COLUMBIA", "EWA"},
    {"DOUGLAS", "EWA"},
    {"FERRY", "EWA"},
    {"FRANKLIN", "EWA"},
    {"GARFIELD", "EWA"},
    {"GRANT", "EWA"},
    {"KITTITAS", "EWA"},
    {"KLICKITAT", "EWA"},
    {"LINCOLN", "EWA"},
    {"OKANOGAN", "EWA"},
    {"PEND OREILLE", "EWA"},
    {"SPOKANE", "EWA"},
    {"STEVENS", "EWA"},
    {"WALLA WALLA", "EWA"},
    {"WHITMAN", "EWA"},
    {"YAKIMA", "EWA"},

    // WWA - Western Washington (west of Cascades)
    {"CLALLAM", "WWA"},
    {"CLARK", "WWA"},
    {"COWLITZ", "WWA"},
    {"GRAYS HARBOR", "WWA"},
    {"ISLAND", "WWA"},
    {"JEFFERSON", "WWA"},
    {"KING", "WWA"},
    {"KITSAP", "WWA"},
    {"LEWIS", "WWA"},
    {"MASON", "WWA"},
    {"PACIFIC", "WWA"},
    {"PIERCE", "WWA"},
    {"SAN JUAN", "WWA"},
    {"SKAGIT", "WWA"},
    {"SKAMANIA", "WWA"},
    {"SNOHOMISH", "WWA"},
    {"THURSTON", "WWA"},
    {"WAHKIAKUM", "WWA"},
    {"WHATCOM", "WWA"}
};

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

/**
 * Normalize county name for consistent lookup:
 * - Convert to uppercase
 * - Trim whitespace
 * - Remove the word "COUNTY" (case-insensitive)
 * - Normalize punctuation (St. -> ST, etc.)
 */
static QString normalizeCounty(const QString& county)
{
    QString normalized = county.trimmed().toUpper();

    // Remove "COUNTY" suffix (with optional comma)
    normalized.replace(QRegularExpression("\\s*,?\\s*COUNTY\\s*$"), "");

    // Normalize "St." and "Saint" to "ST"
    normalized.replace(QRegularExpression("\\bSAINT\\s+"), "ST ");
    normalized.replace(QRegularExpression("\\bST\\.\\s+"), "ST ");

    return normalized.trimmed();
}

/**
 * Normalize state abbreviation:
 * - Convert to uppercase
 * - Trim whitespace
 */
static QString normalizeState(const QString& state)
{
    return state.trimmed().toUpper();
}

// ============================================================================
// PUBLIC API
// ============================================================================

QString sectionForStateCounty(const QString& stateAbbrev, const QString& countyName)
{
    QString state = normalizeState(stateAbbrev);
    QString county = normalizeCounty(countyName);

    // Check subdivided states by county
    if (state == "FL") {
        return floridaCounties.value(county, QString());
    }
    else if (state == "CA") {
        return californiaCounties.value(county, QString());
    }
    else if (state == "TX") {
        return texasCounties.value(county, QString());
    }
    else if (state == "NY") {
        return newYorkCounties.value(county, QString());
    }
    else if (state == "NJ") {
        return newJerseyCounties.value(county, QString());
    }
    else if (state == "MA") {
        return massachusettsCounties.value(county, QString());
    }
    else if (state == "PA") {
        return pennsylvaniaCounties.value(county, QString());
    }
    else if (state == "WA") {
        return washingtonCounties.value(county, QString());
    }

    // Fall back to simple state-level lookup for non-subdivided states
    return simpleStateSections.value(state, QString());
}

QString sectionForState(const QString& stateAbbrev)
{
    QString state = normalizeState(stateAbbrev);
    return simpleStateSections.value(state, QString());
}

QStringList getAllSections()
{
    // Complete list of ARRL (US) and RAC (Canadian) sections
    static QStringList sections = {
        // US ARRL Sections
        // New England Division
        "CT", "EMA", "ME", "NH", "RI", "VT", "WMA",
        // Atlantic Division
        "ENY", "NLI", "NNJ", "NNY", "SNJ", "WNY",
        // Roanoke Division
        "DE", "EPA", "MDC", "WPA",
        // Delta Division
        "AL", "GA", "KY", "NC", "NFL", "SC", "SFL", "WCF", "TN", "VA", "PR", "VI",
        // West Gulf Division
        "AR", "LA", "MS", "NM", "NTX", "OK", "STX", "WTX",
        // Pacific Division
        "EB", "LAX", "ORG", "SB", "SCV", "SDG", "SF", "SJV", "SV", "PAC",
        // Northwestern Division
        "AK", "EWA", "ID", "MT", "NV", "OR", "UT", "WWA", "WY",
        // Great Lakes Division
        "MI", "OH", "WV",
        // Central Division
        "IL", "IN", "WI",
        // Midwest Division
        "CO", "IA", "KS", "MN", "MO", "ND", "NE", "SD",
        // Other
        "AZ", "HI",
        // Canadian RAC Sections
        "AB", "BC", "MB", "NB", "NL", "NLI", "NS", "NT", "ON", "PE", "QC", "SK", "YT",
        // Ontario Subdivisions (like US states, Ontario is subdivided)
        "GH", "ONE", "ONN", "ONS",
        // DX
        "DX"
    };

    return sections;
}

bool isValidSection(const QString& section)
{
    static QStringList validSections = getAllSections();
    return validSections.contains(section.toUpper());
}

QStringList getUSStates()
{
    // 50 US states as 2-letter postal codes (alphabetical)
    static QStringList states = {
        "AL", "AK", "AZ", "AR", "CA", "CO", "CT", "DE", "FL", "GA",
        "HI", "ID", "IL", "IN", "IA", "KS", "KY", "LA", "ME", "MD",
        "MA", "MI", "MN", "MS", "MO", "MT", "NE", "NV", "NH", "NJ",
        "NM", "NY", "NC", "ND", "OH", "OK", "OR", "PA", "RI", "SC",
        "SD", "TN", "TX", "UT", "VT", "VA", "WA", "WV", "WI", "WY"
    };
    return states;
}

QStringList getCanadianProvinces()
{
    // Canadian provinces/territories + Ontario subdivisions (alphabetical)
    static QStringList provinces = {
        "AB",   // Alberta
        "BC",   // British Columbia
        "GH",   // Greater Toronto Area / Golden Horseshoe (Ontario subdivision)
        "MB",   // Manitoba
        "NB",   // New Brunswick
        "NL",   // Newfoundland and Labrador
        "NS",   // Nova Scotia
        "NT",   // Northwest Territories
        "ONE",  // Ontario East
        "ONN",  // Ontario North
        "ONS",  // Ontario South
        "PE",   // Prince Edward Island
        "QC",   // Quebec
        "SK",   // Saskatchewan
        "YT"    // Yukon
    };
    return provinces;
}

QStringList getStatesAndProvinces()
{
    // Combined list for contests like RTTY Roundup, NAQP
    QStringList combined = getUSStates();
    combined.append(getCanadianProvinces());
    combined.sort();  // Keep alphabetical
    return combined;
}

} // namespace Arrl
} // namespace TR4QT
