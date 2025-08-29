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
	int len, o, isTens;
	//int numDir = 1;
	int numImageSuffix = 5;

	char *download = "/home/vin/Downloads/";
	char *imagesSuffix[] = {".png", ".jpg", ".webp", ".jpeg", ".gif", 0};
	char *pdfSufix = ".pdf";
	char *new_dir = "images/";
	//char (directories[16])[256] = {"images/", ""};
	//char **suffixes[16] = {0};
	char endString[8];
	char new_name[256] = "";
	char full_path_old[1024] = "";
	char full_path_new[1024] = "";
	char buffer[sizeof(struct inotify_event) + 256];
	char toFinish[256] = "";
	char toFinishConfirm[256] = "";
	int toFinishCookie = 0;
	
	//suffixes[0] = &imagesSuffix;

	struct inotify_event *event;
	struct dirent dd = {0};
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
	inotify_fd = inotify_init();
	if(inotify_fd < 0){
		perror("Error in the creation of inotify instance");
		exit(1);
	}

	wd = inotify_add_watch(inotify_fd, download, IN_CREATE | IN_CLOSE_WRITE | IN_ATTRIB);
	if(wd < 0){
		perror("Error in the creation of watch item for directory Downloads");
		close(inotify_fd);
		exit(2);
	}

	signal(SIGINT, graceful);

	printf("Created inotify instance and checking Downloads folder, entering cycle...\n");

	while( read(inotify_fd, &buffer, sizeof(struct inotify_event) + 256) > 0 ){
		event = (struct inotify_event*) buffer;
		if( event->wd == wd ){
			if( endsWith(event->name, ".part") || endsWith(event->name, ".tmp") ) continue;
			if( event->mask == IN_CREATE ) {
				strcpy(toFinish, event->name);
				continue;
			}
			if( event->mask == IN_CLOSE_WRITE && !strcmp( event->name, toFinish) ){
				strcpy(toFinishConfirm, toFinish);
				continue;
			}
			if( event->mask != IN_ATTRIB || strcmp(event->name, toFinishConfirm) ) continue;
		}
		toFinish[0] = 0;
		toFinishConfirm[0] = 0;

		printf("Finished File download detected: \"%s\"\n", event->name);
		if( endsWithResult(event->name, imagesSuffix, numImageSuffix, endString) ){
			printf("File is a %s\n", endString);

			strcpy(full_path_old, download);
			strcat(full_path_old, event->name);

			printf("Old path: \"%s\"\n", full_path_old);

			strcpy(full_path_new, download);
			strcat(full_path_new, new_dir);
			strcat(full_path_new, event->name);

			printf("New path: \"%s\"\n", full_path_new);

			for(o = 1, isTens = 0; (fd = open(full_path_new, O_RDONLY)) > 0; o++ ){
				close(fd);

				strcpy(new_name, event->name);

				isTens = (o/10 > 0)? 1 : 0;
				fd = strlen(endString);
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
			if( new_name[0] != '\0' ) printf("Name of file changed to: %s\n", new_name);
			new_name[0] = '\0';
			sleep(1);
			rename(full_path_old, full_path_new);
			printf("Transferred file to /%s in Downloads\n", new_dir);
		}
	}
	perror("Errore nella read dei eventi, boh?");
	close(inotify_fd);
	exit(-1);
}
