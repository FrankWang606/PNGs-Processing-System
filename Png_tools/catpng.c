
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>   
#include <stdlib.h>  
#include <string.h>  
#include <arpa/inet.h>

#include "zutil.h"
#include "crc.h"

int convert_decimal(unsigned char *buf,int a1,int a2,int a3,int a4); //convert 4 char to decimal
unsigned long convert_4byte(unsigned char *buf,int a1,int a2,int a3,int a4); //convert 4 char to one unsigned long
unsigned char* get_IDAT(unsigned char* buf);

int main(int argc, char *argv[]){
    if(argc<3){
        printf("please enter at least 2 files name!");
        return 1;
    }
    unsigned char* buf1;
    unsigned long height;
    unsigned char* before_height;
    unsigned char* after_height_before_IDAT;
    unsigned char* IDAT_data;
    unsigned char* IDAT_type;
    unsigned long IDAT_size;
    unsigned char* IEND;
    unsigned long size;

    FILE *fd = fopen(argv[1],"rb");
    if(fd==NULL){
        printf("Fail to open file!\n");
        return 1;
    }
    //get the bytes number of file
    fseek(fd,0,SEEK_END);
    size = ftell(fd);
    rewind(fd);

    buf1 = (unsigned char*)malloc(size*sizeof(unsigned char));
    ssize_t rdf = fread(buf1,1,size,fd);
    before_height = (unsigned char*)malloc(20*sizeof(unsigned char));
    memcpy(before_height,buf1,20);

    height = convert_4byte(buf1,20,21,22,23);
    after_height_before_IDAT = (unsigned char*)malloc(9*sizeof(unsigned char));
    memcpy(after_height_before_IDAT,buf1+24,9);
    IDAT_type = (unsigned char*)malloc(4*sizeof(unsigned char));
    memcpy(IDAT_type,buf1+37,4);
    IDAT_data = get_IDAT(buf1);
    IDAT_size = convert_4byte(buf1,33,34,35,36);

    IEND = (unsigned char*)malloc((size-45-IDAT_size)*sizeof(unsigned char));
    memcpy(IEND,buf1+IDAT_size+45,size-45-IDAT_size);

    unsigned char* inflated1 = (unsigned char*)malloc((100*IDAT_size)*sizeof(unsigned char));
    unsigned long new_IDAT_size;
    if(mem_inf(inflated1,&new_IDAT_size,IDAT_data,IDAT_size)!=0){
        printf("Fail to mem_inf!\n");
        return 1;
    }
    unsigned long current_size = size - IDAT_size-12;
    free(IDAT_data);IDAT_data = NULL; free(buf1); buf1 = NULL;
    fclose(fd);

    unsigned char* inflated2; unsigned long new_IDAT_size2;
    unsigned char* combine_inflated;
    unsigned char* combine_deflated;
    unsigned long get_height;
    //cat png 1 by 1 from the second png file
    for(int i=2;i<argc;i++){
        FILE *fd = fopen(argv[i],"rb");
        if(fd==NULL){
            printf("Fail to open file!:%s\n",argv[i]);
            return 1;
        }
        fseek(fd,0,SEEK_END);
        size = ftell(fd);
        rewind(fd);
        buf1 = (unsigned char*)malloc(size*sizeof(unsigned char));
        rdf = fread(buf1,1,size,fd);
        get_height = convert_4byte(buf1,20,21,22,23);
        height += get_height;
        IDAT_size = convert_4byte(buf1,33,34,35,36);
        IDAT_data = get_IDAT(buf1);
        inflated2 = (unsigned char*)realloc(inflated2,(100*IDAT_size)*sizeof(unsigned char));
        if(mem_inf(inflated2,&new_IDAT_size2,IDAT_data,IDAT_size)!=0){
            printf("Fail to mem_inf!\n");
            return 1;
        }
        combine_inflated = (unsigned char*)realloc(combine_inflated,(new_IDAT_size + new_IDAT_size2)*sizeof(unsigned char));
        memcpy(combine_inflated,inflated1,new_IDAT_size);
        memcpy(combine_inflated+new_IDAT_size,inflated2,new_IDAT_size2);
        new_IDAT_size=new_IDAT_size+ new_IDAT_size2;
        free(buf1);buf1=NULL;
        inflated1 = (unsigned char*)realloc(inflated1,(new_IDAT_size)*sizeof(unsigned char));
        memcpy(inflated1,combine_inflated,new_IDAT_size);
        fclose(fd);
    }
    combine_inflated =(unsigned char*)realloc(combine_inflated,(new_IDAT_size)*sizeof(unsigned char));
    memcpy(combine_inflated,inflated1,new_IDAT_size);
    combine_deflated = (unsigned char*)malloc((new_IDAT_size)*sizeof(unsigned char));
    unsigned long deflated_size;
    if(mem_def(combine_deflated,&deflated_size,combine_inflated,new_IDAT_size,-1)!=0){
            printf("Fail to mem_def!\n");
            return 1;
        }
    free(combine_inflated);combine_inflated=NULL;
    unsigned char* output= (unsigned char*)malloc((current_size+12+deflated_size)*sizeof(unsigned char));
    memcpy(output,before_height,20); free(before_height);before_height=NULL;
    height = ntohl(height);
    memcpy(output+20,&height,sizeof(unsigned long));
    memcpy(output+24,after_height_before_IDAT,9); free(after_height_before_IDAT);after_height_before_IDAT = NULL;
    unsigned long nt_deflated_size = ntohl(deflated_size);
    memcpy(output+33,&nt_deflated_size,sizeof(unsigned long));
    unsigned long crc_calc;
    unsigned char* IDAT_type_plus_data = (unsigned char*)malloc((4+deflated_size)*sizeof(unsigned char));
    memcpy(IDAT_type_plus_data,IDAT_type,4); free(IDAT_type);IDAT_type=NULL;
    memcpy(IDAT_type_plus_data+4,combine_deflated,deflated_size); free(combine_deflated);combine_deflated =NULL;
    crc_calc = crc(IDAT_type_plus_data,4+deflated_size);
    memcpy(output+37,IDAT_type_plus_data,deflated_size+4); free(IDAT_type_plus_data);IDAT_type_plus_data=NULL;
    crc_calc = ntohl(crc_calc);
    memcpy(output+41+deflated_size,&crc_calc,sizeof(unsigned long));
    memcpy(output+45+deflated_size,IEND,12);free(IEND);IEND = NULL;
    free(inflated2);
    free(inflated1);
    //update IHDR chunk CRC
    unsigned long new_IHDRcrc;
    new_IHDRcrc = crc(output+12,17);
    new_IHDRcrc = ntohl(new_IHDRcrc);
    unsigned int convert_CRC = (unsigned int) new_IHDRcrc;
    memcpy(output+29,&convert_CRC,4);
    //write to all.png
    FILE *op = fopen("all.png","wb+");
    fwrite(output,1,57+deflated_size,op);
    fclose(op);

}

int convert_decimal(unsigned char *buf,int a1,int a2,int a3,int a4){
    return (buf[a1]<<24) | (buf[a2]<<16) | (buf[a3]<<8) | (buf[a4]);
}



unsigned long convert_4byte(unsigned char *buf,int a1,int a2,int a3,int a4){
    unsigned long ret;
    ret  = (unsigned long) buf[a1] << 24 | (unsigned long) buf[a2] << 16;
    ret |= (unsigned long) buf[a3] << 8 | buf[a4]; 
    return ret;
}

unsigned char* get_IDAT(unsigned char* buf){
    int IDAT_long = convert_decimal(buf,33,34,35,36);
    unsigned char* bufIDAT = (unsigned char*)malloc((IDAT_long)*sizeof(unsigned char));
    memcpy(bufIDAT,buf+41,IDAT_long);
    return bufIDAT; 
}