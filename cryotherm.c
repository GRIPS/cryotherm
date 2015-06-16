// cryotherm.c -- get cryostat temperatures using sub-20 board

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <math.h>
#ifdef _MSC_VER
#include <windows.h>          // for HANDLE
#else
#include <termios.h>
#include <unistd.h>
#endif
#include <fcntl.h>
#include <sys/types.h>
//#include <io.h>

#include "libsub/libsub.h"

#define MAXPATH 128

#include "wirth.c"
#define READINGS_TO_MEDIAN 100
unsigned short int to_median[READINGS_TO_MEDIAN];

struct usb_device* dev;
int rc;
extern int errno;
struct tm *times;
time_t ltime;
char obsfilespec[MAXPATH];
int savedata = 1;
int fout;
sub_handle* fd = NULL ;

unsigned short int swbuf[8];
float v[8],t[8],vin[8];

struct table {
	float V;
	float T; };

struct table Ttab[] = {
0.449594, 340.,
0.474822, 330.,
0.499958, 320.,
0.525021, 310.,
0.550000, 300.,
0.574880, 290.,
0.599653, 280.,
0.616583, 273.,
0.624308, 270.,
0.648837, 260.,
0.673232, 250.,
0.697486, 240.,
0.721587, 230.,
0.745530, 220.,
0.769304, 210.,
0.792899, 200.,
0.816302, 190.,
0.839502, 180.,
0.862483, 170.,
0.885224, 160.,
0.907702, 150.,
0.929885, 140.,
0.951741, 130.,
0.973233, 120.,
0.994324, 110.,
1.014970, 100.,
1.035122, 90.,
1.045000, 85.,
1.054744, 80.,
1.059853, 77.35,
1.064352, 75.,
1.073825, 70.,
1.083167, 65.,
1.092385, 60.
};

int subinit()
{
	dev = NULL;
	/* Open USB device */
	fd = sub_open( dev );

	if( !fd )
	{
		printf("sub_open: %s\n", sub_strerror(sub_errno));
		exit( -1);
	}    

	rc = sub_spi_config( fd,SPI_ENABLE | SPI_CLK_8MHZ | SPI_CPOL_FALL | SPI_SMPL_SETUP | SPI_MSB_FIRST , 0 );
	return rc;
}

/* leave this if ever want to save data */
void Startfile()
{
	char stringtemp[80];
	time(&ltime);
	times = localtime(&ltime);
	strftime(stringtemp,23,"data_%y%m%d_%H%M.dat",times);
	strncpy(obsfilespec,stringtemp,MAXPATH - 1);
	obsfilespec[MAXPATH - 1] = '\0';
	savedata = 0;
	printf("%s \n",obsfilespec);
	{
#ifdef _MSC_VER
		if((fout = open(obsfilespec,O_RDWR|O_CREAT|O_BINARY,0600)) < 0)
#else
                if((fout = open(obsfilespec,O_RDWR|O_CREAT,0600)) < 0)
#endif
		{
			printf("Cannot open file\n");
			
		}
		else savedata = 1;
    }    
	
}

void oneline(const char *str1, const char *str2)
{
	FILE *outfile;
	outfile = fopen("monitorboard.dat", "a");
	fprintf(outfile, "%s %s\n", str1, str2);
        fclose(outfile);
	outfile = fopen("monitorboard-oneline.dat", "w");
	fprintf(outfile, "%s %s\n", str1, str2);
        fclose(outfile);
}

float getT(float volt) /* get temperature from table */
{
int i;
float deltav,deltab,delt,del;

	for( i = 0; (Ttab[i].T >= 60.) && (Ttab[i].V < volt ) ; i++);
	if(Ttab[i].T == 60.) return 60.;
	else if( i == 0) return Ttab[0].T;
	else
	{
		deltav = volt - Ttab[i-1].V ;
		deltab = Ttab[i].V - Ttab[i-1].V ;
		delt = Ttab[i].T - Ttab[i-1].T ;
		del = delt*deltav/deltab;
		return Ttab[i-1].T + del;
	}
}

unsigned short int sw(unsigned short int in)
{
unsigned short int top,bot,out;

	top = (in & 0xff00) >> 8;
	bot = in & 0xff;
	out = (bot << 8) | top;
	return out;
}

#ifndef _MSC_VER
int kbhit(void)
{
  struct termios oldt, newt;
  int ch;
  int oldf;

  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  oldf = fcntl(STDIN_FILENO, F_GETFL, 0);

  fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

  ch = getchar();

  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  fcntl(STDIN_FILENO, F_SETFL, oldf);

  if(ch != EOF)
  {
    return ch;
  }

  return 0;
}
#endif

unsigned short int read_last_and_set_next(unsigned short int input)
{
	unsigned char outbuf[2]={0x83,0x30};
	unsigned short int inbuf, swbuf;

	outbuf[0] = 0x83 | ( (input & 0x7) << 2); /* set address bits*/
	outbuf[1] = 0x30 ;
	rc = sub_spi_transfer(fd,outbuf,(unsigned char *) &inbuf,2,SS_CONF(0,SS_LLH)  );
	swbuf = sw(inbuf);
	return swbuf & 0xfff ;
}

int main(int argc, char *argv[])
{

int i,j,key,disptype = 3;
float T0,T1;

	char stringtemp[100];
	char stringtemp2[100];

	if( subinit() != 0) 
	{
		printf(" Couldn't initilize SUB-20 SPI interface -- %s \n",sub_strerror(sub_errno));
		exit(-1);
	}
//	Startfile();
	printf("starting acquisition\n");
	printf("[H]hex [D]dec [F]V_out [T]temperature [V]V_in\n");
	for(;;)
	{

		time(&ltime);
		times = localtime(&ltime);
		strftime(stringtemp,20,"%Y-%m-%d %H:%M:%S",times);

		memset(v, 0, 8);

		read_last_and_set_next(0);
		for( i = 0; i <= 1; i++)
		{
			for( j = 0; j < READINGS_TO_MEDIAN; j++ )
			{
				swbuf[i] = read_last_and_set_next(i);
				to_median[j] = swbuf[i];
			}
			v[i] = median(to_median, READINGS_TO_MEDIAN);
			read_last_and_set_next(i+1);
		}

		for( i = 2; i <= 7; i++)
		{
			swbuf[i] = read_last_and_set_next(i+1);
			v[i] += swbuf[i];
		}

		for( i = 0; i <= 7; i++)
		{
			v[i] *= 2.5/4095. ;
			if(i == 0)
			{
				 vin[0] = 0.51698 + v[0]/3.98; /* resistor values */
//				vin[0] = 0.5209 + v[0]/3.939;  /* measured */
				t[0] = getT(vin[0]);
			}
			else if(i == 1)
			{
				 vin[1] = 0.51698 + v[1]/3.98; /* resistor values */
//				vin[1] = 0.5212 + v[1]/3.9745; /* measured */
				t[1] = getT(vin[1]);
			}
			else
			{
				vin[i] = 2.*v[i];
				t[i] = vin[i]/(0.01) - 273.15; /* nominally 10 mV/K */
			}
			if( savedata != 0)
			{
//				write(fout,(const void *) &swbuf[i],2);
			}
		}

		if(disptype ==0)
			sprintf(stringtemp2, "   %04x  %04x  %04x  %04x  %04x  %04x  %04x  %04x  hex",swbuf[0],swbuf[1],swbuf[2],swbuf[3],
		    swbuf[4],swbuf[5],swbuf[6],swbuf[7]);
		if(disptype == 1)
			sprintf(stringtemp2, "  %05d %05d %05d %05d %05d %05d %05d %05d   A/D #",(swbuf[0]&0xfff),
		    (swbuf[1]&0xfff),(swbuf[2]&0xfff),(swbuf[3]&0xfff),
		    (swbuf[4]&0xfff),(swbuf[5]&0xfff),(swbuf[6]&0xfff),(swbuf[7]&0xfff));
		if(disptype == 2)
			sprintf(stringtemp2, "  %5.3f %5.3f %5.3f %5.3f %5.3f %5.3f %5.3f %5.3f volt",v[0],v[1],
		    v[2],v[3],v[4],v[5],v[6],v[7]);
		if(disptype == 3)
			sprintf(stringtemp2, "  %5.3f %5.3f %5.1f %5.1f %5.1f %5.1f %5.1f %5.1f deg K/C",t[0],t[1],
		    t[2],t[3],t[4],t[5],t[6],t[7]);
		if(disptype == 4)
			sprintf(stringtemp2, "  %5.3f %5.3f %5.3f %5.3f %5.3f %5.3f %5.3f %5.3f volt",vin[0],vin[1],
		    vin[2],vin[3],vin[4],vin[5],vin[6],vin[7]);

		printf("%s%s\r", stringtemp, stringtemp2);
		if (savedata == 1) oneline(stringtemp2, stringtemp);

#ifdef _MSC_VER
		if(_kbhit() != 0)
		{
			key =  0x7f & _getch();
#else
		if((key = kbhit()) != 0)
		{
#endif 
			if( key == 'H' || key == 'h') disptype = 0;
			if( key == 'D' || key == 'd') disptype = 1;
			if( key == 'F' || key == 'f') disptype = 2;
			if( key == 'T' || key == 't') disptype = 3;
			if( key == 'V' || key == 'v') disptype = 4;
			printf("\n                                                         \r");
		}
#ifdef _MSC_VER
		_sleep(1000); /* 1 second sleep */
#else
		sleep(1);
#endif
	}

}

