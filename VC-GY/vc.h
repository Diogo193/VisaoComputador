#pragma once

#ifndef _VC_H_
#define _VC_H_

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//            INSTITUTO POLITÉCNICO DO CÁVADO E DO AVE
//              ENGENHARIA DE SISTEMAS INFORMÁTICOS
//                    VISÃO POR COMPUTADOR
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#define VC_DEBUG

// Escala Real: 280 píxeis = 55 mm
#define VC_PIXEL_TO_MM (55.0f / 280.0f)

// ESTRUTURA DE UMA IMAGEM
typedef struct {
    unsigned char* data;
    int width, height;
    int channels;      // Binário/Cinzentos=1; RGB/BGR=3
    int levels;        // Binário=1; Cor/Cinzas=255
    int bytesperline;  // width * channels
} IVC;

// ESTRUTURA DE UM BLOB (OBJETO DETETADO)
typedef struct {
    int label;          // Etiqueta
    int area;           // Área em píxeis
    float perimeter;    // Perímetro
    int xmin, ymin;     // Bounding Box
    int xmax, ymax;
    int xc, yc;         // Centro de Gravidade
    float diameter;     // Diâmetro em mm
    int caliber;        // Número do calibre
    char category[20];  // Categoria (Extra, I, II, III)
} OVC;

// PROTÓTIPOS DE FUNÇÕES

// Gestão de Memória
IVC* vc_image_new(int width, int height, int channels, int levels);
IVC* vc_image_free(IVC* image);

// Processamento de Cor e Segmentação (Manual)
int vc_bgr_to_hsv(IVC* src, IVC* dst);
int vc_hsv_segmentation(IVC* src, IVC* dst, int hmin, int hmax, int smin, int smax, int vmin, int vmax);

// Morfologia Matemática (Manual)
int vc_binary_erode(IVC* src, IVC* dst, int kernel);
int vc_binary_dilate(IVC* src, IVC* dst, int kernel);
int vc_binary_open(IVC* src, IVC* dst, int kernel);
int vc_binary_close(IVC* src, IVC* dst, int kernel);

// Análise de Blobs (Etiquetagem)
OVC* vc_binary_blob_labelling(IVC* src, IVC* dst, int* nblobs);

// Extração de Características
// --> ADICIONADA: Preenche a estrutura OVC (área, xmin, xmax, ymin, ymax, xc, yc) a partir da imagem etiquetada
int vc_binary_blob_info(IVC* src, OVC* blobs, int nblobs);
float vc_calculate_perimeter(IVC* src, int label);

// Classificação (Calibre/Categoria)
int vc_get_caliber(float diameter_mm);
void vc_get_category(float deformation, char* category);

// Contagem por Cruzamento de Linha
int vc_crosses_line(int prev_y, int curr_y, int line_y);

// Desenho Manual (Sem OpenCV)
int vc_draw_boundary_box(IVC* src, int xmin, int ymin, int xmax, int ymax, int r, int g, int b);
int vc_draw_line(IVC* src, int y_pos, int r, int g, int b);

#endif