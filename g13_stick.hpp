//
// Created by khampf on 07-05-2020.
//

#ifndef G13_G13_STICK_HPP
#define G13_G13_STICK_HPP

#include <vector>
// #include "g13_device.hpp"
// #include "g13.hpp"
#include "helper.hpp"

namespace G13 {
class G13_Device;

typedef Helper::Coord<int> G13_StickCoord;
typedef Helper::Bounds<int> G13_StickBounds;
typedef Helper::Coord<double> G13_ZoneCoord;
typedef Helper::Bounds<double> G13_ZoneBounds;

// *************************************************************************

class G13_StickZone;

enum stick_mode_t {
  STICK_ABSOLUTE,
  // STICK_RELATIVE,
  STICK_KEYS,
  STICK_CALCENTER,
  STICK_CALBOUNDS,
  STICK_CALNORTH
};

class G13_Stick {
public:
  explicit G13_Stick(G13_Device &keypad);

  void parse_joystick(const unsigned char *buf);

  void set_mode(stick_mode_t);
  G13_StickZone *zone(const std::string &, bool create = false);
  void remove_zone(const G13_StickZone &zone);

  /*
    [[nodiscard]] const std::vector<G13_StickZone> &zones() const {
      return _zones;
    }
  */

  void dump(std::ostream &) const;

protected:
  void _recalc_calibrated();

  G13_Device &_keypad;
  std::vector<G13_StickZone> _zones;

  G13_StickBounds _bounds;
  G13_StickCoord _center_pos;
  G13_StickCoord _north_pos;

  G13_StickCoord _current_pos;

  stick_mode_t _stick_mode;
};

} // namespace G13

#endif // G13_G13_STICK_HPP
