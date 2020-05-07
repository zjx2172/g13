#ifndef __G13_H__
#define __G13_H__

// clang-format off
#include "helper.hpp"
#include "g13_device.hpp"
// clang-format on
#include <libusb-1.0/libusb.h>
#include <linux/uinput.h>
#include <csignal>
#include <unistd.h>
#include <fstream>
#include <functional>
#include <log4cpp/Category.hh>
#include <memory>
#include <utility>

// *************************************************************************

namespace G13 {

#define G13_LOG(message) log4cpp::Category::getRoot() << message
#define G13_ERR(message) log4cpp::Category::getRoot() << log4cpp::Priority::ERROR << message
#define G13_DBG(message) log4cpp::Category::getRoot() << log4cpp::Priority::DEBUG << message
#define G13_OUT(message) log4cpp::Category::getRoot() << log4cpp::Priority::INFO << message

// const size_t G13_INTERFACE = 0;
const size_t G13_KEY_ENDPOINT = 1;
const size_t G13_LCD_ENDPOINT = 2;
// const size_t G13_KEY_READ_TIMEOUT = 0;
const size_t G13_VENDOR_ID = 0x046d;
const size_t G13_PRODUCT_ID = 0xc21c;
const size_t G13_REPORT_SIZE = 8;

typedef int LINUX_KEY_VALUE;
const LINUX_KEY_VALUE BAD_KEY_VALUE = -1;

typedef int G13_KEY_INDEX;

// Initialized in g13_keys.cpp
static std::vector<std::string> G13_KEY_STRINGS;
static std::vector<std::string> G13_NONPARSED_KEYS;
static std::vector<std::string> G13_SYMBOLS;
static std::vector<std::string> G13_BTN_SEQ;

// *************************************************************************

using Helper::find_or_throw;
using Helper::repr;

// *************************************************************************

class G13_Action;
class G13_Stick;
class G13_LCD;
class G13_Profile;
class G13_Device;
class G13_Manager;

typedef std::shared_ptr<G13_Profile> ProfilePtr;
typedef std::shared_ptr<G13_Action> G13_ActionPtr;

class G13_CommandException : public std::exception {
   public:
    explicit G13_CommandException(std::string  reason) : _reason(std::move(reason)) {}
    ~G13_CommandException() noexcept override = default;
    [[nodiscard]] const char* what() const noexcept override { return _reason.c_str(); }

    std::string _reason;
};

// *************************************************************************

/*! holds potential actions which can be bound to G13 activity
 *
 */
class G13_Action {
   public:
    explicit G13_Action(G13_Device& keypad) : _keypad(keypad) {}
    virtual ~G13_Action();

    virtual void act(G13_Device&, bool is_down) = 0;
    virtual void dump(std::ostream&) const = 0;

    void act(bool is_down) { act(keypad(), is_down); }

    G13_Device& keypad() { return _keypad; }

    // [[maybe_unused]] [[nodiscard]] const G13_Device& keypad() const { return _keypad; }

    G13_Manager& manager();
    [[nodiscard]] const G13_Manager& manager() const;

   private:
    G13_Device& _keypad;
};

/*!
 * action to send one or more keystrokes
 */
class G13_Action_Keys : public G13_Action {
   public:
    G13_Action_Keys(G13_Device& keypad, const std::string& keys);
    ~G13_Action_Keys() override;

    void act(G13_Device&, bool is_down) override;
    void dump(std::ostream&) const override;

    std::vector<LINUX_KEY_VALUE> _keys;
};

/*!
 * action to send a string to the output pipe
 */
class G13_Action_PipeOut : public G13_Action {
   public:
    G13_Action_PipeOut(G13_Device& keypad, const std::string& out);
    ~G13_Action_PipeOut() override;

    void act(G13_Device&, bool is_down) override;
    void dump(std::ostream&) const override;

    std::string _out;
};

/*!
 * action to send a command to the g13
 */
class G13_Action_Command : public G13_Action {
   public:
    G13_Action_Command(G13_Device& keypad, std::string cmd);
    ~G13_Action_Command() override;

    void act(G13_Device&, bool is_down) override;
    void dump(std::ostream&) const override;

    std::string _cmd;
};

// *************************************************************************
template <class PARENT_T>
class G13_Actionable {
   public:
    G13_Actionable(PARENT_T& parent_arg, std::string  name)
        : _parent_ptr(&parent_arg), _name(std::move(name)) {}
    virtual ~G13_Actionable() { _parent_ptr = nullptr; }

    [[nodiscard]] G13_ActionPtr action() const { return _action; }
    [[nodiscard]] const std::string& name() const { return _name; }
    // PARENT_T& parent() { return *_parent_ptr; }
    // [[nodiscard]] const PARENT_T& parent() const { return *_parent_ptr; }
    // G13_Manager& manager() { return _parent_ptr->manager(); }
    [[nodiscard]] const G13_Manager& manager() const { return _parent_ptr->manager(); }

    virtual void set_action(const G13_ActionPtr& action) { _action = action; }

   protected:
    std::string _name;
    G13_ActionPtr _action;

   private:
    PARENT_T* _parent_ptr;
};

// *************************************************************************
/*! manages the bindings for a G13 key
 *
 */
class G13_Key : public G13_Actionable<G13_Profile> {
   public:
    void dump(std::ostream& o) const;
    [[nodiscard]] G13_KEY_INDEX index() const { return _index.index; }

    void parse_key(const unsigned char* byte, G13_Device* g13);

   protected:
    struct KeyIndex {
        explicit KeyIndex(int key) : index(key), offset(key / 8u), mask(1u << (key % 8u)) {}

        int index;
        unsigned char offset;
        unsigned char mask;
    };

    // G13_Profile is the only class able to instantiate G13_Keys
    friend class G13_Profile;

    G13_Key(G13_Profile& mode, const std::string& name, int index)
        : G13_Actionable<G13_Profile>(mode, name), _index(index), _should_parse(true) {}

    G13_Key(G13_Profile& mode, const G13_Key& key)
        : G13_Actionable<G13_Profile>(mode, key.name()),
          _index(key._index),
          _should_parse(key._should_parse) {
        set_action(key.action());
    }

    KeyIndex _index;
    bool _should_parse;
};

/*!
 * Represents a set of configured key mappings
 *
 * This allows a keypad to have multiple configured
 * profiles and switch between them easily
 */
class G13_Profile {
   public:
    G13_Profile(G13_Device& keypad, std::string  name_arg)
        : _keypad(keypad), _name(std::move(name_arg)) {
        _init_keys();
    }
    G13_Profile(const G13_Profile& other, std::string  name_arg)
        : _keypad(other._keypad), _name(std::move(name_arg)), _keys(other._keys) {}

    // search key by G13 keyname
    G13_Key* find_key(const std::string& keyname);

    void dump(std::ostream& o) const;

    void parse_keys(unsigned char* buf);
    [[nodiscard]] const std::string& name() const { return _name; }

    [[nodiscard]] const G13_Manager& manager() const;

   protected:
    G13_Device& _keypad;
    std::vector<G13_Key> _keys;
    std::string _name;

    void _init_keys();
};

class G13_FontChar {
   public:
    static const int CHAR_BUF_SIZE = 8;
    enum FONT_FLAGS { FF_ROTATE = 0x01 };

    G13_FontChar() {
        memset(bits_regular, 0, CHAR_BUF_SIZE);
        memset(bits_inverted, 0, CHAR_BUF_SIZE);
    }
    void set_character(unsigned char* data, int width, unsigned flags);
    unsigned char bits_regular[CHAR_BUF_SIZE]{};
    unsigned char bits_inverted[CHAR_BUF_SIZE]{};
};

class G13_Font {
   public:
    G13_Font();
    explicit G13_Font(const std::string& name, unsigned int width = 8);

    void set_character(unsigned int c, unsigned char* data);

    template <class ARRAY_T, class FLAGST>
    void install_font(ARRAY_T& data, FLAGST flags, int first = 0);

    [[nodiscard]] const std::string& name() const { return _name; }
    [[nodiscard]] unsigned int width() const { return _width; }

    const G13_FontChar& char_data(unsigned int x) { return _chars[x]; }

   protected:
    std::string _name;
    unsigned int _width;

    G13_FontChar _chars[256];

    // unsigned char font_basic[256][8];
    // unsigned char font_inverted[256][8];
};


// *************************************************************************

class G13_StickZone : public G13_Actionable<G13_Stick> {
   public:
    G13_StickZone(G13_Stick&, const std::string& name, const G13_ZoneBounds&, G13_ActionPtr = nullptr);

    bool operator==(const G13_StickZone& other) const { return _name == other._name; }

    void dump(std::ostream&) const;

    // void parse_key(unsigned char* byte, G13_Device* g13);
    void test(const G13_ZoneCoord& loc);
    void set_bounds(const G13_ZoneBounds& bounds) { _bounds = bounds; }

   protected:
    bool _active;

    G13_ZoneBounds _bounds;
};

typedef std::shared_ptr<G13_StickZone> G13_StickZonePtr;

// *************************************************************************

/*!
 * top level class, holds what would otherwise be in global variables
 */

class G13_Manager {
   public:
    G13_Manager();

    [[nodiscard]] G13_KEY_INDEX find_g13_key_value(const std::string& keyname) const;
    [[nodiscard]] std::string find_g13_key_name(G13_KEY_INDEX) const;

    [[nodiscard]] LINUX_KEY_VALUE find_input_key_value(const std::string& keyname) const;
    [[nodiscard]] std::string find_input_key_name(LINUX_KEY_VALUE) const;

    void set_logo(const std::string& fn) { logo_filename = fn; }
    int run();

    [[nodiscard]] std::string string_config_value(const std::string& name) const;
    void set_string_config_value(const std::string& name, const std::string& val);

    std::string make_pipe_name(G13_Device* d, bool is_input) const;

    void start_logging();
    void set_log_level(log4cpp::Priority::PriorityLevel lvl);
    void set_log_level(const std::string&);

   protected:
    void init_keynames();
    void display_keys();
    void discover_g13s(libusb_device** devs, ssize_t count, std::vector<G13_Device*>& g13s);
    void cleanup();

    std::string logo_filename;
    libusb_device** devs;

    libusb_context* ctx;
    std::vector<G13_Device*> g13s;

    std::map<G13_KEY_INDEX, std::string> g13_key_to_name;
    std::map<std::string, G13_KEY_INDEX> g13_name_to_key;
    std::map<LINUX_KEY_VALUE, std::string> input_key_to_name;
    std::map<std::string, LINUX_KEY_VALUE> input_name_to_key;

    std::map<std::string, std::string> _string_config_values;

    static bool running;
    static void signal_handler(int);
};

// *************************************************************************

// inlines

inline G13_Manager& G13_Action::manager() {
    return _keypad.manager();
}

inline const G13_Manager& G13_Action::manager() const {
    return _keypad.manager();
}

inline const G13_Manager& G13_Profile::manager() const {
    return _keypad.manager();
}

// *************************************************************************

}  // namespace G13

#endif  // __G13_H__
