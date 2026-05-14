#pragma once
#ifndef _VC_H_
#define _VC_H_

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//            INSTITUTO POLITÉCNICO DO CÁVADO E DO AVE
//              ENGENHARIA DE SISTEMAS INFORMÁTICOS
//                    VISÃO POR COMPUTADOR
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// Modo de depuração: ativa prints de diagnóstico em vc.c
#define VC_DEBUG

//--------------------------------------------------------------
// ESCALA DE CONVERSÃO
// 280 píxeis correspondem a 55 mm na cena real.
// Usado para converter a largura da bounding box (px) em mm.
//--------------------------------------------------------------
#define VC_PIXEL_TO_MM (55.0f / 280.0f)

//--------------------------------------------------------------
// NORMAS DE CALIBRE – CITRINOS (Regulamento EU)
// Diâmetro mínimo e máximo por número de calibre (em mm).
// Calibre  1 : >= 92 mm
// Calibre  2 : [87, 92[
// Calibre  3 : [84, 87[
// Calibre  4 : [81, 84[
// Calibre  5 : [77, 81[
// Calibre  6 : [73, 77[
// Calibre  7 : [70, 73[
// Calibre  8 : [67, 70[
// Calibre  9 : [63, 67[
// Calibre 10 : [56, 63[
// Calibre 11 : [53, 56[
// Calibre 12 : [51, 53[
// Calibre 13 : [49, 51[
// Calibre 14 : < 49 mm (fora de norma – ignorar)
//--------------------------------------------------------------

//==============================================================
// ESTRUTURA DE IMAGEM
//==============================================================
typedef struct {
    unsigned char* data;    // Buffer de píxeis
    int width, height;      // Dimensões em píxeis
    int channels;           // 1 = binário/cinzento; 3 = BGR/HSV
    int levels;             // 1 = binário; 255 = escala de cinzento/cor
    int bytesperline;       // width * channels
} IVC;

//==============================================================
// ESTRUTURA DE UM BLOB (LARANJA DETETADA)
//==============================================================
typedef struct {
    int   label;            // Etiqueta atribuída pelo algoritmo CCL
    int   area;             // Área total em píxeis
    float perimeter;        // Perímetro em píxeis (contagem de fronteira)
    int   xmin, ymin;       // Canto superior-esquerdo da bounding box
    int   xmax, ymax;       // Canto inferior-direito da bounding box
    int   xc, yc;           // Centro de gravidade (centróide)
    float diameter_mm;      // Diâmetro calculado a partir da bbox, em mm
    int   caliber;          // Número de calibre (1–13; 0 = fora de norma)
    float deformation;      // Rácio de deformação [0,1]; 0 = círculo perfeito
    char  category[8];      // Categoria: "Extra", "I", "II", "III" ou "N/A"
} OVC;

//==============================================================
// GESTÃO DE MEMÓRIA
//==============================================================

// Aloca uma nova imagem com as dimensões e propriedades indicadas.
// Devolve NULL em caso de falha.
IVC* vc_image_new(int width, int height, int channels, int levels);

// Liberta a memória associada à imagem e devolve NULL.
IVC* vc_image_free(IVC* image);

//==============================================================
// CONVERSÃO DE ESPAÇO DE COR E SEGMENTAÇÃO (implementação manual)
//==============================================================

// Converte uma imagem BGR (3 canais, [0,255]) para HSV.
// H: [0,179]  S: [0,255]  V: [0,255]  (convenção OpenCV)
// src e dst devem ter as mesmas dimensões e 3 canais.
int vc_bgr_to_hsv(IVC* src, IVC* dst);

// Segmenta a imagem HSV pelos intervalos fornecidos e produz uma
// imagem binária (1 canal, levels=1): 255 = laranja; 0 = fundo.
int vc_hsv_segmentation(IVC* src, IVC* dst,
    int hmin, int hmax,
    int smin, int smax,
    int vmin, int vmax);

//==============================================================
// MORFOLOGIA MATEMÁTICA (implementação manual)
// kernel: lado do elemento estruturante quadrado (ex.: 3, 5, 7)
//==============================================================

// Erosão binária: remove píxeis de fronteira.
int vc_binary_erode(IVC* src, IVC* dst, int kernel);

// Dilatação binária: expande regiões brancas.
int vc_binary_dilate(IVC* src, IVC* dst, int kernel);

// Abertura (erosão seguida de dilatação): remove ruído pequeno.
int vc_binary_open(IVC* src, IVC* dst, int kernel);

// Fecho (dilatação seguida de erosão): preenche buracos internos.
int vc_binary_close(IVC* src, IVC* dst, int kernel);

//==============================================================
// ANÁLISE DE BLOBS – ETIQUETAGEM DE COMPONENTES LIGADAS (CCL)
//==============================================================

// Etiqueta cada componente ligada da imagem binária src.
// dst recebe os valores de etiqueta por píxel (imagem de rótulos).
// *nblobs é preenchido com o número de blobs encontrados.
// Devolve um array de OVC com nblobs entradas (ou NULL em erro).
// O chamador é responsável por libertar a memória devolvida.
OVC* vc_binary_blob_labelling(IVC* src, IVC* dst, int* nblobs);

//==============================================================
// EXTRAÇÃO DE CARACTERÍSTICAS DOS BLOBS
//==============================================================

// Percorre a imagem etiquetada (src) e preenche, para cada blob,
// os campos: area, xmin, ymin, xmax, ymax, xc, yc.
// Deve ser chamado após vc_binary_blob_labelling.
int vc_binary_blob_info(IVC* src, OVC* blobs, int nblobs);

// Conta os píxeis de fronteira do blob com a etiqueta indicada
// na imagem etiquetada src, e devolve o perímetro como float.
float vc_calculate_perimeter(IVC* src, int label);

//==============================================================
// CLASSIFICAÇÃO: CALIBRE E CATEGORIA
//==============================================================

// A partir do diâmetro em mm, devolve o número de calibre (1–13).
// Devolve 0 se o diâmetro estiver fora das normas de qualidade.
int vc_get_caliber(float diameter_mm);

// Calcula o rácio de deformação de um blob.
// Compara a área real com a área de um círculo ideal inscrito na
// bounding box: deformation = 1 - (area_blob / area_circulo_ideal).
// Devolve um valor em [0, 1]: 0 = círculo perfeito.
float vc_compute_deformation(OVC* blob);

// Preenche a string category com base no rácio de deformação:
//   deformation < 0.05  -> "Extra"
//   deformation < 0.15  -> "I"
//   deformation < 0.30  -> "II"
//   deformation >= 0.30 -> "III"
// category deve ter pelo menos 8 bytes.
void vc_get_category(float deformation, char* category);

//==============================================================
// CONTAGEM POR CRUZAMENTO DE LINHA
//==============================================================

// Devolve 1 se o centróide passou da posição prev_y para curr_y
// cruzando a linha virtual em line_y (sentido descendente), 0 caso
// contrário. Usado para contar cada laranja uma única vez.
int vc_crosses_line(int prev_y, int curr_y, int line_y);

#endif // _VC_H_