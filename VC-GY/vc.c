#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <malloc.h>
#include "vc.h"
#include <math.h>

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
	image->data = (unsigned char*)malloc(image->width * image->height * image->channels * sizeof(char));

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

int vc_hsv_segmentation(IVC* src, IVC* dst, int hmin, int hmax, int smin, int smax, int vmin, int vmax)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	int bytesperline_src = src->width * src->channels;
	int channels_src = src->channels;
	unsigned char* datadst = (unsigned char*)dst->data;
	int bytesperline_dst = dst->width * dst->channels;
	int channels_dst = dst->channels;
	int width = src->width;
	int height = src->height;
	int x, y;
	long int pos_src, pos_dst;
	unsigned char h, s, v;
	int hmin_255, hmax_255, smin_255, smax_255, vmin_255, vmax_255;
	int in_h, in_s, in_v;

	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if ((src->width != dst->width) || (src->height != dst->height)) return 0;
	if ((src->channels != 3) || (dst->channels != 1)) return 0;

	if ((hmin < 0) || (hmin > 360) || (hmax < 0) || (hmax > 360)) return 0;
	if ((smin < 0) || (smin > 100) || (smax < 0) || (smax > 100) || (smin > smax)) return 0;
	if ((vmin < 0) || (vmin > 100) || (vmax < 0) || (vmax > 100) || (vmin > vmax)) return 0;

	hmin_255 = (hmin * 255) / 360;
	hmax_255 = (hmax * 255) / 360;
	smin_255 = (smin * 255) / 100;
	smax_255 = (smax * 255) / 100;
	vmin_255 = (vmin * 255) / 100;
	vmax_255 = (vmax * 255) / 100;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos_src = y * bytesperline_src + x * channels_src;
			pos_dst = y * bytesperline_dst + x * channels_dst;

			h = datasrc[pos_src];
			s = datasrc[pos_src + 1];
			v = datasrc[pos_src + 2];

			if (hmin_255 <= hmax_255)
				in_h = (h >= hmin_255 && h <= hmax_255);
			else
				in_h = (h >= hmin_255 || h <= hmax_255);

			in_s = (s >= smin_255 && s <= smax_255);
			in_v = (v >= vmin_255 && v <= vmax_255);

			datadst[pos_dst] = (in_h && in_s && in_v) ? 255 : 0;
		}
	}

	return 1;
}

int vc_rgb_to_hsv(IVC* src, IVC* dst)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	int bytesperline_src = src->width * src->channels;
	int channels_src = src->channels;
	unsigned char* datadst = (unsigned char*)dst->data;
	int bytesperline_dst = dst->width * dst->channels;
	int channels_dst = dst->channels;
	int width = src->width;
	int height = src->height;
	int x, y;
	long int pos_src, pos_dst;
	int r, g, b;
	float rf, gf, bf;
	float h, s, v;

	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if ((src->width != dst->width) || (src->height != dst->height)) return 0;
	if ((src->channels != 3) || (dst->channels != 3)) return 0;

	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			pos_src = y * bytesperline_src + x * channels_src;
			pos_dst = y * bytesperline_dst + x * channels_dst;

			b = datasrc[pos_src];
			g = datasrc[pos_src + 1];
			r = datasrc[pos_src + 2];

			rf = r / 255.0f;
			gf = g / 255.0f;
			bf = b / 255.0f;

			float max = rf > gf ? (rf > bf ? rf : bf) : (gf > bf ? gf : bf);
			float min = rf < gf ? (rf < bf ? rf : bf) : (gf < bf ? gf : bf);

			v = max;

			if (max == min) {
				h = 0;
				s = 0;
			}
			else {
				float d = max - min;
				s = max == 0 ? 0 : d / max;

				if (max == rf) h = (gf - bf) / d + (gf < bf ? 6 : 0);
				else if (max == gf) h = (bf - rf) / d + 2;
				else h = (rf - gf) / d + 4;
				h /= 6;
			}

			datadst[pos_dst] = (unsigned char)(h * 255);
			datadst[pos_dst + 1] = (unsigned char)(s * 255);
			datadst[pos_dst + 2] = (unsigned char)(v * 255);
		}
	}

	return 1;
}

int vc_binary_dilate(IVC* src, IVC* dst, int kernel) {
	int x, y, kx, ky;
	int xmin, xmax, ymin, ymax;
	long int pos, posk;
	int max;
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;

	if ((src == NULL) || (dst == NULL)) return 0;
	if ((src->width <= 0) || (src->height <= 0) || (src->channels != 1) ||
		(dst->width <= 0) || (dst->height <= 0) || (dst->channels != 1)) return 0;
	if ((src->width != dst->width) || (src->height != dst->height)) return 0;
	if ((src->data == NULL) || (dst->data == NULL)) return 0;

	if (kernel < 3) kernel = 3;
	if ((kernel % 2) == 0) kernel++;

	for (y = 0; y < src->height; y++) {
		for (x = 0; x < src->width; x++) {
			xmin = x - kernel / 2;
			xmax = x + kernel / 2;
			ymin = y - kernel / 2;
			ymax = y + kernel / 2;

			if (xmin < 0) xmin = 0;
			if (ymin < 0) ymin = 0;
			if (xmax >= src->width) xmax = src->width - 1;
			if (ymax >= src->height) ymax = src->height - 1;

			max = 0;

			for (ky = ymin; ky <= ymax; ky++) {
				for (kx = xmin; kx <= xmax; kx++) {
					posk = ky * src->bytesperline + kx * src->channels;
					if (datasrc[posk] > max) {
						max = datasrc[posk];
					}
				}
			}

			pos = y * src->bytesperline + x * src->channels;
			datadst[pos] = max;
		}
	}

	return 1;
}

int vc_binary_erode(IVC* src, IVC* dst, int kernel) {
	int x, y, kx, ky;
	int xmin, xmax, ymin, ymax;
	long int pos, posk;
	int min;
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;

	if ((src == NULL) || (dst == NULL)) return 0;
	if ((src->width <= 0) || (src->height <= 0) || (src->channels != 1) ||
		(dst->width <= 0) || (dst->height <= 0) || (dst->channels != 1)) return 0;
	if ((src->width != dst->width) || (src->height != dst->height)) return 0;
	if ((src->data == NULL) || (dst->data == NULL)) return 0;

	if (kernel < 3) kernel = 3;
	if ((kernel % 2) == 0) kernel++;

	for (y = 0; y < src->height; y++) {
		for (x = 0; x < src->width; x++) {
			xmin = x - kernel / 2;
			xmax = x + kernel / 2;
			ymin = y - kernel / 2;
			ymax = y + kernel / 2;

			if (xmin < 0) xmin = 0;
			if (ymin < 0) ymin = 0;
			if (xmax >= src->width) xmax = src->width - 1;
			if (ymax >= src->height) ymax = src->height - 1;

			min = 255;

			for (ky = ymin; ky <= ymax; ky++) {
				for (kx = xmin; kx <= xmax; kx++) {
					posk = ky * src->bytesperline + kx * src->channels;
					if (datasrc[posk] < min) {
						min = datasrc[posk];
					}
				}
			}

			pos = y * src->bytesperline + x * src->channels;
			datadst[pos] = min;
		}
	}

	return 1;
}

int vc_binary_open(IVC* src, IVC* dst, int kernel) {
	int ret = 1;
	IVC* tmp = vc_image_new(src->width, src->height, src->channels, src->levels);
	ret &= vc_binary_erode(src, tmp, kernel);
	ret &= vc_binary_dilate(tmp, dst, kernel);
	vc_image_free(tmp);
	return ret;
}

int vc_binary_close(IVC* src, IVC* dst, int kernel) {
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

float vc_calculate_perimeter(IVC* src, int label) {
	int x, y;
	int width = src->width;
	int height = src->height;
	unsigned char* data = src->data;
	int count = 0;

	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			int pos = y * width + x;
			if (data[pos] == label) {
				int is_boundary = 0;
				if (x == 0 || data[y * width + (x - 1)] != label) is_boundary = 1;
				else if (x == width - 1 || data[y * width + (x + 1)] != label) is_boundary = 1;
				else if (y == 0 || data[(y - 1) * width + x] != label) is_boundary = 1;
				else if (y == height - 1 || data[(y + 1) * width + x] != label) is_boundary = 1;

				if (is_boundary) count++;
			}
		}
	}
	return (float)count;
}

int get_tangerine_caliber(float diameter) {
	if (diameter >= 63.0f) return 1;
	if (diameter >= 58.0f) return 3;
	if (diameter >= 54.0f) return 4;
	if (diameter >= 50.0f) return 5;
	if (diameter >= 47.0f) return 6;
	if (diameter >= 45.0f) return 7;
	if (diameter >= 43.0f) return 8;
	return 9;
}

void get_tangerine_category(float deformation, char* category) {
	if (deformation <= 0.30f) strcpy(category, "EXTRA");
	else if (deformation <= 0.70f) strcpy(category, "I");
	else if (deformation <= 1.00f) strcpy(category, "II");
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

	// PASSO 1
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			int pos = y * width + x;

			if (datasrc[pos] == 255)
			{
				int A = (x > 0) ? labels[pos - 1] : 0;
				int B = (y > 0) ? labels[pos - width] : 0;
				int C = (x > 0 && y > 0) ? labels[pos - width - 1] : 0;
				int D = (x < width - 1 && y > 0) ? labels[pos - width + 1] : 0;

				int min = 999999;

				if (A) min = A;
				if (B && B < min) min = B;
				if (C && C < min) min = C;
				if (D && D < min) min = D;

				if (min == 999999)
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

	// PASSO 2
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

	// PASSO 3
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

			datadst[i] = new_labels[root];
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

	if (blobs == NULL)
		return NULL;

	for (i = 0; i < *nblobs; i++)
	{
		blobs[i].label = i + 1;
		blobs[i].area = 0;

		blobs[i].xmin = width;
		blobs[i].ymin = height;
		blobs[i].xmax = 0;
		blobs[i].ymax = 0;

		blobs[i].xc = 0;
		blobs[i].yc = 0;
		
		blobs[i].perimeter = 0.0f;
		blobs[i].diameter = 0.0f;
		blobs[i].caliber = 0;
		memset(blobs[i].category, 0, sizeof(blobs[i].category));
	}

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			int pos = y * width + x;
			int lbl = datadst[pos];

			if (lbl > 0)
			{
				int idx = lbl - 1;

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

	// Características Finais (CG, Perímetro, Calibre, Categoria)
	for (i = 0; i < *nblobs; i++)
	{
		if (blobs[i].area > 0)
		{
			blobs[i].xc /= blobs[i].area;
			blobs[i].yc /= blobs[i].area;

			blobs[i].perimeter = vc_calculate_perimeter(dst, blobs[i].label);

			// 280 pixeis = 55mm  -> 1 pixel = 55.0 / 280.0
			int width_px = blobs[i].xmax - blobs[i].xmin;
			int height_px = blobs[i].ymax - blobs[i].ymin;
			int max_px = (width_px > height_px) ? width_px : height_px;
			
			blobs[i].diameter = (float)max_px * (55.0f / 280.0f);
			blobs[i].caliber = get_tangerine_caliber(blobs[i].diameter);

			// Cálculo da Deformação (Area Real vs Area do Círculo Perfeito)
			float radius_px = max_px / 2.0f;
			float ideal_area = 3.14159265f * radius_px * radius_px;
			float deformation = fabs((float)blobs[i].area - ideal_area) / ideal_area * 100.0f;

			get_tangerine_category(deformation, blobs[i].category);
		}
	}

	return blobs;
}