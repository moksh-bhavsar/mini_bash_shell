#include <string>
#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <sys/stat.h>
#include "Parser.h"
#include <fcntl.h>

using namespace std;

int main(int args, char** argv) {
    struct stat status;
    fstat(STDIN_FILENO, &status);
    string input;

    // Display a character to show the user we are in an active shell.
    if (S_ISCHR(status.st_mode))
        cout << "? ";
        
    // Pipe files or read from the user prompt
    // TODO: Groups will need to swap out getline() to detect the tab character.
    while (getline(cin, input)) {

        cout << endl;
        
        // Add another case here if you are in a group to handle \t!
        if (input == "exit") {
            return 1;
        } else {
            // TODO: Put your command execution logic here

            // get the commands using Parser class
            std::list<Command>* commands = Parser::Parse(input);
            // file descriptor
            int* fd = new int[2];
            int tempFile = open("temp.txt", O_CREAT | O_RDWR);

            // creating pipe            
            if (pipe(fd) == -1){
                cerr << "Cannot pipe the commands" << endl;
                break;
            }

            // iterate through list of commands
            for (auto iter = commands->begin(); iter != commands->end(); iter++){
                
                // create a new process
                pid_t pid = fork();
                
                // check if the process is child or not
                if (pid == 0){

                    // create list of arguments
                    char** argv = new char*[iter->args.size()+2];
                    const char * name = iter->name.c_str();
                    argv[0] = (char *)iter->name.c_str();
                    int i = 1;
                    for (auto it = iter->args.begin(); it != iter->args.end(); it++){
                        argv[i] = (char *)it->c_str();
                        i++;

                    }

                    if (iter != commands->begin()){
                        dup2(tempFile, STDIN_FILENO);
                        close(tempFile);
                    }

                    if (iter == commands->begin()){
                        close(fd[1]);
                    }

                    argv[iter->args.size()+1] = NULL;

                    if (!iter->input_file.empty()){
                        fd[0] = open(iter->input_file.c_str(), O_RDWR,(mode_t) 0777);
                        if (fd[0] == -1 ){
                            cerr << "File cannot be read! Try Again!" << endl;
                            break;
                        }else{
                            dup2(fd[0], STDIN_FILENO);
                            close(fd[0]);
                        }
                    }

                    if (!iter->output_file.empty()){
                        fd[1] = open(iter->output_file.c_str(), O_RDWR | O_CREAT, (mode_t) 0777);

                        if (fd[1] == -1 ){
                            cerr << "File creation/editing failed! Try Again!" << endl;
                            break;
                        }else{
                            dup2(fd[1], STDOUT_FILENO);
                            close(fd[1]);
                        }
                    }

                    if (iter->name != "cd"){
                        execvp(iter->name.c_str(), argv);
                        continue;
                    } else{
                        chdir(argv[1]);
                    }
                   


                } else if (pid < 0){
                    cerr << "Child process not created! Try Again!" << endl;
                    break;
                } else{
                    int status;
                    close(tempFile);
                    remove("temp.txt");

                    waitpid(-1, &status, 0);
                }
            }
        }
        
        // Display a character to show the user we are in an active shell.
        if (S_ISCHR(status.st_mode))
            std::cout << "? ";
    }
}