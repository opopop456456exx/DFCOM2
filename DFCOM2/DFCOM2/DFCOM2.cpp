// DFCOM2.cpp : 定义控制台应用程序的入口点。
//1106

#include "stdafx.h"
#include <iostream>  
#include <cstdlib>  
#include <Windows.h>  
#include<stdlib.h>

using namespace std;
static HANDLE HCom = INVALID_HANDLE_VALUE;
static int ReadableSize = 0;
static char* ErrorMessage = "no error.";

char buffer0[100];

enum {
	Timeout = 1000,               // [msec]
	EachTimeout = 2,              // [msec]
	LineLength = 64 + 3 + 1 + 1 + 1 + 16,
};



static int com_changeBaudrate(long baudrate)
{
	DCB dcb;

	GetCommState(HCom, &dcb);
	dcb.BaudRate = baudrate;
	dcb.ByteSize = 8;
	dcb.Parity = NOPARITY;
	dcb.fParity = FALSE;
	dcb.StopBits = ONESTOPBIT;
	SetCommState(HCom, &dcb);

	return 0;
}



static int com_connect(const char* device, long baudrate)
{
#if defined(RAW_OUTPUT)
	Raw_fd_ = fopen("raw_output.txt", "w");
#endif

	char adjust_device[16];
	_snprintf(adjust_device, 16, "\\\\.\\%s", device);           //第二种打开串口方式，可以打开COM10以上串口
	HCom = CreateFileA(adjust_device, GENERIC_READ | GENERIC_WRITE, 0,
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (HCom == INVALID_HANDLE_VALUE) {
		return -1;
	}

	// Baud rate setting
	return com_changeBaudrate(baudrate);
}



bool SetupTimeout(DWORD ReadInterval, DWORD ReadTotalMultiplier, DWORD
	ReadTotalConstant, DWORD WriteTotalMultiplier, DWORD WriteTotalConstant)
{
	COMMTIMEOUTS timeouts;
	timeouts.ReadIntervalTimeout = ReadInterval;
	timeouts.ReadTotalTimeoutConstant = ReadTotalConstant;
	timeouts.ReadTotalTimeoutMultiplier = ReadTotalMultiplier;
	timeouts.WriteTotalTimeoutConstant = WriteTotalConstant;
	timeouts.WriteTotalTimeoutMultiplier = WriteTotalMultiplier;
	if (!SetCommTimeouts(HCom, &timeouts))
	{
		return false;
	}
	else
		return true;
}


static int com_send(const char* data, int size)
{
	DWORD n;
	WriteFile(HCom, data, size, &n, NULL);   //WriteFile函数将数据写入一个文件。
	return n;
}


// The command is transmitted to URG
static int urg_sendTag(const char* tag)
{
	char send_message[LineLength];
	_snprintf(send_message, LineLength, "%s\n", tag);
	int send_size = (int)strlen(send_message);
	com_send(send_message, send_size);

	return send_size;
}









static int com_recv(char* data, int max_size, int timeout)      //串口读取
{
	if (max_size <= 0) {
		return 0;
	}

	if (ReadableSize < max_size) {
		DWORD dwErrors;
		COMSTAT ComStat;
		ClearCommError(HCom, &dwErrors, &ComStat);
		ReadableSize = ComStat.cbInQue;              //当前串口中存有的数据个数
	}

	if (max_size > ReadableSize) {
		COMMTIMEOUTS pcto;
		int each_timeout = 2;

		if (timeout == 0) {
			max_size = ReadableSize;

		}
		else {
			if (timeout < 0) {
				/* If timeout is 0, this function wait data infinity */   //如果timeout为0，此函数无限制等待数据
				timeout = 0;
				each_timeout = 0;
			}

			/* set timeout */
			GetCommTimeouts(HCom, &pcto);
			pcto.ReadIntervalTimeout = timeout;
			pcto.ReadTotalTimeoutMultiplier = each_timeout;
			pcto.ReadTotalTimeoutConstant = timeout;
			SetCommTimeouts(HCom, &pcto);
		}
	}

	DWORD n;
	ReadFile(HCom, data, (DWORD)max_size, &n, NULL);
#if defined(RAW_OUTPUT)
	if (Raw_fd_) {
		for (int i = 0; i < n; ++i) {
			fprintf(Raw_fd_, "%c", data[i]);
		}
		fflush(Raw_fd_);
	}
#endif
	if (n > 0) {
		ReadableSize -= n;
	}

	return n;
}


static int COMREAD(char *data)
{

	//	char str[100];

	DWORD dwErrors;
	COMSTAT ComStat;

	DWORD wCount;//读取的字节数  
	BOOL bReadStat;

	ClearCommError(HCom, &dwErrors, &ComStat);
	ReadableSize = ComStat.cbInQue;              //当前串口中存有的数据个数
//	printf("ReadableSize=%d", ReadableSize);
	bReadStat = ReadFile(HCom, data, 1, &wCount, NULL);
	//data  = str;
	if (!bReadStat)
	{
		printf("Fail to read the COM!\n");
		return FALSE;
	}

	return wCount;

}


void outputData(long data[], int n, size_t total_index)
{
	char output_file[] = "data_xxxxxxxxxx.csv";
	_snprintf(output_file, sizeof(output_file), "data_%03d.csv", total_index);
	FILE* fd = fopen(output_file, "w");
	if (!fd) {
		perror("fopen");       //perror(s) 用来将上一个函数发生错误的原因输出到标准设备(stderr)。
		return;
	}

	for (int i = 0; i < n; ++i) {
		fprintf(fd, "%ld, ", data[i]);    //格式化输出至磁盘文件
	}
	fprintf(fd, "\n");

	fclose(fd);
}




/**************************************************************************/

//默认串口缓冲是4096byte

/***************************************************************************/



int main(int argc, char *argv[])
{

	const char com_port[] = "COM3";                                //设置串口号和波特率
	const long com_baudrate = 115200;
	int n, i = 0, j = 0,k=0,l=0, linelen=0;

	static char message_buffer[LineLength];
	long* data = new   long[1024];
	char data2[1024];

	char send_message[LineLength];
	

	if (com_connect(com_port, com_baudrate) < 0) {                        //如果波特率、串口设置失败
		_snprintf(message_buffer, LineLength,                     //_snprintf函数将可变个参数(...)按照format格式化成字符串，然后将其复制到str中
			"Cannot connect COM device: %s", com_port);              //写入错误信息到ErrorMessage内
		ErrorMessage = message_buffer;
		printf("urg_connect: %s\n", ErrorMessage);
		getchar();
		return -1;
	}
	else
	{
		printf("open com success!\n");
		printf("set DCB success!\n");
	}

	//if (SetupTimeout(0, 0, 0, 0, 0))
	//	printf("Set timeout success!\n");

	//for (;;) 
	//   {
	n = urg_sendTag("hello world!");
	//	printf("n=%d,", n);
//	printf("i=%d\n", i++);
	if (i >= 65535)i = 0;

	//    }
	printf("Receve:\n");
	for (linelen = 0;; linelen++) {

		for (i = 0;; i++)
		{


			if ((l = COMREAD(buffer0)) > 0) {
				//	printf(",receve:%s,--%d----wd:%d\n", buffer0, j++,l);

				if ((buffer0[0] == '\n'))                                     //if ((buffer0[0] == '\n')||(buffer0[0] == 0x0d))
				{
					 break;
				}
				data2[i] = buffer0[0];


			}
			else
			{
				if (i == 0)
				{
					printf("timeout\n");
					return -1;
				}
				break;
			}
			

		}
		data2[i] = '\0';
		

		if (i > 0)
			printf("the data is:%s,linlen=%d\n", data2, linelen);
		//	cout << "the data is: " << data [0]<< "  linelen: " << linelen << "\n" << endl;
		else
			break;
		if (strcmp(data2, "abc123") == 0)
		{
			printf("Receive:abc123\n");
			_snprintf(send_message, LineLength, "abc123,%d", j++);
			if (j >= 65535)j = 0;
			urg_sendTag(send_message);
			
		}
		else if (strcmp(data2, "abc456") == 0)
		{
			printf("Receive:abc456\n");
			_snprintf(send_message, LineLength, "abc456,%d", j++);
			if (j >= 65535)j = 0;
			urg_sendTag(send_message);

		}


	}
	printf("end\n");
	getchar();




	return 0;
}
