// #define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
// #include <catch2/catch.hpp>
//#include <catch2/catch.hpp>
//
//
// TEST_CASE( "Factorials are computed", "[G13Key]" ) {
//    SECTION("Key symbols can be found after init") {
//        //auto profile = G13::G13_Profile();
//        REQUIRE(true);
//    }
//}

#include "g13.hpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "g13_manager.hpp"
#include "g13_profile.hpp"

class MockManager : public G13_Manager {
   public:
    // MOCK_METHOD0(Run,void());
};

class MockDevice : public G13::G13_Device {
   public:
    MockDevice(G13_Manager& manager) : G13_Device(manager, nullptr, 0) {}
};

class MockProfile : public G13_Profile {
   public:
    MockProfile(G13::G13_Device& device) : G13_Profile(device, std::string("mock")) {}
};

// class MockProfile : public G13::G13_Profile {
// public:
//    G13_Profile(G13_Device& keypad, const std::string& name_arg)
//    : m_keypad(keypad), m_name(name_arg) {
//        _init_keys();
//
//    }    MOCK_METHOD0(_init_keys, void());
//};

TEST(G13Key, g13_key_maps_to_value) {
    MockManager manager;
    
    EXPECT_EQ(manager.find_g13_key_value("G1"), 0);
    EXPECT_EQ(manager.find_g13_key_value("G22"), 21);
    // G13::G13_Device device = MockDevice(manager);
    // G13::G13_Profile profile = MockProfile(device);

    // auto profile = G13::G13_Profile("",std::string(""),"");
    // auto key = profile.FindKey("KEY_0");
    // EXPECT_EQ(key->index(), 10);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);

    int returnValue;

    // Do whatever setup here you will need for your tests here
    //
    //

    returnValue = RUN_ALL_TESTS();

    // Do Your teardown here if required
    //
    //

    return returnValue;
}