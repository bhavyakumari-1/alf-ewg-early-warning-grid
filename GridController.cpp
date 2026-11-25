#include "GridController.h"
#include "Exceptions.h"
#include "Functors.h"
#include "HazardMath.h"
#include "Manipulators.h"
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace DisasterCore {

using namespace Alerts;
using namespace Sensors;

namespace {
    // Column layout of the per-sweep node table.
    const int kNode = 6, kType = 13, kHaz = 7, kLvl = 9, kNote = 34;

    std::vector<int> nodeTableWidths() {
        std::vector<int> w;
        w.push_back(kNode); w.push_back(kType); w.push_back(kHaz); w.push_back(kLvl); w.push_back(kNote);
        return w;
    }
}

GridController::GridController(const std::string& logPath)
    : logger_(logPath), nextIncidentId_(0) {
    nextIncidentId_ = logger_.committedCount() + 1;
}

GridController::~GridController() {
    for (std::size_t i = 0; i < fleet_.size(); ++i) delete fleet_[i];   // virtual destructor chain
    fleet_.clear();
}

void GridController::addZone(const std::string& name, const ZoneProfile& profile) {
    zones_[name] = profile;
    sirens_.push_back(EarlyWarningSiren("SIR-" + name, name));
}

std::size_t GridController::loadStations(std::istream& in) {
    std::size_t loaded = 0;
    std::string line;
    stations_.reserve(32);
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;      // skip comments / blanks
        std::istringstream lineStream(line);
        SensorStation st;
        if (lineStream >> st) {                            // overloaded operator>>
            stations_.push_back(st);
            ++loaded;
        }
    }
    return loaded;
}

void GridController::deployFleet() {
    for (std::size_t i = 0; i < stations_.size(); ++i) {
        const SensorStation& st = stations_[i];
        const std::uint32_t seed = static_cast<std::uint32_t>(1000 + 7919 * (i + 1));
        BaseSensorNode* node = nullptr;
        if (st.kind() == "soil")        node = new SoilGeophoneNode(st.id(), st.location(), seed);
        else if (st.kind() == "tilt")   node = new AcousticTiltmeterNode(st.id(), st.location(), seed);
        else if (st.kind() == "sonar")  node = new RiverSonarNode(st.id(), st.location(), seed);
        else if (st.kind() == "hybrid") node = new AutonomousHybridProbe(st.id(), st.location(), seed);
        else throw std::invalid_argument("unknown sensor kind '" + st.kind() + "' for station " + st.id());
        fleet_.push_back(node);
    }
}

BaseSensorNode* GridController::findNode(const std::string& id) const {
    for (std::size_t i = 0; i < fleet_.size(); ++i)
        if (fleet_[i]->deviceId() == id) return fleet_[i];
    return nullptr;
}

bool GridController::injectFault(const std::string& nodeId, FaultMode mode) {
    BaseSensorNode* n = findNode(nodeId);
    if (!n) return false;
    n->injectFault(mode);
    return true;
}

bool GridController::drainBattery(const std::string& nodeId, double percent) {
    BaseSensorNode* n = findNode(nodeId);
    if (!n) return false;
    n->drainBattery(percent);
    return true;
}

// ---------------------------------------------------------------------------
//  One complete sweep cycle
// ---------------------------------------------------------------------------
void GridController::runSweep(unsigned sweepNo, double baseStress, std::ostream& out) {
    out << "\n=== SWEEP " << sweepNo << "  |  base environmental stress "
        << std::fixed << std::setprecision(2) << baseStress << " ===\n";
    const std::vector<int> widths = nodeTableWidths();
    out << RuleLine(widths)
        << Cell("Node", kNode) << Cell("Type", kType) << Cell("Hazard", kHaz, true)
        << Cell("Level", kLvl) << Cell("Note", kNote) << rowEnd << RuleLine(widths);

    std::vector<std::string> diagnostics;        // exception messages, printed under the table

    logger_.beginTransaction();                  // UNIT IV: one transaction per sweep
    try {
        for (std::size_t i = 0; i < fleet_.size(); ++i) {
            BaseSensorNode* node = fleet_[i];    // base pointer -> runtime polymorphism
            SensorStation& station = stations_[i];

            std::map<std::string, ZoneProfile>::const_iterator zp = zones_.find(station.zone());
            const double exposure = (zp == zones_.end()) ? 1.0 : zp->second.exposure;
            node->setEnvironment(baseStress * exposure, sweepNo);

            try {
                TelemetryReading r = node->readTelemetry();            // virtual call
                if (!checksumValid(r))
                    throw TelemetryChecksumMismatch(node->deviceId(), computeChecksum(r), r.checksum);
                const AlertLevel level = node->evaluateThreshold();    // virtual call
                const double hazard = node->hazardIndex();             // virtual call
                station.recordReading(r, hazard, level);
                ++totals_.acceptedReadings;

                std::string note = "ok";
                if (level >= AlertLevel::Warning) {
                    std::ostringstream msg;
                    msg << toString(level) << " at " << station.name();
                    logger_.append(IncidentRecord::make(nextIncidentId_++, sweepNo, level, hazard,
                                                        station.id(), station.zone(), msg.str()));
                    ++totals_.incidentsLogged;
                    note = "incident logged to disk";
                }
                out << Cell(station.id(), kNode) << Cell(node->nodeType(), kType)
                    << NumCell(hazard, kHaz) << Cell(toString(level), kLvl)
                    << Cell(note, kNote) << rowEnd;
            }
            catch (const SensorFailureException& e) {
                station.markOffline();
                ++totals_.sensorFailures;
                diagnostics.push_back(e.what());
                out << Cell(station.id(), kNode) << Cell(node->nodeType(), kType) << Cell("--", kHaz, true)
                    << Cell("OFFLINE", kLvl) << Cell("sensor failure", kNote) << rowEnd;
            }
            catch (const TelemetryChecksumMismatch& e) {
                ++totals_.checksumErrors;
                diagnostics.push_back(e.what());
                out << Cell(station.id(), kNode) << Cell(node->nodeType(), kType) << Cell("--", kHaz, true)
                    << Cell("DISCARD", kLvl) << Cell("corrupt packet dropped", kNote) << rowEnd;
            }
            catch (const ThresholdOverflowException& e) {
                ++totals_.overflowErrors;
                diagnostics.push_back(e.what());
                out << Cell(station.id(), kNode) << Cell(node->nodeType(), kType) << Cell("--", kHaz, true)
                    << Cell("DISCARD", kLvl) << Cell("value beyond physical range", kNote) << rowEnd;
            }
        }
        logger_.commit();                        // all incidents of this sweep become durable together
    }
    catch (...) {
        logger_.rollback();                      // unexpected failure -> discard the whole sweep's log
        throw;
    }
    out << RuleLine(widths);
    for (std::size_t i = 0; i < diagnostics.size(); ++i) out << "  ! " << diagnostics[i] << '\n';

    // ---- aggregate stations into regional zones (operator+) ----
    std::map<std::string, AlertLevel> zoneLevels;
    std::vector<EvacuationOrder> orders;
    AlertLevel worst = AlertLevel::Normal;
    out << "Regional zones:\n";
    for (std::map<std::string, ZoneProfile>::const_iterator it = zones_.begin(); it != zones_.end(); ++it) {
        RegionalZone zone(it->first);
        for (std::size_t i = 0; i < stations_.size(); ++i)
            if (stations_[i].zone() == it->first) zone = zone + stations_[i];
        zoneLevels[it->first] = zone.worstLevel();
        worst = maxLevel(worst, zone.worstLevel());
        out << "  " << zone << '\n';
        if (zone.worstLevel() >= AlertLevel::Warning) {
            EvacuationOrder order = { it->first, zone.worstLevel(), zone.peakHazard(),
                                      it->second.population, it->second.route };
            orders.push_back(order);
        }
    }

    // ---- trigger sirens with a stateful functor and std::for_each ----
    SirenDispatcher result = std::for_each(sirens_.begin(), sirens_.end(), SirenDispatcher(zoneLevels));
    EarlyWarningSiren::setGlobalAlertLevel(worst);
    out << "Sirens changed state: " << result.changedCount()
        << "  |  global alert level: " << toString(EarlyWarningSiren::globalAlertLevel()) << '\n';

    // ---- priority-based evacuation routing (std::sort + functor) ----
    if (!orders.empty()) {
        std::sort(orders.begin(), orders.end(), EvacuationPriority());
        out << "Evacuation dispatch (highest priority first):\n" << alertTableHeader;
        for (std::size_t i = 0; i < orders.size(); ++i)
            out << Cell(orders[i].zone, Table::kZone) << Cell(toString(orders[i].level), Table::kLevel)
                << NumCell(orders[i].hazard, Table::kHazard) << NumCell(orders[i].population, Table::kPop, 0)
                << Cell(orders[i].route, Table::kRoute) << rowEnd;
        out << alertTableRule;
    }
}

void GridController::printFleetInventory(std::ostream& out) const {
    out << "Fleet inventory (" << fleet_.size() << " nodes, one std::vector<BaseSensorNode*>):\n";
    for (std::size_t i = 0; i < fleet_.size(); ++i) {
        out << "  - " << fleet_[i]->describe();                     // virtual call
        const AutonomousHybridProbe* hybrid = dynamic_cast<const AutonomousHybridProbe*>(fleet_[i]);
        if (hybrid)
            out << "  [diamond check: one shared root = " << (hybrid->singleRootInstance() ? "yes" : "NO") << ']';
        out << '\n';
    }
}

void GridController::printStationRanking(std::ostream& out, std::size_t topN) const {
    std::vector<SensorStation> ranked(stations_);                   // deep copies
    std::sort(ranked.begin(), ranked.end(), HazardDescending());    // uses operator>
    out << "Top " << topN << " stations by hazard index:\n";
    for (std::size_t i = 0; i < ranked.size() && i < topN; ++i)
        out << "  " << (i + 1) << ". " << ranked[i] << '\n';
}

void GridController::printPersistedIncidents(std::ostream& out, std::size_t lastN) {
    const std::vector<IncidentRecord> records = logger_.readAll();
    out << "Incident log '" << logger_.path() << "' holds " << records.size()
        << " committed record(s). Showing the last " << std::min(lastN, records.size()) << ":\n";
    std::vector<int> w;
    w.push_back(5); w.push_back(5); w.push_back(7); w.push_back(16); w.push_back(9); w.push_back(7);
    out << RuleLine(w) << Cell("Id", 5, true) << Cell("Sweep", 5, true) << Cell("Station", 7)
        << Cell("Zone", 16) << Cell("Level", 9) << Cell("Hazard", 7, true) << rowEnd << RuleLine(w);
    const std::size_t start = records.size() > lastN ? records.size() - lastN : 0;
    for (std::size_t i = start; i < records.size(); ++i) {
        const IncidentRecord& r = records[i];
        std::ostringstream id, sw;
        id << r.incidentId; sw << r.sweepNo;
        out << Cell(id.str(), 5, true) << Cell(sw.str(), 5, true) << Cell(r.stationId, 7)
            << Cell(r.zone, 16) << Cell(toString(static_cast<AlertLevel>(r.level)), 9)
            << NumCell(r.hazard, 7) << rowEnd;
    }
    out << RuleLine(w);
}

} // namespace DisasterCore
