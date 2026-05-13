//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//           INSTITUTO POLITÉCNICO DO CÁVADO E DO AVE
//                          2022/2023
//             ENGENHARIA DE SISTEMAS INFORMÁTICOS
//                    VISÃO POR COMPUTADOR
//
//             [  DUARTE DUQUE - dduque@ipca.pt  ]
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// Desabilita (no MSVC++) warnings de funções não seguras (fopen, sscanf, etc...)
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <malloc.h>
#include "vc.h"
#include <math.h>


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//            FUNÇÕES: ALOCAR E LIBERTAR UMA IMAGEM
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


// Alocar memória para uma imagem
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



// Libertar memória de uma imagem
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
//    FUNÇÕES: LEITURA E ESCRITA DE IMAGENS (PBM, PGM E PPM)
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
				// Numa imagem PBM:
				// 1 = Preto
				// 0 = Branco
				//*p |= (datauchar[pos] != 0) << (8 - countbits);

				// Na nossa imagem:
				// 1 = Branco
				// 0 = Preto
				*p = (datauchar[pos] == 0) << (8 - countbits);

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
				// Numa imagem PBM:
				// 1 = Preto
				// 0 = Branco
				//datauchar[pos] = (*p & (1 << (8 - countbits))) ? 1 : 0;

				// Na nossa imagem:
				// 1 = Branco
				// 0 = Preto
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

	// Abre o ficheiro
	if ((file = fopen(filename, "rb")) != NULL)
	{
		// Efectua a leitura do header
		netpbm_get_token(file, tok, sizeof(tok));

		if (strcmp(tok, "P4") == 0) { channels = 1; levels = 1; }	// Se PBM (Binary [0,1])
		else if (strcmp(tok, "P5") == 0) channels = 1;				// Se PGM (Gray [0,MAX(level,255)])
		else if (strcmp(tok, "P6") == 0) channels = 3;				// Se PPM (RGB [0,MAX(level,255)])
		else
		{
#ifdef VC_DEBUG
			printf("ERROR -> vc_read_image():\n\tFile is not a valid PBM, PGM or PPM file.\n\tBad magic number!\n");
#endif

			fclose(file);
			return NULL;
		}

		if (levels == 1) // PBM
		{
			if (sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &width) != 1 ||
				sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &height) != 1)
			{
#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tFile is not a valid PBM file.\n\tBad size!\n");
#endif

				fclose(file);
				return NULL;
			}

			// Aloca memória para imagem
			image = vc_image_new(width, height, channels, levels);
			if (image == NULL) return NULL;

			sizeofbinarydata = (image->width / 8 + ((image->width % 8) ? 1 : 0)) * image->height;
			tmp = (unsigned char*)malloc(sizeofbinarydata);
			if (tmp == NULL) return 0;

#ifdef VC_DEBUG
			printf("\nchannels=%d w=%d h=%d levels=%d\n", image->channels, image->width, image->height, levels);
#endif

			if ((v = fread(tmp, sizeof(unsigned char), sizeofbinarydata, file)) != sizeofbinarydata)
			{
#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tPremature EOF on file.\n");
#endif

				vc_image_free(image);
				fclose(file);
				free(tmp);
				return NULL;
			}

			bit_to_unsigned_char(tmp, image->data, image->width, image->height);

			free(tmp);
		}
		else // PGM ou PPM
		{
			if (sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &width) != 1 ||
				sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &height) != 1 ||
				sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &levels) != 1 || levels <= 0 || levels > 255)
			{
#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tFile is not a valid PGM or PPM file.\n\tBad size!\n");
#endif

				fclose(file);
				return NULL;
			}

			// Aloca memória para imagem
			image = vc_image_new(width, height, channels, levels);
			if (image == NULL) return NULL;

#ifdef VC_DEBUG
			printf("\nchannels=%d w=%d h=%d levels=%d\n", image->channels, image->width, image->height, levels);
#endif

			size = image->width * image->height * image->channels;

			if ((v = fread(image->data, sizeof(unsigned char), size, file)) != size)
			{
#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tPremature EOF on file.\n");
#endif

				vc_image_free(image);
				fclose(file);
				return NULL;
			}
		}

		fclose(file);
	}
	else
	{
#ifdef VC_DEBUG
		printf("ERROR -> vc_read_image():\n\tFile not found.\n");
#endif
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
			printf("Total = %ld\n", totalbytes);
			if (fwrite(tmp, sizeof(unsigned char), totalbytes, file) != totalbytes)
			{
#ifdef VC_DEBUG
				fprintf(stderr, "ERROR -> vc_read_image():\n\tError writing PBM, PGM or PPM file.\n");
#endif

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
#ifdef VC_DEBUG
				fprintf(stderr, "ERROR -> vc_read_image():\n\tError writing PBM, PGM or PPM file.\n");
#endif

				fclose(file);
				return 0;
			}
		}

		fclose(file);

		return 1;
	}

	return 0;
}


int vc_gray_negative(IVC* srcdst)
{
	unsigned char* data = (unsigned char*)srcdst->data;
	int width = srcdst->width;
	int height = srcdst->height;
	int bytesperline = srcdst->width * srcdst->channels;
	int channels = srcdst->channels;
	int x, y;
	long int pos;

	if ((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL)) return 0;
	if (channels != 1) return 0; // Apenas para imagens a cinzentos

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;
			data[pos] = 255 - data[pos];
		}
	}

	return 1;
}

int vc_rgb_to_gray(IVC* src, IVC* dst)
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
	float rf, gf, bf;

	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if ((src->width != dst->width) || (src->height != dst->height)) return 0;
	if ((src->channels != 3) || (dst->channels != 1)) return 0;

	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			pos_src = y * bytesperline_src + x * channels_src;
			pos_dst = y * bytesperline_dst + x * channels_dst;

			rf = (float)datasrc[pos_src];
			gf = (float)datasrc[pos_src + 1];
			bf = (float)datasrc[pos_src + 2];

			datadst[pos_dst] = (unsigned char)((rf * 0.299) + (gf * 0.587) + (bf * 0.114));
		}
	}

	return 1;
}

int vc_rgb_negative(IVC* srcdst)
{
	unsigned char* data = (unsigned char*)srcdst->data;
	int width = srcdst->width;
	int height = srcdst->height;
	int bytesperline = srcdst->width * srcdst->channels;
	int channels = srcdst->channels;
	int x, y;
	long int pos;
	if ((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL)) return 0;
	if (channels != 3) return 0; // Apenas para imagens RGB
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;
			data[pos] = 255 - data[pos];       // R
			data[pos + 1] = 255 - data[pos + 1]; // G
			data[pos + 2] = 255 - data[pos + 2]; // B
		}
	}
	return 1;
}

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

	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if ((src->width != dst->width) || (src->height != dst->height))
		return 0;
	if ((src->channels != 3) || (dst->channels != 1))
		return 0;

	// Intervalos válidos: H [0,360], S/V [0,100]
	if ((hmin < 0) || (hmin > 360) || (hmax < 0) || (hmax > 360))
		return 0;
	if ((smin < 0) || (smin > 100) || (smax < 0) || (smax > 100) || (smin > smax))
		return 0;
	if ((vmin < 0) || (vmin > 100) || (vmax < 0) || (vmax > 100) || (vmin > vmax))
		return 0;

	hmin_255 = (hmin * 255) / 360;
	hmax_255 = (hmax * 255) / 360;
	smin_255 = (smin * 255) / 100;
	smax_255 = (smax * 255) / 100;
	vmin_255 = (vmin * 255) / 100;
	vmax_255 = (vmax * 255) / 100;

	// Primeiro converte RGB para HSV
	IVC* hsv_image = vc_image_new(width, height, 3, 255);
	if (hsv_image == NULL) return 0;

	if (!vc_rgb_to_hsv(src, hsv_image)) {
		vc_image_free(hsv_image);
		return 0;
	}

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos_src = y * hsv_image->bytesperline + x * hsv_image->channels;
			pos_dst = y * bytesperline_dst + x * channels_dst;

			h = hsv_image->data[pos_src];
			s = hsv_image->data[pos_src + 1];
			v = hsv_image->data[pos_src + 2];

			// H é circular: se hmin > hmax, o intervalo cruza 0º
			if (hmin_255 <= hmax_255)
				in_h = (h >= hmin_255 && h <= hmax_255);
			else
				in_h = (h >= hmin_255 || h <= hmax_255);

			in_s = (s >= smin_255 && s <= smax_255);
			in_v = (v >= vmin_255 && v <= vmax_255);

			datadst[pos_dst] = (in_h && in_s && in_v) ? 255 : 0;
		}
	}

	vc_image_free(hsv_image);
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

	// Validações
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if ((src->width != dst->width) || (src->height != dst->height)) return 0;
	if ((src->channels != 3) || (dst->channels != 3)) return 0;

	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			pos_src = y * bytesperline_src + x * channels_src;
			pos_dst = y * bytesperline_dst + x * channels_dst;

			// Lê valores RGB
			r = datasrc[pos_src];
			g = datasrc[pos_src + 1];
			b = datasrc[pos_src + 2];

			// Normaliza para [0, 1]
			rf = r / 255.0f;
			gf = g / 255.0f;
			bf = b / 255.0f;

			// Calcula max e min
			float max = rf > gf ? (rf > bf ? rf : bf) : (gf > bf ? gf : bf);
			float min = rf < gf ? (rf < bf ? rf : bf) : (gf < bf ? gf : bf);

			// Calcula V (Value)
			v = max;

			// Calcula S (Saturation) e H (Hue)
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

			// Converte para o intervalo [0, 255]
			datadst[pos_dst] = (unsigned char)(h * 255);     // H: [0, 360] -> [0, 255]
			datadst[pos_dst + 1] = (unsigned char)(s * 255); // S: [0, 1] -> [0, 255]
			datadst[pos_dst + 2] = (unsigned char)(v * 255); // V: [0, 1] -> [0, 255]
		}
	}

	return 1;
}

int vc_scale_gray_to_color_palette(IVC* src, IVC* dst) {
	unsigned char* datasrc = NULL, * datadst = NULL;
	int width, height;
	int x, y;
	long int pos_src, pos_dst;
	int gray;
	int r, g, b;

	// Verificações
	if (src == NULL || dst == NULL)
		return 0;
	if (src->width != dst->width || src->height != dst->height)
		return 0;
	if (src->channels != 1 || dst->channels != 3)
		return 0;

	width = src->width;
	height = src->height;
	datasrc = (unsigned char*)src->data;
	datadst = (unsigned char*)dst->data;

	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			pos_src = y * width + x;
			pos_dst = (y * width + x) * 3;

			gray = datasrc[pos_src];

			// Palete: Azul -> Ciano -> Verde -> Amarelo -> Vermelho
			if (gray < 64) {
				// Azul -> Ciano
				b = 255;
				g = gray * 4;
				r = 0;
			}
			else if (gray < 128) {
				// Ciano -> Verde
				b = 255 - ((gray - 64) * 4);
				g = 255;
				r = 0;
			}
			else if (gray < 192) {
				// Verde -> Amarelo
				b = 0;
				g = 255;
				r = (gray - 128) * 4;
			}
			else {
				// Amarelo -> Vermelho
				b = 0;
				g = 255 - ((gray - 192) * 4);
				r = 255;
			}

			// Guardar em RGB
			datadst[pos_dst] = (unsigned char)r;
			datadst[pos_dst + 1] = (unsigned char)g;
			datadst[pos_dst + 2] = (unsigned char)b;
		}
	}

	return 1;
}

int vc_gray_to_binary(IVC* src, IVC* dst, int threshold) {
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
	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if ((src->width != dst->width) || (src->height != dst->height))
		return 0;
	if ((src->channels != 1) || (dst->channels != 1))
		return 0;
	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			pos_src = y * bytesperline_src + x * channels_src;
			pos_dst = y * bytesperline_dst + x * channels_dst;
			datadst[pos_dst] = (datasrc[pos_src] >= threshold) ? 255 : 0;
		}
	}
	return 1;
}



//usar calculo de threshold automático = 1/2 * (valor mínimo + valor máximo) da imagem a cinzentos com kernel para calcular na vizinhança
int vc_gray_to_binary_midpoint(IVC* src, IVC* dst, int kernel) {
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int bytesperline_src = src->width * src->channels;
	int bytesperline_dst = dst->width * dst->channels;
	int channels_src = src->channels;
	int channels_dst = dst->channels;
	int width = src->width;
	int height = src->height;
	int x, y, kx, ky;
	long int pos_src, pos_dst;
	long int pos_k;
	unsigned char vmin, vmax;
	unsigned char threshold;
	int offset = kernel / 2;

	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if ((src->width != dst->width) || (src->height != dst->height)) return 0;
	if ((src->channels != 1) || (dst->channels != 1)) return 0;

	// O kernel deve ser um número ímpar e pelo menos 3 (ex: 3, 5, 7, 9...)
	if (kernel < 3 || kernel % 2 == 0) return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos_src = y * bytesperline_src + x * channels_src;
			pos_dst = y * bytesperline_dst + x * channels_dst;

			vmin = 255;
			vmax = 0;

			// Percorrer a vizinhança (kernel)
			for (ky = -offset; ky <= offset; ky++)
			{
				for (kx = -offset; kx <= offset; kx++)
				{
					// Verificar limites da imagem (bordas)
					if ((y + ky) >= 0 && (y + ky) < height && (x + kx) >= 0 && (x + kx) < width)
					{
						pos_k = (y + ky) * bytesperline_src + (x + kx) * channels_src;

						if (datasrc[pos_k] < vmin) vmin = datasrc[pos_k];
						if (datasrc[pos_k] > vmax) vmax = datasrc[pos_k];
					}
				}
			}

			// Calcular threshold com base na vizinhança
			threshold = (unsigned char)((vmin + vmax) / 2);

			// Atribuir novo valor
			if (datasrc[pos_src] > threshold)
				datadst[pos_dst] = 255; // Branco
			else
				datadst[pos_dst] = 0;   // Preto
		}
	}

	return 1;
}

// Dilatação binária
int vc_binary_dilate(IVC* src, IVC* dst, int kernel) {
	int x, y, kx, ky;
	int xmin, xmax, ymin, ymax;
	long int pos, posk;
	int max;
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;

	// Verificação de erros
	if ((src == NULL) || (dst == NULL))
		return 0;
	if ((src->width <= 0) || (src->height <= 0) || (src->channels != 1) ||
		(dst->width <= 0) || (dst->height <= 0) || (dst->channels != 1))
		return 0;
	if ((src->width != dst->width) || (src->height != dst->height))
		return 0;
	if ((src->data == NULL) || (dst->data == NULL))
		return 0;

	// Verificação do kernel
	if (kernel < 3)
		kernel = 3;
	if ((kernel % 2) == 0)
		kernel++;

	for (y = 0; y < src->height; y++) {
		for (x = 0; x < src->width; x++) {
			xmin = x - kernel / 2;
			xmax = x + kernel / 2;
			ymin = y - kernel / 2;
			ymax = y + kernel / 2;

			if (xmin < 0)
				xmin = 0;
			if (ymin < 0)
				ymin = 0;
			if (xmax >= src->width)
				xmax = src->width - 1;
			if (ymax >= src->height)
				ymax = src->height - 1;

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

// Erosão binária
int vc_binary_erode(IVC* src, IVC* dst, int kernel) {
	int x, y, kx, ky;
	int xmin, xmax, ymin, ymax;
	long int pos, posk;
	int min;
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;

	// Verificação de erros
	if ((src == NULL) || (dst == NULL))
		return 0;
	if ((src->width <= 0) || (src->height <= 0) || (src->channels != 1) ||
		(dst->width <= 0) || (dst->height <= 0) || (dst->channels != 1))
		return 0;
	if ((src->width != dst->width) || (src->height != dst->height))
		return 0;
	if ((src->data == NULL) || (dst->data == NULL))
		return 0;

	// Verificação do kernel
	if (kernel < 3)
		kernel = 3;
	if ((kernel % 2) == 0)
		kernel++;

	for (y = 0; y < src->height; y++) {
		for (x = 0; x < src->width; x++) {
			xmin = x - kernel / 2;
			xmax = x + kernel / 2;
			ymin = y - kernel / 2;
			ymax = y + kernel / 2;

			if (xmin < 0)
				xmin = 0;
			if (ymin < 0)
				ymin = 0;
			if (xmax >= src->width)
				xmax = src->width - 1;
			if (ymax >= src->height)
				ymax = src->height - 1;

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

	// PASSO 1: atribuição inicial de labels + equivalências
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			int pos = y * width + x;

			if (datasrc[pos] == 255)
			{
				int A = (x > 0) ? labels[pos - 1] : 0;                 // esquerda
				int B = (y > 0) ? labels[pos - width] : 0;            // cima
				int C = (x > 0 && y > 0) ? labels[pos - width - 1] : 0; // diag esq
				int D = (x < width - 1 && y > 0) ? labels[pos - width + 1] : 0; // diag dir

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

					// guardar equivalências
					if (A && A != min) equiv[A] = min;
					if (B && B != min) equiv[B] = min;
					if (C && C != min) equiv[C] = min;
					if (D && D != min) equiv[D] = min;
				}
			}
		}
	}

	// PASSO 2: resolver equivalências (flatten)
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

	// PASSO 3: relabel + contagem
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

	// Inicializar
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
	}

	// Acumular dados
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

	// Calcular centro de gravidade
	for (i = 0; i < *nblobs; i++)
	{
		if (blobs[i].area > 0)
		{
			blobs[i].xc /= blobs[i].area;
			blobs[i].yc /= blobs[i].area;
		}
	}

	return blobs;
}

int vc_gray_lowpass_mean_filter(IVC* src, IVC* dst, int kernel) {
	int x, y, kx, ky;
	int xmin, xmax, ymin, ymax;
	long int pos, posk;
	int sum;
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	// Verificação de erros
	if ((src == NULL) || (dst == NULL))
		return 0;
	if ((src->width <= 0) || (src->height <= 0) || (src->channels != 1) ||
		(dst->width <= 0) || (dst->height <= 0) || (dst->channels != 1))
		return 0;
	if ((src->width != dst->width) || (src->height != dst->height))
		return 0;
	if ((src->data == NULL) || (dst->data == NULL))
		return 0;
	// Verificação do kernel
	if (kernel < 3)
		kernel = 3;
	if ((kernel % 2) == 0)
		kernel++;
	for (y = 0; y < src->height; y++) {
		for (x = 0; x < src->width; x++) {
			xmin = x - kernel / 2;
			xmax = x + kernel / 2;
			ymin = y - kernel / 2;
			ymax = y + kernel / 2;
			if (xmin < 0)
				xmin = 0;
			if (ymin < 0)
				ymin = 0;
			if (xmax >= src->width)
				xmax = src->width - 1;
			if (ymax >= src->height)
				ymax = src->height - 1;
			sum = 0;
			for (ky = ymin; ky <= ymax; ky++) {
				for (kx = xmin; kx <= xmax; kx++) {
					posk = ky * src->bytesperline + kx * src->channels;
					sum += datasrc[posk];
				}
			}
			pos = y * src->bytesperline + x * src->channels;
			datadst[pos] = sum / ((xmax - xmin + 1) * (ymax - ymin + 1));
		}
	}
	return 1;
}

// Função de comparação para qsort (ordenação de unsigned char)
int compare_unsigned_char(const void* a, const void* b)
{
	unsigned char ua = *(const unsigned char*)a;
	unsigned char ub = *(const unsigned char*)b;
	return (ua > ub) - (ua < ub);
}

int vc_gray_lowpass_median_filter(IVC* src, IVC* dst, int kernel) {
	int x, y, kx, ky;
	int xmin, xmax, ymin, ymax;
	long int pos, posk;
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	// Verificação de erros
	if ((src == NULL) || (dst == NULL))
		return 0;
	if ((src->width <= 0) || (src->height <= 0) || (src->channels != 1) ||
		(dst->width <= 0) || (dst->height <= 0) || (dst->channels != 1))
		return 0;
	if ((src->width != dst->width) || (src->height != dst->height))
		return 0;
	if ((src->data == NULL) || (dst->data == NULL))
		return 0;
	// Verificação do kernel
	if (kernel < 3)
		kernel = 3;
	if ((kernel % 2) == 0)
		kernel++;
	unsigned char* window = (unsigned char*)malloc(kernel * kernel * sizeof(unsigned char));
	if (window == NULL)
		return 0;
	for (y = 0; y < src->height; y++) {
		for (x = 0; x < src->width; x++) {
			xmin = x - kernel / 2;
			xmax = x + kernel / 2;
			ymin = y - kernel / 2;
			ymax = y + kernel / 2;
			if (xmin < 0)
				xmin = 0;
			if (ymin < 0)
				ymin = 0;
			if (xmax >= src->width)
				xmax = src->width - 1;
			if (ymax >= src->height)
				ymax = src->height - 1;
			int idx = 0;
			for (ky = ymin; ky <= ymax; ky++) {
				for (kx = xmin; kx <= xmax; kx++) {
					posk = ky * src->bytesperline + kx * src->channels;
					window[idx++] = datasrc[posk];
				}
			}
			qsort(window, idx, sizeof(unsigned char), compare_unsigned_char);
			pos = y * src->bytesperline + x * src->channels;
			datadst[pos] = window[idx / 2];
		}
	}
	free(window);
	return 1;
}

