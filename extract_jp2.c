#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <ncurses.h>

#define _D_FILE_OFFSET_BITS  64
#define c_exif 0xe1
#define c_jfif 0xe0

unsigned long detect_jpg_size (unsigned char jpeg[])
{
//unsigned short jump;
unsigned long jpg_size;
jpg_size=4+((jpeg[4]*256)+jpeg[5]);

while (jpeg[jpg_size]==0xff) {
jpg_size+=2;
jpg_size+=((jpeg[jpg_size]*256)+jpeg[jpg_size+1]);
}
while ((jpeg[jpg_size]*256)+(jpeg[jpg_size+1])!=0xffd9) {
jpg_size++;
if (jpg_size>=10*1024*1024) break;
}


jpg_size++;
return(jpg_size);
}


int main (int argc,char * argv[])
{
FILE *outfile;
int infile;
unsigned int i,exif,jfif;
unsigned char *buffer;
char filename[128];
unsigned long actual_position,increment,size;
unsigned char bypass;
WINDOW *w_lba,*w_pic;
char s_position[50];
if (argc<2) { printf ("usage extract file [offset in sectors]\nextract jpeg files from a disk image\n"); exit (1);}

//printf ("%lu",sizeof( unsigned short));

buffer=(unsigned char *)malloc(10*1024*1024);
if (buffer==NULL) {printf("failed to allocate memory"); return(-1);}

if ( (infile=open(argv[1],O_RDONLY) )==-1) {

printf ("error opening file with errno:%d\n",errno);
exit (1);
}
initscr();
w_pic=newwin(20,80,0,0);
w_lba=newwin(4,80,21,0);
wmove (w_pic,0,0);
wmove (w_lba,0,0);

if (argc==2) actual_position=0; else actual_position=(atol(argv[2])*512);
printf ("Offset %lu\n",actual_position);
//52815992320; //must be 0 but for test purposes;
//printf ("seeking\n");
lseek(infile,actual_position,SEEK_SET);// test purpose
//printf ("seek\n");
while ((read (infile,buffer,512))!=0){
//printf ("reading\n");
if ((*buffer==0xff) && (*(buffer+1)==0xd8) && (*(buffer+2)==0xff)&&((buffer[3]==c_jfif)||(buffer[3]==c_exif))) { //buffer[3]==e1 select only exif
switch (buffer[3])
{
case c_jfif:jfif++;strcpy(filename,"tmp/jfif/abcde");break;
case c_exif:exif++;strcpy(filename,"tmp/exif/abcde");

}

if ((buffer[3]==c_jfif)||(buffer[3]==c_exif)) { //if (((*(buffer+3))&0xfe)==0xe0) {

read (infile,buffer+512,(10*1024*1024)-512); //possible depassement;
//printf ("read 5242880\n");


sprintf(&(filename[14]),"%lu",actual_position>>9);


//printf ("autodetect size \n");
size=detect_jpg_size(buffer)+1;
//printf ("returned %u \n",size);

if (size>((10*1024*1024)-1)) increment=512;
else increment=(size&(~0x1ff))+512;

if (size>50000){// mod 25-11
if ((outfile=fopen(filename,"w"))==NULL) break;
//printf("file created\n");
fwrite (buffer,sizeof(unsigned char),size,outfile);
//printf("file written\n");
fclose(outfile);
if (buffer[3]==c_exif) wprintw (w_pic,"*"); else wprintw(w_pic,"+");
}//mod 25-11
else  wprintw (w_pic,".");
wrefresh(w_pic);
}
//else increment=512; possible error here....
}
//else 
increment=512;

actual_position+=increment;
if (((actual_position>>9)&0xfffff)==0) {sprintf (s_position,"%ld\n",actual_position>>9); wmove (w_lba,0,0); wprintw (w_lba,s_position); wrefresh(w_lba);}
if ((actual_position&0x1ff)!=0) printf ("Buffer not aligned !");
if (lseek(infile,actual_position,SEEK_SET)==EOF) break;
//printf ("%ld\n",actual_position);
//printf("seek after jpeg written\n");
}
delwin(w_lba);
delwin(w_pic);
endwin();
close(infile);
free(buffer);
return (0);
}
