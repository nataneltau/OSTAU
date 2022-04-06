#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>




/* Credits:
 * https://www.youtube.com/watch?v=PIb2aShU_H4 - help understand dup2() function
 * https://www.youtube.com/watch?v=Z2THA6jX3j8 - help with SIGCHLD (in hebrew)
 * https://stackoverflow.com/questions/7171722/how-can-i-handle-sigchld - help prevent zombies
*/




/*
 * this function deside which command is in the arglist
*/
int tagCommands(int count, char **arglist);

int prepare(void){

    signal(SIGINT, SIG_IGN);//you shall not pass!
    return 0;
}//end of function prepare


int finalize(void){

    signal(SIGINT, SIG_DFL);//you shall pass now :)
    return 0;
}//end of function finalize

int process_arglist(int count, char **arglist){

    int tag = tagCommands(count, arglist); 
    int pid ,status;
    if(signal(SIGINT, SIG_IGN) == SIG_ERR){//you shall not pass! also work for background processes
        fprintf( stderr, "%s", strerror(errno));
        exit(1);
    }

    if(signal(SIGCHLD, SIG_IGN) == SIG_ERR){//prevent zombies 
        fprintf( stderr, "%s", strerror(errno));
        exit(1);
    }


    if(tag == 1){//we are in case: executing commands , section 1.2.1
        if((pid = fork()) < 0){
            fprintf( stderr, "%s", strerror(errno));
            exit(1);
        }

        if(pid == 0){// we are a child
            if(signal(SIGINT, SIG_DFL) == SIG_ERR){
                fprintf( stderr, "%s", strerror(errno));
                exit(1);
            }
            if(signal(SIGCHLD, SIG_DFL) == SIG_ERR){
                fprintf( stderr, "%s", strerror(errno));
                exit(1);
            }
            if(execvp(arglist[0], arglist) == -1){
                fprintf( stderr, "%s", strerror(errno));
                exit(1);
            }
        }
        else{//we are parents
            if(waitpid(pid, &status, 0) < 0){
                if(!(errno == ECHILD || errno == EINTR)){
                    fprintf( stderr, "%s", strerror(errno));
                    exit(1);
                }//end of inner if

            }//end of if
        }
        

    }//end of if(tag == 1)

    else if(tag == 2){ //we are in case: executing commands in the background , section 1.2.2 
        
        arglist[count-1] = NULL;//remove "&" from arglist
        
        if((pid = fork()) < 0){
            fprintf( stderr, "%s", strerror(errno));
            exit(1);
        }

        if(pid == 0){// we are a child
            if(signal(SIGCHLD, SIG_DFL) == SIG_ERR){
                fprintf( stderr, "%s", strerror(errno));
                exit(1);
            }
            if(execvp(arglist[0], arglist) == -1){
                fprintf( stderr, "%s", strerror(errno));
                exit(1);
            }
        }
    

    }//end of if(tag == 2)

    else if(tag == 3){ //we are in case: single piping , section 1.2.3

        int countOfPipe = 0;
        int pfds[2];

        while (strcmp(arglist[countOfPipe++], "|") != 0);//sesrch for "|"

        countOfPipe--;//because the ++ in the while loop we need to go one back

        arglist[countOfPipe] = NULL;//before this, was arglist[countOfPipe] == "|"

        //to one child send arglist as it is to other send arglist+countOfPipe+1

        int anotherPid;

        if(pipe(pfds) < 0){
            fprintf( stderr, "%s", strerror(errno));
            exit(1);
        }

        if((pid = fork()) < 0){
            fprintf( stderr, "%s", strerror(errno));
            exit(1);
        }

        if(pid == 0){//first child
            if(signal(SIGINT, SIG_DFL) == SIG_ERR){
                fprintf( stderr, "%s", strerror(errno));
                exit(1);
            }
            if(signal(SIGCHLD, SIG_DFL) == SIG_ERR){
                fprintf( stderr, "%s", strerror(errno));
                exit(1);
            }
            close(pfds[0]);
            dup2(pfds[1], STDOUT_FILENO);
            close(pfds[1]);
            if(execvp(arglist[0], arglist) == -1){
                fprintf( stderr, "%s", strerror(errno));
                exit(1);
            }

        }
        else{//we are parent

            if((anotherPid = fork()) < 0){
                fprintf( stderr, "%s", strerror(errno));
                exit(1);
            }   

            if(anotherPid == 0){//second child
                if(signal(SIGINT, SIG_DFL) == SIG_ERR){
                    fprintf( stderr, "%s", strerror(errno));
                       exit(1);
                }
                if(signal(SIGCHLD, SIG_DFL) == SIG_ERR){
                    fprintf( stderr, "%s", strerror(errno));
                    exit(1);
                }
                close(pfds[1]);
                dup2(pfds[0], STDIN_FILENO);
                close(pfds[0]);
                if(execvp((arglist+countOfPipe+1)[0], (arglist+countOfPipe+1)) == -1){
                fprintf( stderr, "%s", strerror(errno));
                exit(1);
                }
                
            }
            else{//we are still parent
                close(pfds[1]);
                close(pfds[0]);
                if(waitpid(pid, &status, 0) < 0){
                    if(!(errno == ECHILD || errno == EINTR)){
                      fprintf( stderr, "%s", strerror(errno));
                      exit(1);
                    }//end of inner if

                }//end of if(waitpid(pid))

                if(waitpid(anotherPid, &status, 0) < 0){
                    if(!(errno == ECHILD || errno == EINTR)){
                        fprintf( stderr, "%s", strerror(errno));
                        exit(1);
                    }//end of inner if

                }//end of if(waitpid(anotherPid))
            }
        }//end of else

    }//end of if(tag == 3)

    else{ // we are in case: output redirecting , section 1.2.4
        
        arglist[count-2] = NULL;//remove ">>" from arglist

        if((pid = fork()) < 0){
            fprintf( stderr, "%s", strerror(errno));
            exit(1);
        }

        if(pid == 0){// we are a child
            int redirectOut = open(arglist[count-1], O_CREAT | O_APPEND, 0644);//open the file
            if(redirectOut == -1){//error in open file
                fprintf( stderr, "%s", strerror(errno));
                exit(1);
            }
            if(signal(SIGINT, SIG_DFL) == SIG_ERR){
                fprintf( stderr, "%s", strerror(errno));
                exit(1);
            }
            if(signal(SIGCHLD, SIG_DFL) == SIG_ERR){
                fprintf( stderr, "%s", strerror(errno));
                exit(1);
            }
            dup2(redirectOut, STDOUT_FILENO);//redirect so the output will be written to the file
            close(redirectOut);//close the file
            if(execvp(arglist[0], arglist) == -1){
                fprintf( stderr, "%s", strerror(errno));
                exit(1);
            }
        }
        else{//we are parents
            if(waitpid(pid, &status, 0) < 0){
                if(!(errno == ECHILD || errno == EINTR)){
                    fprintf( stderr, "%s", strerror(errno));
                    exit(1);
                }//end of inner if

            }//end of if
        }

    }//end of else which is equal to if(tag==4)

    return 1; 


}//end of function process_arglist


int tagCommands(int count, char **arglist){

    int index = count-1;
    char *temp;

    while(index>0){
        temp = arglist[index--];

        if(strcmp(temp, "&")){
            return 2;
        }
        else if(strcmp(temp, "|")){
            return 3;
        }
        else if(strcmp(temp, ">>")){
            return 4;
        }

    }//end of while

    return 1;

}//end of function tagCommands