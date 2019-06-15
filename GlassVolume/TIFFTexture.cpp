#include "pch.h"
#include "TIFFTexture.h"


TIFFTexture::TIFFTexture(std::string filename)
{
	//find the number of layers in the tiff file
	TIFF *temp = TIFFOpen(filename.c_str(), "r");
	if (temp)
	{
		depth = 1;
		do
		{
			depth++;
		} while (TIFFReadDirectory(temp));
		TIFFClose(temp);
	}
	else
	{
		std::cerr << "Error Loading TIFF Image" << std::endl;
	}

	TIFF *image = TIFFOpen(filename.c_str(), "r");
	if (image)
	{
		TIFFGetField(image, TIFFTAG_IMAGEWIDTH, &width);
		TIFFGetField(image, TIFFTAG_IMAGELENGTH, &height);
		if (width > 0 && height > 0 && depth > 0)
		{
			uint8_t *buf = (uint8_t *)_TIFFmalloc(width * sizeof(uint8_t));
			data = new uint8_t[height * width * depth];

			for (int layer = 0; layer < depth; layer++)
			{
				for (int row = 0; row < height; row++)
				{
					TIFFReadScanline(image, (tdata_t)buf, row);
					for (int col = 0; col < width; col++)
						data[(layer * width * height) + (row * width) + col] = buf[col];
				}
				TIFFReadDirectory(image);
			}



			_TIFFfree((tdata_t)buf);
			TIFFClose(image);

			//generate the buffers for the texture data which will be supplied by another function
			glActiveTexture(GL_TEXTURE0);
			glGenTextures(1, &texture);
			glBindTexture(GL_TEXTURE_3D, texture);

			//allocate memory for the incoming data and upload it
			glTexImage3D(GL_TEXTURE_3D, 0, GL_R8, width, height, depth, 0, GL_RED, GL_UNSIGNED_BYTE, data);

			free(data);

			//Set Texture Parameters
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}
	}
	else
	{
		std::cerr << "Error Loading TIFF Image" << std::endl;
	}

	//generate the color picking texture

	uint8_t colorPickerData[30];

	for (int i = 0; i < 10; i++)
	{
		colorPickerData[(i * 3) + 0] = 0;
		colorPickerData[(i * 3) + 1] = (10 - i / 10) * 255;
		colorPickerData[(i * 3) + 2] = 0;
	}
	glActiveTexture(GL_TEXTURE1);
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_1D, texture);

	//allocate memory for the incoming data and upload it
	glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB8, 10, 0, GL_GREEN, GL_UNSIGNED_BYTE, colorPickerData);

	//Set Texture Parameters
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}


TIFFTexture::~TIFFTexture()
{
}
