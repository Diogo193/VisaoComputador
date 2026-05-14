#pragma once
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//           INSTITUTO POLITÉCNICO DO CÁVADO E DO AVE
//                          2022/2023
//             ENGENHARIA DE SISTEMAS INFORMÁTICOS
//                    VISÃO POR COMPUTADOR
//
//             [  DUARTE DUQUE - dduque@ipca.pt  ]
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


#define VC_DEBUG


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                   ESTRUTURA DE UMA IMAGEM
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


typedef struct {
	unsigned char* data;
	int width, height;
	int channels;			// Binário/Cinzentos=1; RGB=3
	int levels;				// Binário=1; Cinzentos [1,255]; RGB [1,255]
	int bytesperline;		// width * channels
} IVC;

typedef struct
{
	int label;       // Etiqueta do blob
	int area;        // Número de pixels (Área)
	float perimeter; // Perímetro

	int xmin, ymin;  // Bounding Box
	int xmax, ymax;

	int xc, yc;      // Centro de gravidade

	float diameter;  // Diâmetro em mm
	int caliber;     // Calibre (número)
	char category[20]; // Categoria (Extra, I, II, III)

} OVC;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                    PROTÓTIPOS DE FUNÇÕES
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// FUNÇÕES: ALOCAR E LIBERTAR UMA IMAGEM
IVC* vc_image_new(int width, int height, int channels, int levels);
IVC* vc_image_free(IVC* image);

// FUNÇÕES: LEITURA E ESCRITA DE IMAGENS (PBM, PGM E PPM)
IVC* vc_read_image(char* filename);
int vc_write_image(char* filename, IVC* image);
int vc_hsv_segmentation(IVC* src, IVC* dst, int hmin, int hmax, int smin, int smax, int vmin, int vmax);
int vc_rgb_to_hsv(IVC* src, IVC* dst);
int vc_binary_erode(IVC* src, IVC* dst, int kernel);
int vc_binary_dilate(IVC* src, IVC* dst, int kernel);

// Processamento Morfológico
int vc_binary_open(IVC* src, IVC* dst, int size);
int vc_binary_close(IVC* src, IVC* dst, int size);

// Etiquetagem
OVC* vc_binary_blob_labelling(IVC* src, IVC* dst, int* nblobs);

// Extração de Características
int vc_binary_blob_info(IVC* src, OVC* blobs, int nblobs);

// Cálculo do Perímetro
float vc_calculate_perimeter(IVC* src, int label);

