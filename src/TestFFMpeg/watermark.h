#pragma once

extern "C" {
#include <libavutil/frame.h>
}

#include <iostream>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <ft2build.h>
#include <memory.h>
#include FT_FREETYPE_H
#include <freetype/freetype.h>
#include <freetype/ftglyph.h>
#include "IMAGEDATA.h"

class watermark
{
private:
	
	LONG nWidth;
	LONG nHeight;
	int transparence; // 0:��ȫ͸��----255����͸��

	// �ӣ�offsetX, offsetY)��ʼ��ˮӡ
	int offsetX;	  
	int offsetY;

	char drawText[256];
	int fontSize;
	int slide;
public:
	watermark();
	int flag;		  // 1:ͼƬˮӡ	2:����ˮӡ
	//��ʾλͼ�ļ�ͷ��Ϣ   
	void showBmpHead(BITMAPFILEHEADER pBmpHead);
	void showBmpInforHead(tagBITMAPINFOHEADER pBmpInforHead);
	// ���ͼƬˮӡ
	int BmpinsertToBmp(AVFrame* srcFrame, AVFrame* dstFrame, IMAGEDATA* arrayColor, int bW, int bH);
	// �������ˮӡ
	int WordInsertToBmp(AVFrame* srcFrame, AVFrame* dstFrame);
};

