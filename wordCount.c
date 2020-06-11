/**********************************************************************
Author: Owen Dunn

Multi-Process Text Analysis:
This program allows several processes to be created to search one file
each for the amount of times a certain string appears in each one.
The total count for all the files is reported. The process loops until
the user enters CTL-C signal.
**********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define LINE_SIZE     100 // Maximum line size that can be read
#define TRUE          1   // C language true value
#define FALSE         0   // C language false value
#define READ          0   // The read index of a pipe
#define WRITE         1   // The write index of a pipe

void sigHandler(int SIGNAL);

// Global Variables (to allow them to be used in sigHandler)
pid_t* pids; // all the process ids for created processes

int main(int argc, char** argv) {

	pid_t pid;            // Stores the return value from fork()

	/** Used to temporarily store command line input */
	char* line;

	/** Used to temporarily store a word from user input */
	char* word;
	char* token;          // Stores broken off string pieces
	int wordTotal = 0;    // Total word count over all files
	int fileTotal = 0;    // Total word count for one file
	int in, out;          // File descriptors to read(in) and write(out)
	int i = 0;            // Index value for loops
	char** files;         // Pointers to file name strings

	/** 2D array of all pipes the parent reads from and the child writes
	 *  to. */
	int** parentPipes;    // Parent reads from 0, child writes to 1

	/** 2D array of all pipes the child reads from and the parent writes
	 *  to. */
	int** childPipes;     // Child reads from 0, parent writes to 1
	int childIndex;       // Keeps track which child the process is
	char* fileName;       // A file name (used for child processes)
	int numProcesses = 0; // Total number of processes created
	char* p = 0;
	int fd_text = 0;      // File desciptor for a text file
	char* fileText;       // Stores the text of a file
	struct stat st;       // Used to get the file size

	// loop: Get search files from the user's command line input.
	printf("****************Multi-File Word Search****************\n");
	numProcesses = argc - 1; // Not counting exe file string

	/*
	 * This program can handle as many as files as memory will allow,
	 * but a warning is given.
     */
	if (numProcesses > 10) {
	    printf("Please enter 10 or less files to search at a time.\n");
	    printf("Testing has not been done on more than 10 files.\n");
	}

	/*
	 * Allocate memory for arrays of process ids, files, child pipes,
	 * parent pipes, a raw line of input, and a word to send the search
	 * string to all child processes from the parent process.
	 */
	pids = (pid_t*)malloc((numProcesses+1)*sizeof(pid_t));
	files = (char**)malloc((numProcesses+1)*sizeof(char*));
	childPipes = (int**)malloc((numProcesses)*sizeof(int*));
	parentPipes = (int**)malloc((numProcesses)*sizeof(int*));
	line = (char*)malloc(LINE_SIZE*sizeof(char));
	word = (char*)malloc(LINE_SIZE*sizeof(char));

	// Create pipes and get file names.
	if ( numProcesses > 0 ) {

	    // Create the pipes.
	    for (i = 0; i < numProcesses; ++i) {
	        childPipes[i] = (int*)malloc(2*sizeof(int));
	        parentPipes[i] = (int*)malloc(2*sizeof(int));
	        if ( pipe(childPipes[i]) || pipe(parentPipes[i]) ) {
	            perror ("*** Error: failed to create pipe(s)\n");
	            exit(EXIT_FAILURE);
	        }
	    }

	    // Assign the files to search.
	    for (i = 0; i < numProcesses; ++i) {
	        files[i] = (char*)malloc(LINE_SIZE*sizeof(char));
	        strcpy(files[i], argv[i + 1]);
	        printf("Will search file \"%s\".\n", files[i]);
	    }
	}
	else {
	    printf("Please enter at least one file name to search.\n");
	    printf("Start in format: <exe> <file1> <file2> ...\n");
	    exit(1);
	}
	files[i] = NULL; // end of files list

	// Create all processes (1 for each file).
	for (i = 0; i < numProcesses; ++i) {
	    childIndex = i; // used for children to reference own data
	    if ( (pid = fork()) < 0 ) {
	        perror("*** Error: fork failed\n");
	        exit(EXIT_FAILURE);
	    }
	    else if ( pid == 0 ) { // break out for child processes
	        printf("Searcher process #%d created.\n", getpid());

	        /*
	         * Set up bi-directional pipe for child process of this
	         * index.
	         */
	        in = childPipes[childIndex][READ];
	        out = parentPipes[childIndex][WRITE];

	        // Close unused pipe ends for a child process.
	        close(childPipes[childIndex][WRITE]);
	        close(parentPipes[childIndex][READ]);

            // Copy unique file name to fileName variable for process.
	        fileName = malloc(LINE_SIZE*sizeof(char));
	        strcpy(fileName, files[i]);
	        printf("Searcher process #%d will search file \"%s\".\n",
	               getpid(), fileName);
	        break; // leave to loop (avoid forking more processes)
	    }
	    else { // parent: record all child ids
	        pids[i] = pid;

	        /*
	         * Set up parent process bi-directional pipe for this
	         * process index. in/out are not constant for parent, so
	         * use different child index for each read/write.
	         */
	        close(childPipes[childIndex][READ]);
	        close(parentPipes[childIndex][WRITE]);
	    }
	}
	pids[i] = 0; // signal the end of the array data

	// loop: Enter word(s) to be searched. The total count is returned.
	if ( pid > 0 ) { // parent only
	    signal(SIGINT, sigHandler); // redefine CTL-C behavior
	    sleep(1); // Used to allow word input query to print last.
	    while (TRUE) {

            // Get word to search from the command line.
	        printf("Enter a word to search (enter CTRL-C or \"$$$\"to quit): ");
	        fgets(line, LINE_SIZE, stdin);
	        token = strtok(line, " \n\t\0"); // remove '\n'
	        strcpy(word, token);

	        // Send word to each process by writing it to a pipe.
	        for (i = 0; i < numProcesses; ++i) {
	            if ( write(childPipes[i][WRITE], word, strlen(word)) <= 0 ) {
	                perror("Word not written to pipe successfully.\n");
	                exit(EXIT_FAILURE);
	            }
	            printf("Word \"%s\" written to pipe of process #%d.\n",
	                   word, pids[i]);
	        }

	        // Shut down if sentinal string entered.
	        if ( strcmp(word, "$$$") == 0 ) {
	            break;
	        }

	        printf("Word \"%s\" will be searched.\n", word);

	        /*
	         * Wait for all processes to return totals (in order). The
	         * child processes all still run concurrently. The values
	         * from the pipes are read in a specific order though.
	         */
	        wordTotal = 0; // reset count
	        fileTotal = 0; // reset count
	        for (i = 0; i < numProcesses; ++i) {
	            printf("Parent process #%d waiting for a word total.\n",
	                   getpid());
	            if ( read(parentPipes[i][READ],
	                      &fileTotal, sizeof(int)) > 0 ) { // data read
	                printf("Parent process read count data from pipe of process #%d\n",
	                       pids[i]);
	                printf("Total \"%s\" in file \"%s\": %d\n",
	                       word, files[i], fileTotal);

	                wordTotal += fileTotal;
	            }
	            else {
	                printf("*** Error: pipe not read successfully.\n");
	                exit(EXIT_FAILURE);
	            }
	        }
	        printf("Total \"%s\" found across all files: %d\n\n",
	               word, wordTotal);

	        // Create space for new word and delete old word.
	        free(word);
	        word = (char*)malloc(LINE_SIZE*sizeof(char));
	    }
	}
	else if ( pid == 0 ) { // child process

	    // Get the file to search and store in a string.
	    // -may not work if file is too large? (works for ~200KB)
	    if ( (fd_text = open(fileName, O_RDONLY)) < 0 ) {
	        perror("Failed to open a file.");
	    }
	    stat(fileName, &st); // get file data size
	    fileText = malloc(st.st_size*sizeof(char));
	    if ( read(fd_text, fileText, st.st_size) <= 0 ) {
	        perror("Failed to read a file.");
	    }

	    // loop: Search total occurances of a word. The pipe will send
	    // the word to each process. Each searcher process will then
	    // read the pipe to get the word to search. The search processes
	    // will then concurrently search their assigned files for the
	    // count of the word. The searcher processes will then write to
	    // a second pipe the total count and the master will sum the
	    // total from all the processes. The processes will then wait
	    // for another word and the process will repeat until a quit
	    // signal or "$$$" is sent.
	    while( TRUE ) {

	        // Get the word to search.
	        if ( read(in, word, LINE_SIZE) >= 0 ) { // waits for write
	            if ( strcmp(word, "$$$") == 0 ) {
	                break;
	            }
	            printf("Searcher process #%d begins searching file \"%s\" for word \"%s\".\n",
	                    getpid(), fileName, word);
	        }
	        else {
	            printf("*** Error: pipe not read successfully.\n");
	            exit(1);
	        }

	        // Find the total count of the word in a file.
	        fileTotal = 0;
	        p = fileText;
	        if ( strcmp(word, "") != 0) {
	            while ( (p = strstr(p, word)) != NULL ) {
	                ++fileTotal;
	                ++p;
	            }
	        }

	        // Return the word count to the parent.
	        if ( write(out, &fileTotal, sizeof(int)) <= 0 ) {
	            perror("Child process failed to write to pipe.\n");
	            exit(EXIT_FAILURE);
	        }
	        else {
	            printf("Child process #%d wrote a word count to a pipe\n.",
	                   getpid());
	        }

	        memset(word, 0, LINE_SIZE); // Clear old string.
	    }
	}

	// Free all allocated memory, close all files/pipes, and exit.
	if (pid > 0) { // Run for parent only.
	    sleep(1); // wait for child processes to exit
	    printf("Shutting down parent process.\n");
	    // communicate search string to two searcher processes via pipes
	    for(i = 0; i < numProcesses; ++i) {
	        free(files[i]);
	        close(parentPipes[i][READ]);
	        close(childPipes[i][WRITE]);
	        free(childPipes[i]);
	        free(parentPipes[i]);
	    }
	    free(childPipes);
	    free(parentPipes);
	    free(files);
	    free(word);
	    free(pids);
	    free(line);
	}
	else if (pid == 0) { // child process
	    fprintf(stderr,
	    "Shutting down searcher process #%d.\n",
	    getpid());
	    close(fd_text);
	    close(in);
	    close(out);
	}

	return 0;
}

/**********************************************************************
This function handles the signal CTL-C. All created child processes
and the parent process are gracfully shut down. Only the parent
process calls this function. A global array variable storing all the
child process ids is used to know which processes to sent the
SIGKILL signal. A value of zero must be assigned to signal the end of
the array for this function to shut down all the child processes.
**********************************************************************/
void
sigHandler (int sigNum)
{
	int i = 0;

	// Gracefully exit all child processes using the global pids[] array.
	while (pids[i] != 0) {
	    fprintf(stderr,
	            "\n^C received. Shutting down searcher process #%d.\n",
	            pids[i]);
	    kill(pids[i], SIGKILL);
	    ++i;
	}

	// Exit parent process gracefully.
	perror("\nShutting down parent process.\n");
	exit(EXIT_SUCCESS);
}
