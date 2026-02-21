#include <lpc21xx.h>
#include <string.h>
#include <stdio.h>
#include "delay.h"
#include "spi.c"
#define IR 12

int esp_wait(char *str, int timeout);
char rxbuff[500];
volatile int index = 0;

void uart1_isr(void)__irq;

void uart0_init(void)
{
    PINSEL0 |= 0x00000005;
    U0LCR = 0x83;
    U0DLL = 97;
    U0DLM = 0;
    U0LCR = 0x03;
}

void uart0_tx(char ch)
{
    while(!(U0LSR & 0x20));
    U0THR = ch;
}

void uart0_st(char *str)
{
    while(*str)
    uart0_tx(*str++);
    }
void uart1_init(void)
{
    PINSEL0 |= 0x00050000;  //p0.8 as TXD1 and p0.9 as RXD1 

    U1LCR = 0x83;
    U1DLL = 97;
    U1DLM = 0;
    U1LCR = 0x03;

    U1IER = 0x01;  // Enable RX interrupt

    VICIntEnable = 1<<7;
    VICVectCntl0 = (0x20)|7;
    VICVectAddr0 = (unsigned long)uart1_isr;
}

void uart1_tx(char ch)
{
    while(!(U1LSR & 0x20));
    U1THR = ch;
}
void uart1_st(char *str)
{
    while(*str)
     uart1_tx(*str++);
     }

void uart1_isr(void) __irq
{
    char ch;

    while(U1LSR & 0x01)
    {
     ch = U1RBR;
    if(index < sizeof(rxbuff)-1)
    {
    rxbuff[index++] = ch;
    rxbuff[index] = '\0';
    }
   } 
	VICVectAddr=0;
}
int esp_wait(char *str, int delay)
{
  int t = 0;
  
  while(t < delay)
  {
   if(strstr(rxbuff, str))
   return 1;

   delay_ms(1);
   t++;
  }
   return 0;
}

int main(void)
{
   char http[200];
   char len_cmd[30];
   int len;

   uart0_init();  
   uart1_init();
   spi_init();  

   uart1_st("Environment Monitoring\r\n");

   index=0;
  memset(rxbuff,0,sizeof(rxbuff));

   uart1_st("AT\r\n");	 //AT commands on UART1 terminal
   if(!esp_wait("OK",3000))
   {
    uart0_st("AT failed\r\n"); //status on UART0 terminal
    while(1);
   }
   index=0;
   uart1_st("AT+CWMODE=1\r\n");
   if(!esp_wait("OK",3000))
   {
   uart0_st("mode part failed\r\n");
   while(1);
   }
   index=0;
   uart1_st("AT+CIPMUX=0\r\n");
 
   if(!esp_wait("OK",3000))
   { 
   uart0_st("CIPMUX failed\r\n");
    while(1);  
    }
   index=0;
   uart1_st("AT+CWJAP=\"Gokul\",\"12345678\"\r\n");
   if(!esp_wait("OK",20000)) 
   {
    uart0_st("wifi failed\r\n");
	while(1);
	}
 while(1)
{
   float temp_adc;
   unsigned int temp,IRV;
     
   index=0;
   uart1_st("AT+CIPSTART=\"TCP\",\"api.thingspeak.com\",80\r\n");
   if(!esp_wait("CONNECT",7000))
   {
   uart0_st("connection failed\r\n");
   continue;
   }

   temp_adc=read_mcp3204(0);
   temp=((temp_adc*3300)/4096);
   temp=temp/10;
   IRV=((IOPIN0>>IR)&1);  //assign the status to IRV 0 means obstacle is not detected 
  
 
   sprintf(http, "GET /update?api_key=2XMANMKM3D5OAF0V&field1=%d&field2=%d\r\n",temp,IRV);
   len = strlen(http);
   sprintf(len_cmd,"AT+CIPSEND=%d\r\n",len);
   index=0;
   uart1_st(len_cmd);
   if(!esp_wait(">",3000))
   continue;
   index=0;
   uart1_st(http);
   esp_wait("SEND OK",5000);
   delay_ms(15000);
  }
}
