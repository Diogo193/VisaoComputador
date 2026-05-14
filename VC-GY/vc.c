#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include "vc.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//    FUNÇÕES: ALOCAR E LIBERTAR UMA IMAGEM
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

IVC* vc_image_new(int width, int height, int channels, int levels)
{
	IVC* image = (IVC*)malloc(sizeof(IVC));

	if (image == NULL) return NULL;
	if ((levels <= 0) || (levels > 255)) return NULL;

	image->width = width;
	image->height = height;
	image->channels = channels;
	image->levels = levels;
	image->bytesperline = image->width * image->channels;
	image->data = (unsigned char*)malloc(image->width * image->height * image->channels * sizeof(unsigned char));

	if (image->data == NULL)
	{
		return vc_image_free(image);
	}

	return image;
}

IVC* vc_image_free(IVC* image)
{
	if (image != NULL)
	{
		if (image->data != NULL)
		{
			free(image->data);
			image->data = NULL;
		}

		free(image);
		image = NULL;
	}

	return image;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//   FUNÇÕES: LEITURA E ESCRITA DE IMAGENS (PBM, PGM E PPM)
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

char* netpbm_get_token(FILE* file, char* tok, int len)
{
	char* t;
	int c;

	for (;;)
	{
		while (isspace(c = getc(file)));
		if (c != '#') break;
		do c = getc(file);
		while ((c != '\n') && (c != EOF));
		if (c == EOF) break;
	}

	t = tok;

	if (c != EOF)
	{
		do
		{
			*t++ = c;
			c = getc(file);
		} while ((!isspace(c)) && (c != '#') && (c != EOF) && (t - tok < len - 1));

		if (c == '#') ungetc(c, file);
	}

	*t = 0;

	return tok;
}

long int unsigned_char_to_bit(unsigned char* datauchar, unsigned char* databit, int width, int height)
{
	int x, y;
	int countbits;
	long int pos, counttotalbytes;
	unsigned char* p = databit;

	*p = 0;
	countbits = 1;
	counttotalbytes = 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = width * y + x;

			if (countbits <= 8)
			{
				*p |= (datauchar[pos] == 0) << (8 - countbits);
				countbits++;
			}
			if ((countbits > 8) || (x == width - 1))
			{
				p++;
				*p = 0;
				countbits = 1;
				counttotalbytes++;
			}
		}
	}

	return counttotalbytes;
}

void bit_to_unsigned_char(unsigned char* databit, unsigned char* datauchar, int width, int height)
{
	int x, y;
	int countbits;
	long int pos;
	unsigned char* p = databit;

	countbits = 1;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = width * y + x;

			if (countbits <= 8)
			{
				datauchar[pos] = (*p & (1 << (8 - countbits))) ? 0 : 1;
				countbits++;
			}
			if ((countbits > 8) || (x == width - 1))
			{
				p++;
				countbits = 1;
			}
		}
	}
}

IVC* vc_read_image(char* filename)
{
	FILE* file = NULL;
	IVC* image = NULL;
	unsigned char* tmp;
	char tok[20];
	long int size, sizeofbinarydata;
	int width, height, channels;
	int levels = 255;
	int v;

	if ((file = fopen(filename, "rb")) != NULL)
	{
		netpbm_get_token(file, tok, sizeof(tok));

		if (strcmp(tok, "P4") == 0) { channels = 1; levels = 1; }
		else if (strcmp(tok, "P5") == 0) channels = 1;
		else if (strcmp(tok, "P6") == 0) channels = 3;
		else
		{
			fclose(file);
			return NULL;
		}

		if (levels == 1)
		{
			if (sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &width) != 1 ||
				sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &height) != 1)
			{
				fclose(file);
				return NULL;
			}

			image = vc_image_new(width, height, channels, levels);
			if (image == NULL) return NULL;

			sizeofbinarydata = (image->width / 8 + ((image->width % 8) ? 1 : 0)) * image->height;
			tmp = (unsigned char*)malloc(sizeofbinarydata);
			if (tmp == NULL) return 0;

			if ((v = fread(tmp, sizeof(unsigned char), sizeofbinarydata, file)) != sizeofbinarydata)
			{
				vc_image_free(image);
				fclose(file);
				free(tmp);
				return NULL;
			}

			bit_to_unsigned_char(tmp, image->data, image->width, image->height);
			free(tmp);
		}
		else
		{
			if (sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &width) != 1 ||
				sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &height) != 1 ||
				sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &levels) != 1 || levels <= 0 || levels > 255)
			{
				fclose(file);
				return NULL;
			}

			image = vc_image_new(width, height, channels, levels);
			if (image == NULL) return NULL;

			size = image->width * image->height * image->channels;

			if ((v = fread(image->data, sizeof(unsigned char), size, file)) != size)
			{
				vc_image_free(image);
				fclose(file);
				return NULL;
			}
		}

		fclose(file);
	}

	return image;
}

int vc_write_image(char* filename, IVC* image)
{
	FILE* file = NULL;
	unsigned char* tmp;
	long int totalbytes, sizeofbinarydata;

	if (image == NULL) return 0;

	if ((file = fopen(filename, "wb")) != NULL)
	{
		if (image->levels == 1)
		{
			sizeofbinarydata = (image->width / 8 + ((image->width % 8) ? 1 : 0)) * image->height + 1;
			tmp = (unsigned char*)malloc(sizeofbinarydata);
			if (tmp == NULL) return 0;

			fprintf(file, "%s %d %d\n", "P4", image->width, image->height);

			totalbytes = unsigned_char_to_bit(image->data, tmp, image->width, image->height);
			if (fwrite(tmp, sizeof(unsigned char), totalbytes, file) != totalbytes)
			{
				fclose(file);
				free(tmp);
				return 0;
			}

			free(tmp);
		}
		else
		{
			fprintf(file, "%s %d %d 255\n", (image->channels == 1) ? "P5" : "P6", image->width, image->height);

			if (fwrite(image->data, image->bytesperline, image->height, file) != image->height)
			{
				fclose(file);
				return 0;
			}
		}

		fclose(file);
		return 1;
	}

	return 0;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//   FUNÇÕES: PROCESSAMENTO
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

int vc_bgr_to_hsv(IVC* src, IVC* dst)
{
	unsigned char* datasrc = src->data;
	unsigned char* datadst = dst->data;
	int width = src->width;
	int height = src->height;
	int x, y;
	long int pos_src, pos_dst;

	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if ((src->width != dst->width) || (src->height != dst->height)) return 0;
	if ((src->channels != 3) || (dst->channels != 3)) return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos_src = y * src->bytesperline + x * src->channels;
			pos_dst = y * dst->bytesperline + x * dst->channels;

			float b = datasrc[pos_src] / 255.0f;
			float g = datasrc[pos_src + 1] / 255.0f;
			float r = datasrc[pos_src + 2] / 255.0f;

			float max = (r > g) ? ((r > b) ? r : b) : ((g > b) ? g : b);
			float min = (r < g) ? ((r < b) ? r : b) : ((g < b) ? g : b);
			float delta = max - min;

			float h = 0.0f;
			float s = (max == 0.0f) ? 0.0f : (delta / max);
			float v = max;

			if (delta != 0.0f)
			{
				if (max == r)
					h = 60.0f * fmodf(((g - b) / delta), 6.0f);
				else if (max == g)
					h = 60.0f * (((b - r) / delta) + 2.0f);
				else
					h = 60.0f * (((r - g) / delta) + 4.0f);

				if (h < 0.0f) h += 360.0f;
			}

			unsigned char H = (unsigned char)(h / 2.0f);     // 0..179
			unsigned char S = (unsigned char)(s * 255.0f);   // 0..255
			unsigned char V = (unsigned char)(v * 255.0f);   // 0..255

			datadst[pos_dst] = H;
			datadst[pos_dst + 1] = S;
			datadst[pos_dst + 2] = V;
		}
	}

	return 1;
}

int vc_hsv_segmentation(IVC* src, IVC* dst,
	int hmin, int hmax,
	int smin, int smax,
	int vmin, int vmax)
{
	unsigned char* datasrc = src->data;
	unsigned char* datadst = dst->data;
	int width = src->width;
	int height = src->height;
	int x, y;
	long int pos_src, pos_dst;

	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if ((src->width != dst->width) || (src->height != dst->height)) return 0;
	if ((src->channels != 3) || (dst->channels != 1)) return 0;

	if ((hmin < 0) || (hmin > 179) || (hmax < 0) || (hmax > 179)) return 0;
	if ((smin < 0) || (smin > 255) || (smax < 0) || (smax > 255) || (smin > smax)) return 0;
	if ((vmin < 0) || (vmin > 255) || (vmax < 0) || (vmax > 255) || (vmin > vmax)) return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos_src = y * src->bytesperline + x * src->channels;
			pos_dst = y * dst->bytesperline + x * dst->channels;

			unsigned char h = datasrc[pos_src];
			unsigned char s = datasrc[pos_src + 1];
			unsigned char v = datasrc[pos_src + 2];

			int in_h = (hmin <= hmax) ? (h >= hmin && h <= hmax) : (h >= hmin || h <= hmax);
			int in_s = (s >= smin && s <= smax);
			int in_v = (v >= vmin && v <= vmax);

			datadst[pos_dst] = (in_h && in_s && in_v) ? 255 : 0;
		}
	}

	return 1;
}

int vc_binary_dilate(IVC* src, IVC* dst, int kernel)
{
	int x, y, kx, ky;
	int xmin, xmax, ymin, ymax;
	long int pos, posk;
	int max;
	unsigned char* datasrc = src->data;
	unsigned char* datadst = dst->data;

	if ((src == NULL) || (dst == NULL)) return 0;
	if ((src->width <= 0) || (src->height <= 0) || (src->channels != 1) ||
		(dst->width <= 0) || (dst->height <= 0) || (dst->channels != 1)) return 0;
	if ((src->width != dst->width) || (src->height != dst->height)) return 0;
	if ((src->data == NULL) || (dst->data == NULL)) return 0;

	if (kernel < 3) kernel = 3;
	if ((kernel % 2) == 0) kernel++;

	for (y = 0; y < src->height; y++)
	{
		for (x = 0; x < src->width; x++)
		{
			xmin = x - kernel / 2;
			xmax = x + kernel / 2;
			ymin = y - kernel / 2;
			ymax = y + kernel / 2;

			if (xmin < 0) xmin = 0;
			if (ymin < 0) ymin = 0;
			if (xmax >= src->width) xmax = src->width - 1;
			if (ymax >= src->height) ymax = src->height - 1;

			max = 0;

			for (ky = ymin; ky <= ymax; ky++)
			{
				for (kx = xmin; kx <= xmax; kx++)
				{
					posk = ky * src->bytesperline + kx * src->channels;
					if (datasrc[posk] > max) max = datasrc[posk];
				}
			}

			pos = y * src->bytesperline + x * src->channels;
			datadst[pos] = (unsigned char)max;
		}
	}

	return 1;
}

int vc_binary_erode(IVC* src, IVC* dst, int kernel)
{
	int x, y, kx, ky;
	int xmin, xmax, ymin, ymax;
	long int pos, posk;
	int min;
	unsigned char* datasrc = src->data;
	unsigned char* datadst = dst->data;

	if ((src == NULL) || (dst == NULL)) return 0;
	if ((src->width <= 0) || (src->height <= 0) || (src->channels != 1) ||
		(dst->width <= 0) || (dst->height <= 0) || (dst->channels != 1)) return 0;
	if ((src->width != dst->width) || (src->height != dst->height)) return 0;
	if ((src->data == NULL) || (dst->data == NULL)) return 0;

	if (kernel < 3) kernel = 3;
	if ((kernel % 2) == 0) kernel++;

	for (y = 0; y < src->height; y++)
	{
		for (x = 0; x < src->width; x++)
		{
			xmin = x - kernel / 2;
			xmax = x + kernel / 2;
			ymin = y - kernel / 2;
			ymax = y + kernel / 2;

			if (xmin < 0) xmin = 0;
			if (ymin < 0) ymin = 0;
			if (xmax >= src->width) xmax = src->width - 1;
			if (ymax >= src->height) ymax = src->height - 1;

			min = 255;

			for (ky = ymin; ky <= ymax; ky++)
			{
				for (kx = xmin; kx <= xmax; kx++)
				{
					posk = ky * src->bytesperline + kx * src->channels;
					if (datasrc[posk] < min) min = datasrc[posk];
				}
			}

			pos = y * src->bytesperline + x * src->channels;
			datadst[pos] = (unsigned char)min;
		}
	}

	return 1;
}

int vc_binary_open(IVC* src, IVC* dst, int kernel)
{
	int ret = 1;
	IVC* tmp = vc_image_new(src->width, src->height, src->channels, src->levels);
	ret &= vc_binary_erode(src, tmp, kernel);
	ret &= vc_binary_dilate(tmp, dst, kernel);
	vc_image_free(tmp);
	return ret;
}

int vc_binary_close(IVC* src, IVC* dst, int kernel)
{
	int ret = 1;
	IVC* tmp = vc_image_new(src->width, src->height, src->channels, src->levels);
	ret &= vc_binary_dilate(src, tmp, kernel);
	ret &= vc_binary_erode(tmp, dst, kernel);
	vc_image_free(tmp);
	return ret;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//   FUNÇÕES: ANÁLISE DE BLOBS E CLASSIFICAÇÃO
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

float vc_calculate_perimeter(IVC* src, int label)
{
	int x, y;
	int width = src->width;
	int height = src->height;
	unsigned char* data = src->data;
	int count = 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			int pos = y * width + x;

			if (data[pos] == label)
			{
				int boundary = 0;

				if (x == 0 || data[y * width + (x - 1)] != label) boundary = 1;
				else if (x == width - 1 || data[y * width + (x + 1)] != label) boundary = 1;
				else if (y == 0 || data[(y - 1) * width + x] != label) boundary = 1;
				else if (y == height - 1 || data[(y + 1) * width + x] != label) boundary = 1;

				if (boundary) count++;
			}
		}
	}

	return (float)count;
}

int vc_get_caliber(float diameter_mm)
{
	if (diameter_mm >= 92.0f) return 1;
	if (diameter_mm >= 87.0f) return 2;
	if (diameter_mm >= 84.0f) return 3;
	if (diameter_mm >= 81.0f) return 4;
	if (diameter_mm >= 77.0f) return 5;
	if (diameter_mm >= 73.0f) return 6;
	if (diameter_mm >= 70.0f) return 7;
	if (diameter_mm >= 67.0f) return 8;
	if (diameter_mm >= 63.0f) return 9;
	if (diameter_mm >= 56.0f) return 10;
	if (diameter_mm >= 53.0f) return 11;
	if (diameter_mm >= 51.0f) return 12;
	if (diameter_mm >= 49.0f) return 13;

	return 0; // fora de norma
}

float vc_compute_deformation(OVC* blob)
{
	if (blob == NULL) return 1.0f;

	int width_px = (blob->xmax - blob->xmin) + 1;
	int height_px = (blob->ymax - blob->ymin) + 1;
	int max_px = (width_px > height_px) ? width_px : height_px;

	if (max_px <= 0) return 1.0f;

	float radius = max_px / 2.0f;
	float ideal_area = (float)M_PI * radius * radius;

	if (ideal_area <= 0.0f) return 1.0f;

	float deformation = 1.0f - ((float)blob->area / ideal_area);

	if (deformation < 0.0f) deformation = 0.0f;
	if (deformation > 1.0f) deformation = 1.0f;

	return deformation;
}

void vc_get_category(float deformation, char* category)
{
	if (category == NULL) return;

	if (deformation < 0.05f) strcpy(category, "Extra");
	else if (deformation < 0.15f) strcpy(category, "I");
	else if (deformation < 0.30f) strcpy(category, "II");
	else strcpy(category, "III");
}

OVC* vc_binary_blob_labelling(IVC* src, IVC* dst, int* nblobs)
{
	int x, y, i;
	int width = src->width;
	int height = src->height;
	int size = width * height;

	unsigned char* datasrc = src->data;
	unsigned char* datadst = dst->data;

	int* labels = (int*)calloc(size, sizeof(int));
	int* equiv = (int*)calloc(size, sizeof(int));

	int label = 1;

	if ((src == NULL) || (dst == NULL) || (nblobs == NULL)) return NULL;
	if ((src->channels != 1) || (dst->channels != 1)) return NULL;

	// PASSO 1
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			int pos = y * width + x;

			if (datasrc[pos] != 0)
			{
				int A = (x > 0) ? labels[pos - 1] : 0;
				int B = (y > 0) ? labels[pos - width] : 0;
				int C = (x > 0 && y > 0) ? labels[pos - width - 1] : 0;
				int D = (x < width - 1 && y > 0) ? labels[pos - width + 1] : 0;

				int min = 0;
				if (A) min = A;
				if (B && (min == 0 || B < min)) min = B;
				if (C && (min == 0 || C < min)) min = C;
				if (D && (min == 0 || D < min)) min = D;

				if (min == 0)
				{
					labels[pos] = label;
					equiv[label] = label;
					label++;
				}
				else
				{
					labels[pos] = min;
					if (A && A != min) equiv[A] = min;
					if (B && B != min) equiv[B] = min;
					if (C && C != min) equiv[C] = min;
					if (D && D != min) equiv[D] = min;
				}
			}
		}
	}

	// PASSO 2 (resolver equivalências)
	for (i = 1; i < label; i++)
	{
		int root = i;
		while (equiv[root] != root)
			root = equiv[root];

		int j = i;
		while (equiv[j] != j)
		{
			int parent = equiv[j];
			equiv[j] = root;
			j = parent;
		}
	}

	// PASSO 3 (re-rotular)
	int* new_labels = (int*)calloc(label, sizeof(int));
	int new_label = 1;

	for (i = 0; i < size; i++)
	{
		if (labels[i] != 0)
		{
			int root = equiv[labels[i]];
			if (new_labels[root] == 0)
			{
				new_labels[root] = new_label;
				new_label++;
			}
			datadst[i] = (unsigned char)new_labels[root];
		}
		else
		{
			datadst[i] = 0;
		}
	}

	*nblobs = new_label - 1;

	free(labels);
	free(equiv);
	free(new_labels);

	OVC* blobs = (OVC*)calloc(*nblobs, sizeof(OVC));
	if (blobs == NULL) return NULL;

	for (i = 0; i < *nblobs; i++)
	{
		blobs[i].label = i + 1;
		blobs[i].area = 0;
		blobs[i].perimeter = 0.0f;
		blobs[i].xmin = width;
		blobs[i].ymin = height;
		blobs[i].xmax = 0;
		blobs[i].ymax = 0;
		blobs[i].xc = 0;
		blobs[i].yc = 0;
		blobs[i].diameter_mm = 0.0f;
		blobs[i].caliber = 0;
		blobs[i].deformation = 0.0f;
		strcpy(blobs[i].category, "N/A");
	}

	return blobs;
}

int vc_binary_blob_info(IVC* src, OVC* blobs, int nblobs)
{
	int x, y;
	int width = src->width;
	int height = src->height;
	unsigned char* data = src->data;

	if ((src == NULL) || (blobs == NULL)) return 0;
	if (src->channels != 1) return 0;

	// Reset
	for (int i = 0; i < nblobs; i++)
	{
		blobs[i].area = 0;
		blobs[i].perimeter = 0.0f;
		blobs[i].xmin = width;
		blobs[i].ymin = height;
		blobs[i].xmax = 0;
		blobs[i].ymax = 0;
		blobs[i].xc = 0;
		blobs[i].yc = 0;
		blobs[i].diameter_mm = 0.0f;
		blobs[i].caliber = 0;
		blobs[i].deformation = 0.0f;
		strcpy(blobs[i].category, "N/A");
	}

	// Percorrer imagem etiquetada
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			int pos = y * width + x;
			int label = data[pos];

			if (label > 0 && label <= nblobs)
			{
				int idx = label - 1;

				blobs[idx].area++;
				blobs[idx].xc += x;
				blobs[idx].yc += y;

				if (x < blobs[idx].xmin) blobs[idx].xmin = x;
				if (x > blobs[idx].xmax) blobs[idx].xmax = x;
				if (y < blobs[idx].ymin) blobs[idx].ymin = y;
				if (y > blobs[idx].ymax) blobs[idx].ymax = y;
			}
		}
	}

	// Finalizar características
	for (int i = 0; i < nblobs; i++)
	{
		if (blobs[i].area > 0)
		{
			blobs[i].xc /= blobs[i].area;
			blobs[i].yc /= blobs[i].area;

			blobs[i].perimeter = vc_calculate_perimeter(src, blobs[i].label);

			int width_px = (blobs[i].xmax - blobs[i].xmin) + 1;
			int height_px = (blobs[i].ymax - blobs[i].ymin) + 1;
			int max_px = (width_px > height_px) ? width_px : height_px;

			blobs[i].diameter_mm = (float)max_px * VC_PIXEL_TO_MM;
			blobs[i].caliber = vc_get_caliber(blobs[i].diameter_mm);

			blobs[i].deformation = vc_compute_deformation(&blobs[i]);
			vc_get_category(blobs[i].deformation, blobs[i].category);
		}
	}

	return 1;
}

int vc_crosses_line(int prev_y, int curr_y, int line_y)
{
	return (prev_y < line_y && curr_y >= line_y) ? 1 : 0;
}