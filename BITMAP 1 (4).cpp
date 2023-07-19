#define _CRT_SECURE_NO_WARNINGS
#include <locale>
#include <stdio.h>
typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef unsigned char BYTE;

// макрос для определения количества байт в выровненной по DWORD строке пикселов в DIB 
// Width - длина строки в пикселах; BPP - бит на пиксел
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
	DWORD Size;//Число байтов, необходимое для структуры =40
	DWORD Width;//Ширина точечного рисунка в пикселях
	DWORD Height;//Высота точечного рисунка в пикселях
	WORD Planes;//Число плоскостей целевого устройства =1
	WORD BitCount;//Глубина цвета, число бит на точку = 0,1,4,8,16, 24,32
	DWORD Compression;//Тип сжатия=0 для несжатого изображения
	DWORD SizeImage;//Размер изображения в байтах BitCount*Height*Width
	DWORD XPelsPerMeter;//Разрешающая способность по горизонтали
	DWORD YPelsPerMeter;//Разрешающая способность по вертикали
	DWORD ColorUsed;//Число индексов, используемых цветов. Если все цвета=0
	DWORD ColorImportant;//Число необходимых цветов =0
};
#pragma pack(pop)

#pragma pack(push, 1)
//Cтруктура, содержащая данные изображения
//(описание цвета RGBQ):
struct RGBQUAD {
	BYTE Blue;
	BYTE Green;
	BYTE Red;
	BYTE Reserved; //данныеальфа-каналавпалитровыхизображениях.
};
#pragma pack(pop)

#pragma pack(push, 1)
//Cтруктура, содержащая данные изображения
//(описание цвета RGBT):
struct RGBTRIPLE {
	BYTE Blue;
	BYTE Green;
	BYTE Red;
};
#pragma pack(pop)

class Image {
	BITMAPINFOHEADER BMInfoHeader; //Объектструктуры BITMAPINFOHEADER
	RGBQUAD **Rgbquad; //Двумерныймассивтипа RGBQUAD
	RGBQUAD *Pallete; //одномерный массив типа RGBQUAD для палитровых изображений 1 и 8 бит.
	int LocateInPallete(const RGBQUAD &rgbq);//Определениекол-ваиспользуемыхцветов
public:
	Image(char Mode, unsigned short BCount, int Width, int Height); // Конструкторсозданияизображения
	Image(char *fileName); // Конструктор объекта изображения из файла
	Image(); // Конструктор без параметров, создает пустой контейнер под изображение
	Image(const Image &copy); // Конструкторкопии
	~Image(); // Деструктор
	int loadimage(char *fileName); // Метод загрузки изображения аналогичный конструктору
	void writeimage(char *fileName); // Метод записи изображения в файл
	Image operator = (Image Inp); // Перегрузкаоператора =
	Image operator /=(const Image& mas);   // Перегрузкаоператора /= (масштаб)
	Image operator/(const unsigned short Depth);  // Перегрузкаоператора / (глубинацвета)
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

//Конструкторсозданияизображения:
Image::Image(char Mode, unsigned short BCount, int Width, int Height) {
	// Ининциализация заголовка изображения.
	//Создаём контейнер с заданной высотой, шириной и глубиной цвета:
	BMInfoHeader.Size = 40;  // Число байтов необходимое для структуры = 40
	BMInfoHeader.Width = Width;
	BMInfoHeader.Height = Height;
	BMInfoHeader.Planes = 1; // Число плоскостей целевого устройства = 1
	BMInfoHeader.BitCount = BCount; //Глубина цвета, число бит на точку = 24,32
	BMInfoHeader.Compression = 0; // Тип сжатия = 0 для несжатого изображения.
	//BMInfoHeader.SizeImage = BCount*Width*Height;
	BMInfoHeader.SizeImage = Height * BYTESPERLINE(Width, BCount);
	BMInfoHeader.XPelsPerMeter = 0;
	BMInfoHeader.YPelsPerMeter = 0;
	BMInfoHeader.ColorUsed = 0;
	BMInfoHeader.ColorImportant = 0;

	if (BMInfoHeader.BitCount <= 8) {//еслиглубинацвета = 8
		BMInfoHeader.ColorUsed = (1 << BMInfoHeader.BitCount); // считаемкол-воиспользуемыхцветов
		Pallete = new RGBQUAD[BMInfoHeader.ColorUsed];    // создёмсоответсвующуюпалитру

		for (int i = 0; i < BMInfoHeader.ColorUsed; i++) {// заполняемеё
			Pallete[i].Blue = 0;
			Pallete[i].Green = 0;
			Pallete[i].Red = 0;
			Pallete[i].Reserved = 0;
		}
		Pallete[0].Blue = Mode;
		Pallete[0].Green = Mode;
		Pallete[0].Red = Mode;
		Pallete[0].Reserved = 0;

		Rgbquad = new RGBQUAD*[BMInfoHeader.Height]; // создаёмдвумерныймассивдляегозаполнения
		for (int i = 0; i < BMInfoHeader.Height; i++) {
			Rgbquad[i] = new RGBQUAD[BMInfoHeader.Width];
		}

		for (int i = 0; i < BMInfoHeader.Height; i++) { //заполняем его в соответствии с указанным модификатором
			for (int j = 0; j < BMInfoHeader.Width; j++) {
				Rgbquad[i][j].Red = Mode;
				Rgbquad[i][j].Green = Mode;
				Rgbquad[i][j].Blue = Mode;
				Rgbquad[i][j].Reserved = 0;
			}
		}
	}

	else
		if (BMInfoHeader.BitCount == 32) {//еслиглубинацвета = 32
			Rgbquad = new RGBQUAD*[BMInfoHeader.Height];  //создаёмдвумерныймассив
			for (int i = 0; i < BMInfoHeader.Height; i++) {
				Rgbquad[i] = new RGBQUAD[BMInfoHeader.Width];
			}

			for (int i = 0; i < BMInfoHeader.Height; i++) { //заполняем его в соответствии с указанным модификатором
				for (int j = 0; j < BMInfoHeader.Width; j++) {
					Rgbquad[i][j].Red = Mode;
					Rgbquad[i][j].Green = Mode;
					Rgbquad[i][j].Blue = Mode;
					Rgbquad[i][j].Reserved = 0;
				}
			}
		}
};

//Создаём пустой контейнер под изображение:
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

//Конструктор считывания данных изображения из файла:
Image::Image(char *fileName) {
	FILE *file;   //файл
	errno_t e;
	e = fopen_s(&file, fileName, "rb");//открываемфайл
	BITMAPFILEHEADER BMFileHeader;
	fread(&BMFileHeader, sizeof(BITMAPFILEHEADER), 1, file);

	if (BMFileHeader.Type != 0x4D42) {	//если тип не соответсвует нужному
		printf("Ошибка! Неизвестный формат файла.\n");
		return;
	}
	fread(&BMInfoHeader, sizeof(BITMAPINFOHEADER), 1, file);

	if (BMInfoHeader.BitCount != 32 && BMInfoHeader.BitCount != 24) // если глубина цвета не соответсвует нужной
		fread(&BMInfoHeader, sizeof(BITMAPINFOHEADER), 1, file);
	if (BMInfoHeader.BitCount == 4) {
		printf("Ошибка! Неподдерживаемая битность изображения.\n");
		return;
	}

	if (BMInfoHeader.BitCount <= 8) {// если глубина цвета <= 8
		BMInfoHeader.ColorUsed = 1 << BMInfoHeader.BitCount;
		Pallete = new RGBQUAD[BMInfoHeader.ColorUsed]; // создём соответсвующую палитру

		if (fread(Pallete, sizeof(RGBQUAD)*BMInfoHeader.ColorUsed, 1, file) != 1) {
			delete[]Pallete;
			fclose(file);
			return;
		}
		BMInfoHeader.SizeImage = BMInfoHeader.Height*BYTESPERLINE(BMInfoHeader.Width, BMInfoHeader.BitCount); //считаемразмеризображения
		unsigned char *ucBuff = new unsigned char[BMInfoHeader.SizeImage];
		fread(ucBuff, BMInfoHeader.SizeImage, 1, file);

		Rgbquad = new RGBQUAD*[BMInfoHeader.Height]; // создаёмдвумерныймассив
		for (int i = 0; i < (int)BMInfoHeader.Height; i++) {
			Rgbquad[i] = new RGBQUAD[BMInfoHeader.Width];
		}

		for (int i = 0; i < BMInfoHeader.Height; i++) {// заполняемегосоответсвующимиданными
			for (int j = 0; j < BMInfoHeader.Width; j++) {
				int iIndCol;
				if (BMInfoHeader.BitCount == 8) {//еслиглубинацвета = 8
					iIndCol = ucBuff[i*BYTESPERLINE(BMInfoHeader.Width, BMInfoHeader.BitCount) + j];
				}
				if (BMInfoHeader.BitCount == 1) {//еслиглубинацвета = 1
					unsigned char cNByte = ucBuff[i*BYTESPERLINE(BMInfoHeader.Width, BMInfoHeader.BitCount) + j / 8]; //одинбайтописывает 8 пикселей
					//Если выполняется (j == 8 * (j / 8)) сдвигаем вправо на 1, иначе накладываем маску:
					if ((j + 1) % 8 == 0) iIndCol = (j == (j / 8) * 8) ? (cNByte & 0x80) >> 1 : cNByte & 0x80;
					else iIndCol = cNByte & 0x80;
					if (iIndCol > 255 / 2) iIndCol = 1;
					else iIndCol = 0;
				}
				Rgbquad[i][j] = Pallete[iIndCol];
			}
		}
	}

	if (BMInfoHeader.BitCount == 32) {// еслиглубинацвета = 32
		fseek(file, BMFileHeader.OffsetBits, SEEK_SET);
		size_t padding = 0;
		if ((BMInfoHeader.Width*BMInfoHeader.BitCount / 8) % 4)
			padding = 4 - (BMInfoHeader.Width*BMInfoHeader.BitCount / 8) % 4;

		Rgbquad = new RGBQUAD*[BMInfoHeader.Height];  // создаёмдвумерныймассив
		for (int i = 0; i < BMInfoHeader.Height; i++) {
			Rgbquad[i] = new RGBQUAD[BMInfoHeader.Width];
		}

		for (int i = 0; i < BMInfoHeader.Height; i++) {// заполняемегосоответсвующимиданными
			fread(Rgbquad[i], sizeof(RGBQUAD), BMInfoHeader.Width, file);
			if (padding != 0)
				fread(&Rgbquad, padding, 1, file);
		}
	}

	/*Если длина строки не кратна 4 байтам, то после строки следует 1, 2 или 3 нулевых байта.
	То есть нужно добавить/считать 4-(BITMAPINFOHEADER.Width*BITMAPINFOHEADER.BitCount/8)%4 байта (00) для того, чтобы начать
	запись/считывание следующей строки.*/
	if (BMInfoHeader.BitCount == 24) {// если глубина цвета = 24
		BMInfoHeader.BitCount = 32; // приводим её к 32
		size_t padding = 0;
		if ((BMInfoHeader.Width*BMInfoHeader.BitCount / 8) % 4)
			padding = 4 - (BMInfoHeader.Width*BMInfoHeader.BitCount / 8) % 4;

		Rgbquad = new RGBQUAD*[BMInfoHeader.Height];// создаёмдвумерныймассив
		for (int i = 0; i < BMInfoHeader.Height; i++) {
			Rgbquad[i] = new RGBQUAD[BMInfoHeader.Width];
		}

		for (int i = 0; i < BMInfoHeader.Height; i++) {// заполняемегосоответсвующимиданными
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
	fclose(file); // закрываемфайл
}

//Конструкторкопированияданных:
Image::Image(const Image &copy) {
	BMInfoHeader = copy.BMInfoHeader; // копируемзаголовокфайла
	if (BMInfoHeader.BitCount <= 8) {// еслиглубинацвета<= 8
		Pallete = new RGBQUAD[BMInfoHeader.ColorUsed]; // создёмсоответсвующуюпалитру
		for (int i = 0; i < BMInfoHeader.ColorUsed; i++) // заполняемеё
			Pallete[i] = copy.Pallete[i];

		Rgbquad = new RGBQUAD*[BMInfoHeader.Height]; // создаёмдвумерныймассив
		for (int i = 0; i < BMInfoHeader.Height; i++) {
			Rgbquad[i] = new RGBQUAD[BMInfoHeader.Width];
		}

		for (int i = 0; i < BMInfoHeader.Height; i++) // заполняем его скопированными данными 
			for (int j = 0; j < BMInfoHeader.Width; j++) {
				Rgbquad[i][j] = copy.Rgbquad[i][j];
			}
	}

	else
		if (BMInfoHeader.BitCount == 32) {// еслиглубинацвета = 32
			Rgbquad = new RGBQUAD*[BMInfoHeader.Height]; // создаёмдвумерныймассив
			for (int i = 0; i < BMInfoHeader.Height; i++) {
				Rgbquad[i] = new RGBQUAD[BMInfoHeader.Width];
			}

			for (int i = 0; i < BMInfoHeader.Height; i++) // заполняем его скопированными данными 
				for (int j = 0; j < BMInfoHeader.Width; j++) {
					Rgbquad[i][j] = copy.Rgbquad[i][j];
				}
		}
		else
			printf("Ошбика! Копирование 24-х битных изображений не поддерживается.\n");
}

//Перегрузка оператора = (копирование изображения):
Image Image::operator = (Image Inp) {
	if (BMInfoHeader.Size == 0) {// Если изображение пустое
		BMInfoHeader = Inp.BMInfoHeader; // копируем заголовок

		if (BMInfoHeader.BitCount <= 8) {// если глубина цвета <= 8
			Pallete = new RGBQUAD[BMInfoHeader.ColorUsed]; // создём соответсвующую палитру
			for (int i = 0; i < BMInfoHeader.ColorUsed; i++)// заполняемеё
				Pallete[i] = Inp.Pallete[i];

			Rgbquad = new RGBQUAD*[BMInfoHeader.Height]; // создаёмдвумерныймассив
			for (int i = 0; i < BMInfoHeader.Height; i++) {
				Rgbquad[i] = new RGBQUAD[BMInfoHeader.Width];
			}

			for (int i = 0; i < BMInfoHeader.Height; i++) // заполняем его скопированными данными 
				for (int j = 0; j < BMInfoHeader.Width; j++) {
					Rgbquad[i][j] = Inp.Rgbquad[i][j];
				}
		}
		else
			if (BMInfoHeader.BitCount == 32) {// еслиглубинацвета = 32
				Rgbquad = new RGBQUAD*[BMInfoHeader.Height];// создаёмдвумерныймассив
				for (int i = 0; i < BMInfoHeader.Height; i++) {
					Rgbquad[i] = new RGBQUAD[BMInfoHeader.Width];
				}

				for (int i = 0; i < BMInfoHeader.Height; i++)  // заполняем его скопированными данными 
					for (int j = 0; j < BMInfoHeader.Width; j++) {
						Rgbquad[i][j] = Inp.Rgbquad[i][j];
					}
			}
			else
				printf("Ошбика! Копирование 24-х битных изображений не поддерживается.\n");
	}

	//Если совпадают размеры изображений:
	else if (BMInfoHeader.Width == Inp.BMInfoHeader.Width && BMInfoHeader.Height == Inp.BMInfoHeader.Height && BMInfoHeader.BitCount == Inp.BMInfoHeader.BitCount)
	{
		if (BMInfoHeader.BitCount <= 8) {// если глубина цвета <= 8
			BMInfoHeader.ColorUsed = Inp.BMInfoHeader.ColorUsed; // копируем число используемых цветов
			BMInfoHeader.SizeImage = Inp.BMInfoHeader.SizeImage; // копируем размер изображения 

			for (int i = 0; i < BMInfoHeader.ColorUsed; i++)// заполняемпалитру
				Pallete[i] = Inp.Pallete[i];

			Rgbquad = new RGBQUAD*[BMInfoHeader.Height];// создаёмдвумерныймассив
			for (int i = 0; i < BMInfoHeader.Height; i++) {
				Rgbquad[i] = new RGBQUAD[BMInfoHeader.Width];
			}

			for (int i = 0; i < BMInfoHeader.Height; i++)// заполняем его скопированными данными
				for (int j = 0; j < BMInfoHeader.Width; j++) {
					Rgbquad[i][j] = Inp.Rgbquad[i][j];
				}
		}

		else
			if (BMInfoHeader.BitCount == 32) {// еслиглубинацвета = 32
				Rgbquad = new RGBQUAD*[BMInfoHeader.Height];// создаёмдвумерныймассив
				for (int i = 0; i < BMInfoHeader.Height; i++) {
					Rgbquad[i] = new RGBQUAD[BMInfoHeader.Width];
				}

				for (int i = 0; i < BMInfoHeader.Height; i++)// заполняем его скопированными данными
					for (int j = 0; j < BMInfoHeader.Width; j++) {
						Rgbquad[i][j] = Inp.Rgbquad[i][j];
					}
			}
			else
				printf("Ошбика! Копирование 24-х битных изображений не поддерживается.\n");
	}
	else
		printf("Ошибка! Не совпадает битность или размеры изображения.\n");
	return *this;
}

//Перегрузка оператора /= для масштабирования изображений:
Image Image::operator /=(const Image& mas) {
	double kH = (double)mas.BMInfoHeader.Height / BMInfoHeader.Height;
	double kW = (double)mas.BMInfoHeader.Width / BMInfoHeader.Width;

	if (mas.BMInfoHeader.BitCount == 32) { // еслиглубинацвета = 32
		for (int i = 0; i < BMInfoHeader.Height; i++) // масштабируемизображение
			for (int j = 0; j < BMInfoHeader.Width; j++) {
				Rgbquad[i][j] = mas.Rgbquad[(int)(floor(kH*i))][(int)(floor(kW*j))];
			}
	}

	if (mas.BMInfoHeader.BitCount <= 8) { // еслиглубинацвета<= 8
		Pallete = new RGBQUAD[BMInfoHeader.ColorUsed]; // создёмсоответсвующуюпалитру
		for (int i = 0; i < BMInfoHeader.ColorUsed; i++) // заполняемеё
			Pallete[i] = mas.Pallete[i];

		for (int i = 0; i < BMInfoHeader.Height; i++) // масштабируемизображение
			for (int j = 0; j < BMInfoHeader.Width; j++) {
				Rgbquad[i][j] = mas.Rgbquad[(int)(floor(kH*i))][(int)(floor(kW*j))];
			}
	}
	return *this;
}

//Перегрузка оператора / для изменения глубины цвета:
Image Image::operator /(const unsigned short Depth) {
	Image Itemp = *this;
	if ((Depth == 1) || (Depth == 8) || (Depth == 32)) {// есливыбраннаяглубинаизображениядопустима
		if (Depth == Itemp.BMInfoHeader.BitCount) return Itemp; // есливыбраннаяглубинаравнаглубинеизображения

		if (Depth == 32) {//преобразование 1 в 32, 8 в 32
			delete[] Itemp.Pallete;
			Itemp.Pallete = NULL;
			Itemp.BMInfoHeader.BitCount = 32; // меняемглубинуцвета
			Itemp.BMInfoHeader.ColorUsed = 0;
			// меняемразмеризображения:
			Itemp.BMInfoHeader.SizeImage = Itemp.BMInfoHeader.Height*BYTESPERLINE(Itemp.BMInfoHeader.Width, Itemp.BMInfoHeader.BitCount);
			return Itemp;
		}

		if (Depth == 8 && Itemp.BMInfoHeader.BitCount == 1) { //преобразование 1 в 8
			int OldColorUsed = Itemp.BMInfoHeader.ColorUsed; //сохраняеминформациюопалитре
			Itemp.BMInfoHeader.BitCount = 8;// меняемглубинуцвета
			Itemp.BMInfoHeader.ColorUsed = 256;
			// меняемразмеризображения:
			Itemp.BMInfoHeader.SizeImage = Itemp.BMInfoHeader.Height*BYTESPERLINE(Itemp.BMInfoHeader.Width, Itemp.BMInfoHeader.BitCount);

			RGBQUAD *BufPall = new RGBQUAD[Itemp.BMInfoHeader.ColorUsed];// создёмсоответсвующуюпалитру
			for (int i = 0; i < OldColorUsed; i++) // заполняемеё
				BufPall[i] = Itemp.Pallete[i];
			delete[]Itemp.Pallete;
			Itemp.Pallete = BufPall;
			return Itemp;
		}

		if (Depth == 8 && Itemp.BMInfoHeader.BitCount == 32) { //32 в 8
			Itemp.BMInfoHeader.BitCount = 8; // меняемглубинуцвета
			Itemp.BMInfoHeader.ColorUsed = 256;
			// меняемразмеризображения:
			Itemp.BMInfoHeader.SizeImage = Itemp.BMInfoHeader.Height*BYTESPERLINE(Itemp.BMInfoHeader.Width, Itemp.BMInfoHeader.BitCount);

			Itemp.Pallete = new RGBQUAD[Itemp.BMInfoHeader.ColorUsed];// создёмсоответсвующуюпалитру
			for (int i = 0; i < Itemp.BMInfoHeader.ColorUsed; i++) { // заполняемеё
				RGBQUAD t = { i, i, i, 0 };
				Itemp.Pallete[i] = t;
			}

			for (int i = 0; i < Itemp.BMInfoHeader.Height; i++)  //Изменяемизображение
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

		if (Depth == 1 && Itemp.BMInfoHeader.BitCount == 32) { // 32 в 1 
			Itemp.BMInfoHeader.BitCount = 1; // меняемглубинуцвета
			Itemp.BMInfoHeader.ColorUsed = 2;
			// меняемразмеризображения:
			Itemp.BMInfoHeader.SizeImage = Itemp.BMInfoHeader.Height*BYTESPERLINE(Itemp.BMInfoHeader.Width, Itemp.BMInfoHeader.BitCount);

			Itemp.Pallete = new RGBQUAD[Itemp.BMInfoHeader.ColorUsed];// создёмсоответсвующуюпалитру
			RGBQUAD t = { 255, 255, 255, 0 };// заполняемеё
			Itemp.Pallete[1] = t;
			t = { 0, 0, 0, 0 };
			Itemp.Pallete[0] = t;

			for (int i = 0; i < Itemp.BMInfoHeader.Height; i++) //Изменяемизображение
				for (int j = 0; j < Itemp.BMInfoHeader.Width; j++) {
					RGBQUAD t = Itemp.Rgbquad[i][j];
					//Значение яркости:
					double dd = double(t.Red)*0.299 + double(t.Green)*0.597 + double(t.Blue)*0.114;
					int gray = floor(dd);
					if (gray > 126) gray = 255;
					else gray = 0;
					RGBQUAD t2 = { gray, gray, gray, 0 };
					Itemp.Rgbquad[i][j] = t2;
				}
			return Itemp;
		}

		if (Depth == 1 && Itemp.BMInfoHeader.BitCount == 8) { // 8 в 1
			Itemp.BMInfoHeader.BitCount = 1;// меняемглубинуцвета
			Itemp.BMInfoHeader.ColorUsed = (1 << 1);
			// меняемразмеризображения
			Itemp.BMInfoHeader.SizeImage = Itemp.BMInfoHeader.Height*BYTESPERLINE(Itemp.BMInfoHeader.Width, Itemp.BMInfoHeader.BitCount);

			delete[]Itemp.Pallete;
			Itemp.Pallete = new RGBQUAD[Itemp.BMInfoHeader.ColorUsed];// создёмсоответсвующуюпалитру
			Itemp.Pallete = new RGBQUAD[Itemp.BMInfoHeader.ColorUsed];
			RGBQUAD t = { 0, 0, 0, 0 };// заполняемеё
			Itemp.Pallete[0] = t;
			t = { 255, 255, 255, 0 };
			Itemp.Pallete[1] = t;

			for (int i = 0; i < Itemp.BMInfoHeader.Height; i++)  //Изменяемизображение
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

//Деструктор:
Image::~Image() {
	if (BMInfoHeader.BitCount <= 8 && Pallete != NULL) {//еслиглубинацвета< 8 бит
		delete[]Pallete;  //удаляемпалитру
		for (int i = 0; i < BMInfoHeader.Height; i++) //удаляемизображение
			delete[]Rgbquad[i];
	}
	if (BMInfoHeader.BitCount == 32 && Pallete != NULL) //еслиглубина = 32 бит
	{
		//delete[]Pallete; //удаляемпалитру
		for (int i = 0; i < BMInfoHeader.Height; i++)//удаляемизображение
			delete[]Rgbquad[i];
	}
}

//Метод загрузки изображения аналогичный конструктору:
int Image::loadimage(char *fileName) {
	FILE *file;
	errno_t e = fopen_s(&file, fileName, "rb");;    //открываемфайл
	BITMAPFILEHEADER BMFileHeader;
	fread(&BMFileHeader, sizeof(BITMAPFILEHEADER), 1, file);
	if (BMFileHeader.Type != 0x4D42) { // если тип не соответсвует нужному
		printf("Ошибка! Неизвестный формат файла.\n");
		return 0;
	}
	fread(&BMInfoHeader, sizeof(BITMAPINFOHEADER), 1, file); // если глубина цвета не соответсвует нужной
	if (BMInfoHeader.BitCount == 4) {
		printf("Ошибка! Неподдерживаемая битность изображения.\n");
		return 0;
	}

	if (BMInfoHeader.BitCount <= 8) {// если глубина цвета <= 8
		BMInfoHeader.ColorUsed = 1 << BMInfoHeader.BitCount;
		Pallete = new RGBQUAD[BMInfoHeader.ColorUsed]; // создём соответсвующую палитру
		if (fread(Pallete, sizeof(RGBQUAD)*BMInfoHeader.ColorUsed, 1, file) != 1) {
			delete[]Pallete;
			fclose(file);
			return 1;
		}
		BMInfoHeader.SizeImage = BMInfoHeader.Height*BYTESPERLINE(BMInfoHeader.Width, BMInfoHeader.BitCount); // считаемразмеризображения
		unsigned char *ucBuff = new unsigned char[BMInfoHeader.SizeImage];
		fread(ucBuff, BMInfoHeader.SizeImage, 1, file);

		Rgbquad = new RGBQUAD*[BMInfoHeader.Height]; // создаёмдвумерныймассив
		for (int i = 0; i < BMInfoHeader.Height; i++) {
			Rgbquad[i] = new RGBQUAD[BMInfoHeader.Width];
		}

		for (int i = 0; i < BMInfoHeader.Height; i++) {// заполняемегосоответсвующимиданными
			for (int j = 0; j < BMInfoHeader.Width; j++) {
				int iIndCol;

				if (BMInfoHeader.BitCount == 8) {//еслиглубинацвета = 8
					iIndCol = ucBuff[i*BYTESPERLINE(BMInfoHeader.Width, BMInfoHeader.BitCount) + j];
				}
				if (BMInfoHeader.BitCount == 1) {//еслиглубинацвета = 1
					unsigned char cNByte = ucBuff[i*BYTESPERLINE(BMInfoHeader.Width, BMInfoHeader.BitCount) + j / 8]; //одинбайтописывает 8 пикселей
					if ((j + 1) % 8 == 0) iIndCol = (j == (j / 8) * 8) ? (cNByte & 0x80) >> 1 : cNByte & 0x80; //если выполняется (j == 8 * (j / 8)) сдвигаем вправо на 1, иначе накладываем маску 
					else iIndCol = cNByte & 0x80;
					if (iIndCol > 255 / 2) iIndCol = 1;
					else iIndCol = 0;
				}
				Rgbquad[i][j] = Pallete[iIndCol];
			}
		}
	}

	if (BMInfoHeader.BitCount == 32) {// еслиглубинацвета = 32
		fseek(file, BMFileHeader.OffsetBits, SEEK_SET);
		size_t padding = 0;
		if ((BMInfoHeader.Width*BMInfoHeader.BitCount / 8) % 4)
			padding = 4 - (BMInfoHeader.Width*BMInfoHeader.BitCount / 8) % 4;

		Rgbquad = new RGBQUAD*[BMInfoHeader.Height]; // создаёмдвумерныймассив
		for (int i = 0; i < BMInfoHeader.Height; i++) {
			Rgbquad[i] = new RGBQUAD[BMInfoHeader.Width];
		}

		for (int i = 0; i < BMInfoHeader.Height; i++) {// заполняемегосоответсвующимиданными
			fread(Rgbquad[i], sizeof(RGBQUAD), BMInfoHeader.Width, file);
			if (padding != 0)
				fread(&Rgbquad, padding, 1, file);
		}
	}

	if (BMInfoHeader.BitCount == 24) {// еслиглубинацвета = 24
		BMInfoHeader.BitCount = 32; // приводимеёк 32
		size_t padding = 0;
		if ((BMInfoHeader.Width*BMInfoHeader.BitCount / 8) % 4)
			padding = 4 - (BMInfoHeader.Width*BMInfoHeader.BitCount / 8) % 4;

		Rgbquad = new RGBQUAD*[BMInfoHeader.Height]; // создаёмдвумерныймассив
		for (int i = 0; i < BMInfoHeader.Height; i++) {
			Rgbquad[i] = new RGBQUAD[BMInfoHeader.Width];
		}

		for (int i = 0; i < BMInfoHeader.Height; i++) {// заполняемегосоответсвующимиданными
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
	fclose(file); // закрываемфайл
	return 1;
}

//Метод записи изображения в файл:
void Image::writeimage(char *fileName) {

	if (!Rgbquad) { // если нет данных
		printf("Ошибка! В изображении нет данных для записи.\n");
		return;
	}

	FILE *file;
	errno_t e = fopen_s(&file, fileName, "wb");//открываемфайл

	BITMAPFILEHEADER BMFileHeader; // записываемзаголовок
	BMFileHeader.Type = 0x4D42;
	BMFileHeader.Reserved1 = 0;
	BMFileHeader.Reserved2 = 0;
	BMFileHeader.OffsetBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	BMFileHeader.Size = BMInfoHeader.SizeImage + BMFileHeader.OffsetBits;
	fwrite(&BMFileHeader, sizeof(BITMAPFILEHEADER), 1, file);
	fwrite(&BMInfoHeader, sizeof(BITMAPINFOHEADER), 1, file);

	if (BMInfoHeader.BitCount <= 8) {// еслиглубинацвета<= 8
		fwrite(Pallete, BMInfoHeader.ColorUsed * sizeof(RGBQUAD), 1, file);
		unsigned char *ucBuff = new unsigned char[BMInfoHeader.SizeImage]; //считаемразмерданных

		for (int i = 0; i < BMInfoHeader.Height; i++) {// записываемданные
			for (int j = 0; j < BMInfoHeader.Width; j++) {
				int iIndCol = LocateInPallete(Rgbquad[i][j]);
				if (BMInfoHeader.BitCount == 8) {// еслиглубинацвета = 8 
					ucBuff[i*BYTESPERLINE(BMInfoHeader.Width, BMInfoHeader.BitCount) + j] = iIndCol; // записываемданные
				}
				if (BMInfoHeader.BitCount == 1) {// еслиглубинацвета = 1
					int iNumb = i * BYTESPERLINE(BMInfoHeader.Width, BMInfoHeader.BitCount) + (j / 8); // меняемцветначёрныйилибелыйизаписываем
					if ((j + 1) % 8 != 0) ucBuff[iNumb] = (8 * (j / 8) == j) ? (iIndCol) << 1 : ((ucBuff[iNumb] + iIndCol) << 1);
					else ucBuff[iNumb] = ucBuff[iNumb] + iIndCol;
				}
			}
			if (BMInfoHeader.BitCount == 8) {// еслиглубинацвета = 8
				for (int j = BMInfoHeader.Width; j < BYTESPERLINE(BMInfoHeader.Width, BMInfoHeader.BitCount); j++) { // записываемданные
					ucBuff[i*BYTESPERLINE(BMInfoHeader.Width, BMInfoHeader.BitCount) + j] = 0;
				}
			}
			if (BMInfoHeader.BitCount == 1) {// еслиглубинацвета = 1
				{
					for (int j = (BMInfoHeader.Width / 8) + 1; j < BYTESPERLINE(BMInfoHeader.Width, BMInfoHeader.BitCount); j++) { // меняем цвет на чёрный или белый и записываем
						ucBuff[i*BYTESPERLINE(BMInfoHeader.Width, BMInfoHeader.BitCount) + j] = 0;
					}
				}
			}
		}
		fwrite(ucBuff, BMInfoHeader.SizeImage, 1, file);
		delete[]ucBuff;
	}

	if (BMInfoHeader.BitCount == 32) {// еслиглубинацвета = 32
		size_t padding = 0;
		if ((BMInfoHeader.Width*BMInfoHeader.BitCount / 8) % 4)
			padding = 4 - (BMInfoHeader.Width*BMInfoHeader.BitCount / 8) % 4;
		for (int i = 0; i < BMInfoHeader.Height; i++) {// записываемданные
			fwrite(Rgbquad[i], sizeof(RGBQUAD), BMInfoHeader.Width, file);
			if (padding != 0)
				fwrite(&Rgbquad, padding, 1, file);
		}
	}
	fclose(file);
}

int main() {
	setlocale(LC_ALL, "rus");

	Image img1((char*)"home.bmp"); //Cоздание объекта класса Image, содержащего данные из файла input1.bmp

	Image img2(0, 32, 315, 177); //Cоздание объекта 8 битного изображения размером 315*177
	img2 /= img1; //Приведение img1 к масштабу img2
	img2.writeimage((char*)"home_resized.bmp");

	Image img3(0, 8, img1.Widht(), img1.Height()); //Создание пустого изображения
	img3 /= img1/8; //Запись img1 в img3 с изменением глубины цвета на 8 бит
	img3.writeimage((char*)"home_8.bmp");

	Image img4(0, 1, img1.Widht(), img1.Height()); //Создание пустого изображения
	img4 = (img1 / 1); //Запись img1 в img4 с изменением глубины цвета на 1 бит
	img4.writeimage((char*)"home_1.bmp");

	system("pause");
	return 0;
}