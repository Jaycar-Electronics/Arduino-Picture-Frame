#include <Arduino.h>
//Select one of the following models:
//v1 was first stock (all stock before Feb 2017 is V1)
//v2 arrived Feb 2017 (After Feb 2017, could be either)
#define XC4630_v1
//#define XC4630_v2
//basic 'library' for Jaycar XC4630 240x320 touchscreen display (v2 boards)- still needs mods for set orientation- not reg 0x03 (doesn't affect coordinates)
//include this in your sketch folder, and reference with #include "XC4630a.c"
//microSD card slot can be accessed via Arduino's SD card library (CS=10)
//supports: Mega, Leonaro, Uno with hardware optimizations (Uno fastest)
//supports: all other R3 layout boards via digitalWrite (ie slow)
//colours are 16 bit (binary RRRRRGGGGGGBBBBB)
//functions:
// XC4630_init() - sets up pins/registers on display, sets orientation=1
// XC4630_clear(c) - sets all pixels to colour c (can use names below)
// XC4630_rotate(n) -  sets orientation (top) 1,2,3 or 4
// typical setup code might be:
// XC4630_init();XC4630_clear(BLACK);XC4630_rotate(2) // set up display and make it black, landscape
//
// XC4630_char(x,y,c,f,b) - draw character c with lop left at (x,y), with f foreground and b background colour
// XC4630_chara(x,y,c,f,b) - draw character array c with lop left at (x,y), with f foreground and b background colour
// XC4630_charx(x,y,c,f,b,s) -  scaled version of XC4630_char (s=1 => 6wide by 8 high)
// XC4630_charxa(x,y,c,f,b,s) -  scaled version of XC4630_chara
// XC4630_chars(x,y,c,f,b,s) -  small version of XC4630_char
// XC4630_charsa(x,y,c,f,b,s) -  small version of XC4630_chara
// XC4630_areaset(x1,y1,x2,y2,c) - draw solid rectangular box with colour c
// XC4630_box(x1,y1,x2,y2) - solid box from (x1,y1) to (x2,y2)
// XC4630_tbox(x1,y1,x2,y2,c,f,b,s) - textbox from (x1,y1) to (x2,y2) c=text in f foreground/b background with s scale factor
// XC4630_hline(x1,y,x2,c) -  draw horizontal line from (x1,y) to (x2,y) in colour c
// XC4630_vline(x,y1,y2,c) -  draw vertical line from (x,y1) to (x,y2) in colour c
// XC4630_line(x1,y1,x2,y2,c) -  draw arbitrary line from (x1,y1) to (x2,y2) in colour c
// XC4630_point(x,y,c) - draw a single point at (x,y) in colour c
// XC4630_fcircle(x,y,r,c) - draw a filled circle centred on (x,y) with radius r, colour c
// XC4630_circle(x,y,r,c) - draw a hollow circle centred on (x,y) with radius r, colour c
// XC4630_triangle(x1,y1,x2,y2,x3,y3,c) - draw filled triangle between the three points in colour c
// hollow triangles and rectangles can be done by multiple lines
// XC4630_touchx() returns pixel calibrated touch data, <0 means no touch
// XC4630_touchy() returns pixel calibrated touch data, <0 means no touch
// XC4630_istouch(x1,y1,x2,y2) is area in (x1,y1) to (x2,y2) being touched?
// XC4630_image(x,y,const byte img[]) draw image img at x,y format is byte0=width,byte1=height, followed by width*height 2 byte pairs of raw data)
// XC4630_images(x,y,const byte img[],s) draw image img scaled by s at x,y (image data is byte width, byte height followed by raw 16bit data, in PROGMEM)
// XC4630_mcimage(x,y,const byte img[],f,b)  draw monochrome image with f foreground and b background
// XC4630_imaget(x,y,const byte img[],t) as for XC4630_image, except colour t is drawn transparent
// XC4630_imagewidth(const byte img[])     get image width of img
// XC4630_imageheight(const byte img[])     get image height of img

// Low level functions:
// XC4630_touchrawx() returns raw x touch data (>~900 means no touch)
// XC4630_touchrawy() returns raw y touch data (>~900 means no touch)
// XC4630_areaset(x1,y1,x2,y2) -  set rectangular painting area, only used for raw drawing in other functions
// XC4630_data(d) and XC4630_command - low level functions

//global variables to keep track of orientation etc, can be accessed in sketch
int XC4630_width,XC4630_height,XC4630_orientation;

//some colours with nice names
#define BLACK 0x0000
#define BLUE 0x001F
#define RED 0xF800
#define GREEN 0x07E0
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0  
#define WHITE 0xFFFF
#define GREY 0x8410
//unit colours - multiply by 0-31
#define RED_1 0x0800
#define GREEN_1 0x0040
#define BLUE_1 0x0001

//touch calibration data=> raw values correspond to orientation 1
#define XC4630_TOUCHX0 920
#define XC4630_TOUCHY0 950
#define XC4630_TOUCHX1 170
#define XC4630_TOUCHY1 200

//defines for LCD pins, dependent on board
//For Mega Board
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
#define XC4630SETUP   "MEGA"
#define XC4630RSTOP   DDRF|= 16
#define XC4630RST0    PORTF&= 239
#define XC4630RST1    PORTF|= 16
#define XC4630CSOP    DDRF|=8
#define XC4630CS0     PORTF&=247
#define XC4630CS1     PORTF|=8
#define XC4630RSOP    DDRF|=4
#define XC4630RS0     PORTF&=251
#define XC4630RS1     PORTF|=4
#define XC4630WROP    DDRF|=2
#define XC4630WR0     PORTF&=253
#define XC4630WR1     PORTF|=2
#define XC4630RDOP    DDRF|=1
#define XC4630RD0     PORTF&=254
#define XC4630RD1     PORTF|=1
#define XC4630dataOP  DDRE=DDRE|56;DDRG=DDRG|32;DDRH=DDRH|120;
#define XC4630data(d) PORTE=(PORTE&199)|((d&12)<<2)|((d&32)>>2);PORTG=(PORTG&223)|((d&16)<<1);PORTH=(PORTH&135)|((d&192)>>3)|((d&3)<<5);
#endif

//For Leonardo Board
#if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega16U4__)
#define XC4630SETUP   "LEO"
#define XC4630RSTOP   DDRF |= 2
#define XC4630RST0    PORTF &= 253
#define XC4630RST1    PORTF |= 2
#define XC4630CSOP    DDRF |= 16
#define XC4630CS0     PORTF &= 239
#define XC4630CS1     PORTF |= 16
#define XC4630RSOP    DDRF |= 32
#define XC4630RS0     PORTF &= 223
#define XC4630RS1     PORTF |= 32
#define XC4630WROP    DDRF |= 64
#define XC4630WR0     PORTF &= 191
#define XC4630WR1     PORTF |= 64
#define XC4630RDOP    DDRF |= 128
#define XC4630RD0     PORTF &= 127
#define XC4630RD1     PORTF |= 128
#define XC4630dataOP  DDRB=DDRB|48;DDRC=DDRC|64;DDRD=DDRD|147;DDRE=DDRE|64;
#define XC4630data(d) PORTB=(PORTB&207)|((d&3)<<4);PORTC=(PORTC&191)|((d&32)<<1);PORTD=(PORTD&108)|((d&4)>>1)|((d&8)>>3)|((d&16))|((d&64)<<1);PORTE=(PORTE&191)|((d&128)>>1);
#endif

//for Uno board
#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
#define XC4630SETUP   "UNO"
#define XC4630RSTOP   DDRC|=16
#define XC4630RST0    PORTC&=239
#define XC4630RST1    PORTC|=16
#define XC4630CSOP    DDRC|=8
#define XC4630CS0     PORTC&=247
#define XC4630CS1     PORTC|=8
#define XC4630RSOP    DDRC|=4
#define XC4630RS0     PORTC&=251
#define XC4630RS1     PORTC|=4
#define XC4630WROP    DDRC|=2
#define XC4630WR0     PORTC&=253
#define XC4630WR1     PORTC|=2
#define XC4630RDOP    DDRC|=1
#define XC4630RD0     PORTC&=254
#define XC4630RD1     PORTC|=1
#define XC4630dataOP  DDRD |= 252;DDRB |= 3;
#define XC4630data(d) PORTD=(PORTD&3)|(d&252); PORTB=(PORTB&252)|(d&3); 
#endif

//default that will work for any other board
#ifndef XC4630SETUP
#define XC4630SETUP   "DEFAULT"
#define XC4630RSTOP   pinMode(A4,OUTPUT)
#define XC4630RST0    digitalWrite(A4,LOW)
#define XC4630RST1    digitalWrite(A4,HIGH)
#define XC4630CSOP    pinMode(A3,OUTPUT)
#define XC4630CS0     digitalWrite(A3,LOW)
#define XC4630CS1     digitalWrite(A3,HIGH)
#define XC4630RSOP    pinMode(A2,OUTPUT)
#define XC4630RS0     digitalWrite(A2,LOW)
#define XC4630RS1     digitalWrite(A2,HIGH)
#define XC4630WROP    pinMode(A1,OUTPUT)
#define XC4630WR0     digitalWrite(A1,LOW)
#define XC4630WR1     digitalWrite(A1,HIGH)
#define XC4630RDOP    pinMode(A0,OUTPUT)
#define XC4630RD0     digitalWrite(A0,LOW)
#define XC4630RD1     digitalWrite(A0,HIGH)
#define XC4630dataOP  for(int i=2;i<10;i++){pinMode(i,OUTPUT);}
#define XC4630data(d) for(int i=2;i<10;i++){digitalWrite(i,d&(1<<(i%8)));}
#endif

#include <avr/pgmspace.h>
const unsigned int font1216[][12] PROGMEM ={
{0,0,0,0,0,0,0,0,0,0,0,0},           // 
{0,0,0,124,13311,13311,124,0,0,0,0,0},           //!
{0,0,60,60,0,0,60,60,0,0,0,0},           //"
{512,7696,8080,1008,638,7710,8080,1008,638,30,16,0},           //#
{0,1144,3324,3276,16383,16383,3276,4044,1928,0,0,0},           //$
{12288,14392,7224,3640,1792,896,448,14560,14448,14392,28,0},           //%
{0,7936,16312,12796,8646,14306,7742,7196,13824,8704,0,0},           //&
{0,0,0,39,63,31,0,0,0,0,0,0},           //'
{0,0,1008,4092,8190,14343,8193,8193,0,0,0,0},           //(
{0,0,8193,8193,14343,8190,4092,1008,0,0,0,0},           //)
{0,3224,3768,992,4088,4088,992,3768,3224,0,0,0},           //*
{0,384,384,384,4080,4080,384,384,384,0,0,0},           //+
{0,0,0,47104,63488,30720,0,0,0,0,0,0},           //,
{0,384,384,384,384,384,384,384,384,0,0,0},           //-
{0,0,0,14336,14336,14336,0,0,0,0,0,0},           //.
{6144,7168,3584,1792,896,448,224,112,56,28,14,0},           ///
{2040,8190,7686,13059,12675,12483,12387,12339,6174,8190,2040,0},           //0
{0,0,12300,12300,12302,16383,16383,12288,12288,12288,0,0},           //1
{12316,14366,15367,15875,14083,13187,12739,12515,12407,12350,12316,0},           //2
{3084,7182,14343,12483,12483,12483,12483,12483,14823,8062,3644,0},           //3
{960,992,880,824,796,782,775,16383,16383,768,768,0},           //4
{3135,7295,14435,12387,12387,12387,12387,12387,14563,8131,3971,0},           //5
{4032,8176,14840,12508,12494,12487,12483,12483,14787,8064,3840,0},           //6
{3,3,3,12291,15363,3843,963,243,63,15,3,0},           //7
{3840,8124,14846,12519,12483,12483,12483,12519,14846,8124,3840,0},           //8
{60,126,12519,12483,12483,14531,7363,3779,2023,1022,252,0},           //9
{0,0,0,7280,7280,7280,0,0,0,0,0,0},           //:
{0,0,0,40048,64624,31856,0,0,0,0,0,0},           //;
{0,192,480,1008,1848,3612,7182,14343,12291,0,0,0},           //<
{0,1632,1632,1632,1632,1632,1632,1632,1632,1632,0,0},           //=
{0,12291,14343,7182,3612,1848,1008,480,192,0,0,0},           //>
{28,30,7,3,14211,14275,227,119,62,28,0,0},           //?
{4088,8190,6151,13299,14331,13851,14331,14331,13831,1022,504,0},           //@
{14336,16128,2016,1788,1567,1567,1788,2016,16128,14336,0,0},           //A
{16383,16383,12483,12483,12483,12483,12519,14846,8124,3840,0,0},           //B
{1008,4092,7182,14343,12291,12291,12291,14343,7182,3084,0,0},           //C
{16383,16383,12291,12291,12291,12291,14343,7182,4092,1008,0,0},           //D
{16383,16383,12483,12483,12483,12483,12483,12483,12291,12291,0,0},           //E
{16383,16383,195,195,195,195,195,195,3,3,0,0},           //F
{1008,4092,7182,14343,12291,12483,12483,12483,16327,16326,0,0},           //G
{16383,16383,192,192,192,192,192,192,16383,16383,0,0},           //H
{0,0,12291,12291,16383,16383,12291,12291,0,0,0,0},           //I
{3584,7680,14336,12288,12288,12288,12288,14336,8191,2047,0,0},           //J
{16383,16383,192,480,1008,1848,3612,7182,14343,12291,0,0},           //K
{16383,16383,12288,12288,12288,12288,12288,12288,12288,12288,0,0},           //L
{16383,16383,30,120,480,480,120,30,16383,16383,0,0},           //M
{16383,16383,14,56,240,960,1792,7168,16383,16383,0,0},           //N
{1008,4092,7182,14343,12291,12291,14343,7182,4092,1008,0,0},           //O
{16383,16383,387,387,387,387,387,455,254,124,0,0},           //P
{1008,4092,7182,14343,12291,13827,15879,7182,16380,13296,0,0},           //Q
{16383,16383,387,387,899,1923,3971,7623,14590,12412,0,0},           //R
{3132,7294,14567,12483,12483,12483,12483,14791,8078,3852,0,0},           //S
{0,3,3,3,16383,16383,3,3,3,0,0,0},           //T
{2047,8191,14336,12288,12288,12288,12288,14336,8191,2047,0,0},           //U
{7,63,504,4032,15872,15872,4032,504,63,7,0,0},           //V
{16383,16383,7168,1536,896,896,1536,7168,16383,16383,0,0},           //W
{12291,15375,3612,816,480,480,816,3612,15375,12291,0,0},           //X
{3,15,60,240,16320,16320,240,60,15,3,0,0},           //Y
{12291,15363,15875,13059,12739,12515,12339,12319,12303,12291,0,0},           //Z
{0,0,16383,16383,12291,12291,12291,12291,0,0,0,0},           //[
{14,28,56,112,224,448,896,1792,3584,7168,6144,0},           //backslash
{0,0,12291,12291,12291,12291,16383,16383,0,0,0,0},           //]
{96,112,56,28,14,7,14,28,56,112,96,0},           //^
{49152,49152,49152,49152,49152,49152,49152,49152,49152,49152,49152,0},           //_
{0,0,0,0,62,126,78,0,0,0,0,0},           //`
{7168,15936,13152,13152,13152,13152,13152,13152,16352,16320,0,0},           //a
{16383,16383,12480,12384,12384,12384,12384,14560,8128,3968,0,0},           //b
{3968,8128,14560,12384,12384,12384,12384,12384,6336,2176,0,0},           //c
{3968,8128,14560,12384,12384,12384,12512,12480,16383,16383,0,0},           //d
{3968,8128,15328,13152,13152,13152,13152,13152,5056,384,0,0},           //e
{192,192,16380,16382,199,195,195,3,0,0,0,0},           //f
{896,51136,52960,52320,52320,52320,52320,58976,32736,16352,0,0},           //g
{16383,16383,192,96,96,96,224,16320,16256,0,0,0},           //h
{0,0,12288,12384,16364,16364,12288,12288,0,0,0,0},           //i
{0,0,24576,57344,49152,49248,65516,32748,0,0,0,0},           //j
{0,16383,16383,768,1920,4032,7392,14432,12288,0,0,0},           //k
{0,0,12288,12291,16383,16383,12288,12288,0,0,0,0},           //l
{16352,16320,224,224,16320,16320,224,224,16320,16256,0,0},           //m
{0,16352,16352,96,96,96,96,224,16320,16256,0,0},           //n
{3968,8128,14560,12384,12384,12384,12384,14560,8128,3968,0,0},           //o
{65504,65504,3168,6240,6240,6240,6240,7392,4032,1920,0,0},           //p
{1920,4032,7392,6240,6240,6240,6240,3168,65504,65504,0,0},           //q
{0,16352,16352,192,96,96,96,96,224,192,0,0},           //r
{4544,13280,13152,13152,13152,13152,16224,7744,0,0,0,0},           //s
{96,96,8190,16382,12384,12384,12384,12288,0,0,0,0},           //t
{4064,8160,14336,12288,12288,12288,12288,6144,16352,16352,0,0},           //u
{96,480,1920,7680,14336,14336,7680,1920,480,96,0,0},           //v
{2016,8160,14336,7168,4064,4064,7168,14336,8160,2016,0,0},           //w
{12384,14560,7616,3968,1792,3968,7616,14560,12384,0,0,0},           //x
{0,96,33248,59264,32256,7680,1920,480,96,0,0,0},           //y
{12384,14432,15456,13920,13152,12768,12512,12384,12320,0,0,0},           //z
{0,128,448,8188,16254,28679,24579,24579,24579,0,0,0},           //{
{0,0,0,0,16383,16383,0,0,0,0,0,0},           //|
{0,24579,24579,24579,28679,16254,8188,448,128,0,0,0},           //}
{16,24,12,4,12,24,16,24,12,4,0,0},           //~
{256,1792,7680,30720,30720,7680,1920,480,120,28,6,3}           //tick for check box
};


void XC4630_command(unsigned char d){   
  XC4630RS0;        //RS=0 => command
  XC4630data(d);    //data
  XC4630WR0;        //toggle WR
  XC4630WR1;
  XC4630RS1;
}

void XC4630_data(unsigned char d){
  XC4630data(d);    //data
  XC4630WR0;        //toggle WR
  XC4630WR1;
}

#ifdef XC4630_v1
void XC4630_areaset(int x1,int y1,int x2,int y2){
  if(x2<x1){int i=x1;x1=x2;x2=i;}   //sort x
  if(y2<y1){int i=y1;y1=y2;y2=i;}   //sort y
  XC4630CS0;
  XC4630_command(42);               //set x bounds  
  XC4630_data(x1>>8);
  XC4630_data(x1);
  XC4630_data(x2>>8);
  XC4630_data(x2);
  XC4630_command(43);               //set y bounds
  XC4630_data(y1>>8);
  XC4630_data(y1);
  XC4630_data(y2>>8);
  XC4630_data(y2);
  XC4630_command(44);               //drawing data to follow
  XC4630CS1;
}
void XC4630_init(){
  //set variable defaults
  XC4630_width=240;
  XC4630_height=320;
  XC4630_orientation=1;
  XC4630dataOP;
  XC4630RSTOP;
  XC4630CSOP;
  XC4630RSOP;
  XC4630WROP;
  XC4630RDOP;
  XC4630RST1;
  XC4630CS1;
  XC4630RS1;
  XC4630WR1;
  XC4630RD1;
  delay(5); 
  XC4630RST0;
  delay(15);
  XC4630RST1;
  delay(15);
  XC4630CS0;
  /*
  //These commands aren't in the ILI9341 manual, chip appears to be inexact ILI9341 clone
  XC4630_command(0xCB);  
  XC4630_data(0x39); 
  XC4630_data(0x2C); 
  XC4630_data(0x00); 
  XC4630_data(0x34); 
  XC4630_data(0x02); 
  XC4630_command(0xCF);  
  XC4630_data(0x00); 
  XC4630_data(0XC1); 
  XC4630_data(0X30); 
  XC4630_command(0xE8);  
  XC4630_data(0x85); 
  XC4630_data(0x00); 
  XC4630_data(0x78); 
  XC4630_command(0xEA);  
  XC4630_data(0x00); 
  XC4630_data(0x00); 
  XC4630_command(0xED);  
  XC4630_data(0x64); 
  XC4630_data(0x03); 
  XC4630_data(0X12); 
  XC4630_data(0X81); 
  XC4630_command(0xF7);  
  XC4630_data(0x20); 
  */
  XC4630_command(0xC0);           //power control
  XC4630_data(0x23);
  XC4630_command(0xC1);
  XC4630_data(0x10);
  XC4630_command(0xC5);           //VCOM control
  XC4630_data(0x3e);
  XC4630_data(0x28); 
  XC4630_command(0xC7);
  XC4630_data(0x86);  
  XC4630_command(0x36);           //Memory Access Control 
  XC4630_data(0x48);              //normal orientation=1
  XC4630_command(0x3A);           //Pixel format 0x55=16bpp, 0x66=18bpp
  XC4630_data(0x55);              
  XC4630_command(0xB1);           //Frame Control 
  XC4630_data(0x00);  
  XC4630_data(0x18); 
  XC4630_command(0xB6);           //Display Function Control 
  XC4630_data(0x08); 
  XC4630_data(0x82);
  XC4630_data(0x27);  
  XC4630_command(0x11);           //Sleep out
  delay(120); 
  XC4630_command(0x29);           //Display on
  //XC4630_command(0x2c);         //Drawing mode
  XC4630CS1;
}

void XC4630_rotate(int n){
  XC4630CS0;
  switch(n){
    case 1:
    XC4630_command(0x36);           //Memory Access Control 
    XC4630_data(0x48);              //1=top is 12 o'clock
    XC4630_width=240;
    XC4630_height=320;
    XC4630_orientation=1;
    break;
    case 2:
    XC4630_command(0x36);           //Memory Access Control 
    XC4630_data(0x28);              //2=top is 3 o'clock
    XC4630_width=320;
    XC4630_height=240;
    XC4630_orientation=2;
    break;
    case 3:
    XC4630_command(0x36);           //Memory Access Control 
    XC4630_data(0x88);              //3=top is 6 o'clock
    XC4630_width=240;
    XC4630_height=320;
    XC4630_orientation=3;
    break;
    case 4:
    XC4630_command(0x36);           //Memory Access Control 
    XC4630_data(0xE8);              //4=top is 9 o'clock
    XC4630_width=320;
    XC4630_height=240;
    XC4630_orientation=4;
    break;
  }
  XC4630CS1;
}
#endif

#ifdef XC4630_v2
void XC4630_areaset(int x1,int y1,int x2,int y2){
  if(x2<x1){int i=x1;x1=x2;x2=i;}   //sort x
  if(y2<y1){int i=y1;y1=y2;y2=i;}   //sort y
  int t1,t2,t3,t4;
  //for ST7781, need to internally remap x and y- pixel write direction can be done with reg 3
  t1=x1;
  t2=x2;
  t3=y1;
  t4=y2;
  switch(XC4630_orientation){
    case 1:
      break;
    case 2:
      x1=XC4630_height-1-t4;
      x2=XC4630_height-1-t3;
      y1=t1;
      y2=t2;
      break;
    case 3:
      x1=XC4630_width-1-t2;
      x2=XC4630_width-1-t1;
      y1=XC4630_height-1-t4;
      y2=XC4630_height-1-t3;    
      break;
    case 4:
      x1=t3;
      x2=t4;
      y1=XC4630_width-1-t2;
      y2=XC4630_width-1-t1;
    break;
  }
  XC4630CS0;

  XC4630_command(0);               //hi byte   
  XC4630_command(80);               //set x bounds  
  XC4630_data(x1>>8);
  XC4630_data(x1);
  XC4630_command(0);               //hi byte   
  XC4630_command(81);               //set x bounds  
  XC4630_data(x2>>8);
  XC4630_data(x2);
  XC4630_command(0);               //hi byte   
  XC4630_command(82);               //set y bounds
  XC4630_data(y1>>8);
  XC4630_data(y1);
  XC4630_command(0);               //hi byte   
  XC4630_command(83);               //set y bounds  
  XC4630_data(y2>>8);
  XC4630_data(y2);

  XC4630_command(0);               //hi byte   
  XC4630_command(32);               //set x pos
  XC4630_data(x1>>8);
  XC4630_data(x1);
  XC4630_command(0);               //hi byte   
  XC4630_command(33);               //set y pos
  XC4630_data(y1>>8);
  XC4630_data(y1);
  
  XC4630_command(0);               //hi byte   
  XC4630_command(34);               //drawing data to follow
  XC4630CS1;
}

void XC4630_init(){
  //set variable defaults
  XC4630_width=240;
  XC4630_height=320;
  XC4630_orientation=1;
  XC4630dataOP;
  XC4630RSTOP;
  XC4630CSOP;
  XC4630RSOP;
  XC4630WROP;
  XC4630RDOP;
  XC4630RST1;
  XC4630CS1;
  XC4630RS1;
  XC4630WR1;
  XC4630RD1;
  delay(50); 
  XC4630RST0;
  delay(150);
  XC4630RST1;
  delay(150);
  XC4630CS0;
  XC4630_command(0);XC4630_data(0);XC4630_data(1);
  delay(100); //at least 100ms
  XC4630_command(1);XC4630_data(1);XC4630_data(0);
  XC4630_command(16);XC4630_data(23);XC4630_data(144);
  XC4630_command(96);XC4630_data(167);XC4630_data(0);
  XC4630_command(97);XC4630_data(0);XC4630_data(1);
  XC4630_command(70);XC4630_data(0);XC4630_data(2);
  XC4630_command(19);XC4630_data(128);XC4630_data(16);
  XC4630_command(18);XC4630_data(128);XC4630_data(254);
  XC4630_command(2);XC4630_data(5);XC4630_data(0);
  XC4630_command(3);XC4630_data(16);XC4630_data(48);
  
  XC4630_command(48);XC4630_data(3);XC4630_data(3);
  XC4630_command(49);XC4630_data(3);XC4630_data(3);
  XC4630_command(50);XC4630_data(3);XC4630_data(3);
  XC4630_command(51);XC4630_data(3);XC4630_data(0);
  XC4630_command(52);XC4630_data(0);XC4630_data(3);
  XC4630_command(53);XC4630_data(3);XC4630_data(3);
  XC4630_command(54);XC4630_data(0);XC4630_data(20);
  XC4630_command(55);XC4630_data(3);XC4630_data(3);
  XC4630_command(56);XC4630_data(3);XC4630_data(3);
  XC4630_command(57);XC4630_data(3);XC4630_data(3);
  XC4630_command(58);XC4630_data(3);XC4630_data(0);
  XC4630_command(59);XC4630_data(0);XC4630_data(3);
  XC4630_command(60);XC4630_data(3);XC4630_data(3);
  XC4630_command(61);XC4630_data(20);XC4630_data(0);  
  XC4630_command(146);XC4630_data(2);XC4630_data(0);
  XC4630_command(147);XC4630_data(3);XC4630_data(3);
  XC4630_command(144);XC4630_data(8);XC4630_data(13);
  XC4630_command(1);XC4630_data(1);XC4630_data(0);//  XC4630_command(1);XC4630_data(1/0);XC4630_data(0);  //flip x
  XC4630_command(2);XC4630_data(7);XC4630_data(0);
  XC4630_command(3);XC4630_data(16);XC4630_data(48);
  XC4630_command(4);XC4630_data(0);XC4630_data(0);
  XC4630_command(8);XC4630_data(3);XC4630_data(2);
  XC4630_command(9);XC4630_data(0);XC4630_data(0);
  XC4630_command(10);XC4630_data(0);XC4630_data(0);
  XC4630_command(12);XC4630_data(0);XC4630_data(0);
  XC4630_command(13);XC4630_data(0);XC4630_data(0);
  XC4630_command(15);XC4630_data(0);XC4630_data(0);
  delay(120);
  XC4630_command(48);XC4630_data(3);XC4630_data(3);
  XC4630_command(49);XC4630_data(3);XC4630_data(3);
  XC4630_command(50);XC4630_data(3);XC4630_data(3);
  XC4630_command(51);XC4630_data(3);XC4630_data(0);
  XC4630_command(52);XC4630_data(0);XC4630_data(3);
  XC4630_command(53);XC4630_data(3);XC4630_data(3);
  XC4630_command(54);XC4630_data(0);XC4630_data(20);
  XC4630_command(55);XC4630_data(3);XC4630_data(3);
  XC4630_command(56);XC4630_data(3);XC4630_data(3);
  XC4630_command(57);XC4630_data(3);XC4630_data(3);
  XC4630_command(58);XC4630_data(3);XC4630_data(0);
  XC4630_command(59);XC4630_data(0);XC4630_data(3);
  XC4630_command(60);XC4630_data(3);XC4630_data(3);
  XC4630_command(61);XC4630_data(20);XC4630_data(0);
  XC4630_command(96);XC4630_data(167);XC4630_data(0);
  XC4630_command(97);XC4630_data(0);XC4630_data(1);
  XC4630_command(106);XC4630_data(0);XC4630_data(0);
  XC4630_command(144);XC4630_data(0);XC4630_data(51);
  XC4630_command(149);XC4630_data(1);XC4630_data(16);
  XC4630_command(255);XC4630_data(0);XC4630_data(1);
  XC4630_command(255);XC4630_data(0);XC4630_data(12);
  XC4630_command(255);XC4630_data(0);XC4630_data(0);
  delay(100);
  XC4630_command(3);XC4630_data(16);XC4630_data(48);    //48> 1,40>2 ,0>3, 24>4
  XC4630_command(7);XC4630_data(1);XC4630_data(115);
  delay(50);         
  XC4630CS1;
}

void XC4630_rotate(int n){
  XC4630CS0;
  switch(n){
    case 1:
    XC4630_command(3);XC4630_data(16);XC4630_data(48);    //48> 1,40>2 ,0>3, 24>4
    XC4630_width=240;
    XC4630_height=320;
    XC4630_orientation=1;
    break;
    case 2:
    XC4630_command(3);XC4630_data(16);XC4630_data(40);    //48> 1,40>2 ,0>3, 24>4
    XC4630_width=320;
    XC4630_height=240;
    XC4630_orientation=2;
    break;
    case 3:
    XC4630_command(3);XC4630_data(16);XC4630_data(0);    //48> 1,40>2 ,0>3, 24>4
    XC4630_width=240;
    XC4630_height=320;
    XC4630_orientation=3;
    break;
    case 4:
    XC4630_command(3);XC4630_data(16);XC4630_data(24);    //48> 1,40>2 ,0>3, 24>4
    XC4630_width=320;
    XC4630_height=240;
    XC4630_orientation=4;
    break;
  }
  XC4630CS1;
}
#endif

void XC4630_charx(int x, int y, char c, unsigned int f, unsigned int b,byte s){   //scaled font, 1=> 6x8 (half scaled)
  c=c-32;
  if(c<0){c=0;}                     //valid chars only
  if(c>96){c=0;}
  unsigned int dat[12];
  for(int i=0;i<12;i++){dat[i]=pgm_read_word(&font1216[c][i]);}   //preread data from progmem
  XC4630_areaset(x,y,x+6*s-1,y+8*s-1);    //set area
  XC4630CS0;
  for(byte v=0;v<8*s;v++){
    unsigned int q;
    q=1<<((v*2)/s);
    for(byte u=0;u<12;u++){
      unsigned int col=b;
      if(q&dat[u]){col=f;}      
      for(byte n=0;n<(((u&1)+s)/2);n++){
        XC4630_data(col>>8);
        XC4630_data(col);
      }
    }
  }
}

void XC4630_charxa(int x,int y,char *c, unsigned int f, unsigned int b,byte s){   //char array for scaled font
  while(*c){
    XC4630_charx(x,y,*c++,f,b,s);
    x=x+6*s;
    if(x>XC4630_width-6*s){x=0;y=y+8*s;}      //wrap around (will probably look ugly)
  }  
}

void XC4630_char(int x,int y,char c, unsigned int f, unsigned int b){
  c=c-32;
  if(c<0){c=0;}                     //valid chars only
  if(c>96){c=0;}
  XC4630_areaset(x,y,x+11,y+15);    //set area
  XC4630CS0;
  for(byte v=0;v<16;v++){
    for(byte u=0;u<12;u++){
      unsigned int d=pgm_read_word(&font1216[c][u]);
      if((1<<v)&d){XC4630_data(f>>8);XC4630_data(f);}
      else{XC4630_data(b>>8);XC4630_data(b);}
    }
  }
  XC4630CS1;
}

void XC4630_chars(int x,int y,char c,unsigned int f, unsigned int b){       //6x8 half scaled large font
  c=c-32;
  if(c<0){c=0;}                     //valid chars only
  if(c>96){c=0;}
  XC4630_areaset(x,y,x+5,y+7);    //set area
  XC4630CS0;
  for(byte v=0;v<16;v=v+2){
    for(byte u=1;u<12;u=u+2){
      unsigned int d=pgm_read_word(&font1216[c][u]);
      if((1<<v)&d){XC4630_data(f>>8);XC4630_data(f);}
      else{XC4630_data(b>>8);XC4630_data(b);}
    }
  }
  XC4630CS1;  
}

void XC4630_charsa(int x,int y,char *c, unsigned int f, unsigned int b){   //char array for small font
  while(*c){
    XC4630_chars(x,y,*c++,f,b);
    x=x+6;
    if(x>XC4630_width-6){x=0;y=y+8;}      //wrap around (will probably look ugly)
  }  
}

void XC4630_chara(int x,int y,char *c, unsigned int f, unsigned int b){
  while(*c){
    XC4630_char(x,y,*c++,f,b);
    x=x+12;
    if(x>XC4630_width-12){x=0;y=y+16;}      //wrap around (will probably look ugly)
  }  
}

void XC4630_box(int x1,int y1,int x2,int y2,unsigned int c){
  if(x2<x1){int i=x1;x1=x2;x2=i;}
  if(y2<y1){int i=y1;y1=y2;y2=i;}
  XC4630_areaset(x1,y1,x2,y2);
  x2++;
  y2++;
  XC4630CS0;
  for(int x=x1;x<x2;x++){
    for(int y=y1;y<y2;y++){
      XC4630_data(c>>8);
      XC4630_data(c);  
    }
  }
  XC4630CS1;
}

void XC4630_tbox(int x1,int y1, int x2,int y2, char *c, unsigned int f,unsigned int b,byte s){  //text box from x1,y1 to x2,y2 with scaled text centred
  int w,h,tx,ty,tw,th;
  w=x2-x1+1;
  h=y2-y1+1;
  tw=strlen(c)*6*s;   //text size
  th=8*s;  
  tx=x1+(w-tw)/2;    //text position
  ty=y1+(h-th)/2;
  XC4630_box(x1,y1,x2,ty-1,b);         //bg above text
  XC4630_box(x1,ty,tx-1,ty+th-1,b);    //left
  XC4630_charxa(tx,ty,c,f,b,s);        //text
  XC4630_box(tx+tw,ty,x2,ty+th-1,b);   //right
  XC4630_box(x1,ty+th,x2,y2,b);        //underneath
}

void XC4630_hline(int x1,int y1,int x2,unsigned int c){
  if(x2<x1){int i=x1;x1=x2;x2=i;}
  XC4630_areaset(x1,y1,x2,y1);
  x2++;
  XC4630CS0;
  for(int x=x1;x<x2;x++){
    XC4630_data(c>>8);
    XC4630_data(c);  
  }
  XC4630CS1;
}

void XC4630_vline(int x1,int y1,int y2,unsigned int c){
  if(y2<y1){int i=y1;y1=y2;y2=i;}
  XC4630_areaset(x1,y1,x1,y2);
  y2++;
  XC4630CS0;
  for(int y=y1;y<y2;y++){
    XC4630_data(c>>8);
    XC4630_data(c);  
  }
  XC4630CS1;
}

void XC4630_point(int x,int y, unsigned int c){  //a single point
  XC4630_areaset(x,y,x,y);
  XC4630CS0;
  XC4630_data(c>>8);
  XC4630_data(c);  
  XC4630CS1;  
}

void XC4630_fcircle(int xo,int yo,int r,unsigned int c){ //https://en.wikipedia.org/wiki/Midpoint_circle_algorithm
  int e=0;
  int x=r;
  int y=0;
  while(x>=y){
    XC4630_vline(xo-y,yo+x,yo-x,c);
    XC4630_vline(xo+y,yo+x,yo-x,c);
    e=e+1+2*y;
    y=y+1;
    if(2*(e-x)+1>0){
      y=y-1;
      XC4630_vline(xo-x,yo-y,yo+y,c);
      XC4630_vline(xo+x,yo-y,yo+y,c);
      y=y+1;
      x=x-1;
      e=e+1-2*x;
    }
  }  
}

void XC4630_circle(int xo,int yo,int r,unsigned int c){ //https://en.wikipedia.org/wiki/Midpoint_circle_algorithm
  int e=0;
  int x=r;
  int y=0;
  while(x>=y){
    XC4630_point(xo+x,yo+y,c);
    XC4630_point(xo-x,yo+y,c);
    XC4630_point(xo+x,yo-y,c);
    XC4630_point(xo-x,yo-y,c);
    XC4630_point(xo+y,yo+x,c);
    XC4630_point(xo-y,yo+x,c);
    XC4630_point(xo+y,yo-x,c);
    XC4630_point(xo-y,yo-x,c);
    e=e+1+2*y;
    y=y+1;
    if(2*(e-x)+1>0){
      x=x-1;
      e=e+1-2*x;
    }
  }
}

void XC4630_line(int x1,int y1,int x2,int y2, unsigned int c){
  int steps,stepsx,stepsy,xinc,yinc,x,y,d;
  stepsx=abs(x1-x2);
  stepsy=abs(y1-y2);
  steps=max(stepsx,stepsy)+1;   //if start and end are the same, there's still 1 point
  xinc=constrain(x2-x1,-1,1);
  yinc=constrain(y2-y1,-1,1);
  x=x1;
  y=y1;  
  if(stepsx>stepsy){
    d=stepsx/2;
    for(int i=0;i<steps;i++){
      XC4630_point(x,y,c);
      x=x+xinc;
      d=d+stepsy;
      if(d>stepsx){d=d-stepsx;y=y+yinc;}
    }
  }else{
    d=stepsy/2;
    for(int i=0;i<steps;i++){
      XC4630_point(x,y,c);
      y=y+yinc;
      d=d+stepsx;
      if(d>stepsy){d=d-stepsy;x=x+xinc;}
    } 
  }  
}

void XC4630_triangle(int x1,int y1,int x2,int y2,int x3,int y3,unsigned int c){ //custom Bresenham line algorithm
  //sort values, y1 at top
  if(y1>y2){int i=y1;y1=y2;y2=i;i=x1;x1=x2;x2=i;}
  if(y2>y3){int i=y2;y2=y3;y3=i;i=x2;x2=x3;x3=i;}
  if(y1>y2){int i=y1;y1=y2;y2=i;i=x1;x1=x2;x2=i;}
  if(y1==y3){XC4630_hline(min(x1,min(x2,x3)),y1,max(x1,max(x2,x3)),c);return;}
  if(y1!=y2){
    int dy1=y2-y1;
    int dy2=y3-y1;
    int dx1=x2-x1;
    int dx2=x3-x1;
    int xa,xb;
    xa=x1*dy1-(dx1);
    xb=x1*dy2-(dx2);
    for(int y=y1;y<y2;y++){
     xa=xa+dx1;
     xb=xb+dx2;
     XC4630_hline(xa/dy1,y,xb/dy2,c);
    }
   xb=xb+dx2;
   XC4630_hline(x2,y2,xb/dy2,c);        
  }
  if(y2!=y3){
    int dy1=y2-y3;
    int dy2=y1-y3;
    int dx1=x2-x3;
    int dx2=x1-x3;
    int xa,xb;
    xa=x3*dy1+(dx1);
    xb=x3*dy2+(dx2);
    for(int y=y3;y>y2;y--){
     xa=xa-dx1;
     xb=xb-dx2;
     XC4630_hline(xa/dy1,y,xb/dy2,c);
    }        
   if(y1==y2){
    xb=xb+dx2;
    XC4630_hline(x2,y2,xb/dy2,c);        
   }
  }
}

void XC4630_clear(unsigned int c){
  unsigned int i;
  XC4630_areaset(0,0,XC4630_width-1,XC4630_height-1);
  XC4630CS0;
  for(i=0;i<38400;i++){   //do 2 in each loop cos 76800>unsigned int
    XC4630_data(c>>8);
    XC4630_data(c);
    XC4630_data(c>>8);
    XC4630_data(c);
  }
  XC4630CS1;
}

int XC4630_touchrawx(){           //raw analog value
  int x;
  pinMode(8,OUTPUT);
  pinMode(A2,OUTPUT);
  digitalWrite(8,LOW);            //doesn't matter if this changes
  digitalWrite(A2,HIGH);          //this is normally high between screen commands
  pinMode(A3,INPUT_PULLUP);       //this is normally high between screen commands
  pinMode(9,INPUT_PULLUP);        //doesn't matter if this changes
  analogRead(A3);                 //discard first result after pinMode change
  delayMicroseconds(30);
  x=analogRead(A3);
  pinMode(A3,OUTPUT);
  digitalWrite(A3,HIGH);          //restore output state from above
  pinMode(9,OUTPUT);
  return(x);  
}

int XC4630_touchrawy(){           //raw analog value
  int y;
  pinMode(9,OUTPUT);
  pinMode(A3,OUTPUT);
  digitalWrite(9,LOW);            //doesn't matter if this changes
  digitalWrite(A3,HIGH);          //this is normally high between screen commands
  pinMode(A2,INPUT_PULLUP);       //this is normally high between screen commands
  pinMode(8,INPUT_PULLUP);        //doesn't matter if this changes
  analogRead(A2);                 //discard first result after pinMode change
  delayMicroseconds(30);
  y=analogRead(A2);
  pinMode(A2,OUTPUT);
  digitalWrite(A2,HIGH);          //restore output state from above
  pinMode(8,OUTPUT);
  return(y);  
}

int XC4630_touchx(){
  int x,xc;
  xc=-1;      //default in case of invalid orientation
  switch(XC4630_orientation){
    case 1:
    x=XC4630_touchrawx();
    xc=map(x,XC4630_TOUCHX0,XC4630_TOUCHX1,0,XC4630_width-1);
    break;
    case 2:
    x=XC4630_touchrawy();
    xc=map(x,XC4630_TOUCHY0,XC4630_TOUCHY1,0,XC4630_width-1);
    break;
    case 3:
    x=XC4630_touchrawx();
    xc=map(x,XC4630_TOUCHX1,XC4630_TOUCHX0,0,XC4630_width-1);
    break;
    case 4:
    x=XC4630_touchrawy();
    xc=map(x,XC4630_TOUCHY1,XC4630_TOUCHY0,0,XC4630_width-1);
    break;
  }
  if(xc>XC4630_width-1){xc=-1;}         //off screen
  return xc;
}

int XC4630_touchy(){
  int y,yc;
  yc=-1;      //default in case of invalid orientation
  switch(XC4630_orientation){
    case 1:
    y=XC4630_touchrawy();
    yc=map(y,XC4630_TOUCHY0,XC4630_TOUCHY1,0,XC4630_height-1);
    break;
    case 2:
    y=XC4630_touchrawx();
    yc=map(y,XC4630_TOUCHX1,XC4630_TOUCHX0,0,XC4630_height-1);
    break;
    case 3:
    y=XC4630_touchrawy();
    yc=map(y,XC4630_TOUCHY1,XC4630_TOUCHY0,0,XC4630_height-1);
    break;
    case 4:
    y=XC4630_touchrawx();
    yc=map(y,XC4630_TOUCHX0,XC4630_TOUCHX1,0,XC4630_height-1);
    break;
  }
  if(yc>XC4630_height-1){yc=-1;}         //off screen
  return yc;
}

int XC4630_istouch(int x1,int y1,int x2,int y2){    //touch occurring in box?
  int x,y;
  x=XC4630_touchx();
  if(x<x1){return 0;}
  if(x>x2){return 0;}
  y=XC4630_touchy();
  if(y<y1){return 0;}
  if(y>y2){return 0;}
  return 1;
}

void XC4630_mcimage(int x, int y, const byte img[],unsigned int fg, unsigned int bg){       //draws monochrome image with fg on bg colours, not byte aligned
  int w,h;
  w=pgm_read_byte(&img[0]);         //assume no image is bigger than 255 pixels
  h=pgm_read_byte(&img[1]);         //assume no image is bigger than 255 pixels
  XC4630_areaset(x,y,x+w-1,y+h-1);  //set area
  XC4630CS0;
  unsigned int s=w*h;               //number of pixels
  byte d,b;
  for(int i=0;i<s;i++){
    if((i&7)==0){
      d=pgm_read_byte(&img[(i>>3)+2]);    //read new byte and reset bit position
      b=128;
    }
    if(b&d){
      XC4630_data(fg>>8);
      XC4630_data(fg);
    }else{
      XC4630_data(bg>>8);
      XC4630_data(bg);
    }
    b=b>>1;
  }
}

void XC4630_image(int x,int y,const byte img[]){    //draws image img at x,y const=>progmem. first 2 bytes are width and height, rest is raw screen data, maybe do ram version...
  int w,h;
  w=pgm_read_byte(&img[0]);         //assume no image is bigger than 255 pixels
  h=pgm_read_byte(&img[1]);         //assume no image is bigger than 255 pixels
  XC4630_areaset(x,y,x+w-1,y+h-1);  //set area
  XC4630CS0;
  unsigned int s=w*h*2;             //we'll need to use far pointers to access larger images anyway
  for(unsigned int i=0;i<s;i++){
    XC4630_data(pgm_read_byte(&img[i+2]));
  }
  XC4630CS1;
}

void XC4630_imaget(int x,int y,const byte img[],unsigned int t){        //draws with colour t transparent, optimized to reduce areasets- heaps faster, and handles clipping at RHS better
  int w,h,u,v;
  byte areaset=1;                   //do we need to set new area?
  unsigned int c,p;                 //colour, pointer to data
  w=pgm_read_byte(&img[0]);         //assume no image is bigger than 255 pixels
  h=pgm_read_byte(&img[1]);         //assume no image is bigger than 255 pixels
  p=0;
  for(int v=0;v<h;v++){
    areaset=1;                      //assume set area at start of row
    for(int u=0;u<w;u++){
      p=p+2;
      c=pgm_read_byte(&img[p])<<8;
      c=c|pgm_read_byte(&img[p+1]);
      if(c!=t){
        if(areaset){XC4630_areaset(x+u,y+v,x+w-1,y+v);XC4630CS0;areaset=0;}    //areaset from current point to end of row
        XC4630_data(c>>8);
        XC4630_data(c);
      }else{
        areaset=1;
      }
    }
  }
  XC4630CS1;
}

void XC4630_images(int x,int y,const byte img[],byte s){   //scaled version of image function
  if(s==1){XC4630_image(x,y,img);return;}     //optimise in case parametrised
  int w,h;
  w=pgm_read_byte(&img[0]);         //assume no image is bigger than 255 pixels
  h=pgm_read_byte(&img[1]);         //assume no image is bigger than 255 pixels
  XC4630_areaset(x,y,x+w*s-1,y+h*s-1);  //set area, no boundary checks
  XC4630CS0;
  for(int v=0;v<h;v++){
    for(byte a=0;a<s;a++){          //scale factor,no interpolation
      for(int u =0;u<w;u++){
        byte d=pgm_read_byte(&img[(v*w+u)*2+2]);
        byte e=pgm_read_byte(&img[(v*w+u)*2+3]);
        for(byte b=0;b<s;b++){
          XC4630_data(d);
          XC4630_data(e);
        }
      }      
    }        
  }
  XC4630CS1;  
}

byte XC4630_imagewidth(const byte img[]){     //get image width
  return pgm_read_byte(&img[0]);
}

byte XC4630_imageheight(const byte img[]){     //get image height
  return pgm_read_byte(&img[1]);
}



