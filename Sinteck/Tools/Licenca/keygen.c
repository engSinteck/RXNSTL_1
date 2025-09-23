#include <stdio.h>
#include <string.h>
#include <stdint.h>

uint8_t sn[8] = {0, 2, 0, 1, 3, 8, 6, 0};
char input[10];
char output[10];
char ac[100];
char sequencia[10];
char numdias[10];
uint8_t license[10];
char buf[10];
uint32_t seq;
uint32_t days;
uint32_t check;

uint32_t seq_r;
uint32_t days_r;
uint32_t check_r;

void generate(void);
void regenera(void);

int main(int argc, char *argv[])
{
   printf("Teste Gerador de Licenca Sinteck Linha XT - v.1.0.1\n");
   
   seq = 3;
   days = 33;

   printf("Entre com o Número Serial: ");
   scanf("%s", ac); 
   if(strlen(ac) != 8) printf("Erro Numero Serial.\n");
   //printf("Debug: %s\n", ac);
   printf("Entre com numero Sequencia (00-99): ");
   scanf("%s", sequencia);
   printf("Entre Numero de Dias (000-999): ");
   scanf("%s", numdias);

   input[0] = sequencia[0]; 
   input[1] = sequencia[1];
   
   input[2] = numdias[0]; 
   input[3] = numdias[1];
   input[4] = numdias[2];

   int soma = (input[0] - '0') + (input[1] - '0') + (input[2] - '0') + (input[3] - '0') + (input[4] - '0');
   sprintf(buf, "%03d", soma);
   printf("Debug Soma: %s\n", buf);
   
   input[5] = buf[0]; 
   input[6] = buf[1];
   input[7] = buf[2];

   printf("Debug Input: %s\n", input);
   
   generate();
   printf(" Numero de Serie: %s   Seq: %d   Days: %d  Licensa: %c%c%c%c%c%c%c%c \n", ac,
            seq, days, license[0], license[1], license[2], license[3], license[4], license[5], license[6], license[7] );
   
   // Debug
   regenera();
   
   seq_r = ( ( (output[0] - '0') * 10) + ( output[1] - '0')  );
   days_r = ( ( (output[2] - '0') * 100 ) + ((output[3] - '0') * 10) + output[4] - '0' );
   check_r = ( ((output[5] - '0') * 100 ) + ((output[6] - '0') * 10) + output[7] - '0' ); 

   printf("Output: %X %X %X %X %X %X %X %X\n", output[0], output[1], output[2], output[3], output[4], output[5], output[6], output[7]);
   printf("Debug: Seq: %d  Days: %d   Check: %d\n", seq_r, days_r, check_r );

   printf("\nLicensa: %c%c%c%c%c%c%c%c \n", license[0], license[1], license[2], license[3], license[4], license[5], license[6], license[7] ); 

   return 0;
}

void generate(void)
{
   for(uint8_t x=0; x < 8; x++) {
	license[x] = (uint8_t)( ((input[x] ) ^ (ac[x])) );
	license[x] += 'A';
   }	   
}

void regenera(void)
{
   for(uint8_t x=0; x < 8; x++) {
	   output[x] = (uint8_t)( ((license[x]-'A' ) ^ (ac[x])) );
   }	   
}	
