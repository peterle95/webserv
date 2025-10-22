
#ifndef SERVERCONFIG_HPP
#define SERVERCONFIG_HPP

#include <set>
//  Location configuration structure to hold per-location settings
struct LocationConfig
{
    std::string path;
    std::string root;
    std::string index;
    std::set<std::string> allowedMethods;
    bool autoindex;
    bool cgiPass;
    std::string cgiExtension;
    std::map<int, std::string> redirect;

    LocationConfig()
        : path(""), root(""), index(), allowedMethods(), autoindex(false), cgiPass(false), cgiExtension(""), redirect() {}
};

class ServerConfig
{
private:
    std::string _configFile;
    std::vector<int> _ports;         // Keep for backward compatibility
    std::vector<std::string> _hosts; // Keep for backward compatibility
    // std::vector<std::pair<std::string, int> > _listenAddresses; // host:port pairs (C++98 syntax)
    std::string _root;
    std::string _index;
    std::string _serverName;
    std::map<int, std::string> _errorPage;

    size_t _clientMaxBodySize;
    // std::string _host;     // Keep for backward compatibility
    std::string _location; // to implement later

    // add more directives
    std::set<std::string> _allowedMethods; // to implement later, set because order does not matter

    std::map<std::string, LocationConfig> _locations; // map of location path to LocationConfig

    // Per-directive parsers
    void parseListen(const std::string &val, size_t lineNo);
    void parseRoot(const std::string &val, size_t lineNo, std::string *root);
    void parseIndex(const std::string &val, size_t lineNo, std::string *index);
    void parseServerName(const std::string &val, size_t lineNo);
    void parseClientMaxBodySize(const std::string &val, size_t lineNo);
    void parseAllowedMethods(const std::string &val, size_t lineNo, std::set<std::string> *allowedMethods = NULL);
    void parseErrorPage(const std::string &val, size_t lineNo);

    // Helpers to keep parseLines small
    std::string preprocessLine(const std::string &raw);
    bool isBlockMarker(const std::string &line) const; // '{', '}', or contains '{'
    void requireSemicolon(const std::string &line, size_t lineNo) const;
    std::string stripTrailingSemicolon(const std::string &line) const;
    bool splitKeyVal(const std::string &line, std::string &key, std::string &val) const;
    void handleDirective(const std::string &key, const std::string &val, size_t lineNo);

public:
    ServerConfig(const std::string &root = "html", const std::string &index = "index.html", size_t clientMaxBodySize = 1024 * 1024); // default 1 MiB
    // Construct directly from lines (e.g., read elsewhere)
    ServerConfig(const std::string &root, const std::string &index, size_t clientMaxBodySize, const std::vector<std::string> &lines);
    const std::vector<std::string> &getLines() const;
    ~ServerConfig();

    // Parse minimal NGINX-like config supporting: listen, root, index
    void parse(const std::vector<std::string> &lines);

    // Accessors
    int getListenPort() const;
    const std::vector<int> &getListenPorts() const;         // Get all ports
    const std::vector<std::string> &getListenHosts() const; // Get all hosts
    // const std::vector<std::pair<std::string, int>> &getListenAddresses() const; // Get host:port pairs
    const std::string &getRoot() const;
    const std::string &getIndex() const;

    // ServerName addition -Shruti
    const std::string &getServerName() const;
    const std::map<std::string, LocationConfig> &getLocations() const;
    size_t getClientMaxBodySize() const;
    const std::string &getErrorPage(int status_code) const;

    // TODO: implement error handling
    // TODO: implement parsing more directives (directives are the lines in the config file)
    // Location handling
    void handleLocationDirective(LocationConfig *currentLocation,
                                 const std::string &key,
                                 const std::string &val,
                                 size_t lineNumber);
    // void applyRoot(LocationConfig *loc, const std::string &val);
    void applyIndex(LocationConfig *loc, const std::string &val, size_t lineNo);
    void applyAutoindex(LocationConfig *loc, const std::string &val, size_t lineNo);
    void applyCgiPass(LocationConfig *loc, const std::string &val, size_t lineNo);
    void applyCgiExtension(LocationConfig *loc, const std::string &val, size_t lineNo);
    void applyRedirect(LocationConfig *loc, const std::string &val, size_t lineNo);
};

#endif
