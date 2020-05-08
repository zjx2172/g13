all: g13d pbm2lpbm

FLAGS=$(CXXFLAGS) -std=c++17
LIBS=-lusb-1.0 -llog4cpp -levdev

GIT-VERSION.h:
    $(shell ./GIT-VERSION-GEN)

g13.o: g13.hpp helper.hpp g13.cpp
	g++ $(FLAGS) -c g13.cpp

g13_main.o: GIT-VERSION.h g13.hpp helper.hpp g13_main.cpp
	g++ $(FLAGS) -c g13_main.cpp

g13_log.o: g13.hpp helper.hpp g13_log.cpp
	g++ $(FLAGS) -c g13_log.cpp

g13_fonts.o: g13.hpp helper.hpp g13_fonts.cpp
	g++ $(FLAGS) -c g13_fonts.cpp

g13_lcd.o: g13.hpp helper.hpp g13_lcd.cpp
	g++ $(FLAGS) -c g13_lcd.cpp

g13_stick.o: g13.hpp helper.hpp g13_stick.cpp
	g++ $(FLAGS) -c g13_stick.cpp
	
g13_keys.o: g13.hpp helper.hpp g13_keys.cpp
	g++ $(FLAGS) -c g13_keys.cpp

g13_device.o: g13.hpp helper.hpp g13_device.cpp
	g++ $(FLAGS) -c g13_device.cpp

g13_action.o: g13.hpp helper.hpp g13_action.cpp
	g++ $(FLAGS) -c g13_action.cpp

helper.o: helper.hpp helper.cpp
	g++ $(FLAGS) -c helper.cpp
	
	
g13d: g13_main.o g13.o g13_log.o g13_fonts.o g13_lcd.o g13_stick.o g13_keys.o g13_device.o g13_action.o helper.o
	g++ -o g13d $(FLAGS) \
		g13_main.o g13.o g13_log.o g13_fonts.o g13_lcd.o g13_stick.o g13_keys.o g13_device.o g13_action.o helper.o \
	    $(LIBS)

pbm2lpbm: pbm2lpbm.cpp
	g++ -o pbm2lpbm pbm2lpbm.cpp

package:
	rm -Rf g13-userspace
	mkdir g13-userspace
	cp g13.cpp g13.hpp logo.h Makefile pbm2lpbm.cpp g13-userspace
	tar cjf g13-userspace.tbz2 g13-userspace
	rm -Rf g13-userspace

clean:
	rm -f g13d pbm2lpbm *.o GIT-VERSION-FILE GIT-VERSION.h
