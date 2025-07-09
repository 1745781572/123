#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cctype>
#include <algorithm>
#include <ncurses.h>
#include <cstring>
#include <unistd.h>

class TextEditor {
private:
    std::vector<std::string> buffer; 
    std::string filename;  
    int cursorX, cursorY;  
    bool modified;  
    int screenHeight, screenWidth;  

    enum ColorPair {
        DEFAULT = 1,  
        KEYWORD,
        COMMENT,  
        STRING  
    };

public:
    TextEditor() : cursorX(0), cursorY(0), modified(false) {
        buffer.push_back("");

        initscr(); 
        raw();     
        keypad(stdscr, TRUE);
        noecho();  
        start_color();   

        init_pair(DEFAULT, COLOR_WHITE, COLOR_BLACK); 
        init_pair(KEYWORD, COLOR_CYAN, COLOR_BLACK); 
        init_pair(COMMENT, COLOR_GREEN, COLOR_BLACK);
        init_pair(STRING, COLOR_YELLOW, COLOR_BLACK); 

        getmaxyx(stdscr, screenHeight, screenWidth);
    }

    ~TextEditor() {
        endwin(); 
    }
    void openFile(const std::string& file) {
        filename = file; 
        buffer.clear();  

        std::ifstream infile(file); 
        if (!infile) {  
            buffer.push_back("// New file - " + filename); 
            return;           
        }

        std::string line;
        while (std::getline(infile, line)) { 
            buffer.push_back(line);     
        }

        if (buffer.empty()) buffer.push_back(""); 
        modified = false;     
    }

    void saveFile() {
        if (filename.empty()) {     
            showMessage("Enter filename: ");  
            echo();      
            char fn[256];  
            getstr(fn);       
            noecho();         
            filename = fn;        
        }

        std::ofstream outfile(filename);   
        if (!outfile) {     
            showMessage("Error saving file!"); 
            return;  
        }

        for (const auto& line : buffer) { 
            outfile << line << '\n';  
        }

        modified = false;        
        showMessage("File saved: " + filename);  
    }

    void insertChar(char c) {
        if (cursorY >= buffer.size()) return;  

        if (c == '\n') {          
 
            std::string newLine = buffer[cursorY].substr(cursorX);
            buffer[cursorY] = buffer[cursorY].substr(0, cursorX);  
            buffer.insert(buffer.begin() + cursorY + 1, newLine);
            cursorY++; 
            cursorX = 0;
        }
        else {
            buffer[cursorY].insert(cursorX, 1, c); 
            cursorX++; 
        }
        modified = true;
    }

    void deleteChar() {
        if (cursorY >= buffer.size()) return; 

        if (cursorX > 0) { 
            buffer[cursorY].erase(cursorX - 1, 1); 
            cursorX--; 
        }
        else if (cursorY > 0) { 
            cursorX = buffer[cursorY - 1].size(); 
            buffer[cursorY - 1] += buffer[cursorY]; 
            buffer.erase(buffer.begin() + cursorY);
            cursorY--;
        }
        modified = true; 
    }

    void moveCursor(int dx, int dy) {
        cursorY += dy; 

        if (cursorY < 0) cursorY = 0; 
        if (cursorY >= buffer.size()) cursorY = buffer.size() - 1; 

        cursorX += dx; 
        if (cursorX < 0) cursorX = 0; 
        if (cursorX > buffer[cursorY].size()) cursorX = buffer[cursorY].size(); 
    }

    void find(const std::string& term) {
        for (int y = cursorY; y < buffer.size(); y++) { 
            size_t pos = (y == cursorY) ? buffer[y].find(term, cursorX) : buffer[y].find(term); 
            if (pos != std::string::npos) { 
                cursorY = y;
                cursorX = pos; 
                return;
            }
        }
        showMessage("Pattern not found: " + term); 
    }

    void replace(const std::string& term, const std::string& replacement) {
        if (cursorY >= buffer.size()) return;
        size_t pos = buffer[cursorY].find(term, cursorX);
        if (pos != std::string::npos) {
            buffer[cursorY].replace(pos, term.size(), replacement);
            cursorX = pos + replacement.size();
            modified = true; 
        }
        else {
            showMessage("Pattern not found: " + term);
        }
    }

    void showMessage(const std::string& msg) { 
        int y = screenHeight - 1; 
        move(y, 0);
        clrtoeol(); 
        attron(A_REVERSE); 
        mvprintw(y, 0, "%s", msg.c_str()); 
        attroff(A_REVERSE);
        refresh(); 
        timeout(2000); 
        getch();
        timeout(-1); 
    }

    void highlightSyntax(int lineNum, const std::string& line) { 
        if (line.empty()) return;

        static const std::vector<std::string> keywords = {
            "class", "const", "for", "if", "int", "return",
            "void", "while", "else", "true", "false"
        };

        bool inComment = false;
        bool inString = false;
        std::string word; 

        for (size_t i = 0; i < line.size(); i++) { 
            char c = line[i];

            if (c == '"' && !inComment) {
                if (inString) {
                    attron(COLOR_PAIR(STRING)); 
                    addch(c); 
                    attroff(COLOR_PAIR(STRING)); 
                    inString = false; 
                } else {
                    inString = true;
                    attron(COLOR_PAIR(STRING)); 
                    addch(c);
                }
                continue;
            }

            if (i > 0 && line[i - 1] == '/' && c == '/' && !inString) {
                inComment = true; 
            }

            if (inComment) {
                attron(COLOR_PAIR(COMMENT)); 
                addch(c); 
                attroff(COLOR_PAIR(COMMENT)); 
                continue;
            }

            if (inString) {
                attron(COLOR_PAIR(STRING)); 
                addch(c); 
                attroff(COLOR_PAIR(STRING)); 
                continue;
            }

            if (isalnum(c) || c == '_') {
                word += c; 
            } else {
                if (!word.empty()) { 
                    bool isKeyword = std::find(keywords.begin(), keywords.end(), word) != keywords.end(); 

                    if (isKeyword) {
                        attron(COLOR_PAIR(KEYWORD)); 
                        printw("%s", word.c_str()); 
                        attroff(COLOR_PAIR(KEYWORD));
                    } else {
                        printw("%s", word.c_str()); 
                    }

                    word.clear();
                }
            }
                addch(c);
            }
        }

        if (!word.empty()) {
            bool isKeyword = std::find(keywords.begin(), keywords.end(), word) != keywords.end();

            if (isKeyword) {
                attron(COLOR_PAIR(KEYWORD));
                printw("%s", word.c_str());
                attroff(COLOR_PAIR(KEYWORD));
            }
            else {
                printw("%s", word.c_str());
            }

            word.clear(); 
        }
    }

    void display() {
        clear();


        attron(A_REVERSE);
        std::string title = " C++ Editor - " + filename;
        if (modified) title += " [modified]"; 
        title.resize(screenWidth, ' '); 
        mvprintw(0, 0, "%s", title.c_str()); 
        attroff(A_REVERSE);

        int startLine = std::max(0, cursorY - screenHeight / 2);
        int endLine = std::min((int)buffer.size(), startLine + screenHeight - 2); 

        for (int y = startLine; y < endLine; y++) {
            move(y - startLine + 1, 0); 
            clrtoeol();

            if (y < buffer.size()) {
                highlightSyntax(y, buffer[y]);
            }
        }

        attron(A_REVERSE);
        std::string status = "Line: " + std::to_string(cursorY + 1) +
            " Col: " + std::to_string(cursorX + 1) + 
            " | Ctrl+S: Save | Ctrl+Q: Quit | Ctrl+F: Find"; 
        status.resize(screenWidth, ' ');
        mvprintw(screenHeight - 1, 0, "%s", status.c_str()); 
        attroff(A_REVERSE);

        move(cursorY - startLine + 1, cursorX); 
        refresh(); 

    }

    void run() {
        if (filename.empty()) { 
            showMessage("Enter filename: ");
            echo();
            char fn[256];
            getstr(fn); 
            noecho();
            openFile(fn); 
        }

        display(); 

        // 主循环
        while (true) {
            int ch = getch(); 

            switch (ch) { 
            case KEY_UP:
                moveCursor(0, -1); 
                break;
            case KEY_DOWN:
                moveCursor(0, 1); 
                break;
            case KEY_LEFT:
                moveCursor(-1, 0); 
                break;
            case KEY_RIGHT:
                moveCursor(1, 0);
                break;
            case KEY_BACKSPACE:
            case 127: // Linux backspace
            case 8:   // Windows backspace
                deleteChar(); 
                break;
            case KEY_RESIZE:
                getmaxyx(stdscr, screenHeight, screenWidth); 
                break;
            case 1: // Ctrl+A - 行首
                cursorX = 0; 
                break;
            case 5: // Ctrl+E - 移动到行尾
                if (cursorY < buffer.size()) { 
                    cursorX = buffer[cursorY].size(); 
                }
                break;
            case 6: // Ctrl+F - 查找功能
            {
                showMessage("Find: "); 
                echo(); 
                char term[256]; 
                getstr(term); 
                noecho();
                find(term); 
            }
            break;
            case 18: // Ctrl+R - 替换功能
            {
                showMessage("Replace: ");
                echo();
                char term[256]; 
                getstr(term); 
                showMessage("With: ");
                char replacement[256]; 
                getstr(replacement); 
                noecho(); 
                replace(term, replacement);
            }
            break;
            case 19:
                saveFile(); 
                break;
            case 17: 
                if (modified) { 
                    showMessage("Save changes? (y/n) "); 
                    int confirm = getch(); 
                    if (confirm == 'y' || confirm == 'Y') { 
                        saveFile(); 
                    }
                }
                return; 
            default:
                if (isprint(ch)) { 
                    insertChar(ch); 
                }
            }

            display();
        }
    }};

int main(int argc, char* argv[]) { 
    TextEditor editor; 

    if (argc > 1) { 
        editor.openFile(argv[1]);
    }

    editor.run();
    return 0; 
}}