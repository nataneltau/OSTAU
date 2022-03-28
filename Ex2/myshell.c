#include <stdio.h>
#include <string.h>
#include <errno.h>

/*
 * this function deside which command is in the arglist
*/
int tagCommands(int count, char **arglist);

int prepare(void){
    return 0;
}//end of function prepare


int finalize(void){
    return 0;
}//end of function finalize

int process_arglist(int count, char **arglist){

    int tag = tagCommands(count, arglist); 
    int pid ,status;

    if(tag == 1){//we are in case: executing commands , section 1.2.1
        pid = fork();

        if(pid == 0){// we are a child
            execvp(*arglist[0], *arglist);
        }
        else{
            waitpid(pid, &status, 0);
        }
        //return 1; //modify based on the answer in the forum

    }//end of if(tag == 1)

    else if(tag == 2){ //we are in case: executing commands in the background , section 1.2.2
        
        *arglist[count-1] = NULL;//remove & from arglist
        
        pid = fork();

        if(pid == 0){// we are a child
            execvp(*arglist[0], *arglist);
        }


    }//end of if(tag == 2)

    //NOT
    //FI
    //NI
    //SHED!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    else if(tag == 3){ //we are in case: single piping , section 1.2.3

        int countOfPipe = 0;

        while (*agrlist[countOfPipe++] != "|");
        countOfPipe--;

        arglist[countOfPipe] = NULL;

        //to one child send arglist as it is to other send arglist+countOfPipe+1

    }//end of if(tag == 3)

    //NOT
    //FI
    //NI
    //SHED!!!!!!!
    //need to use dup2()
    else{ // we are in case: output redirecting , section 1.2.4

        FILE *ifp = fopen(*arglist[count-1], "a");

        pid = fork();

        if(pid == 0){// we are a child
            execvp(*arglist[0], *arglist);
        }

    }//end of else which is equal to if(tag==4)

    

    //modify based on the answer in the forum
    /*
        while(-1 != wait(&status));

        return 1;

    */

}//end of function process_arglist
