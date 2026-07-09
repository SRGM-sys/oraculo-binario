import cv2
import mediapipe as mp
import socket # Libreria nativa de Python para enviar datos por red

# 1) Configurar el mensajero
IP = "127.0.0.1"    # Significa "Esta misma computadora"
PUERTO = 5052       # Numero del puerto

# Creamos el objeto que va a enviar los mensajes. 
# AF_INET = Usa direcciones de internet normales. SOCK_DGRAM = Usa el método UDP (envío súper rápido)
mensajero = socket.socket(socket.AF_INET, socket.SOCK_DGRAM) 

# 2) Configurar mediapipe
herramientas_manos = mp.solutions.hands
detector_de_manos = herramientas_manos.Hands()
herramientas_dibujo = mp.solutions.drawing_utils

# 3) Encender la cámara
# El numero 0 le dice a OpenCV que encienda la cámara principal de la compu
camara = cv2.VideoCapture(0)

print("Camara encendida.")
print("Enviando los 4 puntos de los dedos al puerto 5052...")

while True:
    exito, imagen = camara.read() # Leemos la cámara
    
    # Condicional para evitar que se cierre de golpe si la cámara falla
    if exito == False:
        print("Fallo al leer la cámara")
        break

    # Averiguamos cuanto mide la imagen
    dimensiones = imagen.shape
    alto = dimensiones[0]
    ancho = dimensiones[1]

    # Mediapipe funciona con RGB, OpenCV con BGR 
    imagen_rgb = cv2.cvtColor(imagen, cv2.COLOR_BGR2RGB)  # Hacemos una conversión

    resultados = detector_de_manos.process(imagen_rgb)

    # Condicional: ¿Encontraste alguna mano en la foto?
    if resultados.multi_hand_landmarks:

        # Recorrer todas las manos encontradas
        for mano in resultados.multi_hand_landmarks:

            herramientas_dibujo.draw_landmarks(imagen, mano, herramientas_manos.HAND_CONNECTIONS)
        
            # 4) Sacar las coordenadas matemáticas
            # MediaPipe devuelve decimales entre 0 y 1, pero necesitamos pixeles. Multiplicamos por el ancho y alto de la imagen
            # Dedo Índice (Punta = 8, Articulación media = 6)
            y_punta_indice = int(mano.landmark[8].y * alto)
            y_medio_indice = int(mano.landmark[6].y * alto)
            
            # Dedo Medio (Punta = 12, Articulación media = 10)
            y_punta_medio = int(mano.landmark[12].y * alto)
            y_medio_medio = int(mano.landmark[10].y * alto)

            # Dedo Anular (Punta = 16, Articulación media = 14)
            y_punta_anular = int(mano.landmark[16].y * alto)
            y_medio_anular = int(mano.landmark[14].y * alto)

            # Dedo Meñique (Punta = 20, Articulación media = 18)
            y_punta_menique = int(mano.landmark[20].y * alto)
            y_medio_menique = int(mano.landmark[18].y * alto)

            # 5) Enviar los datos a C++
            # Juntamos los 8 números separados por comas (Se van a enviar como String)
            mensaje = f"{y_punta_indice},{y_medio_indice},{y_punta_medio},{y_medio_medio},{y_punta_anular},{y_medio_anular},{y_punta_menique},{y_medio_menique}"

            # Convertirmos el texto a bytes (encode) y lo enviamos al C++ por el puerto 5052
            mensajero.sendto(mensaje.encode(), (IP, PUERTO))

            # Mostramos la imagen en la terminal para ver que todo funciona
            # print("Enviando: " + mensaje)

    # 6) Le decimos a OpenCV que muestre la imagen en una ventana
    cv2.imshow("Sensor de manos (Python)", imagen)

    if cv2.waitKey(1) == 27:
        break # Si presionamos la tecla ESC, salimos del bucle

# En caso de que se rompa el bucle, soltamos la cámara y cerramos la ventana
camara.release()
cv2.destroyAllWindows()