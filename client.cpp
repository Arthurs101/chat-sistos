#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>
#include "protocol.pb.h"

using namespace std;

void recibirMensajes(int sockfd) {
    while (true) {
        char buffer[8500];
        int bytes_recibidos = recv(sockfd, buffer, sizeof(buffer), 0);
        if (bytes_recibidos == -1) {
            perror("recv fallido");
            break;
        } else if (bytes_recibidos == 0) {
            cout << "El servidor se desconectó." << endl;
            break;
        } else {
            // Parsear respuesta
            chat::ServerResponse respuesta;
            if (!respuesta.ParseFromString(buffer)) {
                cerr << "Fallo al parsear la respuesta." << endl;
                break;
            }

            // Mostrar mensaje recibido
            cout << "Mensaje del servidor: " << respuesta.servermessage() << endl;
        }
    }
}

void enviarMensaje(int sockfd, const string& mensaje) {
    chat::MessageCommunication mensaje_comunicacion;
    mensaje_comunicacion.set_message(mensaje);
    string mensaje_serializado;
    if (!mensaje_comunicacion.SerializeToString(&mensaje_serializado)) {
        cerr << "Fallo al serializar el mensaje." << endl;
        return;
    }

    if (send(sockfd, mensaje_serializado.c_str(), mensaje_serializado.size(), 0) == -1) {
        perror("send fallido");
    }
}

int main(int argc, char const *argv[]) {
    if (argc != 4) {
        cerr << "Uso: " << argv[0] << " <nombredeusuario> <IPdelservidor> <puertodelservidor>" << endl;
        return 1;
    }

    // Analizar los argumentos de la línea de comandos
    string nombre_usuario = argv[1];
    const char* ip_servidor = argv[2];
    int puerto = atoi(argv[3]);

    // Crear socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("Fallo al crear el socket");
        return 1;
    }

    // Inicializar dirección del servidor
    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(puerto);
    if (inet_pton(AF_INET, ip_servidor, &servaddr.sin_addr) <= 0) {
        perror("inet_pton fallido");
        return 1;
    }

    // Conectar al servidor
    if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1) {
        perror("Fallo al conectar");
        return 1;
    }

    // Iniciar hilo para recibir mensajes
    thread recibir_thread(recibirMensajes, sockfd);
    recibir_thread.detach();

    // Enviar nombre de usuario al servidor
    chat::Registration registro;
    registro.set_username(nombre_usuario);
    string registro_serializado;
    if (!registro.SerializeToString(&registro_serializado)) {
        cerr << "Fallo al serializar el registro." << endl;
        close(sockfd);
        return 1;
    }

    if (send(sockfd, registro_serializado.c_str(), registro_serializado.size(), 0) == -1) {
        perror("send fallido");
        close(sockfd);
        return 1;
    }

    // Interacción con el usuario
    string opcion;
    while (true) {
        cout << "Seleccione una opción:" << endl;
        cout << "1. Enviar mensaje" << endl;
        cout << "2. Cambiar de status" << endl;
        cout << "3. Listar usuarios conectados" << endl;
        cout << "4. Obtener información de usuario en particular" << endl;
        cout << "5. Ayuda" << endl;
        cout << "6. Salir" << endl;
        cin >> opcion;

        if (opcion == "1") {
            cout << "Ingrese el mensaje: ";
            string mensaje;
            cin.ignore(); // Ignorar el salto de línea anterior
            getline(cin, mensaje);
            enviarMensaje(sockfd, mensaje);
        } else if (opcion == "2") {
            cout << "Seleccione un estado (ACTIVO, OCUPADO, INACTIVO): ";
            string estado;
            cin >> estado;
            cambiarEstado(sockfd, estado);
        } else if (opcion == "3") {
            listarUsuarios(sockfd);
        } else if (opcion == "4") {
            cout << "Ingrese el nombre del usuario: ";
            string nombre_usuario;
            cin >> nombre_usuario;
            obtenerInfoUsuario(sockfd, nombre_usuario);
        } else if (opcion == "5") {
            // Implementar ayuda
            cout << "Ayuda no implementada." << endl;
        } else if (opcion == "6") {
            // Implementar salida
            break;
        } else {
            cout << "Opción no válida." << endl;
        }
    }

    // Cerrar socket
    close(sockfd);

    return 0;
}
