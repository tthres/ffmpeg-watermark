#include "watermark.h"

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
using namespace std;


watermark::watermark() : nWidth(0), nHeight(0) 
					   , transparence(255)
					   , offsetX(0), offsetY(0) 
					   , drawText("默认") , fontSize(1000), slide(0)
{
	// 滑动
	// slide = 1;

	char c;
	cout << "watermark engine init..." << endl;
	cout << "Type of watermark?" << endl;
	cout << "[1] picture" << endl;
	cout << "[2] word" << endl;
	cin >> flag;
	cin.get();
	cout << "nontransparency : 0 - 255 (enter default)..." << endl;
	
	cin.get(c);
	if (c != '\n') {
		cin.putback(c);
		cin >> transparence;
		cin.get();
	}
	cout << "nontransparence = " << transparence << endl;
	cout << endl;
	if (flag == 2) {
		cout << "draw text (enter default):... ";
		cin.get(c);
		if (c != '\n') {
			cin.putback(c);
			cin >> drawText;
			cin.get();
		}
		cout << "draw text : " << " [ " << drawText << " ] " <<  endl;
		cout << "Setting the Font Size (enter default):... " << endl;
		
		cin.get(c);
		if (c != '\n') {
			cin.putback(c);
			cin >> fontSize;
			cin.get();    
		}
		cout << "the Font Size = " << fontSize << endl;
	}
	cout << "pointLeftTop X and Y..." << endl;
	cin >> offsetY >> offsetX;
	cout << "drawing at ( " << offsetX << " , " << offsetY << " )..." << endl;
	cout << endl;
}

#ifdef DEBUG
void watermark::showBmpHead(BITMAPFILEHEADER pBmpHead)
{
	cout << "位图文件头:" << endl;
	cout << "bfType value is " << pBmpHead.bfType << endl;
	cout << "文件大小:" << pBmpHead.bfSize << endl;
	cout << "保留字_1:" << pBmpHead.bfReserved1 << endl;
	cout << "保留字_2:" << pBmpHead.bfReserved2 << endl;
	cout << "实际位图数据的偏移字节数:" << pBmpHead.bfOffBits << endl << endl;
}

void watermark::showBmpInforHead(tagBITMAPINFOHEADER pBmpInforHead)
{
	cout << "位图信息头:" << endl;
	cout << "结构体的长度:" << pBmpInforHead.biSize << endl;
	cout << "位图宽:" << pBmpInforHead.biWidth << endl;
	cout << "位图高:" << pBmpInforHead.biHeight << endl;
	cout << "biPlanes平面数:" << pBmpInforHead.biPlanes << endl;
	cout << "biBitCount采用颜色位数:" << pBmpInforHead.biBitCount << endl;
	cout << "压缩方式:" << pBmpInforHead.biCompression << endl;
	cout << "biSizeImage实际位图数据占用的字节数:" << pBmpInforHead.biSizeImage << endl;
	cout << "X方向分辨率:" << pBmpInforHead.biXPelsPerMeter << endl;
	cout << "Y方向分辨率:" << pBmpInforHead.biYPelsPerMeter << endl;
	cout << "使用的颜色数:" << pBmpInforHead.biClrUsed << endl;
	cout << "重要颜色数:" << pBmpInforHead.biClrImportant << endl;

}
#endif // DEBUG


int watermark::WordInsertToBmp(AVFrame *srcFrame, AVFrame *dstFrame)
{
	FT_Library    pFTLib = NULL;
	FT_Face        pFTFace = NULL;
	FT_Error    error = 0;
	//Init FreeType Lib to manage memory
	error = FT_Init_FreeType(&pFTLib);
	if (error)
	{
		pFTLib = 0;
		printf(" There is some error when Init Library ");
		return   -1;
	}
	//从字体文件创建face，simhei.ttf是黑体
	error = FT_New_Face(pFTLib, "C:/Windows/Fonts/simhei.ttf", 0, &pFTFace);
	if (error)
	{
		std::cout << "字体打开失败" << endl;
	}
	FT_Set_Char_Size(pFTFace, 0, 16 * 64, fontSize, fontSize);//设置字体大小
	//FT_Set_Pixel_Sizes(pFTFace,0,16 );
	FT_Glyph glyph = nullptr;
	char* szAnsi = drawText;//将中文转换为Unicode编码
	//预转换，得到所需空间的大小
	int wcsLen = ::MultiByteToWideChar(CP_ACP, NULL, szAnsi, strlen(szAnsi), NULL, 0);
	// cout << "wcsLen=" << wcsLen << endl;
	//分配空间要给'\0'留个空间，MultiByteToWideChar不会给'\0'空间
	wchar_t* wszString = new wchar_t[wcsLen + 1];
	//转换
	::MultiByteToWideChar(CP_ACP, NULL, szAnsi, strlen(szAnsi), wszString, wcsLen);
	//最后加上'\0'
	wszString[wcsLen] = '\0';
	WORD word;
	if (srcFrame != NULL) {

		//读出图片的像素数据  
		nWidth = srcFrame->width;
		nHeight = srcFrame->height;

		//计算像素总大小
		DWORD size = nWidth * nHeight;
		
		 // 复制到dstFrame
		dstFrame->format = srcFrame->format;
		dstFrame->width = srcFrame->width;
		dstFrame->height = srcFrame->height;
		dstFrame->channels = srcFrame->channels;
		dstFrame->channel_layout = srcFrame->channel_layout;
		dstFrame->nb_samples = srcFrame->nb_samples;
		av_frame_get_buffer(dstFrame, 0);
		av_frame_copy(dstFrame, srcFrame);
		av_frame_copy_props(dstFrame, srcFrame);

		offsetX += slide; // 向下
		offsetY += slide; // 向右

		//字体偏移量，用做字体显示
		int bitmap_width_sum = 0;
		//for循环实现一个字一个字插入到图片中
		for (int k = 0; k < wcsLen; k++) {
			//复制内存块，把wszString中存储的文字一个一个取出来，复制到word中，已方便读取字体位图
			memcpy(&word, wszString + k, 2);
			//读取一个字体位图到face中
			FT_Load_Glyph(pFTFace, FT_Get_Char_Index(pFTFace, word), FT_LOAD_DEFAULT);
			error = FT_Get_Glyph(pFTFace->glyph, &glyph);
			//  convert glyph to bitmap with 256 gray
			FT_Glyph_To_Bitmap(&glyph, ft_render_mode_normal, 0, 1);
			FT_BitmapGlyph bitmap_glyph = (FT_BitmapGlyph)glyph;
			//得到字体的bitmap位图
			FT_Bitmap& bitmap = bitmap_glyph->bitmap;
			
			// 设置文字像素的颜色 R、G、B
			int R = 0, G = 0, B = 0;

			bool insertFlag = true;
			//把字插入到图片中，每个字中间间隔15个像素，并且离左上角offserX, offsetY偏移量
			for (int i = 0; i < bitmap.rows; ++i)
			{
				for (int j = 0; j < bitmap.width; ++j)
				{
#if 1				
					if (bitmap.buffer[i * bitmap.width + j] != 0 && ((i + offsetX) < nHeight) && ((j + bitmap_width_sum + offsetY) < nWidth))
					{
						int Y, U, V;
						Y = ((66 * R + 129 * G + 25 * B + 128) >> 8) + 16;
						U = ((-38 * R - 74 * G + 112 * B + 128) >> 8) + 128;
						V = ((112 * R - 94 * G - 18 * B + 128) >> 8) + 128;
						
						int Y1, U1, V1;
						Y1 = dstFrame->data[0][(i + offsetX) * nWidth + j + bitmap_width_sum + offsetY];
						U1 = dstFrame->data[1][(i + offsetX) * nWidth / 4 + (j + bitmap_width_sum + offsetY) / 2];
						V1 = dstFrame->data[2][(i + offsetX) * nWidth / 4 + (j + bitmap_width_sum + offsetY) / 2];
						Y = (Y * transparence + Y1 * (255 - transparence)) / 255;
						U = (U * transparence + U1 * (255 - transparence)) / 255;
						V = (V * transparence + V1 * (255 - transparence)) / 255;
						
						
						dstFrame->data[0][(i + offsetX) * nWidth + j + bitmap_width_sum + offsetY] = (unsigned char)((Y < 0) ? 0 : ((Y > 255) ? 255 : Y)); // (i + X, j + bitmap + Y)
						if ((i + offsetX) % 2 == 0 && (j + bitmap_width_sum + offsetY) % 2 == 0) {
							dstFrame->data[1][(i + offsetX) * nWidth / 4 + (j + bitmap_width_sum + offsetY) / 2] = (unsigned char)((U < 0) ? 0 : ((U > 255) ? 255 : U));
						}
						else if ((i + offsetX) % 2 == 0 && (j + bitmap_width_sum + offsetY) % 2 == 1) {
							dstFrame->data[2][(i + offsetX) * nWidth / 4 + (j + bitmap_width_sum + offsetY) / 2] = (unsigned char)((V < 0) ? 0 : ((V > 255) ? 255 : V));
						}
						

					}
#endif
				}
			}
			bitmap_width_sum += bitmap.width + 15;
			//  free glyph
			FT_Done_Glyph(glyph);
			glyph = NULL;
			
		}
		
		//  free glyph
		FT_Done_Glyph(glyph);
		glyph = NULL;
		//  free face
		FT_Done_Face(pFTFace);
		pFTFace = NULL;

		//  free FreeType Lib
		FT_Done_FreeType(pFTLib);
		pFTLib = NULL;

		
	}
	else {
		cout << "file open error!" << endl;
		return -1;
	}
	if (wszString) delete[] wszString;
	//  free glyph
	FT_Done_Glyph(glyph);
	glyph = NULL;
	//  free face
	FT_Done_Face(pFTFace);
	pFTFace = NULL;

	//  free FreeType Lib
	FT_Done_FreeType(pFTLib);
	pFTLib = NULL;
	// _CrtDumpMemoryLeaks();
	return 0;
}

// 添加图片水印
int watermark::BmpinsertToBmp(AVFrame* srcFrame, AVFrame* dstFrame, IMAGEDATA *arrayColor, int bW, int bH) {
	if (srcFrame != NULL) {

		//读出图片的像素数据  
		nWidth = srcFrame->width;
		nHeight = srcFrame->height;

		//计算像素总大小
		DWORD size = nWidth * nHeight;

		// 复制到dstFrame
		dstFrame->format = srcFrame->format;
		dstFrame->width = srcFrame->width;
		dstFrame->height = srcFrame->height;
		dstFrame->channels = srcFrame->channels;
		dstFrame->channel_layout = srcFrame->channel_layout;
		dstFrame->nb_samples = srcFrame->nb_samples;
		av_frame_get_buffer(dstFrame, 32);
		av_frame_copy(dstFrame, srcFrame);
		av_frame_copy_props(dstFrame, srcFrame);

		// 把bmp插入到帧
		for (int i = 0; i < bH && ((i + offsetX) < nHeight); i++) {
			for (int j = 0; j < bW && ((j + offsetY) < nWidth); j++) {
				int R, G, B;
				R = arrayColor[i * bW + j].red;
				G = arrayColor[i * bW + j].green;
				B = arrayColor[i * bW + j].blue;
				int Y, U, V;
				Y = ((66 * R + 129 * G + 25 * B + 128) >> 8) + 16;
				U = ((-38 * R - 74 * G + 112 * B + 128) >> 8) + 128;
				V = ((112 * R - 94 * G - 18 * B + 128) >> 8) + 128;

				int Y1, U1, V1;
				Y1 = dstFrame->data[0][(i + offsetX) * nWidth + j + offsetY];
				U1 = dstFrame->data[1][(i + offsetX) * nWidth / 4 + (j + offsetY) / 2];
				V1 = dstFrame->data[2][(i + offsetX) * nWidth / 4 + (j + offsetY) / 2];

				Y = (Y * transparence + Y1 * (255 - transparence)) / 255;
				U = (U * transparence + U1 * (255 - transparence)) / 255;
				V = (V * transparence + V1 * (255 - transparence)) / 255;

				dstFrame->data[0][(i + offsetX) * nWidth + j + offsetY] = (unsigned char)((Y < 0) ? 0 : ((Y > 255) ? 255 : Y));

				dstFrame->data[0][(i + offsetX) * nWidth + j + offsetY] = (unsigned char)((Y < 0) ? 0 : ((Y > 255) ? 255 : Y)); // (i + X, j + bitmap + Y)
				if ((i + offsetX) % 2 == 0 && (j + offsetY) % 2 == 0) {
					dstFrame->data[1][(i + offsetX) * nWidth / 4 + (j + offsetY) / 2] = (unsigned char)((U < 0) ? 0 : ((U > 255) ? 255 : U));
				}
				else if ((i + offsetX) % 2 == 0 && (j + offsetY) % 2 == 1) {
					dstFrame->data[2][(i + offsetX) * nWidth / 4 + (j + offsetY) / 2] = (unsigned char)((V < 0) ? 0 : ((V > 255) ? 255 : V));
				}
				
			}
		}
	}
	else {
		cout << "file open error!" << endl;
		return -1;
	}
	return 0;
}
