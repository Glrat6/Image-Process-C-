#include "stdafx.h"
#include <windows.h>
#include <math.h>

#define PI 3.14159265


BYTE* LoadBMP(int* width, int* height, long* size, LPCTSTR bmpfile)
{
	// declare bitmap structures
	BITMAPFILEHEADER bmpheader;
	BITMAPINFOHEADER bmpinfo;
	// value to be used in ReadFile funcs
	DWORD bytesread;
	// open file to read from
	HANDLE file = CreateFile(bmpfile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (NULL == file)
		return NULL; // coudn't open file

	// read file header
	if (ReadFile(file, &bmpheader, sizeof (BITMAPFILEHEADER), &bytesread, NULL) == false)  {
		       CloseHandle(file);
		       return NULL;
	      }
	//read bitmap info
	if (ReadFile(file, &bmpinfo, sizeof (BITMAPINFOHEADER), &bytesread, NULL) == false) {
		        CloseHandle(file);
		        return NULL;
	      }
	// check if file is actually a bmp
	if (bmpheader.bfType != 'MB')  	{
		       CloseHandle(file);
		       return NULL;
	      }
	// get image measurements
	*width = bmpinfo.biWidth;
	*height = abs(bmpinfo.biHeight);

	// check if bmp is uncompressed
	if (bmpinfo.biCompression != BI_RGB)  {
		      CloseHandle(file);
		      return NULL;
	      }
	// check if we have 24 bit bmp
	if (bmpinfo.biBitCount != 24) {
		      CloseHandle(file);
		      return NULL;
	     }

	// create buffer to hold the data
	*size = bmpheader.bfSize - bmpheader.bfOffBits;
	BYTE* Buffer = new BYTE[*size];
	// move file pointer to start of bitmap data
	SetFilePointer(file, bmpheader.bfOffBits, NULL, FILE_BEGIN);
	// read bmp data
	if (ReadFile(file, Buffer, *size, &bytesread, NULL) == false)  {
		     delete[] Buffer;
		     CloseHandle(file);
		     return NULL;
	      }
	// everything successful here: close file and return buffer
	CloseHandle(file);

	return Buffer;
}//LOADPMB

bool SaveBMP(BYTE* Buffer, int width, int height, long paddedsize, LPCTSTR bmpfile)
{
	// declare bmp structures 
	BITMAPFILEHEADER bmfh;
	BITMAPINFOHEADER info;

	// andinitialize them to zero
	memset(&bmfh, 0, sizeof (BITMAPFILEHEADER));
	memset(&info, 0, sizeof (BITMAPINFOHEADER));

	// fill the fileheader with data
	bmfh.bfType = 0x4d42;       // 0x4d42 = 'BM'
	bmfh.bfReserved1 = 0;
	bmfh.bfReserved2 = 0;
	bmfh.bfSize = sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER)+paddedsize;
	bmfh.bfOffBits = 0x36;		// number of bytes to start of bitmap bits

	// fill the infoheader

	info.biSize = sizeof(BITMAPINFOHEADER);
	info.biWidth = width;
	info.biHeight = height;
	info.biPlanes = 1;			// we only have one bitplane
	info.biBitCount = 24;		// RGB mode is 24 bits
	info.biCompression = BI_RGB;
	info.biSizeImage = 0;		// can be 0 for 24 bit images
	info.biXPelsPerMeter = 0x0ec4;     // paint and PSP use this values
	info.biYPelsPerMeter = 0x0ec4;
	info.biClrUsed = 0;			// we are in RGB mode and have no palette
	info.biClrImportant = 0;    // all colors are important

	// now we open the file to write to
	HANDLE file = CreateFile(bmpfile, GENERIC_WRITE, FILE_SHARE_READ,
		NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file == NULL)  	{
		    CloseHandle(file);
		    return false;
	   }
	// write file header
	unsigned long bwritten;
	if (WriteFile(file, &bmfh, sizeof (BITMAPFILEHEADER), &bwritten, NULL) == false)  {
		       CloseHandle(file);
		       return false;
	      }
	// write infoheader
	if (WriteFile(file, &info, sizeof (BITMAPINFOHEADER), &bwritten, NULL) == false)  {
		     CloseHandle(file);
		     return false;
	      }
	// write image data
	if (WriteFile(file, Buffer, paddedsize, &bwritten, NULL) == false)  {
		      CloseHandle(file);
		      return false;
	     }

	// and close file
	CloseHandle(file);

	return true;
} // SaveBMP

BYTE* ConvertBMPToIntensity(BYTE* Buffer, int width, int height)
{
	// first make sure the parameters are valid
	if ((NULL == Buffer) || (width == 0) || (height == 0))
		return NULL;

	// find the number of padding bytes

	int padding = 0;
	int scanlinebytes = width * 3;
	while ((scanlinebytes + padding) % 4 != 0)     // DWORD = 4 bytes
		padding++;
	// get the padded scanline width
	int psw = scanlinebytes + padding;

	// create new buffer
	BYTE* newbuf = new BYTE[width*height];

	// now we loop trough all bytes of the original buffer, 
	// swap the R and B bytes and the scanlines
	long bufpos = 0;
	long newpos = 0;
	for (int row = 0; row < height; row++)
	for (int column = 0; column < width; column++)  {
		      newpos = row * width + column;
		      bufpos = (height - row - 1) * psw + column * 3;
		      newbuf[newpos] = BYTE(0.11*Buffer[bufpos + 2] + 0.59*Buffer[bufpos + 1] + 0.3*Buffer[bufpos]);
	      }

	return newbuf;
}//ConvetBMPToIntensity

BYTE* ConvertIntensityToBMP(BYTE* Buffer, int width, int height, long* newsize)
{
	// first make sure the parameters are valid
	if ((NULL == Buffer) || (width == 0) || (height == 0))
		return NULL;

	// now we have to find with how many bytes
	// we have to pad for the next DWORD boundary	

	int padding = 0;
	int scanlinebytes = width * 3;
	while ((scanlinebytes + padding) % 4 != 0)     // DWORD = 4 bytes
		padding++;
	// get the padded scanline width
	int psw = scanlinebytes + padding;
	// we can already store the size of the new padded buffer
	*newsize = height * psw;

	// and create new buffer
	BYTE* newbuf = new BYTE[*newsize];

	// fill the buffer with zero bytes then we dont have to add
	// extra padding zero bytes later on
	memset(newbuf, 0, *newsize);

	// now we loop trough all bytes of the original buffer, 
	// swap the R and B bytes and the scanlines
	long bufpos = 0;
	long newpos = 0;
	for (int row = 0; row < height; row++)
	for (int column = 0; column < width; column++)  	{
		bufpos = row * width + column;     // position in original buffer
		newpos = (height - row - 1) * psw + column * 3;           // position in padded buffer
		newbuf[newpos] = Buffer[bufpos];       //  blue
		newbuf[newpos + 1] = Buffer[bufpos];   //  green
		newbuf[newpos + 2] = Buffer[bufpos];   //  red
	}

	return newbuf;
} //ConvertIntensityToBMP

//BYTE* ciz_cember(BYTE* intensity, int width, int height, int centerX, int centerY, int yaricap)
//{
//	int cemberX, cemberY;
//
//	for (double aci = 0; aci < 360; aci += 0.001)
//	{
//		cemberX = yaricap * cos(aci) + centerX;
//		cemberY = yaricap * sin(aci) + centerY;
//		if (cemberX >= 0 && cemberX <= width && cemberY >= 0 && cemberY <= height)
//			intensity[(cemberY-1)*width + (cemberX-1)] = 255;
//	}
//	return intensity;
//}


BYTE* ciz_cember(BYTE* intensity, int width, int height, int centerX, int centerY, int yaricap)
{
	for (int cemberX = 0; cemberX < width; cemberX+= 1)
		for (int cemberY = 0; cemberY < height; cemberY+= 1)
			if ((centerX-cemberX)*(centerX-cemberX) + (centerY-cemberY)*(centerY-cemberY) == yaricap*yaricap)
				intensity[(cemberY - 1)*width + cemberX] = 255;

	return intensity;
}


//BYTE* ciz_dikdortgen(BYTE* intensity, int width, int height, int centerX, int centerY, int en, int boy)
//{
//	int a=0, b=0;
//	int i = 0;
//	a = en / 2;
//	b = boy / 2;
//
//	for (i = ((centerY - b - 1)*width + centerX - a - 1); i < en; i++)
//		intensity[i] = 255;
//	
//	for (i = (((centerY - b - 1) + 1)*width + centerX - a - 1); i < boy; i++)
//		intensity[i] = 255;
//
//	for (i = (((centerY - b - 1) + 1)*width + centerX + a - 1); i < boy; i++)
//		intensity[i] = 255;
//
//	for (i = ((centerY + b - 1)*width + centerX - a - 1); i < en; i++)
//		intensity[i] = 255;
//
//	return intensity;
//}

void sirala(int A[9])
{
	int araDegisken = 0;

	for (int j = 0; j<8; j++)
	{
		for (int i = 0; i<8; i++)
		{
			if (A[i]<A[i + 1])
			{
				araDegisken = A[i + 1];
				A[i + 1] = A[i];
				A[i] = araDegisken;
			}
		}
	}

}

BYTE* mean_filter(BYTE* intensity, int width, int height, long* newsize)
{
	if ((NULL == intensity) || (width == 0) || (height == 0))
		return NULL;

	int padding = 0;
	int scanlinebytes = width * 3;
	while ((scanlinebytes + padding) % 4 != 0)    
		padding++;

	int psw = scanlinebytes + padding;

	*newsize = height * psw;

	BYTE* mean_buf = new BYTE[*newsize];

	
	for (int i = 0; i < *newsize; i++)
		mean_buf[i] = 0;

	for (int i = 0; i < height - 2; i++)
		for (int j = 0; j < width - 2; j++)
			mean_buf[width*i + width + j + 1] = (

				intensity[width*i + j] +
				intensity[width*i + j + 1] +
				intensity[width*i + j + 2] +
				intensity[width*i + j + width] +
				intensity[width*i + j + 1 + width] +
				intensity[width*i + j + 2 + width] +
				intensity[width*i + j + 2 * width] +
				intensity[width*i + j + 1 + 2 * width] +
				intensity[width*i + j + 2 + 2 * width]) / 9;

	return mean_buf;

}

BYTE* median_filter(BYTE* intensity, int width, int height, long* newsize)
{
	if ((NULL == intensity) || (width == 0) || (height == 0))
		return NULL;

	int padding = 0;
	int scanlinebytes = width * 3;
	while ((scanlinebytes + padding) % 4 != 0)
		padding++;

	int psw = scanlinebytes + padding;

	*newsize = height * psw;

	BYTE* median_buf = new BYTE[*newsize];

	for (int i = 0; i < *newsize; i++)
		median_buf[i] = 0;

	int A[9];
	for (int i = 0; i < 9; i++)
		A[i] = 0;


	for (int i = 0; i < height - 2; i++)
		for (int j = 0; j < width - 2; j++)
		{

			A[0] = intensity[width*i + j];
			A[1] = intensity[width*i + j + 1];
			A[2] = intensity[width*i + j + 2];
			A[3] = intensity[width*i + j + width];
			A[4] = intensity[width*i + j + 1 + width];
			A[5] = intensity[width*i + j + 2 + width];
			A[6] = intensity[width*i + j + 2 * width];
			A[7] = intensity[width*i + j + 1 + 2 * width];
			A[8] = intensity[width*i + j + 2 + 2 * width];

			sirala(A);

			median_buf[width*i + j + 1 + width] = A[4];

		}

	return median_buf;
}

BYTE* gaussian_filter(BYTE* intensity, int width, int height, long* newsize)
{
	if ((NULL == intensity) || (width == 0) || (height == 0))
		return NULL;

	int padding = 0;
	int scanlinebytes = width * 3;
	while ((scanlinebytes + padding) % 4 != 0)
		padding++;

	int psw = scanlinebytes + padding;

	*newsize = height * psw;

	BYTE* gaussian_buf = new BYTE[*newsize];

	for (int i = 0; i < *newsize; i++)
		gaussian_buf[i] = 0;

	for (int i = 0; i < height - 2; i++)
		for (int j = 0; j < width - 2; j++)
			gaussian_buf[width*i + width + j + 1] = (

			intensity[width*i + j]/4 +
			intensity[width*i + j + 1]/2 +
			intensity[width*i + j + 2]/4 +
			intensity[width*i + j + width]/2 +
			intensity[width*i + j + 1 + width] +
			intensity[width*i + j + 2 + width]/2 +
			intensity[width*i + j + 2 * width]/4 +
			intensity[width*i + j + 1 + 2 * width]/2 +
			intensity[width*i + j + 2 + 2 * width]/4)/4;


	return gaussian_buf;

}

BYTE* histogram(BYTE* intensity, int width, int height)
{
	BYTE* th = new BYTE[256];

	for (int i = 0; i < 255; i++)
		th[i] = 0;

	for (int i = 0; i < height*width; i++)
		th[intensity[i]]++;

	return th;

}

BYTE* thresholding(BYTE* intensity, int width, int height,long* newsize)
{
	int t1 = 0, t2 = 255, t12 = 1, t22 = 1;
	int a = 0, b = 0;
	*newsize = width*height;

	BYTE* th2 = new BYTE[256];
	BYTE* binary = new BYTE[*newsize];

	th2 = histogram(intensity, width, height);

	while (1)
	{
		for (int i = 0; i < 255; i++)
		{
			if (-(t1 - i) < (t2 - i))
				th2[i] = 1;
			else
				th2[i] = 2;
		}
		for (int j = 0; j < 255; j++)
			if (th2[j] == 1)
				a = a + j, b++;
		t12 = a / b, a = 0, b = 0;
			
		for (int k = 0; k < 255; k++)
			if (th2[k] == 2)
				a = a + k, b++;
		t22 = a / b, a = 0, b = 0;

		if (t1 == t12 && t2 == t22)
			break;
		else
		{
			t1 = t12, t12 = 0;
			t2 = t22, t22 = 0;
		}
	}

	int T = t12 + abs(t12 - t22) / 2;

	for (int x = 0; x < width*height; x++)
		binary[x] = 0;

	for (int i = 0; i < width*height; i++)
		if (intensity[i]>=T)
			binary[i] = 255;
		else
			binary[i] = 0;

	return binary;
}

BYTE* dilation(BYTE* intensity, int width, int height, long* newsize)
{
	if ((NULL == intensity) || (width == 0) || (height == 0))
		return NULL;

	*newsize = height * width;

	BYTE* dilation = new BYTE[*newsize];

	for (int k = 0; k < width*height; k++)
		dilation[k] = 255;

	for (int i = 0; i < width*height;i++)
		if (intensity[i] == 0)
		{
			if (i == 0)  // sol ust kose
			{
				dilation[i] = 0;
				dilation[i + 1] = 0;
				dilation[i + width] = 0;
			}
			if (i == width - 1)  // sag ust kose
			{
				dilation[i - 1] = 0;
				dilation[i] = 0;
				dilation[i + width] = 0;
			}
			if (i == width*height - width)  // sol alt kose
			{
				dilation[i - height];
				dilation[i] = 0;
				dilation[i + 1] = 0;
			}
			if (i == width*height - 1)  //sag alt kose
			{
				dilation[i - height];
				dilation[i - 1] = 0;
				dilation[i] = 0;
			}
			if (i%width == 0 && i != 0) // sol kenar
			{
				dilation[i - height];
				dilation[i] = 0;
				dilation[i + 1] = 0;
				dilation[i + height] = 0;
			}
			if ((i + 1) % width == 0 && i > width)  //sag kenar
			{
				dilation[i - height];
				dilation[i - 1] = 0;
				dilation[i] = 0;
				dilation[i + height] = 0;
			}
			if (i > 0 && i < width - 1) // üst kenar
			{
				dilation[i - 1] = 0;
				dilation[i] = 0;
				dilation[i + 1] = 0;
				dilation[i + height] = 0;
			}
			if (i > (width*height) - width && i < (width*height) - 1)  // alt kenar
			{
				dilation[i - height] = 0;
				dilation[i - 1] = 0;
				dilation[i] = 0;
				dilation[i + 1] = 0;
			}
			if (i > width && i < width*height - width - 1 && i % width != 0 && (i + 1) % width != 0)  //orta kýsým
			{
				dilation[i - height] = 0;
				dilation[i - 1] = 0;
				dilation[i] = 0;
				dilation[i + 1] = 0;
				dilation[i + height] = 0;
			}
		}
		

	//for (int i = 0; i < height - 2; i++)
	//	for (int j = 0; j < width - 2;j++)
	//		if (intensity[width*i + j] == 255)
	//		{
	//			//dilation[width*i + j] = 0;
	//			//dilation[width*i + j + 1] = 0;
	//			dilation[width*i + j + 2] = 0;
	//			//dilation[width*i + j + width] = 0;
	//			dilation[width*i + j + 1 + width] = 0;
	//			//dilation[width*i + j + 2 + width] = 0;
	//			dilation[width*i + j + 2 * width] = 0;
	//			//dilation[width*i + j + 1 + 2 * width] = 0;
	//			//dilation[width*i + j + 2 + 2 * width] = 0;
	//		}

	return dilation;
}

BYTE* erosion(BYTE* intensity, int width, int height, long* newsize)
{
	if ((NULL == intensity) || (width == 0) || (height == 0))
		return NULL;

	*newsize = height*width;

	BYTE* erosion = new BYTE[*newsize];

	for (int k = 0; k < width*height; k++)
		erosion[k] = 255;

	int B[9] = { 0, 0, 0, 255, 255, 255, 0, 0, 0 };
	int a = 0;

	for (int i = 0; i < width*height; i++)
	{
		if (intensity[i] == 0)
		{
			if (i == 0)  // sol ust kose
				a = intensity[i] + intensity[i + 1] + B[4] + B[5];
				if (a / 4 < 255)
					erosion[i] = 255;
				a = 0;
			if (i == width - 1)  // sag ust kose
			{
				a = intensity[i - 1] + intensity[i]  + B[3] + B[4];
				if (a / 4 < 255)
					erosion[i] = 255;
				a = 0;
			}
			if (i == width*height - width)  // sol alt kose
			{
				a = intensity[i] + intensity[i + 1] + B[4] + B[5];
				if (a / 4 < 255)
					erosion[i] = 255;
				a = 0;
				}
			if (i == width*height - 1)  //sag alt kose
			{
				a = intensity[i - 1] + intensity[i] +  B[3] + B[4];
				if (a / 4 < 255)
					erosion[i] = 255;
				a = 0;
			}
			if (i%width == 0 && i != 0) // sol kenar
			{
				a = intensity[i] + intensity[i + 1] + B[4] + B[5];
				if (a / 4 < 255)
					erosion[i] = 255;
				a = 0;
			}
			if ((i + 1) % width == 0 && i > width)  //sag kenar
			{
				a = intensity[i - 1] + intensity[i] + B[3] + B[4];
				if (a / 4 < 255)
					intensity[i] = 255;
				a = 0;
			}
			if (i > 0 && i < width - 1) // üst kenar
			{
				a = intensity[i - 1] + intensity[i] + intensity[i + 1] + B[3] + B[4] + B[5];
				if (a / 6 < 255)
					intensity[i] = 255;
				a = 0;
			}
			if (i >(width*height) - width && i < (width*height) - 1)  // alt kenar
			{
				a = intensity[i - 1] + intensity[i] + intensity[i + 1] + B[3] + B[4] + B[5];
				if (a / 6 < 255)
					intensity[i] = 255;
				a = 0;
			}
			if (i > width && i < width*height - width - 1 && i % width != 0 && (i + 1) % width != 0)  //orta kýsým
			{
				a = intensity[i - 1] + intensity[i] + intensity[i + 1] + B[3] + B[4] + B[5];
				if (a / 6 < 255)
					intensity[i] = 255;
				a = 0;
			}
		}
	}
	return erosion;
}

//BYTE* Mask(BYTE* Buffer, int width, int height, long* newsize)
//{
//	BYTE *ptr1, *ptr2, *ptr3;
//	if ((NULL == Buffer) || (width == 0) || (height == 0))
//		return NULL;
//
//	int padding = 0;
//	int scanlinebytes = width * 3;
//	while ((scanlinebytes + padding) % 4 != 0)
//		padding++;
//
//	int psw = scanlinebytes + padding;
//
//	*newsize = height * psw;
//
//	BYTE* yenibuf = new BYTE[*newsize];
//
//	for (int i = 0; i < *newsize; i++)
//		yenibuf[i] = 0;
//
//	ptr1 = Buffer;
//	ptr2 = ptr1 + width;
//	ptr3 = ptr2 + width;
//
//	for (int j = 0; j < *newsize; j++)
//	{
//		yenibuf[j + 1] = (ptr1[j] + ptr1[j + 1] + ptr1[j + 2] + ptr2[j] + ptr2[j + 1] + ptr2[j + 2] + ptr3[j] + ptr3[j + 1] + ptr3[j + 2]) / 9;
//	}
//
//
//	return yenibuf;
//}

BYTE* non_maxima_suppression(BYTE* intensity, BYTE* intensity2, int width, int height, long* newsize)
{
	BYTE* buf = new BYTE[*newsize];
	for (int i = 0; i < *newsize; i++)
		buf[i] = intensity[i];

	for (int i = 0; i < height - 2; i++)
		for (int j = 0; j < width - 2; j++)
		{
			if (intensity2[width*i + width + j + 1] < 0)
				intensity2[width*i + width + j + 1] += 360;

			if (intensity2[width*i + width + j + 1] < 90)
				if (intensity[width*i + width + j + 1] < intensity[width*i + j + 2] || intensity[width*i + width + j + 1] < intensity[width*i + j + 2 * width])
					buf[width*i + width + j + 1] = 0;

			if (intensity2[width*i + width + j + 1] > 180 && intensity2[width*i+width+j+1] < 270)
				if (intensity[width*i + width + j + 1] < intensity[width*i + j + 2] || intensity[width*i + width + j + 1] < intensity[width*i + j + 2 * width])
					buf[width*i + width + j + 1] = 0;

			if (intensity2[width*i + width + j + 1] > 90 && intensity2[width*i + width + j + 1] < 180)
				if (intensity[width*i + width + j + 1] < intensity[width*i + j] || intensity[width*i + width + j + 1] < intensity[width*i + j + 2 + 2 * width])
					buf[width*i + width + j + 1] = 0;

			if (intensity2[width*i + width + j + 1] > 270 && intensity2[width*i + width + j + 1] < 360)
				if (intensity[width*i + width + j + 1] < intensity[width*i + j] || intensity[width*i + width + j + 1] < intensity[width*i + j + 2 + 2 * width])
					buf[width*i + width + j + 1] = 0;

			if (intensity2[width*i + width + j + 1] == 90 || intensity2[width*i + width + j + 1] == 270)
				if (intensity[width*i + width + j + 1] < intensity[width*i + j + 1] || intensity[width*i + width + j + 1] < intensity[width*i + j + 1 + 2 * width])
					buf[width*i + width + j + 1] = 0;

			if (intensity2[width*i + width + j + 1] == 180 || intensity2[width*i + width + j + 1] == 360)
				if (intensity[width*i + width + j + 1] < intensity[width*i + j + width] || intensity[width*i + width + j + 1] < intensity[width*i + j + 2 + width])
					buf[width*i + width + j + 1] = 0;
		}

	return buf;

}

BYTE* historize(BYTE* intensity, int width, int height, long* newsize)
{
	BYTE* buff = new BYTE[*newsize];

	for (int i = 0; i < *newsize; i++)
		buff[i] = 0;
		

	int t_low = 50;
	int t_high = 100;

	for (int i = 0; i < *newsize; i++)
	{
		if (intensity[i] <= t_low)
			buff[i] = 0;
		if (intensity[i] >= t_high)
			buff[i] = 255;
		if (intensity[i] > t_low && intensity[i] < t_high)
			buff[i] = 1;
	}
	
	for (int i = 0; i < *newsize; i++)
		if (buff[i] == 1)
			if (buff[i - width + 1] == 255 && buff[i + width - 1] == 255)
				buff[i] = 255;

	return buff;
}

BYTE* hough(BYTE* intensity, BYTE* intensity2, BYTE* intensity3, int width, int height, long* newsize, long* newsize2)
{
	*newsize = height*width;
	*newsize2 = (height - 1 + width - 1) * 360;

	BYTE* buf2 = new BYTE[*newsize2];
	BYTE* buf3 = new BYTE[10];
	BYTE* buf4 = new BYTE[*newsize];
	BYTE* d2 = new BYTE[10];

	int d, araDegisken = 0;

	for (int i = 0; i < 10; i++)
		buf3[i] = 0, d2[i] = 0;

	for (int i = 0; i < *newsize; i++)
		buf4[i] = intensity[i];

	for (int j = 0; j < (width-1+height-1)*360; j++)
		buf2[j] = 0;


	//hough uzayini olusturma


	for (int i = 0; i < height - 2; i++)
		for (int j = 0; j < width - 2; j++)
			if (intensity2[i*width + width + j + 1] == 255)
			{
				d = abs((i + 1)*sin(float((intensity3[i*width + width + j + 1]))) + (j + 1)*cos(float((intensity3[i*width + width + j + 1]))));
				buf2[d*360 + 360 + 1 + (intensity3[i*width + width + j + 1])] += 1;
			}

	return buf2;   //hough uzayi

	int a[10] = {0,0,0,0,0,0,0,0,0,0}, b = 0;


    //hough uzayindaki en buyuk 5 degeri bulma kismi

	

	for (int j = 0; j<10; j++)                                     
	{
		for (int i = 0; i < int(*newsize2) - 1; i++)
		{
			if (buf2[i] > buf2[i + 1] && buf2[i] > buf3[j])
				buf3[j] = buf2[i], a[j] = i, d2[j] = abs((-360 - 1 - intensity3[i]) / 360);
			else if (buf2[i] < buf2[i + 1] && buf2[i + 1] > buf3[j])
				buf3[j] = buf2[i + 1], a[j] = i + 1, d2[j] = abs((-360 - 1 - intensity3[i]) / 360);

	    }
		for (int k = 0; k < 10; k++)
			for (int g = 0; g < *newsize; g++)
				if (buf2[g] == buf3[k])
					buf2[g] = 0;
	}


    //b kadar ilerleyip aciya gore cizgi cizme kismi


	for (int i = 0; i < 10; i++)                                     
	{
		if (intensity3[a[i]] < 90 && intensity3[a[i]] > 0)
		{
			b = abs(a[i] - d2[i] * (width - 1));
			for (int i = 0; i < 5; i++)
				buf4[b + i*width + i] = 255;
		}
		if (intensity[a[i]] > 180 && intensity[a[i]] < 270)
		{
			b = abs(a[i] + d2[i] * (width - 1));
			for (int i = 0; i < 5; i++)
				buf4[b + i*width + i] = 255;
		}
		if (intensity[a[i]] > 90 && intensity[a[i]] < 180)
		{
			b = abs(a[i] - d2[i] * (width + 1));
			for (int i = 0; i < 5; i++)
				buf4[b + i*width - i] = 255;
		}
		if (intensity[a[i]] > 270 && intensity[a[i]] < 360)
		{
			b = abs(a[i] + d2[i] * (width + 1));
			for (int i = 0; i < 5; i++)
				buf4[b + i*width - i] = 255;
		}
		if (intensity[a[i]] == 360)
		{
			b = a[i] + d2[i];
			for (int i = 0; i < 5; i++)
				buf4[b + i*width] = 255;
		}
		if (intensity[a[i]] == 180)
		{
			b = abs(a[i] - d2[i]);
			for (int i = 0; i < 5; i++)
				buf4[b + i*width] = 255;
		}
		if (intensity[a[i]] == 270)
		{
			b = a[i] + d2[i] * width;
			for (int i = 0; i < 5; i++)
				buf4[b + i] = 255;
		}
		if (intensity[a[i]] == 90)
		{
			b = abs(a[i] - d2[i] * width);
			for (int i = 0; i < 5; i++)
				buf4[b + i] = 255;
		}
	}
	return buf4;
}

BYTE* canny_edge(BYTE* intensity, int width, int height, long* newsize, long* newsize2)
{
	*newsize = height*width;
	*newsize2 = (width - 1 + height - 1) * 360;

	BYTE* buf1 = new BYTE[*newsize];
	BYTE* buf2 = new BYTE[*newsize];
	BYTE* buf3 = new BYTE[*newsize];
	BYTE* buf4 = new BYTE[*newsize];
	BYTE* buf5 = new BYTE[*newsize];
	BYTE* buf6 = new BYTE[*newsize];
	BYTE* buf7 = new BYTE[*newsize2];

	for (int i = 0; i < *newsize; i++)
		buf1[i] = 0, buf2[i] = 0, buf3[i] = 0, buf4[i] = 0, buf5[i] = 0, buf6[i] = 0, buf7[i] = 0;

	for (int i = 0; i < height - 2; i++)
		for (int j = 0; j < width - 2; j++)
			buf1[width*i + width + j + 1] = abs(

			-intensity[width*i + j] 
			-intensity[width*i + j + 1] 
			-intensity[width*i + j + 2] 
			+intensity[width*i + j + 2 * width] 
			+intensity[width*i + j + 1 + 2 * width] 
			+intensity[width*i + j + 2 + 2 * width]);

	for (int i = 0; i < height - 2; i++)
		for (int j = 0; j < width - 2; j++)
			buf2[width*i + width + j + 1] = abs(

			-intensity[width*i + j] 
			+intensity[width*i + j + 2] 
			-intensity[width*i + j + width] 
			+intensity[width*i + j + 2 + width] 
			-intensity[width*i + j + 2 * width] 
			+intensity[width*i + j + 2 + 2 * width]);

	for (int i = 0; i < *newsize; i++)
		buf3[i] = buf1[i] + buf2[i];

	for (int i = 0; i < height - 2; i++)
		for (int j = 0; j < width - 2; j++)
			buf4[width*i + width + j + 1] = atan(float(buf2[width*i + width + j + 1]) / float(buf1[width*i + width + j + 1]))*180/PI;

	buf5 = non_maxima_suppression(buf3, buf4, width, height, newsize);
	buf6 = historize(buf5, width, height, newsize);
	buf7 = hough(intensity, buf6, buf4, width, height, newsize, newsize2);

	//return buf3;   //edge
	//return buf5;   //non-maxima
	//return buf6;   //historize
	return buf7;   //hough


}

/*for (int i = 0; i < width; i++)
for (int j = 0; j < width; j++)
if (buf2[i] < buf2[i + 1])
{
araDegisken = buf2[i + 1];
buf2[i + 1] = buf2[i];
buf2[i] = araDegisken;
}

for (int k = 0; k < 5;k++)
for (int i = 0; i < 5; i++)
{
a[i] = buf2[i];
for (int j = 0; j < i; j++)
if (a[j] == a[j + 1])
a[j + 1] = 0;
}*/