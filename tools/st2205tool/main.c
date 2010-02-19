/*
    Tool to interface with ST2205U-based picture frames
    Copyright (C) 2008 Jeroen Domburg <jeroen@spritesmods.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <asm/fcntl.h>
#include <gd.h>
#include <dirent.h>
#include <sys/mman.h>


/*
Two routines to allocate/deallocate page-aligned memory, for use with the
O_DIRECT-opened files.
*/

void *malloc_aligned(long size) {
    int f;
    char *buff;
    f=open("/dev/zero",O_RDONLY);
    buff=mmap(0,size,PROT_READ|PROT_WRITE,MAP_PRIVATE,f,0);
    close(f);
    return buff;
}

void free_aligned(void *addr, long size) {
    munmap(addr,size);
}


/*
Checks if the device is a photo frame by reading the first 512 bytes and
comparing against the known string that's there
*/
int is_photoframe(int f) {
    int y,res;
    char id[]="SITRONIX CORP.";
    char *buff;
    buff=malloc_aligned(0x200);
    lseek(f,0x0,SEEK_SET);
    y=read(f,buff,0x200);
    buff[15]=0;
//    fprintf(stderr,"ID=%s\n",buff);
    res=strcmp(buff,id)==0?1:0;
    free_aligned(buff,0x200);
    return res;
}

/*
The interface works by writing bytes to the raw 'disk' at certain positions.
Commands go to offset 0x6200, data to be read from the device comes from 0xB000,
data to be written goes to 0x6600. Hacked firmware has an extra address,
0x4200: bytes written there will go straight to the LCD.
*/

#define POS_CMD 0x6200
#define POS_WDAT 0x6600
#define POS_RDAT 0xb000

int sendcmd(int f,int cmd, unsigned int arg1, unsigned int arg2, unsigned char arg3) {
    unsigned char *buff;
    buff=malloc_aligned(0x200);
    buff[0]=cmd;
    buff[1]=(arg1>>24)&0xff;
    buff[2]=(arg1>>16)&0xff;
    buff[3]=(arg1>>8)&0xff;
    buff[4]=(arg1>>0)&0xff;
    buff[5]=(arg2>>24)&0xff;
    buff[6]=(arg2>>16)&0xff;
    buff[7]=(arg2>>8)&0xff;
    buff[8]=(arg2>>0)&0xff;
    buff[9]=(arg3);
    lseek(f,POS_CMD,SEEK_SET);
    return write(f,buff,0x200);
}

int read_data(int f, char* buff, int len) {
    lseek(f,POS_RDAT,SEEK_SET);
    return read(f,buff,len);
}

int write_data(int f,char* buff, int len) {
    lseek(f,0x6600,SEEK_SET);
    return write(f,buff,len);
}

/*
Debugging routine to dump a buffer in a hexdump-like fashion.
*/
void dumpmem(unsigned char* mem, int len) {
    int x,y;
    for (x=0; x<len; x+=16) {
	printf("%04x: ",x);
	for (y=0; y<16; y++) {
	    if ((x+y)>len) {
		printf("   ");
	    } else {
		printf("%02hhx ",mem[x+y]);
	    }
	}
	printf("- ");
	for (y=0; y<16; y++) {
	    if ((x+y)<=len) {
		if (mem[x+y]<32 || mem[x+y]>127) {
		    printf(".");
		} else {
		    printf("%c",mem[x+y]);
		}
	    }
	}
	printf("\n");
    }
}

/*
This routine reads a png-file from disk and puts it into buff in a format
the LCD understands. (basically 16-bit rgb)
*/
int readpic(char* filename, char* buff) {
    FILE *in;
    gdImagePtr im;
    int x,y;
    int c,r,g,b;
    in=fopen(filename,"rb");
    if (in==NULL) return 0;
    //Should try other formats too
    im=gdImageCreateFromPng(in);
    fclose(in);
    if (im==NULL) return 0;
    for (y=0; y<128; y++) {
	for (x=0; x<128; x++) {
	    c = gdImageGetPixel(im, x, y);
	    if (gdImageTrueColor(im) ) {
		r=gdTrueColorGetRed(c);
		g=gdTrueColorGetGreen(c);
		b=gdTrueColorGetBlue(c);
	    } else {
		r=gdImageRed(im,c);
		g=gdImageGreen(im,c);
		b=gdImageBlue(im,c);
	    }
	    r>>=3;
	    g>>=2;
	    b>>=3;
	    c=(r<<11)+(g<<5)+b;
	    buff[x*2+y*256]=(c>>8);
	    buff[x*2+y*256+1]=(c&255);
	}
    }
    gdImageDestroy(im);
    return 1;
}

/*
Send a picture to the LCD. Buff should point to a large enough page-aligned
buffer. It's not allocated inside the routine for speed reasons.
*/
int sendpic(int f, char* file, char* buff) {
    int y;
    y=readpic(file,buff);
    if (!y) return 0;
    lseek(f,0x4200,SEEK_SET);
    write(f,buff,128*128*2);
    return 1;
}

#define M_UP	1
#define M_DMP	2
#define M_FUP	3
#define M_FDMP	4
#define M_MSG	5
#define M_LCD   6
#define M_RAWEXEC   7

int main(int argc, char** argv) {
    int f,o;
    unsigned int x,y;
    int mode=0;
    unsigned char *buff;

    if (argc<2) {
	printf("Usage:\n%s [-d|-u|-df|-uf|-m|-l] file [device]\n", argv[0]);
	printf(" -d: dump mem\n");
	printf(" -u: upload mem\n");
	printf(" -df: dump firmware\n");
	printf(" -uf: upload firmware\n");
	printf(" -m: set message (10 chars)\n");
	printf(" -l: send png- to framebuffer mem (hacked fw only)\n");
	printf(" -r: send raw 256 byte code block to $1000 mem and execute (JRK hacked fw only)\n");
	printf(" file: file to dump to or upload from\n");
	printf(" device: /dev/sdX (default: /dev/sda)\n");
	printf("-l accepts directories too, in which case it'll proceed to send every file in the\n");
	printf("directory to the LCD.\n");
	exit(0);
    }
    
    //check requested command
    if (strcmp(argv[1],"-u")==0) mode=M_UP;
    if (strcmp(argv[1],"-d")==0) mode=M_DMP;
    if (strcmp(argv[1],"-uf")==0) mode=M_FUP;
    if (strcmp(argv[1],"-df")==0) mode=M_FDMP;
    if (strcmp(argv[1],"-m")==0) mode=M_MSG;
    if (strcmp(argv[1],"-l")==0) mode=M_LCD;
    if (strcmp(argv[1],"-r")==0) mode=M_RAWEXEC;
    if (mode==0) {
	printf("Invalid command: %s\n",argv[1]);
	exit(1);
    }
    
    //open the device
    if (argc>=4) {
	f=open(argv[3],O_RDWR|O_DIRECT|O_SYNC);
    } else {
	f=open("/dev/sda",O_RDWR|O_DIRECT|O_SYNC);
    }

    //check if dev really is a photo-frame
    if (!is_photoframe(f)) {
	fprintf(stderr,"No photoframe found there.\n");
	exit(1);
    }

    //open file, if needed
    if (mode==M_DMP || mode==M_FDMP) {    
	o=open(argv[2],O_WRONLY|O_TRUNC|O_CREAT,0644);
    } else if (mode==M_UP || mode==M_FUP) {
	o=open(argv[2],O_RDONLY);
    }
    if (o<0 && mode!=M_MSG && mode!=M_LCD) {
	fprintf(stderr,"Error opening %s.\n",argv[2]);
	exit(1);
    }
    
    //Allocate buffer and send a command. Check the result as an extra caution 
    //against non-photoframe devices.
    buff=malloc_aligned(0x8000);
    sendcmd(f,1,0,0,0);
    read_data(f,buff,0x200);
    if (buff[0]!=8) {
	printf("Expected response 8 on cmd 1, got 0x%hhx!\n",buff[0]);
	exit(1);
    }

    if (mode==M_DMP) {
	//dump picture memory
	//get everything except the last 64K (wraps around to the firmware)
	//in 32K chunks.
	for (x=0; x<((2048-64)/32); x++) {
	    sendcmd(f,4,x,0x8000,0);
	    read_data(f,buff,0x8000);
	    write(o,buff,0x8000);
	    fprintf(stderr,".");
	}
	fprintf(stderr,"\n");
	printf("Dump: %u pages dumped.\n", x);
    } else if (mode==M_FDMP) {
	//Use a trick to get the 64K of firmware: if we request the data starting
	//at (2M-64K), the data gets read from a mirror of the flash, position 0.
	for (x=((2048-64)/32); x<(2048/32); x++) {
	    sendcmd(f,4,x,0x8000,0);
	    read_data(f,buff,0x8000);
	    write(o,buff,0x8000);
	    fprintf(stderr,".");
	}
	fprintf(stderr,"\n");
	printf("Firmware dumped.\n");
    } else if (mode==M_UP) {
	//send everything except the last 64K (wraps around to the firmware)
	//in 32K chunks.
	for (x=0; x<((2048-64)/32); x++) {
	    sendcmd(f,3,x,0x8000,0);
	    y=read(o,buff,0x8000);
	    write_data(f,buff,0x8000);
	    sendcmd(f,2,x,0x8000,0);
	    read_data(f,buff,0x200);
	    if (y!=0x8000) {
		printf("Premature file end.\n");
		x=9999;
	    }
	    fprintf(stderr,".");
	}
	fprintf(stderr,"\n");
	printf("Memory uploaded.\n");
    } else if (mode==M_FUP) {
	printf("Firmware update! If unsure, press ctrl-C NOW!\n");
	sleep(3);
	printf("Too late. Commencing firmware update...\n");
	for (x=0; x<2; x++) {
	    sendcmd(f,3,x|0x80000000,0x8000,0);
	    y=read(o,buff,0x8000);
	    write_data(f,buff,0x8000);
	    sendcmd(f,2|0x80000000,x,0x8000,0);
	    read_data(f,buff,0x200);
	    sendcmd(f,3,x|0x1f40,0x8000,0);
	    write_data(f,buff,0x8000);
	    if (y!=0x8000) {
		printf("Premature file end. Hope everything still works OK.\n");
		x=9999;
	    }
	    fprintf(stderr,".");
	}
	fprintf(stderr,"\n");
	printf("Firmware upgraded. Un- and replug USB connection to restart device.\n");
    } else if (mode==M_MSG) {
      //Debug-feature? A message consisting of 10 bytes can be written to the
      //lcd. No hacked firmware is necessary for this.
      sendcmd(f,9,0,0,0);
      strcpy(buff,argv[2]);
      write_data(f,buff,0x200);
      printf("Message written.\n");
    } else if (mode==M_LCD) {
      y=sendpic(f,argv[2],buff);
      if (!y) {
        //p'rhaps a dir?
        DIR *dir;
        struct dirent *dp;
        char fn[2048];
        dir=opendir(argv[2]);
        if (dir==NULL) {
          //Nope :/
          printf("Couldn't open %s.\n",argv[2]);
          exit(1);
        }
        while ((dp = readdir (dir)) != NULL) {
          strcpy(fn,argv[2]);
          strcat(fn,"/");
          strcat(fn,dp->d_name);
          sendpic(f,fn,buff);
        }
      }
    } else if (mode == M_RAWEXEC )
      {
        int a;
        FILE * fp = fopen(argv[2], "rb" );
        if (!fp)
          {
            perror("fopen:");
            exit(1);
          }
        unsigned char * buf=malloc_aligned(0x200);
        while(!feof(fp))
          {
            // fill buffer with RTS opcodes
            memset( buf, 0x60, 512 );
            int br = fread( buf, 1, 512, fp );
            if (br==0)
              break;
            printf("read %d packet\n", br);
            for(a=0;a<512;a++)
              printf("%02x ", buf[a]);
            printf("\n");
            lseek(f,0x4600,SEEK_SET);
            int ret = write(f,buf,512);
            if (ret != 512)
              {
                printf("ret=%d\n", ret );
                perror("write");
              }
            else
              printf("wrote 1*512 byte packet\n");
          }
        fclose( fp );
      }
    
    exit(0);
}

