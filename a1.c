#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <limits.h>

//la testul de la findall nu merge sa imi deschida fisierul test_root
//dar la testul de la recursive listing merge sa deschida fisierul test_root (codul de deschidere este exact la fel)
//ar trebui sa mearga si nu stiu de ce 

struct section_header{
	char name[8];
	int type;
	int offset;
	int size;
};

struct header_total{
	char magic[3];
	int header_size;
	int version;
	int nb_of_sections;
	struct section_header section[10];
};

struct header_total header_extractor(char *path){
	struct header_total read_header;
	read_header.header_size = 0;
	read_header.version = 0;
	read_header.nb_of_sections = 0;
	
	int fd = -1;
	fd  = open(path, O_RDONLY);
	if ( fd == -1 ){
		printf("ERROR\nCould not open input file");
		exit(-1);
	}
	
	//read magic
	read(fd, read_header.magic, 2);
	read_header.magic[2] = '\0';
	
	
	//read header_size
	read(fd, &read_header.header_size, 2);
	
	//read version
	read(fd, &read_header.version, 2);
	
	//read nb of sections
	read(fd, &read_header.nb_of_sections, 1);
	if ( read_header.nb_of_sections < 7 || read_header.nb_of_sections > 10 ){
		return read_header;
	}
	
	for( int i = 0; i < read_header.nb_of_sections; i++){
		read(fd, read_header.section[i].name, 7);
		read_header.section[i].name[7] = '\0';
		
		read_header.section[i].type = 0;
		read(fd, &read_header.section[i].type, 2);
		
		read(fd, &read_header.section[i].offset, 4);
		
		read(fd, &read_header.section[i].size, 4);
		
	}
	
	close(fd);
	return read_header;
}

bool verif_header(struct header_total read_header){
	if ( strcmp(read_header.magic, "Ih") != 0 ){
		return false;
	}
	
	if ( read_header.version < 46 || read_header.version > 147 ){
		return false;
	}
	
	if ( read_header.nb_of_sections < 7 || read_header.nb_of_sections > 10 ){
		return false;
	}
	
	for( int i = 0; i < read_header.nb_of_sections; i++){
		if ( read_header.section[i].type != 66 && read_header.section[i].type != 97 && read_header.section[i].type != 23 && read_header.section[i].type != 28 && read_header.section[i].type != 55 && read_header.section[i].type != 57 && read_header.section[i].type != 54 ){
			return false;
		}
	}
	return true;
}

void findall(char *path){
	DIR *dir = NULL;
	struct dirent *entry = NULL;
	char full_path[512];
	struct stat statbuf;
	static bool first = true;
	
	dir = opendir(path);
    if(dir == NULL) {
       	printf("ERROR\ninvalid directory path\n");
       	exit(-1);
    }
    if( first ){
    	printf("SUCCESS\n");
    	first = false;
    }
    
    while((entry = readdir(dir)) != NULL) {
        if(strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            snprintf(full_path, 512, "%s/%s", path, entry->d_name);
            if(lstat(full_path, &statbuf) == 0) {
                if(S_ISDIR(statbuf.st_mode)) {
                    findall(full_path);
                }
                else{
                	struct header_total read_header  = header_extractor(full_path);
                	if ( verif_header(read_header) ){
                		bool gasit = false;
                		for( int i = 0; i < read_header.nb_of_sections; i++){
                			if ( read_header.section[i].type == 57 ){
                				gasit = true;
                			}
                		}
                		if ( gasit ){
                			printf("%s\n", full_path);
                		}
                	}
                }
            }
        }
    }
    closedir(dir);
}

void extract(char *path, int sect_nr, int line){
	struct header_total read_header  = header_extractor(path);
	
	if ( !verif_header(read_header) ) {
		printf("ERROR\ninvalid file");
		exit(-1);
	}
	
	if ( sect_nr < 1 || sect_nr > read_header.nb_of_sections ){
		printf("%d\n", read_header.nb_of_sections);
		printf("ERROR\ninvalid section");
		exit(-1);
	}
	int fd = -1;
	fd = open(path, O_RDONLY);
	lseek(fd, read_header.section[sect_nr-1].offset, SEEK_SET);
	int curr_line = 1;
	bool gasit = false;
	
	for ( int i = 0; i < read_header.section[sect_nr-1].size; i++)
	{
		char aux;
		read(fd, &aux, 1);
		
		if (aux == 0x0A){
			curr_line++;
		}
		
		if ( curr_line == line ){
			gasit = true;
		}
	}
	
	if ( !gasit ){
		printf("ERROR\ninvalid line");
		close(fd);
		exit(-1);
	}
	
	lseek(fd, read_header.section[sect_nr-1].offset, SEEK_SET);
	printf("SUCCESS\n");
	curr_line = 1;
	for ( int i = 0; i < read_header.section[sect_nr-1].size; i++)
	{
		char aux;
		read(fd, &aux, 1);
		
		if (aux == 0x0A){
			curr_line++;
		}
		else if (curr_line == line){
			printf("%c", aux);
		}
	}
	
	printf("\n");
	
	
	close(fd);
}

void parse(char* path){
	struct header_total read_header  = header_extractor(path);
	
	if ( strcmp(read_header.magic, "Ih") != 0 ){
		printf("ERROR\nwrong magic");
		exit(-1);
	}
	
	if ( read_header.version < 46 || read_header.version > 147 ){
		printf("ERROR\nwrong version");
		exit(-1);
	}
	
	if ( read_header.nb_of_sections < 7 || read_header.nb_of_sections > 10 ){
		printf("ERROR\nwrong sect_nr");
		exit(-1);
	}
	
	for( int i = 0; i < read_header.nb_of_sections; i++){
		if ( read_header.section[i].type != 66 && read_header.section[i].type != 97 && read_header.section[i].type != 23 && read_header.section[i].type != 28 && read_header.section[i].type != 55 && read_header.section[i].type != 57 && read_header.section[i].type != 54 ){
			printf("ERROR\nwrong sect_types");
			exit(-1);
		}
	}
	
	printf("SUCCESS\n");
	printf("version=%d\n", read_header.version);
	printf("nr_sections=%d\n", read_header.nb_of_sections);
	for ( int i = 0; i < read_header.nb_of_sections; i++){
		printf("section%d: %s %d %d\n", i+1, read_header.section[i].name, read_header.section[i].type, read_header.section[i].size);
	}
}


void list_iterative(char* path, int size_smaller, char* ends_with){
	DIR *dir = NULL;
	struct dirent *entry = NULL;
	char full_path[512];
	struct stat statbuf;
	
	dir = opendir(path);
    if(dir == NULL) {
       	printf("ERROR\ninvalid directory path\n");
       	exit(-1);
    }
    else{
    	printf("SUCCESS\n");
    }
    
    while((entry = readdir(dir)) != NULL) {
        if(strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            snprintf(full_path, 512, "%s/%s", path, entry->d_name);
            if(lstat(full_path, &statbuf) == 0) {
            	if(S_ISDIR(statbuf.st_mode)){
            		if( size_smaller == INT_MAX ){
            			if( ends_with[0] != '\0' ){
		        			int file_name_length = strlen(entry->d_name);
		        			int ends_with_length = strlen(ends_with);
		        			if( strcmp(entry->d_name + (file_name_length - ends_with_length), ends_with) == 0 ){
		        				printf("%s\n", full_path);
		        			}
		        		}
		        		else{
		        			printf("%s\n", full_path);
		        		}
            		}
            	}
            	else if( statbuf.st_size < size_smaller ){
            		if( ends_with[0] != '\0' ){
            			int file_name_length = strlen(entry->d_name);
            			int ends_with_length = strlen(ends_with);
            			if( strcmp(entry->d_name + (file_name_length - ends_with_length), ends_with) == 0 ){
            				printf("%s\n", full_path);
            			}
            		}
            		else{
            			printf("%s\n", full_path);
            		}
            	}
            }
        }
    }
    
    closedir(dir);
}


void list_rec(char* path, int size_smaller, char* ends_with){
	DIR *dir = NULL;
	struct dirent *entry = NULL;
	char full_path[512];
	struct stat statbuf;
	static bool first = true;
	
	dir = opendir(path);
    if(dir == NULL) {
       	printf("ERROR\ninvalid directory path\n");
       	exit(-1);
    }
    if( first ){
    	printf("SUCCESS\n");
    	first = false;
    }
    
    while((entry = readdir(dir)) != NULL) {
        if(strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            snprintf(full_path, 512, "%s/%s", path, entry->d_name);
            if(lstat(full_path, &statbuf) == 0) {
                if(S_ISDIR(statbuf.st_mode)) {
                    list_rec(full_path, size_smaller, ends_with);
                    if ( size_smaller == INT_MAX ){
		                if( ends_with[0] != '\0' ){
				    		int file_name_length = strlen(entry->d_name);
				    		int ends_with_length = strlen(ends_with);
				    		if( strcmp(entry->d_name + (file_name_length - ends_with_length), ends_with) == 0 ){
				    			printf("%s\n", full_path);
				    		}
				    	}
				    	else{
		                	printf("%s\n", full_path);
		                }
					}
                }
                else{
                	if(	statbuf.st_size < size_smaller ){
		        		if( ends_with[0] != '\0' ){
		        			int file_name_length = strlen(entry->d_name);
		        			int ends_with_length = strlen(ends_with);
		        			if( strcmp(entry->d_name + (file_name_length - ends_with_length), ends_with) == 0 ){
		        				printf("%s\n", full_path);
		        			}
		        		}
		        		else{
		        			printf("%s\n", full_path);
		        		}
		        	}
                }
            }
        }
    }
    closedir(dir);
}

int main(int argc, char **argv){
    if(argc >= 2){
        if(strcmp(argv[1], "variant") == 0){
            printf("87771\n");
        }
        if (strcmp(argv[1], "list") == 0){
        	bool rec = false;
        	int size_smaller = INT_MAX;
        	char ends_with[100];
        	ends_with[0] = '\0';
        	char path[256];
        	if(argc < 3){
        		fprintf(stderr, "Usage: %s %s [recursive] <filtering_options> path=<dir_path>\n", argv[0], argv[1]);
        		return -1;
        	}
        	
        	if(strstr(argv[argc-1], "path=") == NULL){
        		fprintf(stderr, "Usage: %s %s [recursive] <filtering_options> path=<dir_path>\n", argv[0], argv[1]);
        		return -1;
        	}
        	else{
        		strncpy(path, argv[argc-1]+5, strlen(argv[argc-1])-5);
        		path[strlen(argv[argc-1])-5] = '\0';
        	}
        	
        	
        	for ( int i = 2; i < argc - 1; i++){
        		if(strcmp(argv[i], "recursive") == 0){
        			rec = true;
        		}
        		
        		if(strstr(argv[i], "size_smaller=") == argv[i]){
		    		char size[100];
		    		strncpy(size, argv[i] + 13, strlen(argv[i])-13);
		    		size[strlen(argv[i])-13] = '\0';
		    		size_smaller = atoi(size);
		    	}
        		
        		if(strstr(argv[i], "name_ends_with=") == argv[i]){
		    		strncpy(ends_with, argv[i] + 15, strlen(argv[i])-15);
		    		ends_with[strlen(argv[i])-15] = '\0';
		    	}
        	}
        	
        	if ( rec ){
        		list_rec(path, size_smaller, ends_with);
        	}
        	else{
        		list_iterative(path, size_smaller, ends_with);
        	}
        	
        }
        if (strcmp(argv[1], "parse") == 0){
        	char path[256];
        	if(argc < 3){
        		fprintf(stderr, "Usage: %s %s path=<dir_path>\n", argv[0], argv[1]);
        		return -1;
        	}
        	if(strstr(argv[2], "path=") == argv[2]){
		    	strncpy(path, argv[2] + 5, strlen(argv[2])-5);
		    	path[strlen(argv[2])-5] = '\0';
		    }
		    parse(path);
        }
        
        if (strcmp(argv[1], "extract") == 0){
        	char path[256];
        	int sect_nr = 0;
        	int line = 0;
        	if ( argc < 6 ){
        		
        	}
        	//read path
        	if (strstr(argv[2], "path=") == argv[2] ){
        		strncpy(path, argv[2] + 5, strlen(argv[2])-5);
		    	path[strlen(argv[2])-5] = '\0';
        	}
        	else{
        		fprintf(stderr, "Usage: %s %s path=<file_path> section=<sect_nr> line=<line_nr>\n", argv[0], argv[1]);
        		return -1;
        	}
        	
		    
		    //read section
		    if (strstr(argv[3], "section=") == argv[3] ){
        		char aux[50];
		    	strncpy(aux, argv[3] + 8, strlen(argv[3])-8);
		    	aux[strlen(argv[3])-8] = '\0';
		    	sect_nr = atoi(aux);
        	}
		    else{
        		fprintf(stderr, "Usage: %s %s path=<file_path> section=<sect_nr> line=<line_nr>\n", argv[0], argv[1]);
        		return -1;
        	}
		    
		    //read line
		    if (strstr(argv[4], "line=") == argv[4] ){
		    	char aux[50];
        		strncpy(aux, argv[4] + 5, strlen(argv[4])-5);
		    	aux[strlen(argv[4])-5] = '\0';
		    	line = atoi(aux);
        	}
        	else{
        		fprintf(stderr, "Usage: %s %s path=<file_path> section=<sect_nr> line=<line_nr>\n", argv[0], argv[1]);
        		return -1;
        	}
		    extract(path, sect_nr, line);
        }
        
        if (strcmp(argv[1], "findall") == 0){
        	char path[256];
        	if ( argc != 3 ){
        		fprintf(stderr, "Usage: %s %s path=<dir_path>\n", argv[0], argv[1]);
        		return -1;
        	}
        	if ( strstr(argv[2], "path=") == argv[2] ){
        		strncpy(path, argv[2]+5, strlen(argv[2])-5);
        	}
        	findall(path);
        }
    }
    
    return 0;
}
