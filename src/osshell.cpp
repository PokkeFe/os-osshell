#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <sstream>
#include <vector>
#include <unistd.h>
#include <list>
#include <filesystem>
#include <sys/wait.h>

void splitString(std::string text, char d, std::vector<std::string>& result);
void vectorOfStringsToArrayOfCharArrays(std::vector<std::string>& list, char ***result);
void freeArrayOfCharArrays(char **array, size_t array_length);
void printHistory(std::list<std::string> command_history, int history_list_size);
bool findInPath(std::string command, std::vector<std::string>* os_path_list, std::string* foundPath);
bool foundInFolder(std::string command, std::string folderPath);
void executeCommand(std::string path, char ***command_list_exec);

int main (int argc, char **argv)
{
    // Get list of paths to binary executables
    std::vector<std::string> os_path_list;
    char* os_path = getenv("PATH");
    splitString(os_path, ':', os_path_list);

    // Allocate command history doubly-linked list
    std::list<std::string> command_history;
    // TODO: Attempt to load command history from file

    // Welcome message
    printf("Welcome to OSShell! Please enter your commands ('exit' to quit).\n");

    std::vector<std::string> command_list; // to store command user types in, split into its variour parameters
    char **command_list_exec; // command_list converted to an array of character arrays
    // Repeat:
    //  Print prompt for user input: "osshell> " (no newline)
    //  Get user input for next command
    //  If command is `exit` exit loop / quit program
    //  If command is `history` print previous N commands
    //  For all other commands, check if an executable by that name is in one of the PATH directories
    //   If yes, execute it
    //   If no, print error statement: "<command_name>: Error command not found" (do include newline)

    int running = 1;
    std::string user_input;
    while(running) {
        printf("osshell> ");
        std::getline(std::cin, user_input);

        // extract command from user input
        splitString(user_input, ' ', command_list);

        std::string command = command_list[0];

        // check preset commands
        if(command.compare("exit") == 0){
            // 'exit' command
            running = 0;
        } else if(command.compare("history") == 0) {
            // 'history' command
            if(command_list.size() > 1) {
                std::string argument = command_list.at(1);
                if(argument.compare("clear") == 0) {
                    // argument is `clear`, i.e. `history clear`
                    // empty out the command history
                    while(command_history.size() > 0) {
                        command_history.pop_back();
                    }
                } else {
                    // parse argument to int
                    int num = std::stoi(argument);
                    if(num > command_history.size()) {
                        printf("ERROR: Command only has %d entries.\n", (int)command_history.size());
                    } else if(num < 0) {
                        printf("ERROR: Input cannot be less than zero.");
                    } else {
                        printHistory(command_history, num);
                    }
                }
            } else {
                int history_list_size = (int)command_history.size();
                printHistory(command_history, history_list_size);
            }
        } else {
            // not a pre-defined command
            // check if searching for a local command
            if(command.substr(0,2).compare("./") == 0) {
                // command is a local executable
                // if found in local folder, execute command in process
                if(foundInFolder(command, "")){
                    vectorOfStringsToArrayOfCharArrays(command_list, &command_list_exec);
                    executeCommand(command, &command_list_exec);
                    freeArrayOfCharArrays(command_list_exec, command_list.size() + 1);
                } else {
                    printf("%s: Error command not found\n", command.c_str());
                }
            } else {
                // command is a global executable, check path
                // search PATH folders for matching file.
                std::string foundPath;

                // if found, execute 
                if(findInPath(command, &os_path_list, &foundPath)) {
                    vectorOfStringsToArrayOfCharArrays(command_list, &command_list_exec);
                    executeCommand(foundPath, &command_list_exec);
                    freeArrayOfCharArrays(command_list_exec, command_list.size() + 1);
                } else {
                    printf("%s: Error command not found\n", command.c_str());
                }
            }
        }

        // add the command string to history
        if(!user_input.compare("history clear") == 0) {
            command_history.push_front(user_input);
            while(command_history.size() > 128) {
                command_history.pop_back();
            }
        }

    }

    return 0;
}

/*
   text: string to split
   d: character delimiter to split `text` on
   result: vector of strings - result will be stored here
*/
void splitString(std::string text, char d, std::vector<std::string>& result)
{
    enum states { NONE, IN_WORD, IN_STRING } state = NONE;

    int i;
    std::string token;
    result.clear();
    for (i = 0; i < text.length(); i++)
    {
        char c = text[i];
        switch (state) {
            case NONE:
                if (c != d)
                {
                    if (c == '\"')
                    {
                        state = IN_STRING;
                        token = "";
                    }
                    else
                    {
                        state = IN_WORD;
                        token = c;
                    }
                }
                break;
            case IN_WORD:
                if (c == d)
                {
                    result.push_back(token);
                    state = NONE;
                }
                else
                {
                    token += c;
                }
                break;
            case IN_STRING:
                if (c == '\"')
                {
                    result.push_back(token);
                    state = NONE;
                }
                else
                {
                    token += c;
                }
                break;
        }
    }
    if (state != NONE)
    {
        result.push_back(token);
    }
}

/*
   list: vector of strings to convert to an array of character arrays
   result: pointer to an array of character arrays when the vector of strings is copied to
*/
void vectorOfStringsToArrayOfCharArrays(std::vector<std::string>& list, char ***result)
{
    int i;
    int result_length = list.size() + 1;
    *result = new char*[result_length];
    for (i = 0; i < list.size(); i++)
    {
        (*result)[i] = new char[list[i].length() + 1];
        strcpy((*result)[i], list[i].c_str());
    }
    (*result)[list.size()] = NULL;
}

/*
   array: list of strings (array of character arrays) to be freed
   array_length: number of strings in the list to free
*/
void freeArrayOfCharArrays(char **array, size_t array_length)
{
    int i;
    for (i = 0; i < array_length; i++)
    {
        if (array[i] != NULL)
        {
            delete[] array[i];
        }
    }
}

void printHistory(std::list<std::string> command_history, int history_list_size) {
    int print_marker = (int)command_history.size() - history_list_size;
    int history_index = 0;
    for(std::list<std::string>::reverse_iterator it = command_history.rbegin(); it != command_history.rend(); ++it){
        history_index += 1;
        if (history_index > print_marker){
            std::string cmd = (*it);
            std::cout << "  " << history_index << ": " << cmd << '\n';
        }
    }
}

bool findInPath(std::string command, std::vector<std::string>* os_path_list, std::string* foundPath) {
    for(int i = 0; i < (*os_path_list).size(); i++) {
        std::string path = (*os_path_list).at(i) + "/";
        if(foundInFolder(command, path)) {
            (*foundPath) = path + command;
            return true;
        }
    }
    return false;
}

bool foundInFolder(std::string command, std::string folderPath) {
    std::string path = folderPath + command;
    std::filesystem::file_status fstatus = std::filesystem::status(path);
    if(std::filesystem::is_regular_file(fstatus)) {
        if(!access(path.c_str(), X_OK)) {
            return true;
        }
    }
    return false;
}

void executeCommand(std::string path, char ***command_list_exec) {
    pid_t pid = fork();

    if(pid == 0) {
        // child
        execv(path.c_str(), (*command_list_exec));
        exit(0);
    } else {
        // parent
        int status;
        waitpid(pid, &status, 0);
    }
}