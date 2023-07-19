#define _CRT_SECURE_NO_WARNINGS
#include <locale>
#include <stdio.h>
typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef unsigned char BYTE;

// ������ ��� ����������� ���������� ���� � ����������� �� DWORD ������ �������� � DIB 
// Width - ����� ������ � ��������; BPP - ��� �� ������
#define BYTESPERLINE(Width, BPP) ((unsigned short)((((unsigned long)(Width) * (unsigned long)(BPP)+31) >> 5)) << 2)

#pragma pack(push,1)
struct BITMAPFILEHEADER {

	WORD Type;
	DWORD   Size;
	WORD  Reserved1;
	WORD  Reserved2;
	DWORD   OffsetBits;
};
#pragma pack(pop)

#pragma pack(push,1)

struct BITMAPINFOHEADER
{
	DWORD Size;//����� ������, ����������� ��� ��������� =40
	DWORD Width;//������ ��������� ������� � ��������
	DWORD Height;//������ ��������� ������� � ��������
	WORD Planes;//����� ���������� �������� ���������� =1
	WORD BitCount;//������� �����, ����� ��� �� ����� = 0,1,4,8,16, 24,32
	DWORD Compression;//��� ������=0 ��� ��������� �����������
	DWORD SizeImage;//������ ����������� � ������ BitCount*Height*Width
	DWORD XPelsPerMeter;//����������� ����������� �� �����������
	DWORD YPelsPerMeter;//����������� ����������� �� ���������
	DWORD ColorUsed;//����� ��������, ������������ ������. ���� ��� �����=0
	DWORD ColorImportant;//����� ����������� ������ =0
};
#pragma pack(pop)

#pragma pack(push, 1)
//C��������, ���������� ������ �����������
//(�������� ����� RGBQ):
struct RGBQUAD {
	BYTE Blue;
	BYTE Green;
	BYTE Red;
	BYTE Reserved; //�����������-�����������������������������.
};
#pragma pack(pop)

#pragma pack(push, 1)
//C��������, ���������� ������ �����������
//(�������� ����� RGBT):
struct RGBTRIPLE {
	BYTE Blue;
	BYTE Green;
	BYTE Red;
};
#pragma pack(pop)

class Image {
	BITMAPINFOHEADER BMInfoHeader; //��������������� BITMAPINFOHEADER
	RGBQUAD **Rgbquad; //������������������� RGBQUAD
	RGBQUAD *Pallete; //���������� ������ ���� RGBQUAD ��� ���������� ����������� 1 � 8 ���.
	int LocateInPallete(const RGBQUAD &rgbq);//��������������-��������������������
public:
	Image(char Mode, unsigned short BCount, int Width, int Height); // ������������������������������
	Image(char *fileName); // ����������� ������� ����������� �� �����
	Image(); // ����������� ��� ����������, ������� ������ ��������� ��� �����������
	Image(const Image &copy); // ����������������
	~Image(); // ����������
	int loadimage(char *fileName); // ����� �������� ����������� ����������� ������������
	void writeimage(char *fileName); // ����� ������ ����������� � ����
	Image operator = (Image Inp); // ������������������� =
	Image operator /=(const Image& mas);   // ������������������� /= (�������)
	Image operator/(const unsigned short Depth);  // ������������������� / (������������)
	int Height(void);
	int Widht(void);
};

int Image::Height(void)
{
	return BMInfoHeader.Height;
}

int Image::Widht(void)
{
	return BMInfoHeader.Width;
}

int Image::LocateInPallete(const RGBQUAD &rgbq) {
	if (!Pallete) return -1;
	for (int i = 0; i < this->BMInfoHeader.ColorUsed; i++)
		if ((Pallete[i].Blue == rgbq.Blue) && (Pallete[i].Green == rgbq.Green) && (Pallete[i].Red == rgbq.Red)) return i;
	return -1;
}

//������������������������������:
Image::Image(char Mode, unsigned short BCount, int Width, int Height) {
	// �������������� ��������� �����������.
	//������ ��������� � �������� �������, ������� � �������� �����:
	BMInfoHeader.Size = 40;  // ����� ������ ����������� ��� ��������� = 40
	BMInfoHeader.Width = Width;
	BMInfoHeader.Height = Height;
	BMInfoHeader.Planes = 1; // ����� ���������� �������� ���������� = 1
	BMInfoHeader.BitCount = BCount; //������� �����, ����� ��� �� ����� = 24,32
	BMInfoHeader.Compression = 0; // ��� ������ = 0 ��� ��������� �����������.
	//BMInfoHeader.SizeImage = BCount*Width*Height;
	BMInfoHeader.SizeImage = Height * BYTESPERLINE(Width, BCount);
	BMInfoHeader.XPelsPerMeter = 0;
	BMInfoHeader.YPelsPerMeter = 0;
	BMInfoHeader.ColorUsed = 0;
	BMInfoHeader.ColorImportant = 0;

	if (BMInfoHeader.BitCount <= 8) {//���������������� = 8
		BMInfoHeader.ColorUsed = (1 << BMInfoHeader.BitCount); // ����������-��������������������
		Pallete = new RGBQUAD[BMInfoHeader.ColorUsed];    // ��������������������������

		for (int i = 0; i < BMInfoHeader.ColorUsed; i++) {// ����������
			Pallete[i].Blue = 0;
			Pallete[i].Green = 0;
			Pallete[i].Red = 0;
			Pallete[i].Reserved = 0;
		}
		Pallete[0].Blue = Mode;
		Pallete[0].Green = Mode;
		Pallete[0].Red = Mode;
		Pallete[0].Reserved = 0;

		Rgbquad = new RGBQUAD*[BMInfoHeader.Height]; // �������������������������������������
		for (int i = 0; i < BMInfoHeader.Height; i++) {
			Rgbquad[i] = new RGBQUAD[BMInfoHeader.Width];
		}

		for (int i = 0; i < BMInfoHeader.Height; i++) { //��������� ��� � ������������ � ��������� �������������
			for (int j = 0; j < BMInfoHeader.Width; j++) {
				Rgbquad[i][j].Red = Mode;
				Rgbquad[i][j].Green = Mode;
				Rgbquad[i][j].Blue = Mode;
				Rgbquad[i][j].Reserved = 0;
			}
		}
	}

	else
		if (BMInfoHeader.BitCount == 32) {//���������������� = 32
			Rgbquad = new RGBQUAD*[BMInfoHeader.Height];  //���������������������
			for (int i = 0; i < BMInfoHeader.Height; i++) {
				Rgbquad[i] = new RGBQUAD[BMInfoHeader.Width];
			}

			for (int i = 0; i < BMInfoHeader.Height; i++) { //��������� ��� � ������������ � ��������� �������������
				for (int j = 0; j < BMInfoHeader.Width; j++) {
					Rgbquad[i][j].Red = Mode;
					Rgbquad[i][j].Green = Mode;
					Rgbquad[i][j].Blue = Mode;
					Rgbquad[i][j].Reserved = 0;
				}
			}
		}
};

//������ ������ ��������� ��� �����������:
Image::Image() {
	Rgbquad = NULL;
	BMInfoHeader.Size = 0;
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

//����������� ���������� ������ ����������� �� �����:
Image::Image(char *fileName) {
	FILE *file;   //����
	errno_t e;
	e = fopen_s(&file, fileName, "rb");//�������������
	BITMAPFILEHEADER BMFileHeader;
	fread(&BMFileHeader, sizeof(BITMAPFILEHEADER), 1, file);

	if (BMFileHeader.Type != 0x4D42) {	//���� ��� �� ������������ �������
		printf("������! ����������� ������ �����.\n");
		return;
	}
	fread(&BMInfoHeader, sizeof(BITMAPINFOHEADER), 1, file);

	if (BMInfoHeader.BitCount != 32 && BMInfoHeader.BitCount != 24) // ���� ������� ����� �� ������������ ������
		fread(&BMInfoHeader, sizeof(BITMAPINFOHEADER), 1, file);
	if (BMInfoHeader.BitCount == 4) {
		printf("������! ���������������� �������� �����������.\n");
		return;
	}

	if (BMInfoHeader.BitCount <= 8) {// ���� ������� ����� <= 8
		BMInfoHeader.ColorUsed = 1 << BMInfoHeader.BitCount;
		Pallete = new RGBQUAD[BMInfoHeader.ColorUsed]; // ����� �������������� �������

		if (fread(Pallete, sizeof(RGBQUAD)*BMInfoHeader.ColorUsed, 1, file) != 1) {
			delete[]Pallete;
			fclose(file);
			return;
		}
		BMInfoHeader.SizeImage = BMInfoHeader.Height*BYTESPERLINE(BMInfoHeader.Width, BMInfoHeader.BitCount); //������������������������
		unsigned char *ucBuff = new unsigned char[BMInfoHeader.SizeImage];
		fread(ucBuff, BMInfoHeader.SizeImage, 1, file);

		Rgbquad = new RGBQUAD*[BMInfoHeader.Height]; // ���������������������
		for (int i = 0; i < (int)BMInfoHeader.Height; i++) {
			Rgbquad[i] = new RGBQUAD[BMInfoHeader.Width];
		}

		for (int i = 0; i < BMInfoHeader.Height; i++) {// ����������������������������������
			for (int j = 0; j < BMInfoHeader.Width; j++) {
				int iIndCol;
				if (BMInfoHeader.BitCount == 8) {//���������������� = 8
					iIndCol = ucBuff[i*BYTESPERLINE(BMInfoHeader.Width, BMInfoHeader.BitCount) + j];
				}
				if (BMInfoHeader.BitCount == 1) {//���������������� = 1
					unsigned char cNByte = ucBuff[i*BYTESPERLINE(BMInfoHeader.Width, BMInfoHeader.BitCount) + j / 8]; //����������������� 8 ��������
					//���� ����������� (j == 8 * (j / 8)) �������� ������ �� 1, ����� ����������� �����:
					if ((j + 1) % 8 == 0) iIndCol = (j == (j / 8) * 8) ? (cNByte & 0x80) >> 1 : cNByte & 0x80;
					else iIndCol = cNByte & 0x80;
					if (iIndCol > 255 / 2) iIndCol = 1;
					else iIndCol = 0;
				}
				Rgbquad[i][j] = Pallete[iIndCol];
			}
		}
	}

	if (BMInfoHeader.BitCount == 32) {// ���������������� = 32
		fseek(file, BMFileHeader.OffsetBits, SEEK_SET);
		size_t padding = 0;
		if ((BMInfoHeader.Width*BMInfoHeader.BitCount / 8) % 4)
			padding = 4 - (BMInfoHeader.Width*BMInfoHeader.BitCount / 8) % 4;

		Rgbquad = new RGBQUAD*[BMInfoHeader.Height];  // ���������������������
		for (int i = 0; i < BMInfoHeader.Height; i++) {
			Rgbquad[i] = new RGBQUAD[BMInfoHeader.Width];
		}

		for (int i = 0; i < BMInfoHeader.Height; i++) {// ����������������������������������
			fread(Rgbquad[i], sizeof(RGBQUAD), BMInfoHeader.Width, file);
			if (padding != 0)
				fread(&Rgbquad, padding, 1, file);
		}
	}

	/*���� ����� ������ �� ������ 4 ������, �� ����� ������ ������� 1, 2 ��� 3 ������� �����.
	�� ���� ����� ��������/������� 4-(BITMAPINFOHEADER.Width*BITMAPINFOHEADER.BitCount/8)%4 ����� (00) ��� ����, ����� ������
	������/���������� ��������� ������.*/
	if (BMInfoHeader.BitCount == 24) {// ���� ������� ����� = 24
		BMInfoHeader.BitCount = 32; // �������� � � 32
		size_t padding = 0;
		if ((BMInfoHeader.Width*BMInfoHeader.BitCount / 8) % 4)
			padding = 4 - (BMInfoHeader.Width*BMInfoHeader.BitCount / 8) % 4;

		Rgbquad = new RGBQUAD*[BMInfoHeader.Height];// ���������������������
		for (int i = 0; i < BMInfoHeader.Height; i++) {
			Rgbquad[i] = new RGBQUAD[BMInfoHeader.Width];
		}

		for (int i = 0; i < BMInfoHeader.Height; i++) {// ����������������������������������
			for (int j = 0; j < BMInfoHeader.Width; j++) {
				RGBTRIPLE fileTriple;
				fread(&fileTriple, sizeof(RGBTRIPLE), 1, file);
				Rgbquad[i][j].Red = fileTriple.Red;
				Rgbquad[i][j].Green = fileTriple.Green;
				Rgbquad[i][j].Blue = fileTriple.Blue;
				Rgbquad[i][j].Reserved = 0;
			}
			if (padding != 0)
				fread(&Rgbquad, padding, 1, file);
		}
	}
	fclose(file); // �������������
}

//����������������������������:
Image::Image(const Image &copy) {
	BMInfoHeader = copy.BMInfoHeader; // ����������������������
	if (BMInfoHeader.BitCount <= 8) {// ����������������<= 8
		Pallete = new RGBQUAD[BMInfoHeader.ColorUsed]; // ��������������������������
		for (int i = 0; i < BMInfoHeader.ColorUsed; i++) // ����������
			Pallete[i] = copy.Pallete[i];

		Rgbquad = new RGBQUAD*[BMInfoHeader.Height]; // ���������������������
		for (int i = 0; i < BMInfoHeader.Height; i++) {
			Rgbquad[i] = new RGBQUAD[BMInfoHeader.Width];
		}

		for (int i = 0; i < BMInfoHeader.Height; i++) // ��������� ��� �������������� ������� 
			for (int j = 0; j < BMInfoHeader.Width; j++) {
				Rgbquad[i][j] = copy.Rgbquad[i][j];
			}
	}

	else
		if (BMInfoHeader.BitCount == 32) {// ���������������� = 32
			Rgbquad = new RGBQUAD*[BMInfoHeader.Height]; // ���������������������
			for (int i = 0; i < BMInfoHeader.Height; i++) {
				Rgbquad[i] = new RGBQUAD[BMInfoHeader.Width];
			}

			for (int i = 0; i < BMInfoHeader.Height; i++) // ��������� ��� �������������� ������� 
				for (int j = 0; j < BMInfoHeader.Width; j++) {
					Rgbquad[i][j] = copy.Rgbquad[i][j];
				}
		}
		else
			printf("������! ����������� 24-� ������ ����������� �� ��������������.\n");
}

//���������� ��������� = (����������� �����������):
Image Image::operator = (Image Inp) {
	if (BMInfoHeader.Size == 0) {// ���� ����������� ������
		BMInfoHeader = Inp.BMInfoHeader; // �������� ���������

		if (BMInfoHeader.BitCount <= 8) {// ���� ������� ����� <= 8
			Pallete = new RGBQUAD[BMInfoHeader.ColorUsed]; // ����� �������������� �������
			for (int i = 0; i < BMInfoHeader.ColorUsed; i++)// ����������
				Pallete[i] = Inp.Pallete[i];

			Rgbquad = new RGBQUAD*[BMInfoHeader.Height]; // ���������������������
			for (int i = 0; i < BMInfoHeader.Height; i++) {
				Rgbquad[i] = new RGBQUAD[BMInfoHeader.Width];
			}

			for (int i = 0; i < BMInfoHeader.Height; i++) // ��������� ��� �������������� ������� 
				for (int j = 0; j < BMInfoHeader.Width; j++) {
					Rgbquad[i][j] = Inp.Rgbquad[i][j];
				}
		}
		else
			if (BMInfoHeader.BitCount == 32) {// ���������������� = 32
				Rgbquad = new RGBQUAD*[BMInfoHeader.Height];// ���������������������
				for (int i = 0; i < BMInfoHeader.Height; i++) {
					Rgbquad[i] = new RGBQUAD[BMInfoHeader.Width];
				}

				for (int i = 0; i < BMInfoHeader.Height; i++)  // ��������� ��� �������������� ������� 
					for (int j = 0; j < BMInfoHeader.Width; j++) {
						Rgbquad[i][j] = Inp.Rgbquad[i][j];
					}
			}
			else
				printf("������! ����������� 24-� ������ ����������� �� ��������������.\n");
	}

	//���� ��������� ������� �����������:
	else if (BMInfoHeader.Width == Inp.BMInfoHeader.Width && BMInfoHeader.Height == Inp.BMInfoHeader.Height && BMInfoHeader.BitCount == Inp.BMInfoHeader.BitCount)
	{
		if (BMInfoHeader.BitCount <= 8) {// ���� ������� ����� <= 8
			BMInfoHeader.ColorUsed = Inp.BMInfoHeader.ColorUsed; // �������� ����� ������������ ������
			BMInfoHeader.SizeImage = Inp.BMInfoHeader.SizeImage; // �������� ������ ����������� 

			for (int i = 0; i < BMInfoHeader.ColorUsed; i++)// ����������������
				Pallete[i] = Inp.Pallete[i];

			Rgbquad = new RGBQUAD*[BMInfoHeader.Height];// ���������������������
			for (int i = 0; i < BMInfoHeader.Height; i++) {
				Rgbquad[i] = new RGBQUAD[BMInfoHeader.Width];
			}

			for (int i = 0; i < BMInfoHeader.Height; i++)// ��������� ��� �������������� �������
				for (int j = 0; j < BMInfoHeader.Width; j++) {
					Rgbquad[i][j] = Inp.Rgbquad[i][j];
				}
		}

		else
			if (BMInfoHeader.BitCount == 32) {// ���������������� = 32
				Rgbquad = new RGBQUAD*[BMInfoHeader.Height];// ���������������������
				for (int i = 0; i < BMInfoHeader.Height; i++) {
					Rgbquad[i] = new RGBQUAD[BMInfoHeader.Width];
				}

				for (int i = 0; i < BMInfoHeader.Height; i++)// ��������� ��� �������������� �������
					for (int j = 0; j < BMInfoHeader.Width; j++) {
						Rgbquad[i][j] = Inp.Rgbquad[i][j];
					}
			}
			else
				printf("������! ����������� 24-� ������ ����������� �� ��������������.\n");
	}
	else
		printf("������! �� ��������� �������� ��� ������� �����������.\n");
	return *this;
}

//���������� ��������� /= ��� ��������������� �����������:
Image Image::operator /=(const Image& mas) {
	double kH = (double)mas.BMInfoHeader.Height / BMInfoHeader.Height;
	double kW = (double)mas.BMInfoHeader.Width / BMInfoHeader.Width;

	if (mas.BMInfoHeader.BitCount == 32) { // ���������������� = 32
		for (int i = 0; i < BMInfoHeader.Height; i++) // �����������������������
			for (int j = 0; j < BMInfoHeader.Width; j++) {
				Rgbquad[i][j] = mas.Rgbquad[(int)(floor(kH*i))][(int)(floor(kW*j))];
			}
	}

	if (mas.BMInfoHeader.BitCount <= 8) { // ����������������<= 8
		Pallete = new RGBQUAD[BMInfoHeader.ColorUsed]; // ��������������������������
		for (int i = 0; i < BMInfoHeader.ColorUsed; i++) // ����������
			Pallete[i] = mas.Pallete[i];

		for (int i = 0; i < BMInfoHeader.Height; i++) // �����������������������
			for (int j = 0; j < BMInfoHeader.Width; j++) {
				Rgbquad[i][j] = mas.Rgbquad[(int)(floor(kH*i))][(int)(floor(kW*j))];
			}
	}
	return *this;
}

//���������� ��������� / ��� ��������� ������� �����:
Image Image::operator /(const unsigned short Depth) {
	Image Itemp = *this;
	if ((Depth == 1) || (Depth == 8) || (Depth == 32)) {// ����������������������������������������
		if (Depth == Itemp.BMInfoHeader.BitCount) return Itemp; // �������������������������������������������

		if (Depth == 32) {//�������������� 1 � 32, 8 � 32
			delete[] Itemp.Pallete;
			Itemp.Pallete = NULL;
			Itemp.BMInfoHeader.BitCount = 32; // ������������������
			Itemp.BMInfoHeader.ColorUsed = 0;
			// �����������������������:
			Itemp.BMInfoHeader.SizeImage = Itemp.BMInfoHeader.Height*BYTESPERLINE(Itemp.BMInfoHeader.Width, Itemp.BMInfoHeader.BitCount);
			return Itemp;
		}

		if (Depth == 8 && Itemp.BMInfoHeader.BitCount == 1) { //�������������� 1 � 8
			int OldColorUsed = Itemp.BMInfoHeader.ColorUsed; //���������������������������
			Itemp.BMInfoHeader.BitCount = 8;// ������������������
			Itemp.BMInfoHeader.ColorUsed = 256;
			// �����������������������:
			Itemp.BMInfoHeader.SizeImage = Itemp.BMInfoHeader.Height*BYTESPERLINE(Itemp.BMInfoHeader.Width, Itemp.BMInfoHeader.BitCount);

			RGBQUAD *BufPall = new RGBQUAD[Itemp.BMInfoHeader.ColorUsed];// ��������������������������
			for (int i = 0; i < OldColorUsed; i++) // ����������
				BufPall[i] = Itemp.Pallete[i];
			delete[]Itemp.Pallete;
			Itemp.Pallete = BufPall;
			return Itemp;
		}

		if (Depth == 8 && Itemp.BMInfoHeader.BitCount == 32) { //32 � 8
			Itemp.BMInfoHeader.BitCount = 8; // ������������������
			Itemp.BMInfoHeader.ColorUsed = 256;
			// �����������������������:
			Itemp.BMInfoHeader.SizeImage = Itemp.BMInfoHeader.Height*BYTESPERLINE(Itemp.BMInfoHeader.Width, Itemp.BMInfoHeader.BitCount);

			Itemp.Pallete = new RGBQUAD[Itemp.BMInfoHeader.ColorUsed];// ��������������������������
			for (int i = 0; i < Itemp.BMInfoHeader.ColorUsed; i++) { // ����������
				RGBQUAD t = { i, i, i, 0 };
				Itemp.Pallete[i] = t;
			}

			for (int i = 0; i < Itemp.BMInfoHeader.Height; i++)  //�������������������
				for (int j = 0; j < Itemp.BMInfoHeader.Width; j++) {
					RGBQUAD t = Itemp.Rgbquad[i][j];
					double dd = double(t.Red)*0.299 + double(t.Green)*0.597 + double(t.Blue)*0.114;
					int gray = int(floor(dd + 0.5));
					if (gray > 255) gray = 255;
					if (gray < 0) gray = 0;
					RGBQUAD t2 = { gray, gray, gray, 0 };
					Itemp.Rgbquad[i][j] = t2;
				}
			return Itemp;
		}

		if (Depth == 1 && Itemp.BMInfoHeader.BitCount == 32) { // 32 � 1 
			Itemp.BMInfoHeader.BitCount = 1; // ������������������
			Itemp.BMInfoHeader.ColorUsed = 2;
			// �����������������������:
			Itemp.BMInfoHeader.SizeImage = Itemp.BMInfoHeader.Height*BYTESPERLINE(Itemp.BMInfoHeader.Width, Itemp.BMInfoHeader.BitCount);

			Itemp.Pallete = new RGBQUAD[Itemp.BMInfoHeader.ColorUsed];// ��������������������������
			RGBQUAD t = { 255, 255, 255, 0 };// ����������
			Itemp.Pallete[1] = t;
			t = { 0, 0, 0, 0 };
			Itemp.Pallete[0] = t;

			for (int i = 0; i < Itemp.BMInfoHeader.Height; i++) //�������������������
				for (int j = 0; j < Itemp.BMInfoHeader.Width; j++) {
					RGBQUAD t = Itemp.Rgbquad[i][j];
					//�������� �������:
					double dd = double(t.Red)*0.299 + double(t.Green)*0.597 + double(t.Blue)*0.114;
					int gray = floor(dd);
					if (gray > 126) gray = 255;
					else gray = 0;
					RGBQUAD t2 = { gray, gray, gray, 0 };
					Itemp.Rgbquad[i][j] = t2;
				}
			return Itemp;
		}

		if (Depth == 1 && Itemp.BMInfoHeader.BitCount == 8) { // 8 � 1
			Itemp.BMInfoHeader.BitCount = 1;// ������������������
			Itemp.BMInfoHeader.ColorUsed = (1 << 1);
			// �����������������������
			Itemp.BMInfoHeader.SizeImage = Itemp.BMInfoHeader.Height*BYTESPERLINE(Itemp.BMInfoHeader.Width, Itemp.BMInfoHeader.BitCount);

			delete[]Itemp.Pallete;
			Itemp.Pallete = new RGBQUAD[Itemp.BMInfoHeader.ColorUsed];// ��������������������������
			Itemp.Pallete = new RGBQUAD[Itemp.BMInfoHeader.ColorUsed];
			RGBQUAD t = { 0, 0, 0, 0 };// ����������
			Itemp.Pallete[0] = t;
			t = { 255, 255, 255, 0 };
			Itemp.Pallete[1] = t;

			for (int i = 0; i < Itemp.BMInfoHeader.Height; i++)  //�������������������
				for (int j = 0; j < Itemp.BMInfoHeader.Width; j++) {
					RGBQUAD t = Itemp.Rgbquad[i][j];
					double dd = double(t.Red)*0.299 + double(t.Green)*0.597 + double(t.Blue)*0.114;
					int gray = int(floor(dd + 0.5));
					if (gray > 127) gray = 255;
					else gray = 0;
					int Col2 = int(gray);
					RGBQUAD t2 = { Col2, Col2, Col2, 0 };
					Itemp.Rgbquad[i][j] = t2;
				}
			return Itemp;
		}
		return Itemp;
	}
	else return Itemp;
}

//����������:
Image::~Image() {
	if (BMInfoHeader.BitCount <= 8 && Pallete != NULL) {//����������������< 8 ���
		delete[]Pallete;  //��������������
		for (int i = 0; i < BMInfoHeader.Height; i++) //������������������
			delete[]Rgbquad[i];
	}
	if (BMInfoHeader.BitCount == 32 && Pallete != NULL) //����������� = 32 ���
	{
		//delete[]Pallete; //��������������
		for (int i = 0; i < BMInfoHeader.Height; i++)//������������������
			delete[]Rgbquad[i];
	}
}

//����� �������� ����������� ����������� ������������:
int Image::loadimage(char *fileName) {
	FILE *file;
	errno_t e = fopen_s(&file, fileName, "rb");;    //�������������
	BITMAPFILEHEADER BMFileHeader;
	fread(&BMFileHeader, sizeof(BITMAPFILEHEADER), 1, file);
	if (BMFileHeader.Type != 0x4D42) { // ���� ��� �� ������������ �������
		printf("������! ����������� ������ �����.\n");
		return 0;
	}
	fread(&BMInfoHeader, sizeof(BITMAPINFOHEADER), 1, file); // ���� ������� ����� �� ������������ ������
	if (BMInfoHeader.BitCount == 4) {
		printf("������! ���������������� �������� �����������.\n");
		return 0;
	}

	if (BMInfoHeader.BitCount <= 8) {// ���� ������� ����� <= 8
		BMInfoHeader.ColorUsed = 1 << BMInfoHeader.BitCount;
		Pallete = new RGBQUAD[BMInfoHeader.ColorUsed]; // ����� �������������� �������
		if (fread(Pallete, sizeof(RGBQUAD)*BMInfoHeader.ColorUsed, 1, file) != 1) {
			delete[]Pallete;
			fclose(file);
			return 1;
		}
		BMInfoHeader.SizeImage = BMInfoHeader.Height*BYTESPERLINE(BMInfoHeader.Width, BMInfoHeader.BitCount); // ������������������������
		unsigned char *ucBuff = new unsigned char[BMInfoHeader.SizeImage];
		fread(ucBuff, BMInfoHeader.SizeImage, 1, file);

		Rgbquad = new RGBQUAD*[BMInfoHeader.Height]; // ���������������������
		for (int i = 0; i < BMInfoHeader.Height; i++) {
			Rgbquad[i] = new RGBQUAD[BMInfoHeader.Width];
		}

		for (int i = 0; i < BMInfoHeader.Height; i++) {// ����������������������������������
			for (int j = 0; j < BMInfoHeader.Width; j++) {
				int iIndCol;

				if (BMInfoHeader.BitCount == 8) {//���������������� = 8
					iIndCol = ucBuff[i*BYTESPERLINE(BMInfoHeader.Width, BMInfoHeader.BitCount) + j];
				}
				if (BMInfoHeader.BitCount == 1) {//���������������� = 1
					unsigned char cNByte = ucBuff[i*BYTESPERLINE(BMInfoHeader.Width, BMInfoHeader.BitCount) + j / 8]; //����������������� 8 ��������
					if ((j + 1) % 8 == 0) iIndCol = (j == (j / 8) * 8) ? (cNByte & 0x80) >> 1 : cNByte & 0x80; //���� ����������� (j == 8 * (j / 8)) �������� ������ �� 1, ����� ����������� ����� 
					else iIndCol = cNByte & 0x80;
					if (iIndCol > 255 / 2) iIndCol = 1;
					else iIndCol = 0;
				}
				Rgbquad[i][j] = Pallete[iIndCol];
			}
		}
	}

	if (BMInfoHeader.BitCount == 32) {// ���������������� = 32
		fseek(file, BMFileHeader.OffsetBits, SEEK_SET);
		size_t padding = 0;
		if ((BMInfoHeader.Width*BMInfoHeader.BitCount / 8) % 4)
			padding = 4 - (BMInfoHeader.Width*BMInfoHeader.BitCount / 8) % 4;

		Rgbquad = new RGBQUAD*[BMInfoHeader.Height]; // ���������������������
		for (int i = 0; i < BMInfoHeader.Height; i++) {
			Rgbquad[i] = new RGBQUAD[BMInfoHeader.Width];
		}

		for (int i = 0; i < BMInfoHeader.Height; i++) {// ����������������������������������
			fread(Rgbquad[i], sizeof(RGBQUAD), BMInfoHeader.Width, file);
			if (padding != 0)
				fread(&Rgbquad, padding, 1, file);
		}
	}

	if (BMInfoHeader.BitCount == 24) {// ���������������� = 24
		BMInfoHeader.BitCount = 32; // ���������� 32
		size_t padding = 0;
		if ((BMInfoHeader.Width*BMInfoHeader.BitCount / 8) % 4)
			padding = 4 - (BMInfoHeader.Width*BMInfoHeader.BitCount / 8) % 4;

		Rgbquad = new RGBQUAD*[BMInfoHeader.Height]; // ���������������������
		for (int i = 0; i < BMInfoHeader.Height; i++) {
			Rgbquad[i] = new RGBQUAD[BMInfoHeader.Width];
		}

		for (int i = 0; i < BMInfoHeader.Height; i++) {// ����������������������������������
			for (int j = 0; j < BMInfoHeader.Width; j++) {
				RGBTRIPLE fileTriple;
				fread(&fileTriple, sizeof(RGBTRIPLE), 1, file);
				Rgbquad[i][j].Red = fileTriple.Red;
				Rgbquad[i][j].Green = fileTriple.Green;
				Rgbquad[i][j].Blue = fileTriple.Blue;
				Rgbquad[i][j].Reserved = 0;
			}
			if (padding != 0)
				fread(&Rgbquad, padding, 1, file);
		}
	}
	fclose(file); // �������������
	return 1;
}

//����� ������ ����������� � ����:
void Image::writeimage(char *fileName) {

	if (!Rgbquad) { // ���� ��� ������
		printf("������! � ����������� ��� ������ ��� ������.\n");
		return;
	}

	FILE *file;
	errno_t e = fopen_s(&file, fileName, "wb");//�������������

	BITMAPFILEHEADER BMFileHeader; // �������������������
	BMFileHeader.Type = 0x4D42;
	BMFileHeader.Reserved1 = 0;
	BMFileHeader.Reserved2 = 0;
	BMFileHeader.OffsetBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	BMFileHeader.Size = BMInfoHeader.SizeImage + BMFileHeader.OffsetBits;
	fwrite(&BMFileHeader, sizeof(BITMAPFILEHEADER), 1, file);
	fwrite(&BMInfoHeader, sizeof(BITMAPINFOHEADER), 1, file);

	if (BMInfoHeader.BitCount <= 8) {// ����������������<= 8
		fwrite(Pallete, BMInfoHeader.ColorUsed * sizeof(RGBQUAD), 1, file);
		unsigned char *ucBuff = new unsigned char[BMInfoHeader.SizeImage]; //�������������������

		for (int i = 0; i < BMInfoHeader.Height; i++) {// ����������������
			for (int j = 0; j < BMInfoHeader.Width; j++) {
				int iIndCol = LocateInPallete(Rgbquad[i][j]);
				if (BMInfoHeader.BitCount == 8) {// ���������������� = 8 
					ucBuff[i*BYTESPERLINE(BMInfoHeader.Width, BMInfoHeader.BitCount) + j] = iIndCol; // ����������������
				}
				if (BMInfoHeader.BitCount == 1) {// ���������������� = 1
					int iNumb = i * BYTESPERLINE(BMInfoHeader.Width, BMInfoHeader.BitCount) + (j / 8); // �������������������������������������
					if ((j + 1) % 8 != 0) ucBuff[iNumb] = (8 * (j / 8) == j) ? (iIndCol) << 1 : ((ucBuff[iNumb] + iIndCol) << 1);
					else ucBuff[iNumb] = ucBuff[iNumb] + iIndCol;
				}
			}
			if (BMInfoHeader.BitCount == 8) {// ���������������� = 8
				for (int j = BMInfoHeader.Width; j < BYTESPERLINE(BMInfoHeader.Width, BMInfoHeader.BitCount); j++) { // ����������������
					ucBuff[i*BYTESPERLINE(BMInfoHeader.Width, BMInfoHeader.BitCount) + j] = 0;
				}
			}
			if (BMInfoHeader.BitCount == 1) {// ���������������� = 1
				{
					for (int j = (BMInfoHeader.Width / 8) + 1; j < BYTESPERLINE(BMInfoHeader.Width, BMInfoHeader.BitCount); j++) { // ������ ���� �� ������ ��� ����� � ����������
						ucBuff[i*BYTESPERLINE(BMInfoHeader.Width, BMInfoHeader.BitCount) + j] = 0;
					}
				}
			}
		}
		fwrite(ucBuff, BMInfoHeader.SizeImage, 1, file);
		delete[]ucBuff;
	}

	if (BMInfoHeader.BitCount == 32) {// ���������������� = 32
		size_t padding = 0;
		if ((BMInfoHeader.Width*BMInfoHeader.BitCount / 8) % 4)
			padding = 4 - (BMInfoHeader.Width*BMInfoHeader.BitCount / 8) % 4;
		for (int i = 0; i < BMInfoHeader.Height; i++) {// ����������������
			fwrite(Rgbquad[i], sizeof(RGBQUAD), BMInfoHeader.Width, file);
			if (padding != 0)
				fwrite(&Rgbquad, padding, 1, file);
		}
	}
	fclose(file);
}

int main() {
	setlocale(LC_ALL, "rus");

	Image img1((char*)"home.bmp"); //C������� ������� ������ Image, ����������� ������ �� ����� input1.bmp

	Image img2(0, 32, 315, 177); //C������� ������� 8 ������� ����������� �������� 315*177
	img2 /= img1; //���������� img1 � �������� img2
	img2.writeimage((char*)"home_resized.bmp");

	Image img3(0, 8, img1.Widht(), img1.Height()); //�������� ������� �����������
	img3 /= img1/8; //������ img1 � img3 � ���������� ������� ����� �� 8 ���
	img3.writeimage((char*)"home_8.bmp");

	Image img4(0, 1, img1.Widht(), img1.Height()); //�������� ������� �����������
	img4 = (img1 / 1); //������ img1 � img4 � ���������� ������� ����� �� 1 ���
	img4.writeimage((char*)"home_1.bmp");

	system("pause");
	return 0;
}