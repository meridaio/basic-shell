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
        std::vector<int*> filevec;
        std::vector< std::vector<std::string> > commands = parse_line(line, &pipevec, &filevec);
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

            //for (std::vector<std::string>::const_iterator i = args.begin(); i != args.end(); i++) {
            //    std::cout << (*i).c_str() << std::endl;

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

            pid_t pid = fork();
            if (pid == 0) {
                //do redirection stuff
                if (filevec[i] != NULL) {
                    dup2(*(filevec[i]), STDOUT_FILENO);
                }
                else {
                if( i < pipevec.size() ) {
                    if (pipevec[i] != NULL) {
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
                }
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


std::vector< std::vector<std::string> > parse_line(std::string line, std::vector<int*> *pipevec, std::vector<int*> *filevec) {
        std::stringstream ss(line);
        std::istream_iterator<std::string> begin(ss);
        std::istream_iterator<std::string> end;
        std::vector<std::string> vstrings(begin, end);
        std::vector<std::string>::iterator it; 
        std::vector< std::vector<std::string> > command_list;

//        regex_t reg;
//        int regret;
//        char regbuf[100];
//        regret = regcomp(&reg, "^[a-zA-Z0-9_-/]+$", 0);

        std::vector<std::string> temp;
        for (it = vstrings.begin();; it++) {
            //std::cout << "hey i've got this thing here: " << *it << std::endl;
            if (it == vstrings.end()) {
                command_list.push_back(temp);
                break;
            }
            if (*it == "|") {
                command_list.push_back(temp);
                temp.clear();

                int *pipefd = (int*)malloc(2*sizeof(int));
                pipe(pipefd);
                (*pipevec).push_back(pipefd);
                (*filevec).push_back(NULL);
                //std::cout << "temp : " << temp.size() << std::endl;
                //break;
            }
            else if (*it == ">") {
                command_list.push_back(temp);
                temp.clear();

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
                int *fd = (int*) malloc(sizeof(int));
                *fd = open(filepath.c_str(), O_CREAT);
                (*filevec).push_back(fd);
                (*pipevec).push_back(NULL);
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
