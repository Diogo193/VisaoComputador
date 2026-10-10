Contador de laranjas
Aplicação em C/C++ para detectar, acompanhar e contar laranjas num vídeo. O programa utiliza o OpenCV para ler e apresentar as imagens e funções próprias (vc.c/vc.h) para a segmentação e análise dos objectos.

Funcionalidades
Segmenta objectos cor de laranja no espaço de cor HSV e aplica operações morfológicas para reduzir o ruído.
Filtra os objectos por área, calibre, deformação, preenchimento e circularidade.
Acompanha os objectos entre imagens e incrementa a contagem quando uma laranja cruza a linha horizontal, posicionada a 65% da altura do vídeo.
Apresenta na janela o identificador, as medidas, a categoria, o número de laranjas na imagem e o total contado.

Requisitos
Windows e Visual Studio 2022 com as ferramentas de C++.
OpenCV 4.13 instalado e configurado no projecto. As definições actuais do .vcxproj referenciam caminhos locais, como C:\opencv\build, e a biblioteca opencv_world4130 (ou opencv_world4130d em modo Debug); ajuste-as se o OpenCV estiver instalado noutro local.

Compilação e execução
Abra VC-GY.sln no Visual Studio.
Se necessário, ajuste os caminhos dos cabeçalhos e das bibliotecas do OpenCV nas propriedades do projecto.
Seleccione x64 e a configuração Debug ou Release; compile e execute o projecto.
O executável procura o ficheiro video.avi na pasta de trabalho actual. O repositório inclui esse vídeo em VC-GY/video.avi; defina VC-GY como pasta de trabalho ou copie o vídeo para a pasta de execução.
Prima q na janela do vídeo para terminar. Antes e depois do processamento, o programa aguarda que prima Enter na consola para apresentar o tempo decorrido.

Estrutura
VC-GY/Source.cpp — leitura do vídeo, acompanhamento, contagem e visualização.
VC-GY/vc.c e VC-GY/vc.h — operações de imagem, segmentação, análise dos objectos e classificação.
VC-GY/video.avi — vídeo de entrada incluído.
VC-GY/VC-GY.vcxproj e VC-GY.sln — projecto e solução do Visual Studio.
