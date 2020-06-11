# word-counter
Find the number of occurrences of a word in a text file on Unix-like operating systems.

This program was created to help learn about processes and pipes used by operating systems.

Several large text files are included for testing. The output is very verbose to explain how the program works as it runs. 

When the program starts, a message from the command line instructs the user to enter a word (or string) to search in all listed file names. As the program runs, a child process is created for each text file to parse. A parent process is used to retreive word counts from all files. To achieve this, pipes are created to exchange data between the parent and child processes. One set of read and write pipes are created for each child process. Once all files have been parsed the word counts for all files will have been printed as they were found and the total word count summed from all files will be printed as well.

[Example run](/exampleRun.txt)

## Instructions
* **To Run:** `<exe> [text-file]...`

* **Use:** enter a word to search and count

* **Quit:** `CTRL-C or "$$$"`

## **Possible improvements:**
- [ ] Include options, such as -v for verbose, to not be verbose by default.
- [ ] Include options to do other types of string searches, such as first line found, if it was found at all, and more.

