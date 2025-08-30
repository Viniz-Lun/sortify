#define _GNU_SOURCE

#include <inotifytools/inotifytools.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <inotifytools/inotify.h>


void graceful(int signal){
	close_range(3, ~0U, CLOSE_RANGE_UNSHARE);
	printf("\n--Servizio Terminato da utente--\n");
	exit(signal);
}


int endsWith(char* string, char* endString){
	int len1, len2;
	len1 = strlen(string);
	len2 = strlen(endString);

	if(len1 < len2) return 0;
	for( int i = 0; i < len2; i++){
		if(string[len1 - len2 + i] != endString[i]) return 0; 
	}
	return 1;
}

// Checks if the string ends with a certain substring inside a list of strings, sizeOfList is number of strings present
// in said file.
// The corresponding first match string is copied on the address pointed by resultSuffix (limited to 7 bytes).
// return value 1 means match, 0 means no match
int endsWithResult(char* string, char** listOfStrings, int sizeOfList, char* resultSuffix){
	for(int i = 0; (i < sizeOfList); i++ ){
		if( endsWith(string, listOfStrings[i]) ){
			strncpy(resultSuffix, listOfStrings[i], 7);
			return 1;
		}
	}
	return 0;
}

int main(int argc, char** argv){
	int wd, inotify_fd;
	int fd;
	int len, o, isTens, listNum;
	
	char *imagesSuffix[] = {".png", ".jpg", ".webp", ".jpeg", ".gif", "svg", 0};
	int numImageSuffix = 6;

	char *image_dir = "images/";
	char *pdfSufix = ".pdf";
	char *pdf_dir = "pdfs/";
	char *new_dir = "";
	char endString[8];
	char new_name[256] = "";
	char full_path_old[1024] = "";
	char full_path_new[1024] = "";
	char buffer[sizeof(struct inotify_event) + 256];
	char toFinish[256] = "";
	char toFinishConfirm[256] = "";
	int toFinishCookie = 0;
	
	struct inotify_event *event;
	struct dirent dd = {0};

	char download[512] = "/home/";
	strcat(download, getlogin());
	strcat(download, "/Downloads/");

	//int numDir = 1;
	//char (directories[16])[256] = {"images/", ""};
	//char **suffixes[16] = {0};
	//suffixes[0] = &imagesSuffix;
	//if(argc > 1){
	//	if( !strcmp(argv[0], "h") || !strcmp(argv[0], "-h") ){
	//		printf("help haha\n");
	//		exit(0);
	//	}
	//	if( !strcmp(argv[0], "a") || !strcmp(argv[0], "-a") ){
	//		printf("Insert directory to forward to (Ctl-d to stop adding):\n");
	//		while(scanf("%4096s", directories[numDir]) > 0){
	//			while(getchar() != '\n');
	//			do{
	//			printf("Sufix to forward to \"%s\":\n", directories[numDir]);
	//			}while(scanf("%8s", suffixes)
	//		}
	//	}
	//}

	
// Initialize inotify instance
	inotify_fd = inotify_init();
	if(inotify_fd < 0){
		perror("Error in the creation of inotify instance");
		exit(1);
	}

// Create inotify watcher on directory, to scan for creation, finished file write and attribute change.
	wd = inotify_add_watch(inotify_fd, download, IN_CREATE | IN_CLOSE_WRITE | IN_ATTRIB);
	if(wd < 0){
		perror("Error in the creation of watch item for directory Downloads");
		close(inotify_fd);
		exit(2);
	}

// Set SIGINT to call function to close all file descriptors used by program.
	signal(SIGINT, graceful);

	printf("Created inotify instance and checking Downloads folder, entering cycle...\n");

// Start event listening cycle, event gets saved to buffer and accessed with event pointer.
	while( read(inotify_fd, &buffer, sizeof(struct inotify_event) + 256) > 0 ){
		event = (struct inotify_event*) buffer;
// This if is kinda useless but why not check.
		if( event->wd == wd ){
// If the file is a temporary file to facilitate the download we ignore it.
			if( endsWith(event->name, ".part") || endsWith(event->name, ".tmp") ) continue;
// We store the name of the file created and wait for another event.
			if( event->mask == IN_CREATE ) {
				strcpy(toFinish, event->name);
				continue;
			}
// If the file we *just* stored the value for was written to, we store that same value at a different variable
// and wait for another event.
			if( event->mask == IN_CLOSE_WRITE && !strcmp( event->name, toFinish) ){
				strcpy(toFinishConfirm, toFinish);
				continue;
			}
// If the event is that of an attribute change on the file we noticed the behaviour:
// CREATE->WRITE->ATTRIBUTE_CHANGE
// Then we assume the file has finished its download sequence.
// So we can go ahead and check if it fits the conditions to get moved.
			if( event->mask != IN_ATTRIB || strcmp(event->name, toFinishConfirm) ) continue;
		}
// Reset variables.
		toFinish[0] = '\0';
		toFinishConfirm[0] = '\0';
		listNum = 0;
		printf("Finished File download detected: \"%s\"\n", event->name);
// Check if it ends with any relevant suffix.
		if( (endsWithResult(event->name, imagesSuffix, numImageSuffix, endString) && (listNum = 1)) ||
				(endsWith(event->name, pdfSufix) && (listNum = 2)) ){
			printf("File is a %s\n", endString);

			strcpy(full_path_old, download);
			strcat(full_path_old, event->name);

			printf("Old path: \"%s\"\n", full_path_old);

// Based on the suffix, choose the sub-directory.
			new_dir = (listNum == 1)? image_dir: (listNum == 2)? pdf_dir: "";

			strcpy(full_path_new, download);
			strcat(full_path_new, new_dir);
			strcat(full_path_new, event->name);

			printf("New path: \"%s\"\n", full_path_new);

// If the file already exists in subdirectory, change it's name to be unique (only goes up to 99).
			for(o = 1, isTens = 0; (fd = open(full_path_new, O_RDONLY)) > 0; o++ ){
				close(fd);
				if( o == 1 || (o == 10 && (isTens = 1)) ){
					fd = strlen(endString);
					strcpy(new_name, event->name);
					len = strlen(new_name); 
					if( len + 3 + isTens > 256 ) len = len - 3 - isTens;

					new_name[len - fd ] = '(';
					if( isTens ) new_name[len - fd + 1 ] = '0' + (o / 10);
					new_name[len - fd + 1 + isTens] = '0' + (o % 10);
					new_name[len - fd + 2 + isTens] = ')';
					new_name[len - fd + 3 + isTens] = '\0';
					strcat(new_name, endString);

					strcpy(full_path_new, download);
					strcat(full_path_new, new_dir);
					strcat(full_path_new, new_name);
				}
				else{
					if( isTens ) full_path_new[len - fd - 2 ] = '0' + (o / 10);
					full_path_new[len - fd - 1 + isTens] = '0' + (o % 10);
				}
			}

			if( new_name[0] != '\0' ) printf("Name of file changed to: %s\n", &full_path_new[strlen(download) + strlen(new_dir) + 1] );
			new_name[0] = '\0';
// Buffer of one second before moving file to not interrupt other processes
			sleep(1);
			rename(full_path_old, full_path_new);
			printf("Transferred file to /%s in Downloads\n", new_dir);
		}
	}
	perror("Error at the read of events, inotify instance probably killed");
	close(inotify_fd);
	exit(-1);
}
