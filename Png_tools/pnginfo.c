
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h> 
#include <stdio.h>   
#include <stdlib.h>  

#include "crc.h"
#include <string.h>  


int is_png(unsigned char *buf, int n); //check if png based on byte data
int convert_decimal(unsigned char *buf,int a1,int a2,int a3,int a4); //convert 4 char to decimal
unsigned long convert_4byte(unsigned char *buf,int a1,int a2,int a3,int a4); //convert 4 char to one unsigned long

int main(int argc, char *argv[]){
    if(argc != 2){
        printf("please enter file name!");
        return 1;
    } 
    unsigned char* buf;
    int size = 0;
    FILE *fd = fopen(argv[1],"rb");
    if(fd==NULL){
        printf("Fail to open file!\n");
        return 1;
    }
    //get the bytes number of file
    fseek(fd,0,SEEK_END);
    size = ftell(fd);
    rewind(fd);

    buf = (unsigned char*)malloc(size*sizeof(unsigned char));
    ssize_t rdf = fread(buf,1,size,fd);
    if(is_png(buf,8)==0){
        printf("%s",argv[1]);
        int width = convert_decimal(buf,16,17,18,19);
        int height = convert_decimal(buf,20,21,22,23);

        printf(": %i",width); printf(" x %i",height); printf("\n");

        //check IHDR chunk CRC
        int start=8; 
        int IHDR_long = convert_decimal(buf,start,start+1,start+2,start+3);
        unsigned char* bufIHDR = (unsigned char*)malloc((IHDR_long+4)*sizeof(unsigned char));
        memcpy(bufIHDR,buf+start+4,IHDR_long+4);
        unsigned long crcIHDR = crc(bufIHDR,IHDR_long+4);
        start= start+4+IHDR_long+4; 
        unsigned long crcIHDR_r= convert_4byte(buf,start,start+1,start+2,start+3);
        start += 4;
        if(crcIHDR != crcIHDR_r){
            printf("IHDR chunk CRC error: computed %x",crcIHDR);
            printf(", expected %x",crcIHDR_r);printf("\n");
        }
        free(bufIHDR); bufIHDR = NULL;

        //check IDAT chunk CRC
        int IDAT_long = convert_decimal(buf,start,start+1,start+2,start+3);
        unsigned char* bufIDAT = (unsigned char*)malloc((IDAT_long+4)*sizeof(unsigned char));
        memcpy(bufIDAT,buf+start+4,IDAT_long+4);
        unsigned long crcIDAT = crc(bufIDAT,IDAT_long+4);
        start= start+4+IDAT_long+4; 
        unsigned long crcIDAT_r= convert_4byte(buf,start,start+1,start+2,start+3);
        start += 4;
        if(crcIDAT != crcIDAT_r){
            printf("IDAT chunk CRC error: computed %x",crcIDAT);
            printf(", expected %x",crcIDAT_r);printf("\n");
        }
        free(bufIDAT); bufIDAT = NULL;

        //check IEND chunk CRC
        int IEND_long = convert_decimal(buf,start,start+1,start+2,start+3);
        unsigned char* bufIEND = (unsigned char*)malloc((IEND_long+4)*sizeof(unsigned char));
        memcpy(bufIEND,buf+start+4,IEND_long+4);
        unsigned long crcIEND = crc(bufIEND,IEND_long+4);
        start= start+4+IEND_long+4; 
        unsigned long crcIEND_r= convert_4byte(buf,start,start+1,start+2,start+3);
        start += 4;
        if(crcIEND != crcIEND_r){
            printf("IEND chunk CRC error: computed %x",crcIEND);
            printf(", expected %x",crcIEND_r); printf("\n");
        }
        free(bufIEND); bufIEND = NULL;

    }else{
        printf("%s",argv[1]); printf(": Not a PNG file\n");
    }
    
    free(buf);
    fclose(fd);
    return 0;
}
int convert_decimal(unsigned char *buf,int a1,int a2,int a3,int a4){
    return (buf[a1]<<24) | (buf[a2]<<16) | (buf[a3]<<8) | (buf[a4]);
}

int is_png(unsigned char *buf, int n){
    unsigned char png[8] = {0x89,0x50,0x4e,0x47,0x0d,0x0a,0x1a,0x0a};
    for(int i=0;i<n;i++){
        if(png[i]!= buf[i]){
            return 1;
        }
    }
    return 0;
}

unsigned long convert_4byte(unsigned char *buf,int a1,int a2,int a3,int a4){
    unsigned long ret;
    ret  = (unsigned long) buf[a1] << 24 | (unsigned long) buf[a2] << 16;
    ret |= (unsigned long) buf[a3] << 8 | buf[a4]; 
    return ret;
}