//
// Created by khampf on 13-05-2020.
//

#include "g13_profile.hpp"

namespace G13 {
    [[maybe_unused]]
    inline const G13::G13_Manager& G13_Profile::manager() const {
        return _keypad.manager();
    }

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

    G13_Key* G13_Profile::find_key(const std::string &keyname) {
        auto key = _keypad.manager().find_g13_key_value(keyname);
        if (key >= 0 && key < _keys.size()) {
            return &_keys[key];
        }
        return nullptr;
    }
}