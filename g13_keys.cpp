/* This file contains code for managing keys an profiles
 *
 */

// TODO: check out why g13.h needs to be included before libevdev
// clang-format off
#include "g13.hpp"
#include "g13_device.hpp"
#include "g13_keys.hpp"
#include "g13_profile.hpp"
#include "helper.hpp"
#include <libevdev-1.0/libevdev/libevdev.h>
// clang-format on

namespace G13 {

void G13_Key::dump(std::ostream &o) const {
  o << G13_Manager::Instance()->find_g13_key_name(index()) << "(" << index()
    << ") : ";
  if (action()) {
    action()->dump(o);
  } else {
    o << "(no action)";
  }
}

// *************************************************************************

void G13_Key::ParseKey(const unsigned char *byte, G13_Device *g13) {
  bool key_is_down = byte[_index.offset] & _index.mask;
  auto key_state_changed = g13->update(_index.index, key_is_down);

  if (key_state_changed && _action) {
    _action->act(*g13, key_is_down);
  }
}

// *************************************************************************

} // namespace G13
