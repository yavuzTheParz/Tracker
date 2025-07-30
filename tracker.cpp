#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;

// User file name generator
std::string getUserFile(const std::string& user) {
    return "rawdata_" + user + ".txt";
}

// Write entry to user's file
void writeEntry(const std::string& user, const std::string& title, const std::string& summary) {
    std::ofstream file(getUserFile(user), std::ios::app);
    file << "---\n";
    file << "User: " << user << "\n";
    file << "Title: " << title << "\n";
    file << "Summary: " << summary << "\n";
}

void deleteEntry(const std::string& user, int index) {
    std::string path = getUserFile(user);
    std::ifstream in(path);
    if (!in.is_open()) {
        std::cout << "Dosya açılamadı.\n";
        return;
    }
    std::ofstream out("temp.txt");
    std::ofstream trash("trash.txt", std::ios::app);

    std::string line;
    int count = -1;
    bool inBlock = false;
    bool deleted = false;
    std::stringstream entryBuffer;

    while (std::getline(in, line)) {
        if (line == "---") {
            if (inBlock) {
                if (count == index) {
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
        }
        if (inBlock) {
            entryBuffer << line << "\n";
        }
    }
    // Son blok
    if (!entryBuffer.str().empty()) {
        if (count == index) {
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
        std::cout << "Silindi.\n";
    else
        std::cout << "Geçersiz index, silme işlemi yapılmadı.\n";
}

void listTrashTitles() {
    std::ifstream trash("trash.txt");
    if (!trash.is_open()) {
        std::cout << "trash.txt açılamadı veya yok.\n";
        return;
    }
    std::string line, title;
    bool inBlock = false;
    bool foundAny = false;
    while (std::getline(trash, line)) {
        if (line == "---") {
            title.clear();
            inBlock = true;
        } else if (inBlock && line.rfind("Title: ", 0) == 0) {
            title = line.substr(7);
            std::cout << "- " << title << "\n";
            foundAny = true;
            inBlock = false; // Sadece ilk başlık satırını alır, aynı blokta tekrar aramaz
        }
    }
    if (!foundAny) std::cout << "Trash boş.\n";
}

// Restore entry from trash.txt by title
void restoreFromTrash(const std::string& title) {
    std::ifstream trash("trash.txt");
    if (!trash.is_open()) {
        std::cerr << "trash.txt açılamadı.\n";
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
                        std::cout << "Eşleşme bulundu, " << getUserFile(username) << " dosyasına restore ediliyor.\n";
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

    // Son blok için
    if (!currentBlock.str().empty()) {
        if (currentBlockMatches && !username.empty()) {
            std::ofstream userFile(getUserFile(username), std::ios::app);
            if (userFile.is_open()) {
                std::cout << "Eşleşme bulundu (son blok), " << getUserFile(username) << " dosyasına restore ediliyor.\n";
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
        std::cout << "Restore tamamlandı.\n";
    else
        std::cout << "Eşleşen başlık bulunamadı. Restore başarısız.\n";
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

// Show progress for each user
void showProgress(const std::vector<std::string>& users, int totalPapersPerUser) {
    for (const auto& user : users) {
        std::ifstream file(getUserFile(user));
        int count = 0;
        std::string line;
        while (std::getline(file, line)) {
            if (line == "---") count++;
        }
        std::cout << user << ": " << count << "/" << totalPapersPerUser << "\n";
    }
}

// List all titles for a user
void listUserTitles(const std::string& user) {
    std::ifstream in(getUserFile(user));
    std::string line;
    while (std::getline(in, line)) {
        if (line.find("Title: ") == 0) {
            std::cout << "- " << line.substr(7) << "\n";
        }
    }
}

void listAllSummaries() {
    std::ifstream in("data.txt");
    std::string line;
    std::string title, summary;
    bool inSummary = false;

    while (std::getline(in, line)) {
        if (line == "---") {
            if (!title.empty() && !summary.empty()) {
                std::cout << "- " << title << "\n" << summary << "\n";
            }
            title.clear();
            summary.clear();
            inSummary = false;
        } else if (line.rfind("Title: ", 0) == 0) {
            title = line.substr(7);
        } else if (line.rfind("Summary: ", 0) == 0) {
            summary = line.substr(9) + "\n";
            inSummary = true;
        } else if (inSummary) {
            summary += line + "\n";
        }
    }
    // Son blok eklemesi
    if (!title.empty() && !summary.empty()) {
        std::cout << "- " << title << "\n" << summary << "\n";
    }
}

void showSummaryByTitle(const std::string& searchTitle) {
    std::ifstream in("data.txt");
    std::string line, title, summary;
    bool inSummary = false, found = false;

    while (std::getline(in, line)) {
        if (line == "---") {
            if (title == searchTitle && !summary.empty()) {
                std::cout << "Summary for \"" << title << "\":\n" << summary << "\n";
                found = true;
                break;
            }
            title.clear();
            summary.clear();
            inSummary = false;
        } else if (line.rfind("Title: ", 0) == 0) {
            title = line.substr(7);
        } else if (line.rfind("Summary: ", 0) == 0) {
            summary = line.substr(9) + "\n";
            inSummary = true;
        } else if (inSummary) {
            summary += line + "\n";
        }
    }
    // Son blok kontrolü
    if (!found && title == searchTitle && !summary.empty()) {
        std::cout << "Summary for \"" << title << "\":\n" << summary << "\n";
        found = true;
    }
    if (!found) std::cout << "Makale bulunamadı.\n";
}

// List all titles in data.txt
void listAllTitles() {
    std::ifstream in("data.txt");
    std::string line;
    while (std::getline(in, line)) {
        if (line.find("Title: ") == 0) {
            std::cout << "- " << line.substr(7) << "\n";
        }
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
    std::cout << "Komutlar: add <user>, delete <user> <index>, restore <title>, progress\n"
                 "          list_all, list_user <user>, summary_of <title>, list_titles, list_trash, exit\n\n";
    while (true) {
        std::cout << "> ";
        std::getline(std::cin, input);
        std::istringstream iss(input);
        std::string command;
        iss >> command;

        if (command == "exit") break;

        else if (command == "add") {
            std::string user;
            iss >> user;
            std::cin.ignore();

            std::string title, summary, line;
            std::cout << "Makale başlığı: ";
            std::getline(std::cin, title);

            std::cout << "Özet girin (bitirmek için 'end' yazın):" << std::endl;
            summary = "";
            while (true) {
                std::getline(std::cin, line);
                if (line == "end") break;
                summary += line + "\n";
            }

            writeEntry(user, title, summary);
            mergeRawData();
        }

        else if (command == "delete") {
            std::string user;
            int index;
            iss >> user >> index;
            deleteEntry(user, index);
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
            std::string title;
            std::getline(iss >> std::ws, title);
            showSummaryByTitle(title);
        }

        else if (command == "list_titles") {
            listAllTitles();
        }
        else if (command == "list_trash") {
	    listTrashTitles();
	}

        else {
            std::cout << "Geçersiz komut.\n";
        }
    }

    return 0;
}
