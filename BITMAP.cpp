#define _CRT_SECURE_NO_WARNINGS
#include <locale>
#include <stdio.h>

using namespace std;
#pragma pack(push,1)

struct BITMAPFILEHEADER
{
	unsigned short Type; // �BM� 0x4D42
	unsigned long Size; // ������ ����� � ������, BitCount*Height*Width+ OffsetBits
	unsigned short Reserved1; // ��������������; ������ ���� ����
	unsigned short Reserved2; // ��������������; ������ ���� ����
	unsigned long OffsetBits; // �������� ������ �� ������ ����� � ������

	// = sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER)
};

#pragma pack(pop)

#pragma pack(push,1)

struct BITMAPINFOHEADER
{
	unsigned long Size; // ����� ������ ����������� ��� ��������� = 40
	unsigned long Width; // ������ ��������� ������� � ��������
	unsigned long Height; // ������ ��������� ������� � ��������
	unsigned short Planes; // ����� ���������� �������� ���������� = 1
	unsigned short BitCount; // ������� �����, ����� ��� �� ����� = 0,1,4,8,16,24,32
	unsigned long Compression; // ��� ������ = 0 ��� ��������� �����������
	unsigned long SizeImage; // ������ ����������� � ������ BitCount*Height*Width
	unsigned long XPelsPerMeter; // ����������� ����������� �� �����������
	unsigned long YPelsPerMeter; // ����������� ����������� �� ���������
	unsigned long ColorUsed; // ����� �������� ������������ ������. ���� ��� ����� = 0
	unsigned long ColorImportant; // ����� ����������� ������ = 0
};

#pragma pack(pop)

#pragma pack(push,1)

struct RGBTRIPLE
{
	unsigned char Blue;
	unsigned char Green;
	unsigned char Red;
};

#pragma pack(pop)

#pragma pack(push,1)

struct RGBQUAD
{
	unsigned char Blue;
	unsigned char Green;
	unsigned char Red;
	unsigned char Reserved;
};

#pragma pack(pop)

class Image {
	BITMAPINFOHEADER BMInfoHeader; // �������������� ��������� �����������
	RGBTRIPLE* Rgbtriple; // ��������� ������ � ��������� �������� ���� RGBTRIPLE
	RGBQUAD* Palette; // ������� �����������(������������ ������, ���� ������� ����� ����� 1, 4 ��� 8)
	void setEmptyImageParams(); // ����������� ���������� ����������, ��������������� ������� �����������
	int loadImageDataFromFile(FILE* f); // ��������� ����������� �� ����������� �����
	void saveImageDataToFile(FILE* f); // ��������� ����������� � ����
	void setHeaderAndAllocateMemory(int Width, int Height, int BitCount); // ������������� ��������� ��������� ����������� (BMInfoHeader)
	// �������� ������ ��� �����������
	// ���� ��������� ��������, ������� � ��������� �������
	void initializeFromImage(const Image& Inp); // ������ ����� ����������� (��������� ��������� � �������� ������)
	void copyDataFromImage(const Image& Inp); // �������� ������ ��� ���������� ���������� �����������
	void copyAndConvertDataFromImage(const Image& Inp); // �������� � ����������� ������ ����������� ����������� � ������� ������

	bool isAllowedBitCount(unsigned short BitCount) // ��������� ������������ �������� �����������
	{
		return BitCount == 24 || BitCount == 8 || BitCount == 4 || BitCount == 1; // 3-�� �������
	}

	int getAdditionalRowOffsetInFile(int Width, unsigned short BitCount) // ������� ������� ����� ���������� � ���� ���� ����� ������ ������ �����������, ����� �������� ������ ������ ������� 4
	{
		int remainder = getImageRowSize(Width, BitCount) % 4; // ��������� ������� �� ������� ����� ������ �� 
		return remainder ? 4 - remainder : 0; // ���������� ���������� �� 4
	}

	int getImageRowSize(int Width, unsigned short BitCount) // ���������� ������� ���� ��������� ��� ������ ������ ����������� � ����
	{
		// Width * BitCount ���� ����������� ����� ��� ��� �������� ������ �����������
		// ((Width * BitCount + 7) / 8) ��������� ���������� ���� � ��������� � ������� �������
		return (Width * BitCount + 7) / 8;
	}

	int getTotalImageSize(int Width, int Height, unsigned short BitCount) // ���������� ������ ������ ������ ��� ������ � ����
	{
		// ��������� ������� ���� ��������� ��� ������ ������ ����������� � ����, ��������� ������ ���������� ���� ��� ��������� 4 � �������� �� ���������� �����
		return (getImageRowSize(BMInfoHeader.Width, BMInfoHeader.BitCount) + getAdditionalRowOffsetInFile(Width, BitCount)) * Height;
	}

	unsigned char getGrayscaleColor(unsigned char Red, unsigned char Green, unsigned char Blue) // ��������� �������� ������ ��� ����������� �����
	{
		int result = (int)(Red * 0.299 + Green * 0.597 + Blue * 0.114); // ������� �� ���������
		if (result > 255) result = 255;// ��������� ����� �� ������� �����
		return (unsigned char)result;
	}

	unsigned char getGrayscaleColor(RGBTRIPLE color) // ��������� �������� ������ ��� ����������� �����
	{
		return getGrayscaleColor(color.Red, color.Green, color.Blue);
	}

	unsigned char getGrayscaleColor(RGBQUAD color) // ��������� �������� ������ ��� ����������� �����
	{
		return getGrayscaleColor(color.Red, color.Green, color.Blue);
	}

	unsigned char getNearestPaletteColorIndex(unsigned char grayscaleColor) // �������� ������� ���� ��������� ���� � ������� (��� ��� ��� ����������� �� ����������� �������� ������)
	{
		int minIndex = 0; // ��������� ������������ ������� ��� ������
		int maxIndex = BMInfoHeader.ColorUsed - 1; // ��������� ������������� ������� ��� ������
		while (maxIndex >= minIndex) // ���������� ������ ���� ����������� ������ �� �������� �� ������������
		// (� ���� ������ �������� grayscaleColor ��������� ����� Palette[minIndex - 1] � Palette[minIndex], ��� ��� ������� ������
		// ��������� ����� �� �������� minIndex), ���� ���� �� ������ ������� ��������
		{
			int middleIndex = (minIndex + maxIndex) / 2; // ���������� �������� ������� ����� ��������
			if (Palette[middleIndex].Red == grayscaleColor) // ��������� �� ����� �� ������� �������� (��� ��� ������� ������� �� �������� ������
			// Palette[middleIndex].Red == Palette[middleIndex].Green == Palette[middleIndex].Blue)
			{
				return middleIndex; // ���������� ������ ������� � ������ ������
			}
			else if (Palette[middleIndex].Red < grayscaleColor) // ���� ������� �������� ������� ��������� ������ ����������,
			// �������� ����� ���� ������
			// ���������: ���� �������� grayscaleColor ��� � �������,
			// �� �� ������������� ������ � while ����� ��������
			// ����� minIndex == middleIndex == maxIndex - 1 �
			// Palette[middleIndex].Red < grayscaleColor < Palette[middleIndex + 1].Red
			// ����� �� ���� ������ ���������� ���������� ������� ������� � minIndex ������ ����� maxIndex
			// ����� ���� ��������� ��������� ����� � whil� � maxIndex ������ ����� minIndex - 1
			// ����� �������, �������� grayscaleColor ����� ��������� ����� Palette[minIndex - 1] � Palette[minIndex]
				minIndex = middleIndex + 1;
			else maxIndex = middleIndex - 1; // ���� ������� �������� ������� ��������� ������ ����������, �������� ������ ���� ������
		}

		if (minIndex == BMInfoHeader.ColorUsed) // ���� minIndex ����� ������ ������ ���������� ���������, �� ������ ���������� �������� ������ ������������� �������� � �������
			return (unsigned char)BMInfoHeader.ColorUsed - 1; // ������� ���������� ������ ����������� �������� �������

		if (minIndex == 0) // ���� minIndex �� ���������, �� ��� ����� ��������� ������ ����� ������� ����� ������ �������� grayscaleColor ������ ������������ � �������
			return 0; // ������� ���������� ������ ������������ �������� �������

		int prevDelta = grayscaleColor - Palette[minIndex - 1].Red; // � ���������� ������, ��� ���� ������� �����,
		// �������� grayscaleColor ��������� ����� Palette[minIndex - 1] � Palette[minIndex]
		// ������ ������� �� ������ �������� ��� ������� ������
		int nextDelta = Palette[minIndex].Red - grayscaleColor;
		return prevDelta < nextDelta ? minIndex - 1 : minIndex; // � ���������� ���� ������
	}

public:
	Image(char Mode, unsigned short BCount, int Width, int Height); // ����������� �������� �����������
	Image(char* fileName); // ����������� ������� ����������� �� �����
	Image(); // ����������� ��� ����������, ������� ������ ��������� ��� �����������
	Image(const Image& i); // ����������� �����
	~Image(); // ����������
	int loadimage(char* fileName); // ����� �������� ����������� ����������� ������������, ���������� 0 � ������ ������
	void writeimage(char* fileName); // ����� ������ ����������� � ����
	Image& operator = (const Image& Inp); // ���������� ��������� =
	Image operator / (short Depth); // ���������� ��������� /
	Image& operator /= (const Image& Inp); // ���������� ��������� /=
};

Image::Image()// ���������� ������������ ��� ����������
{
	setEmptyImageParams(); // ��������� ��������� ������� �����������
}

Image::Image(char* fileName)// ���������� ������������ ��� �������� �� �����(� ��������� �����)
{
	setEmptyImageParams(); // ��������� ��������� ������� �����������
	loadimage(fileName); // ��������� �������� �� ����� � ���������� ������
}

Image::Image(const Image& im)// ���������� ������������ � ������������ �����������
{
	initializeFromImage(im); // ������ ����� �����������
}

Image::~Image()// ���������� �����������
{
	if (Rgbtriple) // ���� ���� ��������� �� ������, �� ����� �������� ���������� ��� ����������� ������
	{
		delete[] Rgbtriple; // ������� ������, ���������� ������ ���������� �� ������
		Rgbtriple = NULL; // �������������� ��������� �� ������ ���������� ���������
		// (�� �����������, ��� ��� ����������� � �����������)
	}
	if (Palette) // �������� �������, ���� ��� ���� �������
	{
		delete[] Palette;
		Palette = NULL;
	}
}

void Image::setHeaderAndAllocateMemory(int Width, int Height, int BitCount)
{
	// ��������� ����������� ���� ���������� �� �����������
	BMInfoHeader.Width = Width; // ������ ��������� ������� � ��������
	BMInfoHeader.Height = Height; // ������ ��������� ������� � ��������
	BMInfoHeader.Planes = 1; // ����� ���������� �������� ���������� = 1
	BMInfoHeader.BitCount = BitCount; // ������� �����, ����� ��� �� ����� = 1,4,8,24 (������� 3)
	BMInfoHeader.SizeImage = getTotalImageSize(Width, Height, BitCount); // ������ ����������� � ������
	if (BMInfoHeader.BitCount <= 8)
	{
		BMInfoHeader.ColorUsed = 1 << BMInfoHeader.BitCount; // = 2 � ������� BMInfoHeader.BitCount
		Palette = new RGBQUAD[BMInfoHeader.ColorUsed]; // ��������� ������ ��� ������� ������
		for (int i = 0; i < (int)BMInfoHeader.ColorUsed; i++) // ��������� �������� ������� ���������� ������ �� 0 �� 255
		{
			unsigned char color = (unsigned char)(255 * i / (BMInfoHeader.ColorUsed - 1));
			Palette[i].Red = color;
			Palette[i].Green = color;
			Palette[i].Blue = color;
			Palette[i].Reserved = 0;
		}
	}

	// ��������� ������ ��� ���������� ������� �������� Height*Width ���� RGBTRIPLE
	if (BMInfoHeader.SizeImage > 0)	Rgbtriple = new RGBTRIPLE[BMInfoHeader.Height * BMInfoHeader.Width];
	else Rgbtriple = NULL;
}

Image::Image(char Mode, unsigned short BCount, int Width, int Height)// ���������� ������������ �������� �����������(� ��������� ���������� �����������)
{
	setEmptyImageParams();// ����������� ��������� ����������, ��������������� ������� �����������

	// ��������� ����� �������� �������� ��� �������� � �����������
	// �������������� �������� ������ ������ � RGBTRIPLE(������� 3)
	if (isAllowedBitCount(BCount))
	{
		setHeaderAndAllocateMemory(Width, Height, BCount); // ��������� ��������� ����������� � �������� ������
		if (Palette) Mode = Palette[getNearestPaletteColorIndex(Mode)].Red; // �������� ��������� � ����������� ���� � �������, ���� ��� ������������

		// ���������� ������ ����������� ���������� ���������
		for (int i = 0; i < (int)BMInfoHeader.Height; i++)
			for (int j = 0; j < (int)BMInfoHeader.Width; j++)
			{
				Rgbtriple[i * BMInfoHeader.Width + j].Red = Mode;
				Rgbtriple[i * BMInfoHeader.Width + j].Green = Mode;
				Rgbtriple[i * BMInfoHeader.Width + j].Blue = Mode;
			}
	}// ������ ���������������� �������� ��������	
	else printf("������: �������������� �������� ����������� ������ 24 ���.\n");// ������� ��������� �� ������
}

// ����������� ����������, ��������������� ������� �����������
void Image::setEmptyImageParams()
{
	Rgbtriple = NULL;
	Palette = NULL;
	BMInfoHeader.Size = 40;
	BMInfoHeader.Width = 0;
	BMInfoHeader.Height = 0;
	BMInfoHeader.Planes = 0;
	BMInfoHeader.BitCount = 0;
	BMInfoHeader.Compression = 0;
	BMInfoHeader.SizeImage = 0;
	BMInfoHeader.XPelsPerMeter = 0;
	BMInfoHeader.YPelsPerMeter = 0;
	BMInfoHeader.ColorUsed = 0;
	BMInfoHeader.ColorImportant = 0;
}

int Image::loadImageDataFromFile(FILE* f)
{
	BITMAPFILEHEADER BMFileHeader;
	BITMAPINFOHEADER FileBMInfoHeader;
	RGBQUAD* filePalette = NULL;

	// ��������� ��������� �����������
	if (fread(&BMFileHeader, sizeof(BITMAPFILEHEADER), 1, f) != 1)
	{
		printf("������: �� ������� ������� ��������� ����� �����������\n");
		return 0;
	}

	// ��������� ���������
	if (BMFileHeader.Type != 0x4D42)
	{
		printf("������: ����������� ������ �����\n");
		return 0;
	}

	// ��������� ��������� �����������
	if (fread(&FileBMInfoHeader, sizeof(BITMAPINFOHEADER), 1, f) != 1)
	{
		printf("������: �� ������� ������� ���������� �� �����������\n");
		return 0;
	}

	// ���������, ����� �� ��������� ������ ������
	if (!isAllowedBitCount(FileBMInfoHeader.BitCount) && FileBMInfoHeader.BitCount != 32)
	{
		printf("������: ���������������� �������� �����������: '%i'\n", (int)FileBMInfoHeader.BitCount);
		return 0;
	}

	setHeaderAndAllocateMemory(FileBMInfoHeader.Width, FileBMInfoHeader.Height, FileBMInfoHeader.BitCount);// ��������� ��������� � �������� ������ ��� ��������, ��������� � �����

	// ������� � ��������� �������� �������, ���� ��� ������������
	if (BMInfoHeader.BitCount <= 8)
	{
		filePalette = new RGBQUAD[BMInfoHeader.ColorUsed];
		fread(filePalette, sizeof(RGBQUAD), BMInfoHeader.ColorUsed, f);
	}

	fseek(f, BMFileHeader.OffsetBits, SEEK_SET);// ��������� � ������ ������ �����������
	const int additionalRowOffset = getAdditionalRowOffsetInFile(FileBMInfoHeader.Width, FileBMInfoHeader.BitCount);// ��������� �������� ����� ��������

	if (FileBMInfoHeader.BitCount == 24)
	{
		for (int i = BMInfoHeader.Height - 1; i >= 0; i--)
		{
			fread(Rgbtriple + i * FileBMInfoHeader.Width, sizeof(RGBTRIPLE), FileBMInfoHeader.Width, f);// ��������� ����� ��� ������ �����������, ��� ��� ��� ��������� � ���������� �������� �������� � ������
			if (additionalRowOffset) fseek(f, additionalRowOffset, SEEK_CUR);// ���������� �������� ����� ��������
		}
	}
	else if (FileBMInfoHeader.BitCount == 32)
	{
		// ��������� ������ ������� ��������
		for (int i = BMInfoHeader.Height - 1; i >= 0; i--)
		{
			for (int j = 0; j < (int)BMInfoHeader.Width; j++)
			{
				RGBQUAD fileQuad;
				fread(&fileQuad, sizeof(RGBQUAD), 1, f);

				Rgbtriple[i * BMInfoHeader.Width + j].Red = fileQuad.Red;
				Rgbtriple[i * BMInfoHeader.Width + j].Green = fileQuad.Green;
				Rgbtriple[i * BMInfoHeader.Width + j].Blue = fileQuad.Blue;
			}
		}
	}
	else // �������� ����������� � ��������
	{
		const unsigned char topBitsOffset = 8 - BMInfoHeader.BitCount; // ���������� �� ������� ����� ���������� ���� �� ������� ����� �����, ����� �������� ����
		unsigned char colorMask = (1 << BMInfoHeader.BitCount) - 1; // ���������� ������� BMInfoHeader.BitCount ��� � 1 ��� ��������� ����� �����
		colorMask <<= 8 - BMInfoHeader.BitCount; // ���������� ��� ������� � ������� ���� �����, ��� ��� ������ ������������� �� ������� ����� � �������

		for (int i = BMInfoHeader.Height - 1; i >= 0; i--)
		{
			int leftBits = 0; // ��������� ������� ������������� ��� �������� � �����
			unsigned char paletteByte = 0; // ������� ��������� ���� � �����������
			for (int j = 0; j < (int)BMInfoHeader.Width; j++)
			{
				if (!leftBits) // ���� � ������� ����� ����������� ��� ����, �� ��������� ���������
				{
					fread(&paletteByte, 1, 1, f);
					leftBits = 8;
				}

				int paletteIndex = (paletteByte & colorMask) >> topBitsOffset; // �������� ������� ������ � ������� ����� ����������� ����� � ������� ����������� ��������
				leftBits -= BMInfoHeader.BitCount; // ��������� ���������� �������������� ���
				paletteByte <<= BMInfoHeader.BitCount; // ���������� �� � ������� ����� �����

				unsigned char sourceGrayscale = getGrayscaleColor(filePalette[paletteIndex]); // ��������� ������� ������ ��� ����� ������� �� �����
				unsigned char grayscale = Palette[getNearestPaletteColorIndex(sourceGrayscale)].Red; // �������� ��������� � ���� ���� �� ������

				// ��������� ������ �����������
				Rgbtriple[i * BMInfoHeader.Width + j].Red = grayscale;
				Rgbtriple[i * BMInfoHeader.Width + j].Green = grayscale;
				Rgbtriple[i * BMInfoHeader.Width + j].Blue = grayscale;
			}

			if (additionalRowOffset)fseek(f, additionalRowOffset, SEEK_CUR);// ���������� �������� ����� �������� ����������� � �����
		}
	}

	if (filePalette)delete[] filePalette;// ������� �������� �������, ���� ��� ���� �������
	return 1;
}

// ���������� ������ �������� �����������
int Image::loadimage(char* fileName)
{
	if (Rgbtriple)
	{
		printf("������: ������ ��������� ������ � ��� ��������� �����������\n");
		return 0;
	}

	FILE* f = fopen(fileName, "rb"); // ���������� ��������� �������� ����
	if (!f)
	{
		printf("������: �� ������� ��������� ���� %s\n", fileName);
		return 0;
	}

	int resultCode = loadImageDataFromFile(f);// ���������� �������� ������
	fclose(f);// ��������� ����
	return resultCode;
}

void Image::saveImageDataToFile(FILE* f) // ��������� ������ � ���������� ����
{
	const int additionalRowOffset = getAdditionalRowOffsetInFile(BMInfoHeader.Width, BMInfoHeader.BitCount);// �������� �������� ����� �������� ����������� � �����

	// ��������� ��������� ����� BMP
	BITMAPFILEHEADER BMFileHeader;
	BMFileHeader.Type = 0x4D42; // ���������
	BMFileHeader.OffsetBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER); // �������� �� ������ �����������

	// ������ ������ ����� = ����� ������� ���� ����������, ������� � ������ �����
	BMFileHeader.Size = BMFileHeader.OffsetBits + getTotalImageSize(BMInfoHeader.Width, BMInfoHeader.Height, BMInfoHeader.BitCount);
	BMFileHeader.Reserved1 = 0; // �� ������������
	BMFileHeader.Reserved2 = 0; // �� ������������

	if (Palette) // ��������� ������ � �������
	{
		const int paletteSize = sizeof(RGBQUAD) * BMInfoHeader.ColorUsed; // ��������� ������ �������
		BMFileHeader.Size += paletteSize; // ��������� ��� � ������ ������� �����
		BMFileHeader.OffsetBits += paletteSize; // � � �������� �� ������ �����������
	}

	// ���������� ��������� �����
	if (fwrite(&BMFileHeader, sizeof(BITMAPFILEHEADER), 1, f) != 1)
	{
		printf("������: �� ������� �������� ��������� ����� �����������\n");
		return;
	}

	// ���������� ��������� �����������
	if (fwrite(&BMInfoHeader, sizeof(BITMAPINFOHEADER), 1, f) != 1)
	{
		printf("������: �� ������� �������� ���������� �� �����������\n");
		return;
	}

	if (BMInfoHeader.BitCount == 24)
	{
		for (int i = BMInfoHeader.Height - 1; i >= 0; i--)
		{
			// ��� ������� 24 ��� ������ �������� � ���� ������ �����������, ��� ��� �� ��������� � ���������� �������� ��������
			fwrite(Rgbtriple + i * BMInfoHeader.Width, sizeof(RGBTRIPLE), BMInfoHeader.Width, f);

			// ���������� ������ ���������� 0 ��� �������� ����� �������� �����������
			if (additionalRowOffset)
			{
				const int Zero = 0;
				fwrite(&Zero, 1, additionalRowOffset, f);
			}
		}
	}
	else // ����������� � ��������
	{
		// ������� ���������� �������
		if (fwrite(Palette, sizeof(RGBQUAD), BMInfoHeader.ColorUsed, f) != BMInfoHeader.ColorUsed)
		{
			printf("������: �� ������� �������� �������\n");
			return;
		}

		// ��������� �������� ����������� ��� ����������� ������� ����� ������� ������� � ������� ����
		const int startPaletteDataOffset = 8 - BMInfoHeader.BitCount;

		// ���������� ����������� � ��������
		for (int i = BMInfoHeader.Height - 1; i >= 0; i--)
		{
			int currentPaletteDataOffset = startPaletteDataOffset; // ���������� �� ������� ����� �������� ��������� �������� ��� ������ � ����
			int leftBits = 8; // ������ ������� ��� ��� ����� ������������ � �����
			int paletteByte = 0; // ������ ������� �������� ��� ������ � ����
			for (int j = 0; j < (int)BMInfoHeader.Width; j++)
			{
				unsigned char paletteColorIndex = getNearestPaletteColorIndex(Rgbtriple[i * BMInfoHeader.Width + j].Red); // �������� ������ ������� ��� �������� �����
				paletteByte |= paletteColorIndex << currentPaletteDataOffset; // ������� ��� �� ������ ���������� ��� � ���������� � ���������
				currentPaletteDataOffset -= BMInfoHeader.BitCount; // ��������� ��������� ����� ��� ���������� ������
				leftBits -= BMInfoHeader.BitCount; // ��������� ���������� ���������� ���

				if (!leftBits)
				{
					fwrite(&paletteByte, 1, 1, f);// ���� ��������� ��������� ����, �� ���������� ��������� ���� � ����

					// � ���������� ��������� �� ���������
					currentPaletteDataOffset = startPaletteDataOffset;
					leftBits = 8;
					paletteByte = 0;
				}
			}

			// ���� ����� ���������� ��������� ������ �������� ������������ ���� (���� ��������� �� ����������), �� ���������� �� � ����
			if (leftBits != 8) fwrite(&paletteByte, 1, 1, f);

			// ���������� ������ ���������� 0 ��� �������� ����� �������� �����������
			if (additionalRowOffset)
			{
				const int Zero = 0;
				fwrite(&Zero, 1, additionalRowOffset, f);
			}
		}
	}
}

void Image::initializeFromImage(const Image& Inp) // ������ ����� ����������� (��������� ��������� � �������� ������)
{
	setEmptyImageParams(); // ��������� ��������� ������� �����������
	setHeaderAndAllocateMemory(Inp.BMInfoHeader.Width, Inp.BMInfoHeader.Height, Inp.BMInfoHeader.BitCount); // ��������� ��������� � �������� ������ ������
	copyDataFromImage(Inp); // �������� ������ �����������
}

void Image::copyDataFromImage(const Image& Inp)
{
	// ��������� ���������� ���������� � ��������, ����� ���� ��������� ������
	if (BMInfoHeader.Width == Inp.BMInfoHeader.Width &&
		BMInfoHeader.Height == Inp.BMInfoHeader.Height &&
		BMInfoHeader.BitCount == Inp.BMInfoHeader.BitCount) memcpy(Rgbtriple, Inp.Rgbtriple, BMInfoHeader.Height * BMInfoHeader.Width * sizeof(RGBTRIPLE));
	else printf("������: � ����������� ��� �������������������� ������ ����������� �/��� ���������\n");
}

void Image::writeimage(char* fileName) // ��������� ������ � ����
{
	if (!Rgbtriple)
	{
		printf("������: � ����������� ��� ������ ��� ����������\n");
		return;
	}

	FILE* f = fopen(fileName, "wb"); //��������� �������� ���� �� ������	

	if (!f)
	{
		printf("������: �� ������� ������� ���� %s\n", fileName);
		return;
	}

	saveImageDataToFile(f); // ��������� ������ � ����
	fclose(f); // ��������� ����
}

Image& Image::operator = (const Image& Inp) // ���������� ��������� =
{
	if (Rgbtriple) copyDataFromImage(Inp); //���� ����������� ��� ������� �� ������� ������ ����������� ������ (��� ���������� ��������)
	else initializeFromImage(Inp); // ����� ������� �����

	return *this;
}

void Image::copyAndConvertDataFromImage(const Image& Inp) // ������� �������������� ������ ����������� ����������� � ������� ������
{
	// ��������� ���������� ���������� � ��������, ����� ���� ��������� ������
	if (BMInfoHeader.Width != Inp.BMInfoHeader.Width &&
		BMInfoHeader.Height != Inp.BMInfoHeader.Height)
	{
		printf("������: �� ��������� ���������� ��� ��������������� �������� �����������\n");
		return;
	}

	// ���������, ��� ������������� ���������� ��������
	if (Inp.BMInfoHeader.BitCount < BMInfoHeader.BitCount)
	{
		printf("������: �������� ������ ���������� �������� �����������\n");
		return;
	}

	const bool isSourceWithPalette = Inp.Palette != NULL; // ������������ �� � �������� ����������� �������
	for (int i = 0; i < (int)BMInfoHeader.Height; i++)
		for (int j = 0; j < (int)BMInfoHeader.Width; j++)
		{
			unsigned char grayscale = isSourceWithPalette ? Inp.Rgbtriple[i * BMInfoHeader.Width + j].Red // ��� ��������� ����������� � �������� ����� ������� ������ ����� �� ������ ��������
				: getGrayscaleColor(Inp.Rgbtriple[i * BMInfoHeader.Width + j]); // ����� ��������� ���

			grayscale = Palette[getNearestPaletteColorIndex(grayscale)].Red; // �������� �������� ������ � ������� ��������� �����������

			// ���������� ������� ������ � ������� ��������� �����������
			Rgbtriple[i * BMInfoHeader.Width + j].Red = grayscale;
			Rgbtriple[i * BMInfoHeader.Width + j].Green = grayscale;
			Rgbtriple[i * BMInfoHeader.Width + j].Blue = grayscale;
		}
}

Image Image::operator / (short Depth) // ���������� ��������� /, ���������� ����� �����������, ���������� ������ �������, �� � ��������� Depth
{
	// ���������, ��� �������� ��������
	if (!isAllowedBitCount(Depth))
	{
		printf("������: ���������������� ��������\n");
		return Image(*this);
	}

	if (Depth > BMInfoHeader.BitCount)
	{
		printf("������: �������� ������ ���������� ��������\n");
		return Image(*this);
	}

	Image result(0, Depth, BMInfoHeader.Width, BMInfoHeader.Height); // ������� ������ �����������
	result.copyAndConvertDataFromImage(*this); // ��������� �������������� �������� ����������� � ��������
	return result;
}

Image& Image::operator /= (const Image& Inp) // ���������� ��������� /=, ��������� ������ ����������� ����������� � ������� � ���������� �������
{
	if (BMInfoHeader.BitCount != Inp.BMInfoHeader.BitCount)	// �������� ���������� ��������
	{
		printf("������: ������ �������� �����������, ��������� ����������� � ����� �������� ����������\n");
		return *this;
	}

	float xRatio = (float)Inp.BMInfoHeader.Width / BMInfoHeader.Width; // ���������� ����������� ������ �����������
	float yRatio = (float)Inp.BMInfoHeader.Height / BMInfoHeader.Height; // ���������� ����������� ������ �����������

	for (int i = 0; i < (int)BMInfoHeader.Height; i++)
		for (int j = 0; j < (int)BMInfoHeader.Width; j++)
		{
			int sourceX = (int)(j * xRatio); // ���������� x ���������� � �������� �����������
			int sourceY = (int)(i * yRatio); // ���������� y ���������� � �������� �����������
			Rgbtriple[i * BMInfoHeader.Width + j] = Inp.Rgbtriple[sourceY * Inp.BMInfoHeader.Width + sourceX]; // ������ �������� ����� �� ��������� ����������� � �������
		}

	return *this;
}

int main(void)
{
	setlocale(LC_ALL, "Russian");
	Image* im1 = new Image((char*)"volleyball.bmp"); // �������� ������� ����������� �� ����� 
	Image* im2 = new Image(0, 24, 1000, 500);// �������� ����������� � ��������� ����������� 
	Image im3, im4, im5;// �������� ������� ����������� 

	(*im2) /= *im1; // ���������� img � �������� img2 
	im3 = *im1 / 8; // ������ �������� 8
	im4 = *im1 / 4; // ������ �������� 4
	im5 = *im1 / 1; // ������ �������� 1
	im3.writeimage((char*)"Mercedes_8.bmp");
	im4.writeimage((char*)"Mercedes_4.bmp");
	im5.writeimage((char*)"Mercedes_1.bmp");
	im2->writeimage((char*)"Mercedes_res.bmp");

	printf("Mercedes.bmp have been converted.\n");
	scanf(" ");
	return 0;
}
