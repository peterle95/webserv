bool ConfigParser::parse(const std::string& configPath) {
    std::cout << "Debug: Starting config parse for: " << configPath << std::endl;
    
    if (!fileExists(configPath)) {
        std::cerr << "Config file not found: " << configPath << std::endl;
        return false;
    }
    
    try {
        // Add your existing parse logic here
        std::cout << "Debug: Config parse complete" << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception during config parse: " << e.what() << std::endl;
        return false;
    }
}

bool ConfigParser::fileExists(const std::string& path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}