#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <sstream>
#include <winsock2.h>	// Librería de Windows para manejar redes
#include <thread>		// Librería para usar hilos (Porgramación recurrente)
#include <opencv2/opencv.hpp> // Gráficos

// Aquí le decimos a Visual que vincule la librería de red automáticamente
// Sin necesidad de hacerlo en Proyecto -> Propiedades
#pragma comment(lib, "ws2_32.lib")

using namespace std;
using namespace cv;

// Variable globales
int respuestaActual = -1;		// guardar la respuesta actual (0, 1 o -1 si no hay gesto claro)
int tablaActual = 0;			// Aquí voy a manejar el índice de las tablas
int rondaActual = 0;			// Aquí voy a manejar la ronda del mes y la ronda del día
int reiniciar = 0;				// 0: Terminará la lectura ; 1: Se reiniciará la lectura
bool avanzarTabla = false;		// Esta variable nos indica si debemos avanzar a la siguiente tabla
vector<int> respuestasFinales;	// Aquí guardaremos las respuestas para hacer el calculo correspondiente


// Declaración de funciones
void comunicador();
void iniciarRed();
void imagenesManager();
void detectarClick(int, int, int, int, void*);
void calcularFecha();
void restablecer();

int main() {

	iniciarRed(); // La antena queda fuera del ciclo ya que solo necesita iniciarse una única vez
	
	do {
		reiniciar = 0;			// Reiniciamos la variable de reinicio
		
		imagenesManager();		// Cargamos las imágenes y mostramos la ventana principal
		calcularFecha();		// Calculamos la fecha final y la mostramos en pantalla

	} while (reiniciar == 1);
	
	return 0;
}

void comunicador() {

	// FASE 1: Encendemos la antena, pedimos permiso al sistema operativo
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	// FASE 2: Crear el Socket
	// Usar AF_INET es lo mismo que usar IPv4
	SOCKET receptor = socket(AF_INET, SOCK_DGRAM, 0);

	// FASE 3: Sintonizar el puerto (BIND)
	sockaddr_in direccion;
	direccion.sin_family = AF_INET;
	direccion.sin_port = htons(5052); // Este es el mismo puerto que pusimos en python
	direccion.sin_addr.s_addr = INADDR_ANY; // Escucchar desde cualquier IP local 

	// Atamos (BIND) nuestro socket a esa dirección y puerto
	::bind(receptor, (sockaddr*)&direccion, sizeof(direccion));

	cout << "Receptor C++ encendido. Escuchando el puerto 5052..." << endl;

	// FASE 4:El bucle de escucha
	char mensaje[1024]; // Un buffer o espacio de memoria para guardar el texto que llegue
	sockaddr_in direccionEmisor;
	int sizeEmisor = sizeof(direccionEmisor);

	while (true) {
		// Limpiamos la memoria del mensaje anterior
		memset(mensaje, 0, sizeof(mensaje));

		// Nos quedamos esperando a que llegue un paquete de datos (recvfrom) (Simon, muchas cosas XD)
		int bytesRecibidos = recvfrom(receptor, mensaje, sizeof(mensaje), 0, (sockaddr*)&direccionEmisor, &sizeEmisor);

		// Si recibimos algo mayor a 0 bytes , significa que Python habló
		if (bytesRecibidos > 0) {
			// Preparamos el mensaje recibido para poder cortarlo en partes y guardarlas en variables
			string textoRecibido(mensaje);
			stringstream cortador(textoRecibido);
			string p_ind, m_ind, p_med, m_med, p_anu, m_anu, p_men, m_men;

			// Cortamos el mensaje en partes usando la coma como separador
			getline(cortador, p_ind, ','); getline(cortador, m_ind, ',');
			getline(cortador, p_med, ','); getline(cortador, m_med, ',');
			getline(cortador, p_anu, ','); getline(cortador, m_anu, ',');
			getline(cortador, p_men, ','); getline(cortador, m_men, ',');

			// Convertimos las partes cortadas a int
			int yp_ind = stoi(p_ind); int ym_ind = stoi(m_ind);
			int yp_med = stoi(p_med); int ym_med = stoi(m_med);
			int yp_anu = stoi(p_anu); int ym_anu = stoi(m_anu);
			int yp_men = stoi(p_men); int ym_men = stoi(m_men);

			// Aquí hacemos un analisis de los valores recibidos para determinar la posición de los dedos
			bool indice_up = yp_ind < ym_ind;
			bool medio_up = yp_med < ym_med;
			bool anular_up = yp_anu < ym_anu;
			bool menique_up = yp_men < ym_men;

			// Para que sea 0: SÓLO el índice arriba, los otros 3 ABAJO obligatoriamente.
			if (indice_up && !medio_up && !anular_up && !menique_up) {
				respuestaActual = 0;
				//cout << "Respuesta: 0 (No esta en la tabla)" << endl;
			}
			// Para que sea 1 (Paz): Índice y medio ARRIBA, anular y meñique ABAJO obligatoriamente.
			else if (indice_up && medio_up && !anular_up && !menique_up) {
				respuestaActual = 1;
				//cout << "Respuesta : 1 (SI esta en la tabla)" << endl;
			}
			else {
				respuestaActual = -1;
			}
		}
	}

	// Apagamos todo, aunque con el while true, nunca llegaremos aqui XD (Es una buena practica)
	closesocket(receptor);
	WSACleanup();
}

void iniciarRed() {
	// Iniciamos un hilo para el comunicador
	thread hiloComunicador(comunicador);
	
	// Dejamos que el hilo corra de manera independiente en segundo plano
	hiloComunicador.detach();
}

void imagenesManager() {
	vector<Mat> listaTablas(5);
	String path = "F:\\Main\\Programacion\\OpenCV\\Chapter1\\Resources\\tabla";
	String ronda = "Mes";

	listaTablas[0] = imread(path + "0.png");
	listaTablas[1] = imread(path + "1.png");
	listaTablas[2] = imread(path + "2.png");
	listaTablas[3] = imread(path + "3.png");
	listaTablas[4] = imread(path + "4.png");

	// Creamos la ventana principal para que OpenCV empiece a escuchar el mouse
	namedWindow("Oraculo Binario (C++)");
	setMouseCallback("Oraculo Binario (C++)", detectarClick, NULL);

	// Bucle gráfico
	while (true) {
		if (tablaActual >= 5) {
			cout << "Se han completado las tablas" << endl;
			break;
		}

		// Mostramos la tabla actual
		Mat pantalla = listaTablas[tablaActual].clone();

		// Dibujamos el texto
		if (respuestaActual == 0) {
			putText(pantalla, "NO", Point(20, 45), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 0, 255), 2);
		}
		else if (respuestaActual == 1) {
			putText(pantalla, "SI", Point(20, 45), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 255, 0), 2);
		}
		else {
			putText(pantalla, "Indefinido", Point(15, 45), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 255, 255), 2);
		}

		// Dibujamos el botón de avanzar tabla
		// Point 1 (Izquierda, Arriba) ; Point 2 (Derecha, Abajo)
		rectangle(pantalla, Point(420, 20), Point(610, 55), Scalar(150, 150, 150), -1);
		// Dibujamos el texto del botón
		putText(pantalla, "SIGUIENTE ("+ronda+")", Point(425, 45), FONT_HERSHEY_SIMPLEX, 0.7, Scalar(0, 0, 0), 2);

		// Lógica de transición de tabla
		if (avanzarTabla) {
			tablaActual++;
			avanzarTabla = false; // Reiniciamos la bandera de avanzar tabla
			respuestaActual = -1; // Reiniciamos la respuesta actual

			if (tablaActual == 4 && rondaActual == 0) {
				tablaActual = 0;
				rondaActual = 1;
				ronda = "Dia";
			}

			else if (tablaActual == 5 && rondaActual == 1) {
				rondaActual = 2; // Esto indica que ya terminamos las rondas 
			}

			continue; // Esto sirve para reiniciar el ciclo while inmediatamente
		}

		imshow("Oraculo Binario (C++)", pantalla);

		if (waitKey(1) == 27) { // Si presionamos ESC, salimos del bucle
			break;
		}

	}
}

void detectarClick(int evento, int x, int y, int flags, void* userdata) {
	// Si detectamos un click izquierdo del mouse
	if (evento == EVENT_LBUTTONDOWN) {
		// BOTON AVANZAR
		if (x >= 420 && x <= 610 && y >= 20 && y <= 55) {
			// Validamos si hay una respuesta clara
			if (respuestaActual != -1) {
				cout << "Respuesta guardada: " << respuestaActual << endl;
				respuestasFinales.push_back(respuestaActual); // Guardamos la respuesta en el vector
				avanzarTabla = true; // Indicamos que se debe avanzar a la siguiente tabla
			}
			else {
				cout << "No hay una respuesta clara para guardar." << endl;
			}
		}
		if (rondaActual == 2 && x >= 300 && x <= 600 && y >= 320 && y <= 365) {
			restablecer();
			reiniciar = 1;
		}
	}
}

void calcularFecha() {
	vector<String> meses = { "Enero", "Febrero", "Marzo", "Abril", "Mayo", "Junio", "Julio", "Agosto", "Septiembre", "Octubre", "Noviembre", "Diciembre" };
	int mes = 0;
	int dia = 0;

	// Mi vector respuestasFinales tendrá un total de 9 respuestas, las primeras 4 son para el mes y las últimas 5 para el día
	// Dibujare una pantalla nueva para presentar el texto en grande, con una pantalla negra de fondo
	Mat pantallaFinal(480, 640, CV_8UC3, Scalar(0, 0, 0));

	// Calculamos el mes
	for (int i = 0; i < 4; i++) {
		if (respuestasFinales[i] == 1) {
			mes += (int)pow(2, i);
		}
	}

	// Calculamos el día
	for (int i = 4; i < 9; i++) {
		if (respuestasFinales[i] == 1) {
			dia += (int)pow(2, i - 4);
		}
	}

	String textoFecha = "";
	if (mes >= 1 && mes <= 12 && dia >= 1 && dia <= 31) {
		textoFecha = to_string(dia) + " de " + meses[mes - 1];
	}
	else {
		textoFecha = "Fecha invalida (Mala lectura)";
	}

	putText(pantallaFinal, "Tu cumples el:", Point(200, 130), FONT_HERSHEY_SIMPLEX, 0.8, Scalar(255, 255, 255), 2);
	putText(pantallaFinal, textoFecha, Point(120, 220), FONT_HERSHEY_SIMPLEX, 1.5, Scalar(0, 255, 0), 3);

	// DIBUJAMOS EL BOTON PARA REALIZAR OTRA LECTURA
	rectangle(pantallaFinal, Point(300, 320), Point(600, 365), Scalar(150, 150, 150), -1);
	putText(pantallaFinal, "REALIZAR OTRA LECTURA", Point(310, 355), FONT_HERSHEY_SIMPLEX, 0.7, Scalar(0, 0, 0), 2);

	imshow("Oraculo Binario (C++)", pantallaFinal);
	
	// Nos quedamos aquí atrapados hasta que el mouse cambie "reiniciar" a 1
	while (reiniciar == 0) {

		// Seguimos escuchando la ventana por si el usuario quiere salir a la fuerza
		if (waitKey(1) == 27) { // ESC
			exit(0); // Mata el programa por completo
		}
	}
}

void restablecer() {
	// Reiniciamos todas las variables para poder hacer otra lectura
	tablaActual = 0;
	rondaActual = 0;
	respuestaActual = -1;
	respuestasFinales.clear();
}