#include <SPI.h>
#include"Ap_29demo.h"
//IO settings
int BUSY_Pin = 37; 
int RES_Pin = 16; 
int DC_Pin = 21; 
int CS_Pin = 2; 

#define EPD_W21_CS_0 digitalWrite(CS_Pin,LOW)
#define EPD_W21_CS_1 digitalWrite(CS_Pin,HIGH)
#define EPD_W21_DC_0  digitalWrite(DC_Pin,LOW)
#define EPD_W21_DC_1  digitalWrite(DC_Pin,HIGH)
#define EPD_W21_RST_0 digitalWrite(RES_Pin,LOW)
#define EPD_W21_RST_1 digitalWrite(RES_Pin,HIGH)
#define isEPD_W21_BUSY digitalRead(BUSY_Pin)
////////FUNCTION//////
void SPI_Write(unsigned char value);
void EPD_W21_WriteDATA(unsigned char command);
void EPD_W21_WriteCMD(unsigned char command);
//EPD
//EPD
void EPD_W21_Init(void);
void EPD_init(void);
void PIC_display (const unsigned char* picData);
void EPD_sleep(void);
void EPD_refresh(void);
void lcd_chkstatus(void);
void PIC_display_Clear(void);
//Display canvas function
void EPD_Display(unsigned char *Image); 
void EPD_init_Fast(void);
void EPD_Dis_Part(unsigned int x_start,unsigned int y_start,const unsigned char * old_data,const unsigned char * new_data,unsigned int PART_COLUMN,unsigned int PART_LINE,unsigned char mode); //mode 0: first  1: other...
void EPD_Part_Init(void);
void Power_off(void);

void setup() {
   pinMode(BUSY_Pin, INPUT); 
   pinMode(RES_Pin, OUTPUT);  
   pinMode(DC_Pin, OUTPUT);    
   pinMode(CS_Pin, OUTPUT);   
   //SPI
   SPI.beginTransaction(SPISettings(10000000, MSBFIRST, SPI_MODE0)); 
   SPI.begin ();  
}

//Tips//
/*When the electronic paper is refreshed in full screen, the picture flicker is a normal phenomenon, and the main function is to clear the display afterimage in the previous picture.
  When the local refresh is performed, the screen does not flash.*/
/*When you need to transplant the driver, you only need to change the corresponding IO. The BUSY pin is the input mode and the others are the output mode. */



void loop() {
  while(1)
  {
    //ALl display
    EPD_init(); //EPD init
    PIC_display(gImage_1);//EPD_picture1
    EPD_sleep();//EPD_sleep,Sleep instruction is necessary, please do not delete!!!
    delay(1000); //1s  
  ///////////////////////////Quick refresh picture display/////////////////////////////////////////////////////////////////
    /************Quick refresh picture display*******************/
    EPD_init_Fast(); //EPD init
    PIC_display(gImage_1);//EPD_picture1
    EPD_sleep();//EPD_sleep,Sleep instruction is necessary, please do not delete!!!
    delay(1000); //1s    
    EPD_init_Fast(); //EPD init
    PIC_display(gImage_2);//EPD_picture1
    EPD_sleep();//EPD_sleep,Sleep instruction is necessary, please do not delete!!!
    delay(1000); //1s   
    
    /************Partial refresh picture display*******************/
    EPD_init_Fast(); //EPD init
    PIC_display(gImage_basemap);//EPD_picture1     
    EPD_Part_Init(); //EPD part init    
    EPD_Dis_Part(0,0,gImage_1,gImage_1,320,240,0); //partial display 2
    delay(1000); //1s  
    EPD_Part_Init(); //EPD part init        
    EPD_Dis_Part(0,0,gImage_1,gImage_2,320,240,1); //partial display 3 
    delay(1000); //1s

//Clear   
    EPD_init(); //EPD init
    PIC_display_Clear();//EPD Clear
    EPD_sleep();//EPD_sleep,Sleep instruction is necessary, please do not delete!!! 
    while(1); 

  }
}




///////////////////EXTERNAL FUNCTION////////////////////////////////////////////////////////////////////////
void SPI_Write(unsigned char value)                                    
{                                                           
  SPI.transfer(value);
}

void EPD_W21_WriteCMD(unsigned char command)
{
  EPD_W21_CS_0;                   
  EPD_W21_DC_0;   // command write
  SPI_Write(command);
  EPD_W21_CS_1;
}
void EPD_W21_WriteDATA(unsigned char command)
{
  EPD_W21_CS_0;                   
  EPD_W21_DC_1;   // command write
  SPI_Write(command);
  EPD_W21_CS_1;
}



/////////////////EPD settings Functions/////////////////////
void EPD_W21_Init(void)
{
  EPD_W21_RST_0;    // Module reset
  delay(10); //At least 10ms
  EPD_W21_RST_1;
  delay(10);  
}

//UC8253
void EPD_init(void)
{  
    EPD_W21_Init(); //Electronic paper IC reset  

    EPD_W21_WriteCMD(0x01);     // POWER SETTING   PWR
    EPD_W21_WriteDATA(0x03);      //  x x x x x x VDS_EN VDG_EN 
    EPD_W21_WriteDATA(0x10);      //  x x x VCOM_SLWE VGH[3:0]   VGH=20V, VGL=-20V  
    EPD_W21_WriteDATA(0x3F);      //  x x VSH[5:0]  VSH = 15V
    EPD_W21_WriteDATA(0x3F);      //  x x VSL[5:0]  VSL=-15V
    EPD_W21_WriteDATA(0x14);      //  OPTEN VDHR[6:0]  VHDR=6.4V

    EPD_W21_WriteCMD(0x03);     // POWER OFF sequence setting    PFS
    EPD_W21_WriteDATA(0x00);      // x x T_VDS_OFF[1:0] x x x x                 // T_VDS_OFF[1:0] 00=1 frame; 01=2 frame; 10=3 frame; 11=4 fram
    
    EPD_W21_WriteCMD(0x06);     // booster soft start   BTST 
    EPD_W21_WriteDATA(0x17);      //  BT_PHA[7:0]   
    EPD_W21_WriteDATA(0x17);      //  BT_PHB[7:0] 
    EPD_W21_WriteDATA(0x17);      //  x x BT_PHC[5:0]   
  
    EPD_W21_WriteCMD(0x00);     // panel setting   PSR
    EPD_W21_WriteDATA(0x1F);      // RES1 RES0 REG KW/R     UD    SHL   SHD_N  RST_N   0xFF=LUT from register,0xDF=LUT from OTP. (Default)
    
    EPD_W21_WriteCMD(0x04);  //Power on
    lcd_chkstatus();        //waiting for the electronic paper IC to release the idle signal
  }
void EPD_init_Fast(void)
{ 
    EPD_W21_Init(); //Electronic paper IC reset  

    EPD_W21_WriteCMD(0x01);     // POWER SETTING   PWR
    EPD_W21_WriteDATA(0x03);      //  x x x x x x VDS_EN VDG_EN 
    EPD_W21_WriteDATA(0x10);      //  x x x VCOM_SLWE VGH[3:0]   VGH=20V, VGL=-20V  
    EPD_W21_WriteDATA(0x3F);      //  x x VSH[5:0]  VSH = 15V
    EPD_W21_WriteDATA(0x3F);      //  x x VSL[5:0]  VSL=-15V
    EPD_W21_WriteDATA(0x14);      //  OPTEN VDHR[6:0]  VHDR=6.4V

    EPD_W21_WriteCMD(0x03);     // POWER OFF sequence setting    PFS
    EPD_W21_WriteDATA(0x00);      // x x T_VDS_OFF[1:0] x x x x                 // T_VDS_OFF[1:0] 00=1 frame; 01=2 frame; 10=3 frame; 11=4 fram
    
    EPD_W21_WriteCMD(0x06);     // booster soft start   BTST 
    EPD_W21_WriteDATA(0x17);      //  BT_PHA[7:0]   
    EPD_W21_WriteDATA(0x17);      //  BT_PHB[7:0] 
    EPD_W21_WriteDATA(0x17);      //  x x BT_PHC[5:0] 
  
    EPD_W21_WriteCMD(0x04);  //Power on
    lcd_chkstatus();        //waiting for the electronic paper IC to release the idle signal*/

    EPD_W21_WriteCMD(0x00);     // panel setting   PSR
    EPD_W21_WriteDATA(0x1F);      // RES1 RES0 REG KW/R     UD    SHL   SHD_N  RST_N   0xFF=LUT from register,0xDF=LUT from OTP. (Default)
    
    EPD_W21_WriteCMD(0xE0);
    EPD_W21_WriteDATA(0x02); 
    
    EPD_W21_WriteCMD(0xE5);
    EPD_W21_WriteDATA(0x64);
  
}
//0.6s
unsigned char LUT_Part[216]={             
0x01, 0x14, 0x0A, 0x00, 0x00, 0x01, 0x01, 
0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 
0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 
0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 
0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 
0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 
0x01, 0x14, 0x0A, 0x00, 0x00, 0x01, 0x01, 
0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 
0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 
0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 
0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 
0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 
0x01, 0x94, 0x0A, 0x00, 0x00, 0x01, 0x01, 
0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 
0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 
0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 
0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 
0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 
0x01, 0x54, 0x4A, 0x00, 0x00, 0x01, 0x01, 
0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 
0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 
0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 
0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 
0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 
0x01, 0x54, 0x4A, 0x00, 0x00, 0x01, 0x01, 
0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 
0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 
0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 
0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 
0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 
0x09, 0x10, 0x3F, 0x3F, 0x00, 0x00,   
//Frame VGH VSH VSL VSHR  VCOM    
};  

void WRITE_PartLUT(void)
{
  unsigned int i;     

  EPD_W21_WriteCMD(0x20);// write VCOM register
  for(i=0;i<42;i++)
  {
      EPD_W21_WriteDATA(LUT_Part[i]);
  }
  EPD_W21_WriteCMD(0x21);// write LUTWW register
  for(i=42;i<84;i++)
  {
      EPD_W21_WriteDATA(LUT_Part[i]);
  }
  EPD_W21_WriteCMD(0x22);// write LUTBW register
  for(i=84;i<126;i++)
  {
      EPD_W21_WriteDATA(LUT_Part[i]);
  }
  EPD_W21_WriteCMD(0x23);// write LUTWB register
  for(i=126;i<168;i++)
  { 
      EPD_W21_WriteDATA(LUT_Part[i]); 
  }
  EPD_W21_WriteCMD(0x24);// write LUTBB register
  for(i=168;i<210;i++)
  { 
      EPD_W21_WriteDATA(LUT_Part[i]); 
  }
}
void EPD_Part_Init(void)
{
  EPD_W21_Init(); //Electronic paper IC reset  
  lcd_chkstatus();  
  EPD_W21_WriteCMD(0x04); //POWER ON
  lcd_chkstatus();  
  
  EPD_W21_WriteCMD(0x00);//Panel Setting   
  EPD_W21_WriteDATA(0xFF);//LUT FROM MCU
  EPD_W21_WriteDATA(0x8D);//   SHL   SHD_N  RST_N  0xFF=LUT from register,0xDF=LUT from OTP. (Default)

  EPD_W21_WriteCMD(0x01);// power setting   
  EPD_W21_WriteDATA(0x03);//Enable internal VSH\VSL\VGH\VGL 
  EPD_W21_WriteDATA(LUT_Part[211]);//VGH=20V,VGL=-20V
  EPD_W21_WriteDATA(LUT_Part[212]);//VSH=15V
  EPD_W21_WriteDATA(LUT_Part[213]);//VSL=-15V
  EPD_W21_WriteDATA(LUT_Part[214]);//VSHR

  EPD_W21_WriteCMD(0x06);//booster soft start 
  EPD_W21_WriteDATA(0xD7);
  EPD_W21_WriteDATA(0xD7);
  EPD_W21_WriteDATA(0x2F);

  EPD_W21_WriteCMD(0x30);//PLL  -Frame rate 
  EPD_W21_WriteDATA(LUT_Part[210]);// 
  
  EPD_W21_WriteCMD(0x50); //CDI   
  EPD_W21_WriteDATA(0xD7);//KW MODE   VBD=10,DDX=01,CDI=0111

  EPD_W21_WriteCMD(0x60);//TCON SETTING 
  EPD_W21_WriteDATA(0x22);//
  
  EPD_W21_WriteCMD(0x65); 
  EPD_W21_WriteDATA(0x00);
  EPD_W21_WriteDATA(0x00);
  EPD_W21_WriteDATA(0x00);
  EPD_W21_WriteDATA(0x00);
  
  EPD_W21_WriteCMD(0x82);//VCOM_DC   
  EPD_W21_WriteDATA(LUT_Part[215]);//-2.0V  

  //Power Saving Register 
  EPD_W21_WriteCMD(0xE3);        
  EPD_W21_WriteDATA(0x88);

  WRITE_PartLUT();
}
  
void EPD_sleep(void)
{
    EPD_W21_WriteCMD(0X50);  //VCOM AND DATA INTERVAL SETTING     
    EPD_W21_WriteDATA(0xf7); //WBmode:VBDF 17|D7 VBDW 97 VBDB 57    WBRmode:VBDF F7 VBDW 77 VBDB 37  VBDR B7  

    EPD_W21_WriteCMD(0X02);   //power off
    lcd_chkstatus();          //waiting for the electronic paper IC to release the idle signal
    EPD_W21_WriteCMD(0X07);   //deep sleep
    EPD_W21_WriteDATA(0xA5);
}



void PIC_display(const unsigned char* picData)
{
    unsigned int i;
    //Write Data  
    EPD_W21_WriteCMD(0x10);        //Transfer new data
    for(i=0;i<9600;i++)      
    {
      EPD_W21_WriteDATA(pgm_read_byte(&picData[i]));  //Transfer the actual displayed data
    }
    EPD_W21_WriteCMD(0x13);        //Transfer new data
    for(i=0;i<9600;i++)      
    {
      EPD_W21_WriteDATA(pgm_read_byte(&picData[i]));  //Transfer the actual displayed data
    }
   //Refresh
    EPD_W21_WriteCMD(0x12);   //DISPLAY REFRESH   
    delay(1);   //!!!The delay here is necessary, 200uS at least!!!     
    lcd_chkstatus();          //waiting for the electronic paper IC to release the idle signal


}
void PIC_display_Clear(void)
{
    unsigned int i;
    //Write Data
    EPD_W21_WriteCMD(0x13);        //Transfer new data
    for(i=0;i<9600;i++)      
    {
    EPD_W21_WriteDATA(0xFF);  //Transfer the actual displayed data
    }
    //Refresh
    EPD_W21_WriteCMD(0x12);   //DISPLAY REFRESH   
    delay(1);              //!!!The delay here is necessary, 200uS at least!!!     
    lcd_chkstatus();  
}

void lcd_chkstatus(void)
{
  while(1)
  {   //=0 BUSY
     if(isEPD_W21_BUSY==1) break;
  }                         
}
void EPD_Dis_Part(unsigned int x_start,unsigned int y_start,const unsigned char * old_data,const unsigned char * new_data,unsigned int PART_COLUMN,unsigned int PART_LINE,unsigned char mode) //mode 0: first  1: other...
{
unsigned int datas,i;
unsigned int x_end,y_end;
  x_end=x_start+PART_LINE-1; 
  y_end=y_start+PART_COLUMN-1;
  datas=PART_LINE*PART_COLUMN/8;

    EPD_W21_WriteCMD(0x91);   //This command makes the display enter partial mode
    EPD_W21_WriteCMD(0x90);   //resolution setting
    EPD_W21_WriteDATA (x_start);   //x-start     
    EPD_W21_WriteDATA (x_end-1);   //x-end  

    EPD_W21_WriteDATA (y_start/256);
    EPD_W21_WriteDATA (y_start%256);   //y-start    
    
    EPD_W21_WriteDATA (y_end/256);    
    EPD_W21_WriteDATA (y_end%256-1);  //y-end
    EPD_W21_WriteDATA (0x01);  //28
  
    EPD_W21_WriteCMD(0x10);        //writes New data to SRAM.
    for(i=0;i<9600;i++)       
   {
       if(mode==0)
       {
         EPD_W21_WriteDATA(0xff); 
       }
       else
       {
          EPD_W21_WriteDATA(pgm_read_byte(&old_data[i]));
       }
   } 
    EPD_W21_WriteCMD(0x13);        //writes Old data to SRAM for programming
    for(i=0;i<9600;i++)       
     {
        EPD_W21_WriteDATA(pgm_read_byte(&new_data[i]));    
     }   
    EPD_W21_WriteCMD(0x12);    //DISPLAY REFRESH                 
    delay(10);     //!!!The delay here is necessary, 200uS at least!!!     
    lcd_chkstatus();
    Power_off(); 
}


void Power_off(void)
{ 
  EPD_W21_WriteCMD(0x02); //POWER ON
  lcd_chkstatus();
}
