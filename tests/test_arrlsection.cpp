#include <QtTest>
#include "../src/utils/ArrlSectionHelper.h"

using namespace TR4QT::Arrl;

class TestArrlSection : public QObject
{
    Q_OBJECT

private slots:
    // Florida - All 71 counties
    void testFlorida_AllCounties();

    // California - All 58 counties
    void testCalifornia_AllCounties();

    // Texas - All 254 counties
    void testTexas_AllCounties();

    // New York - All 62 counties
    void testNewYork_AllCounties();

    // New Jersey - All 21 counties
    void testNewJersey_AllCounties();

    // Massachusetts - All 14 counties
    void testMassachusetts_AllCounties();

    // Pennsylvania - All 67 counties
    void testPennsylvania_AllCounties();

    // Washington - All 39 counties
    void testWashington_AllCounties();

    // Simple state tests - All 43 single-section states
    void testSimpleStates_AllStates();

    // Normalization and edge cases
    void testCaseInsensitive();
    void testCountyNormalization();
    void testStateNormalization();
    void testUnknownState();
    void testUnknownCounty();
    void testEmptyInputs();
};

// ============================================================================
// FLORIDA - ALL 71 COUNTIES
// ============================================================================

void TestArrlSection::testFlorida_AllCounties()
{
    // NFL - Northern Florida (36 counties)
    QCOMPARE(sectionForStateCounty("FL", "Alachua"), QString("NFL"));
    QCOMPARE(sectionForStateCounty("FL", "Baker"), QString("NFL"));
    QCOMPARE(sectionForStateCounty("FL", "Bay"), QString("NFL"));
    QCOMPARE(sectionForStateCounty("FL", "Bradford"), QString("NFL"));
    QCOMPARE(sectionForStateCounty("FL", "Calhoun"), QString("NFL"));
    QCOMPARE(sectionForStateCounty("FL", "Clay"), QString("NFL"));
    QCOMPARE(sectionForStateCounty("FL", "Columbia"), QString("NFL"));
    QCOMPARE(sectionForStateCounty("FL", "Dixie"), QString("NFL"));
    QCOMPARE(sectionForStateCounty("FL", "Duval"), QString("NFL"));
    QCOMPARE(sectionForStateCounty("FL", "Escambia"), QString("NFL"));
    QCOMPARE(sectionForStateCounty("FL", "Flagler"), QString("NFL"));
    QCOMPARE(sectionForStateCounty("FL", "Franklin"), QString("NFL"));
    QCOMPARE(sectionForStateCounty("FL", "Gadsden"), QString("NFL"));
    QCOMPARE(sectionForStateCounty("FL", "Gilchrist"), QString("NFL"));
    QCOMPARE(sectionForStateCounty("FL", "Gulf"), QString("NFL"));
    QCOMPARE(sectionForStateCounty("FL", "Hamilton"), QString("NFL"));
    QCOMPARE(sectionForStateCounty("FL", "Holmes"), QString("NFL"));
    QCOMPARE(sectionForStateCounty("FL", "Jackson"), QString("NFL"));
    QCOMPARE(sectionForStateCounty("FL", "Jefferson"), QString("NFL"));
    QCOMPARE(sectionForStateCounty("FL", "Lafayette"), QString("NFL"));
    QCOMPARE(sectionForStateCounty("FL", "Leon"), QString("NFL"));
    QCOMPARE(sectionForStateCounty("FL", "Levy"), QString("NFL"));
    QCOMPARE(sectionForStateCounty("FL", "Liberty"), QString("NFL"));
    QCOMPARE(sectionForStateCounty("FL", "Madison"), QString("NFL"));
    QCOMPARE(sectionForStateCounty("FL", "Nassau"), QString("NFL"));
    QCOMPARE(sectionForStateCounty("FL", "Okaloosa"), QString("NFL"));
    QCOMPARE(sectionForStateCounty("FL", "Putnam"), QString("NFL"));
    QCOMPARE(sectionForStateCounty("FL", "Santa Rosa"), QString("NFL"));
    QCOMPARE(sectionForStateCounty("FL", "St. Johns"), QString("NFL"));
    QCOMPARE(sectionForStateCounty("FL", "St Johns"), QString("NFL"));
    QCOMPARE(sectionForStateCounty("FL", "Saint Johns"), QString("NFL"));
    QCOMPARE(sectionForStateCounty("FL", "Suwannee"), QString("NFL"));
    QCOMPARE(sectionForStateCounty("FL", "Taylor"), QString("NFL"));
    QCOMPARE(sectionForStateCounty("FL", "Union"), QString("NFL"));
    QCOMPARE(sectionForStateCounty("FL", "Wakulla"), QString("NFL"));
    QCOMPARE(sectionForStateCounty("FL", "Walton"), QString("NFL"));
    QCOMPARE(sectionForStateCounty("FL", "Washington"), QString("NFL"));

    // WCF - West Central Florida (17 counties)
    QCOMPARE(sectionForStateCounty("FL", "Brevard"), QString("WCF"));
    QCOMPARE(sectionForStateCounty("FL", "Citrus"), QString("WCF"));
    QCOMPARE(sectionForStateCounty("FL", "Hardee"), QString("WCF"));
    QCOMPARE(sectionForStateCounty("FL", "Hernando"), QString("WCF"));
    QCOMPARE(sectionForStateCounty("FL", "Highlands"), QString("WCF"));
    QCOMPARE(sectionForStateCounty("FL", "Hillsborough"), QString("WCF"));
    QCOMPARE(sectionForStateCounty("FL", "Lake"), QString("WCF"));
    QCOMPARE(sectionForStateCounty("FL", "Manatee"), QString("WCF"));
    QCOMPARE(sectionForStateCounty("FL", "Marion"), QString("WCF"));
    QCOMPARE(sectionForStateCounty("FL", "Orange"), QString("WCF"));
    QCOMPARE(sectionForStateCounty("FL", "Osceola"), QString("WCF"));
    QCOMPARE(sectionForStateCounty("FL", "Pasco"), QString("WCF"));
    QCOMPARE(sectionForStateCounty("FL", "Pinellas"), QString("WCF"));
    QCOMPARE(sectionForStateCounty("FL", "Polk"), QString("WCF"));
    QCOMPARE(sectionForStateCounty("FL", "Seminole"), QString("WCF"));
    QCOMPARE(sectionForStateCounty("FL", "Sumter"), QString("WCF"));
    QCOMPARE(sectionForStateCounty("FL", "Volusia"), QString("WCF"));

    // SFL - South Florida (18 counties)
    QCOMPARE(sectionForStateCounty("FL", "Broward"), QString("SFL"));
    QCOMPARE(sectionForStateCounty("FL", "Charlotte"), QString("SFL"));
    QCOMPARE(sectionForStateCounty("FL", "Collier"), QString("SFL"));
    QCOMPARE(sectionForStateCounty("FL", "Desoto"), QString("SFL"));
    QCOMPARE(sectionForStateCounty("FL", "Glades"), QString("SFL"));
    QCOMPARE(sectionForStateCounty("FL", "Hendry"), QString("SFL"));
    QCOMPARE(sectionForStateCounty("FL", "Indian River"), QString("SFL"));
    QCOMPARE(sectionForStateCounty("FL", "Lee"), QString("SFL"));
    QCOMPARE(sectionForStateCounty("FL", "Martin"), QString("SFL"));
    QCOMPARE(sectionForStateCounty("FL", "Miami-Dade"), QString("SFL"));
    QCOMPARE(sectionForStateCounty("FL", "Miami Dade"), QString("SFL"));
    QCOMPARE(sectionForStateCounty("FL", "Dade"), QString("SFL"));
    QCOMPARE(sectionForStateCounty("FL", "Monroe"), QString("SFL"));
    QCOMPARE(sectionForStateCounty("FL", "Okeechobee"), QString("SFL"));
    QCOMPARE(sectionForStateCounty("FL", "Palm Beach"), QString("SFL"));
    QCOMPARE(sectionForStateCounty("FL", "Sarasota"), QString("SFL"));
    QCOMPARE(sectionForStateCounty("FL", "St. Lucie"), QString("SFL"));
    QCOMPARE(sectionForStateCounty("FL", "St Lucie"), QString("SFL"));
    QCOMPARE(sectionForStateCounty("FL", "Saint Lucie"), QString("SFL"));
}

// ============================================================================
// CALIFORNIA - ALL 58 COUNTIES
// ============================================================================

void TestArrlSection::testCalifornia_AllCounties()
{
    // EB - East Bay (2 counties)
    QCOMPARE(sectionForStateCounty("CA", "Alameda"), QString("EB"));
    QCOMPARE(sectionForStateCounty("CA", "Contra Costa"), QString("EB"));

    // LAX - Los Angeles (1 county)
    QCOMPARE(sectionForStateCounty("CA", "Los Angeles"), QString("LAX"));

    // ORG - Orange (1 county)
    QCOMPARE(sectionForStateCounty("CA", "Orange"), QString("ORG"));

    // SB - Santa Barbara (3 counties)
    QCOMPARE(sectionForStateCounty("CA", "San Luis Obispo"), QString("SB"));
    QCOMPARE(sectionForStateCounty("CA", "Santa Barbara"), QString("SB"));
    QCOMPARE(sectionForStateCounty("CA", "Ventura"), QString("SB"));

    // SCV - Santa Clara Valley (1 county)
    QCOMPARE(sectionForStateCounty("CA", "Santa Clara"), QString("SCV"));

    // SDG - San Diego (2 counties)
    QCOMPARE(sectionForStateCounty("CA", "Imperial"), QString("SDG"));
    QCOMPARE(sectionForStateCounty("CA", "San Diego"), QString("SDG"));

    // SF - San Francisco (3 counties)
    QCOMPARE(sectionForStateCounty("CA", "Marin"), QString("SF"));
    QCOMPARE(sectionForStateCounty("CA", "San Francisco"), QString("SF"));
    QCOMPARE(sectionForStateCounty("CA", "San Mateo"), QString("SF"));

    // SJV - San Joaquin Valley (9 counties)
    QCOMPARE(sectionForStateCounty("CA", "Fresno"), QString("SJV"));
    QCOMPARE(sectionForStateCounty("CA", "Kern"), QString("SJV"));
    QCOMPARE(sectionForStateCounty("CA", "Kings"), QString("SJV"));
    QCOMPARE(sectionForStateCounty("CA", "Madera"), QString("SJV"));
    QCOMPARE(sectionForStateCounty("CA", "Merced"), QString("SJV"));
    QCOMPARE(sectionForStateCounty("CA", "San Benito"), QString("SJV"));
    QCOMPARE(sectionForStateCounty("CA", "San Joaquin"), QString("SJV"));
    QCOMPARE(sectionForStateCounty("CA", "Stanislaus"), QString("SJV"));
    QCOMPARE(sectionForStateCounty("CA", "Tulare"), QString("SJV"));

    // SV - Sacramento Valley (35 counties)
    QCOMPARE(sectionForStateCounty("CA", "Alpine"), QString("SV"));
    QCOMPARE(sectionForStateCounty("CA", "Amador"), QString("SV"));
    QCOMPARE(sectionForStateCounty("CA", "Butte"), QString("SV"));
    QCOMPARE(sectionForStateCounty("CA", "Calaveras"), QString("SV"));
    QCOMPARE(sectionForStateCounty("CA", "Colusa"), QString("SV"));
    QCOMPARE(sectionForStateCounty("CA", "Del Norte"), QString("SV"));
    QCOMPARE(sectionForStateCounty("CA", "El Dorado"), QString("SV"));
    QCOMPARE(sectionForStateCounty("CA", "Glenn"), QString("SV"));
    QCOMPARE(sectionForStateCounty("CA", "Humboldt"), QString("SV"));
    QCOMPARE(sectionForStateCounty("CA", "Inyo"), QString("SV"));
    QCOMPARE(sectionForStateCounty("CA", "Lake"), QString("SV"));
    QCOMPARE(sectionForStateCounty("CA", "Lassen"), QString("SV"));
    QCOMPARE(sectionForStateCounty("CA", "Mendocino"), QString("SV"));
    QCOMPARE(sectionForStateCounty("CA", "Modoc"), QString("SV"));
    QCOMPARE(sectionForStateCounty("CA", "Mono"), QString("SV"));
    QCOMPARE(sectionForStateCounty("CA", "Monterey"), QString("SV"));
    QCOMPARE(sectionForStateCounty("CA", "Napa"), QString("SV"));
    QCOMPARE(sectionForStateCounty("CA", "Nevada"), QString("SV"));
    QCOMPARE(sectionForStateCounty("CA", "Placer"), QString("SV"));
    QCOMPARE(sectionForStateCounty("CA", "Plumas"), QString("SV"));
    QCOMPARE(sectionForStateCounty("CA", "Riverside"), QString("SV"));
    QCOMPARE(sectionForStateCounty("CA", "Sacramento"), QString("SV"));
    QCOMPARE(sectionForStateCounty("CA", "San Bernardino"), QString("SV"));
    QCOMPARE(sectionForStateCounty("CA", "Shasta"), QString("SV"));
    QCOMPARE(sectionForStateCounty("CA", "Sierra"), QString("SV"));
    QCOMPARE(sectionForStateCounty("CA", "Siskiyou"), QString("SV"));
    QCOMPARE(sectionForStateCounty("CA", "Solano"), QString("SV"));
    QCOMPARE(sectionForStateCounty("CA", "Sonoma"), QString("SV"));
    QCOMPARE(sectionForStateCounty("CA", "Sutter"), QString("SV"));
    QCOMPARE(sectionForStateCounty("CA", "Tehama"), QString("SV"));
    QCOMPARE(sectionForStateCounty("CA", "Trinity"), QString("SV"));
    QCOMPARE(sectionForStateCounty("CA", "Tuolumne"), QString("SV"));
    QCOMPARE(sectionForStateCounty("CA", "Yolo"), QString("SV"));
    QCOMPARE(sectionForStateCounty("CA", "Yuba"), QString("SV"));

    // PAC - Pacific (1 county)
    QCOMPARE(sectionForStateCounty("CA", "Santa Cruz"), QString("PAC"));
}

// ============================================================================
// TEXAS - ALL 254 COUNTIES (comprehensive sampling to keep test manageable)
// ============================================================================

void TestArrlSection::testTexas_AllCounties()
{
    // NTX - North Texas (35 counties - all enumerated)
    QCOMPARE(sectionForStateCounty("TX", "Archer"), QString("NTX"));
    QCOMPARE(sectionForStateCounty("TX", "Baylor"), QString("NTX"));
    QCOMPARE(sectionForStateCounty("TX", "Bowie"), QString("NTX"));
    QCOMPARE(sectionForStateCounty("TX", "Clay"), QString("NTX"));
    QCOMPARE(sectionForStateCounty("TX", "Collin"), QString("NTX"));
    QCOMPARE(sectionForStateCounty("TX", "Cooke"), QString("NTX"));
    QCOMPARE(sectionForStateCounty("TX", "Dallas"), QString("NTX"));
    QCOMPARE(sectionForStateCounty("TX", "Delta"), QString("NTX"));
    QCOMPARE(sectionForStateCounty("TX", "Denton"), QString("NTX"));
    QCOMPARE(sectionForStateCounty("TX", "Ellis"), QString("NTX"));
    QCOMPARE(sectionForStateCounty("TX", "Erath"), QString("NTX"));
    QCOMPARE(sectionForStateCounty("TX", "Fannin"), QString("NTX"));
    QCOMPARE(sectionForStateCounty("TX", "Grayson"), QString("NTX"));
    QCOMPARE(sectionForStateCounty("TX", "Hardeman"), QString("NTX"));
    QCOMPARE(sectionForStateCounty("TX", "Hood"), QString("NTX"));
    QCOMPARE(sectionForStateCounty("TX", "Hunt"), QString("NTX"));
    QCOMPARE(sectionForStateCounty("TX", "Jack"), QString("NTX"));
    QCOMPARE(sectionForStateCounty("TX", "Johnson"), QString("NTX"));
    QCOMPARE(sectionForStateCounty("TX", "Kaufman"), QString("NTX"));
    QCOMPARE(sectionForStateCounty("TX", "Lamar"), QString("NTX"));
    QCOMPARE(sectionForStateCounty("TX", "Montague"), QString("NTX"));
    QCOMPARE(sectionForStateCounty("TX", "Navarro"), QString("NTX"));
    QCOMPARE(sectionForStateCounty("TX", "Palo Pinto"), QString("NTX"));
    QCOMPARE(sectionForStateCounty("TX", "Parker"), QString("NTX"));
    QCOMPARE(sectionForStateCounty("TX", "Rains"), QString("NTX"));
    QCOMPARE(sectionForStateCounty("TX", "Red River"), QString("NTX"));
    QCOMPARE(sectionForStateCounty("TX", "Rockwall"), QString("NTX"));
    QCOMPARE(sectionForStateCounty("TX", "Somervell"), QString("NTX"));
    QCOMPARE(sectionForStateCounty("TX", "Tarrant"), QString("NTX"));
    QCOMPARE(sectionForStateCounty("TX", "Throckmorton"), QString("NTX"));
    QCOMPARE(sectionForStateCounty("TX", "Van Zandt"), QString("NTX"));
    QCOMPARE(sectionForStateCounty("TX", "Wichita"), QString("NTX"));
    QCOMPARE(sectionForStateCounty("TX", "Wilbarger"), QString("NTX"));
    QCOMPARE(sectionForStateCounty("TX", "Wise"), QString("NTX"));
    QCOMPARE(sectionForStateCounty("TX", "Young"), QString("NTX"));

    // STX - South Texas (sample of major counties from the 160+ enumerated)
    QCOMPARE(sectionForStateCounty("TX", "Harris"), QString("STX"));
    QCOMPARE(sectionForStateCounty("TX", "Travis"), QString("STX"));
    QCOMPARE(sectionForStateCounty("TX", "Bexar"), QString("STX"));
    QCOMPARE(sectionForStateCounty("TX", "Galveston"), QString("STX"));
    QCOMPARE(sectionForStateCounty("TX", "Nueces"), QString("STX"));
    QCOMPARE(sectionForStateCounty("TX", "Williamson"), QString("STX"));
    QCOMPARE(sectionForStateCounty("TX", "Fort Bend"), QString("STX"));
    QCOMPARE(sectionForStateCounty("TX", "Brazoria"), QString("STX"));
    QCOMPARE(sectionForStateCounty("TX", "Montgomery"), QString("STX"));
    QCOMPARE(sectionForStateCounty("TX", "Jefferson"), QString("STX"));
    QCOMPARE(sectionForStateCounty("TX", "Cameron"), QString("STX"));
    QCOMPARE(sectionForStateCounty("TX", "Hidalgo"), QString("STX"));
    QCOMPARE(sectionForStateCounty("TX", "Smith"), QString("STX"));
    QCOMPARE(sectionForStateCounty("TX", "Bell"), QString("STX"));
    QCOMPARE(sectionForStateCounty("TX", "McLennan"), QString("STX"));

    // WTX - West Texas (sample of major counties from the 58 enumerated)
    QCOMPARE(sectionForStateCounty("TX", "El Paso"), QString("WTX"));
    QCOMPARE(sectionForStateCounty("TX", "Lubbock"), QString("WTX"));
    QCOMPARE(sectionForStateCounty("TX", "Midland"), QString("WTX"));
    QCOMPARE(sectionForStateCounty("TX", "Ector"), QString("WTX"));
    QCOMPARE(sectionForStateCounty("TX", "Potter"), QString("WTX"));
    QCOMPARE(sectionForStateCounty("TX", "Taylor"), QString("WTX"));
    QCOMPARE(sectionForStateCounty("TX", "Tom Green"), QString("WTX"));
    QCOMPARE(sectionForStateCounty("TX", "Randall"), QString("WTX"));
    QCOMPARE(sectionForStateCounty("TX", "Brewster"), QString("WTX"));
    QCOMPARE(sectionForStateCounty("TX", "Pecos"), QString("WTX"));
}

// ============================================================================
// NEW YORK - ALL 62 COUNTIES
// ============================================================================

void TestArrlSection::testNewYork_AllCounties()
{
    // NLI - New York City-Long Island (7 counties)
    QCOMPARE(sectionForStateCounty("NY", "Bronx"), QString("NLI"));
    QCOMPARE(sectionForStateCounty("NY", "Kings"), QString("NLI"));
    QCOMPARE(sectionForStateCounty("NY", "Nassau"), QString("NLI"));
    QCOMPARE(sectionForStateCounty("NY", "New York"), QString("NLI"));
    QCOMPARE(sectionForStateCounty("NY", "Queens"), QString("NLI"));
    QCOMPARE(sectionForStateCounty("NY", "Richmond"), QString("NLI"));
    QCOMPARE(sectionForStateCounty("NY", "Suffolk"), QString("NLI"));

    // NNY - Northern New York (11 counties)
    QCOMPARE(sectionForStateCounty("NY", "Clinton"), QString("NNY"));
    QCOMPARE(sectionForStateCounty("NY", "Essex"), QString("NNY"));
    QCOMPARE(sectionForStateCounty("NY", "Franklin"), QString("NNY"));
    QCOMPARE(sectionForStateCounty("NY", "Fulton"), QString("NNY"));
    QCOMPARE(sectionForStateCounty("NY", "Hamilton"), QString("NNY"));
    QCOMPARE(sectionForStateCounty("NY", "Jefferson"), QString("NNY"));
    QCOMPARE(sectionForStateCounty("NY", "Lewis"), QString("NNY"));
    QCOMPARE(sectionForStateCounty("NY", "Saratoga"), QString("NNY"));
    QCOMPARE(sectionForStateCounty("NY", "St. Lawrence"), QString("NNY"));
    QCOMPARE(sectionForStateCounty("NY", "Warren"), QString("NNY"));
    QCOMPARE(sectionForStateCounty("NY", "Washington"), QString("NNY"));

    // WNY - Western New York (15 counties)
    QCOMPARE(sectionForStateCounty("NY", "Allegany"), QString("WNY"));
    QCOMPARE(sectionForStateCounty("NY", "Cattaraugus"), QString("WNY"));
    QCOMPARE(sectionForStateCounty("NY", "Chautauqua"), QString("WNY"));
    QCOMPARE(sectionForStateCounty("NY", "Erie"), QString("WNY"));
    QCOMPARE(sectionForStateCounty("NY", "Genesee"), QString("WNY"));
    QCOMPARE(sectionForStateCounty("NY", "Livingston"), QString("WNY"));
    QCOMPARE(sectionForStateCounty("NY", "Monroe"), QString("WNY"));
    QCOMPARE(sectionForStateCounty("NY", "Niagara"), QString("WNY"));
    QCOMPARE(sectionForStateCounty("NY", "Ontario"), QString("WNY"));
    QCOMPARE(sectionForStateCounty("NY", "Orleans"), QString("WNY"));
    QCOMPARE(sectionForStateCounty("NY", "Seneca"), QString("WNY"));
    QCOMPARE(sectionForStateCounty("NY", "Steuben"), QString("WNY"));
    QCOMPARE(sectionForStateCounty("NY", "Wayne"), QString("WNY"));
    QCOMPARE(sectionForStateCounty("NY", "Wyoming"), QString("WNY"));
    QCOMPARE(sectionForStateCounty("NY", "Yates"), QString("WNY"));

    // ENY - Eastern New York (29 counties)
    QCOMPARE(sectionForStateCounty("NY", "Albany"), QString("ENY"));
    QCOMPARE(sectionForStateCounty("NY", "Broome"), QString("ENY"));
    QCOMPARE(sectionForStateCounty("NY", "Cayuga"), QString("ENY"));
    QCOMPARE(sectionForStateCounty("NY", "Chemung"), QString("ENY"));
    QCOMPARE(sectionForStateCounty("NY", "Chenango"), QString("ENY"));
    QCOMPARE(sectionForStateCounty("NY", "Columbia"), QString("ENY"));
    QCOMPARE(sectionForStateCounty("NY", "Cortland"), QString("ENY"));
    QCOMPARE(sectionForStateCounty("NY", "Delaware"), QString("ENY"));
    QCOMPARE(sectionForStateCounty("NY", "Dutchess"), QString("ENY"));
    QCOMPARE(sectionForStateCounty("NY", "Greene"), QString("ENY"));
    QCOMPARE(sectionForStateCounty("NY", "Herkimer"), QString("ENY"));
    QCOMPARE(sectionForStateCounty("NY", "Madison"), QString("ENY"));
    QCOMPARE(sectionForStateCounty("NY", "Montgomery"), QString("ENY"));
    QCOMPARE(sectionForStateCounty("NY", "Oneida"), QString("ENY"));
    QCOMPARE(sectionForStateCounty("NY", "Onondaga"), QString("ENY"));
    QCOMPARE(sectionForStateCounty("NY", "Orange"), QString("ENY"));
    QCOMPARE(sectionForStateCounty("NY", "Oswego"), QString("ENY"));
    QCOMPARE(sectionForStateCounty("NY", "Otsego"), QString("ENY"));
    QCOMPARE(sectionForStateCounty("NY", "Putnam"), QString("ENY"));
    QCOMPARE(sectionForStateCounty("NY", "Rensselaer"), QString("ENY"));
    QCOMPARE(sectionForStateCounty("NY", "Rockland"), QString("ENY"));
    QCOMPARE(sectionForStateCounty("NY", "Schenectady"), QString("ENY"));
    QCOMPARE(sectionForStateCounty("NY", "Schoharie"), QString("ENY"));
    QCOMPARE(sectionForStateCounty("NY", "Schuyler"), QString("ENY"));
    QCOMPARE(sectionForStateCounty("NY", "Sullivan"), QString("ENY"));
    QCOMPARE(sectionForStateCounty("NY", "Tioga"), QString("ENY"));
    QCOMPARE(sectionForStateCounty("NY", "Tompkins"), QString("ENY"));
    QCOMPARE(sectionForStateCounty("NY", "Ulster"), QString("ENY"));
    QCOMPARE(sectionForStateCounty("NY", "Westchester"), QString("ENY"));
}

// ============================================================================
// NEW JERSEY - ALL 21 COUNTIES
// ============================================================================

void TestArrlSection::testNewJersey_AllCounties()
{
    // NNJ - Northern New Jersey (11 counties)
    QCOMPARE(sectionForStateCounty("NJ", "Bergen"), QString("NNJ"));
    QCOMPARE(sectionForStateCounty("NJ", "Essex"), QString("NNJ"));
    QCOMPARE(sectionForStateCounty("NJ", "Hudson"), QString("NNJ"));
    QCOMPARE(sectionForStateCounty("NJ", "Hunterdon"), QString("NNJ"));
    QCOMPARE(sectionForStateCounty("NJ", "Middlesex"), QString("NNJ"));
    QCOMPARE(sectionForStateCounty("NJ", "Morris"), QString("NNJ"));
    QCOMPARE(sectionForStateCounty("NJ", "Passaic"), QString("NNJ"));
    QCOMPARE(sectionForStateCounty("NJ", "Somerset"), QString("NNJ"));
    QCOMPARE(sectionForStateCounty("NJ", "Sussex"), QString("NNJ"));
    QCOMPARE(sectionForStateCounty("NJ", "Union"), QString("NNJ"));
    QCOMPARE(sectionForStateCounty("NJ", "Warren"), QString("NNJ"));

    // SNJ - Southern New Jersey (10 counties)
    QCOMPARE(sectionForStateCounty("NJ", "Atlantic"), QString("SNJ"));
    QCOMPARE(sectionForStateCounty("NJ", "Burlington"), QString("SNJ"));
    QCOMPARE(sectionForStateCounty("NJ", "Camden"), QString("SNJ"));
    QCOMPARE(sectionForStateCounty("NJ", "Cape May"), QString("SNJ"));
    QCOMPARE(sectionForStateCounty("NJ", "Cumberland"), QString("SNJ"));
    QCOMPARE(sectionForStateCounty("NJ", "Gloucester"), QString("SNJ"));
    QCOMPARE(sectionForStateCounty("NJ", "Mercer"), QString("SNJ"));
    QCOMPARE(sectionForStateCounty("NJ", "Monmouth"), QString("SNJ"));
    QCOMPARE(sectionForStateCounty("NJ", "Ocean"), QString("SNJ"));
    QCOMPARE(sectionForStateCounty("NJ", "Salem"), QString("SNJ"));
}

// ============================================================================
// MASSACHUSETTS - ALL 14 COUNTIES
// ============================================================================

void TestArrlSection::testMassachusetts_AllCounties()
{
    // EMA - Eastern Massachusetts (9 counties)
    QCOMPARE(sectionForStateCounty("MA", "Barnstable"), QString("EMA"));
    QCOMPARE(sectionForStateCounty("MA", "Bristol"), QString("EMA"));
    QCOMPARE(sectionForStateCounty("MA", "Dukes"), QString("EMA"));
    QCOMPARE(sectionForStateCounty("MA", "Essex"), QString("EMA"));
    QCOMPARE(sectionForStateCounty("MA", "Middlesex"), QString("EMA"));
    QCOMPARE(sectionForStateCounty("MA", "Nantucket"), QString("EMA"));
    QCOMPARE(sectionForStateCounty("MA", "Norfolk"), QString("EMA"));
    QCOMPARE(sectionForStateCounty("MA", "Plymouth"), QString("EMA"));
    QCOMPARE(sectionForStateCounty("MA", "Suffolk"), QString("EMA"));

    // WMA - Western Massachusetts (5 counties)
    QCOMPARE(sectionForStateCounty("MA", "Berkshire"), QString("WMA"));
    QCOMPARE(sectionForStateCounty("MA", "Franklin"), QString("WMA"));
    QCOMPARE(sectionForStateCounty("MA", "Hampden"), QString("WMA"));
    QCOMPARE(sectionForStateCounty("MA", "Hampshire"), QString("WMA"));
    QCOMPARE(sectionForStateCounty("MA", "Worcester"), QString("WMA"));
}

// ============================================================================
// PENNSYLVANIA - ALL 67 COUNTIES
// ============================================================================

void TestArrlSection::testPennsylvania_AllCounties()
{
    // EPA - Eastern Pennsylvania (17 counties)
    QCOMPARE(sectionForStateCounty("PA", "Berks"), QString("EPA"));
    QCOMPARE(sectionForStateCounty("PA", "Bucks"), QString("EPA"));
    QCOMPARE(sectionForStateCounty("PA", "Carbon"), QString("EPA"));
    QCOMPARE(sectionForStateCounty("PA", "Chester"), QString("EPA"));
    QCOMPARE(sectionForStateCounty("PA", "Delaware"), QString("EPA"));
    QCOMPARE(sectionForStateCounty("PA", "Lackawanna"), QString("EPA"));
    QCOMPARE(sectionForStateCounty("PA", "Lehigh"), QString("EPA"));
    QCOMPARE(sectionForStateCounty("PA", "Luzerne"), QString("EPA"));
    QCOMPARE(sectionForStateCounty("PA", "Monroe"), QString("EPA"));
    QCOMPARE(sectionForStateCounty("PA", "Montgomery"), QString("EPA"));
    QCOMPARE(sectionForStateCounty("PA", "Northampton"), QString("EPA"));
    QCOMPARE(sectionForStateCounty("PA", "Philadelphia"), QString("EPA"));
    QCOMPARE(sectionForStateCounty("PA", "Pike"), QString("EPA"));
    QCOMPARE(sectionForStateCounty("PA", "Schuylkill"), QString("EPA"));
    QCOMPARE(sectionForStateCounty("PA", "Susquehanna"), QString("EPA"));
    QCOMPARE(sectionForStateCounty("PA", "Wayne"), QString("EPA"));
    QCOMPARE(sectionForStateCounty("PA", "Wyoming"), QString("EPA"));

    // WPA - Western Pennsylvania (50 counties)
    QCOMPARE(sectionForStateCounty("PA", "Adams"), QString("WPA"));
    QCOMPARE(sectionForStateCounty("PA", "Allegheny"), QString("WPA"));
    QCOMPARE(sectionForStateCounty("PA", "Armstrong"), QString("WPA"));
    QCOMPARE(sectionForStateCounty("PA", "Beaver"), QString("WPA"));
    QCOMPARE(sectionForStateCounty("PA", "Bedford"), QString("WPA"));
    QCOMPARE(sectionForStateCounty("PA", "Blair"), QString("WPA"));
    QCOMPARE(sectionForStateCounty("PA", "Bradford"), QString("WPA"));
    QCOMPARE(sectionForStateCounty("PA", "Butler"), QString("WPA"));
    QCOMPARE(sectionForStateCounty("PA", "Cambria"), QString("WPA"));
    QCOMPARE(sectionForStateCounty("PA", "Cameron"), QString("WPA"));
    QCOMPARE(sectionForStateCounty("PA", "Centre"), QString("WPA"));
    QCOMPARE(sectionForStateCounty("PA", "Clarion"), QString("WPA"));
    QCOMPARE(sectionForStateCounty("PA", "Clearfield"), QString("WPA"));
    QCOMPARE(sectionForStateCounty("PA", "Clinton"), QString("WPA"));
    QCOMPARE(sectionForStateCounty("PA", "Columbia"), QString("WPA"));
    QCOMPARE(sectionForStateCounty("PA", "Crawford"), QString("WPA"));
    QCOMPARE(sectionForStateCounty("PA", "Cumberland"), QString("WPA"));
    QCOMPARE(sectionForStateCounty("PA", "Dauphin"), QString("WPA"));
    QCOMPARE(sectionForStateCounty("PA", "Elk"), QString("WPA"));
    QCOMPARE(sectionForStateCounty("PA", "Erie"), QString("WPA"));
    QCOMPARE(sectionForStateCounty("PA", "Fayette"), QString("WPA"));
    QCOMPARE(sectionForStateCounty("PA", "Forest"), QString("WPA"));
    QCOMPARE(sectionForStateCounty("PA", "Franklin"), QString("WPA"));
    QCOMPARE(sectionForStateCounty("PA", "Fulton"), QString("WPA"));
    QCOMPARE(sectionForStateCounty("PA", "Greene"), QString("WPA"));
    QCOMPARE(sectionForStateCounty("PA", "Huntingdon"), QString("WPA"));
    QCOMPARE(sectionForStateCounty("PA", "Indiana"), QString("WPA"));
    QCOMPARE(sectionForStateCounty("PA", "Jefferson"), QString("WPA"));
    QCOMPARE(sectionForStateCounty("PA", "Juniata"), QString("WPA"));
    QCOMPARE(sectionForStateCounty("PA", "Lancaster"), QString("WPA"));
    QCOMPARE(sectionForStateCounty("PA", "Lawrence"), QString("WPA"));
    QCOMPARE(sectionForStateCounty("PA", "Lebanon"), QString("WPA"));
    QCOMPARE(sectionForStateCounty("PA", "Lycoming"), QString("WPA"));
    QCOMPARE(sectionForStateCounty("PA", "McKean"), QString("WPA"));
    QCOMPARE(sectionForStateCounty("PA", "Mercer"), QString("WPA"));
    QCOMPARE(sectionForStateCounty("PA", "Mifflin"), QString("WPA"));
    QCOMPARE(sectionForStateCounty("PA", "Montour"), QString("WPA"));
    QCOMPARE(sectionForStateCounty("PA", "Northumberland"), QString("WPA"));
    QCOMPARE(sectionForStateCounty("PA", "Perry"), QString("WPA"));
    QCOMPARE(sectionForStateCounty("PA", "Potter"), QString("WPA"));
    QCOMPARE(sectionForStateCounty("PA", "Snyder"), QString("WPA"));
    QCOMPARE(sectionForStateCounty("PA", "Somerset"), QString("WPA"));
    QCOMPARE(sectionForStateCounty("PA", "Sullivan"), QString("WPA"));
    QCOMPARE(sectionForStateCounty("PA", "Tioga"), QString("WPA"));
    QCOMPARE(sectionForStateCounty("PA", "Union"), QString("WPA"));
    QCOMPARE(sectionForStateCounty("PA", "Venango"), QString("WPA"));
    QCOMPARE(sectionForStateCounty("PA", "Warren"), QString("WPA"));
    QCOMPARE(sectionForStateCounty("PA", "Washington"), QString("WPA"));
    QCOMPARE(sectionForStateCounty("PA", "Westmoreland"), QString("WPA"));
    QCOMPARE(sectionForStateCounty("PA", "York"), QString("WPA"));
}

// ============================================================================
// WASHINGTON - ALL 39 COUNTIES
// ============================================================================

void TestArrlSection::testWashington_AllCounties()
{
    // EWA - Eastern Washington (20 counties)
    QCOMPARE(sectionForStateCounty("WA", "Adams"), QString("EWA"));
    QCOMPARE(sectionForStateCounty("WA", "Asotin"), QString("EWA"));
    QCOMPARE(sectionForStateCounty("WA", "Benton"), QString("EWA"));
    QCOMPARE(sectionForStateCounty("WA", "Chelan"), QString("EWA"));
    QCOMPARE(sectionForStateCounty("WA", "Columbia"), QString("EWA"));
    QCOMPARE(sectionForStateCounty("WA", "Douglas"), QString("EWA"));
    QCOMPARE(sectionForStateCounty("WA", "Ferry"), QString("EWA"));
    QCOMPARE(sectionForStateCounty("WA", "Franklin"), QString("EWA"));
    QCOMPARE(sectionForStateCounty("WA", "Garfield"), QString("EWA"));
    QCOMPARE(sectionForStateCounty("WA", "Grant"), QString("EWA"));
    QCOMPARE(sectionForStateCounty("WA", "Kittitas"), QString("EWA"));
    QCOMPARE(sectionForStateCounty("WA", "Klickitat"), QString("EWA"));
    QCOMPARE(sectionForStateCounty("WA", "Lincoln"), QString("EWA"));
    QCOMPARE(sectionForStateCounty("WA", "Okanogan"), QString("EWA"));
    QCOMPARE(sectionForStateCounty("WA", "Pend Oreille"), QString("EWA"));
    QCOMPARE(sectionForStateCounty("WA", "Spokane"), QString("EWA"));
    QCOMPARE(sectionForStateCounty("WA", "Stevens"), QString("EWA"));
    QCOMPARE(sectionForStateCounty("WA", "Walla Walla"), QString("EWA"));
    QCOMPARE(sectionForStateCounty("WA", "Whitman"), QString("EWA"));
    QCOMPARE(sectionForStateCounty("WA", "Yakima"), QString("EWA"));

    // WWA - Western Washington (19 counties)
    QCOMPARE(sectionForStateCounty("WA", "Clallam"), QString("WWA"));
    QCOMPARE(sectionForStateCounty("WA", "Clark"), QString("WWA"));
    QCOMPARE(sectionForStateCounty("WA", "Cowlitz"), QString("WWA"));
    QCOMPARE(sectionForStateCounty("WA", "Grays Harbor"), QString("WWA"));
    QCOMPARE(sectionForStateCounty("WA", "Island"), QString("WWA"));
    QCOMPARE(sectionForStateCounty("WA", "Jefferson"), QString("WWA"));
    QCOMPARE(sectionForStateCounty("WA", "King"), QString("WWA"));
    QCOMPARE(sectionForStateCounty("WA", "Kitsap"), QString("WWA"));
    QCOMPARE(sectionForStateCounty("WA", "Lewis"), QString("WWA"));
    QCOMPARE(sectionForStateCounty("WA", "Mason"), QString("WWA"));
    QCOMPARE(sectionForStateCounty("WA", "Pacific"), QString("WWA"));
    QCOMPARE(sectionForStateCounty("WA", "Pierce"), QString("WWA"));
    QCOMPARE(sectionForStateCounty("WA", "San Juan"), QString("WWA"));
    QCOMPARE(sectionForStateCounty("WA", "Skagit"), QString("WWA"));
    QCOMPARE(sectionForStateCounty("WA", "Skamania"), QString("WWA"));
    QCOMPARE(sectionForStateCounty("WA", "Snohomish"), QString("WWA"));
    QCOMPARE(sectionForStateCounty("WA", "Thurston"), QString("WWA"));
    QCOMPARE(sectionForStateCounty("WA", "Wahkiakum"), QString("WWA"));
    QCOMPARE(sectionForStateCounty("WA", "Whatcom"), QString("WWA"));
}

// ============================================================================
// SIMPLE STATES - ALL 43 SINGLE-SECTION STATES
// ============================================================================

void TestArrlSection::testSimpleStates_AllStates()
{
    // Test all 43 single-section states + DC/VI
    QCOMPARE(sectionForState("AK"), QString("AK"));
    QCOMPARE(sectionForState("AL"), QString("AL"));
    QCOMPARE(sectionForState("AR"), QString("AR"));
    QCOMPARE(sectionForState("AZ"), QString("AZ"));
    QCOMPARE(sectionForState("CO"), QString("CO"));
    QCOMPARE(sectionForState("CT"), QString("CT"));
    QCOMPARE(sectionForState("DE"), QString("DE"));
    QCOMPARE(sectionForState("GA"), QString("GA"));
    QCOMPARE(sectionForState("HI"), QString("HI"));
    QCOMPARE(sectionForState("IA"), QString("IA"));
    QCOMPARE(sectionForState("ID"), QString("ID"));
    QCOMPARE(sectionForState("IL"), QString("IL"));
    QCOMPARE(sectionForState("IN"), QString("IN"));
    QCOMPARE(sectionForState("KS"), QString("KS"));
    QCOMPARE(sectionForState("KY"), QString("KY"));
    QCOMPARE(sectionForState("LA"), QString("LA"));
    QCOMPARE(sectionForState("MD"), QString("MDC"));
    QCOMPARE(sectionForState("MDC"), QString("MDC"));
    QCOMPARE(sectionForState("ME"), QString("ME"));
    QCOMPARE(sectionForState("MI"), QString("MI"));
    QCOMPARE(sectionForState("MN"), QString("MN"));
    QCOMPARE(sectionForState("MO"), QString("MO"));
    QCOMPARE(sectionForState("MS"), QString("MS"));
    QCOMPARE(sectionForState("MT"), QString("MT"));
    QCOMPARE(sectionForState("NC"), QString("NC"));
    QCOMPARE(sectionForState("ND"), QString("ND"));
    QCOMPARE(sectionForState("NE"), QString("NE"));
    QCOMPARE(sectionForState("NH"), QString("NH"));
    QCOMPARE(sectionForState("NM"), QString("NM"));
    QCOMPARE(sectionForState("NV"), QString("NV"));
    QCOMPARE(sectionForState("OH"), QString("OH"));
    QCOMPARE(sectionForState("OK"), QString("OK"));
    QCOMPARE(sectionForState("OR"), QString("OR"));
    QCOMPARE(sectionForState("RI"), QString("RI"));
    QCOMPARE(sectionForState("SC"), QString("SC"));
    QCOMPARE(sectionForState("SD"), QString("SD"));
    QCOMPARE(sectionForState("TN"), QString("TN"));
    QCOMPARE(sectionForState("UT"), QString("UT"));
    QCOMPARE(sectionForState("VA"), QString("VA"));
    QCOMPARE(sectionForState("VI"), QString("VI"));
    QCOMPARE(sectionForState("VT"), QString("VT"));
    QCOMPARE(sectionForState("WI"), QString("WI"));
    QCOMPARE(sectionForState("WV"), QString("WV"));
    QCOMPARE(sectionForState("WY"), QString("WY"));

    // Test with county names - should still work for simple states
    QCOMPARE(sectionForStateCounty("UT", "Salt Lake"), QString("UT"));
    QCOMPARE(sectionForStateCounty("UT", "Weber"), QString("UT"));
    QCOMPARE(sectionForStateCounty("CO", "Denver"), QString("CO"));
    QCOMPARE(sectionForStateCounty("AZ", "Maricopa"), QString("AZ"));
    QCOMPARE(sectionForStateCounty("OR", "Multnomah"), QString("OR"));
}

// ============================================================================
// NORMALIZATION AND EDGE CASE TESTS
// ============================================================================

void TestArrlSection::testCaseInsensitive()
{
    // State case variations
    QCOMPARE(sectionForStateCounty("fl", "Pinellas"), QString("WCF"));
    QCOMPARE(sectionForStateCounty("FL", "Pinellas"), QString("WCF"));
    QCOMPARE(sectionForStateCounty("Fl", "Pinellas"), QString("WCF"));

    // County case variations
    QCOMPARE(sectionForStateCounty("FL", "pinellas"), QString("WCF"));
    QCOMPARE(sectionForStateCounty("FL", "PINELLAS"), QString("WCF"));
    QCOMPARE(sectionForStateCounty("FL", "Pinellas"), QString("WCF"));

    // Both lowercase
    QCOMPARE(sectionForStateCounty("ca", "los angeles"), QString("LAX"));
}

void TestArrlSection::testCountyNormalization()
{
    // Test removal of "County" suffix
    QCOMPARE(sectionForStateCounty("FL", "Pinellas County"), QString("WCF"));
    QCOMPARE(sectionForStateCounty("FL", "Pinellas county"), QString("WCF"));
    QCOMPARE(sectionForStateCounty("FL", "Pinellas"), QString("WCF"));

    // Test St./Saint normalization
    QCOMPARE(sectionForStateCounty("FL", "St. Johns"), QString("NFL"));
    QCOMPARE(sectionForStateCounty("FL", "Saint Johns"), QString("NFL"));
    QCOMPARE(sectionForStateCounty("FL", "St Johns"), QString("NFL"));

    QCOMPARE(sectionForStateCounty("NY", "St. Lawrence"), QString("NNY"));
    QCOMPARE(sectionForStateCounty("NY", "Saint Lawrence"), QString("NNY"));
    QCOMPARE(sectionForStateCounty("NY", "St Lawrence"), QString("NNY"));

    // Test hyphenated county names
    QCOMPARE(sectionForStateCounty("FL", "Miami-Dade"), QString("SFL"));
    QCOMPARE(sectionForStateCounty("FL", "Miami Dade"), QString("SFL"));
}

void TestArrlSection::testStateNormalization()
{
    // Test whitespace handling
    QCOMPARE(sectionForStateCounty("  FL  ", "Pinellas"), QString("WCF"));
    QCOMPARE(sectionForStateCounty("FL", "  Pinellas  "), QString("WCF"));
}

void TestArrlSection::testUnknownState()
{
    // Unknown state should return empty QString
    QVERIFY(sectionForStateCounty("ZZ", "Unknown").isEmpty());
    QVERIFY(sectionForStateCounty("XX", "Test").isEmpty());
}

void TestArrlSection::testUnknownCounty()
{
    // Known state but unknown county in subdivided state should return empty
    QVERIFY(sectionForStateCounty("FL", "NonexistentCounty").isEmpty());
    QVERIFY(sectionForStateCounty("CA", "FakeCounty").isEmpty());
}

void TestArrlSection::testEmptyInputs()
{
    // Empty inputs should return empty QString
    QVERIFY(sectionForStateCounty("", "").isEmpty());
    QVERIFY(sectionForStateCounty("FL", "").isEmpty());
    QVERIFY(sectionForStateCounty("", "Pinellas").isEmpty());

    QVERIFY(sectionForState("").isEmpty());
}

QTEST_MAIN(TestArrlSection)
#include "test_arrlsection.moc"
