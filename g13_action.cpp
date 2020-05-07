//
// Created by khampf on 07-05-2020.
//

#include "g13.hpp"
#include "g13_action.hpp"
#include "g13_device.hpp"

// *************************************************************************
namespace G13 {
    G13_Action::~G13_Action() = default;

    G13_Action_Keys::G13_Action_Keys(G13_Device &keypad, const std::string &keys_string)
            : G13_Action(keypad) {
        auto keys = Helper::split < std::vector<std::string>>
        (keys_string, "+");

        for (auto &key : keys) {
            auto kval = manager().find_input_key_value(key);
            if (kval == BAD_KEY_VALUE) {
                throw G13_CommandException("create action unknown key : " + key);
            }
            _keys.push_back(kval);
        }

        std::vector<int> _keys_local;
    }

    G13_Action_Keys::~G13_Action_Keys() = default;

    void G13_Action_Keys::act(G13_Device &g13, bool is_down) {
        if (is_down) {
            for (int &_key : _keys) {
                g13.send_event(EV_KEY, _key, is_down);
                G13_LOG(log4cpp::Priority::DEBUG << "sending KEY DOWN " << _key);
            }
        } else {
            for (int i = _keys.size() - 1; i >= 0; i--) {
                g13.send_event(EV_KEY, _keys[i], is_down);
                G13_LOG(log4cpp::Priority::DEBUG << "sending KEY UP " << _keys[i]);
            }
        }
    }

    void G13_Action_Keys::dump(std::ostream &out) const {
        out << " SEND KEYS: ";

        for (size_t i = 0; i < _keys.size(); i++) {
            if (i)
                out << " + ";
            out << manager().find_input_key_name(_keys[i]);
        }
    }

    G13_Action_PipeOut::G13_Action_PipeOut(G13_Device &keypad, const std::string &out)
            : G13_Action(keypad), _out(out + "\n") {}

    G13_Action_PipeOut::~G13_Action_PipeOut() = default;

    void G13_Action_PipeOut::act(G13_Device &kp, bool is_down) {
        if (is_down) {
            kp.write_output_pipe(_out);
        }
    }

    void G13_Action_PipeOut::dump(std::ostream &o) const {
        o << "WRITE PIPE : " << Helper::repr(_out);
    }

    G13_Action_Command::G13_Action_Command(G13_Device &keypad, std::string cmd)
            : G13_Action(keypad), _cmd(std::move(cmd)) {}

    G13_Action_Command::~G13_Action_Command() = default;

    void G13_Action_Command::act(G13_Device &kp, bool is_down) {
        if (is_down) {
            keypad().command(_cmd.c_str());
        }
    }

    void G13_Action_Command::dump(std::ostream &o) const {
        o << "COMMAND : " << Helper::repr(_cmd);
    }

    // inlines
    inline G13_Manager& G13_Action::manager() {
        return _keypad.manager();
    }

    inline const G13_Manager& G13_Action::manager() const {
        return _keypad.manager();
    }
}