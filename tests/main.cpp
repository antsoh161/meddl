#include <catch2/catch_session.hpp>

// If you need custom setup/teardown
int main(int argc, char* argv[]) {
    // Setup code here if needed
    
    int result = Catch::Session().run(argc, argv);
    
    // Cleanup code here if needed
    
    return result;
}
