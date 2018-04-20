#include <iostream>
#include <iterator>
#include <sstream>
#include <stdio.h>
#include <fcntl.h>
#include <cstdio>
#include <vector>
#include <string>
#include <cstring>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <regex.h>
#include "hw4.h"



int main() {
    int i;
    std::string line;
    char *cwd_cstr = getcwd(NULL, 0);
    std::string cwd(cwd_cstr);
    //std::string teststr = "ls";
    //std::vector<char> char_array(teststr.begin(), teststr.end());
    //char *const test_args[] = {&char_array[0]};

    while(1) {
        // our prompt
        char *const environ_var[] = { NULL };
        std::cout << ">";
        std::getline(std::cin, line);
        if (line == "")
            exit(EXIT_SUCCESS);


        std::vector<int*> pipevec;
        int valid;
        std::vector< std::vector<std::string> > commands = parse_line(line, &pipevec, &valid);
        if (valid) {
            std::cout << "invalid input." << std::endl;
            continue;
        }
        //std::cout << "pipevec size: " << pipevec.size() << std::endl;
        //std::cout << "filevec size: " << filevec.size() << std::endl;
        //std::cout << "readvec size: " << readvec.size() << std::endl;
        //std::cout << "PIPEVEC " << pipevec[0][0] << std::endl;
//        std::stringstream ss(line);
//        std::istream_iterator<std::string> begin(ss);
//        std::istream_iterator<std::string> end;
//        std::vector<std::string> vstrings(begin, end);
//
//        std::vector< std::vector<std::string> > commands;
//        commands.push_back(vstrings);
        //end parse_line

        std::vector<std::string> args;
        
        std::vector<pid_t> pids;

        int status;
        
        // for each command in the input
        for (i = 0; i < commands.size(); i++) {
            std::vector<std::string> args(commands[i].begin(), commands[i].end());
            std::string command = commands[i][0];
            //std::cout << "command to be executed: " << command << std::endl;

            bool is_fwrite = 0;
            bool is_fread = 0;

            if (command == "exit") 
                exit(EXIT_SUCCESS);

            if (command == ">") {
                is_fwrite = 1;
                args.erase(args.begin());
                command = args[0];
            }

            if (command == "<") {
                is_fread = 1;
                args.erase(args.begin());
                command = args[0];
            }
            //std::vector<char*> cstrings;
            char** cstrings = (char**)malloc((args.size() + 1)*sizeof(char*));
            //cstrings.reserve(args.size());
            std::string filepath = "";
            if(command[0] != '/')
                filepath += cwd;
            filepath += command;
            const char *filepath2 = filepath.c_str();

            //for (std::vector<std::string>::const_iterator iter = args.begin(); iter != args.end(); iter++) {
            //    std::cout << (*iter).c_str() << std::endl;

            //}

            if (args.size() == 1) {
                cstrings[0] = (char*)malloc(100);
                strcpy(cstrings[0], args[0].c_str());
                cstrings[1] = NULL;
            }
            else {
                for (int j = 0; j < args.size(); j++) {
                    //cstrings[j] = const_cast<char*>(args[j].c_str());
                    //std::cout << "shitty code: " << j << " " << const_cast<char*>(args[j].c_str()) << std::endl;
                    cstrings[j] = (char*)malloc(100);
                    strcpy(cstrings[j], args[j].c_str());
                }
                cstrings[args.size()] = NULL;
            }

            //std::cout << (filevec[i]) << std::endl;
            pid_t pid = fork();
            if (pid == 0) {
                //do redirection stuff
                //std::cout << *(filevec[i]) << std::endl;
                    //std::cout << "i!=0" << std::endl;
                if( i < pipevec.size() && pipevec[i] != NULL) {
                    if (pipevec[i][3] != -1) {
                        dup2(pipevec[i][3], STDOUT_FILENO);
                    }
                    else if (pipevec[i][2] != -1) {
                        dup2(pipevec[i][2], STDIN_FILENO);
                    }
                    if (pipevec[i][0] != -1) {
                        int *pipeinfo = pipevec[i];
                        close(pipeinfo[0]);
                        dup2(pipeinfo[1], STDOUT_FILENO);
                        close(pipeinfo[1]);
                    }
                }
                if(i != 0 && pipevec.size() > 0) {
                    int *prevpipeinfo = pipevec[i-1];
                    close(prevpipeinfo[1]);
                    dup2(prevpipeinfo[0], STDIN_FILENO);
                    close(prevpipeinfo[0]);
                }
                //std::cout << "execing " << i << std::endl;
                execve(filepath2, cstrings, environ_var);
                perror("execve");
                exit(EXIT_FAILURE);
            }
            else {
                for (int k = 0; k < args.size(); k++) {
                    free(cstrings[k]);
                }
                free(cstrings);
                pids.push_back(pid);
                if ((i < commands.size() - 1) && pipevec[i] != NULL) {
                    close(pipevec[i][1]);
                }
            }
        }
        for (i = 0; i < commands.size(); i++) {
            if (i < commands.size() - 1)
                close(pipevec[i][1]);
            waitpid(pids[i], &status, 0);
            //if (i != 0)
            //    close(pipevec[i][0]);
            //if (i != commands.size() - 1)
            //    close(pipevec[i][1]);
            std::cerr << "Child finished with exit code: " <<  WEXITSTATUS(status) << std::endl;
            //std::cout << "child " << i << " finished" << std::endl;
        }

        
    }
}


std::vector< std::vector<std::string> > parse_line(std::string line, std::vector<int*> *pipevec, int *valid) {
        std::stringstream ss(line);
        std::istream_iterator<std::string> begin(ss);
        std::istream_iterator<std::string> end;
        std::vector<std::string> vstrings(begin, end);
        std::vector<std::string>::iterator it; 
        std::vector< std::vector<std::string> > command_list;
        //std::cout << "Validate input: " << validate_input(vstrings) << std::endl;
        *valid = validate_input(vstrings);
        if (*valid) {
            return command_list;
        }

//        regex_t reg;
//        int regret;
//        char regbuf[100];
//        regret = regcomp(&reg, "^[a-zA-Z0-9_-/]+$", 0);
        int *fd = (int*)malloc(4*sizeof(int));
        fd[0] = -1;
        fd[1] = -1;
        fd[2] = -1;
        fd[3] = -1;

        std::vector<std::string> temp;
        for (it = vstrings.begin();; it++) {
            //std::cout << "hey i've got this thing here: " << *it << std::endl;
            if (it == vstrings.end()) {
                command_list.push_back(temp);
                //std::cout << fd[0] << fd[1] << fd[2] << fd[3] << std::endl;
                if(fd[0] != -1 || fd[1] != -1 || fd[2] != -1 || fd[3] != -1)
                    pipevec->push_back(fd);
                break;
            }
            if (*it == "|") {
                command_list.push_back(temp);
                temp.clear();

                pipe(fd);
                (*pipevec).push_back(fd);
                fd = (int*)malloc(4*sizeof(int));
                fd[0] = -1;
                fd[1] = -1;
                fd[2] = -1;
                fd[3] = -1;

                //(*filevec).push_back(NULL);
                //(*readvec).push_back(NULL);
                //std::cout << "temp : " << temp.size() << std::endl;
                //break;
            }
            else if (*it == ">") {
                //command_list.push_back(temp);
                //temp.clear();

                it++;
                std::string file_to_open = *(it);
                std::string filepath = "";
                char *cwd_cstr = getcwd(NULL, 0);
                std::string cwd(cwd_cstr);
                if (file_to_open[0] != '/') {
                    filepath += cwd;
                    filepath += "/";
                }
                filepath += file_to_open;
                fd[3] = open(filepath.c_str(), O_CREAT|O_RDWR, 00644);
                fd[2] = -1;
                //(*filevec).push_back(fd);
                //(*readvec).push_back(NULL);
            }
            else if (*it == "<") {
                it++;
                std::string file_to_open = *(it);
                std::string filepath = "";
                char *cwd_cstr = getcwd(NULL, 0);
                std::string cwd(cwd_cstr);
                if (file_to_open[0] != '/') {
                    filepath += cwd;
                    filepath += "/";
                }
                filepath += file_to_open;
                //int *fd = (int*) malloc(2*sizeof(int));
                fd[2] = open(filepath.c_str(), O_RDONLY);
                fd[3] = -1;
                //(*readvec).push_back(fd);
                //(*filevec).push_back(NULL);
            }
            else {
                //std::cout << "pushing back " << *it << " to temp" << std::endl;
                temp.push_back(*it);
            }
            //regret = regexec(&reg, "er$or", 0, NULL, 0);
            //if(regret = REG_NOMATCH) {
            //    std::cerr << "Invalid token in word: " << *it << std::endl;
            //}
            //else {
            //    std::cout << "token was valid" << std::endl;
            //}
        }   

        //command_list.push_back(vstrings);
        return command_list;
}

//1 = invalid character in input
//2 = invalid input sequence
//3 = no program supplied to pipe
int validate_input(std::vector<std::string> input) {
    std::vector<std::string>::iterator it; 
    int is_command = 1;
    int is_fileout = 0;
    int prev_was_token = 0;
    int prev_was_pipe = 0;
    for (it = input.begin(); it != input.end(); it++) {

        //first basic check for any invalid characters
        if ((*it).find_first_not_of("-0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ |/.><") != std::string::npos) {
            return 1;
        }
        //if the word is a token, make sure that it is in a valid place
        if (*it == "|" || *it == ">" || *it == "<") {
            if (is_command)
                return 2;
            if (prev_was_token)
                return 2;
            prev_was_token = 1;
            if (*it == ">") {
                is_fileout = 1;
                prev_was_pipe = 0;
            }
            if (*it == "|") {
                if(is_fileout) 
                    return 2;
                is_command = 1;
                prev_was_pipe = 1;
            }
            continue;
        }
        prev_was_pipe = 0;
        prev_was_token = 0;
        
        //if the word is a command, make sure that it doesn't contain any invalid tokens
        if ((*it).find_first_not_of("-/._0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ") != std::string::npos){
            return 1;
        }
        is_command = 0;
    }
    if (prev_was_token)
        return 3;
    return 0;
}
