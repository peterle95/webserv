#include <iostream>
#include <dirent.h>
#include <cstring>

void generateDirectoryListing(std::string path)
{
    std::string distlist_page;
    DIR *dir = opendir(path.c_str());
    if(dir == nullptr)
    {
        std::cerr << "Error: Unable to open directory " << path << std::endl;
    }
    struct dirent *entry;
    distlist_page += ("<html>\n");
    distlist_page += ("<head>\n");
    distlist_page += ("<title>Directory Listing for " + path + "</title>\n");
    distlist_page += ("</head>\n");
    distlist_page += ("<body>\n");
    distlist_page += ("<table>\n");
    while((entry = readdir(dir)) != nullptr)
    { 
     if(strcmp(entry->d_name, ".") == 0 || strcmp((entry->d_name), "..") == 0)
        continue;  
    distlist_page += ("<tr><td><a href = ");
    distlist_page +=  entry->d_name ;
    distlist_page += "</a>";
    distlist_page +=  entry->d_name ;
    distlist_page += "</td></tr>\n";
    }
    distlist_page += ("</table>\n");
    distlist_page += ("</body>\n");
    distlist_page += ("</html>");
    closedir(dir);
    std::cout << distlist_page << std::endl;
}