#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <curl/curl.h>
#include <sys/shm.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <unistd.h>
#include "useful.h"
#include "zutil.h"
#include "crc.h"

#define SHM_SIZE 1024 
#define SEM_PROC 1
#define NUM_SEMS 2
#define INFLATE_SIZE 9606

#define IMG_URL "http://ece252-1.uwaterloo.ca:2530/image?img=1&part=00"
#define DUM_URL "https://example.com/"
#define ECE252_HEADER "X-Ece252-Fragment: "
#define BUF_SIZE 10240 /* 1024*10 = 10K */

typedef struct recv_buf_flat {
    char *buf;       /* memory to hold a copy of received data */
    size_t size;     /* size of valid data in buf in bytes*/
    size_t max_size; /* max capacity of buf in bytes*/
    int seq;         /* >=0 sequence number extracted from http header */
    int used;
    int completed;
} RECV_BUF;

size_t header_cb_curl(char *p_recv, size_t size, size_t nmemb, void *userdata);
size_t write_cb_curl(char *p_recv, size_t size, size_t nmemb, void *p_userdata);
int recv_buf_init(RECV_BUF *ptr, size_t max_size);

int recv_buf_cleanup(RECV_BUF *ptr);
int write_file(const char *path, const void *in, size_t len);

int recv_buf_init(RECV_BUF *ptr, size_t max_size)
{
    void *p = NULL;
    
    if (ptr == NULL) {
        return 1;
    }

    p = malloc(max_size);
    if (p == NULL) {
	return 2;
    }
    
    ptr->buf = p;
    ptr->size = 0;
    ptr->max_size = max_size;
    ptr->seq = -1;              /* valid seq should be non-negative */
    return 0;
}

size_t header_cb_curl(char *p_recv, size_t size, size_t nmemb, void *userdata)
{
    int realsize = size * nmemb;
    RECV_BUF *p = userdata;
    
    if (realsize > strlen(ECE252_HEADER) &&
	strncmp(p_recv, ECE252_HEADER, strlen(ECE252_HEADER)) == 0) {

        /* extract img sequence number */
	p->seq = atoi(p_recv + strlen(ECE252_HEADER));

    }
    return realsize;
}


size_t write_cb_curl(char *p_recv, size_t size, size_t nmemb, void *p_userdata)
{
    size_t realsize = size * nmemb;
    RECV_BUF *p = (RECV_BUF *)p_userdata;
 
    if (p->size + realsize + 1 > p->max_size) {/* hope this rarely happens */ 
        fprintf(stderr, "User buffer is too small, abort...\n");
        abort();
    }

    memcpy(p->buf + p->size, p_recv, realsize); /*copy data from libcurl*/
    p->size += realsize;
    p->buf[p->size] = 0;

    return realsize;
}

int sizeof_shm_recv_buf(size_t nbytes)
{
    return (sizeof(RECV_BUF) + sizeof(char) * nbytes);
}


int shm_recv_buf_init(RECV_BUF *ptr, size_t nbytes)
{
    if ( ptr == NULL ) {
        return 1;
    }
    
    ptr->buf = (char *)ptr + sizeof(RECV_BUF);
    ptr->size = 0;
    ptr->max_size = nbytes;
    ptr->seq = -1;              /* valid seq should be non-negative */
    ptr->used= 0;
    ptr->completed = 0;
    
    return 0;
}

int recv_buf_cleanup(RECV_BUF *ptr)
{
    if (ptr == NULL) {
	return 1;
    }
    
    free(ptr->buf);
    ptr->size = 0;
    ptr->max_size = 0;
    ptr->used=0;
    return 0;
}

int write_file(const char *path, const void *in, size_t len)
{
    FILE *fp = NULL;

    if (path == NULL) {
        fprintf(stderr, "write_file: file name is null!\n");
        return -1;
    }

    if (in == NULL) {
        fprintf(stderr, "write_file: input data is null!\n");
        return -1;
    }

    fp = fopen(path, "wb");
    if (fp == NULL) {
        perror("fopen");
        return -2;
    }

    if (fwrite(in, 1, len, fp) != len) {
        fprintf(stderr, "write_file: imcomplete write!\n");
        return -3; 
    }
    return fclose(fp);
}
int B,P,C,X,N;
char haha[2];
int current_buf,current_seq;
unsigned long inf_size;
unsigned char* IDAT_data;
unsigned long IDAT_size;

int main( int argc, char** argv) {
    if(argc!=6){
        printf("please enter correct number of attributes");
        return 1;
    }
    B = atoi(argv[1]);
    P = atoi(argv[2]);
    C = atoi(argv[3]);
    X = atoi(argv[4]);
    N = atoi(argv[5]);

    CURL *curl_handle;
    CURLcode res;
    char url[256];
    RECV_BUF *p_shm_recv_buf[50];
    RECV_BUF *sample_png;
    int shmid[50];
    int sample_png_id;
    int shm_size = B*sizeof_shm_recv_buf(BUF_SIZE);
    //char fname[256];
    unsigned char* inflated_buf;
    int inflated_id;

    int int_id;
    int* int_track;
    int_id = shmget(IPC_PRIVATE, 4*sizeof(int), IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
    int_track= shmat(int_id, NULL, 0);
    int_track[0] = 0;
    int_track[1] = 0;

    double times[2];
//create sem
    sem_t *sems;
    
    int shmid_sems = shmget(IPC_PRIVATE, sizeof(sem_t) * 4, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);

    if (shmid_sems == -1) {
        perror("shmget");
        abort();
    }

    sems = shmat(shmid_sems, NULL, 0);
    if (sems == (void *) -1 ) {
        perror("shmat");
        abort();
    }
    sem_init(&sems[0], SEM_PROC, B); //used to track if there is empty buf
    sem_init(&sems[1], SEM_PROC, 0); //used to track how many buf is ready to proceed
    sem_init(&sems[2], SEM_PROC, 1); //used to restrict only 1 process can do work
    sem_init(&sems[3], SEM_PROC, 1); //used to restrict only 1 process can do work

    strcpy(url, IMG_URL);
    url[44] = *argv[5];

    for(int i=0;i<B;i++){
        shmid[i] = shmget(IPC_PRIVATE, shm_size, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
            if ( shmid == -1 ) {
                perror("shmget");
                abort();
        }
    }
    for(int i=0;i<B;i++){
        p_shm_recv_buf[i] = shmat(shmid[i], NULL, 0);
        shm_recv_buf_init(p_shm_recv_buf[i], BUF_SIZE);
    }

    sample_png_id = shmget(IPC_PRIVATE, sizeof_shm_recv_buf(BUF_SIZE), IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
    sample_png = shmat(sample_png_id, NULL, 0);
    shm_recv_buf_init(sample_png, BUF_SIZE);

    inflated_id = shmget(IPC_PRIVATE, 50*INFLATE_SIZE, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
    inflated_buf = shmat(inflated_id, NULL, 0);


    curl_global_init(CURL_GLOBAL_DEFAULT);
    struct timeval tv;

    if (gettimeofday(&tv, NULL) != 0) {
        perror("gettimeofday");
        abort();
    }
    times[0] = (tv.tv_sec) + tv.tv_usec/1000000.;

    for(int i=0;i<P;i++){
        url[14] = (i%3+1) + '0';
        switch(fork()) {
		    case -1: // error
			    perror("fork failed");
			    return 1;

		    case 0: // child int_track[0]<50
                while(int_track[0]<50){
                sem_wait(&sems[0]);
                sem_wait(&sems[2]);
                if(int_track[0]>=50){
                    sem_post(&sems[0]);
                    sem_post(&sems[2]);
                    exit(0);
                }
                current_seq = int_track[0];
                int_track[0]++;
                sprintf(haha,"%d",current_seq);
                url[51]=haha[0]; url[52]=haha[1]; 
                //printf(": URL is %s\n", url);

                for(int i=0;i<B;i++){
                    if(p_shm_recv_buf[i]->used==0){
                        current_buf = i;
                        p_shm_recv_buf[i]->used=1;
                        break;
                    }
                }
                sem_post(&sems[2]);

                curl_handle = curl_easy_init();

                if (curl_handle == NULL) {
                    fprintf(stderr, "curl_easy_init: returned NULL\n");
                    return 1;
                }

                /* specify URL to get */
                curl_easy_setopt(curl_handle, CURLOPT_URL, url);

                /* register write call back function to process received data */
                curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_cb_curl); 
                /* user defined data structure passed to the call back function */
                curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)p_shm_recv_buf[current_buf]);

                /* register header call back function to process received header data */
                curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, header_cb_curl); 
                /* user defined data structure passed to the call back function */
                curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, (void *)p_shm_recv_buf[current_buf]);

                /* some servers requires a user-agent field */
                curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
 
                res = curl_easy_perform(curl_handle);
                if( res != CURLE_OK) {
                    fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
                } else {
	               // printf("%lu bytes received in memory %p, seq=%d.\n",  \
                    p_shm_recv_buf[current_buf]->size, p_shm_recv_buf[current_buf]->buf,p_shm_recv_buf[current_buf]->seq);
        
                }
                if(sample_png->used==0){
                    sample_png->used=1;
                    memcpy(sample_png->buf,p_shm_recv_buf[current_buf]->buf,p_shm_recv_buf[current_buf]->size);
                    sample_png->size = p_shm_recv_buf[current_buf]->size;
                }
                p_shm_recv_buf[current_buf]->completed = 1;
                sem_post(&sems[1]);


                curl_easy_cleanup(curl_handle);
                }
			    exit(0);

		    default: { // parent
			    
		    }
	    }
    }
    for(int i=0;i<C;i++){
        switch (fork())
        {
        case -1: // error
			perror("fork failed");
			return 1;
        case 0: //child
            while(int_track[1]<50){
                sem_wait(&sems[1]);
                sem_wait(&sems[3]);
                if(int_track[1]>=50){
                    sem_post(&sems[1]);
                    sem_post(&sems[3]);
                    exit(0);
                }
                int_track[1]++;
                if(int_track[1]==50){
                    for(int j=0;j<C;j++){
                        sem_post(&sems[1]);
                    }
                }
                for(int i=0;i<B;i++){
                    if(p_shm_recv_buf[i]->completed==1){
                        current_buf = i;
                        current_seq = p_shm_recv_buf[i]->seq;
                        p_shm_recv_buf[i]->completed=0;
                        break;
                    }
                }
                sem_post(&sems[3]);
                //printf("consumer received:%d\n",p_shm_recv_buf[current_buf]->seq);
                usleep(1000*X);
                IDAT_data = get_IDAT((unsigned char*)p_shm_recv_buf[current_buf]->buf);
                IDAT_size = convert_4byte((unsigned char*)p_shm_recv_buf[current_buf]->buf,33,34,35,36);
                mem_inf(inflated_buf+current_seq*INFLATE_SIZE,&inf_size,IDAT_data,IDAT_size);
                free(IDAT_data); 

                shm_recv_buf_init(p_shm_recv_buf[current_buf], BUF_SIZE);
                sem_post(&sems[0]);

            }
            exit(0);
        
        default:{}
            
        }
    }
    for(int i=0;i<P+C;i++){
        wait(NULL);
    }
    //catpng
    unsigned char* buf1;
    unsigned long height;
    unsigned char* before_height;
    unsigned char* after_height_before_IDAT;
    unsigned char* IDAT_data;
    unsigned char* IDAT_type;
    unsigned long IDAT_size;
    unsigned char* IEND;
    unsigned long size;
    unsigned long width;


    size = sample_png->size;

    buf1 = (unsigned char*)sample_png->buf;
    before_height = (unsigned char*)malloc(20*sizeof(unsigned char));
    memcpy(before_height,buf1,20);

    height = convert_4byte(buf1,20,21,22,23);
    width = convert_4byte(buf1,16,17,18,19);
    after_height_before_IDAT = (unsigned char*)malloc(9*sizeof(unsigned char));
    memcpy(after_height_before_IDAT,buf1+24,9);
    IDAT_type = (unsigned char*)malloc(4*sizeof(unsigned char));
    memcpy(IDAT_type,buf1+37,4);
    IDAT_data = get_IDAT(buf1);
    IDAT_size = convert_4byte(buf1,33,34,35,36);

    IEND = (unsigned char*)malloc((size-45-IDAT_size)*sizeof(unsigned char));
    memcpy(IEND,buf1+IDAT_size+45,size-45-IDAT_size);

    unsigned char* inflated1 = (unsigned char*)malloc((height*(width*4+1))*sizeof(unsigned char));
    unsigned long new_IDAT_size;
    if(mem_inf(inflated1,&new_IDAT_size,IDAT_data,IDAT_size)!=0){
        printf("Fail to mem_inf!\n");
        return 1;
    }
    unsigned long current_size = size - IDAT_size-12;
    free(IDAT_data);IDAT_data = NULL;

    unsigned char* combine_deflated;

    combine_deflated = (unsigned char*)malloc((50*INFLATE_SIZE)*sizeof(unsigned char));
    unsigned long deflated_size;
    if(mem_def(combine_deflated,&deflated_size,inflated_buf,50*INFLATE_SIZE,-1)!=0){
            printf("Fail to mem_def!\n");
            return 1;
        }
    height = 300;
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

    if (gettimeofday(&tv, NULL) != 0) {
            perror("gettimeofday");
            abort();
        }
    times[1] = (tv.tv_sec) + tv.tv_usec/1000000.;
    printf("paster2 execution time: %.6lf seconds\n", times[1] - times[0]);
    
    curl_global_cleanup();
    for(int i=0;i<B;i++){
        shmctl(shmid[i], IPC_RMID, NULL);
    }
    shmctl(sample_png_id, IPC_RMID, NULL);
    shmctl(inflated_id, IPC_RMID, NULL);
    shmctl(shmid_sems, IPC_RMID, NULL);
    shmctl(int_id, IPC_RMID, NULL);
    for(int i=0;i<4;i++){
        sem_destroy(&sems[i]);
    }
    return 0;
}
