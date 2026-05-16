#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>

extern "C" {
#include "vc.h"
}

// Estrutura para rastreamento simples e contagem por linha
struct LaranjaRastreada {
	int id;
	int xc, yc;
	int prev_y;
	int frame_ultima_vez;
	bool contada;
};

void vc_timer(void) {
	static bool running = false;
	static std::chrono::steady_clock::time_point previousTime = std::chrono::steady_clock::now();

	if (!running) {
		running = true;
	}
	else {
		std::chrono::steady_clock::time_point currentTime = std::chrono::steady_clock::now();
		std::chrono::steady_clock::duration elapsedTime = currentTime - previousTime;

		std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(elapsedTime);
		double nseconds = time_span.count();

		std::cout << "Tempo decorrido: " << nseconds << " segundos" << std::endl;
		std::cout << "Pressione qualquer tecla para continuar...\n";
		std::cin.get();
	}
}

// Desenha o centro de gravidade manualmente na matriz de píxeis
void desenha_cruz_manual(cv::Mat& frame, int xc, int yc, int tamanho) {
	for (int i = -tamanho; i <= tamanho; i++) {
		if (xc + i >= 0 && xc + i < frame.cols && yc >= 0 && yc < frame.rows) {
			int pos = (yc * frame.step) + ((xc + i) * 3);
			frame.data[pos] = 0;
			frame.data[pos + 1] = 0;
			frame.data[pos + 2] = 255;
		}
		if (yc + i >= 0 && yc + i < frame.rows && xc >= 0 && xc < frame.cols) {
			int pos = ((yc + i) * frame.step) + (xc * 3);
			frame.data[pos] = 0;
			frame.data[pos + 1] = 0;
			frame.data[pos + 2] = 255;
		}
	}
}

int main(void) {
	char videofile[20] = "video.avi";
	cv::VideoCapture capture;
	struct {
		int width, height;
		int ntotalframes;
		int fps;
		int nframe;
	} video;

	int key = 0;

	capture.open(videofile);

	if (!capture.isOpened()) {
		std::cerr << "Erro ao abrir o ficheiro de vídeo!\n";
		return 1;
	}

	video.ntotalframes = (int)capture.get(cv::CAP_PROP_FRAME_COUNT);
	video.fps = (int)capture.get(cv::CAP_PROP_FPS);
	video.width = (int)capture.get(cv::CAP_PROP_FRAME_WIDTH);
	video.height = (int)capture.get(cv::CAP_PROP_FRAME_HEIGHT);

	cv::namedWindow("VC - VISUALIZACAO", cv::WINDOW_AUTOSIZE);

	vc_timer();

	IVC* image_bgr = vc_image_new(video.width, video.height, 3, 255);
	IVC* image_hsv = vc_image_new(video.width, video.height, 3, 255);
	IVC* image_bin = vc_image_new(video.width, video.height, 1, 1);
	IVC* image_labels = vc_image_new(video.width, video.height, 1, 255);

	int contador_total = 0;
	std::vector<LaranjaRastreada> laranjas_registadas;
	int proximo_id = 1;

	const int line_y = (int)(video.height * 0.65f);

	cv::Mat frame;
	cv::Mat mask_seg(video.height, video.width, CV_8UC1, image_bin->data);
	cv::Mat mask_morf(video.height, video.width, CV_8UC1, image_bin->data);

	while (key != 'q') {
		capture.read(frame);
		if (frame.empty()) break;

		video.nframe = (int)capture.get(cv::CAP_PROP_POS_FRAMES);

		memcpy(image_bgr->data, frame.data, video.width * video.height * 3);

		// 1. BGR -> HSV
		vc_bgr_to_hsv(image_bgr, image_hsv);

		// 2. Segmentação (HSV em [0..179], [0..255], [0..255])
		vc_hsv_segmentation(image_hsv, image_bin, 5, 20, 150, 255, 80, 255);

		// Visualizar segmentaçãoAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
		cv::imshow("VC - SEGMENTACAO", mask_seg);

		// 3. Morfologia
		vc_binary_close(image_bin, image_bin, 5);
		vc_binary_open(image_bin, image_bin, 5);

		// Visualizar após morfologiAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
		cv::imshow("VC - MORFOLOGIA", mask_morf);

		// 4. CCL + info
		int nblobs = 0;
		OVC* blobs = vc_binary_blob_labelling(image_bin, image_labels, &nblobs);

		if (blobs != NULL) {
			vc_binary_blob_info(image_labels, blobs, nblobs);
		}

		int laranjas_na_frame = 0;

		if (blobs != NULL) {
			for (int i = 0; i < nblobs; i++) {
				int width_px = (blobs[i].xmax - blobs[i].xmin) + 1;
				int height_px = (blobs[i].ymax - blobs[i].ymin) + 1;
				int bbox_area = width_px * height_px;
				float fill = (bbox_area > 0) ? ((float)blobs[i].area / (float)bbox_area) : 0.0f;

				float circularity = (blobs[i].perimeter > 0.0f)
					? (4.0f * 3.14159265358979323846f * (float)blobs[i].area) / (blobs[i].perimeter * blobs[i].perimeter)
					: 0.0f;

				if (blobs[i].area > 1500 &&
					blobs[i].caliber > 0 &&
					blobs[i].deformation < 0.30f &&
					fill > 0.60f &&
					circularity > 0.60f) {
					laranjas_na_frame++;

					// RASTREAMENTO SIMPLES
					int min_dist = 999999;
					int best_index = -1;

					for (size_t j = 0; j < laranjas_registadas.size(); j++) {
						int dx = laranjas_registadas[j].xc - blobs[i].xc;
						int dy = laranjas_registadas[j].yc - blobs[i].yc;
						int dist = (dx * dx) + (dy * dy);

						if (dist < min_dist) {
							min_dist = dist;
							best_index = (int)j;
						}
					}

					int id_atual = -1;

					if (min_dist < 4000 && best_index != -1) {
						laranjas_registadas[best_index].prev_y = laranjas_registadas[best_index].yc;
						laranjas_registadas[best_index].xc = blobs[i].xc;
						laranjas_registadas[best_index].yc = blobs[i].yc;
						laranjas_registadas[best_index].frame_ultima_vez = video.nframe;
						id_atual = laranjas_registadas[best_index].id;

						if (!laranjas_registadas[best_index].contada &&
							vc_crosses_line(laranjas_registadas[best_index].prev_y, laranjas_registadas[best_index].yc, line_y)) {
							contador_total++;
							laranjas_registadas[best_index].contada = true;
						}
					}
					else {
						int novo_id = proximo_id++;
						laranjas_registadas.push_back({ novo_id, blobs[i].xc, blobs[i].yc, blobs[i].yc, video.nframe, false });
						id_atual = novo_id;
					}

					// VISUALIZAÇÃO
					cv::rectangle(frame, cv::Point(blobs[i].xmin, blobs[i].ymin),
						cv::Point(blobs[i].xmax, blobs[i].ymax), cv::Scalar(255, 0, 0), 2);

					desenha_cruz_manual(frame, blobs[i].xc, blobs[i].yc, 5);

					char texto[200];

					sprintf(texto, "ID:%d LABEL:%d AREA:%d PERIMETRO:%.0f", id_atual, blobs[i].label, blobs[i].area, blobs[i].perimeter);
					cv::putText(frame, texto, cv::Point(blobs[i].xmin, blobs[i].ymin - 25),
						cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(255, 255, 255), 1);

					sprintf(texto, "CALIBRE:%d DIAMETRO:%.0fmm", blobs[i].caliber, blobs[i].diameter_mm);
					cv::putText(frame, texto, cv::Point(blobs[i].xmin, blobs[i].ymin - 12),
						cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(255, 255, 255), 1);

					sprintf(texto, "CATEGORIA:%s", blobs[i].category);
					cv::putText(frame, texto, cv::Point(blobs[i].xmin, blobs[i].ymax + 15),
						cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(0, 255, 255), 1);
				}
			}
			free(blobs);
		}

		// Limpeza de rastros antigos
		laranjas_registadas.erase(
			std::remove_if(laranjas_registadas.begin(), laranjas_registadas.end(),
				[&](const LaranjaRastreada& l) { return (video.nframe - l.frame_ultima_vez) > 15; }),
			laranjas_registadas.end()
		);

		// Linha virtual (3/3 OpenCV permitido)
		cv::line(frame, cv::Point(0, line_y), cv::Point(video.width, line_y), cv::Scalar(0, 0, 255), 2);

		// Info global
		char info_global[100];
		sprintf(info_global, "LARANJAS NA FRAME: %d | CONTAGEM TOTAL: %d", laranjas_na_frame, contador_total);
		cv::putText(frame, info_global, cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 0, 0), 3);
		cv::putText(frame, info_global, cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 0), 1);

		cv::imshow("VC - VISUALIZACAO", frame);
		key = cv::waitKey(1);
	}

	vc_image_free(image_bgr);
	vc_image_free(image_hsv);
	vc_image_free(image_bin);
	vc_image_free(image_labels);

	vc_timer();
	cv::destroyWindow("VC - VISUALIZACAO");
	capture.release();

	return 0;
}