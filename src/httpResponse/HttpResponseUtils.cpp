/*#include <iostream>
#include <dirent.h>
#include <cstring>*/
#include "Common.hpp"

std::string generateDirectoryListing(std::string path, std::string requestPath)
{
    std::string distlist_page;
    DIR *dir = opendir(path.c_str());
    if(dir == NULL)
    {
        std::cerr << "Error: " << path << std::endl;
        //distlist_page = "";
        return "";
    }
    struct dirent *entry;
    distlist_page.append("<html>\n");
    distlist_page.append("<head>\n");
    distlist_page.append("<title>Directory Listing for " + path + "</title>\n");
    distlist_page.append("</head>\n");
    distlist_page.append("<body>\n");
    distlist_page.append("<table>\n");
    while((entry = readdir(dir)) != NULL)
    {
        if(strcmp(entry->d_name, ".") == 0 || strcmp((entry->d_name), "..") == 0)
            continue;
        distlist_page.append("<tr><td><a href=\"");
        distlist_page.append(requestPath);
        distlist_page.append("/");
        distlist_page.append(entry->d_name);
        distlist_page.append("\">");
        distlist_page.append(entry->d_name);
        distlist_page.append("</a></td></tr>\n");
    }
    distlist_page.append("</table>\n");
    distlist_page.append("</body>\n");
    distlist_page.append("</html>");
    closedir(dir);
    /*std::cout << "=== EXACT HTML ===" << std::endl;
    std::cout << "Size: " << distlist_page.size() << " bytes" << std::endl;
for (size_t i = 0; i < distlist_page.size(); i++) {
    std::cout << distlist_page[i];
    if (distlist_page[i] == '\n') std::cout << " [NEWLINE]";
}
std::cout << "==================" << std::endl;*/
    return distlist_page;
   // std::cout << distlist_page << std::endl;
}       