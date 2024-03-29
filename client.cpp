#include <iostream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
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

int main(int argc, char const *argv[]) {
    if (argc != 3) {
        cerr << "Uso: " << argv[0] << " <ip_servidor> <puerto>" << endl;
        return 1;
    }

    // Analizar los argumentos de la línea de comandos
    const char* ip_servidor = argv[1];
    int puerto = atoi(argv[2]);

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

    // Recibir mensaje de bienvenida
    char buffer[8500];
    int bytes_recibidos = recv(sockfd, buffer, sizeof(buffer), 0);
    if (bytes_recibidos == -1) {
        perror("recv fallido");
        return 1;
    } else if (bytes_recibidos == 0) {
        cout << "El servidor se desconectó." << endl;
        return 1;
    } else {
        // Parsear respuesta
        chat::ServerResponse respuesta;
        if (!respuesta.ParseFromString(buffer)) {
            cerr << "Fallo al parsear la respuesta." << endl;
            return 1;
        }

        // Mostrar mensaje de bienvenida
        cout << "¡Bienvenido al servidor de chat!" << endl;
        cout << "Mensaje del servidor: " << respuesta.servermessage() << endl;
    }

    // Registrarse en el servidor
    cout << "Por favor ingrese su nombre de usuario: ";
    string nombre_usuario;
    cin >> nombre_usuario;
    chat::ClientPetition solicitud;
    solicitud.set_option(1); // Registro
    solicitud.mutable_registration()->set_username(nombre_usuario);

    // Serializar la solicitud
    string solicitud_serializada;
    if (!solicitud.SerializeToString(&solicitud_serializada)) {
        cerr << "Fallo al serializar la solicitud." << endl;
        return 1;
    }

    // Enviar solicitud de registro al servidor
    if (send(sockfd, solicitud_serializada.c_str(), solicitud_serializada.size(), 0) == -1) {
        perror("Fallo al enviar");
        return 1;
    }

    // Iniciar hilo para recibir mensajes del servidor
    thread hilo_recv(recibirMensajes, sockfd);

    // Bucle de chat
    string mensaje;
    cin.ignore(); // Limpiar el buffer de entrada
    while (true) {
        cout << "Ingrese un mensaje (escriba 'exit' para salir): ";
        getline(cin, mensaje);

        // Comprobar si el usuario quiere salir
        if (mensaje == "exit") {
            break;
        }

        // Preparar el mensaje
        chat::ClientPetition solicitud;
        solicitud.set_option(4); // Mensaje
        solicitud.mutable_messagecommunication()->set_sender(nombre_usuario);
        solicitud.mutable_messagecommunication()->set_message(mensaje);

        // Serializar la solicitud
        string solicitud_serializada;
        if (!solicitud.SerializeToString(&solicitud_serializada)) {
            cerr << "Fallo al serializar la solicitud." << endl;
            break;
        }

        // Enviar mensaje al servidor
        if (send(sockfd, solicitud_serializada.c_str(), solicitud_serializada.size(), 0) == -1) {
            perror("Fallo al enviar");
            break;
        }
    }

    // Unir el hilo de recepción
    hilo_recv.join();

    // Cerrar el socket
    close(sockfd);

    return 0;
}
