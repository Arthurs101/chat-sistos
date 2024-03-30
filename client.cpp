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
        char buffer[8192];
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
            if (!respuesta.ParseFromArray(buffer, bytes_recibidos)) {
                cerr << "Fallo al parsear la respuesta." << endl;
                break;
            }

            // Mostrar mensaje recibido
            cout << "Mensaje del servidor: " << respuesta.servermessage() << endl;
        }
    }
}

void enviarMensaje(int sockfd, const string& mensaje, const string& destinatario = "everyone", const string& remitente = "") {
    chat::MessageCommunication mensaje_comunicacion;
    mensaje_comunicacion.set_message(mensaje);
    mensaje_comunicacion.set_recipient(destinatario);
    mensaje_comunicacion.set_sender(remitente);
    string mensaje_serializado;
    if (!mensaje_comunicacion.SerializeToString(&mensaje_serializado)) {
        cerr << "Fallo al serializar el mensaje." << endl;
        return;
    }

    if (send(sockfd, mensaje_serializado.c_str(), mensaje_serializado.size(), 0) == -1) {
        perror("send fallido");
    }
}

void chateoPrivado(int sockfd, const string& destinatario, const string& mensaje) {
    chat::MessageCommunication private_msg;
    private_msg.set_recipient(destinatario);
    private_msg.set_message(mensaje);
    string private_msg_serialized;
    if (!private_msg.SerializeToString(&private_msg_serialized)) {
        cerr << "Fallo al serializar el mensaje privado." << endl;
        return;
    }

    if (send(sockfd, private_msg_serialized.c_str(), private_msg_serialized.size(), 0) == -1) {
        perror("send fallido");
    }
}

void cambiarEstado(int sockfd, const string& estado) {
    chat::ChangeStatus cambio_estado;
    cambio_estado.set_status(estado);
    string cambio_estado_serializado;
    if (!cambio_estado.SerializeToString(&cambio_estado_serializado)) {
        cerr << "Fallo al serializar el cambio de estado." << endl;
        return;
    }

    if (send(sockfd, cambio_estado_serializado.c_str(), cambio_estado_serializado.size(), 0) == -1) {
        perror("send fallido");
    }
}

void listarUsuarios(int sockfd) {
    chat::ClientPetition peticion;
    peticion.set_option(2); // Opción para listar usuarios conectados
    string peticion_serializada;
    if (!peticion.SerializeToString(&peticion_serializada)) {
        cerr << "Fallo al serializar la petición de lista de usuarios." << endl;
        return;
    }

    if (send(sockfd, peticion_serializada.c_str(), peticion_serializada.size(), 0) == -1) {
        perror("send fallido");
    }
}

void obtenerInfoUsuario(int sockfd, const string& nombre_usuario) {
    chat::UserRequest *usuarios_info;
    usuarios_info->set_user(nombre_usuario);
    chat::ClientPetition peticion;
    peticion.set_option(5); // Opción para obtener información de usuario en particular
    peticion.set_allocated_users(usuarios_info);
    string peticion_serializada;
    if (!peticion.SerializeToString(&peticion_serializada)) {
        cerr << "Fallo al serializar la petición de información de usuario." << endl;
        return;
    }

    if (send(sockfd, peticion_serializada.c_str(), peticion_serializada.size(), 0) == -1) {
        perror("send fallido");
    }
}

int main(int argc, char const *argv[]) {
    if (argc != 4) {
        cerr << "Uso: " << argv[0] << " <nombredeusuario> <IPdelservidor> <puertodelservidor>" << endl;
        return 1;
    }

    string nombre_usuario = argv[1];
    string ip_servidor = argv[2];
    int puerto_servidor = stoi(argv[3]);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("socket fallido");
        return 1;
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(puerto_servidor);
    inet_pton(AF_INET, ip_servidor.c_str(), &server_addr.sin_addr);

    if (connect(sockfd, (sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect fallido");
        return 1;
    }

    std::string message_serialized;
    chat::ClientPetition *request = new chat::ClientPetition();
    chat::ServerResponse *response = new chat::ServerResponse();
    chat::UserRegistration *new_user = new chat::UserRegistration();

    new_user->set_ip("10.0.2.15");
    new_user->set_username(nombre_usuario);
    request->set_option(1);
    request->set_allocated_registration(new_user);
    char buffer[8192];

    request->SerializeToString(&message_serialized);
	strcpy(buffer, message_serialized.c_str());
	send(sockfd, buffer, message_serialized.size() + 1, 0);
	recv(sockfd, buffer, 8192, 0);
	response->ParseFromString(buffer);
	
	// si hubo error al buscar 
	if (response->code() != 200) {
		std::cout << response->servermessage()<< std::endl;
		return 0;
	}
    std::cout << "SERVER: "<< response->servermessage()<< std::endl;
    cout << "Conectado al servidor." << endl;
    
    // Iniciar hilo para recibir mensajes del servidor
    thread recibir_thread(recibirMensajes, sockfd);
    recibir_thread.detach();

    // Interacción con el usuario
    string opcion;
    while (true) {
        cout << "Seleccione una opción:" << endl;
        cout << "1. Enviar mensaje" << endl;
        cout << "2. Chatear privadamente" << endl;
        cout << "3. Cambiar de estado" << endl;
        cout << "4. Listar usuarios conectados" << endl;
        cout << "5. Obtener información de usuario en particular" << endl;
        cout << "6. Ayuda" << endl;
        cout << "7. Salir" << endl;
        cin >> opcion;

        if (opcion == "1") {
            // Código para enviar mensaje general
        } else if (opcion == "2") {
            // Código para chatear privadamente
        } else if (opcion == "3") {
            cout << "Seleccione un estado (ACTIVO, OCUPADO, INACTIVO): ";
            string estado;
            cin >> estado;
            cambiarEstado(sockfd, estado);
        } else if (opcion == "4") {
            listarUsuarios(sockfd);
        } else if (opcion == "5") {
            cout << "Ingrese el nombre del usuario: ";
            string nombre_usuario;
            cin >> nombre_usuario;
            obtenerInfoUsuario(sockfd, nombre_usuario);
        } else if (opcion == "6") {
            // Implementar ayuda
            cout << "Ayuda no implementada." << endl;
        } else if (opcion == "7") {
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
