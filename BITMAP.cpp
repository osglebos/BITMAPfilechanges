#define _CRT_SECURE_NO_WARNINGS
#include <locale>
#include <stdio.h>

using namespace std;
#pragma pack(push,1)

struct BITMAPFILEHEADER
{
	unsigned short Type; // ‘BM’ 0x4D42
	unsigned long Size; // Размер файла в байтах, BitCount*Height*Width+ OffsetBits
	unsigned short Reserved1; // Зарезервирован; должен быть нуль
	unsigned short Reserved2; // Зарезервирован; должен быть нуль
	unsigned long OffsetBits; // Смещение данных от начала файла в байтах

	// = sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER)
};

#pragma pack(pop)

#pragma pack(push,1)

struct BITMAPINFOHEADER
{
	unsigned long Size; // Число байтов необходимое для структуры = 40
	unsigned long Width; // Ширина точечного рисунка в пикселях
	unsigned long Height; // Высота точечного рисунка в пикселях
	unsigned short Planes; // Число плоскостей целевого устройства = 1
	unsigned short BitCount; // Глубина цвета, число бит на точку = 0,1,4,8,16,24,32
	unsigned long Compression; // Тип сжатия = 0 для несжатого изображения
	unsigned long SizeImage; // Размер изображения в байтах BitCount*Height*Width
	unsigned long XPelsPerMeter; // Разрешающая способность по горизонтали
	unsigned long YPelsPerMeter; // Разрешающая способность по вертикали
	unsigned long ColorUsed; // Число индексов используемых цветов. Если все цвета = 0
	unsigned long ColorImportant; // Число необходимых цветов = 0
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
	BITMAPINFOHEADER BMInfoHeader; // Информационный заголовок изображения
	RGBTRIPLE* Rgbtriple; // Двумерный массив с описанием пикселей типа RGBTRIPLE
	RGBQUAD* Palette; // Палитра изображения(присутствует только, если глубина цвета равна 1, 4 или 8)
	void setEmptyImageParams(); // Выставление внутренних параметров, соответствующих пустому изображению
	int loadImageDataFromFile(FILE* f); // загружает изображение из переданного файла
	void saveImageDataToFile(FILE* f); // сохраняет изображение в файл
	void setHeaderAndAllocateMemory(int Width, int Height, int BitCount); // устанавливает параметры заголовка изображения (BMInfoHeader)
	// выделяет память под изображение
	// если требуется форматом, создает и заполняет палитру
	void initializeFromImage(const Image& Inp); // делает копию изображения (заполняет заголовок и копирует данные)
	void copyDataFromImage(const Image& Inp); // копирует данные при совпадении заголовков изображений
	void copyAndConvertDataFromImage(const Image& Inp); // копирует и преобразует данные переданного изображения в текущий формат

	bool isAllowedBitCount(unsigned short BitCount) // проверяет допустимость битности изображения
	{
		return BitCount == 24 || BitCount == 8 || BitCount == 4 || BitCount == 1; // 3-ий вариант
	}

	int getAdditionalRowOffsetInFile(int Width, unsigned short BitCount) // считает сколько нужно дописывать в файл байт после записи строки изображения, чтобы получить размет строки кратным 4
	{
		int remainder = getImageRowSize(Width, BitCount) % 4; // вычисляем остаток от деления длины строки на 
		return remainder ? 4 - remainder : 0; // возвращаем дополнение до 4
	}

	int getImageRowSize(int Width, unsigned short BitCount) // возвращает сколько байт требуется для записи строки изображения в файл
	{
		// Width * BitCount дает необходимое число бит для хранения строки изображения
		// ((Width * BitCount + 7) / 8) вычисляет количество байт и округляет в большую сторону
		return (Width * BitCount + 7) / 8;
	}

	int getTotalImageSize(int Width, int Height, unsigned short BitCount) // возвращает полный размер данных при записи в файл
	{
		// вычисляем сколько байт требуется для записи строки изображения в файл, добавляем нужное количество байт для кратности 4 и умножаем на количество строк
		return (getImageRowSize(BMInfoHeader.Width, BMInfoHeader.BitCount) + getAdditionalRowOffsetInFile(Width, BitCount)) * Height;
	}

	unsigned char getGrayscaleColor(unsigned char Red, unsigned char Green, unsigned char Blue) // вычисляет градацию серого для переданного цвета
	{
		int result = (int)(Red * 0.299 + Green * 0.597 + Blue * 0.114); // формула из методички
		if (result > 255) result = 255;// проверяем выход за границы байта
		return (unsigned char)result;
	}

	unsigned char getGrayscaleColor(RGBTRIPLE color) // вычисляет градацию серого для переданного цвета
	{
		return getGrayscaleColor(color.Red, color.Green, color.Blue);
	}

	unsigned char getGrayscaleColor(RGBQUAD color) // вычисляет градацию серого для переданного цвета
	{
		return getGrayscaleColor(color.Red, color.Green, color.Blue);
	}

	unsigned char getNearestPaletteColorIndex(unsigned char grayscaleColor) // бинарным поиском ищет ближайший цвет в палитре (так как она упорядочена по возрастанию оттенков серого)
	{
		int minIndex = 0; // установка минимального индекса для поиска
		int maxIndex = BMInfoHeader.ColorUsed - 1; // установка максимального индекса для поиска
		while (maxIndex >= minIndex) // продолжаем искать пока минимальный индекс не перейдет за максимальный
		// (в этом случае значение grayscaleColor находится между Palette[minIndex - 1] и Palette[minIndex], так как сначала всегда
		// проверяем можно ли изменить minIndex), либо пока не найдем искомое значение
		{
			int middleIndex = (minIndex + maxIndex) / 2; // вычисление среднего индекса между крайними
			if (Palette[middleIndex].Red == grayscaleColor) // проверяем не нашли ли искомое значение (так как палитка состоит из оттенков серого
			// Palette[middleIndex].Red == Palette[middleIndex].Green == Palette[middleIndex].Blue)
			{
				return middleIndex; // возвращаем индекс палитры в случае успеха
			}
			else if (Palette[middleIndex].Red < grayscaleColor) // если текущее значение палитры оказалось меньше требуемого,
			// сдвигаем левый край поиска
			// Замечание: если значения grayscaleColor нет в палитре,
			// то на предпоследнем заходе в while будет ситуация
			// когда minIndex == middleIndex == maxIndex - 1 и
			// Palette[middleIndex].Red < grayscaleColor < Palette[middleIndex + 1].Red
			// Тогда на этом заходе произойдет выполнение данного условия и minIndex станет равно maxIndex
			// После чего состоится последний заход в whilе и maxIndex станет равен minIndex - 1
			// Таким образом, значение grayscaleColor будет находится между Palette[minIndex - 1] и Palette[minIndex]
				minIndex = middleIndex + 1;
			else maxIndex = middleIndex - 1; // если текущее значение палитры оказалось больше требуемого, сдвигаем правый край поиска
		}

		if (minIndex == BMInfoHeader.ColorUsed) // если minIndex стало равным общему количеству элементов, то значит переданное значение больше максимального значения в палитре
			return (unsigned char)BMInfoHeader.ColorUsed - 1; // поэтому возвращаем индекс наибольшего значения палитры

		if (minIndex == 0) // если minIndex не изменился, то все время сдвигался правый конец области поиск значит значение grayscaleColor меньше минимального в палитре
			return 0; // поэтому возвращаем индекс минимального значения палитры

		int prevDelta = grayscaleColor - Palette[minIndex - 1].Red; // в оставшемся случае, как было указано ранее,
		// значение grayscaleColor находится между Palette[minIndex - 1] и Palette[minIndex]
		// потому считаем от какого значения оно отстоит меньше
		int nextDelta = Palette[minIndex].Red - grayscaleColor;
		return prevDelta < nextDelta ? minIndex - 1 : minIndex; // и возвращаем этот индекс
	}

public:
	Image(char Mode, unsigned short BCount, int Width, int Height); // Конструктор создания изображения
	Image(char* fileName); // Конструктор объекта изображения из файла
	Image(); // Конструктор без параметров, создает пустой контейнер под изображение
	Image(const Image& i); // Конструктор копии
	~Image(); // Деструктор
	int loadimage(char* fileName); // метод загрузки изображения аналогичный конструктору, возвращает 0 в случае ошибки
	void writeimage(char* fileName); // метод записи изображения в файл
	Image& operator = (const Image& Inp); // Перегрузка оператора =
	Image operator / (short Depth); // Перегрузка оператора /
	Image& operator /= (const Image& Inp); // Перегрузка оператора /=
};

Image::Image()// Реализация конструктора без параметров
{
	setEmptyImageParams(); // Заполняем параметры пустого изображения
}

Image::Image(char* fileName)// Реализация конструктора при загрузке из файла(с указанием файла)
{
	setEmptyImageParams(); // Заполняем параметры пустого изображения
	loadimage(fileName); // выполняем загрузку из файла с переданным именем
}

Image::Image(const Image& im)// Реализация конструктора с копированием изображения
{
	initializeFromImage(im); // делаем копию изображения
}

Image::~Image()// Реализация деструктора
{
	if (Rgbtriple) // Если есть указатель на данные, то нужно очистить выделенную под изображение память
	{
		delete[] Rgbtriple; // Удаляем память, содержащую массив указателей на строки
		Rgbtriple = NULL; // Инициализируем указатель на данные невалидным значением
		// (не обязательно, так как пороисходит в деструкторе)
	}
	if (Palette) // Удаление палитры, если она была создана
	{
		delete[] Palette;
		Palette = NULL;
	}
}

void Image::setHeaderAndAllocateMemory(int Width, int Height, int BitCount)
{
	// Заполняем необходимые поля информации об изображении
	BMInfoHeader.Width = Width; // Ширина точечного рисунка в пикселях
	BMInfoHeader.Height = Height; // Высота точечного рисунка в пикселях
	BMInfoHeader.Planes = 1; // Число плоскостей целевого устройства = 1
	BMInfoHeader.BitCount = BitCount; // Глубина цвета, число бит на точку = 1,4,8,24 (вариант 3)
	BMInfoHeader.SizeImage = getTotalImageSize(Width, Height, BitCount); // Размер изображения в байтах
	if (BMInfoHeader.BitCount <= 8)
	{
		BMInfoHeader.ColorUsed = 1 << BMInfoHeader.BitCount; // = 2 в степени BMInfoHeader.BitCount
		Palette = new RGBQUAD[BMInfoHeader.ColorUsed]; // Выделение памяти под палитру цветов
		for (int i = 0; i < (int)BMInfoHeader.ColorUsed; i++) // заполняем значения палитры градациями серого от 0 до 255
		{
			unsigned char color = (unsigned char)(255 * i / (BMInfoHeader.ColorUsed - 1));
			Palette[i].Red = color;
			Palette[i].Green = color;
			Palette[i].Blue = color;
			Palette[i].Reserved = 0;
		}
	}

	// Выделение памяти для двумерного массива размером Height*Width типа RGBTRIPLE
	if (BMInfoHeader.SizeImage > 0)	Rgbtriple = new RGBTRIPLE[BMInfoHeader.Height * BMInfoHeader.Width];
	else Rgbtriple = NULL;
}

Image::Image(char Mode, unsigned short BCount, int Width, int Height)// Реализация конструктора создания изображения(с указанием параметров изображения)
{
	setEmptyImageParams();// Выставление начальных параметров, соответствующих пустому изображению

	// Проверяем какую желаемую битность нам передали в конструктор
	// Поддерживается хранение данных только в RGBTRIPLE(вариант 3)
	if (isAllowedBitCount(BCount))
	{
		setHeaderAndAllocateMemory(Width, Height, BCount); // заполняем заголовок изображения и выделяем память
		if (Palette) Mode = Palette[getNearestPaletteColorIndex(Mode)].Red; // получаем ближайший к переданному цвет в палитре, если она используется

		// Заполнение данных изображения переданным значением
		for (int i = 0; i < (int)BMInfoHeader.Height; i++)
			for (int j = 0; j < (int)BMInfoHeader.Width; j++)
			{
				Rgbtriple[i * BMInfoHeader.Width + j].Red = Mode;
				Rgbtriple[i * BMInfoHeader.Width + j].Green = Mode;
				Rgbtriple[i * BMInfoHeader.Width + j].Blue = Mode;
			}
	}// Задали неподдерживаемый параметр битности	
	else printf("Ошибка: поддерживается создание изображения только 24 бит.\n");// Выводим сообщение об ошибке
}

// Выставление параметров, соответствующих пустому изображению
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

	// считываем заголовок изображения
	if (fread(&BMFileHeader, sizeof(BITMAPFILEHEADER), 1, f) != 1)
	{
		printf("Ошибка: не удалось считать заголовок файла изображения\n");
		return 0;
	}

	// проверяем сигнатуру
	if (BMFileHeader.Type != 0x4D42)
	{
		printf("Ошибка: неизвестный формат файла\n");
		return 0;
	}

	// загружаем заголовок изображения
	if (fread(&FileBMInfoHeader, sizeof(BITMAPINFOHEADER), 1, f) != 1)
	{
		printf("Ошибка: не удалось считать информацию об изображении\n");
		return 0;
	}

	// проверяем, можем ли загружать данный формат
	if (!isAllowedBitCount(FileBMInfoHeader.BitCount) && FileBMInfoHeader.BitCount != 32)
	{
		printf("Ошибка: неподдерживаемая битность изображения: '%i'\n", (int)FileBMInfoHeader.BitCount);
		return 0;
	}

	setHeaderAndAllocateMemory(FileBMInfoHeader.Width, FileBMInfoHeader.Height, FileBMInfoHeader.BitCount);// заполняем заголовок и выделяем память для картинки, описанной в файле

	// Создаем и загружаем файловую палитру, если она присутствует
	if (BMInfoHeader.BitCount <= 8)
	{
		filePalette = new RGBQUAD[BMInfoHeader.ColorUsed];
		fread(filePalette, sizeof(RGBQUAD), BMInfoHeader.ColorUsed, f);
	}

	fseek(f, BMFileHeader.OffsetBits, SEEK_SET);// переходим к началу данных изображения
	const int additionalRowOffset = getAdditionalRowOffsetInFile(FileBMInfoHeader.Width, FileBMInfoHeader.BitCount);// вычисляем смещение между строками

	if (FileBMInfoHeader.BitCount == 24)
	{
		for (int i = BMInfoHeader.Height - 1; i >= 0; i--)
		{
			fread(Rgbtriple + i * FileBMInfoHeader.Width, sizeof(RGBTRIPLE), FileBMInfoHeader.Width, f);// считываем сразу всю строку изображенмя, так как она совпадает с внутренним форматом хранения в памяти
			if (additionalRowOffset) fseek(f, additionalRowOffset, SEEK_CUR);// пропускаем смещение между строками
		}
	}
	else if (FileBMInfoHeader.BitCount == 32)
	{
		// считываем каждый пиксель отдельно
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
	else // загрузка изображений с палитрой
	{
		const unsigned char topBitsOffset = 8 - BMInfoHeader.BitCount; // показывает на сколько нужно переметить биты из старшей части байта, чтобы получить цвет
		unsigned char colorMask = (1 << BMInfoHeader.BitCount) - 1; // выставляем младшие BMInfoHeader.BitCount бит в 1 для получения маски цвета
		colorMask <<= 8 - BMInfoHeader.BitCount; // перемещаем эти единицы в старшие биты байта, так как данные располагаются от старших битов к младшим

		for (int i = BMInfoHeader.Height - 1; i >= 0; i--)
		{
			int leftBits = 0; // указывает сколько непрочитанных бит осталось в байте
			unsigned char paletteByte = 0; // текущий считанный байт в изображении
			for (int j = 0; j < (int)BMInfoHeader.Width; j++)
			{
				if (!leftBits) // если в текущем байте рассмотрели все биты, то считываем следующий
				{
					fread(&paletteByte, 1, 1, f);
					leftBits = 8;
				}

				int paletteIndex = (paletteByte & colorMask) >> topBitsOffset; // получаем текущий индекс в палитре файла применением маски и сдвигом полученного значения
				leftBits -= BMInfoHeader.BitCount; // обновляем количество необработанных бит
				paletteByte <<= BMInfoHeader.BitCount; // перемещаем их в старшую часть байта

				unsigned char sourceGrayscale = getGrayscaleColor(filePalette[paletteIndex]); // вычисляем оттенок серого для цвета пикселя из файла
				unsigned char grayscale = Palette[getNearestPaletteColorIndex(sourceGrayscale)].Red; // получаем ближайший к нему цвет из палиты

				// заполняем данные изображения
				Rgbtriple[i * BMInfoHeader.Width + j].Red = grayscale;
				Rgbtriple[i * BMInfoHeader.Width + j].Green = grayscale;
				Rgbtriple[i * BMInfoHeader.Width + j].Blue = grayscale;
			}

			if (additionalRowOffset)fseek(f, additionalRowOffset, SEEK_CUR);// пропускаем смещение между строками изображения в файле
		}
	}

	if (filePalette)delete[] filePalette;// удаляем файловую палитру, если она была создана
	return 1;
}

// Реализация метода загрузки изображения
int Image::loadimage(char* fileName)
{
	if (Rgbtriple)
	{
		printf("Ошибка: нельзя загружать данные в уже созданное изображение\n");
		return 0;
	}

	FILE* f = fopen(fileName, "rb"); // необходимо открывать бинарный файл
	if (!f)
	{
		printf("Ошибка: не удалось прочитать файл %s\n", fileName);
		return 0;
	}

	int resultCode = loadImageDataFromFile(f);// производим загрузку данных
	fclose(f);// закрываем файл
	return resultCode;
}

void Image::saveImageDataToFile(FILE* f) // сохраняет данные в переданный файл
{
	const int additionalRowOffset = getAdditionalRowOffsetInFile(BMInfoHeader.Width, BMInfoHeader.BitCount);// получаем смещение между строками изображения в файле

	// Заполняем заголовок файла BMP
	BITMAPFILEHEADER BMFileHeader;
	BMFileHeader.Type = 0x4D42; // сигнатура
	BMFileHeader.OffsetBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER); // смещение до данных изображения

	// полный размер файла = сумма размера всех заголовков, палитры и данных файла
	BMFileHeader.Size = BMFileHeader.OffsetBits + getTotalImageSize(BMInfoHeader.Width, BMInfoHeader.Height, BMInfoHeader.BitCount);
	BMFileHeader.Reserved1 = 0; // не используется
	BMFileHeader.Reserved2 = 0; // не используется

	if (Palette) // добавляем данные о палитре
	{
		const int paletteSize = sizeof(RGBQUAD) * BMInfoHeader.ColorUsed; // вычисляем размер палитры
		BMFileHeader.Size += paletteSize; // добавляем его к общему размеру файла
		BMFileHeader.OffsetBits += paletteSize; // и к смещению до данных изображения
	}

	// записываем заголовок файла
	if (fwrite(&BMFileHeader, sizeof(BITMAPFILEHEADER), 1, f) != 1)
	{
		printf("Ошибка: не удалось записать заголовок файла изображения\n");
		return;
	}

	// записываем заголовок изображения
	if (fwrite(&BMInfoHeader, sizeof(BITMAPINFOHEADER), 1, f) != 1)
	{
		printf("Ошибка: не удалось записать информацию об изображении\n");
		return;
	}

	if (BMInfoHeader.BitCount == 24)
	{
		for (int i = BMInfoHeader.Height - 1; i >= 0; i--)
		{
			// для формата 24 бит просто копируем в файл строку изображения, так как он совпадает с внутренним форматом хранения
			fwrite(Rgbtriple + i * BMInfoHeader.Width, sizeof(RGBTRIPLE), BMInfoHeader.Width, f);

			// записываем нужное количество 0 для смещения между строками изображения
			if (additionalRowOffset)
			{
				const int Zero = 0;
				fwrite(&Zero, 1, additionalRowOffset, f);
			}
		}
	}
	else // изображение с палитрой
	{
		// сначала записываем палитру
		if (fwrite(Palette, sizeof(RGBQUAD), BMInfoHeader.ColorUsed, f) != BMInfoHeader.ColorUsed)
		{
			printf("Ошибка: не удалось записать палитру\n");
			return;
		}

		// вычисляем смещение необходимое для перемещения индекса цвета первого пикселя в старшие биты
		const int startPaletteDataOffset = 8 - BMInfoHeader.BitCount;

		// сохранение изображений с палитрой
		for (int i = BMInfoHeader.Height - 1; i >= 0; i--)
		{
			int currentPaletteDataOffset = startPaletteDataOffset; // выставляем на сколько нужно сместить следующее значение при записи в байт
			int leftBits = 8; // храним сколько бит еще можно использовать в байте
			int paletteByte = 0; // хранит текущее значение для записи в файл
			for (int j = 0; j < (int)BMInfoHeader.Width; j++)
			{
				unsigned char paletteColorIndex = getNearestPaletteColorIndex(Rgbtriple[i * BMInfoHeader.Width + j].Red); // получаем индекс палитры для текущего цвета
				paletteByte |= paletteColorIndex << currentPaletteDataOffset; // смещаем его на нужное количество бит и записываем в результат
				currentPaletteDataOffset -= BMInfoHeader.BitCount; // уменьшаем требуемый сдвиг для следующего индека
				leftBits -= BMInfoHeader.BitCount; // уменьшаем количество оставшихся бит

				if (!leftBits)
				{
					fwrite(&paletteByte, 1, 1, f);// если кончились свободные биты, то записываем собранный байт в файл

					// и выставляем параметры на начальные
					currentPaletteDataOffset = startPaletteDataOffset;
					leftBits = 8;
					paletteByte = 0;
				}
			}

			// если после завершения обработки строки остались незаписанные биты (байт полностью не заполнился), то записываем их в файл
			if (leftBits != 8) fwrite(&paletteByte, 1, 1, f);

			// записываем нужное количество 0 для смещения между строками изображения
			if (additionalRowOffset)
			{
				const int Zero = 0;
				fwrite(&Zero, 1, additionalRowOffset, f);
			}
		}
	}
}

void Image::initializeFromImage(const Image& Inp) // делает копию изображения (заполняет заголовок и копирует данные)
{
	setEmptyImageParams(); // Заполняем параметры пустого изображения
	setHeaderAndAllocateMemory(Inp.BMInfoHeader.Width, Inp.BMInfoHeader.Height, Inp.BMInfoHeader.BitCount); // заполняем заголовок и выделяем нужную память
	copyDataFromImage(Inp); // копируем данные изображения
}

void Image::copyDataFromImage(const Image& Inp)
{
	// Проверяем совпадение разрешения и битности, после чего скопируем данные
	if (BMInfoHeader.Width == Inp.BMInfoHeader.Width &&
		BMInfoHeader.Height == Inp.BMInfoHeader.Height &&
		BMInfoHeader.BitCount == Inp.BMInfoHeader.BitCount) memcpy(Rgbtriple, Inp.Rgbtriple, BMInfoHeader.Height * BMInfoHeader.Width * sizeof(RGBTRIPLE));
	else printf("Ошибка: в изображение уже проинициализированно другим разрешением и/или битностью\n");
}

void Image::writeimage(char* fileName) // выполняет запись в файл
{
	if (!Rgbtriple)
	{
		printf("Ошибка: в изображении нет данных для сохранения\n");
		return;
	}

	FILE* f = fopen(fileName, "wb"); //открываем бинарный файл на запись	

	if (!f)
	{
		printf("Ошибка: не удалось создать файл %s\n", fileName);
		return;
	}

	saveImageDataToFile(f); // сохраняет данные в файл
	fclose(f); // закрывает файл
}

Image& Image::operator = (const Image& Inp) // Перегрузка оператора =
{
	if (Rgbtriple) copyDataFromImage(Inp); //если изображение уже создано то пробуем только скопировать данные (при совпадении форматов)
	else initializeFromImage(Inp); // иначе создаем копию

	return *this;
}

void Image::copyAndConvertDataFromImage(const Image& Inp) // функция преобразования данных переданного изображения в текущий формат
{
	// Проверяем совпадение разрешения и битности, после чего скопируем данные
	if (BMInfoHeader.Width != Inp.BMInfoHeader.Width &&
		BMInfoHeader.Height != Inp.BMInfoHeader.Height)
	{
		printf("Ошибка: не совпадают разрешения при конвертировании битности изображений\n");
		return;
	}

	// проверяем, что произоводится уменьшение битности
	if (Inp.BMInfoHeader.BitCount < BMInfoHeader.BitCount)
	{
		printf("Ошибка: возможно только уменьшение битности изображений\n");
		return;
	}

	const bool isSourceWithPalette = Inp.Palette != NULL; // используется ли в исходном изображении палитра
	for (int i = 0; i < (int)BMInfoHeader.Height; i++)
		for (int j = 0; j < (int)BMInfoHeader.Width; j++)
		{
			unsigned char grayscale = isSourceWithPalette ? Inp.Rgbtriple[i * BMInfoHeader.Width + j].Red // для исходного изображения с палитрой берем оттенок серого сразу из данных картинки
				: getGrayscaleColor(Inp.Rgbtriple[i * BMInfoHeader.Width + j]); // иначе вычисляем его

			grayscale = Palette[getNearestPaletteColorIndex(grayscale)].Red; // получаем значение серого в палитре итогового изображения

			// записываем оттенок серого в пиксель итогового изображения
			Rgbtriple[i * BMInfoHeader.Width + j].Red = grayscale;
			Rgbtriple[i * BMInfoHeader.Width + j].Green = grayscale;
			Rgbtriple[i * BMInfoHeader.Width + j].Blue = grayscale;
		}
}

Image Image::operator / (short Depth) // Перегрузка оператора /, возвращает новое изображение, являющееся копией данного, но с битностью Depth
{
	// проверяем, что параметр допустим
	if (!isAllowedBitCount(Depth))
	{
		printf("Ошибка: неподдерживаемая битность\n");
		return Image(*this);
	}

	if (Depth > BMInfoHeader.BitCount)
	{
		printf("Ошибка: возможно только уменьшение битности\n");
		return Image(*this);
	}

	Image result(0, Depth, BMInfoHeader.Width, BMInfoHeader.Height); // создаем пустое изображение
	result.copyAndConvertDataFromImage(*this); // выполняем преобразование текущего изображения в итоговое
	return result;
}

Image& Image::operator /= (const Image& Inp) // Перегрузка оператора /=, выполняет запись переданного изображения в текущее с изменением размера
{
	if (BMInfoHeader.BitCount != Inp.BMInfoHeader.BitCount)	// проверка совпадения форматов
	{
		printf("Ошибка: разная битность изображений, получение изображения с новым размером невозможно\n");
		return *this;
	}

	float xRatio = (float)Inp.BMInfoHeader.Width / BMInfoHeader.Width; // вычисление соотношения ширины изображений
	float yRatio = (float)Inp.BMInfoHeader.Height / BMInfoHeader.Height; // вычисление соотношения высоты изображений

	for (int i = 0; i < (int)BMInfoHeader.Height; i++)
		for (int j = 0; j < (int)BMInfoHeader.Width; j++)
		{
			int sourceX = (int)(j * xRatio); // вычисление x координаты в исходном изображении
			int sourceY = (int)(i * yRatio); // вычисление y координаты в исходном изображении
			Rgbtriple[i * BMInfoHeader.Width + j] = Inp.Rgbtriple[sourceY * Inp.BMInfoHeader.Width + sourceX]; // запись значения цвета из исходного изображения в текущее
		}

	return *this;
}

int main(void)
{
	setlocale(LC_ALL, "Russian");
	Image* im1 = new Image((char*)"volleyball.bmp"); // Создание объекта изображения из файла 
	Image* im2 = new Image(0, 24, 1000, 500);// Создание изображения с заданными параметрами 
	Image im3, im4, im5;// Создание пустого изображения 

	(*im2) /= *im1; // Приведение img к масштабу img2 
	im3 = *im1 / 8; // делаем битность 8
	im4 = *im1 / 4; // делаем битность 4
	im5 = *im1 / 1; // делаем битность 1
	im3.writeimage((char*)"Mercedes_8.bmp");
	im4.writeimage((char*)"Mercedes_4.bmp");
	im5.writeimage((char*)"Mercedes_1.bmp");
	im2->writeimage((char*)"Mercedes_res.bmp");

	printf("Mercedes.bmp have been converted.\n");
	scanf(" ");
	return 0;
}
