#include "g13.hpp"
#include "g13_device.hpp"

#include <memory>
#include <utility>
#include <signal.h>
#include "helper.hpp"
#include "logo.hpp"

// *************************************************************************

#define CONTROL_DIR std::string("/tmp/")

namespace G13 {

// *************************************************************************


// *************************************************************************

void G13_Manager::discover_g13s(libusb_device** devs,
                                ssize_t count,
                                std::vector<G13_Device*>& g13s) {
    for (int i = 0; i < count; i++) {
        libusb_device_descriptor desc;
        int r = libusb_get_device_descriptor(devs[i], &desc);
        if (r < 0) {
            G13_ERR("Failed to get device descriptor");
            return;
        }
        if (desc.idVendor == G13_VENDOR_ID && desc.idProduct == G13_PRODUCT_ID) {
            libusb_device_handle* handle;
            int r = libusb_open(devs[i], &handle);
            if (r != 0) {
                G13_ERR("Error opening G13 device");
                return;
            }
            if (libusb_kernel_driver_active(handle, 0) == 1) {
                if (libusb_detach_kernel_driver(handle, 0) == 0) {
                    G13_ERR("Kernel driver detached");
                }
            }

            r = libusb_claim_interface(handle, 0);
            if (r < 0) {
                G13_ERR("Cannot Claim Interface");
                libusb_release_interface(handle, 0);
                libusb_close(handle);
                return;
            }
            g13s.push_back(new G13_Device(*this, handle, g13s.size()));
        }
    }
}

// *************************************************************************

void G13_Manager::cleanup() {
    G13_OUT("Cleaning up");
    for (auto & g13 : g13s) {
        g13->cleanup();
        delete g13;
    }
    libusb_exit(ctx);
}


G13_Manager::G13_Manager() : ctx(nullptr), devs(nullptr) {
    init_keynames();
}

// *************************************************************************

bool G13_Manager::running = true;

void G13_Manager::signal_handler(int signal) {
    G13_OUT("Caught signal " << signal << " (" << strsignal(signal) << ")");
    running = false;
}

std::string G13_Manager::string_config_value(const std::string& name) const {
    try {
        return Helper::find_or_throw(_string_config_values, name);
    } catch (...) {
        return "";
    }
}
void G13_Manager::set_string_config_value(const std::string& name, const std::string& value) {
    G13_DBG("set_string_config_value " << name << " = " << Helper::repr(value));
    _string_config_values[name] = value;
}

std::string G13_Manager::make_pipe_name(G13_Device* d, bool is_input) const {
    if (is_input) {
        std::string config_base = string_config_value("pipe_in");
        if (!config_base.empty()) {
            if (d->id_within_manager() == 0) {
                return config_base;
            } else {
                return config_base + "-" + std::to_string(d->id_within_manager());
            }
        }
        return CONTROL_DIR + "g13-" + std::to_string(d->id_within_manager());
    } else {
        std::string config_base = string_config_value("pipe_out");
        if (!config_base.empty()) {
            if (d->id_within_manager() == 0) {
                return config_base;
            } else {
                return config_base + "-" + std::to_string(d->id_within_manager());
            }
        }

        return CONTROL_DIR + "g13-" + std::to_string(d->id_within_manager()) + "_out";
    }
}

static int LIBUSB_CALL hotplug_callback(struct libusb_context *ctx, struct libusb_device *dev,
                      libusb_hotplug_event event, void *user_data) {
    static libusb_device_handle *handle = NULL;
    struct libusb_device_descriptor desc;
    int rc;

    (void)libusb_get_device_descriptor(dev, &desc);
    if (LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED == event) {
        rc = libusb_open(dev, &handle);
        if (LIBUSB_SUCCESS != rc) {
            fprintf(stderr,"Could not open USB device\n");
        }
    } else if (LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT == event) {
        if (handle) {
            libusb_close(handle);
            handle = NULL;
        }
    } else {
        fprintf(stderr, "Unhandled event %d", event);
    }
    return 0;
}

static int LIBUSB_CALL hotplug_callback_detach(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data)
{
    (void)ctx;
    (void)dev;
    (void)event;
    (void)user_data;

    // running = false;
    return 0;
}

int G13_Manager::run() {
    static const int class_id = LIBUSB_HOTPLUG_MATCH_ANY;

    display_keys();

    ssize_t cnt;
    int ret;

    ret = libusb_init(&ctx);
    if (ret != LIBUSB_SUCCESS) {
        G13_ERR("Initialization error: " << ret);
        cleanup();
        return EXIT_FAILURE;
    }
    libusb_set_option(ctx, LIBUSB_OPTION_LOG_LEVEL, 3);

    cnt = libusb_get_device_list(ctx, &devs);
    if (cnt < 0) {
        G13_ERR("Error while getting device list");
        cleanup();
        return EXIT_FAILURE;
    }

    discover_g13s(devs, cnt, g13s);
    libusb_free_device_list(devs, 1);
    G13_OUT("Found " << g13s.size() << " G13s");
    if (g13s.empty()) {
        G13_ERR("Unable to open any device");
        cleanup();
        return EXIT_FAILURE;
    }

    libusb_hotplug_callback_handle hp[2];
    if (libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG)) {
        G13_OUT("Registering hotplug callbacks");
        ret = libusb_hotplug_register_callback(nullptr, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED,
                                               LIBUSB_HOTPLUG_NO_FLAGS,
                                               G13_VENDOR_ID, G13_PRODUCT_ID, class_id, hotplug_callback,
                                               nullptr, &hp[0]);
        if (ret != LIBUSB_SUCCESS) {
            G13_ERR("Error registering hotplug callback 0");
        }
        ret = libusb_hotplug_register_callback(nullptr, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT,
                                               LIBUSB_HOTPLUG_NO_FLAGS, G13_VENDOR_ID,
                                               G13_PRODUCT_ID, class_id, hotplug_callback_detach, nullptr,
                                               &hp[1]);
        if (ret != LIBUSB_SUCCESS) {
            G13_ERR("Error registering hotplug callback 1");
        }
    }

    for (auto & g13 : g13s) {
        g13->register_context(ctx);
    }
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    if (!g13s.empty() && !logo_filename.empty()) {
        g13s[0]->write_lcd_file(logo_filename);
    }

    G13_OUT("Active Stick zones ");
    g13s[0]->stick().dump(std::cout);

    std::string config_fn = string_config_value("config");
    if (!config_fn.empty()) {
        G13_OUT("config_fn = " << config_fn);
        g13s[0]->read_config_file(config_fn);
    }

    do {
        if (!g13s.empty())
            for (auto & g13 : g13s) {
                int status = g13->read_keys();
                g13->read_commands();
                if (status < 0) {
                    running = false;
                }
            }
    } while (running);

    cleanup();
    G13_OUT("Exit");
    return EXIT_SUCCESS;
}
}  // namespace G13
