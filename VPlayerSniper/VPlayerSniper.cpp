// VPlayerSniper.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <Windows.h>
#include <vector>

using namespace std;

const int IMG_MAX_SIZE_X = 800;
const int IMG_MAX_SIZE_Y = 600;
const int MAX_RECTS = 100;




int screen_sizeX = 0;
int screen_sizeY = 0;


BYTE screenshot_32b_img[IMG_MAX_SIZE_X * IMG_MAX_SIZE_Y * 4];

BYTE scr8b[IMG_MAX_SIZE_X*IMG_MAX_SIZE_Y];
BYTE scr8b_prev[IMG_MAX_SIZE_X*IMG_MAX_SIZE_Y];
BYTE scr8b_diff[IMG_MAX_SIZE_X*IMG_MAX_SIZE_Y];

RECT rects[MAX_RECTS];

vector<RECT> player_candidates;

RECT playerR;


SIZE calc_stretched_size(int orig_w, int orig_h, int max_w, int  max_h)
{
	
	SIZE sz;
	if (orig_h < orig_w)
	{
		float k = (float)max_w / orig_w;
		sz.cx = max_w;
		sz.cy = (int)((float)max_h * k);
	}
	else
	{
		float k = (float)max_h / orig_h;
		sz.cy = max_h;
		sz.cx = (int)((float)max_w * k);
	}

	return sz;

}

void write_32b_window_pixels(const HWND window, const SIZE max_size, BYTE *screenshot_32b_img)
{
	RECT window_rect;
	HDC hScreen = GetDC(window);
	GetWindowRect(window, &window_rect);

	//SIZE stretched_size = calc_stretched_size(window_rect.right - window_rect.left,
		//window_rect.bottom - window_rect.top, max_size.cx, max_size.cy);

	

	HDC hdcMem = CreateCompatibleDC(hScreen);
	HBITMAP hBitmap = CreateCompatibleBitmap(hScreen, max_size.cx, max_size.cy);
	HGDIOBJ hOld = SelectObject(hdcMem, hBitmap);
	
	StretchBlt(hdcMem, 0, 0, max_size.cx, max_size.cy, hScreen, 0, 0,
		window_rect.right - window_rect.left, window_rect.bottom - window_rect.top, SRCCOPY | CAPTUREBLT);
	//BitBlt(hdcMem, 0, 0, stretched_size.cx, stretched_size.cy, hScreen, 0, 0, SRCCOPY);
	SelectObject(hdcMem, hOld);

	BITMAPINFOHEADER bmi = { 0 };
	bmi.biSize = sizeof(BITMAPINFOHEADER);
	bmi.biPlanes = 1;
	bmi.biBitCount = 32;
	bmi.biWidth = max_size.cx;
	bmi.biHeight = -max_size.cy;
	bmi.biCompression = BI_RGB;
	bmi.biSizeImage = 0;// 3 * ScreenX * ScreenY;

	//if (screenshot_24b_img && (screen_sizeX != size_x || screen_sizeY != size_y))
	//	free(screenshot_24b_img);
	//screenshot_24b_img = (BYTE*)malloc(4 * size_x * size_y);

	GetDIBits(hdcMem, hBitmap, 0, max_size.cy, screenshot_32b_img, (BITMAPINFO*)&bmi, DIB_RGB_COLORS);

	ReleaseDC(GetDesktopWindow(), hScreen);
	DeleteDC(hdcMem);
	DeleteObject(hBitmap);
}


void Test_save_bmp_32(TCHAR *path, BYTE *pixels)
{
	BITMAPINFOHEADER bmi = { 0 };
	bmi.biSize = sizeof(BITMAPINFOHEADER);
	bmi.biPlanes = 1;
	bmi.biBitCount = 32;
	bmi.biWidth = IMG_MAX_SIZE_X;
	bmi.biHeight = -IMG_MAX_SIZE_Y;
	bmi.biCompression = BI_RGB;
	bmi.biSizeImage = 0;// 3 * ScreenX * ScreenY;

	DWORD dwBmpSize = ((IMG_MAX_SIZE_X * 32 + 31) / 32) * 4 *
		IMG_MAX_SIZE_Y;

	DWORD dwSizeofDIB = dwBmpSize + sizeof(BITMAPFILEHEADER) +
		sizeof(BITMAPINFOHEADER);

	BITMAPFILEHEADER bmfh;

	bmfh.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) +
		(DWORD)sizeof(BITMAPINFOHEADER);

	//Size of the file
	bmfh.bfSize = dwSizeofDIB;

	//bfType must always be BM for Bitmaps
	bmfh.bfType = 0x4D42; //BM   

	HANDLE hFile = CreateFile(path,
		GENERIC_WRITE,
		0,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL, NULL);


	DWORD dwBytesWritten = 0;
	WriteFile(hFile, (LPSTR)&bmfh, sizeof(BITMAPFILEHEADER), &dwBytesWritten, NULL);
	WriteFile(hFile, (LPSTR)&bmi, sizeof(BITMAPINFOHEADER), &dwBytesWritten, NULL);
	WriteFile(hFile, (LPSTR)pixels, dwBmpSize, &dwBytesWritten, NULL);

	CloseHandle(hFile);
}

void Test_save_bmp_8(TCHAR *path, BYTE *pixels_8b)
{
	static BYTE pixels32[IMG_MAX_SIZE_X * IMG_MAX_SIZE_Y * 4];
	memset(pixels32, 0, IMG_MAX_SIZE_X * IMG_MAX_SIZE_Y * 4);

	for (int x = 0; x < IMG_MAX_SIZE_X; x++)
	{
		for (int y = 0; y < IMG_MAX_SIZE_Y; y++)
		{
			pixels32[(y*IMG_MAX_SIZE_X + x) * 4 + 0] = pixels_8b[y*IMG_MAX_SIZE_X + x];
			pixels32[(y*IMG_MAX_SIZE_X + x) * 4 + 1] = pixels_8b[y*IMG_MAX_SIZE_X + x];
			pixels32[(y*IMG_MAX_SIZE_X + x) * 4 + 2] = pixels_8b[y*IMG_MAX_SIZE_X + x];
		}
	}
	Test_save_bmp_32(path, pixels32);
}

void get_wnd_32b_screenshot(HWND wnd, BYTE *pixels)
{
	memset(pixels, 0, IMG_MAX_SIZE_X * IMG_MAX_SIZE_Y * 4);
	write_32b_window_pixels(wnd, { IMG_MAX_SIZE_X, IMG_MAX_SIZE_Y }, pixels);
}

void conv_pixels_32b_to_8b(BYTE *pixels_32b, BYTE *pixels_8b)
{

	for (int x = 0; x < IMG_MAX_SIZE_X; x++)
	{
		for (int y = 0; y < IMG_MAX_SIZE_Y; y++)
		{
			pixels_8b[y*IMG_MAX_SIZE_X + x] =
				(pixels_32b[(y*IMG_MAX_SIZE_X + x) * 4 + 0] +
					pixels_32b[(y*IMG_MAX_SIZE_X + x) * 4 + 1] +
					pixels_32b[(y*IMG_MAX_SIZE_X + x) * 4 + 2]) / 3;
		}
	}
}

void calc_8b_pixels_diff(const BYTE *new_pixels, const BYTE *old_pixels, BYTE *diff_pixels)
{
	for (int x = 0; x < IMG_MAX_SIZE_X; x++)
	{
		for (int y = 0; y < IMG_MAX_SIZE_Y; y++)
		{
			diff_pixels[y*IMG_MAX_SIZE_X + x] = new_pixels[y*IMG_MAX_SIZE_X + x] ^ old_pixels[y*IMG_MAX_SIZE_X + x];
		}
	}
}

bool point_in_rect(const POINT &p, const RECT &r)
{
	return p.x >= r.left && p.x <= r.right && p.y >= r.top && p.y <= r.bottom;
}

bool rects_intersected(const RECT &lr, const RECT &rr)
{
	return point_in_rect(POINT{ lr.left, lr.top }, rr) ||
		point_in_rect(POINT{ lr.right, lr.top }, rr) ||
		point_in_rect(POINT{ lr.left, lr.bottom }, rr) ||
		point_in_rect(POINT{ lr.right, lr.bottom }, rr);

}

//returns rects
//rects can be dispensing in array
void calc_diff_rects(BYTE *diff_pixels, int r, RECT *rects)
{
	for (int x = 0; x < IMG_MAX_SIZE_X; x++)
	{
		for (int y = 0; y < IMG_MAX_SIZE_Y; y++)
		{
			if (diff_pixels[y*IMG_MAX_SIZE_X + x] > 0)
			{
				RECT rect;

				
				while (1)
				{
					
				}
			}
		}
	}
}

int main()
{
	int i = 0;
	RECT zero_rect;
	memset(&zero_rect, 0, sizeof(RECT));

	Sleep(5000);
	memset(scr8b_prev, 0, sizeof(scr8b_prev));

	while (1)
	{

		//TODO: speed check. if not anought frame rate then stop detection.		
		get_wnd_32b_screenshot(GetDesktopWindow(), screenshot_32b_img);
		conv_pixels_32b_to_8b(screenshot_32b_img, scr8b);
		calc_8b_pixels_diff(scr8b, scr8b_prev, scr8b_diff);
		memcpy(scr8b_prev, scr8b, sizeof(scr8b_prev));
		//Test_save_bmp_8(L"C:\\temp\\src0001.bmp", scr8b_diff);
		
		calc_diff_rects(scr8b_diff, 4, rects);


		//printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
		for (int j = 0; j < IMG_MAX_SIZE_X*IMG_MAX_SIZE_Y; j++)
		{
			if (0 == memcmp(&rects[j], &zero_rect, sizeof(RECT)))
			{
				continue;
			}
			printf("%ix%i\n", rects[j].left, rects[j].top);
		}
			
		//update_player_candidates(diff_rects, player_candidates);
		//update_player_rect(player_candidates, playerR);
		if (i > 100)
			break;
		Sleep(500);
	}


    return 0;
}

