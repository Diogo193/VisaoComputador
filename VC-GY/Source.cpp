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

// Estrutura para ajudar na Contagem Total (Rastreamento Simples)
struct LaranjaRastreada {
	int id;
	int xc, yc;
	int frame_ultima_vez;
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

// Função auxiliar para desenhar o centro de gravidade diretamente na matriz de píxeis (Poupando limite OpenCV)
void desenha_cruz_manual(cv::Mat& frame, int xc, int yc, int tamanho) {
	for (int i = -tamanho; i <= tamanho; i++) {
		if (xc + i >= 0 && xc + i < frame.cols && yc >= 0 && yc < frame.rows) {
			int pos = (yc * frame.step) + ((xc + i) * 3);
			frame.data[pos] = 0;       // B
			frame.data[pos + 1] = 0;   // G
			frame.data[pos + 2] = 255; // R (Vermelho)
		}
		if (yc + i >= 0 && yc + i < frame.rows && xc >= 0 && xc < frame.cols) {
			int pos = ((yc + i) * frame.step) + (xc * 3);
			frame.data[pos] = 0;       // B
			frame.data[pos + 1] = 0;   // G
			frame.data[pos + 2] = 255; // R (Vermelho)
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

	std::string str;
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

	IVC* image_rgb = vc_image_new(video.width, video.height, 3, 255);
	IVC* image_hsv = vc_image_new(video.width, video.height, 3, 255);
	IVC* image_bin = vc_image_new(video.width, video.height, 1, 255);

	int contador_total = 0;
	std::vector<LaranjaRastreada> laranjas_registadas;
	int proximo_id = 1;

	cv::Mat frame;

	while (key != 'q') {
		capture.read(frame);
		if (frame.empty()) break;

		video.nframe = (int)capture.get(cv::CAP_PROP_POS_FRAMES);

		// Copia frame para IVC
		memcpy(image_rgb->data, frame.data, video.width * video.height * 3);

		// 1. Converter RGB para HSV
		vc_rgb_to_hsv(image_rgb, image_hsv);

		// 2. Segmentação da Laranja (Ajusta o H, S e V conforme a iluminação do teu vídeo)
		// Laranjas costumam estar no Hue [10, 40], Saturation > 50, Value > 50
		vc_hsv_segmentation(image_hsv, image_bin, 10, 40, 50, 100, 50, 100);

		// 3. Limpeza Morfológica (Fecha buracos e remove ruído pequeno)
		vc_binary_close(image_bin, image_bin, 5);
		vc_binary_open(image_bin, image_bin, 5);

		// 4. Etiquetagem e Extração de Características
		int nblobs = 0;
		OVC* blobs = vc_binary_blob_labelling(image_bin, image_bin, &nblobs);

		int laranjas_na_frame = 0;

		if (blobs != NULL) {
			for (int i = 0; i < nblobs; i++) {
				// Filtra ruidos pequenos (Ajustar este valor de área mínima se necessário)
				if (blobs[i].area > 1500) {
					laranjas_na_frame++;

					// ----- LÓGICA DE CONTAGEM TOTAL (RASTREADOR) -----
					int min_dist = 999999;
					int best_index = -1;

					// Procura se esta laranja já foi vista nas frames anteriores
					for (size_t j = 0; j < laranjas_registadas.size(); j++) {
						int dx = laranjas_registadas[j].xc - blobs[i].xc;
						int dy = laranjas_registadas[j].yc - blobs[i].yc;
						int dist = (dx * dx) + (dy * dy);

						if (dist < min_dist) {
							min_dist = dist;
							best_index = j;
						}
					}

					// Se a distância for pequena (ex: < 4000 píxeis quadrados), é a mesma laranja que se moveu
					if (min_dist < 4000 && best_index != -1) {
						laranjas_registadas[best_index].xc = blobs[i].xc;
						laranjas_registadas[best_index].yc = blobs[i].yc;
						laranjas_registadas[best_index].frame_ultima_vez = video.nframe;
					}
					else {
						// É uma LARANJA NOVA que entrou no ecrã!
						contador_total++;
						laranjas_registadas.push_back({ proximo_id++, blobs[i].xc, blobs[i].yc, video.nframe });
					}


					// ----- INTERFACE VISUAL NO ECRÃ -----

					// Desenha a Bounding Box (Uso 1/3 das funções OpenCV permitidas)
					cv::rectangle(frame, cv::Point(blobs[i].xmin, blobs[i].ymin), cv::Point(blobs[i].xmax, blobs[i].ymax), cv::Scalar(255, 0, 0), 2);

					// Desenha o Centro de Gravidade Manualmente (Sem gastar OpenCV)
					desenha_cruz_manual(frame, blobs[i].xc, blobs[i].yc, 5);

					// Formatar e desenhar o texto (Uso 2/3 das funções OpenCV permitidas)
					char texto[200];

					// Linha 1
					sprintf(texto, "LABEL:%d AREA:%d PERIMETRO:%.0f", blobs[i].label, blobs[i].area, blobs[i].perimeter);
					cv::putText(frame, texto, cv::Point(blobs[i].xmin, blobs[i].ymin - 25), cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(255, 255, 255), 1);

					// Linha 2
					sprintf(texto, "CALIBRE:%d DIAMETRO:%.0fmm", blobs[i].caliber, blobs[i].diameter);
					cv::putText(frame, texto, cv::Point(blobs[i].xmin, blobs[i].ymin - 12), cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(255, 255, 255), 1);

					// Linha 3
					sprintf(texto, "CATEGORIA:%s", blobs[i].category);
					cv::putText(frame, texto, cv::Point(blobs[i].xmin, blobs[i].ymin + blobs[i].ymax - blobs[i].ymin + 15), cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(0, 255, 255), 1);
				}
			}
			free(blobs);
		}

		// Desenhar a Informação Global no canto superior esquerdo
		char info_global[100];
		sprintf(info_global, "LARANJAS NA FRAME: %d | CONTAGEM TOTAL: %d", laranjas_na_frame, contador_total);
		cv::putText(frame, info_global, cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 0, 0), 3); // Sombra preta
		cv::putText(frame, info_global, cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 0), 1); // Texto verde

		/* Exibe a frame final */
		cv::imshow("VC - VISUALIZACAO", frame);

		/* Sai da aplicação se premir 'q' */
		key = cv::waitKey(1);
	}

	vc_image_free(image_rgb);
	vc_image_free(image_hsv);
	vc_image_free(image_bin);

	vc_timer();
	cv::destroyWindow("VC - VISUALIZACAO");
	capture.release();

	return 0;
}