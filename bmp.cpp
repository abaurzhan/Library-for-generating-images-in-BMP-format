#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

class Image
{
	struct pixel
	{
		uint8_t blue,
			green,
			red;
	};

	struct bmp_header {
		uint16_t type;              // "BM" (0x42, 0x4D)
		uint32_t size;              // file size
		uint16_t reserved1;         // not used (0)
		uint16_t reserved2;         // not used (0)
		uint32_t offset;            // offset to image data (54B)
		uint32_t dib_size;          // DIB header size (40B)
		uint32_t width;             // width in pixels
		uint32_t height;            // height in pixels
		uint16_t planes;            // 1
		uint16_t bpp;               // bits per pixel (1/4/8/24/32)
		uint32_t compression;       // compression type (0/1/2) 0
		uint32_t image_size;        // size of picture in bytes, 0
		uint32_t x_ppm;             // X Pixels per meter (0)
		uint32_t y_ppm;             // X Pixels per meter (0)
		uint32_t num_colors;        // number of colors (0)
		uint32_t important_colors;  // important colors (0)
	};

	size_t width;
	size_t height;
	pixel** pixelarray = nullptr;

	void clear_array();
	Image() {}
public:
	Image(int, int, int);
	Image(const char* filename);
	Image(const Image& obj);
	Image operator= (const Image& img);
	~Image();
	void save(const char* imagename);
	bool load(const char* filename);
	bool load(std::istream& file);

	void square_draw(int height_m, int width_m, int x, int y, int color);
	Image& crop(const int start_y, const int start_x, const int height, const int width);
	void scale(float factor);
	void histogram(int* arr, int n, int color, int y_max_scale);
	void dice(int k);
	void brightness(float);
	void contrast(float);

};

Image::Image(const char* filename)
{
	load(filename);
}

Image::Image(int width, int height, int color) //color 72 80 FF 00
{
	this->height = height;
	this->width = width;
	pixelarray = new pixel *[height];
	for (int i = 0; i < height; ++i)
	{
		pixelarray[i] = new pixel[width];
		for (int j = 0; j < width; ++j)
		{
			pixelarray[i][j] = *((pixel*)&color);
		}
	}
}

Image::Image(const Image& obj)
{
	width = obj.width;
	height = obj.height;
	pixelarray = new pixel *[obj.height];
	for (int i = 0; i < obj.height; ++i)
	{
		pixelarray[i] = new pixel[obj.width];
		for (int j = 0; j < width; ++j)
		{
			pixelarray[i][j] = obj.pixelarray[i][j];
		}
	}

	//std::cout << "COPY\n";
}

// c = b = a = img;
Image Image::operator= (const Image& img)
{
	if (this == &img)
	{
		return *this;
	}
	clear_array();
	width = img.width;
	height = img.height;
	pixelarray = new pixel *[img.height];
	for (int i = 0; i < img.height; ++i)
	{
		pixelarray[i] = new pixel[img.width];
		for (int j = 0; j < width; ++j)
		{
			pixelarray[i][j] = img.pixelarray[i][j];
		}
	}
	// Доделать
	//std::cout << "OP =\n";
	return *this;
}

Image::~Image()
{
	clear_array();
}

void Image::clear_array()
{
	if (!pixelarray) return;
	for (int i = 0; i < height; ++i)
	{
		delete[] pixelarray[i];
	}
	delete[] pixelarray;
	pixelarray = nullptr;
}

void Image::save(const char* imagename)
{
	std::ofstream f(imagename, std::ios::out | std::ios::binary);

	if (!f.is_open())
	{
		std::cout << "File is not opened! Method:save\n";
		return;
	}
	char header[54] = {};
	header[0] = 'B';
	header[1] = 'M';
	*((int*)(header + 2)) = width * height * 3 + 54;
	//offset to image data
	*((int*)(header + 10)) = 54;
	//DIB header size(40B)
	*((int*)(header + 14)) = 40;
	//width
	*((int*)(header + 18)) = width;
	//height
	*((int*)(header + 22)) = height;
	////planes
	*((int*)(header + 26)) = 1;
	////bits per pixel
	*((int*)(header + 28)) = 24;

	f.write((char*)header, 54);
	for (int i = 0; i < height; ++i)
	{
		f.write((char*)pixelarray[i], 3 * width);
	}
	f.close();
}

bool Image::load(const char* filename)
{
	std::ifstream file(filename, std::ios::binary);
	if (!file)
	{
		std::cerr << "Failure to open file. Method load\n";
		exit(1);
	}
	bool result = load(file);
	file.close();
	return result;
}

bool Image::load(std::istream& file)
{
	if (file.get() != 0x42 || file.get() != 0x4D)
	{
		std::cerr << "File  isn't a bmp file. Method:load\n";
		return false;
	}
	if (pixelarray)
	{
		clear_array();
	}
	int size = 0;
	file.read((char*)&size, 4);

	file.seekg(18, std::ios::beg);
	file.read((char*)&width, 4);
	file.read((char*)&height, 4);
	file.seekg(54, std::ios::beg);
	std::cout << "W: " << width << "\nH: " << height << '\n';

	pixelarray = new pixel *[height];
	for (int i = 0; i < height; ++i)
	{
		pixelarray[i] = new pixel[width];
		file.read((char*)pixelarray[i], 3 * width);
	}
	if (size != file.tellg())
	{
		std::cerr << "Looks like bmp file is corrupted\n";
		clear_array();
		return false;
	}
	return true;
}

void Image::square_draw(int height, int width, int x, int y, int color)
{
	if (x + width > this->width || y + height > this->height || this->width < 0 || this->height < 0)
	{
		std::cout << "Out of range! Method:square draw\n";
		return;
	}
	for (int i = 0; i < height; ++i)
	{
		for (int j = 0; j < width; ++j)
		{
			pixelarray[i + y][j + x] = *((pixel*)&color);
		}
	}
}

//Image Image::crop(const int start_y, const int start_x, const int height, const int width)
//{
//	if (start_x + width > this->width || start_y + height > this->height || this->width < 0 || this->height < 0)
//	{
//		return;
//	}
//  Image img;


//	pixel** temp = new pixel * [this->height];
//	for (int i = 0; i < this->height; ++i)
//	{
//		temp[i] = new pixel[this->width];
//		for (int j = 0; j < this->width; ++j)
//		{
//			temp[i][j] = pixelarray[i][j];
//		}
//	}
//	
//	pixel** new_image = new pixel * [height];
//	for (int i = 0; i < height; ++i)
//	{
//		new_image[i] = new pixel[width];
//		for (int j = 0; j < width; ++j)
//		{
//			new_image[i][j] = temp[i+start_y][j+start_x];
//		}
//	}
//	for (int i = 0; i < this->height; ++i)
//	{
//		delete[] temp[i];
//	}
//	delete[] temp;
//	
//	return new_image;
//}

void Image::scale(float factor)
{
	if (!pixelarray)
	{
		return;
	}
	int height = round(this->height * factor);
	width = round(width * factor);
	pixel** temp = new pixel *[height];
	for (int i = 0; i < height; ++i)
	{
		temp[i] = new pixel[width];
		for (int j = 0; j < width; ++j)
		{
			temp[i][j] = pixelarray[(int)(i / factor)][(int)(j / factor)];
		}
	}
	clear_array();
	this->height = height;
	pixelarray = temp;
}

void Image::histogram(int* arr, int n, int color, int y_max_scale)
{
	if (!pixelarray)
	{
		return;
	}
	int d = width / n / 10;
	int width2 = width / n - d;
	int h;
	for (int i = 0; i < n; ++i)
	{
		if (y_max_scale < arr[i])
		{
			h = height;
		}
		else
		{
			h = height / (float)y_max_scale * arr[i];
		}
		square_draw(h, width2, d / 2 + width2 * i + i * d, 0, color);
	}
}

//x % n = {0..n-1}

void Image::dice(int k)
{
	int* res = new int[11]();
	for (int i = 0; i < k; ++i)
	{
		int num = (rand() % 6 + 1) + (rand() % 6 + 1);
		res[num - 2]++;
	}
	histogram(res, 11, 0xff0000, k / 3);

	delete[]res;
}


void Image::brightness(float brightAdj)
{
	if (!pixelarray)
	{
		return;
	}

	for (int i = 0; i < height; ++i)
	{
		for (int j = 0; j < width; ++j)
		{
			int red = (int)(pixelarray[i][j].red + brightAdj);
			int green = (int)(pixelarray[i][j].green + brightAdj);
			int blue = (int)(pixelarray[i][j].blue + brightAdj);

			pixelarray[i][j].red = red > 255 ? 255 : red < 0 ? 0 : red;
			pixelarray[i][j].green = green > 255 ? 255 : green < 0 ? 0 : green;
			pixelarray[i][j].blue = blue > 255 ? 255 : blue < 0 ? 0 : blue;
		}
	}
}

void Image::contrast(float factor)
{
	if (!pixelarray || factor < 0)
	{
		return;
	}

	for (int i = 0; i < height; ++i)
	{
		for (int j = 0; j < width; ++j)
		{
			int red = (int)(pixelarray[i][j].red * factor);
			int green = (int)(pixelarray[i][j].green * factor);
			int blue = (int)(pixelarray[i][j].blue * factor);

			pixelarray[i][j].red = red > 255 ? 255 : red;
			pixelarray[i][j].green = green > 255 ? 255 : green;
			pixelarray[i][j].blue = blue > 255 ? 255 : blue;
		}
	}
}


int main()
{
	//std::cout << sizeof(Image::bmp_header) << '\n';
	Image img("yard.bmp");
	img.contrast(1.5);
	img.save("new.bmp");
	img.load("yard.bmp");
	img.brightness(-30);
	img.save("new1.bmp");
	


	//system("pause");
	return 0;
}
