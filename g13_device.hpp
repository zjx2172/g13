//
// Created by khampf on 07-05-2020.
//
#ifndef G13_G13_DEVICE_HPP
#define G13_G13_DEVICE_HPP

#include <libusb-1.0/libusb.h>
#include <functional>
#include <linux/uinput.h>
#include <memory>
#include <map>
#include "g13_lcd.hpp"
#include "g13_stick.hpp"

namespace G13 {
// *************************************************************************

    class G13_Manager;
    class G13_Profile;
    class G13_Action;


    class G13_Font;

    typedef std::shared_ptr<G13_Profile> ProfilePtr;
    typedef std::shared_ptr<G13_Action> G13_ActionPtr;
    typedef std::shared_ptr<G13_Font> FontPtr;

    const size_t G13_NUM_KEYS = 40;

    class G13_Device {
    public:

        G13_Device(G13_Manager &manager, libusb_device_handle *handle, int id);

        G13_Manager &manager() { return _manager; }

        [[nodiscard]] const G13_Manager &manager() const { return _manager; }

        G13_LCD &lcd() { return _lcd; }

        [[nodiscard]] const G13_LCD &lcd() const { return _lcd; }

        G13_Stick &stick() { return _stick; }

        [[nodiscard]] const G13_Stick &stick() const { return _stick; }

        FontPtr switch_to_font(const std::string &name);

        void switch_to_profile(const std::string &name);

        ProfilePtr profile(const std::string &name);

        void dump(std::ostream &, int detail = 0);

        void command(char const *str);

        void read_commands();

        void read_config_file(const std::string &filename);

        int read_keys();

        void parse_joystick(unsigned char *buf);

        G13_ActionPtr make_action(const std::string &);

        void set_key_color(int red, int green, int blue);

        void set_mode_leds(int leds);

        void send_event(int type, int code, int val);

        void write_output_pipe(const std::string &out) const;

        void write_lcd(unsigned char *data, size_t size);

        bool is_set(int key);

        bool update(int key, bool v);

        // used by G13_Manager
        void cleanup();

        void register_context(libusb_context *ctx);

        void write_lcd_file(const std::string &filename);

        G13_Font &current_font() { return *_current_font; }

        G13_Profile &current_profile() { return *_current_profile; }

        [[nodiscard]] int id_within_manager() const { return _id_within_manager; }

        // typedef boost::function<void(const char*)> COMMAND_FUNCTION;
        typedef std::function<void(const char *)> COMMAND_FUNCTION;
        typedef std::map<std::string, COMMAND_FUNCTION> CommandFunctionTable;


    protected:
        void _init_fonts();

        void init_lcd();

        void _init_commands();

        // typedef void (COMMAND_FUNCTION)( G13_Device*, const char *, const char * );
        CommandFunctionTable _command_table;

        // struct timeval _event_time;
        struct input_event _event;

        int _id_within_manager;
        libusb_device_handle *handle;
        libusb_context *ctx;

        int _uinput_fid;

        int _input_pipe_fid;
        std::string _input_pipe_name;
        int _output_pipe_fid;
        std::string _output_pipe_name;

        std::map<std::string, FontPtr> _fonts;
        FontPtr _current_font;
        std::map<std::string, ProfilePtr> _profiles;
        ProfilePtr _current_profile;

        G13_Manager &_manager;
        G13_LCD _lcd;
        G13_Stick _stick;

        bool keys[G13_NUM_KEYS];
    };

/*
inline bool G13_Device::is_set(int key) {
    return keys[key];
}
*/

    inline bool G13_Device::update(int key, bool v) {
        bool old = keys[key];
        keys[key] = v;
        return old != v;
    }
} // namespace G13

#endif //G13_G13_DEVICE_HPP

