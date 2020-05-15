//
// Created by khampf on 07-05-2020.
//

#include <fstream>
#include <unistd.h>
// #include <libnet.h>
#include "g13.hpp"
#include "g13_device.hpp"
#include "g13_fonts.hpp"
#include "g13_log.hpp"
#include "g13_manager.hpp"
#include "g13_profile.hpp"
#include "g13_stick.hpp"
#include "logo.hpp"

namespace G13 {
// *************************************************************************

G13_Device::G13_Device(libusb_device *dev, libusb_device_handle *handle, int _id)
    : _lcd(*this), _stick(*this), device(dev), handle(handle), _id_within_manager(_id),
      _uinput_fid(-1), ctx(nullptr) {
  _current_profile = std::make_shared<G13_Profile>(*this, "default");
  _profiles["default"] = _current_profile;

  for (bool &key : keys) {
    key = false;
  }

  lcd().image_clear();

  InitFonts();
  _init_commands();
}

// *************************************************************************

std::string G13_Device::describe_libusb_error_code(int code) {
  auto description = std::string(libusb_error_name(code)) + " " +
                     std::string(libusb_strerror((libusb_error)code));
  return std::move(description);
  // return "unknown error";
}

int g13_create_fifo(const char *fifo_name) {
  // mkfifo(g13->fifo_name(), 0777); - didn't work
  mkfifo(fifo_name, 0666);
  chmod(fifo_name, 0777);

  return open(fifo_name, O_RDWR | O_NONBLOCK);
}

int g13_create_uinput(G13_Device *g13) {
  struct uinput_user_dev uinp {};
  struct input_event event {};
  const char *dev_uinput_fname =
      access("/dev/input/uinput", F_OK) == 0
          ? "/dev/input/uinput"
          : access("/dev/uinput", F_OK) == 0 ? "/dev/uinput" : nullptr;
  if (!dev_uinput_fname) {
    G13_ERR("Could not find an uinput device");
    return -1;
  }
  if (access(dev_uinput_fname, W_OK) != 0) {
    G13_ERR(dev_uinput_fname << " doesn't grant write permissions");
    return -1;
  }
  int ufile = open(dev_uinput_fname, O_WRONLY | O_NDELAY);
  if (ufile <= 0) {
    G13_ERR("Could not open uinput");
    return -1;
  }
  memset(&uinp, 0, sizeof(uinp));
  char name[] = "G13";
  strncpy(uinp.name, name, sizeof(name));
  uinp.id.version = 1;
  uinp.id.bustype = BUS_USB;
  uinp.id.product = G13_PRODUCT_ID;
  uinp.id.vendor = G13_VENDOR_ID;
  uinp.absmin[ABS_X] = 0;
  uinp.absmin[ABS_Y] = 0;
  uinp.absmax[ABS_X] = 0xff;
  uinp.absmax[ABS_Y] = 0xff;
  //  uinp.absfuzz[ABS_X] = 4;
  //  uinp.absfuzz[ABS_Y] = 4;
  //  uinp.absflat[ABS_X] = 0x80;
  //  uinp.absflat[ABS_Y] = 0x80;

  ioctl(ufile, UI_SET_EVBIT, EV_KEY);
  ioctl(ufile, UI_SET_EVBIT, EV_ABS);
  /*  ioctl(ufile, UI_SET_EVBIT, EV_REL);*/
  ioctl(ufile, UI_SET_MSCBIT, MSC_SCAN);
  ioctl(ufile, UI_SET_ABSBIT, ABS_X);
  ioctl(ufile, UI_SET_ABSBIT, ABS_Y);
  /*  ioctl(ufile, UI_SET_RELBIT, REL_X);
   ioctl(ufile, UI_SET_RELBIT, REL_Y);*/
  for (int i = 0; i < 256; i++) {
    ioctl(ufile, UI_SET_KEYBIT, i);
  }

  // Mouse buttons
  for (int i = 0x110; i < 0x118; i++) {
    ioctl(ufile, UI_SET_KEYBIT, i);
  }
  ioctl(ufile, UI_SET_KEYBIT, BTN_THUMB);

  int retcode = write(ufile, &uinp, sizeof(uinp));
  if (retcode < 0) {
    G13_ERR("Could not write to uinput device (" << retcode << ")");
    return -1;
  }
  retcode = ioctl(ufile, UI_DEV_CREATE);
  if (retcode) {
    G13_ERR("Error creating uinput device for G13");
    return -1;
  }
  return ufile;
}

// *************************************************************************

void G13_Device::send_event(int type, int code, int val) {
  memset(&_event, 0, sizeof(_event));
  gettimeofday(&_event.time, 0);
  _event.type = type;
  _event.code = code;
  _event.value = val;
  write(_uinput_fid, &_event, sizeof(_event));
}

void G13_Device::write_output_pipe(const std::string &out) const {
  write(_output_pipe_fid, out.c_str(), out.size());
}

void G13_Device::set_mode_leds(int leds) {
  unsigned char usb_data[] = {5, 0, 0, 0, 0};
  usb_data[1] = leds;
  int r = libusb_control_transfer(
      handle, LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE, 9, 0x305,
      0, usb_data, 5, 1000);
  if (r != 5) {
    G13_ERR("Problem sending data");
    return;
  }
}

void G13_Device::set_key_color(int red, int green, int blue) {
  int error;
  unsigned char usb_data[] = {5, 0, 0, 0, 0};
  usb_data[1] = red;
  usb_data[2] = green;
  usb_data[3] = blue;

  error = libusb_control_transfer(
      handle, LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE, 9, 0x307,
      0, usb_data, 5, 1000);
  if (error != 5) {
    G13_ERR("Problem sending data");
    return;
  }
}

/*! reads and processes key state report from G13
 *
 */
int G13_Device::read_keys() {
  unsigned char buffer[G13_REPORT_SIZE];
  int size;
  int error =
      libusb_interrupt_transfer(handle, LIBUSB_ENDPOINT_IN | G13_KEY_ENDPOINT,
                                buffer, G13_REPORT_SIZE, &size, 100);

  if (error && error != LIBUSB_ERROR_TIMEOUT) {
    G13_ERR("Error while reading keys: "
            << error << " (" << describe_libusb_error_code(error) << ")");
    if (error == LIBUSB_ERROR_NO_DEVICE || error == LIBUSB_ERROR_IO) {
      G13_DBG("Giving libusb a nudge");
      libusb_handle_events(ctx);
    }
  }
  if (size == G13_REPORT_SIZE) {
    parse_joystick(buffer);
    _current_profile->ParseKeys(buffer);
    send_event(EV_SYN, SYN_REPORT, 0);
  }
  return 0;
}

void G13_Device::read_config_file(const std::string &filename) {
  std::ifstream s(filename);

  G13_OUT("reading configuration from " << filename);
  while (s.good()) {
    // grab a line
    char buf[1024];
    buf[0] = 0;
    buf[sizeof(buf) - 1] = 0;
    s.getline(buf, sizeof(buf) - 1);

    // strip comment
    char *comment = strchr(buf, '#');
    if (comment) {
      comment--;
      while (comment > buf && isspace(*comment))
        comment--;
      *comment = 0;
    }

    // send it
    if (buf[0]) {
      G13_OUT("  cfg: " << buf);
      command(buf);
    }
  }
}

void G13_Device::read_commands() {
  fd_set set;
  FD_ZERO(&set);
  FD_SET(_input_pipe_fid, &set);
  struct timeval tv {};
  tv.tv_sec = 0;
  tv.tv_usec = 0;
  int ret = select(_input_pipe_fid + 1, &set, nullptr, nullptr, &tv);
  if (ret > 0) {
    unsigned char buf[1024 * 1024];
    memset(buf, 0, 1024 * 1024);
    ret = read(_input_pipe_fid, buf, 1024 * 1024);
    G13_LOG(log4cpp::Priority::DEBUG << "read " << ret << " characters");

    if (ret ==
        960) { // TODO probably image, for now, don't test, just assume image
      lcd().image(buf, ret);
    } else {
      std::string buffer = reinterpret_cast<const char *>(buf);
      auto lines = Helper::split<std::vector<std::string>>(
          buffer, "\n\r", Helper::split::no_empties);

      for (auto &cmd : lines) {
        auto command_comment = Helper::split<std::vector<std::string>>(
            cmd, "#", Helper::split::no_empties);

        if (!command_comment.empty() && command_comment[0] != std::string("")) {
          G13_OUT("command: " << command_comment[0]);
          command(command_comment[0].c_str());
        }
      }
    }
  }
}

FontPtr G13_Device::switch_to_font(const std::string &name) {
  FontPtr rv = _fonts[name];
  if (rv) {
    _current_font = rv;
  }
  return rv;
}

void G13_Device::switch_to_profile(const std::string &name) {
  _current_profile = profile(name);
}

ProfilePtr G13_Device::profile(const std::string &name) {
  ProfilePtr rv = _profiles[name];
  if (!rv) {
    rv = std::make_shared<G13_Profile>(*_current_profile, name);
    _profiles[name] = rv;
  }
  return rv;
}

G13_ActionPtr G13_Device::make_action(const std::string &action) {
  if (action.empty()) {
    throw G13_CommandException("empty action string");
  }
  if (action[0] == '>') {
    return G13_ActionPtr(new G13_Action_PipeOut(*this, &action[1]));
  } else if (action[0] == '!') {
    return G13_ActionPtr(new G13_Action_Command(*this, &action[1]));
  } else {
    return G13_ActionPtr(new G13_Action_Keys(*this, action));
  }
  // UNREACHABLE: throw G13_CommandException("can't create action for " +
  // action);
}

// *************************************************************************

void G13_Device::dump(std::ostream &o, int detail) {
  o << "G13 id=" << id_within_manager() << std::endl;
  o << "   input_pipe_name=" << Helper::repr(_input_pipe_name) << std::endl;
  o << "   output_pipe_name=" << Helper::repr(_output_pipe_name) << std::endl;
  o << "   current_profile=" << _current_profile->name() << std::endl;
  o << "   current_font=" << _current_font->name() << std::endl;

  if (detail > 0) {
    o << "STICK" << std::endl;
    stick().dump(o);
    if (detail == 1) {
      _current_profile->dump(o);
    } else {
      for (auto &_profile : _profiles) {
        _profile.second->dump(o);
      }
    }
  }
}

struct command_adder {
  command_adder(G13_Device::CommandFunctionTable &t, const char *name)
      : _t(t), _name(name) {}

  command_adder(G13_Device::CommandFunctionTable &t, const char *name,
                G13_Device::COMMAND_FUNCTION f)
      : _t(t), _name(name) {
    _t[_name] = std::move(f);
  }

  G13_Device::CommandFunctionTable &_t;
  std::string _name;

  command_adder &operator+=(G13_Device::COMMAND_FUNCTION f) {
    _t[_name] = std::move(f);
    return *this;
  };
};

void G13_Device::_init_commands() {
  using Helper::advance_ws;
  // const char *remainder;

  command_adder add_out(_command_table, "out", [this](const char *remainder) {
    lcd().write_string(remainder);
  });

  command_adder add_pos(_command_table, "pos", [this](const char *remainder) {
    int row, col;
    if (sscanf(remainder, "%i %i", &row, &col) == 2) {
      lcd().write_pos(row, col);
    } else {
      G13_ERR("bad pos : " << remainder);
    }
  });

  command_adder add_bind(_command_table, "bind", [this](const char *remainder) {
    std::string keyname;
    advance_ws(remainder, keyname);
    std::string action = remainder;
    try {
      if (auto key = _current_profile->FindKey(keyname)) {
        key->set_action(make_action(action));
      } else if (auto stick_key = _stick.zone(keyname)) {
        stick_key->set_action(make_action(action));
      } else {
        G13_ERR("bind key " << keyname << " unknown");
        return;
      }
      G13_LOG(log4cpp::Priority::DEBUG << "bind " << keyname << " [" << action
                                       << "]");
    } catch (const std::exception &ex) {
      G13_ERR("bind " << keyname << " " << action << " failed : " << ex.what());
    }
  });

  command_adder add_profile(
      _command_table, "profile",
      [this](const char *remainder) { switch_to_profile(remainder); });

  command_adder add_font(_command_table, "font", [this](const char *remainder) {
    switch_to_font(remainder);
  });

  command_adder add_mod(_command_table, "mod", [this](const char *remainder) {
    set_mode_leds(atoi(remainder));
  });

  command_adder add_textmode(
      _command_table, "textmode",
      [this](const char *remainder) { lcd().text_mode = atoi(remainder); });

  command_adder add_rgb(_command_table, "rgb", [this](const char *remainder) {
    int red, green, blue;
    if (sscanf(remainder, "%i %i %i", &red, &green, &blue) == 3) {
      set_key_color(red, green, blue);
    } else {
      G13_ERR("rgb bad format: <" << remainder << ">");
    }
  });

  command_adder add_stickmode(
      _command_table, "stickmode", [this](const char *remainder) {
        std::string mode = remainder;
        // TODO: this could be part of a G13::Constants class I think
        const std::set<std::string> modes = {"ABSOLUTE",  "RELATIVE",
                                             "KEYS",      "CALCENTER",
                                             "CALBOUNDS", "CALNORTH"};
        int index = 0;
        for (auto &test : modes) {
          if (test == mode) {
            _stick.set_mode((G13::stick_mode_t)index);
            return;
          }
          index++;
        }
        G13_ERR("unknown stick mode : <" << mode << ">");
      });

  command_adder add_stickzone(
      _command_table, "stickzone", [this](const char *remainder) {
        std::string operation, zonename;
        advance_ws(remainder, operation);
        advance_ws(remainder, zonename);
        if (operation == "add") {
          /* G13_StickZone* zone = */
          _stick.zone(zonename, true);
        } else {
          G13_StickZone *zone = _stick.zone(zonename);
          if (!zone) {
            throw G13_CommandException("unknown stick zone");
          }
          if (operation == "action") {
            zone->set_action(make_action(remainder));
          } else if (operation == "bounds") {
            double x1, y1, x2, y2;
            if (sscanf(remainder, "%lf %lf %lf %lf", &x1, &y1, &x2, &y2) != 4) {
              throw G13_CommandException("bad bounds format");
            }
            zone->set_bounds(G13_ZoneBounds(x1, y1, x2, y2));

          } else if (operation == "del") {
            _stick.remove_zone(*zone);
          } else {
            G13_ERR("unknown stickzone operation: <" << operation << ">");
          }
        }
      });

  command_adder add_dump(_command_table, "dump", [this](const char *remainder) {
    std::string target;
    advance_ws(remainder, target);
    if (target == "all") {
      dump(std::cout, 3);
    } else if (target == "current") {
      dump(std::cout, 1);
    } else if (target == "summary") {
      dump(std::cout, 0);
    } else {
      G13_ERR("unknown dump target: <" << target << ">");
    }
  });

  command_adder add_log_level(_command_table, "log_level",
                              [this](const char *remainder) {
                                std::string level;
                                advance_ws(remainder, level);
                                G13_Manager::Instance()->SetLogLevel(level);
                              });

  command_adder add_refresh(
      _command_table, "refresh",
      [this](const char *remainder) { lcd().image_send(); });

  command_adder add_clear(_command_table, "clear",
                          [this](const char *remainder) {
                            lcd().image_clear();
                            lcd().image_send();
                          });
}

void G13_Device::command(char const *str) {
  const char *remainder = str;

  try {
    using Helper::advance_ws;

    std::string cmd;
    advance_ws(remainder, cmd);

    auto i = _command_table.find(cmd);
    if (i == _command_table.end()) {
      G13_ERR("unknown command : " << cmd);
    } else {
      COMMAND_FUNCTION f = i->second;
      f(remainder);
    }
  } catch (const std::exception &ex) {
    G13_ERR("command failed : " << ex.what());
  }
}

void G13_Device::register_context(libusb_context *_ctx) {
  ctx = _ctx;

  int leds = 0;
  int red = 0;
  int green = 0;
  int blue = 255;
  init_lcd();

  set_mode_leds(leds);
  set_key_color(red, green, blue);

  write_lcd(g13_logo, sizeof(g13_logo));

  _uinput_fid = g13_create_uinput(this);

  _input_pipe_name = G13_Manager::Instance()->MakePipeName(this, true);
  _input_pipe_fid = g13_create_fifo(_input_pipe_name.c_str());
  if (_input_pipe_fid == -1) {
    G13_ERR("failed opening input pipe " << _input_pipe_name);
  }
  _output_pipe_name = G13_Manager::Instance()->MakePipeName(this, false);
  _output_pipe_fid = g13_create_fifo(_output_pipe_name.c_str());
  if (_output_pipe_fid == -1) {
    G13_ERR("failed opening output pipe " << _output_pipe_name);
  }
}

void G13_Device::cleanup() {
  remove(_input_pipe_name.c_str());
  remove(_output_pipe_name.c_str());
  ioctl(_uinput_fid, UI_DEV_DESTROY);
  close(_uinput_fid);
  libusb_release_interface(handle, 0);
  libusb_close(handle);
}

  libusb_device_handle *G13_Device::Handle() const {
    return handle;
  }

  libusb_device *G13_Device::Device() const {
    return device;
  }

} // namespace G13