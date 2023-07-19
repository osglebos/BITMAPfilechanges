#define _CRT_SECURE_NO_WARNINGS
#include <locale>
#include <stdio.h>
typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef unsigned char BYTE;

#pragma pack(push,1)
struct BITMAPFILEHEADER
{
	WORD Type; // �BM� 0x4D42
	DWORD Size; // ������ ����� � ������, BitCount*Height*Width+ OffsetBits
	WORD Reserved1; // ��������������; ������ ���� ����
	WORD Reserved2; // ��������������; ������ ���� ����
	DWORD OffsetBits; // �������� ������ �� ������ ����� � ������
   // = sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER)
};
#pragma pack(pop)

#pragma pack(push,1)
struct BITMAPINFOHEADER
{
	DWORD Size; // ����� ������ ����������� ��� ��������� = 40
	DWORD Width; // ������ ��������� ������� � ��������
	DWORD Height; // ������ ��������� ������� � ��������
	WORD Planes; // ����� ���������� �������� ���������� = 1
	WORD BitCount; // ������� �����, ����� ��� �� ����� = 0,1,4,8,16,24,32
	DWORD Compression; // ��� ������ = 0 ��� ��������� �����������
	DWORD SizeImage; // ������ ����������� � ������ BitCount*Height*Width
	DWORD XPelsPerMeter; // ����������� ����������� �� �����������
	DWORD YPelsPerMeter; // ����������� ����������� �� ���������
	DWORD ColorUsed; // ����� �������� ������������ ������. ���� ��� ����� = 0
	DWORD ColorImportant; // ����� ����������� ������ = 0
};
#pragma pack(pop)

#pragma pack(push,1)
struct RGBTRIPLE
{
	BYTE Red;
	BYTE Green;
	BYTE Blue;
};
#pragma pack(pop)

#pragma pack(push,1)
struct RGBQUAD
{
	BYTE Red;
	BYTE Green;
	BYTE Blue;
	BYTE Reserved;
};
#pragma pack(pop)

class Image {
	BITMAPINFOHEADER BMInfoHeader;
	RGBQUAD *Rgbquad;
	RGBQUAD	*Palette;
	void setEmptyImageParams();
	bool isAllowedBitCount(WORD BitCount);
	void setHeaderAndAllocateMemory(DWORD Width, DWORD Height, WORD BitCount);
	int getAdditionalRowOffsetInFile(DWORD Width, WORD BitCount);
	int getImageRowSize(DWORD Width, WORD BitCount);
	int getTotalImageSize(DWORD Width, DWORD Height, WORD BitCount);
	BYTE getGrayscaleColor(BYTE Red, BYTE Green, BYTE Blue);
	BYTE getNearestPaletteColorIndex(BYTE grayscaleColor);
	int loadImageDataFromFile(FILE* f);
	void saveImageDataToFile(FILE* f);
	void initializeFromImage(const Image& Inp);
	void copyDataFromImage(const Image& Inp);
	void copyAndConvertDataFromImage(const Image& Inp);
public:
	Image();
	~Image();
	Image(char Mode, WORD BCount, DWORD Width, DWORD Height);
	Image(char* fileName);
	int loadimage(char* fileName);
	void writeimage(char* fileName);
	Image(const Image& i);
	Image& operator = (const Image& Inp);
	Image& operator /= (const Image& Inp);
	Image operator / (short Depth);
};

Image& Image::operator = (const Image& Inp) // ���������� ��������� =
{
	if (Rgbquad) copyDataFromImage(Inp); //���� ����������� ��� ������� �� ������� ������ ����������� ������ (��� ���������� ��������)
	else initializeFromImage(Inp); // ����� ������� �����

	return *this;
}

void Image::copyAndConvertDataFromImage(const Image& Inp) // ������� �������������� ������ ����������� ����������� � ������� ������
{
	// ��������� ���������� ���������� � ��������, ����� ���� ��������� ������
	if (BMInfoHeader.Width != Inp.BMInfoHeader.Width && BMInfoHeader.Height != Inp.BMInfoHeader.Height)
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
			BYTE grayscale = isSourceWithPalette ? Inp.Rgbquad[i * BMInfoHeader.Width + j].Red // ��� ��������� ����������� � �������� ����� ������� ������ ����� �� ������ ��������
				: getGrayscaleColor(Inp.Rgbquad[i * BMInfoHeader.Width + j].Red, Inp.Rgbquad[i * BMInfoHeader.Width + j].Green, Inp.Rgbquad[i * BMInfoHeader.Width + j].Blue); // ����� ��������� ���

			grayscale = Palette[getNearestPaletteColorIndex(grayscale)].Red; // �������� �������� ������ � ������� ��������� �����������

			// ���������� ������� ������ � ������� ��������� �����������
			Rgbquad[i * BMInfoHeader.Width + j].Red = grayscale;
			Rgbquad[i * BMInfoHeader.Width + j].Green = grayscale;
			Rgbquad[i * BMInfoHeader.Width + j].Blue = grayscale;
			Rgbquad[i * BMInfoHeader.Width + j].Reserved = 0;
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
			Rgbquad[i * BMInfoHeader.Width + j] = Inp.Rgbquad[sourceY * Inp.BMInfoHeader.Width + sourceX]; // ������ �������� ����� �� ��������� ����������� � �������
		}

	return *this;
}

void Image::copyDataFromImage(const Image& Inp) // �������� ������ ��� ���������� ���������� �����������
{
	// ��������� ���������� ���������� � ��������, ����� ���� ��������� ������
	if (BMInfoHeader.Width == Inp.BMInfoHeader.Width &&	BMInfoHeader.Height == Inp.BMInfoHeader.Height && BMInfoHeader.BitCount == Inp.BMInfoHeader.BitCount)
	{
		Rgbquad = new RGBQUAD[Inp.BMInfoHeader.Height*Inp.BMInfoHeader.Width];
		for (int i = 0; i < (int)Inp.BMInfoHeader.Height; i++)
			for (int j = 0; j < (int)Inp.BMInfoHeader.Width; j++)
				Rgbquad[i*Inp.BMInfoHeader.Width + j] = Inp.Rgbquad[i*Inp.BMInfoHeader.Width + j];

	}
	else printf("������: � ����������� ��� �������������������� ������ ����������� �/��� ���������\n");
}

void Image::initializeFromImage(const Image& Inp) // ������ ����� ����������� (��������� ��������� � �������� ������)
{
	setEmptyImageParams(); // ��������� ��������� ������� �����������
	setHeaderAndAllocateMemory(Inp.BMInfoHeader.Width, Inp.BMInfoHeader.Height, Inp.BMInfoHeader.BitCount); // ��������� ��������� � �������� ������ ������
	copyDataFromImage(Inp); // �������� ������ �����������
}

Image::Image(const Image& im)// ���������� ������������ � ������������ �����������
{
	initializeFromImage(im); // ������ ����� �����������
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
			for (int j = 0; j < (int)BMInfoHeader.Width; j++)
			{
				RGBTRIPLE fileTriple;
				fileTriple.Blue = Rgbquad[i*BMInfoHeader.Width + j].Blue;
				fileTriple.Green = Rgbquad[i*BMInfoHeader.Width + j].Green;
				fileTriple.Red = Rgbquad[i*BMInfoHeader.Width + j].Red;
				fwrite(&(fileTriple), sizeof(RGBTRIPLE), 1, f);
			}

			// ���������� ������ ���������� 0 ��� �������� ����� �������� �����������
			if (additionalRowOffset)
			{
				const int Zero = 0;
				fwrite(&Zero, 1, additionalRowOffset, f);
			}
		}
	}
	else if (BMInfoHeader.BitCount == 32)
	{
		for (int i = BMInfoHeader.Height - 1; i >= 0; i--)
		{
			for (int j = 0; j < (int)BMInfoHeader.Width; j++)
				fwrite(&(Rgbquad[i*BMInfoHeader.Width + j]), sizeof(RGBQUAD), 1, f);

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
				unsigned char paletteColorIndex = getNearestPaletteColorIndex(Rgbquad[i*BMInfoHeader.Width + j].Red); // �������� ������ ������� ��� �������� �����
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

void Image::writeimage(char* fileName) // ��������� ������ � ����
{
	if (!Rgbquad)
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

int Image::loadImageDataFromFile(FILE* f)
{
	BITMAPFILEHEADER BMFileHeader;
	BITMAPINFOHEADER FileBMInfoHeader;
	RGBQUAD *filePalette = NULL;

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
	if (!isAllowedBitCount(FileBMInfoHeader.BitCount))
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
			for (int j = 0; j < (int)BMInfoHeader.Width; j++)
			{
				RGBTRIPLE fileTriple;
				fread(&fileTriple, sizeof(RGBTRIPLE), 1, f);

				Rgbquad[i * BMInfoHeader.Width + j].Red = fileTriple.Red;
				Rgbquad[i * BMInfoHeader.Width + j].Green = fileTriple.Green;
				Rgbquad[i * BMInfoHeader.Width + j].Blue = fileTriple.Blue;
				Rgbquad[i * BMInfoHeader.Width + j].Reserved = 0;
			}
		}
	}
	else if (FileBMInfoHeader.BitCount == 32)
	{
		for (int i = BMInfoHeader.Height - 1; i >= 0; i--)
		{
			for (int j = 0; j < (int)BMInfoHeader.Width; j++)
			{
				RGBQUAD fileQuad;
				fread(&fileQuad, sizeof(RGBQUAD), 1, f);

				Rgbquad[i * BMInfoHeader.Width + j].Red = fileQuad.Red;
				Rgbquad[i * BMInfoHeader.Width + j].Green = fileQuad.Green;
				Rgbquad[i * BMInfoHeader.Width + j].Blue = fileQuad.Blue;
				Rgbquad[i * BMInfoHeader.Width + j].Reserved = fileQuad.Reserved;
			}
		}
	}
	else // �������� ����������� � ��������
	{
		const BYTE topBitsOffset = 8 - BMInfoHeader.BitCount; // ���������� �� ������� ����� ���������� ���� �� ������� ����� �����, ����� �������� ����
		BYTE colorMask = (1 << BMInfoHeader.BitCount) - 1; // ���������� ������� BMInfoHeader.BitCount ��� � 1 ��� ��������� ����� �����
		colorMask <<= 8 - BMInfoHeader.BitCount; // ���������� ��� ������� � ������� ���� �����, ��� ��� ������ ������������� �� ������� ����� � �������

		for (int i = BMInfoHeader.Height - 1; i >= 0; i--)
		{
			int leftBits = 0; // ��������� ������� ������������� ��� �������� � �����
			BYTE paletteByte = 0; // ������� ��������� ���� � �����������
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

				BYTE sourceGrayscale = getGrayscaleColor(filePalette[paletteIndex].Red, filePalette[paletteIndex].Green, filePalette[paletteIndex].Blue); // ��������� ������� ������ ��� ����� ������� �� �����
				BYTE grayscale = Palette[getNearestPaletteColorIndex(sourceGrayscale)].Red; // �������� ��������� � ���� ���� �� ������

				// ��������� ������ �����������
				Rgbquad[i * BMInfoHeader.Width + j].Red = grayscale;
				Rgbquad[i * BMInfoHeader.Width + j].Green = grayscale;
				Rgbquad[i * BMInfoHeader.Width + j].Blue = grayscale;
				Rgbquad[i * BMInfoHeader.Width + j].Reserved = 0;
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
	if (Rgbquad)
	{
		printf("������: ������ ��������� ������ � ��� ��������� �����������\n");
		return 0;
	}

	FILE* f; // ��������� ��������� ����
	f = fopen(fileName, "rb"); // ���������� ��������� �������� ����
	if (!f)
	{
		printf("������: �� ������� ��������� ���� %s\n", fileName);
		return 0;
	}

	int result = loadImageDataFromFile(f);// ���������� �������� ������
	fclose(f);// ��������� ����
	return result;
}

Image::Image(char* fileName)// ���������� ������������ ��� �������� �� �����(� ��������� �����)
{
	setEmptyImageParams(); // ��������� ��������� ������� �����������
	loadimage(fileName); // ��������� �������� �� ����� � ���������� ������
}

BYTE Image::getGrayscaleColor(BYTE Red, BYTE Green, BYTE Blue) // ��������� �������� ������ ��� ����������� �����
{
	int result = (int)(Red * 0.299 + Green * 0.597 + Blue * 0.114); // ������� �� ���������
	if (result > 255) result = 255;// ��������� ����� �� ������� �����
	return (unsigned char)result;
}

BYTE Image::getNearestPaletteColorIndex(BYTE grayscaleColor) // �������� ������� ���� ��������� ���� � ������� (��� ��� ��� ����������� �� ����������� �������� ������)
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
			return middleIndex; // ���������� ������ ������� � ������ ������
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

int Image::getAdditionalRowOffsetInFile(DWORD Width, WORD BitCount) // ������� ������� ����� ���������� � ���� ���� ����� ������ ������ �����������, ����� �������� ������ ������ ������� 4
{
	int remainder = getImageRowSize(Width, BitCount) % 4; // ��������� ������� �� ������� ����� ������ �� 
	return remainder ? 4 - remainder : 0; // ���������� ���������� �� 4
}

int Image::getImageRowSize(DWORD Width, WORD BitCount) // ���������� ������� ���� ��������� ��� ������ ������ ����������� � ����
{
	// Width * BitCount ���� ����������� ����� ��� ��� �������� ������ �����������
	// ((Width * BitCount + 7) / 8) ��������� ���������� ���� � ��������� � ������� �������
	return (Width * BitCount + 7) / 8;
}

int Image::getTotalImageSize(DWORD Width, DWORD Height, WORD BitCount) // ���������� ������ ������ ������ ��� ������ � ����
{
	// ��������� ������� ���� ��������� ��� ������ ������ ����������� � ����, ��������� ������ ���������� ���� ��� ��������� 4 � �������� �� ���������� �����
	return (getImageRowSize(BMInfoHeader.Width, BMInfoHeader.BitCount) + getAdditionalRowOffsetInFile(Width, BitCount)) * Height;
}

bool Image::isAllowedBitCount(WORD BitCount) // ��������� ������������ �������� �����������
{
	return BitCount == 32 || BitCount == 24 || BitCount == 8 || BitCount == 4 || BitCount == 1; // 3-�� �������
}

void Image::setHeaderAndAllocateMemory(DWORD Width, DWORD Height, WORD BitCount)
{
	// ��������� ����������� ���� ���������� �� �����������
	BMInfoHeader.Width = Width; // ������ ��������� ������� � ��������
	BMInfoHeader.Height = Height; // ������ ��������� ������� � ��������
	BMInfoHeader.Planes = 1; // ����� ���������� �������� ���������� = 1
	BMInfoHeader.BitCount = BitCount; // ������� �����, ����� ��� �� ����� = 1,4,8,24,32 
	BMInfoHeader.SizeImage = getTotalImageSize(Width, Height, BitCount); // ������ ����������� � ������
	if ((int)BMInfoHeader.BitCount <= 8)
	{
		BMInfoHeader.ColorUsed = 1 << BMInfoHeader.BitCount; // = 2 � ������� BMInfoHeader.BitCount
		Palette = new RGBQUAD[BMInfoHeader.ColorUsed]; // ��������� ������ ��� ������� ������
		for (int i = 0; i < (int)BMInfoHeader.ColorUsed; i++) // ��������� �������� ������� ���������� ������ �� 0 �� 255
		{
			BYTE color = (BYTE)(255 * i / (BMInfoHeader.ColorUsed - 1));
			Palette[i].Red = color;
			Palette[i].Green = color;
			Palette[i].Blue = color;
			Palette[i].Reserved = 0;
		}
	}

	// ��������� ������ ��� ���������� ������� �������� Height*Width ���� RGBQUAD
	if (BMInfoHeader.SizeImage > 0)	Rgbquad = new RGBQUAD[BMInfoHeader.Height * BMInfoHeader.Width];
	else Rgbquad = NULL;
}

Image::Image(char Mode, WORD BCount, DWORD Width, DWORD Height)// ���������� ������������ �������� �����������(� ��������� ���������� �����������)
{
	setEmptyImageParams();// ����������� ��������� ����������, ��������������� ������� �����������

	// ��������� ����� �������� �������� ��� �������� � �����������
	// �������������� �������� ������ ������ � RGBQUAD(������� 3)
	if (isAllowedBitCount(BCount))
	{
		setHeaderAndAllocateMemory(Width, Height, BCount); // ��������� ��������� ����������� � �������� ������
		if (Palette) Mode = Palette[getNearestPaletteColorIndex(Mode)].Red; // �������� ��������� � ����������� ���� � �������, ���� ��� ������������

		// ���������� ������ ����������� ���������� ���������
		for (int i = 0; i < (int)BMInfoHeader.Height; i++)
			for (int j = 0; j < (int)BMInfoHeader.Width; j++)
			{
				Rgbquad[i * BMInfoHeader.Width + j].Red = Mode;
				Rgbquad[i * BMInfoHeader.Width + j].Green = Mode;
				Rgbquad[i * BMInfoHeader.Width + j].Blue = Mode;
				Rgbquad[i * BMInfoHeader.Width + j].Reserved = Mode;
			}

	}// ������ ���������������� �������� ��������	
	else printf("������: �������������� �������� ����������� ������ 4, 8, 32 ���.\n");// ������� ��������� �� ������
}

// ����������� ����������, ��������������� ������� �����������
void Image::setEmptyImageParams()
{
	Rgbquad = NULL;
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

Image::Image()// ���������� ������������ ��� ����������
{
	setEmptyImageParams(); // ��������� ��������� ������� �����������
}

Image::~Image()// ���������� �����������
{
	if (Rgbquad) // ���� ���� ��������� �� ������, �� ����� �������� ���������� ��� ����������� ������
	{
		delete[] Rgbquad; // ������� ������, ���������� ������ ���������� �� ������
		Rgbquad = NULL; // �������������� ��������� �� ������ ���������� ���������
		// (�� �����������, ��� ��� ����������� � �����������)
	}
	if (Palette) // �������� �������, ���� ��� ���� �������
	{
		delete[] Palette;
		Palette = NULL;
	}
}

int main(void)
{
	setlocale(LC_ALL, "Russian");
	Image* im1 = new Image((char*)"beach.bmp"); // �������� ������� ����������� �� ����� 
	Image* im2 = new Image(0, 24, 500, 400);// �������� ����������� � ��������� ����������� 
	Image im3, im4;// �������� ������� ����������� 

	(*im2) /= *im1; // ���������� img � �������� img2 
	im3 = *im1 / 8; // ������ �������� 8
	im4 = *im1 / 1; // ������ �������� 4
	im3.writeimage((char*)"beach_8.bmp");
	im4.writeimage((char*)"beach_1.bmp");
	im2->writeimage((char*)"beach_res.bmp");

	printf("beach.bmp have been converted.\n");
	scanf(" ");
	return 0;
}