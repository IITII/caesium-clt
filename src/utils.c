#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <errno.h>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <ctype.h>
#include <dirent.h>

#ifdef __linux
	#include <linux/limits.h>
#endif


#include "utils.h"


int string_to_int(char* in_string) {
	long value = 0;
	char* endptr;
	errno = 0; //Error checking

	value = strtol(in_string, &endptr, 0); //Convert the string
	
	//Check errors
	if ((errno == ERANGE) || (errno != 0 && value == 0)) {
        perror("strtol");
        exit(-8);
    }

   if (endptr == in_string) {
        fprintf(stderr, "Parse error: No digits were found for -q option. Aborting.\n");
        exit(-7);
    }
	
	return value;
}

void print_help() {
	fprintf(stdout,
		"CCLT - Caesium Command Line Tools\n\n"
		"Usage: caesiumclt [OPTIONs] INPUT...\n"
		"Compress your pictures up to 90%% without visible quality loss.\n\n"

		"Options:\n"
			"\t-q\tset output file quality between [1-100], ignored for non-JPEGs\n"
			"\t-e\tkeeps EXIF info during compression\n"
			"\t-o\tcompress to custom folder\n"
			"\t-l\tuse lossless optimization\n"
			"\t-s\tscale to value, expressed as percentage (e.g. 20%%)\n"
			//TODO Remove this warning
			"\t-R\tif input is a folder, scan subfolders too [NOT IMPLEMENTED YET]\n"
			"\t-h\tdisplay this help and exit\n"
			"\t-v\toutput version information and exit\n\n");
	exit(0);
}

void print_progress(int current, int max, char* message) {
	fprintf(stdout, "\e[?25l");
	fprintf(stdout, "\r%s[%d%%]", message, current * 100 / max);
	if (current == max) {
		fprintf(stdout, "\e[?25h\n");
	}
}

//TODO Recheck
int mkpath(const char *pathname, mode_t mode) {

	//Need include in Linux, not on OSX
	char parent[PATH_MAX], *p;
	/* make a parent directory path */
	strncpy(parent, pathname, sizeof(parent));
	parent[sizeof(parent) - 1] = '\0';
	for(p = parent + strlen(parent); *p != '/' && p != parent; p--);
	*p = '\0';
	/* try make parent directory */
	if(p != parent && mkpath(parent, mode) != 0) {
		return -1;
	}
	/* make this one if parent has been made */
	if(mkdir(pathname, mode) == 0) {
		return 0;
	}
	/* if it already exists that is fine */
	if (errno == EEXIST) {
		return 0;
	}
	return -1;
}

char** scan_folder(char* dir, int depth) {
	int i = 0;
	DIR *dp;
    struct dirent *entry;
    struct stat statbuf;
    char** files = (char**) malloc(sizeof(char*));
    if ((dp = opendir(dir)) == NULL) {
        fprintf(stderr, "Cannot open %s. Aborting.\n", dir);
        exit(-14);
    }
    chdir(dir);
    while ((entry = readdir(dp)) != NULL) {
        lstat(entry->d_name, &statbuf);
        if (S_ISDIR(statbuf.st_mode)) {
            if (strcmp(".", entry->d_name) == 0 || strcmp("..", entry->d_name) == 0) {
                continue;
            }
            files = (char**) realloc(files, sizeof(files) + sizeof(char*));
            printf("QUI\n");
            files[i] = entry->d_name;
            i++;
            scan_folder(entry->d_name, depth+4);
        }
        else {
        	files = (char**) realloc(files, sizeof(files) + sizeof(char*));
        	printf("QUI\n");
            files[i] = entry->d_name;
            i++;
        }
    }

    chdir("..");
    closedir(dp);
	printf("SEG\n");
    return files;
}

void printdir(char *dir, int depth)
{
    DIR *dp;
    struct dirent *entry;
    struct stat statbuf;
    if((dp = opendir(dir)) == NULL) {
        fprintf(stderr,"cannot open directory: %s\n", dir);
        return;
    }
    chdir(dir);
    while((entry = readdir(dp)) != NULL) {
        lstat(entry->d_name,&statbuf);
        if(S_ISDIR(statbuf.st_mode)) {
            /* Found a directory, but ignore . and .. */
            if(strcmp(".",entry->d_name) == 0 ||
                strcmp("..",entry->d_name) == 0)
                continue;
            printf("%*s%s/\n",depth,"",entry->d_name);
            /* Recurse at a new indent level */
            printdir(entry->d_name,depth+4);
        }
        else printf("%*s%s\n",depth,"",entry->d_name);
    }
    chdir("..");
    closedir(dp);
}

enum image_type detect_image_type(char* path) {
	FILE* fp;
	unsigned char* type_buffer = valloc(2);

	fp = fopen(path, "r");

	if (fp == NULL) {
		fprintf(stderr, "Cannot open input file for type detection. Aborting.\n");
		exit(-14);
	}

	if (fread(type_buffer, 1, 2, fp) < 2) {
		fprintf(stderr, "Cannot read file type. Aborting.\n");
		exit(-15);
	}

	fclose(fp);

	if (((int) type_buffer[0] == 0xFF) && ((int) type_buffer[1] == 0xD8)) {
		free(type_buffer);
		return JPEG;
	} else if (((int) type_buffer[0] == 0x89) && ((int) type_buffer[1] == 0x50)) {
		free(type_buffer);
		return PNG;
	} else {
		fprintf(stderr, "Unsupported file type. Skipping.\n");
		return UNKN;
	}
}

