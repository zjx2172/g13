/* This file contains code for managing keys an profiles
 *
 */

// TODO: check out why g13.h needs to be included before libevdev
// clang-format off
#include "g13.hpp"
#include "g13_device.hpp"
#include "g13_keys.hpp"
#include "helper.hpp"
#include <libevdev-1.0/libevdev/libevdev.h>
// clang-format on

namespace G13 {

    void G13_Profile::_init_keys() {
        // create a G13_Key entry for every key in G13_KEY_SEQ
        int key_index = 0;
        // std::string str = G13_KEY_STRINGS[0];

        for (auto &symbol : G13_KEY_STRINGS) {
            _keys.emplace_back(G13_Key(*this, symbol, key_index));
            key_index++;
        }
        assert(_keys.size() == G13_NUM_KEYS);

        // now disable testing for keys in G13_NONPARSED_KEY_SEQ
        for (auto &symbol : G13_NONPARSED_KEYS) {
            G13_Key *key = find_key(symbol);
            key->_should_parse = false;
        }
    }

    void G13_Key::dump(std::ostream &o) const {
        o << manager().find_g13_key_name(index()) << "(" << index() << ") : ";
        if (action()) {
            action()->dump(o);
        } else {
            o << "(no action)";
        }
    }

    void G13_Profile::dump(std::ostream &o) const {
        o << "Profile " << Helper::repr(name()) << std::endl;
        // BOOST_FOREACH (const G13_Key& key, _keys) {
        for (auto &key : _keys) {
            if (key.action()) {
                o << "   ";
                key.dump(o);
                o << std::endl;
            }
        }
    }

    void G13_Profile::parse_keys(unsigned char *buf) {
        buf += 3;
        for (auto &_key : _keys) {
            if (_key._should_parse) {
                _key.parse_key(buf, &_keypad);
            }
        }
    }

    G13_Key *G13_Profile::find_key(const std::string &keyname) {
        auto key = _keypad.manager().find_g13_key_value(keyname);
        if (key >= 0 && key < _keys.size()) {
            return &_keys[key];
        }
        return nullptr;
    }

// *************************************************************************

    void G13_Key::parse_key(const unsigned char *byte, G13_Device *g13) {
        bool key_is_down = byte[_index.offset] & _index.mask;
        auto key_state_changed = g13->update(_index.index, key_is_down);

        if (key_state_changed && _action) {
            _action->act(*g13, key_is_down);
        }
    }

// *************************************************************************

    void G13_Manager::init_keynames() {

        int key_index = 0;

        // setup maps to let us convert between strings and G13 key names
        for (auto &name : G13_KEY_STRINGS) {
            g13_key_to_name[key_index] = name;
            g13_name_to_key[name] = key_index;
            G13_DBG("mapping G13 " << name << " = " << key_index);
            key_index++;
        }

        // setup maps to let us convert between strings and linux key names
        for (auto &symbol : G13_SYMBOLS) {
            auto keyname = std::string("KEY_" + std::string(symbol));

            int code = libevdev_event_code_from_name(EV_KEY, keyname.c_str());
            if (code < 0) {
                G13_ERR("No input event code found for " << keyname);
            } else {
                // TODO: this seems to map ok but the result is off
                // assert(keyname.compare(libevdev_event_code_get_name(EV_KEY,code)) == 0);
                // linux/input-event-codes.h

                input_key_to_name[code] = symbol;
                input_name_to_key[symbol] = code;
                G13_DBG("mapping " << symbol << " " << keyname << "=" << code);
            }
        }

        // setup maps to let us convert between strings and linux button names
        for (auto &symbol : G13_BTN_SEQ) {
            auto name = std::string("M" + std::string(symbol));
            auto keyname = std::string("BTN_" + std::string(symbol));
            int code = libevdev_event_code_from_name(EV_KEY, keyname.c_str());
            if (code < 0) {
                G13_ERR("No input event code found for " << keyname);
            } else {
                input_key_to_name[code] = name;
                input_name_to_key[name] = code;
                G13_DBG("mapping " << name << " " << keyname << "=" << code);
            }
        }
    }

    LINUX_KEY_VALUE
    G13_Manager::find_g13_key_value(const std::string &keyname) const {
        auto i = g13_name_to_key.find(keyname);
        if (i == g13_name_to_key.end()) {
            return BAD_KEY_VALUE;
        }
        return i->second;
    }

    LINUX_KEY_VALUE
    G13_Manager::find_input_key_value(const std::string &keyname) const {
        // if there is a KEY_ prefix, strip it off
        if (!strncmp(keyname.c_str(), "KEY_", 4)) {
            return find_input_key_value(keyname.c_str() + 4);
        }

        auto i = input_name_to_key.find(keyname);
        if (i == input_name_to_key.end()) {
            return BAD_KEY_VALUE;
        }
        return i->second;
    }

    std::string G13_Manager::find_input_key_name(LINUX_KEY_VALUE v) const {
        try {
            return Helper::find_or_throw(input_key_to_name, v);
        } catch (...) {
            return "(unknown linux key)";
        }
    }

    std::string G13_Manager::find_g13_key_name(G13_KEY_INDEX v) const {
        try {
            return Helper::find_or_throw(g13_key_to_name, v);
        } catch (...) {
            return "(unknown G13 key)";
        }
    }

    void G13_Manager::display_keys() {
        typedef std::map<std::string, int> mapType;
        G13_OUT("Known keys on G13:");
        G13_OUT(Helper::map_keys_out(g13_name_to_key));

        G13_OUT("Known keys to map to:");
        G13_OUT(Helper::map_keys_out(input_name_to_key));
    }

}  // namespace G13
