//Arduino Picture Frame
//displays bitmaps (.BMP, 1,2,4,8,24 or 32 bit) from / on micro SD card to screen
//with long/short file name (some LFNs>100 characters not supported, and won't fit anyway)
//delay between pictures configurable

#include "XC4630d.c"
#include <SPI.h>
#include <SD.h>
#define MAXLFNCHAIN 8

File root,f;
//core bitmap parameters read from file
unsigned int bmtype,bmdataptr;                            //from header
unsigned int bmhdrsize,bmwidth,bmheight,bmbpp,bmpltsize;  //from DIB Header   
unsigned int bmbpl;                                       //bytes per line- derived
unsigned int bmplt[256];                                  //palette- stored encoded for LCD
int picdelay=5;                                           //delay between images
byte showname=1;                                          //flag to show filename or not

void setup(){
  SD.begin(10);                 //SS is D10 on XC4630
  root = SD.open("/");          //look in root folder
  XC4630_init();
  XC4630_rotate(4);
  XC4630_clear(BLACK);          //Blank screen
}

void loop(){
  int i,ystart,xend,u,v;
  f=root.openNextFile();
  if(!f){root.rewindDirectory();} //run out of files
  if(isbmp(f.name())){            //check if .BMP
    getbmpparms();
    if((bmtype==19778)&&(bmwidth>0)&&(bmheight>0)&&(bmbpp>0)){     //validate bitmap
      u=160-bmwidth/2;                                          //centre image
      v=120-bmheight/2;
      if(u<0){u=0;}
      if(v<0){v=0;}
      if(bmbpp<9){
        bmloadplt();                                    //load palette if palettized
        drawbmpal(u,v);
      }else{
        drawbmtrue(u,v);
      }
    }
    if(showname){printlfn();}
    delay(1000L*picdelay);        //wait between
    XC4630_clear(BLACK);          //Blank screen
  }
  f.close();
}

void bmloadplt(){
  byte r,g,b;  
  if(bmpltsize==0){
    bmpltsize=1<<bmbpp;           //load default palette size
  }
  f.seek(54);                     //palette position in type 0x28 bitmaps
  for(int i=0;i<bmpltsize;i++){
    b=f.read();
    g=f.read();
    r=f.read();
    f.read();    //dummy byte
    bmplt[i]=((r>>3)<<11)|((g>>2)<<5)|(b>>3);
  }
}

void drawbmpal(int u,int v){                        //draw palettized bitmap at (u,v)
  byte bmbitmask;
  int i,ystart,xend,bmppb,p;
  unsigned long x,y;
  unsigned int c,d;
  bmbpl=((bmbpp*bmwidth+31)/32)*4;                  //bytes per line
  bmppb=8/bmbpp;                                    //pixels/byte
  bmbitmask=((1<<bmbpp)-1);                         //mask for each pixel
  ystart=0;
  xend=bmwidth;
  if(bmheight>240){ystart=bmheight-240;}            //don't draw if it's outside screen
  if(xend>319){xend=319;}
  for(y=ystart;y<bmheight;y++){                     //invert in calculation (y=0 is bottom)
    f.seek(bmdataptr+y*bmbpl);                      //seek to start of line
    x=0;
    XC4630_areaset(u,v+bmheight-1-y,u+bmwidth-1,v+bmheight-1-y);    //set paint area
    XC4630CS0;
    p=0;
    while(x<xend){
      if(p<1){
        d=f.read();
        p=bmppb;
      }
      d=d<<bmbpp;
      c=bmplt[(bmbitmask&(d>>8))];
      XC4630_data(c>>8);
      XC4630_data(c);
      p--;
      x++;
    }
    XC4630CS1;
  }
}

void drawbmtrue(int u,int v){                        //draw true colour bitmap at (u,v) handles 24/32 not 16bpp yet
  int i,ystart,xend;
  unsigned long x,y;
  byte r,g,b;
  unsigned int c;
  bmbpl=((bmbpp*bmwidth+31)/32)*4;                  //bytes per line, due to 32bit chunks
  ystart=0;
  xend=bmwidth;
  if(bmheight>240){ystart=bmheight-240;}            //don't draw if it's outside screen
  if(xend>319){xend=319;}
  for(y=ystart;y<bmheight;y++){                     //invert in calculation (y=0 is bottom)
    f.seek(bmdataptr+y*bmbpl);                      //seek at start of line
    XC4630_areaset(u,v+bmheight-1-y,u+bmwidth-1,v+bmheight-1-y);    //set paint area
    XC4630CS0;
    for(x=0;x<xend;x++){
      b=f.read();
      g=f.read();
      r=f.read();
      if(bmbpp==32){f.read();}                      //dummy byte for 32bit
      c=((r>>3)<<11)|((g>>2)<<5)|(b>>3);
      XC4630_data(c>>8);
      XC4630_data(c);
    }
    XC4630CS1;
  }
}

void getbmpparms(){     //load into globals as ints-some parameters are 32 bit, but we can't handle this size anyway
  byte h[48];           //header is 54 bytes typically, but we don't need it all
  int i;
  f.seek(0);                        //set start of file
  for(i=0;i<48;i++){h[i]=f.read();} //read header
  bmtype=h[0]+(h[1]<<8);            //offset 0 'BM'
  bmdataptr=h[10]+(h[11]<<8);       //offset 0xA pointer to image data
  bmhdrsize=h[14]+(h[15]<<8);       //dib header size (0x28 is usual)  
  //files may vary here, if !=28, unsupported type, put default values
  bmwidth=0;
  bmheight=0;
  bmbpp=0;
  bmpltsize=0;
  if(bmhdrsize==0x28){
    bmwidth=h[18]+(h[19]<<8);       //width
    bmheight=h[22]+(h[23]<<8);      //height
    bmbpp=h[28]+(h[29]<<8);         //bits per pixel
    bmpltsize=h[46]+(h[47]<<8);     //palette size
  }
}

byte isbmp(char n[]){               //check if bmp extension
  int k;
  k=strlen(n);
  if(k<5){return 0;}    //name not long enough
  if(n[k-1]!='P'){return 0;}
  if(n[k-2]!='M'){return 0;}
  if(n[k-3]!='B'){return 0;}
  if(n[k-4]!='.'){return 0;}
  return 1;             //passes all tests  
}

void printlfn(){                     //assumes root is directory and f is file, does not support lower case LFN type
  File root;                         //needs to be opened separately to not interfere with scanning of folder
  int i=0,j=0;                       //counters
  int n=-1;                          //directory entry of file
  char csum=0;                       //checksum
  char fatfn[12]="           ";      //FAT version of filename ie 'file.ext' > 'file    ext'
  byte offset[13]={1,3,5,7,9,14,16,18,20,22,24,28,30};    //offsets of chars in dir entry
  char lfn[13*MAXLFNCHAIN+1]="";     //to store lfn
  char direntry[32];                 //to store directory entry
  root = SD.open("/");
  for(j=0;j<strlen(f.name());j++){   //convert to FAT version of filename
    if(f.name()[j]=='.'){
      i=8;                           //jump to extension
    }else{
      fatfn[i]=f.name()[j];          //copy
      i++;
    }
  }
  for(j=0;j<11;j++){csum=(((csum&1)<<7)|((csum&0xFE)>>1))+fatfn[j];}    //calculate checksum
  for(j=0;j<(root.size()/32);j++){                       //find file in directory entry (LFN is just before it)
    root.seek(j*32);
    int flag=1;                                        
    for(int i=0;i<11;i++){if(root.read()!=fatfn[i]){flag=0;}} //match flag, clear on mismatch
    if(flag){n=j;}
  }
  for(j=n-1;(j>n-MAXLFNCHAIN+1)&&(j>0);j--){            //read backwards from directory entry
    root.seek(j*32);
    for(int i=0;i<32;i++){direntry[i]=root.read();}     //read dir entry
    if(direntry[11]==0xF){                              //if LFN entry
      if(direntry[0]!=0xE5){                            //if not deleted
        if((direntry[0]&0x1F)<MAXLFNCHAIN){             //if is part of chain
          for(int i=0;i<13;i++){                        //copy bytes (could be Unicode, but Arduino can't handle)
            lfn[i+13*(n-1-j)]=direntry[offset[i]];
          }
          lfn[13*(n-j)]=0;
        }
      }
    }
    if(direntry[0]&0x40){                               //last one found
      j=-1;                                             //short circuit loop
      lfn[53]=0;                                        //truncate in case too long for screen
      if(lfn[0]&&(csum==direntry[13])){                 //checksum ok and lfn found
        XC4630_charsa(160-strlen(lfn)*3,230,lfn,WHITE,BLACK);   //print LFN
      }else{
        XC4630_charsa(160-strlen(f.name())*3,230,f.name(),WHITE,BLACK);   //else print short name
      }
    }
  }
  root.close();
}

