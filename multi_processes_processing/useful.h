


#pragma once


int convert_decimal(unsigned char *buf,int a1,int a2,int a3,int a4); //convert 4 char to decimal
unsigned long convert_4byte(unsigned char *buf,int a1,int a2,int a3,int a4); //convert 4 char to one unsigned long
unsigned char* get_IDAT(unsigned char* buf);
int catpng(int num,char* path[]);