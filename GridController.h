// ============================================================================
//  GridController.h  -  the central processing node of the ALF-EWG
//  Owns the stations, the polymorphic sensor fleet, the sirens and the log.
//  Each call to runSweep() performs one complete sensor sweep cycle.
// ============================================================================
#ifndef ALF_GRID_CONTROLLER_H
#define ALF_GRID_CONTROLLER_H

#include <istream>
#include <map>
#include <ostream>
#include <string>
#include <vector>
#include "EarlyWarningSiren.h"
#include "IncidentLogger.h"
#include "SensorNodes.h"
#include "SensorStation.h"

namespace DisasterCore {

struct ZoneProfile {
    unsigned population;
    double exposure;          // multiplies the simulated environmental stress
    std::string route;        // pre-planned evacuation route
    ZoneProfile() : population(0), exposure(1.0), route() {}
    ZoneProfile(unsigned p, double e, const std::string& r) : population(p), exposure(e), route(r) {}
};

struct SweepStats {
    unsigned acceptedReadings;
    unsigned sensorFailures;
    unsigned checksumErrors;
    unsigned overflowErrors;
    unsigned incidentsLogged;
    SweepStats() : acceptedReadings(0), sensorFailures(0), checksumErrors(0),
                   overflowErrors(0), incidentsLogged(0) {}
};

class GridController {
    std::vector<Sensors::SensorStation> stations_;
    std::vector<Sensors::BaseSensorNode*> fleet_;       // base-class pointers -> dynamic dispatch
    std::vector<Alerts::EarlyWarningSiren> sirens_;
    std::map<std::string, ZoneProfile> zones_;
    Alerts::IncidentLogger logger_;
    std::uint32_t nextIncidentId_;
    SweepStats totals_;

    Sensors::BaseSensorNode* findNode(const std::string& id) const;

public:
    explicit GridController(const std::string& logPath);
    ~GridController();                                  // deletes every node via virtual destructor
    GridController(const GridController&) = delete;
    GridController& operator=(const GridController&) = delete;

    void addZone(const std::string& name, const ZoneProfile& profile);
    std::size_t loadStations(std::istream& in);         // uses SensorStation::operator>>
    void deployFleet();
    bool injectFault(const std::string& nodeId, Sensors::FaultMode mode);
    bool drainBattery(const std::string& nodeId, double percent);

    void runSweep(unsigned sweepNo, double baseStress, std::ostream& out);

    void printFleetInventory(std::ostream& out) const;
    void printStationRanking(std::ostream& out, std::size_t topN) const;
    void printPersistedIncidents(std::ostream& out, std::size_t lastN);

    const SweepStats& totals() const { return totals_; }
    std::size_t fleetSize() const { return fleet_.size(); }
    std::size_t sirenCount() const { return sirens_.size(); }
    const std::vector<Sensors::SensorStation>& stations() const { return stations_; }
};

} // namespace DisasterCore

#endif
