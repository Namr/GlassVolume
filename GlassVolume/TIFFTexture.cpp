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

			//generate gradient data
			gradient = new uint8_t[height * width * depth * 3];
			
			for (int i = 0; i < (height * width * depth * 3); i++)
			{
				gradient[i] = 0;
			}

			for (int z = 1; z < depth - 1; z++)
			{
				for (int y = 1; y < height - 1; y++)
				{
					for (int x = 1; x < width - 1; x++)
					{
						gradient[(z * width * height * 3) + (y * width * 3) + (x * 3) + 0] = 0.5f *
							data[(z * width * height) + (y * width) + (x+1)] - data[(z * width * height) + (y * width) + (x - 1)];

						gradient[(z * width * height * 3) + (y * width * 3) + (x * 3) + 1] = 0.5f * 
							data[(z * width * height) + ((y + 1) * width) + x] - data[(z * width * height ) + ((y-1) * width) + (x - 1)];

						gradient[(z * width * height * 3) + (y * width * 3) + (x * 3) + 2] = 0.5f * 
							data[((z+1) * width * height) + (y * width) + x] - data[((z - 1) * width * height) + (y * width) + (x - 1)];
					}
				}
			}

			//generate the buffers for the texture data which will be supplied by another function
			glActiveTexture(GL_TEXTURE3);
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

			//do the same with the gradient
			glActiveTexture(GL_TEXTURE5);
			glGenTextures(1, &texture);
			glBindTexture(GL_TEXTURE_3D, texture);
			//allocate memory for the incoming data and upload it
			glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB8, width, height, depth, 0, GL_RGB, GL_UNSIGNED_BYTE, gradient);

			free(gradient);

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

	int numShades = 50;
	uint8_t *colorPickerData = new uint8_t[numShades * 3];
	
	for (int i = 0; i < numShades;i++)
	{
		int index = i * 3;
		colorPickerData[index + 0] = 0; //R
		colorPickerData[index + 1] = (float) i / numShades * 255; //G
		colorPickerData[index + 2] = 0; //B
	}

	glActiveTexture(GL_TEXTURE4);
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_1D, texture);

	//allocate memory for the incoming data and upload it
	glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB8, numShades, 0, GL_RGB, GL_UNSIGNED_BYTE, &colorPickerData[0]);

	//Set Texture Parameters
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glActiveTexture(GL_TEXTURE0);
}


TIFFTexture::~TIFFTexture()
{
}
