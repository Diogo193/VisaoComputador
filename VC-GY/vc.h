#pragma once
#ifndef _VC_H_
#define _VC_H_
#define VC_DEBUG
#define VC_PIXEL_TO_MM (55.0f / 280.0f)
typedef struct {
    unsigned char* data;    
    int width, height;      
    int channels;           
    int levels;             
    int bytesperline;       
} IVC;

typedef struct {
    int   label;            
    int   area;             
    float perimeter;        
    int   xmin, ymin;       
    int   xmax, ymax;       
    int   xc, yc;           
    float diameter_mm;      
    int   caliber;          
    float deformation;     
    char  category[8];     
} OVC;
IVC* vc_image_new(int width, int height, int channels, int levels);
IVC* vc_image_free(IVC* image);
int vc_bgr_to_hsv(IVC* src, IVC* dst);
int vc_hsv_segmentation(IVC* src, IVC* dst,
    int hmin, int hmax,
    int smin, int smax,
    int vmin, int vmax);
int vc_binary_erode(IVC* src, IVC* dst, int kernel);
int vc_binary_dilate(IVC* src, IVC* dst, int kernel);
int vc_binary_open(IVC* src, IVC* dst, int kernel);
int vc_binary_close(IVC* src, IVC* dst, int kernel);
OVC* vc_binary_blob_labelling(IVC* src, IVC* dst, int* nblobs);
int vc_binary_blob_info(IVC* src, OVC* blobs, int nblobs);
float vc_calculate_perimeter(IVC* src, int label);
int vc_get_caliber(float diameter_mm);
float vc_compute_deformation(OVC* blob);
void vc_get_category(float deformation, char* category);
int vc_crosses_line(int prev_y, int curr_y, int line_y);
int vc_draw_hline(IVC* src, int y, int r, int g, int b);
int vc_draw_rectangle(IVC* src, int xmin, int ymin, int xmax, int ymax, int r, int g, int b);

#endif