// ============================================================================
//  Autonomous Landslide & Flash-Flood Early Warning Grid (ALF-EWG)
//  Course  : Object-Oriented Programming Using C++ (CIC-211)
//  Author  : Bhavya Kumari      Enrollment No. : 02096402725
//  Institute: Maharaja Agrasen Institute of Technology
//
//  main.cpp - runs one demonstration section per syllabus unit and then a
//  complete six-sweep simulation of the warning grid.
// ============================================================================
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

#include "Exceptions.h"
#include "GridController.h"
#include "HazardMath.h"
#include "IncidentLogger.h"
#include "Manipulators.h"
#include "SensorNodes.h"
#include "SensorStation.h"
#include "TelemetryAuditor.h"

using namespace DisasterCore;
using namespace DisasterCore::Sensors;
using namespace DisasterCore::Alerts;

namespace {

const char* kEmbeddedStations =
    "# embedded fallback copy of data/stations.txt\n"
    "S-101 Kedar_Soil_A Z-KEDAR soil 30.73520 79.06690 3583\n"
    "S-102 Kedar_Sonar_1 Z-KEDAR sonar 30.73010 79.07020 3490\n"
    "S-103 Kedar_Hybrid_1 Z-KEDAR hybrid 30.74100 79.06110 3720\n"
    "S-201 Chamoli_Tilt_A Z-CHAMOLI tilt 30.40250 79.32170 1130\n"
    "S-202 Chamoli_Soil_B Z-CHAMOLI soil 30.41100 79.31500 1210\n"
    "S-203 Chamoli_Sonar_1 Z-CHAMOLI sonar 30.39800 79.33000 1085\n"
    "S-301 Pithora_Tilt_A Z-PITHORAGARH tilt 29.58290 80.21820 1650\n"
    "S-302 Pithora_Hybrid_1 Z-PITHORAGARH hybrid 29.59000 80.20500 1720\n"
    "S-401 Uttark_Soil_A Z-UTTARKASHI soil 30.72680 78.43540 1160\n"
    "S-402 Uttark_Sonar_1 Z-UTTARKASHI sonar 30.73100 78.44300 1105\n";

void banner(const std::string& title) {
    std::cout << "\n##############################################################\n"
              << "#  " << title << '\n'
              << "##############################################################\n";
}

// ===========================================================================
//  UNIT I : namespaces, inline functions, overloading, references, friend
// ===========================================================================
void demoUnitI() {
    banner("UNIT I - namespaces, inline, overloading, references, friend");
    std::cout << std::fixed << std::setprecision(2);

    std::cout << "Namespaces in use: DisasterCore::Sensors and DisasterCore::Alerts\n\n";

    // Overloaded, inline computeHazardIndex(): same name, different parameter sets.
    const RainfallSlope rs = { 85.0, 40.0 };
    const SoilVibration sv = { 72.0, 14.0 };
    const RiverRain rr = { 5.5, 90.0 };
    std::cout << "computeHazardIndex(rainfall 85 mm/h + slope 40 deg)        = "
              << computeHazardIndex(rs) << '\n';
    std::cout << "computeHazardIndex(soil 72 % + vibration 14 Hz)            = "
              << computeHazardIndex(sv) << '\n';
    std::cout << "computeHazardIndex(river 5.5 m + rainfall 90 mm/h)         = "
              << computeHazardIndex(rr) << '\n';
    std::cout << "computeHazardIndex(rain/slope, soil/vibration, river 5.5 m) = "
              << computeHazardIndex(rs, sv, 5.5) << "\n\n";

    // Call by reference + return by reference: in-place, chainable transformation.
    const GeoCoordinate origin(30.7352, 79.0669, 3583.0);
    GeoCoordinate point(30.7410, 79.0611, 3720.0);
    GeoCoordinate& ref = toLocalGridKm(point, origin);        // returns a reference to 'point'
    std::cout << "toLocalGridKm() returned a reference to the SAME object: "
              << (&ref == &point ? "yes" : "no") << '\n';
    std::cout << "Local grid (km)   : north=" << point.latitude << " east=" << point.longitude
              << " rise(m)=" << point.elevation << '\n';
    scaleCoordinate(point, 1000.0);                            // chained use, still no copies
    std::cout << "Scaled x1000      : north=" << point.latitude << " east=" << point.longitude
              << " rise=" << point.elevation << "\n";

    TelemetryReading a, b, total;
    a.rainfallRate = 10; a.soilSaturation = 40;
    b.rainfallRate = 25; b.soilSaturation = 55;
    accumulate(accumulate(total, a), b);                       // chained accumulation by reference
    std::cout << "accumulate(accumulate(total,a),b): rainfall=" << total.rainfallRate
              << " saturation=" << total.soilSaturation << '\n';
    std::cout << "(friend class TelemetryAuditor is demonstrated in Unit II)\n";
}

// ===========================================================================
//  UNIT II : abstraction, statics, dynamic memory, operator overloading
// ===========================================================================
void demoUnitII() {
    banner("UNIT II - encapsulation, static members, dynamic memory, operators");
    std::cout << std::fixed << std::setprecision(1);
    std::cout << "Active stations at start      : " << SensorStation::activeStations() << '\n';

    // ---- stream extraction (>>) from a stringstream ----
    std::istringstream config(
        "S-901 Demo_Alpha Z-DEMO soil 30.1 79.1 2000\n"
        "S-902 Demo_Beta  Z-DEMO tilt 30.2 79.2 2100\n"
        "S-903 Demo_Gamma Z-DEMO sonar 30.3 79.3 2200\n");
    SensorStation s1, s2, s3;
    config >> s1 >> s2 >> s3;
    std::cout << "Stations read with operator>> : " << SensorStation::activeStations()
              << " active, " << SensorStation::totalCreated() << " created so far\n";

    // feed some readings
    TelemetryReading r;
    r.soilSaturation = 70; r.vibrationHz = 12;
    s1.recordReading(r, 61.5, AlertLevel::Warning);
    s2.recordReading(r, 34.0, AlertLevel::Normal);
    s3.recordReading(r, 82.3, AlertLevel::Evacuate);

    // ---- stream insertion (<<) to console and to a file ----
    std::cout << "\nConsole output with operator<<:\n  " << s1 << "\n  " << s2 << "\n  " << s3 << '\n';
    {
        std::ofstream file("stations_snapshot.txt");
        file << "# snapshot written with operator<< and the 'configFormat' manipulator\n"
             << configFormat << s1 << '\n' << s2 << '\n' << s3 << '\n';
    }
    {
        std::ifstream file("stations_snapshot.txt");
        std::string header;
        std::getline(file, header);
        SensorStation back;
        file >> back;
        std::cout << "\nRead back from stations_snapshot.txt via operator>>:\n  " << back
                  << "\n  (the file layout stores identity + location only, so hazard restarts at 0)\n";
    }

    // ---- operator> and operator+ ----
    std::cout << "\nOperator > : (Gamma > Alpha) = " << (s3 > s1 ? "true" : "false")
              << ",  (Beta > Alpha) = " << (s2 > s1 ? "true" : "false") << '\n';
    RegionalZone zone = s1 + s2 + s3;                           // chained operator+
    std::cout << "Operator + : " << zone << '\n';

    // ---- deep copy ----
    {
        SensorStation copy(s1);                                 // deep copy constructor
        TelemetryReading other;
        other.rainfallRate = 99;
        copy.recordReading(other, 10.0, AlertLevel::Normal);
        copy.recordReading(other, 11.0, AlertLevel::Normal);
        std::cout << "\nDeep copy test (original kept 1 reading, copy has 3):\n";
        TelemetryAuditor::audit(s1, std::cout);                 // friend class at work
        TelemetryAuditor::audit(copy, std::cout);
        std::cout << "Active stations while the copy exists : " << SensorStation::activeStations() << '\n';
    }
    std::cout << "Active stations after the copy is gone  : " << SensorStation::activeStations() << '\n';

    // ---- dynamic constructors / deep copy of the raw buffer ----
    TelemetryReading samples[3];
    for (int i = 0; i < 3; ++i) samples[i].rainfallRate = 10.0 * (i + 1);
    TelemetryBuffer dyn(samples, 3);                            // dynamic constructor from an array
    TelemetryBuffer dynCopy(dyn);                               // deep copy
    dynCopy.at(0).rainfallRate = -1.0;                          // return-by-reference write
    std::cout << "\nTelemetryBuffer: original[0]=" << dyn.at(0).rainfallRate
              << "  copy[0]=" << dynCopy.at(0).rainfallRate << "  (independent heap blocks)\n";
    std::cout << "TelemetryBuffer average rainfall = " << dyn.average().rainfallRate << '\n';

    // ---- static data on sirens ----
    {
        EarlyWarningSiren sirenA("SIR-A", "Z-DEMO"), sirenB("SIR-B", "Z-DEMO");
        sirenA.activate(AlertLevel::Warning);
        EarlyWarningSiren::setGlobalAlertLevel(AlertLevel::Warning);
        std::cout << "\n" << sirenA << "\n" << sirenB << '\n'
                  << "Static state shared by all sirens: count=" << EarlyWarningSiren::sirenCount()
                  << ", global level=" << toString(EarlyWarningSiren::globalAlertLevel()) << '\n';
    }
    EarlyWarningSiren::setGlobalAlertLevel(AlertLevel::Normal);
    std::cout << "Sirens alive after scope ends: " << EarlyWarningSiren::sirenCount() << '\n';
}

// ===========================================================================
//  UNIT III : abstract class, diamond, containership, polymorphism
// ===========================================================================
void demoUnitIII() {
    banner("UNIT III - inheritance, containership, runtime polymorphism");
    std::cout << std::fixed << std::setprecision(1);

    // BaseSensorNode is abstract: "BaseSensorNode n;" would not compile.
    AutonomousHybridProbe probe("H-900", GeoCoordinate(30.0, 79.0, 2500), 42u);
    std::cout << "Diamond: AutonomousHybridProbe -> {SolarPoweredNode, WirelessLoRaTransceiver}"
                 " -> BaseSensorNode -> DisasterTelemetryDevice\n";
    std::cout << "Both parents share ONE root object (virtual inheritance): "
              << (probe.singleRootInstance() ? "yes" : "NO") << '\n';
    std::cout << "sizeof(AutonomousHybridProbe) = " << sizeof(probe) << " bytes\n";
    std::cout << "Containership: node HAS-A PowerSubsystem, battery = "
              << probe.powerSubsystem().batteryPercent() << " %\n\n";

    // Base-class pointer vector -> dynamic dispatch through the vtable.
    std::vector<BaseSensorNode*> nodes;
    nodes.push_back(new SoilGeophoneNode("D-1", GeoCoordinate(), 11u));
    nodes.push_back(new AcousticTiltmeterNode("D-2", GeoCoordinate(), 12u));
    nodes.push_back(new RiverSonarNode("D-3", GeoCoordinate(), 13u));
    nodes.push_back(&probe);                                    // stack object, not deleted below

    std::cout << "Dynamic dispatch with stress = 0.70 for every node:\n";
    for (std::size_t i = 0; i < nodes.size(); ++i) {
        nodes[i]->setEnvironment(0.70, 1);
        nodes[i]->readTelemetry();                              // resolved at run time
        std::cout << "  " << std::left << std::setw(13) << nodes[i]->nodeType() << std::right
                  << " hazard=" << std::setw(5) << nodes[i]->hazardIndex()
                  << " level=" << toString(nodes[i]->evaluateThreshold()) << '\n';
    }
    for (std::size_t i = 0; i + 1 < nodes.size(); ++i) delete nodes[i];   // virtual destructors
    std::cout << "Heap nodes deleted through base pointers (virtual destructor chain runs).\n";
}

// ===========================================================================
//  UNIT IV : exceptions, binary persistence + rollback, STL
// ===========================================================================
void demoUnitIV() {
    banner("UNIT IV - exceptions, transactional binary log, STL");

    std::cout << "Custom exceptions (derived from std::exception):\n";
    try { throw SensorFailureException("S-X", "demo failure"); }
    catch (const std::exception& e) { std::cout << "  " << e.what() << '\n'; }
    try { throw TelemetryChecksumMismatch("S-X", 0xCAFEBABEu, 0x0BADF00Du); }
    catch (const std::exception& e) { std::cout << "  " << e.what() << '\n'; }
    try { throw ThresholdOverflowException("S-X", "rainfallRate", 9999.0, 400.0); }
    catch (const std::exception& e) { std::cout << "  " << e.what() << '\n'; }

    std::cout << "\nTransactional binary log (std::fstream + seekp/seekg):\n";
    const std::string path = "demo_incident_log.bin";
    std::remove(path.c_str());
    IncidentLogger log(path);

    log.beginTransaction();
    for (unsigned i = 1; i <= 3; ++i)
        log.append(IncidentRecord::make(i, 1, AlertLevel::Warning, 60.0 + i, "S-DEMO", "Z-DEMO", "txn 1 record"));
    log.commit();
    std::cout << "  txn 1 committed  -> committed records = " << log.committedCount() << '\n';

    try {
        log.beginTransaction();
        log.append(IncidentRecord::make(4, 2, AlertLevel::Evacuate, 91.0, "S-DEMO", "Z-DEMO", "txn 2 record"));
        log.append(IncidentRecord::make(5, 2, AlertLevel::Evacuate, 93.0, "S-DEMO", "Z-DEMO", "txn 2 record"));
        throw ThresholdOverflowException("S-DEMO", "vibrationHz", 450.0, 100.0);   // failure mid-transaction
        log.commit();
    } catch (const std::exception& e) {
        std::cout << "  failure inside txn 2: " << e.what() << '\n';
        log.rollback();
        std::cout << "  txn 2 rolled back -> committed records = " << log.committedCount()
                  << ", visible on disk = " << log.readAll().size() << '\n';
    }

    log.beginTransaction();
    log.append(IncidentRecord::make(4, 3, AlertLevel::Warning, 66.0, "S-DEMO", "Z-DEMO", "txn 3 record"));
    log.commit();
    const std::vector<IncidentRecord> all = log.readAll();
    std::cout << "  txn 3 committed  -> records on disk = " << all.size() << " (ids:";
    for (std::size_t i = 0; i < all.size(); ++i) std::cout << ' ' << all[i].incidentId;
    std::cout << ")\n";

    std::cout << "\nCustom manipulators + STL are shown in the sweep tables below\n"
                 "(std::vector, std::map, std::sort, std::for_each, functors, Cell/NumCell/RuleLine).\n";
}

// ===========================================================================
//  Full simulation of the grid
// ===========================================================================
void runSimulation(bool keepLog) {
    banner("FULL SIMULATION - ALF-EWG with 10 stations and 6 sensor sweeps");

    const std::string logPath = "alf_ewg_incidents.bin";
    if (!keepLog) std::remove(logPath.c_str());

    GridController grid(logPath);
    grid.addZone("Z-KEDAR",        ZoneProfile(8200,  1.00, "Route A: Ridge Rd > Shelter-1"));
    grid.addZone("Z-CHAMOLI",      ZoneProfile(15400, 0.85, "Route B: NH Link > Relief Camp-2"));
    grid.addZone("Z-PITHORAGARH",  ZoneProfile(6100,  0.60, "Route C: Valley Rd > School Camp-3"));
    grid.addZone("Z-UTTARKASHI",   ZoneProfile(12800, 0.45, "Route D: Bridge Rd > Stadium Camp-4"));

    std::ifstream file("data/stations.txt");
    std::size_t loaded = 0;
    if (file) {
        std::cout << "Loading stations from data/stations.txt\n";
        loaded = grid.loadStations(file);
    } else {
        std::cout << "data/stations.txt not found - using embedded station list\n";
        std::istringstream embedded(kEmbeddedStations);
        loaded = grid.loadStations(embedded);
    }
    grid.deployFleet();
    std::cout << loaded << " stations loaded, " << SensorStation::activeStations()
              << " active, " << grid.sirenCount() << " sirens ("
              << EarlyWarningSiren::sirenCount() << " counted by the static member)\n\n";
    grid.printFleetInventory(std::cout);
    grid.drainBattery("S-402", 92.0);          // S-402 is almost flat: it will brown out
    std::cout << "\n(S-402 battery drained to 8 % to simulate a failing node)\n";

    const double stress[6] = { 0.20, 0.35, 0.50, 0.65, 0.80, 0.95 };
    for (unsigned sweep = 1; sweep <= 6; ++sweep) {
        // scripted fault injection to exercise the exception framework
        if (sweep == 2) grid.injectFault("S-203", FaultMode::DeadSensor);
        if (sweep == 3) { grid.injectFault("S-203", FaultMode::None);
                          grid.injectFault("S-103", FaultMode::CorruptPacket); }
        if (sweep == 4) { grid.injectFault("S-103", FaultMode::None);
                          grid.injectFault("S-301", FaultMode::RangeOverflow); }
        if (sweep == 5) grid.injectFault("S-301", FaultMode::None);
        grid.runSweep(sweep, stress[sweep - 1], std::cout);
    }

    banner("SIMULATION SUMMARY");
    const SweepStats& t = grid.totals();
    std::cout << "Accepted readings        : " << t.acceptedReadings << '\n'
              << "Sensor failures          : " << t.sensorFailures << '\n'
              << "Checksum mismatches      : " << t.checksumErrors << '\n'
              << "Threshold overflows      : " << t.overflowErrors << '\n'
              << "Incidents written to disk: " << t.incidentsLogged << '\n'
              << "Final global alert level : " << toString(EarlyWarningSiren::globalAlertLevel()) << '\n'
              << "Active stations (static) : " << SensorStation::activeStations() << "\n\n";
    grid.printStationRanking(std::cout, 5);
    std::cout << '\n';
    grid.printPersistedIncidents(std::cout, 10);
    std::cout << "\nFriend-class audit of the most critical station:\n";
    const std::vector<SensorStation>& all = grid.stations();
    std::size_t best = 0;
    for (std::size_t i = 1; i < all.size(); ++i) if (all[i] > all[best]) best = i;
    TelemetryAuditor::audit(all[best], std::cout);
}

} // namespace

int main(int argc, char* argv[]) {
    bool keepLog = (argc > 1 && std::strcmp(argv[1], "--append") == 0);
    std::cout << "==============================================================\n"
              << " Autonomous Landslide & Flash-Flood Early Warning Grid\n"
              << " (ALF-EWG)  -  CIC-211 Object-Oriented Programming Using C++\n"
              << " Bhavya Kumari | Enrollment No. 02096402725\n"
              << "==============================================================\n";
    try {
        demoUnitI();
        demoUnitII();
        demoUnitIII();
        demoUnitIV();
        runSimulation(keepLog);
    } catch (const std::exception& e) {
        std::cerr << "\nFATAL: " << e.what() << '\n';
        return 1;
    }
    std::cout << "\nAll units demonstrated. Program finished normally.\n";
    return 0;
}
