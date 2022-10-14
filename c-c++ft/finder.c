#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(unix) || defined(__unix__) || defined(__unix)  
/* using POSIX API to deal with directories */
#include <dirent.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#elif defined (_WIN32)
//
#endif

int is_directory(const char*);
int list_files_in_dir(const char*);

int 
main(int argc, char** argv)
{
    if (argc != 2) {  /* if no or more than one directory is specified */
        printf("USAGE: finder (start folder name)\n");
        return EXIT_FAILURE;
    }
    if (chdir(argv[1]) != 0) {  /* change working dir to given one */
        perror("An error occured while changing working directory");
        return EXIT_FAILURE;
    }
    return list_files_in_dir(".");  /* walk through all contents of given dir */
}

int  /* not 'bool' for compatibility with older standards  */
is_directory(const char* path)
{
    struct stat statbuf;  /* POSIX struct for holding file info */
    
    if (lstat(path, &statbuf) != 0)  /* using lstat to avoid symlink dereferencing */
        return 0;
    
    if (S_ISDIR(statbuf.st_mode))  /* check if specified file is a directory */
        return 1;

    return 0;
}

int 
list_files_in_dir(const char* path)
{
    /* POSIX structures for dir info */
    DIR* dirp;
    struct dirent* direntp;

    size_t str_len;  /* aux variable for holding strings' lengths */
    char* subpath;  /* string that accumulates path to file */
    size_t subpath_len;
    int ret = EXIT_SUCCESS;  /* return value */
    
    /* accessing DIR handle for current directory */    
    if ((dirp = opendir(path)) == NULL) {
        if (errno == EACCES) {  /* if no permission to open directory, skipping it */
            printf("%s\n\n\tfinder: %s\n\n", path, strerror(errno));
            errno = 0;  /* considering that wasn't an error; continue traversing */
            return EXIT_SUCCESS;  
        }
        perror("An error occured while opening directory");
        return EXIT_FAILURE;
    }
    str_len = strlen(path);

    /* if specified path does not include trailing '/', reserve one byte for it */
    subpath_len = str_len + (path[str_len - 1] == '/' ? 0 : 1);
    if ((subpath = (char*) malloc(subpath_len + 1)) == NULL) {  /* 1 extra byte for '\0' */
        fprintf(stderr, "Error in malloc()\n");
        return EXIT_FAILURE;
    }
    strcpy(subpath, path);  /* store previous path */
    
    /* append '/', if it is not present */
    if (subpath_len == str_len + 1)
        strcat(subpath, "/");

    /* successively iterating entries in directory */
    while (errno == 0 && (direntp = readdir(dirp)) != NULL) {  
        if (strcmp(direntp->d_name, ".") == 0
            || strcmp(direntp->d_name, "..") == 0)  /* skipping '.' and '..' */
            continue;
        
        str_len = strlen(direntp->d_name) + 1;  /* directory entry name length + '\0' */
        
        /* allocating space in path for entry name  */
        if ((subpath = realloc(subpath, subpath_len + str_len)) == NULL) {
            fprintf(stderr, "Error in realloc()\n");
            ret = EXIT_FAILURE;
            break;
        }
        strcpy(subpath + subpath_len, direntp->d_name);  /* append that entry name to path */
        
        /* if given entry is a directory, recursively descending to it contents */
        if (is_directory(subpath)) {
            errno = 0;
            if (list_files_in_dir(subpath) != EXIT_SUCCESS) {
                ret = EXIT_FAILURE;
                errno = 0;  /* for being able to detect readdir() errors */
                break;
            }
        } else if (errno == 0) {  /* if it is not a directory, print it */
            printf("%s\n", subpath);
        } else if (errno == EACCES) {  /* if unable to open directory in is_directory() */
            printf("%s %s\n", subpath, strerror(errno));
            ret = EXIT_SUCCESS;
            errno = 0;
            break;
        } else {  /* other errors is_directory() */
            perror("An error occured while determining file type");
            ret = EXIT_FAILURE;
            errno = 0;  
            break;
        }
    }
    if (errno != 0)  /* if error occured in readdir() */
        perror("An error occured while reading directory");
        
    free(subpath);
    if (closedir(dirp) != 0)
        perror("An error occured while closing directory");
    
    return ret;
}
