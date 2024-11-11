#define BOOST_TEST_MODULE HelloWorldTests
#include <boost/test/included/unit_test.hpp>
#include <fmt/core.h>
#include <version.h>

BOOST_AUTO_TEST_CASE(hello_world_test) {
    std::string output = fmt::format("{}.{}.{}",
                                     ANARIViewerProject_VERSION_MAJOR,
                                     ANARIViewerProject_VERSION_MINOR,
                                     ANARIViewerProject_VERSION_PATCH);
    BOOST_TEST(output == ANARIViewerProject_VERSION_STRING);
}
