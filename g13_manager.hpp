//
// Created by khampf on 13-05-2020.
//

#ifndef G13_G13_MANAGER_HPP
#define G13_G13_MANAGER_HPP

#include "g13.hpp"
#include "g13_log.hpp"
#include "g13_action.hpp"
#include <libusb-1.0/libusb.h>
#include "g13_device.hpp"
#include "g13_action.hpp"

#define CONTROL_DIR std::string("/tmp/")

/*!
 * top level class, holds what would otherwise be in global variables
 */
namespace G13 {
    class G13_Manager {
    private:
        G13_Manager();
    public:
        static G13_Manager* Instance(); // Singleton pattern instead of passing references around

        [[nodiscard]] static int find_g13_key_value(const std::string &keyname);

        [[nodiscard]] static std::string find_g13_key_name(int);

        [[nodiscard]] G13::LINUX_KEY_VALUE find_input_key_value(const std::string &keyname) const;

        [[nodiscard]] static std::string find_input_key_name(G13::LINUX_KEY_VALUE);

        void set_logo(const std::string &fn) { logo_filename = fn; }

        int run();

        [[nodiscard]] std::string string_config_value(const std::string &name) const;

        void set_string_config_value(const std::string &name, const std::string &val);

        std::string make_pipe_name(G13::G13_Device *d, bool is_input) const;

        void start_logging();

        [[maybe_unused]] void set_log_level(log4cpp::Priority::PriorityLevel lvl);

        void set_log_level(const std::string &);

    protected:
        static void init_keynames();

        void display_keys();

        void DiscoverG13s(libusb_device **devs, ssize_t count);

        void Cleanup();

        std::string logo_filename;
        libusb_device **devs;

        libusb_context *ctx;

        std::map<std::string, std::string> _string_config_values;

        static bool running;

        static void signal_handler(int);

        void setupDevice(G13::G13_Device *g13);

        static int LIBUSB_CALL
        hotplug_callback(struct libusb_context *ctx, struct libusb_device *dev,
                         libusb_hotplug_event event, void *user_data);

        static int OpenAndAddG13(libusb_device *dev);
    };
}

#endif //G13_G13_MANAGER_HPP
