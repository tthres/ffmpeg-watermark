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
	int transparence; // 0:完全透明----255：不透明

	// 从（offsetX, offsetY)开始画水印
	int offsetX;	  
	int offsetY;

	char drawText[256];
	int fontSize;
	int slide;
public:
	watermark();
	int flag;		  // 1:图片水印	2:文字水印
	//显示位图文件头信息   
	void showBmpHead(BITMAPFILEHEADER pBmpHead);
	void showBmpInforHead(tagBITMAPINFOHEADER pBmpInforHead);
	// 添加图片水印
	int BmpinsertToBmp(AVFrame* srcFrame, AVFrame* dstFrame, IMAGEDATA* arrayColor, int bW, int bH);
	// 添加文字水印
	int WordInsertToBmp(AVFrame* srcFrame, AVFrame* dstFrame);
};

