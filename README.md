# Autonomous Landslide & Flash-Flood Early Warning Grid (ALF-EWG)

**Course:** Object-Oriented Programming Using C++ (CIC-211)
**Institute:** Maharaja Agrasen Institute of Technology, Dept. of Computer Science & Engineering
**Project:** Option 1 - ALF-EWG
**Student:** Bhavya Kumari  |  **Enrollment No.:** 02096402725  (individual submission)

A console back-end for a network of solar-powered IoT telemetry nodes (soil moisture / geophone probes,
acoustic tiltmeters, river-level sonar gauges and hybrid solar + LoRa probes). It ingests sensor sweeps,
computes hazard indices, aggregates stations into regional zones, triggers sirens, plans prioritised
evacuation routes and logs incidents to a transactional binary file. All data is **simulated**.

## Build and run

Requires any C++11 (or newer) compiler: g++, clang++ or MinGW. Run everything from the project root.

```bash
make            # builds ./alf_ewg
make run        # builds and runs
make sanitize   # AddressSanitizer + UBSan run (Linux/macOS) - verifies no leaks
make clean
```

Without `make` (e.g. Windows + MinGW):

```bash
g++ -std=c++11 -Wall -Wextra -Iinclude src/*.cpp -o alf_ewg
./alf_ewg
```

The program writes `alf_ewg_incidents.bin` (persistent incident log) and `stations_snapshot.txt` into the
current directory. The log is reset on every start; run `./alf_ewg --append` to keep and extend it.
Output is deterministic (own LCG random generator), so every run prints the same tables.

## Project layout

```
include/   headers (one concept per file)
src/       implementation + main.cpp (one demo section per syllabus unit + full simulation)
data/      stations.txt - station configuration read with operator>>
docs/      Project report (docx/pdf) and class diagrams
```

## Where each syllabus requirement lives

| Unit | Requirement | Implemented in |
|------|-------------|----------------|
| I | Namespaces `DisasterCore::Sensors`, `DisasterCore::Alerts` | all headers |
| I | Inline risk-index functions | `HazardMath.h` |
| I | Overloaded `computeHazardIndex()` (5 overloads) | `HazardMath.h` |
| I | Call by reference / return by reference | `toLocalGridKm`, `scaleCoordinate`, `accumulate`, `TelemetryBuffer::at` |
| I | Friend class `TelemetryAuditor` | `TelemetryAuditor.h/.cpp` |
| II | Abstraction + const correctness | `SensorStation`, `EarlyWarningSiren`, `TelemetryBuffer` |
| II | Static data/functions (station count, global alert level) | `SensorStation`, `EarlyWarningSiren` |
| II | Parameterized + dynamic ctors, deep copy, destructor | `TelemetryBuffer.h/.cpp` |
| II | `operator+`, `operator>`, `operator<<`, `operator>>` | `SensorStation.h/.cpp` |
| III | Abstract `BaseSensorNode` (2 pure virtuals) | `SensorNodes.h` |
| III | Diamond solved with virtual bases | `DisasterTelemetryDevice` -> `SolarPoweredNode` / `WirelessLoRaTransceiver` -> `AutonomousHybridProbe` |
| III | Containership (`PowerSubsystem`) | `BaseSensorNode::power_` |
| III | `std::vector<BaseSensorNode*>` dynamic dispatch | `GridController::runSweep` |
| IV | Custom exceptions deriving `std::exception` | `Exceptions.h` |
| IV | Binary `std::fstream` log, rollback with `seekp()/seekg()` | `IncidentLogger.h/.cpp` |
| IV | STL, functors, manipulators | `GridController.cpp`, `Functors.h`, `Manipulators.h` |

See `docs/` for the full project report with design explanations and sample output.

## Submission

```bash
git init && git add . && git commit -m "ALF-EWG CIC-211 project - Bhavya Kumari 02096402725"
git branch -M main
git remote add origin <your-repository-url>
git push -u origin main
```
Then zip the folder and upload it through the submission Google Form (deadline 27 Nov 2026, 11:59 PM).
