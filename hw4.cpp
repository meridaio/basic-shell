//Nathaniel George - ntg9vz
//4-20-2018
//Written for hw4: shell


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
#include "hw4.h"



int main() {
    int i;
    std::string line;
    char *cwd_cstr = getcwd(NULL, 0);
    std::string cwd(cwd_cstr);

    while(1) {
        //if this shell supported environment variables, they would go here :)
        char *const environ_var[] = { NULL };
        // our prompt
        std::cout << ">";
        std::getline(std::cin, line);
        //if we recieve a blank line or an EOF, exit shell
        if (line == "")
            exit(EXIT_SUCCESS);
        if (line.size() > 100) {
            std::cout << "line is too long." << std::endl;
            continue;
        }

        std::vector<int*> pipevec;
        int valid;
        std::vector< std::vector<std::string> > commands = parse_line(line, &pipevec, &valid);
        if (valid) {
            std::cout << "invalid input." << std::endl;
            continue;
        }

        std::vector<std::string> args;
        
        std::vector<pid_t> pids;

        int status;
        
        // for each command in the input
        for (i = 0; i < commands.size(); i++) {
            std::vector<std::string> args(commands[i].begin(), commands[i].end());
            std::string command = commands[i][0];

            if (command == "exit") 
                exit(EXIT_SUCCESS);

            char** cstrings = (char**)malloc((args.size() + 1)*sizeof(char*));
            std::string filepath = "";
            if(command[0] != '/')
                filepath += cwd;
            filepath += command;
            const char *filepath2 = filepath.c_str();

            //malloc and copy in each argument as a string.
            //the last string in the array passed to exec() must be NULL
            for (int j = 0; j < args.size(); j++) {
                cstrings[j] = (char*)malloc(100);
                strcpy(cstrings[j], args[j].c_str());
            }
            cstrings[args.size()] = NULL;

            pid_t pid = fork();
            if (pid == 0) {
                //do redirection stuff
                if( i < pipevec.size() && pipevec[i] != NULL) {
                    if (pipevec[i][3] == -2 || pipevec[i][2] == -2) {
                        //unable to open a file for reading or writing
                        exit(EXIT_FAILURE);
                    }
                    if (pipevec[i][3] != -1) {
                        //file is open for writing from stdout
                        dup2(pipevec[i][3], STDOUT_FILENO);
                    }
                    else if (pipevec[i][2] != -1) {
                        //file is open for reading from stdin
                        dup2(pipevec[i][2], STDIN_FILENO);
                    }
                    if (pipevec[i][0] != -1) {
                        //a pipe is open to pipe to another command
                        int *pipeinfo = pipevec[i];
                        close(pipeinfo[0]);
                        dup2(pipeinfo[1], STDOUT_FILENO);
                        close(pipeinfo[1]);
                    }
                }
                //get the input file descriptor from the previous command's pipe info
                if(i != 0 && pipevec.size() > 0) {
                    int *prevpipeinfo = pipevec[i-1];
                    close(prevpipeinfo[1]);
                    dup2(prevpipeinfo[0], STDIN_FILENO);
                    close(prevpipeinfo[0]);
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
            std::cerr << "Child finished with exit code: " <<  WEXITSTATUS(status) << std::endl;
        }

        
    }
}

//takes a string as input and returns a vector of vectors. Each vector represents a single command
//separated by a pipe, and each string in the vectors represents a word separated by a space.
//if the input is not valid, the method returns a null vector and the int valid is set to indicate 
//that the input was not valid.
std::vector< std::vector<std::string> > parse_line(std::string line, std::vector<int*> *pipevec, int *valid) {
        std::stringstream ss(line);
        std::istream_iterator<std::string> begin(ss);
        std::istream_iterator<std::string> end;
        std::vector<std::string> vstrings(begin, end);
        std::vector<std::string>::iterator it; 
        std::vector< std::vector<std::string> > command_list;

        *valid = validate_input(vstrings);
        if (*valid) {
            return command_list;
        }

        //a file descriptor array consisting of 4 integers. fd[0] and fd[1] refer to the in
        //and out file descriptors (respectively) of the pipe. fd[2] refers to file in,
        //fd[3] refers to file out. -1 means the file descriptor does not exist.
        int *fd = (int*)malloc(4*sizeof(int));
        fd[0] = -1;
        fd[1] = -1;
        fd[2] = -1;
        fd[3] = -1;

        //temp is where we construct a single command.
        std::vector<std::string> temp;
        for (it = vstrings.begin();; it++) {
            if (it == vstrings.end()) {
                command_list.push_back(temp);
                if(fd[0] != -1 || fd[1] != -1 || fd[2] != -1 || fd[3] != -1)
                    pipevec->push_back(fd);
                break;
            }
            if (*it == "|") {
                //we can push back temp and then clear it because c++ automatically
                //passes in a copy of temp, rather than the object itself
                //isn't c++ great? :)
                command_list.push_back(temp);
                temp.clear();

                pipe(fd);
                (*pipevec).push_back(fd);
                //reset our fd variable now that we are dealing with a new command
                fd = (int*)malloc(4*sizeof(int));
                fd[0] = -1;
                fd[1] = -1;
                fd[2] = -1;
                fd[3] = -1;

            }
            else if (*it == ">") {
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
                //if fd[3] is 0, open failed. -1 is what we use to describe a nonexistant 
                //file descriptor, so change it to -2 on failure.
                if (fd[3] == -1) {
                    std::cout << "unable to open file for writing" << std::endl;
                    fd[3] = -2;
                }
                fd[2] = -1;
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
                fd[2] = open(filepath.c_str(), O_RDONLY);
                if (fd[2] == -1) {
                    std::cout << "unable to open file for reading" << std::endl;
                    fd[2] = -2;
                }
                fd[3] = -1;
            }
            else {
                temp.push_back(*it);
            }
        }   

        return command_list;
}

//1 = invalid character in input
//2 = invalid input sequence
//3 = no program supplied to pipe
int validate_input(std::vector<std::string> input) {
    std::vector<std::string>::iterator it; 
    int is_command = 1;
    int is_fileout = 0;
    int is_filein = 0;
    int seen_filename = 0;
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
                //program hangs if < and > are used on the same command, these
                //seen_filename statements would be uncommented if it worked
                //seen_filename = 0;
            }
            if (*it == "<") {
                is_filein = 1;
                //seen_filename = 0;
            }
            if (*it == "|") {
                if(is_fileout) 
                    return 2;
                is_command = 1;
                prev_was_pipe = 1;
                seen_filename = 0;
            }
            continue;
        }
        prev_was_pipe = 0;
        prev_was_token = 0;

        if (is_fileout || is_filein) {
            if(seen_filename)
                return 2;
            seen_filename = 1;
        }
        
        //if the word is a command, make sure that it doesn't contain any invalid tokens
        if ((*it).find_first_not_of("-/._0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ") != std::string::npos){
            return 1;
        }
        is_command = 0;
    }
    //if the last word is a token
    if (prev_was_token)
        return 3;
    //we passed all the checks, and return the success code 0
    return 0;
}
