
namespace ThirdParty {
    template<typename T = void> void say_hello () {
		fmt::print ("Hello, world! from {}\n", __FILE__);
	}
}