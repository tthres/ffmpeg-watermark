#include "watermark.h"

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
using namespace std;


watermark::watermark() : nWidth(0), nHeight(0) 
					   , transparence(255)
					   , offsetX(0), offsetY(0) 
					   , drawText("Ĭ��") , fontSize(1000), slide(0)
{
	// ����
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
	cout << "λͼ�ļ�ͷ:" << endl;
	cout << "bfType value is " << pBmpHead.bfType << endl;
	cout << "�ļ���С:" << pBmpHead.bfSize << endl;
	cout << "������_1:" << pBmpHead.bfReserved1 << endl;
	cout << "������_2:" << pBmpHead.bfReserved2 << endl;
	cout << "ʵ��λͼ���ݵ�ƫ���ֽ���:" << pBmpHead.bfOffBits << endl << endl;
}

void watermark::showBmpInforHead(tagBITMAPINFOHEADER pBmpInforHead)
{
	cout << "λͼ��Ϣͷ:" << endl;
	cout << "�ṹ��ĳ���:" << pBmpInforHead.biSize << endl;
	cout << "λͼ��:" << pBmpInforHead.biWidth << endl;
	cout << "λͼ��:" << pBmpInforHead.biHeight << endl;
	cout << "biPlanesƽ����:" << pBmpInforHead.biPlanes << endl;
	cout << "biBitCount������ɫλ��:" << pBmpInforHead.biBitCount << endl;
	cout << "ѹ����ʽ:" << pBmpInforHead.biCompression << endl;
	cout << "biSizeImageʵ��λͼ����ռ�õ��ֽ���:" << pBmpInforHead.biSizeImage << endl;
	cout << "X����ֱ���:" << pBmpInforHead.biXPelsPerMeter << endl;
	cout << "Y����ֱ���:" << pBmpInforHead.biYPelsPerMeter << endl;
	cout << "ʹ�õ���ɫ��:" << pBmpInforHead.biClrUsed << endl;
	cout << "��Ҫ��ɫ��:" << pBmpInforHead.biClrImportant << endl;

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
	//�������ļ�����face��simhei.ttf�Ǻ���
	error = FT_New_Face(pFTLib, "C:/Windows/Fonts/simhei.ttf", 0, &pFTFace);
	if (error)
	{
		std::cout << "�����ʧ��" << endl;
	}
	FT_Set_Char_Size(pFTFace, 0, 16 * 64, fontSize, fontSize);//���������С
	//FT_Set_Pixel_Sizes(pFTFace,0,16 );
	FT_Glyph glyph = nullptr;
	char* szAnsi = drawText;//������ת��ΪUnicode����
	//Ԥת�����õ�����ռ�Ĵ�С
	int wcsLen = ::MultiByteToWideChar(CP_ACP, NULL, szAnsi, strlen(szAnsi), NULL, 0);
	// cout << "wcsLen=" << wcsLen << endl;
	//����ռ�Ҫ��'\0'�����ռ䣬MultiByteToWideChar�����'\0'�ռ�
	wchar_t* wszString = new wchar_t[wcsLen + 1];
	//ת��
	::MultiByteToWideChar(CP_ACP, NULL, szAnsi, strlen(szAnsi), wszString, wcsLen);
	//������'\0'
	wszString[wcsLen] = '\0';
	WORD word;
	if (srcFrame != NULL) {

		//����ͼƬ����������  
		nWidth = srcFrame->width;
		nHeight = srcFrame->height;

		//���������ܴ�С
		DWORD size = nWidth * nHeight;
		
		 // ���Ƶ�dstFrame
		dstFrame->format = srcFrame->format;
		dstFrame->width = srcFrame->width;
		dstFrame->height = srcFrame->height;
		dstFrame->channels = srcFrame->channels;
		dstFrame->channel_layout = srcFrame->channel_layout;
		dstFrame->nb_samples = srcFrame->nb_samples;
		av_frame_get_buffer(dstFrame, 0);
		av_frame_copy(dstFrame, srcFrame);
		av_frame_copy_props(dstFrame, srcFrame);

		offsetX += slide; // ����
		offsetY += slide; // ����

		//����ƫ����������������ʾ
		int bitmap_width_sum = 0;
		//forѭ��ʵ��һ����һ���ֲ��뵽ͼƬ��
		for (int k = 0; k < wcsLen; k++) {
			//�����ڴ�飬��wszString�д洢������һ��һ��ȡ���������Ƶ�word�У��ѷ����ȡ����λͼ
			memcpy(&word, wszString + k, 2);
			//��ȡһ������λͼ��face��
			FT_Load_Glyph(pFTFace, FT_Get_Char_Index(pFTFace, word), FT_LOAD_DEFAULT);
			error = FT_Get_Glyph(pFTFace->glyph, &glyph);
			//  convert glyph to bitmap with 256 gray
			FT_Glyph_To_Bitmap(&glyph, ft_render_mode_normal, 0, 1);
			FT_BitmapGlyph bitmap_glyph = (FT_BitmapGlyph)glyph;
			//�õ������bitmapλͼ
			FT_Bitmap& bitmap = bitmap_glyph->bitmap;
			
			// �����������ص���ɫ R��G��B
			int R = 0, G = 0, B = 0;

			bool insertFlag = true;
			//���ֲ��뵽ͼƬ�У�ÿ�����м���15�����أ����������Ͻ�offserX, offsetYƫ����
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

// ���ͼƬˮӡ
int watermark::BmpinsertToBmp(AVFrame* srcFrame, AVFrame* dstFrame, IMAGEDATA *arrayColor, int bW, int bH) {
	if (srcFrame != NULL) {

		//����ͼƬ����������  
		nWidth = srcFrame->width;
		nHeight = srcFrame->height;

		//���������ܴ�С
		DWORD size = nWidth * nHeight;

		// ���Ƶ�dstFrame
		dstFrame->format = srcFrame->format;
		dstFrame->width = srcFrame->width;
		dstFrame->height = srcFrame->height;
		dstFrame->channels = srcFrame->channels;
		dstFrame->channel_layout = srcFrame->channel_layout;
		dstFrame->nb_samples = srcFrame->nb_samples;
		av_frame_get_buffer(dstFrame, 32);
		av_frame_copy(dstFrame, srcFrame);
		av_frame_copy_props(dstFrame, srcFrame);

		// ��bmp���뵽֡
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
