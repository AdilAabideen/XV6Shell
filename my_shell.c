#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

/* Read a line of characters from stdin. */
int getcmd(char *buf, int nbuf) {

  // Prompting the User for a command and clearing buf
  write(2, ">>> ", 4);
  memset(buf, 0, nbuf);


  // Read input from the user into the buf
  // File descriptor 0 means standard input, reads up to nbuf
  int num = read(0, buf, nbuf);

  // Edge Cases
  // If there is an error reading, Nothing is read, return -1 to signal error
  if ( num < 0){
    printf("Error : Unfortunately we could not read the command Please try again\n");
    return -1;
  }
  if (num == 0 ){
    // If it is the end of the input
    return -1;
  }

  // Replace the newline with a null terminator to make ti a c string for c processing
  buf[num-1] = '\0';

  // Checks if there is atleast 1 character in the buf
  int x = 1;
  for (int y = 0; y < num - 1; y++){
    if (buf[y] != ' ' && buf[y] != '\t'){
      x = 0;
      break;
    }
  }

  // If no character return -2 to signify reprompt
  if (x){
    return -2;
  }
  return 0;
}



/*
  A recursive function which parses the command
  at *buf and executes it.
*/
__attribute__((noreturn))
void run_command(char *buf, int nbuf, int *pcp) {

  /* Useful data structures and flags. */
  char *arguments[10];
  int numargs = 0;
  /* Word start/end */
  int ws = 1;
  int we = 0;

  int redirection_left = 0;
  int redirection_right = 0;
  char *file_name_l = 0;
  char *file_name_r = 0;

  int p[2];
  int pipe_cmd = 0;

  // Pipe Position to determine where right command is 
  int pipe_pos = -1; 

  int sequence_cmd = 0;

  int i = 0;
  /* Parse the command character by character. */
  for (; i < nbuf; i++) {

    /* Parse the current character and set-up various flags:
       sequence_cmd, redirection, pipe_cmd and similar. */

    char c = buf[i];

    // If it is a <
    if (c == '<') {

      // File direction is left and null terminate the word
      redirection_left = 1;
      buf[i] = '\0';

      // If char before is not a space then make sure to add it to args aswell
      if (buf[i-1] != ' ' && buf[i-1] != '\0'){
          buf[i] = '\0';
          arguments[numargs++] = &buf[we];
          ws = 1; 
        }
      continue;

    // If it is a >
    } else if (c == '>') {

      // File direction is right and null terminate the word
      redirection_right = 1;
      buf[i] = '\0';

      // If char before is not a space then make sure to add it to args as well
      if (buf[i-1] != ' ' && buf[i-1] != '\0'){
          buf[i] = '\0';
          arguments[numargs++] = &buf[we];
          ws = 1; 
        }
      continue;

    // If it is a | ( Pipe )
    } else if ( c == '|') {

        // Set Pipe CMD and Pipe_pos to be able to retrieve right side, also null terminate
        pipe_cmd = 1;
        buf[i] = '\0';
        pipe_pos = i;

        // If char before is not a space then make sure to add it to args as well
        if (buf[i-1] != ' ' && buf[i-1] != '\0'){
          buf[i] = '\0';
          arguments[numargs++] = &buf[we];
          ws = 1; 
        }

        // Break as we dont need to parse the right command
        break;

    // If it is a ; ( List )
    } else if ( c == ';') {

      // Set sequence CMD and null terminate
        sequence_cmd = 1;
        buf[i] = '\0';

        // If char before is not a space then make sure to add it to args as well
        if (buf[i-1] != ' ' && buf[i-1] != '\0'){
          buf[i] = '\0';
          arguments[numargs++] = &buf[we];
          ws = 1; 
        }

        // Break as we dont need to parse the right command
        break;
    }
    

    // Reset C following null termination
    c = buf[i];

    if (!(redirection_left || redirection_right )) {
      /* No redirection, continue parsing command. */
      
      // Detecting white spaces or null termination, signifies End of Word
      if (c == ' ' || c == '\0'){
        if (!ws){

            // Add word to arguments, null terminate and reset start for the next word
            buf[i] = '\0'; 
            arguments[numargs++] = &buf[we]; 
            ws = 1;
        }

        // End of Command so Break
        if ( c == '\0') break;
      } else {

        // Keep looping in the middle of a word
        if (ws){
            we = i;
            ws = 0;
        }
      }
    } else {
      /* Redirection command. Capture the file names. */

        // Remove trailing white spaces before filename
        while (buf[i] == ' ') i++;

        // Depending of on redirection set file name 
        if (redirection_left && !file_name_l) {
            file_name_l = &buf[i];
        } else if (redirection_right && !file_name_r) {
            file_name_r = &buf[i];
        }

        // Move until we get to the next Command or white space
        while (buf[i] != ' ' && buf[i] != '\0' && buf[i] != ';' && buf[i] != '|' && buf[i] != '<' && buf[i] != '>') {
            i++;
        }
        c = buf[i];

        // Check for further operators
        if (c == '<') {
            redirection_left = 1;
            buf[i] = '\0';

            // Only add argument if it hasn't been added yet in this iteration
            if (!ws && (buf[i-1] != ' ' && buf[i-1] != '\0')) {
                arguments[numargs++] = &buf[we];
                ws = 1; 
            }
            continue;

        } else if (c == '>') {
            redirection_right = 1;
            buf[i] = '\0';

            // Only add argument if it hasn't been added yet in this iteration
            if (!ws && (buf[i-1] != ' ' && buf[i-1] != '\0')) {
                arguments[numargs++] = &buf[we];
                ws = 1; 
            }
            continue;

        } else if (c == '|') {
            pipe_cmd = 1;
            buf[i] = '\0';
            pipe_pos = i;

            // Only add argument if it hasn't been added yet in this iteration
            if (!ws && (buf[i-1] != ' ' && buf[i-1] != '\0')) {
                arguments[numargs++] = &buf[we];
                ws = 1; 
            }
            break;

        } else if (c == ';') {
            sequence_cmd = 1;
            buf[i] = '\0';

            // Only add argument if it hasn't been added yet in this iteration
            if (!ws && (buf[i-1] != ' ' && buf[i-1] != '\0')) {
                arguments[numargs++] = &buf[we];
                ws = 1; 
            }
            break;
        }

      // Null terminate to define end of filename
      buf[i] = '\0';
        
        
    }
  }

    arguments[numargs] = 0;

    /*
      Sequence command. Continue this command in a new process.
      Wait for it to complete and execute the command following ';'.
    */
    if (sequence_cmd) {
      sequence_cmd = 0;
      if (fork() != 0) {

        // Wait for child process to finish then return back and execute via recursion
        wait(0);
        buf = buf + i + 1;
        nbuf -= (i + 1);
        run_command(buf, nbuf, pcp);
        
      } 
      
    }

  /*
    If this is a redirection command,
    tie the specified files to std in/out.
  */
   if (redirection_left) {

        // Open file
        int fd = open(file_name_l, O_RDONLY);
        if (fd < 0) {
            fprintf(2, "Error: could not open input file %s\n", file_name_l);
            exit(1);
        }

        // Close the current standard input and duplicate the file and close original file
        close(0);
        dup(fd);
        close(fd);
    }

    if (redirection_right) {
        int fd = open(file_name_r, O_WRONLY | O_CREATE | O_TRUNC);
        if (fd < 0) {
            fprintf(2, "Error: could not open output file %s\n", file_name_r);
            exit(1);
        }

        // Close the current standard input and duplicate the file and close original file
        close(1);
        dup(fd);
        close(fd);
    }

    

  /* Parsing done. Execute the command. */

  /*
    If this command is a CD command, write the arguments to the pcp pipe
    and exit with '2' to tell the parent process about this.
  */
  if (strcmp(arguments[0], "cd") == 0) {

    // Check for num args
    if (numargs < 2){
        printf("Error cd: Missing arguments\n");
        exit(1);
    }

    // If it cant cd into it 
    if (chdir(arguments[1]) < 0){
        printf("Error cd : Cannot change to directory to %s\n", arguments[1]);
        exit(1);
    }

    // write argument to the pipe and exit with 2 to show CD command
    write(pcp[1], arguments[1], strlen(arguments[1]) + 1);
    exit(2);
  } else {
    /*
      Pipe command: fork twice. Execute the left hand side directly.
      Call run_command recursion for the right side of the pipe.
    */
    if (pipe_cmd) {
        if (pipe(p) < 0) {
            fprintf(2, "Error: pipe failed\n");
            exit(1);
        }


        // First child process for the left command
        if (fork() == 0) {

            // Close stdout and redirect it to pipes write end, close unused read end of pipe and original write end after duplication
            close(1);
            dup(p[1]);
            close(p[0]);
            close(p[1]);

            // Execute the left command with exec
            exec(arguments[0], arguments);
            printf("Error: exec failed to execute command %s\n", arguments[0]);
            exit(0);
        }

        // Second child process for the right command
        if (fork() == 0) {

            // Close stdin and redirect it to pipes read end, close unused write end of pipe and original read end after duplication
            close(0);
            dup(p[0]);
            close(p[1]);
            close(p[0]);

            // Execute the right command using recursion
            run_command(&buf[pipe_pos + 1], nbuf - (pipe_pos + 1), pcp);
            exit(0);
        }

        // Close both ends of the pipe in the parent process
        close(p[0]);
        close(p[1]);

        // Wait for both child processes to complete
        wait(0);
        wait(0);

    } else {
      
      // Execute commands
      exec(arguments[0], arguments);
      printf("Error: exec failed to execute command %s\n", arguments[0]);
    }
  }
  exit(0);
}

int main(void) {

  static char buf[100];

  int pcp[2];
  pipe(pcp);

  /* Read and run input commands. */
  
  while(1){

    // Get the status backf rom getCMD
    int getStatus = getcmd(buf, sizeof(buf));

    // If it is -1 it is an error and if it is -2 it is null command
    if (getStatus == -1){
      break;
    } else if ( getStatus == -2 ){
      continue;
    }


    if(fork() == 0)
      run_command(buf, 100, pcp);

    // This is the parent process, we need to wait for the child to finish executing
    int child_status;
    wait(&child_status);

    /*
      Check if run_command found this is
      a CD command and run it if required.
    */
    if (child_status == 2) {
      // `cd` was requested: read the target directory from the pipe
      
      char cd_path[100];
      read(pcp[0], cd_path, sizeof(cd_path));
      // Execute `chdir` in the parent process
      if (chdir(cd_path) < 0) {
        fprintf(2, "Error: cd cannot change directory to %s\n", cd_path);
      }
      continue;
    }
    
  }
  exit(0);
}
