#define _CRT_SECURE_NO_WARNINGS
#include <locale>
#include <stdio.h>
typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef unsigned char BYTE;

#pragma pack(push,1)
struct BITMAPFILEHEADER
{
	WORD Type; // ‘BM’ 0x4D42
	DWORD Size; // Размер файла в байтах, BitCount*Height*Width+ OffsetBits
	WORD Reserved1; // Зарезервирован; должен быть нуль
	WORD Reserved2; // Зарезервирован; должен быть нуль
	DWORD OffsetBits; // Смещение данных от начала файла в байтах
   // = sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER)
};
#pragma pack(pop)

#pragma pack(push,1)
struct BITMAPINFOHEADER
{
	DWORD Size; // Число байтов необходимое для структуры = 40
	DWORD Width; // Ширина точечного рисунка в пикселях
	DWORD Height; // Высота точечного рисунка в пикселях
	WORD Planes; // Число плоскостей целевого устройства = 1
	WORD BitCount; // Глубина цвета, число бит на точку = 0,1,4,8,16,24,32
	DWORD Compression; // Тип сжатия = 0 для несжатого изображения
	DWORD SizeImage; // Размер изображения в байтах BitCount*Height*Width
	DWORD XPelsPerMeter; // Разрешающая способность по горизонтали
	DWORD YPelsPerMeter; // Разрешающая способность по вертикали
	DWORD ColorUsed; // Число индексов используемых цветов. Если все цвета = 0
	DWORD ColorImportant; // Число необходимых цветов = 0
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

Image& Image::operator = (const Image& Inp) // Перегрузка оператора =
{
	if (Rgbquad) copyDataFromImage(Inp); //если изображение уже создано то пробуем только скопировать данные (при совпадении форматов)
	else initializeFromImage(Inp); // иначе создаем копию

	return *this;
}

void Image::copyAndConvertDataFromImage(const Image& Inp) // функция преобразования данных переданного изображения в текущий формат
{
	// Проверяем совпадение разрешения и битности, после чего скопируем данные
	if (BMInfoHeader.Width != Inp.BMInfoHeader.Width && BMInfoHeader.Height != Inp.BMInfoHeader.Height)
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
			BYTE grayscale = isSourceWithPalette ? Inp.Rgbquad[i * BMInfoHeader.Width + j].Red // для исходного изображения с палитрой берем оттенок серого сразу из данных картинки
				: getGrayscaleColor(Inp.Rgbquad[i * BMInfoHeader.Width + j].Red, Inp.Rgbquad[i * BMInfoHeader.Width + j].Green, Inp.Rgbquad[i * BMInfoHeader.Width + j].Blue); // иначе вычисляем его

			grayscale = Palette[getNearestPaletteColorIndex(grayscale)].Red; // получаем значение серого в палитре итогового изображения

			// записываем оттенок серого в пиксель итогового изображения
			Rgbquad[i * BMInfoHeader.Width + j].Red = grayscale;
			Rgbquad[i * BMInfoHeader.Width + j].Green = grayscale;
			Rgbquad[i * BMInfoHeader.Width + j].Blue = grayscale;
			Rgbquad[i * BMInfoHeader.Width + j].Reserved = 0;
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
			Rgbquad[i * BMInfoHeader.Width + j] = Inp.Rgbquad[sourceY * Inp.BMInfoHeader.Width + sourceX]; // запись значения цвета из исходного изображения в текущее
		}

	return *this;
}

void Image::copyDataFromImage(const Image& Inp) // копирует данные при совпадении заголовков изображений
{
	// Проверяем совпадение разрешения и битности, после чего скопируем данные
	if (BMInfoHeader.Width == Inp.BMInfoHeader.Width &&	BMInfoHeader.Height == Inp.BMInfoHeader.Height && BMInfoHeader.BitCount == Inp.BMInfoHeader.BitCount)
	{
		Rgbquad = new RGBQUAD[Inp.BMInfoHeader.Height*Inp.BMInfoHeader.Width];
		for (int i = 0; i < (int)Inp.BMInfoHeader.Height; i++)
			for (int j = 0; j < (int)Inp.BMInfoHeader.Width; j++)
				Rgbquad[i*Inp.BMInfoHeader.Width + j] = Inp.Rgbquad[i*Inp.BMInfoHeader.Width + j];

	}
	else printf("Ошибка: в изображение уже проинициализированно другим разрешением и/или битностью\n");
}

void Image::initializeFromImage(const Image& Inp) // делает копию изображения (заполняет заголовок и копирует данные)
{
	setEmptyImageParams(); // Заполняем параметры пустого изображения
	setHeaderAndAllocateMemory(Inp.BMInfoHeader.Width, Inp.BMInfoHeader.Height, Inp.BMInfoHeader.BitCount); // заполняем заголовок и выделяем нужную память
	copyDataFromImage(Inp); // копируем данные изображения
}

Image::Image(const Image& im)// Реализация конструктора с копированием изображения
{
	initializeFromImage(im); // делаем копию изображения
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
			for (int j = 0; j < (int)BMInfoHeader.Width; j++)
			{
				RGBTRIPLE fileTriple;
				fileTriple.Blue = Rgbquad[i*BMInfoHeader.Width + j].Blue;
				fileTriple.Green = Rgbquad[i*BMInfoHeader.Width + j].Green;
				fileTriple.Red = Rgbquad[i*BMInfoHeader.Width + j].Red;
				fwrite(&(fileTriple), sizeof(RGBTRIPLE), 1, f);
			}

			// записываем нужное количество 0 для смещения между строками изображения
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
				unsigned char paletteColorIndex = getNearestPaletteColorIndex(Rgbquad[i*BMInfoHeader.Width + j].Red); // получаем индекс палитры для текущего цвета
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

void Image::writeimage(char* fileName) // выполняет запись в файл
{
	if (!Rgbquad)
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

int Image::loadImageDataFromFile(FILE* f)
{
	BITMAPFILEHEADER BMFileHeader;
	BITMAPINFOHEADER FileBMInfoHeader;
	RGBQUAD *filePalette = NULL;

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
	if (!isAllowedBitCount(FileBMInfoHeader.BitCount))
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
	else // загрузка изображений с палитрой
	{
		const BYTE topBitsOffset = 8 - BMInfoHeader.BitCount; // показывает на сколько нужно переметить биты из старшей части байта, чтобы получить цвет
		BYTE colorMask = (1 << BMInfoHeader.BitCount) - 1; // выставляем младшие BMInfoHeader.BitCount бит в 1 для получения маски цвета
		colorMask <<= 8 - BMInfoHeader.BitCount; // перемещаем эти единицы в старшие биты байта, так как данные располагаются от старших битов к младшим

		for (int i = BMInfoHeader.Height - 1; i >= 0; i--)
		{
			int leftBits = 0; // указывает сколько непрочитанных бит осталось в байте
			BYTE paletteByte = 0; // текущий считанный байт в изображении
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

				BYTE sourceGrayscale = getGrayscaleColor(filePalette[paletteIndex].Red, filePalette[paletteIndex].Green, filePalette[paletteIndex].Blue); // вычисляем оттенок серого для цвета пикселя из файла
				BYTE grayscale = Palette[getNearestPaletteColorIndex(sourceGrayscale)].Red; // получаем ближайший к нему цвет из палиты

				// заполняем данные изображения
				Rgbquad[i * BMInfoHeader.Width + j].Red = grayscale;
				Rgbquad[i * BMInfoHeader.Width + j].Green = grayscale;
				Rgbquad[i * BMInfoHeader.Width + j].Blue = grayscale;
				Rgbquad[i * BMInfoHeader.Width + j].Reserved = 0;
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
	if (Rgbquad)
	{
		printf("Ошибка: нельзя загружать данные в уже созданное изображение\n");
		return 0;
	}

	FILE* f; // открываем требуемый файл
	f = fopen(fileName, "rb"); // необходимо открывать бинарный файл
	if (!f)
	{
		printf("Ошибка: не удалось прочитать файл %s\n", fileName);
		return 0;
	}

	int result = loadImageDataFromFile(f);// производим загрузку данных
	fclose(f);// закрываем файл
	return result;
}

Image::Image(char* fileName)// Реализация конструктора при загрузке из файла(с указанием файла)
{
	setEmptyImageParams(); // Заполняем параметры пустого изображения
	loadimage(fileName); // выполняем загрузку из файла с переданным именем
}

BYTE Image::getGrayscaleColor(BYTE Red, BYTE Green, BYTE Blue) // вычисляет градацию серого для переданного цвета
{
	int result = (int)(Red * 0.299 + Green * 0.597 + Blue * 0.114); // формула из методички
	if (result > 255) result = 255;// проверяем выход за границы байта
	return (unsigned char)result;
}

BYTE Image::getNearestPaletteColorIndex(BYTE grayscaleColor) // бинарным поиском ищет ближайший цвет в палитре (так как она упорядочена по возрастанию оттенков серого)
{
	int minIndex = 0; // установка минимального индекса для поиска
	int maxIndex = BMInfoHeader.ColorUsed - 1; // установка максимального индекса для поиска
	while (maxIndex >= minIndex) // продолжаем искать пока минимальный индекс не перейдет за максимальный
	// (в этом случае значение grayscaleColor находится между Palette[minIndex - 1] и Palette[minIndex], так как сначала всегда
	// проверяем можно ли изменить minIndex), либо пока не найдем искомое значение
	{
		int middleIndex = (minIndex + maxIndex) / 2; // вычисление среднего индекса между крайними
		if (Palette[middleIndex].Red == grayscaleColor) // проверяем не нашли ли искомое значение (так как палитра состоит из оттенков серого
		// Palette[middleIndex].Red == Palette[middleIndex].Green == Palette[middleIndex].Blue)
			return middleIndex; // возвращаем индекс палитры в случае успеха
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

int Image::getAdditionalRowOffsetInFile(DWORD Width, WORD BitCount) // считает сколько нужно дописывать в файл байт после записи строки изображения, чтобы получить размет строки кратным 4
{
	int remainder = getImageRowSize(Width, BitCount) % 4; // вычисляем остаток от деления длины строки на 
	return remainder ? 4 - remainder : 0; // возвращаем дополнение до 4
}

int Image::getImageRowSize(DWORD Width, WORD BitCount) // возвращает сколько байт требуется для записи строки изображения в файл
{
	// Width * BitCount дает необходимое число бит для хранения строки изображения
	// ((Width * BitCount + 7) / 8) вычисляет количество байт и округляет в большую сторону
	return (Width * BitCount + 7) / 8;
}

int Image::getTotalImageSize(DWORD Width, DWORD Height, WORD BitCount) // возвращает полный размер данных при записи в файл
{
	// вычисляем сколько байт требуется для записи строки изображения в файл, добавляем нужное количество байт для кратности 4 и умножаем на количество строк
	return (getImageRowSize(BMInfoHeader.Width, BMInfoHeader.BitCount) + getAdditionalRowOffsetInFile(Width, BitCount)) * Height;
}

bool Image::isAllowedBitCount(WORD BitCount) // проверяет допустимость битности изображения
{
	return BitCount == 32 || BitCount == 24 || BitCount == 8 || BitCount == 4 || BitCount == 1; // 3-ий вариант
}

void Image::setHeaderAndAllocateMemory(DWORD Width, DWORD Height, WORD BitCount)
{
	// Заполняем необходимые поля информации об изображении
	BMInfoHeader.Width = Width; // Ширина точечного рисунка в пикселях
	BMInfoHeader.Height = Height; // Высота точечного рисунка в пикселях
	BMInfoHeader.Planes = 1; // Число плоскостей целевого устройства = 1
	BMInfoHeader.BitCount = BitCount; // Глубина цвета, число бит на точку = 1,4,8,24,32 
	BMInfoHeader.SizeImage = getTotalImageSize(Width, Height, BitCount); // Размер изображения в байтах
	if ((int)BMInfoHeader.BitCount <= 8)
	{
		BMInfoHeader.ColorUsed = 1 << BMInfoHeader.BitCount; // = 2 в степени BMInfoHeader.BitCount
		Palette = new RGBQUAD[BMInfoHeader.ColorUsed]; // Выделение памяти под палитру цветов
		for (int i = 0; i < (int)BMInfoHeader.ColorUsed; i++) // заполняем значения палитры градациями серого от 0 до 255
		{
			BYTE color = (BYTE)(255 * i / (BMInfoHeader.ColorUsed - 1));
			Palette[i].Red = color;
			Palette[i].Green = color;
			Palette[i].Blue = color;
			Palette[i].Reserved = 0;
		}
	}

	// Выделение памяти для двумерного массива размером Height*Width типа RGBQUAD
	if (BMInfoHeader.SizeImage > 0)	Rgbquad = new RGBQUAD[BMInfoHeader.Height * BMInfoHeader.Width];
	else Rgbquad = NULL;
}

Image::Image(char Mode, WORD BCount, DWORD Width, DWORD Height)// Реализация конструктора создания изображения(с указанием параметров изображения)
{
	setEmptyImageParams();// Выставление начальных параметров, соответствующих пустому изображению

	// Проверяем какую желаемую битность нам передали в конструктор
	// Поддерживается хранение данных только в RGBQUAD(вариант 3)
	if (isAllowedBitCount(BCount))
	{
		setHeaderAndAllocateMemory(Width, Height, BCount); // заполняем заголовок изображения и выделяем память
		if (Palette) Mode = Palette[getNearestPaletteColorIndex(Mode)].Red; // получаем ближайший к переданному цвет в палитре, если она используется

		// Заполнение данных изображения переданным значением
		for (int i = 0; i < (int)BMInfoHeader.Height; i++)
			for (int j = 0; j < (int)BMInfoHeader.Width; j++)
			{
				Rgbquad[i * BMInfoHeader.Width + j].Red = Mode;
				Rgbquad[i * BMInfoHeader.Width + j].Green = Mode;
				Rgbquad[i * BMInfoHeader.Width + j].Blue = Mode;
				Rgbquad[i * BMInfoHeader.Width + j].Reserved = Mode;
			}

	}// Задали неподдерживаемый параметр битности	
	else printf("Ошибка: поддерживается создание изображения только 4, 8, 32 бит.\n");// Выводим сообщение об ошибке
}

// Выставление параметров, соответствующих пустому изображению
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

Image::Image()// Реализация конструктора без параметров
{
	setEmptyImageParams(); // Заполняем параметры пустого изображения
}

Image::~Image()// Реализация деструктора
{
	if (Rgbquad) // Если есть указатель на данные, то нужно очистить выделенную под изображение память
	{
		delete[] Rgbquad; // Удаляем память, содержащую массив указателей на строки
		Rgbquad = NULL; // Инициализируем указатель на данные невалидным значением
		// (не обязательно, так как пороисходит в деструкторе)
	}
	if (Palette) // Удаление палитры, если она была создана
	{
		delete[] Palette;
		Palette = NULL;
	}
}

int main(void)
{
	setlocale(LC_ALL, "Russian");
	Image* im1 = new Image((char*)"beach.bmp"); // Создание объекта изображения из файла 
	Image* im2 = new Image(0, 24, 500, 400);// Создание изображения с заданными параметрами 
	Image im3, im4;// Создание пустого изображения 

	(*im2) /= *im1; // Приведение img к масштабу img2 
	im3 = *im1 / 8; // делаем битность 8
	im4 = *im1 / 1; // делаем битность 4
	im3.writeimage((char*)"beach_8.bmp");
	im4.writeimage((char*)"beach_1.bmp");
	im2->writeimage((char*)"beach_res.bmp");

	printf("beach.bmp have been converted.\n");
	scanf(" ");
	return 0;
}