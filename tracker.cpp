#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <filesystem>
#include <set>

namespace fs = std::filesystem;

// User file name generator
std::string getUserFile(const std::string& user) {
    return "rawdata_" + user + ".txt";
}

// Write entry to user's file (for paper or idea)
void writeEntry(const std::string& user, const std::string& type, const std::string& title, const std::string& summary) {
    std::ofstream file(getUserFile(user), std::ios::app);
    file << "---\n";
    file << "User: " << user << "\n";
    file << "Type: " << type << "\n";
    file << "Title: " << title << "\n";
    file << "Summary: " << summary << "\n";
}

// Delete entry (by index & type) from user's file, move to trash.txt
void deleteEntry(const std::string& user, int index, const std::string& type = "") {
    std::string path = getUserFile(user);
    std::ifstream in(path);
    if (!in.is_open()) {
        std::cout << "File could not be opened.\n";
        return;
    }
    std::ofstream out("temp.txt");
    std::ofstream trash("trash.txt", std::ios::app);

    std::string line, entryType;
    int count = -1;
    bool inBlock = false;
    bool deleted = false;
    std::stringstream entryBuffer;

    while (std::getline(in, line)) {
        if (line == "---") {
            if (inBlock) {
                if (((type.empty() && count == index) || (type == entryType && count == index))) {
                    trash << entryBuffer.str();
                    deleted = true;
                } else {
                    out << entryBuffer.str();
                }
                entryBuffer.str("");
                entryBuffer.clear();
            }
            inBlock = true;
            count++;
            entryType.clear();
        }
        if (inBlock) {
            entryBuffer << line << "\n";
            if (line.rfind("Type: ", 0) == 0) {
                entryType = line.substr(6);
            }
        }
    }
    // Last block
    if (!entryBuffer.str().empty()) {
        if (((type.empty() && count == index) || (type == entryType && count == index))) {
            trash << entryBuffer.str();
            deleted = true;
        } else {
            out << entryBuffer.str();
        }
    }

    in.close();
    out.close();
    trash.close();
    fs::remove(path);
    fs::rename("temp.txt", path);

    if (deleted)
        std::cout << "Deleted.\n";
    else
        std::cout << "Invalid index, nothing deleted.\n";
}

void listTrashTitles() {
    std::ifstream trash("trash.txt");
    if (!trash.is_open()) {
        std::cout << "trash.txt could not be opened or does not exist.\n";
        return;
    }
    std::string line, title, type;
    bool inBlock = false;
    bool foundAny = false;
    while (std::getline(trash, line)) {
        if (line == "---") {
            title.clear();
            type.clear();
            inBlock = true;
        } else if (inBlock && line.rfind("Title: ", 0) == 0) {
            title = line.substr(7);
        } else if (inBlock && line.rfind("Type: ", 0) == 0) {
            type = line.substr(6);
        } else if (!title.empty() && !type.empty()) {
            std::cout << "- [" << type << "] " << title << "\n";
            foundAny = true;
            inBlock = false;
        }
    }
    if (!foundAny) std::cout << "Trash is empty.\n";
}

// Restore entry from trash.txt by title
void restoreFromTrash(const std::string& title) {
    std::ifstream trash("trash.txt");
    if (!trash.is_open()) {
        std::cerr << "trash.txt could not be opened.\n";
        return;
    }

    std::stringstream kept, currentBlock;
    std::string line, username;
    bool inBlock = false;
    bool matched = false;
    bool currentBlockMatches = false;

    while (std::getline(trash, line)) {
        if (line == "---") {
            if (inBlock) {
                if (currentBlockMatches && !username.empty()) {
                    std::ofstream userFile(getUserFile(username), std::ios::app);
                    if (userFile.is_open()) {
                        std::cout << "Match found, restoring to " << getUserFile(username) << ".\n";
                        userFile << currentBlock.str();
                        userFile.close();
                        matched = true;
                    }
                } else {
                    kept << currentBlock.str();
                }
                currentBlock.str("");
                currentBlock.clear();
                username.clear();
                currentBlockMatches = false;
            }
            inBlock = true;
            currentBlock << line << "\n";
        } else if (inBlock) {
            currentBlock << line << "\n";
            if (line.rfind("Title: ", 0) == 0) {
                std::string currentTitle = line.substr(7);
                if (currentTitle == title) {
                    currentBlockMatches = true;
                }
            } else if (line.rfind("User: ", 0) == 0) {
                username = line.substr(6);
            }
        }
    }

    // Last block
    if (!currentBlock.str().empty()) {
        if (currentBlockMatches && !username.empty()) {
            std::ofstream userFile(getUserFile(username), std::ios::app);
            if (userFile.is_open()) {
                std::cout << "Match found (last block), restoring to " << getUserFile(username) << ".\n";
                userFile << currentBlock.str();
                matched = true;
            }
        } else {
            kept << currentBlock.str();
        }
    }

    trash.close();
    std::ofstream out("trash.txt");
    out << kept.str();
    out.close();

    if (matched)
        std::cout << "Restore completed.\n";
    else
        std::cout << "No matching title found. Restore failed.\n";
}

// Merge all user rawdata files into data.txt
void mergeRawData() {
    std::ofstream out("data.txt");
    for (const auto& entry : fs::directory_iterator(".")) {
        std::string fname = entry.path().filename().string();
        if (fname.rfind("rawdata_", 0) == 0 && fname.size() > 8) {
            std::ifstream in(entry.path());
            std::string line;
            while (std::getline(in, line)) {
                out << line << "\n";
            }
        }
    }
}

// List all titles for a user (paper & idea)
void listUserTitles(const std::string& user) {
    std::ifstream in(getUserFile(user));
    std::string line, title, type;
    bool inBlock = false;
    while (std::getline(in, line)) {
        if (line == "---") {
            title.clear();
            type.clear();
            inBlock = true;
        } else if (inBlock && line.rfind("Type: ", 0) == 0) {
            type = line.substr(6);
        } else if (inBlock && line.rfind("Title: ", 0) == 0) {
            title = line.substr(7);
            if (!type.empty()) {
                std::cout << "- [" << type << "] " << title << "\n";
            } else {
                std::cout << "- " << title << "\n";
            }
            inBlock = false;
        }
    }
}

void listAllSummaries() {
    std::ifstream in("data.txt");
    std::string line, title, summary, type;
    bool inSummary = false;
    while (std::getline(in, line)) {
        if (line == "---") {
            if (!title.empty() && !summary.empty() && !type.empty()) {
                std::cout << "- [" << type << "] " << title << "\n" << summary << "\n";
            }
            title.clear();
            summary.clear();
            type.clear();
            inSummary = false;
        } else if (line.rfind("Title: ", 0) == 0) {
            title = line.substr(7);
        } else if (line.rfind("Type: ", 0) == 0) {
            type = line.substr(6);
        } else if (line.rfind("Summary: ", 0) == 0) {
            summary = line.substr(9) + "\n";
            inSummary = true;
        } else if (inSummary) {
            summary += line + "\n";
        }
    }
    // Last block
    if (!title.empty() && !summary.empty() && !type.empty()) {
        std::cout << "- [" << type << "] " << title << "\n" << summary << "\n";
    }
}

void showSummaryByTitle(const std::string& searchTitle, const std::string& type = "") {
    std::ifstream in("data.txt");
    std::string line, title, summary, entryType;
    bool inSummary = false, found = false;
    while (std::getline(in, line)) {
        if (line == "---") {
            if (title == searchTitle && !summary.empty() && (type.empty() || entryType == type)) {
                std::cout << "Summary for [" << entryType << "] \"" << title << "\":\n" << summary << "\n";
                found = true;
                break;
            }
            title.clear();
            summary.clear();
            entryType.clear();
            inSummary = false;
        } else if (line.rfind("Title: ", 0) == 0) {
            title = line.substr(7);
        } else if (line.rfind("Type: ", 0) == 0) {
            entryType = line.substr(6);
        } else if (line.rfind("Summary: ", 0) == 0) {
            summary = line.substr(9) + "\n";
            inSummary = true;
        } else if (inSummary) {
            summary += line + "\n";
        }
    }
    // Last block
    if (!found && title == searchTitle && !summary.empty() && (type.empty() || entryType == type)) {
        std::cout << "Summary for [" << entryType << "] \"" << title << "\":\n" << summary << "\n";
        found = true;
    }
    if (!found) std::cout << "Entry not found.\n";
}

// List all titles in data.txt (paper & idea)
void listAllTitles() {
    std::ifstream in("data.txt");
    std::string line, title, type;
    bool inBlock = false;
    while (std::getline(in, line)) {
        if (line == "---") {
            title.clear();
            type.clear();
            inBlock = true;
        } else if (inBlock && line.rfind("Type: ", 0) == 0) {
            type = line.substr(6);
        } else if (inBlock && line.rfind("Title: ", 0) == 0) {
            title = line.substr(7);
            if (!type.empty()) {
                std::cout << "- [" << type << "] " << title << "\n";
            } else {
                std::cout << "- " << title << "\n";
            }
            inBlock = false;
        }
    }
}

// --- VIDEO SYSTEM ---

std::string getVideosFile() { return "videos.txt"; }

// Add a new video
void addVideo(const std::string& title, const std::string& url) {
    std::ofstream file(getVideosFile(), std::ios::app);
    file << "---\n";
    file << "Title: " << title << "\n";
    file << "URL: " << url << "\n";
}

// Get total videos count
int getTotalVideos() {
    std::ifstream file(getVideosFile());
    std::string line;
    int count = 0;
    while (std::getline(file, line)) {
        if (line == "---") count++;
    }
    return count;
}

// Get watched videos for user
std::set<int> getUserWatchedVideos(const std::string& user) {
    std::ifstream file(getUserFile(user));
    std::string line, type;
    std::set<int> watched;
    bool inBlock = false;
    while (std::getline(file, line)) {
        if (line == "---") {
            type.clear();
            inBlock = true;
        } else if (inBlock && line.rfind("Type: ", 0) == 0) {
            type = line.substr(6);
        } else if (inBlock && type == "videos" && line.rfind("Videos: ", 0) == 0) {
            std::string nums = line.substr(8);
            std::stringstream ss(nums);
            std::string idx;
            while (std::getline(ss, idx, ',')) {
                try {
                    int v = std::stoi(idx);
                    watched.insert(v);
                } catch (...) {}
            }
            inBlock = false;
        }
    }
    return watched;
}

// Mark video as watched for a user (by index)
void watchVideo(const std::string& user, int index) {
    // Read all lines, update videos block or add if not exists
    std::ifstream file(getUserFile(user));
    std::vector<std::string> lines;
    std::string line;
    bool updated = false;
    bool inBlock = false;
    std::string type;
    while (std::getline(file, line)) {
        if (line == "---") {
            lines.push_back(line);
            type.clear();
            inBlock = true;
        } else if (inBlock && line.rfind("Type: ", 0) == 0) {
            type = line.substr(6);
            lines.push_back(line);
            if (type == "videos") {
                std::getline(file, line); // Videos: ...
                if (line.rfind("Videos: ", 0) == 0) {
                    std::string nums = line.substr(8);
                    std::set<int> watched;
                    std::stringstream ss(nums);
                    std::string idx;
                    while (std::getline(ss, idx, ',')) {
                        try {
                            watched.insert(std::stoi(idx));
                        } catch (...) {}
                    }
                    watched.insert(index);
                    std::string newLine = "Videos: ";
                    int i = 0;
                    for (int v : watched) {
                        if (i > 0) newLine += ",";
                        newLine += std::to_string(v);
                        i++;
                    }
                    lines.push_back(newLine);
                    updated = true;
                } else {
                    lines.push_back("Videos: " + std::to_string(index));
                    updated = true;
                }
                inBlock = false;
            }
        } else {
            lines.push_back(line);
        }
    }
    file.close();

    if (!updated) {
        lines.push_back("---");
        lines.push_back("Type: videos");
        lines.push_back("Videos: " + std::to_string(index));
    }

    std::ofstream out(getUserFile(user));
    for (auto& l : lines) out << l << "\n";
    std::cout << "Marked as watched.\n";
}

// List all videos with their indices
void listAllVideos() {
    std::ifstream file(getVideosFile());
    std::string line, title, url;
    int idx = -1;
    bool inBlock = false;
    while (std::getline(file, line)) {
        if (line == "---") {
            if (!title.empty() && !url.empty()) {
                std::cout << "[" << idx << "] " << title << " (" << url << ")\n";
            }
            idx++;
            title.clear();
            url.clear();
            inBlock = true;
        } else if (inBlock && line.rfind("Title: ", 0) == 0) {
            title = line;
        } else if (inBlock && line.rfind("URL: ", 0) == 0) {
            url = line;
        }
    }
    // Son blok için
    if (!title.empty() && !url.empty()) {
        std::cout << "[" << idx << "] " << title << " (" << url << ")\n";
    }
}

// Progress bar örneği ile birlikte:
void showProgress(const std::vector<std::string>& users, int totalPapersPerUser) {
    const int barWidth = 15;
    size_t maxUserLen = 0;
    for (const auto& user : users) maxUserLen = std::max(maxUserLen, user.size());
    int totalVideos = getTotalVideos();

    for (const auto& user : users) {
        std::ifstream file(getUserFile(user));
        int paperCount = 0, ideaCount = 0;
        std::string line, type;
        bool inBlock = false;
        while (std::getline(file, line)) {
            if (line == "---") { type.clear(); inBlock = true; }
            else if (inBlock && line.rfind("Type: ", 0) == 0) {
                type = line.substr(6);
                if (type == "paper") paperCount++;
                else if (type == "idea") ideaCount++;
                inBlock = false;
            }
        }
        auto watchedVideos = getUserWatchedVideos(user);
        int filledPaper = (totalPapersPerUser > 0) ? (paperCount * barWidth) / totalPapersPerUser : 0;
        std::cout << std::left << std::setw(maxUserLen + 2) << user << " Papers: [";
        for (int i = 0; i < barWidth; i++) std::cout << (i < filledPaper ? "#" : " ");
        std::cout << "] " << paperCount << "/" << totalPapersPerUser
                  << " | Ideas: " << ideaCount
                  << " | Videos: " << watchedVideos.size() << "/" << totalVideos << "\n";
    }
}

int main() {
    std::vector<std::string> users = {"yavuz", "birdem", "emre"};
    int totalPapersPerUser = 15;

    std::string input;
    std::cout << R"(
        ,----,                                                                            
      ,/   .`|                                                ,--.                        
    ,`   .'  : ,-.----.       ,---,          ,----..      ,--/  /|     ,---,. ,-.----.    
  ;    ;     / \    /  \     '  .' \        /   /   \  ,---,': / '   ,'  .' | \    /  \   
.'___,/    ,'  ;   :    \   /  ;    '.     |   :     : :   : '/ /  ,---.'   | ;   :    \  
|    :     |   |   | .\ :  :  :       \    .   |  ;. / |   '   ,   |   |   .' |   | .\ :  
;    |.';  ;   .   : |: |  :  |   /\   \   .   ; /--`  '   |  /    :   :  |-, .   : |: |  
`----'  |  |   |   |  \ :  |  :  ' ;.   :  ;   | ;     |   ;  ;    :   |  ;/| |   |  \ :  
    '   :  ;   |   : .  /  |  |  ;/  \   \ |   : |     :   '   \   |   :   .' |   : .  /  
    |   |  '   ;   | |  \  '  :  | \  \ ,' .   | '___  |   |    '  |   |  |-, ;   | |  \  
    '   :  |   |   | ;\  \ |  |  '  '--'   '   ; : .'| '   : |.  \ '   :  ;/| |   | ;\  \ 
    ;   |.'    :   ' | \.' |  :  :         '   | '/  : |   | '_\.' |   |    \ :   ' | \.' 
    '---'      :   : :-'   |  | ,'         |   :    /  '   : |     |   :   .' :   : :-'   
               |   |.'     `--''            \   \ .'   ;   |,'     |   | ,'   |   |.'     
               `---'                         `---`     '---'       `----'     `---'       
                                                                         
)" << std::endl;
    std::cout << "Commands: add_paper <user>, add_idea <user>, delete_paper <user> <index>, delete_idea <user> <index>, restore <title>, progress\n"
                 "          list_all, list_user <user>, summary_of <title> [paper|idea], list_titles, list_trash, add_video, list_videos, watch_video <user> <index>, exit\n\n";
showProgress(users, totalPapersPerUser);

    while (true) {
        std::cout << "> ";
        std::getline(std::cin, input);
        std::istringstream iss(input);
        std::string command;
        iss >> command;

        if (command == "exit") break;

        else if (command == "add_paper" || command == "add_idea") {
            std::string user, type = (command == "add_paper" ? "paper" : "idea");
            iss >> user;
            std::cin.ignore();

            std::string title, summary, line;
            std::cout << "Entry title: ";
            std::getline(std::cin, title);

            std::cout << "Enter summary (type 'end' to finish):" << std::endl;
            summary = "";
            while (true) {
                std::getline(std::cin, line);
                if (line == "end") break;
                summary += line + "\n";
            }

            writeEntry(user, type, title, summary);
            mergeRawData();
        }

        else if (command == "delete_paper" || command == "delete_idea") {
            std::string user, type = (command == "delete_paper" ? "paper" : "idea");
            int index;
            iss >> user >> index;
            deleteEntry(user, index, type);
            mergeRawData();
        }

        else if (command == "restore") {
            std::string title;
            std::getline(iss >> std::ws, title);
            restoreFromTrash(title);
            mergeRawData();
        }

        else if (command == "progress") {
            showProgress(users, totalPapersPerUser);
        }

        else if (command == "list_all") {
            listAllSummaries();
        }

        else if (command == "list_user") {
            std::string user;
            iss >> user;
            listUserTitles(user);
        }

        else if (command == "summary_of") {
            std::string title, type;
            std::getline(iss >> std::ws, title, ' ');
            iss >> type; // optional
            showSummaryByTitle(title, type);
        }

        else if (command == "list_titles") {
            listAllTitles();
        }
        else if (command == "list_trash") {
            listTrashTitles();
        }

	else if (command == "add_video") {
	    std::cout << "\nWrite title in quotes such as: \"Title of the video\" \n";
	    std::string title, url;
	    std::cout << "Enter video title: ";
	    std::getline(std::cin, title);
	    std::cout<<"Enter the URL: ";
	    std::getline(std::cin, url);

	    addVideo(title, url);
	}

        else if (command == "list_videos") {
            listAllVideos();
        }

        else if (command == "watch_video") {
            std::string user;
            int index;
            iss >> user >> index;
            watchVideo(user, index);
        }

        else {
            std::cout << "Invalid command.\n";
        }
    }

    return 0;
}
