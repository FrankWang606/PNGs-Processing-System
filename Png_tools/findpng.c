//
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>   /* for printf().  man 3 printf */
#include <stdlib.h>  /* for exit().    man 3 exit   */
#include <string.h>  /* for strcat().  man strcat   */

int check_file_type(char* path);
char* str_withoutmodify(const char* s1,const char* s2);
void read_again(char* path,int *count);

int is_png(char* path); //check if png 

int main(int argc, char *argv[]) 
{
    DIR *p_dir;
    struct dirent *p_dirent;
    char str[64];
    int find_count =0;

    if (argc == 1) {
        fprintf(stderr, "Usage: %s <directory name>\n", argv[0]);
        exit(1);
    }
    char* current_dir = argv[1];
    

    if ((p_dir = opendir(argv[1])) == NULL) {
        sprintf(str, "opendir(%s)", argv[1]);
        perror(str);
        exit(2);
    }

    while ((p_dirent = readdir(p_dir)) != NULL) {
        char *str_path = p_dirent->d_name;  /* relative path name! */
        if(strcmp(str_path,"..")==0 || strcmp(str_path,".")==0){
            continue;
        }
        if (str_path == NULL) {
            fprintf(stderr,"Null pointer found!"); 
            exit(3);
        } else {
            
            char* new_path = str_withoutmodify(current_dir,"/");
            strcat(new_path,str_path);
            if(check_file_type(new_path)==1){
                if(is_png(new_path)==0){
                    printf("%s\n", new_path);
                    find_count=1;
                }
            }else if(check_file_type(new_path)==0){
                read_again(new_path,&find_count);
           }
   
            free(new_path);
            new_path=NULL;
        }
    }
    if(find_count==0){
        printf("findpng: No PNG file found\n");
    }
    if ( closedir(p_dir) != 0 ) {
        perror("closedir");
        exit(3);
    }
    
    return 0;
}

int check_file_type(char* path){
    int i=2;
    char *ptr;
    struct stat buf;

    if (lstat(path, &buf) < 0) {
        perror("lstat error");
    }
            

        if      (S_ISREG(buf.st_mode))  i = 1;
        else if (S_ISDIR(buf.st_mode))  i = 0;
        else if (S_ISCHR(buf.st_mode))  ptr = "character special";
        else if (S_ISBLK(buf.st_mode))  ptr = "block special";
        else if (S_ISFIFO(buf.st_mode)) ptr = "fifo";
#ifdef S_ISLNK
        else if (S_ISLNK(buf.st_mode))  ptr = "symbolic link";
#endif
#ifdef S_ISSOCK
        else if (S_ISSOCK(buf.st_mode)) ptr = "socket";
#endif
        else                            ptr = "**unknown mode**";
    
    return i;
}

char* str_withoutmodify(const char* s1,const char* s2){
    char *result = malloc(3*(strlen(s1) + strlen(s2) + 1)); 
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}

int is_png(char* path){
    FILE *fd = fopen(path,"rb");
    unsigned char* buf;
    buf = (unsigned char*)malloc(8*sizeof(unsigned char));
    ssize_t rdf = fread(buf,1,8,fd);
    unsigned char png[8] = {0x89,0x50,0x4e,0x47,0x0d,0x0a,0x1a,0x0a};
    for(int i=0;i<8;i++){
        if(png[i]!= buf[i]){
            free(buf);
            buf=NULL;
            return 1;
        }
    }
    free(buf);
    return 0;
}

void read_again(char* path,int *count){
    DIR *p_dir;
    struct dirent *p_dirent;
    char str[64];

    char* current_dir = path;
    

    if ((p_dir = opendir(path)) == NULL) {
        sprintf(str, "opendir(%s)", path);
        perror(str);
        exit(2);
    }

    while ((p_dirent = readdir(p_dir)) != NULL) {
        char *str_path = p_dirent->d_name;  /* relative path name! */
        if(strcmp(str_path,"..")==0 || strcmp(str_path,".")==0){
            continue;
        }
        if (str_path == NULL) {
            fprintf(stderr,"Null pointer found!"); 
            exit(3);
        } else {
            char* new_path = str_withoutmodify(path,"/");
            strcat(new_path,str_path);
            if(check_file_type(new_path)==1){
                if(is_png(new_path)==0){
                    printf("%s\n", new_path);
                    *count=1;
                }
            }else if(check_file_type(new_path)==0){
                read_again(new_path,count);
           }
            free(new_path);
            new_path=NULL;
        }
    }

    if ( closedir(p_dir) != 0 ) {
        perror("closedir");
        exit(3);
    }
    return;
}
