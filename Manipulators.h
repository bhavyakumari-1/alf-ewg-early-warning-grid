// ============================================================================
//  Manipulators.h  -  UNIT IV : custom stream manipulators for the tabular
//                     alert-dispatch report.
//    * function-style manipulators : alertTableRule, alertTableHeader, rowEnd
//    * parameterised manipulators  : Cell, NumCell, RuleLine
//  None of them changes the stream's formatting flags (they pad manually).
// ============================================================================
#ifndef ALF_MANIPULATORS_H
#define ALF_MANIPULATORS_H

#include <iomanip>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

namespace DisasterCore {
namespace Alerts {

namespace Table {
    const int kZone = 16, kLevel = 9, kHazard = 8, kPop = 10, kRoute = 36;
}

inline std::string padRight(const std::string& s, int w) {
    return static_cast<int>(s.size()) >= w ? s.substr(0, static_cast<std::size_t>(w))
                                           : s + std::string(static_cast<std::size_t>(w) - s.size(), ' ');
}
inline std::string padLeft(const std::string& s, int w) {
    return static_cast<int>(s.size()) >= w ? s.substr(0, static_cast<std::size_t>(w))
                                           : std::string(static_cast<std::size_t>(w) - s.size(), ' ') + s;
}

// "| text   " - one text cell of a table row
class Cell {
    std::string text_;
    int width_;
    bool rightAligned_;
public:
    Cell(const std::string& text, int width, bool rightAligned = false)
        : text_(text), width_(width), rightAligned_(rightAligned) {}
    friend std::ostream& operator<<(std::ostream& os, const Cell& c) {
        return os << "| " << (c.rightAligned_ ? padLeft(c.text_, c.width_) : padRight(c.text_, c.width_)) << ' ';
    }
};

// "|   12.3 " - one numeric cell, right aligned with fixed precision
class NumCell {
    double value_;
    int width_;
    int precision_;
public:
    NumCell(double value, int width, int precision = 1)
        : value_(value), width_(width), precision_(precision) {}
    friend std::ostream& operator<<(std::ostream& os, const NumCell& n) {
        std::ostringstream tmp;
        tmp << std::fixed << std::setprecision(n.precision_) << n.value_;
        return os << Cell(tmp.str(), n.width_, true);
    }
};

// "+------+-----+" - horizontal rule for arbitrary column widths
class RuleLine {
    std::vector<int> widths_;
public:
    explicit RuleLine(const std::vector<int>& widths) : widths_(widths) {}
    friend std::ostream& operator<<(std::ostream& os, const RuleLine& r) {
        for (std::size_t i = 0; i < r.widths_.size(); ++i)
            os << '+' << std::string(static_cast<std::size_t>(r.widths_[i]) + 2, '-');
        return os << "+\n";
    }
};

inline std::ostream& rowEnd(std::ostream& os) { return os << "|\n"; }

inline std::ostream& alertTableRule(std::ostream& os) {
    std::vector<int> w;
    w.push_back(Table::kZone);  w.push_back(Table::kLevel); w.push_back(Table::kHazard);
    w.push_back(Table::kPop);   w.push_back(Table::kRoute);
    return os << RuleLine(w);
}

inline std::ostream& alertTableHeader(std::ostream& os) {
    alertTableRule(os);
    os << Cell("Zone", Table::kZone) << Cell("Level", Table::kLevel)
       << Cell("Hazard", Table::kHazard, true) << Cell("Population", Table::kPop, true)
       << Cell("Evacuation route", Table::kRoute) << rowEnd;
    return alertTableRule(os);
}

} // namespace Alerts
} // namespace DisasterCore

#endif
