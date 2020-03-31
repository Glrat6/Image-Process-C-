// Image_Process1.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <windows.h>
#include <math.h>

//User defined includes
#include "imge_bmp.h"

using namespace std;

int _tmain(int argc, _TCHAR* argv[])
{
	LPCTSTR input, output, output2;
	int Width, Height;
	long Size, new_size, new_size2;

	cout << "Haydi Bismillah" << endl;
	input = L"C://Users//pc//Desktop//a//aaa.bmp";

	
	BYTE* buffer = LoadBMP(&Width, &Height, &Size, input);
	BYTE* raw_intensity = ConvertBMPToIntensity(buffer, Width, Height);
	
	BYTE* raw2_intensity = canny_edge(raw_intensity, Width, Height, &new_size, &new_size2);

	char ch;
	cout << "Sonucu diske kaydetsin mi? E/H:"; cin >> ch;
	if ((ch == 'E') || (ch == 'e')) {

		BYTE* display_imge = ConvertIntensityToBMP(raw2_intensity, Width, Height, &new_size);
		//output = L"C://Users//pc//Desktop//a//edge.bmp";
		//output = L"C://Users//pc//Desktop//a//edge2.bmp";
		//output = L"C://Users//pc//Desktop//a//edge3.bmp";
		output = L"C://Users//pc//Desktop//a//hough.bmp";
		//output = L"C://Users//pc//Desktop//a//hough2.bmp";

		if (SaveBMP(display_imge, Width, Height, new_size, output))
			cout << " Output Image was successfully saved" << endl;
		else cout << "Error on saving image" << endl;
		delete[] display_imge;
	}


	delete[] buffer;
	delete[] raw_intensity;
	
	return 0;
}







