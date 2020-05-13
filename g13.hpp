#ifndef __G13_H__
#define __G13_H__


#include "g13_device.hpp"
#include "g13_action.hpp"
#include "g13_log.hpp"
#include <libusb-1.0/libusb.h>

// *************************************************************************

namespace G13 {



// const size_t G13_INTERFACE = 0;
const size_t G13_KEY_ENDPOINT = 1;
const size_t G13_LCD_ENDPOINT = 2;
// const size_t G13_KEY_READ_TIMEOUT = 0;
const size_t G13_VENDOR_ID = 0x046d;
const size_t G13_PRODUCT_ID = 0xc21c;
const size_t G13_REPORT_SIZE = 8;

typedef int G13_KEY_INDEX;
typedef int LINUX_KEY_VALUE;

// *************************************************************************

class G13_Action;
class G13_Stick;
class G13_Profile;
class G13_Manager;

class G13_CommandException : public std::exception {
   public:
    explicit G13_CommandException(std::string  reason) : _reason(std::move(reason)) {}
    ~G13_CommandException() noexcept override = default;
    [[nodiscard]] const char* what() const noexcept override { return _reason.c_str(); }

    std::string _reason;
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
        set_action(key.action());   // TODO: do not invoke virtual member function from ctor
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

    [[maybe_unused]] [[nodiscard]] const G13_Manager& manager() const;

   protected:
    G13_Device& _keypad;
    std::vector<G13_Key> _keys;
    std::string _name;

    void _init_keys();
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


// *************************************************************************

/*!
 * top level class, holds what would otherwise be in global variables
 */

class G13_Manager {
   public:
    G13_Manager();

    [[nodiscard]] static G13_KEY_INDEX find_g13_key_value(const std::string& keyname) ;
    [[nodiscard]] static std::string find_g13_key_name(G13_KEY_INDEX) ;

    [[nodiscard]] LINUX_KEY_VALUE find_input_key_value(const std::string& keyname) const;
    [[nodiscard]] static std::string find_input_key_name(LINUX_KEY_VALUE) ;

    void set_logo(const std::string& fn) { logo_filename = fn; }
    int run();

    [[nodiscard]] std::string string_config_value(const std::string& name) const;
    void set_string_config_value(const std::string& name, const std::string& val);

    std::string make_pipe_name(G13_Device* d, bool is_input) const;

    void start_logging();

    [[maybe_unused]] void set_log_level(log4cpp::Priority::PriorityLevel lvl);
    void set_log_level(const std::string&);

   protected:
    static void init_keynames();
    void display_keys();
    void DiscoverG13s(libusb_device** devs, ssize_t count);
    void Cleanup();

    std::string logo_filename;
    libusb_device** devs;

    libusb_context* ctx;
    std::vector<G13_Device*> g13s;

    std::map<std::string, std::string> _string_config_values;

    static bool running;
    static void signal_handler(int);

    void setupDevice(G13_Device *g13);
    static int LIBUSB_CALL hotplug_callback(struct libusb_context *ctx, struct libusb_device *dev,
                                                  libusb_hotplug_event event, void *user_data);
    static int OpenAndAddG13(libusb_device *dev);

};

    // *************************************************************************

    [[maybe_unused]]
    inline const G13_Manager& G13_Profile::manager() const {
        return _keypad.manager();
    }

}  // namespace G13

#endif  // __G13_H__
