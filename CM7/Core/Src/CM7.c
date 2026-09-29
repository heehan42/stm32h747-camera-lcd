#include "main.h"
#include <stdio.h>
#include "fonts.h"

extern UART_HandleTypeDef huart7;
extern DCMI_HandleTypeDef hdcmi;
extern DMA_HandleTypeDef hdma_dcmi;
extern TIM_HandleTypeDef htim4;
extern TIM_HandleTypeDef htim6;
extern TIM_HandleTypeDef htim7;
extern UART_HandleTypeDef huart7;

int _write(int file, uint8_t *p, int len){ HAL_UART_Transmit(&huart7, p, len, 100);	return len;	}

/* LCD define*/
#define  BLACK       0x0000      /*   0,   0,   0 */
#define  NAVY        0x000F      /*   0,   0, 128 */
#define  DARKGREEN   0x03E0      /*   0, 128,   0 */
#define  DARKCYAN    0x03EF      /*   0, 128, 128 */
#define  MAROON      0x7800      /* 128,   0,   0 */
#define  PURPLE      0x780F      /* 128,   0, 128 */
#define  OLIVE       0x7BE0      /* 128, 128,   0 */
#define  LIGHTGREY   0xC618      /* 192, 192, 192 */
#define  DARKGREY    0x7BEF      /* 128, 128, 128 */
#define  BLUE        0x001F      /*   0,   0, 255 */
#define  GREEN       0x07E0      /*   0, 255,   0 */
#define  CYAN        0x07FF      /*   0, 255, 255 */
#define  RED         0xF800      /* 255,   0,   0 */
#define  MAGENTA     0xF81F      /* 255,   0, 255 */
#define  YELLOW      0xFFE0      /* 255, 255,   0 */
#define  WHITE       0xFFFF      /* 255, 255, 255 */
#define  ORANGE      0xFD20      /* 255, 165,   0 */
#define  GREENYELLOW 0xAFE5      /* 173, 255,  47 */
#define putPixel(x,y,color) Buffer[x+y*800]=color
#define LCD_WIDTH       	800
#define LCD_HEIGHT      	480
#define LCD_WIDTH32     	400
#define MAX_X 800
#define MAX_Y 480
const unsigned short int  COLOR_TABLE[16] = {  BLACK, BLUE, GREEN, RED, NAVY, DARKGREEN, DARKCYAN,
						MAROON, PURPLE, OLIVE, LIGHTGREY, DARKGREY, CYAN, MAGENTA, ORANGE, GREENYELLOW};
sFONT *font = &Font16;
uint16_t font_col = WHITE;
uint16_t *Buffer=(unsigned short int *)0x70000000;
uint32_t *Frame_buffer=(uint32_t *)0x70000000;
void draw_line(int x1, int y1, int x2, int y2, uint16_t color);
void y_line(int x, int y1, int y2, uint16_t color);
void clear_screen(uint16_t color);
int abs(int x){ return x>=0?x:-x; }
int strlen(const char *str);
void put_font(sFONT *font, int idx, int x1, int y1, uint16_t font_col);
void put_char(int ch);
void put_char_col(int ch,uint16_t color);
void put_str(const char* str);
void put_str_col(const char* str,uint16_t color);
void put_num(int num);
void LCD_Init();

/* Camera*/
#define TW9910_WA			0x88
#define TW9910_RA			0x89
#define TW_SCL_H()			TW_SCL_GPIO_Port->BSRR=TW_SCL_Pin
#define TW_SCL_L()			TW_SCL_GPIO_Port->BSRR=TW_SCL_Pin<<16
#define TW_SDA_H()			TW_SDA_GPIO_Port->BSRR=TW_SDA_Pin
#define TW_SDA_L()			TW_SDA_GPIO_Port->BSRR=TW_SDA_Pin<<16
#define TW_READ_SDA()		TW_SDA_GPIO_Port->IDR&TW_SDA_Pin
#define TW_READ_FIELD()		(FIELD_GPIO_Port->IDR&FIELD_Pin)
#define delay_4us() 		for(volatile int i=0;i<328;i++)
#define delay_3500ns()		for(volatile int i=0;i<290;i++)
#define delay_3us()			for(volatile int i=0;i<250;i++)
#define delay_2500ns()		for(volatile int i=0;i<225;i++)
#define delay_2us()			for(volatile int i=0;i<200;i++)
#define delay_1us()			for(volatile int i=0;i<90;i++)
#define delay_500ns()		for(volatile int i=0;i<50;i++)
#define ACK 				0
#define NACK 				1
#define AXI_SRAM_BUFFER 	(int)0x2402B600
#define DB_LENGTH			86400
#define NORMAL 				0x00
#define SMALL 				0x01
#define SMALL_ROT			0x02
#define IS_NOT_JUMP_LINE 	(h%3!=2)
#define IS_JUMP_LINE 		(h%3==2)
#define DECODING_FIELD		(TW_READ_FIELD()==FIELD_Pin?1:0)
#define ODD_FIELD_DECODING 	DECODING_FIELD==0
#define EVEN_FIELD_DECODING	DECODING_FIELD==1
#define CURRENT_TARGET 		(((DMA_Stream_TypeDef *)(hdcmi.DMA_Handle->Instance))->CR & DMA_SxCR_CT)
#define M0					0
#define M1					1<<19
#define DB_M0				AXI_SRAM_BUFFER
#define DB_M1				AXI_SRAM_BUFFER+2*DB_LENGTH
void drawfield();
void draw_NORMAL();
void draw_SMALL();
void draw_SMALL_ROT();
//void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){ if(GPIO_Pin==FIELD_Pin && draw_flag) drawfield(); }
//void HAL_DCMI_LineEventCallback(DCMI_HandleTypeDef *hdcmi){	HS_num++; }
//void HAL_DCMI_ErrorCallback(DCMI_HandleTypeDef *hdcmi){ uint32_t FCR=((DMA_Stream_TypeDef *)hdcmi->DMA_Handle->Instance)->FCR; hdcmi->DMA_Handle->ErrorCode=HAL_DMA_ERROR_NONE; printf("FCR: 0x%08X\r\n", FCR); }
//void HAL_DCMI_FrameEventCallback(DCMI_HandleTypeDef *hdcmi){ printf("FR\r\n"); }
void HAL_DCMI_VsyncEventCallback(DCMI_HandleTypeDef *hdcmi);
void TW_I2C_Init();
int TW_I2C_write_byte(uint8_t data);
uint8_t TW_I2C_read_sequence(uint8_t index);
void TW_I2C_write_sequence(uint8_t index, uint8_t data);
void M0_half_cplt(struct __DMA_HandleTypeDef * hdma);
void M1_half_cplt(struct __DMA_HandleTypeDef * hdma);
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint8_t rx_data;
unsigned int draw_time;
int MODE=SMALL_ROT;		//0x00 , 0x01: , 0x02:
int f=0;
void CM7()
{
	HAL_TIM_Base_Start(&htim6);
	HAL_UART_Receive_IT(&huart7, &rx_data, 1);
	LCD_Init();
	TW_I2C_Init();

	while(1)
	{
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void draw_NORMAL()
{
	register int cb, y0, cr, y1;
	register int rgbR, rgbL, r, g, b, partR, partG, partB;      //uint32_t -> int (unsigned이면 0보다 작은지 판단하는 조건문을 항상 통과함)
	register uint8_t  *DB;                          			// DCMI_BUFFER
	register uint16_t *FB;
	register int jump_line=880;
	register int h, h_max;

	f++;
	/* after M0 transfer cplt */
	if(CURRENT_TARGET == M1)
	{
		DB=(uint8_t *)DB_M0;
		if(ODD_FIELD_DECODING){ FB=(uint16_t *)0x70000000; h=0; h_max=238; }
		else{ FB=(uint16_t *)0x70000640; h=1; h_max=239; }
	}
	/* after M1 transfer cplt */
	else
	{
	    DB=(uint8_t *)DB_M1;
	    if(ODD_FIELD_DECODING){ FB=(uint16_t *)0x70000000+800*240; h=240; h_max=478; }
	    else{ FB=(uint16_t *)0x70000640+800*240; h=241; h_max=479; }
	}

	for(;h<=h_max;h+=2)
	{
	    for(register int v=0;v<360;v++)
	    {
	    	cb=*DB++; y0=*DB++; cr=*DB++; y1=*DB++;
	        partR=409*cr-56992;
	        partG=34784-100*cb-208*cr;
	        partB=516*cb-70688;

	        r=298*y0+partR;
	        g=298*y0+partG;
	        b=298*y0+partB;
	        if(r>65535) r=65535; else if(r<0) r=0;
	        if(g>65535) g=65535; else if(g<0) g=0;
	        if(b>65535) b=65535; else if(b<0) b=0;
	        rgbR=((r&(0xF800)) | (g&(0xFC00))>>5 | (b&0xF800)>>11);

	        r=298*y1+partR;
	        g=298*y1+partG;
	        b=298*y1+partB;
	        if(r>65535) r=65535; else if(r<0) r=0;
	        if(g>65535) g=65535; else if(g<0) g=0;
	        if(b>65535) b=65535; else if(b<0) b=0;
	        rgbL=((r&(0xF800)) | (g&(0xFC00))>>5 | (b&0xF800)>>11);

	        if(MODE==NORMAL)
	        {
	            *FB++=rgbR;
	            *FB++=rgbL;
	        }
	    }
	    FB+=jump_line;
	}
	SCB_CleanDCache();
}

void draw_SMALL()
{
	register int cb, y0, cr, y1;
	register int rgbR, rgbL, r, g, b, partR, partG, partB;      //uint32_t -> int (unsigned이면 0보다 작은지 판단하는 조건문을 항상 통과함)
	register uint8_t  *DB;                          // DCMI_BUFFER
	register uint16_t *FB;
	register int jump_line=1120;
	register int h, h_max;

	f++;
	/* after M0 transfer cplt */
	if(CURRENT_TARGET == M1)
	{
	    DB=(uint8_t *)DB_M0;
	    if(ODD_FIELD_DECODING){ FB=(uint16_t *)0x70000000; h=0; h_max=238; }
	    else{ FB=(uint16_t *)0x70000000+800; h=1; h_max=239; }

	}
	/* after M1 transfer cplt */
	else
	{
	    DB=(uint8_t *)DB_M1;
	    if(ODD_FIELD_DECODING){ FB=(uint16_t *)0x70000000+800*160; h=240; h_max=478; }
	   	else{ FB=(uint16_t *)0x70000000+800*161; h=241; h_max=479; }
	}

	for(;h<=h_max;h+=2)
	{
		if(IS_JUMP_LINE){ DB+=1440; continue; }

		for(register int v=0;v<360;v++)
	    {
	    	cb=*DB++; y0=*DB++; cr=*DB++; y1=*DB++;
	        partR=409*cr-56992;
	        partG=34784-100*cb-208*cr;
	        partB=516*cb-70688;

	        r=298*y0+partR;
	        g=298*y0+partG;
	        b=298*y0+partB;
	        if(r>65535) r=65535; else if(r<0) r=0;
	        if(g>65535) g=65535; else if(g<0) g=0;
	        if(b>65535) b=65535; else if(b<0) b=0;
	        rgbR=((r&(0xF800)) | (g&(0xFC00))>>5 | (b&0xF800)>>11);

	        r=298*y1+partR;
	        g=298*y1+partG;
	        b=298*y1+partB;
	        if(r>65535) r=65535; else if(r<0) r=0;
	        if(g>65535) g=65535; else if(g<0) g=0;
	        if(b>65535) b=65535; else if(b<0) b=0;
	        rgbL=((r&(0xF800)) | (g&(0xFC00))>>5 | (b&0xF800)>>11);

	        if(v%3==0){ *FB++=rgbR; *FB++=rgbL; }
			else if(v%3==1){ *FB++=rgbL; }
			else{ *FB++=rgbR; }

	    }
	    FB+=jump_line;
	}
	SCB_CleanDCache();
}

void draw_SMALL_ROT()
{
	register int cb, y0, cr, y1, axi_idx, ycbcr;
	register int rgbR, rgbL, r, g, b, partR, partG, partB;      	//uint32_t -> int (unsigned이면 0보다 작은지 판단하는 조건문을 항상 통과함)
	register uint8_t *DB;                           				// DCMI_BUFFER
	register uint32_t *DB2;
	register uint16_t *FB;
	int fb_y, fb_y_start, mode=2;

	f++;
	/* after M0 transfer cplt */
	if(CURRENT_TARGET == M1)
	{
		DB=(uint8_t *)DB_M0;
		DB2=(uint32_t *)DB_M0;
		if(ODD_FIELD_DECODING){ FB=(uint16_t *)0x70000000; fb_y_start=0;  }
		else{ FB=(uint16_t *)0x70000000+1; fb_y_start=1;}
	}
	/* after M1 transfer cplt */
	else
	{
		DB=(uint8_t *)DB_M1;
		DB2=(uint32_t *)DB_M0+43200;
		if(ODD_FIELD_DECODING){ FB=(uint16_t *)0x70000000+160; fb_y_start=240; }
		else{ FB=(uint16_t *)0x70000000+161; fb_y_start=241;}
	}

	for(int axi_x=359;axi_x>=0;axi_x--)
	{
		fb_y=fb_y_start;
		for(int axi_y=0;axi_y<120;axi_y++)
		{
			if(fb_y%3==2){ fb_y+=2; continue; }

			axi_idx=(axi_x+360*axi_y)*4;
			cb=DB[axi_idx++]; y0=DB[axi_idx++]; cr=DB[axi_idx++]; y1=DB[axi_idx++];

ycbcr=DB2[axi_x+360*axi_y];
cb=ycbcr&0x000000FF;     y0=(ycbcr&0x0000FF00)>>8;  cr=(ycbcr&0x00FF0000)>>16; y1=(ycbcr&0xFF000000)>>24;

			partR=409*cr-56992;
			partG=34784-100*cb-208*cr;
			partB=516*cb-70688;
			r=298*y0+partR;
			g=298*y0+partG;
			b=298*y0+partB;
			if(r>65535) r=65535; else if(r<0) r=0;
			if(g>65535) g=65535; else if(g<0) g=0;
			if(b>65535) b=65535; else if(b<0) b=0;
			rgbR=((r&(0xF800)) | (g&(0xFC00))>>5 | (b&0xF800)>>11);

			r=298*y1+partR;
			g=298*y1+partG;
			b=298*y1+partB;
			if(r>65535) r=65535; else if(r<0) r=0;
			if(g>65535) g=65535; else if(g<0) g=0;
			if(b>65535) b=65535; else if(b<0) b=0;
			rgbL=((r&(0xF800)) | (g&(0xFC00))>>5 | (b&0xF800)>>11);


			if(mode==0){ *FB=rgbL; *(FB+800)=rgbR; }
			else if(mode==1){ *FB=rgbL; }
			else if(mode==2){ *FB=rgbR; }

			FB+=2;
			fb_y+=2;
		}
		if(mode==0) FB+=1440; else FB+=640;
		mode==0?mode=2:mode--;
//printf("8 : 0x%02X, 0x%02X, 0x%02X, 0x%02X\r\n",cb, y0, cr, y1);
//cb=ycbcr&0x000000FF;     y0=(ycbcr&0x0000FF00)>>8;  cr=(ycbcr&0x00FF0000)>>16; y1=(ycbcr&0xFF000000)>>24;
//printf("32: 0x%02X, 0x%02X, 0x%02X, 0x%02X | ycbcr: 0x%08X\r\n",cb, y0, cr, y1, ycbcr);
//cb=ycbcr&0x000000FF;     y0=ycbcr&0x0000FF00;  cr=ycbcr&0x00FF0000; y1=ycbcr&0xFF000000;
//printf("32: 0x%02X, 0x%02X, 0x%02X, 0x%02X | ycbcr: 0x%08X\r\n",cb, y0>>8, cr>>16, y1>>24, ycbcr);
	}
	SCB_CleanDCache();
}

void TW_I2C_Init()
{
	//TW_I2C_write_sequence(0x01, 0x01);		// DET50
	TW_I2C_write_sequence(0x02, 0x44);			// MUX=1
	TW_I2C_write_sequence(0x03, 0x06);			// TRI_SEL
	TW_I2C_write_sequence(0x05, 0x10);			// V/HSYNC
	TW_I2C_write_sequence(0x08, 0x12);			// VDELAY_LO
	TW_I2C_write_sequence(0x0A, 0x10);			// HDELAY_LO

	TW_I2C_write_sequence(0x07, 0x02);			// CROP_HI: V/HDELAY_HI, V/HACTIVE_HI
	TW_I2C_write_sequence(0x09, 0xF0);			// VACTIVE_LO
	TW_I2C_write_sequence(0x0B, 0xD0);			// HACTIVE_LO
	TW_I2C_write_sequence(0x0E, 0x11);			// V/HSCALE_HI

	TW_I2C_write_sequence(0x15, 0x00);			// HUE
	TW_I2C_write_sequence(0x19, 0x56);			// RTSEL(FIELD)
	TW_I2C_write_sequence(0x24, 0x3C);			// CLMPL

	uint32_t *DB=(uint32_t *)AXI_SRAM_BUFFER;
	for(int v=0;v<240;v++)
	{
		for(int h=0;h<360;h++)
		{
			DB[360*v+h]=0x12345678;
		}
	}
	uint8_t *DB2=(uint8_t *)AXI_SRAM_BUFFER;
	//printf("DB[0]: %u, DB[1]: %u, DB[2]: %u, DB[3]: %u | %d, %d, %d, %d\r\n", (DB2[0]&0xFF000000)>>24, (DB[0]&0x00FF0000)>>16, (DB[0]&0x0000FF00)>>8, DB[0]*0x000000FF, *DB++, *DB++, *DB++, *DB++);
	printf("DB2[0]: %d, DB2[1]: %d, DB2[2]: %d, DB2[3]: %d | %d, %d, %d, %d | %d, %d, %d, %d\r\n", DB2[0], DB2[1], DB2[2], DB2[3], *DB2++, *DB2++, *DB2++, *DB2++, DB[0]&0x000000FF, (DB[0]&0x0000FF00)>>8, (DB[0]&0x00FF0000)>>16, (DB[0]&0xFF000000)>>24);
	__HAL_DCMI_DISABLE_IT(&hdcmi, DCMI_IT_LINE);
	HAL_DCMI_Start_DMA(&hdcmi, DCMI_MODE_CONTINUOUS, (int)AXI_SRAM_BUFFER, DB_LENGTH);
	printf("-----TW_I2C_Init()-----\r\n");
}


void TW_I2C_Start()
{
	TW_SDA_L();
	delay_1us();
	TW_SCL_L();
	delay_4us();
}

void TW_I2C_Restart()
{
	TW_SDA_H();
	delay_1us();
	TW_SCL_H();
	delay_3500ns();
	TW_SDA_L();
	delay_500ns();
	TW_SCL_L();
	delay_3us();
}

void TW_I2C_Stop()
{
	delay_1us();
	TW_SCL_H();
	delay_1us();
	TW_SDA_H();
	delay_1us();
}

int TW_I2C_write_byte(uint8_t data)
{
	int ck=-1;

	/* write byte */
	for(int j=7;j>=0;j--)
	{
		if(data&(1<<j)){ TW_SDA_H(); } else { TW_SDA_L(); }
		delay_1us();
		TW_SCL_H();
		delay_4us();
		TW_SCL_L();
		delay_500ns();
		TW_SDA_L();
		if(j) delay_2500ns();
	}

	/* read ACK */
	TW_SDA_H();
	delay_3500ns();
	TW_SCL_H();
	delay_4us();
	ck=TW_READ_SDA()&TW_SDA_Pin;
	TW_SCL_L();
	delay_500ns();
	TW_SDA_L();
	delay_2500ns();

	return ck;
}


uint8_t TW_I2C_read_sequence(uint8_t index)
{
	uint8_t input=0;

	TW_I2C_Start();

	if(TW_I2C_write_byte(TW9910_WA)==ACK)
	{
		if(TW_I2C_write_byte(index)==ACK)
		{
			TW_I2C_Restart();

			if(TW_I2C_write_byte(TW9910_RA)==ACK)
			{
				/* read byte */
				for(int j=7;j>=0;j--)
				{
					TW_SDA_H();
					delay_1us();
					TW_SCL_H();
					delay_4us();
					input|=((TW_READ_SDA()&TW_SDA_Pin)==0?0:1)<<j;
					TW_SCL_L();
					delay_500ns();
					TW_SDA_L();
					if(j){ delay_2us(); delay_500ns(); }
				}

				/* read NACK */
				TW_SDA_H();
				delay_3500ns();
				TW_SCL_H();
				delay_4us();
				TW_SCL_L();
				delay_500ns();
				TW_SDA_L();
				delay_2500ns()
				TW_I2C_Stop();

				//printf("read data: 0x%02X\r\n", input);
			}
			else{ TW_I2C_Stop(); printf("read second addr NACK\r\n"); }
		}
		else{ TW_I2C_Stop(); printf("read index NACK\r\n"); };
	}
	else{ TW_I2C_Stop(); printf("read addr NACK\r\n"); };

	return input;
}

void TW_I2C_write_sequence(uint8_t index, uint8_t data)
{
	TW_I2C_Start();

	if(TW_I2C_write_byte(TW9910_WA)==ACK)
	{
		if(TW_I2C_write_byte(index)==ACK)
		{
			if(TW_I2C_write_byte(data)==ACK)
			{
				TW_I2C_Stop();

				//printf("write data: %d\r\n");
			}
			else{ TW_I2C_Stop(); printf("data NACK\r\n"); }
		}
		else{ TW_I2C_Stop(); printf("index NACK\r\n"); }
	}
	else{ TW_I2C_Stop(); printf("addr NACK\r\n"); }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void draw_line(int x1, int y1, int x2, int y2, uint16_t color)
{	int x,y,X,Y;
	int w = abs(x2-x1), h = abs(y2-y1);
	if(w>h)
	{
		x = x1 < x2 ? x1 : x2,	X = x1 < x2 ? x2 : x1;
		y = x == x1 ? y1 : y2,	Y = x == x1 ? y2 : y1;

		for(int i=x;i<=X;i++) putPixel(i, ((Y-y)*(i-x))/(X-x)+y, color);
	}else
	{
		y = y1 < y2 ? y1 : y2, Y = y1 < y2 ? y2 : y1;
		x = y == y1 ? x1 : x2, X = y == y1 ? x2 : x1;

		for(int i=y;i<=Y;i++) putPixel(((X-x)*(i-y))/(Y-y)+x, i, color);
	}
}

void y_line(int x, int y1, int y2, uint16_t color)
{	int y, Y;
	if(y1<=y2){ Y=y2; y=y1; }else{ Y=y1; y=y2; }
	for(int h=y;h<=Y;h++) putPixel(x,h,color);
}

void clear_screen(uint16_t color)
{	int x, y, y2;
	uint32_t color32 = (color << 16) | color;

	for (y=0; y < LCD_HEIGHT; y++) {
		y2 = LCD_WIDTH32 * y;
		for (x=0; x <LCD_WIDTH32; x++) {
			Frame_buffer[y2 + x] = color32;
		}
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int gx=0,gy=0;

int strlen(const char *str)
{	int len=0;
	while(*str++) len++;
	return len;
}

void put_font(sFONT *font, int idx, int x1, int y1, uint16_t font_col)
{	const uint8_t *table = font->table;
	int w_byte = font->Width%8==0? font->Width/8:font->Width/8+1;
	int h = font->Height;
	int start = idx*w_byte*h;
	int i,j,k;

	for(j=0;j<h;j++)
	{
		for(i=0;i<w_byte;i++)
		{
			for(k=7;k>=0;k--)
			{
				if(table[start+j*w_byte+i]&(1<<k)) Buffer[(y1+j)*800+(x1+i*8+7-k)]=font_col;
			}
		}
	}
}

void put_char(int ch)
{	if(gx > MAX_X-font->Width){ gx=0;gy+=font->Height; }
	put_font(font,ch-' ',gx,gy, font_col);
	gx+=font->Width;
}

void put_char_col(int ch,uint16_t color)
{	if(gx > MAX_X-font->Width){ gx=0;gy+=font->Height; }
	put_font(font,ch-' ', gx, gy, color);
	gx+=font->Width;
}

void put_str(const char* str)
{	for(int i=0; i<strlen(str); i++) put_char(str[i]);
}

void put_str_col(const char* str,uint16_t color)
{	font_col=color;
	for(int i=0; i<strlen(str); i++)put_char(str[i]);
}

void put_num(int num)
{	int i=1, k=0, num_=num, ch;
	while(num_)
	{	num_/=10;
		k++;
	}
	for(int j=1;j<k;j++) i*=10;
	while(num)
	{	ch=num/i+'0';
		put_char(ch);
		num%=i;
		i/=10;
	}
}

void LCD_Init()
{
	HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1);
	clear_screen(BLACK);
	put_str("hello, world!");
	y_line(720, 0, 479, RED);
	y_line(722, 0, 479, RED);
	y_line(480, 0, 479, YELLOW);
	y_line(482, 0, 479, YELLOW);
	draw_line(0, 320, 799, 320, YELLOW);
	draw_line(0, 322, 799, 322, YELLOW);
	draw_line(320, 0, 320, 479, BLUE);
	draw_line(322, 0, 322, 479, BLUE);
	draw_line(160, 0, 160, 479, BLUE);
	draw_line(161, 0, 161, 479, BLUE);
	draw_line(162, 0, 162, 479, BLUE);
	SCB_CleanDCache();
	printf("-----LCD_Init-----\r\n");
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if(huart->Instance==UART7)
	{
		printf("key preesed: %c\r\n", rx_data);
		if(rx_data=='c')
		{
			if(hdcmi.Instance->CR&DCMI_CR_CAPTURE){ hdcmi.Instance->CR&=(~DCMI_CR_CAPTURE); printf("camera off\r\n"); }
			else{ hdcmi.Instance->CR|=DCMI_CR_CAPTURE; printf("camera on\r\n"); }
		}
		else if(rx_data=='m')
		{

		}

		HAL_UART_Receive_IT(&huart7, &rx_data, 1);
	}
}
