#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>
#include "protocol.pb.h"

using namespace std;

// Variables globales para el socket y el nombre de usuario
int sockfd;
string nombre_usuario;

// Función para recibir mensajes del servidor en un hilo separado
void recibirMensajes() {
    while (true) {
        char buffer[8192];
        int bytes_recibidos = recv(sockfd, buffer, sizeof(buffer), 0);
        if (bytes_recibidos == -1) {
            perror("recv fallido");
            close(sockfd);
            exit(EXIT_FAILURE);
        } else if (bytes_recibidos == 0) {
            cout << "El servidor se desconectó." << endl;
            close(sockfd);
            exit(EXIT_SUCCESS);
        } else {
            // Parsear respuesta
            chat::ServerResponse respuesta;
            if (!respuesta.ParseFromArray(buffer, bytes_recibidos)) {
                cerr << "Fallo al parsear la respuesta.\n aborting" << endl;
                close(sockfd);
                exit(EXIT_FAILURE);
            }
            // Mostrar mensaje recibido
            cout <<"-------------------INCOMMING TRANSMISSION-------------------" << endl;
            cout << "Mensaje del servidor: " << respuesta.servermessage() << endl;
            cout << respuesta.messagecommunication().sender() << ":\n" << respuesta.messagecommunication().message() << endl;
            cout <<"------------------- END OF TRANSMISSION -------------------" << endl;
        }
    }
}

// Función para enviar un mensaje al servidor
void enviarMensaje(const string& mensaje, const string& destinatario = "everyone") {
    chat::ClientPetition request;
    chat::MessageCommunication *mensaje_comunicacion = request.mutable_messagecommunication();
    mensaje_comunicacion->set_message(mensaje);
    mensaje_comunicacion->set_recipient(destinatario);
    mensaje_comunicacion->set_sender(nombre_usuario);
    request.set_option(4);
    
    string mensaje_serializado;
    if(!request.SerializeToString(&mensaje_serializado)){
        cerr << "Fallo al serializar el mensaje." << endl;
        return;
    }

    char buffer[8192];
    strcpy(buffer, mensaje_serializado.c_str());
    send(sockfd, buffer, mensaje_serializado.size() + 1, 0);
}

// Función para cambiar el estado del usuario
void cambiarEstado(const string& estado) {
    chat::ClientPetition request;
    chat::ChangeStatus *cambio_estado = request.mutable_change();
    cambio_estado->set_username(nombre_usuario);
    cambio_estado->set_status(estado);
    request.set_option(3);

    string cambio_estado_serializado;
    if (!request.SerializeToString(&cambio_estado_serializado)) {
        cerr << "Fallo al serializar el cambio de estado." << endl;
        return;
    }

    char buffer[8192];
    strcpy(buffer, cambio_estado_serializado.c_str());
    if (send(sockfd, buffer, cambio_estado_serializado.size()+1, 0) == -1) {
        perror("send fallido");
        return;
    }
}

// Función para listar usuarios conectados
void listarUsuarios() {
    chat::ClientPetition peticion;
    peticion.set_option(2); // Opción para listar usuarios conectados

    string peticion_serializada;
    if (!peticion.SerializeToString(&peticion_serializada)) {
        cerr << "Fallo al serializar la petición de lista de usuarios." << endl;
        return;
    }

    char buffer[8192];
    strcpy(buffer, peticion_serializada.c_str());
    if (send(sockfd, buffer, peticion_serializada.size() + 1, 0) == -1) {
        perror("send fallido");
        return;
    }

    chat::ServerResponse response;
    int bytes_recibidos = recv(sockfd, buffer, sizeof(buffer), 0);
    if (bytes_recibidos == -1) {
        perror("recv fallido");
        return;
    }

    if (!response.ParseFromArray(buffer, bytes_recibidos)) {
        cerr << "Fallo al parsear la respuesta del servidor." << endl;
        return;
    }

    cout << "---------------Retriieved Users:---------------" << endl;
    for (int i = 0; i < response.connectedusers().connectedusers_size(); ++i) {
        auto user = response.connectedusers().connectedusers(i);
        cout << "User: " << user.username() << endl;
    }
    cout << "-----------------------------------------------" << endl;
}

// Función para obtener información de un usuario en particular
void obtenerInfoUsuario(const string& nombre_usuario) {
    chat::ClientPetition peticion;
    peticion.set_option(5); // Opción para obtener información de usuario

    chat::UserRequest *user_req = peticion.mutable_users();
    user_req->set_user(nombre_usuario);

    string peticion_serializada;
    if (!peticion.SerializeToString(&peticion_serializada)) {
        cerr << "Fallo al serializar la petición de usuario en específico." << endl;
        return;
    }

    char buffer[8192];
    strcpy(buffer, peticion_serializada.c_str());
    if (send(sockfd, buffer, peticion_serializada.size() + 1, 0) == -1) {
        perror("send fallido");
        return;
    }

    chat::ServerResponse response;
    int bytes_recibidos = recv(sockfd, buffer, sizeof(buffer), 0);
    if (bytes_recibidos == -1) {
        perror("recv fallido");
        return;
    }

    if (!response.ParseFromArray(buffer, bytes_recibidos)) {
        cerr << "Fallo al parsear la respuesta del servidor." << endl;
        return;
    }

    if (response.code() != 200) {
        cerr << response.servermessage() << endl;
    } else {
        cout << "---------------Retriieved User Info:---------------" << endl;
        cout << "USER: " << response.userinforesponse().username() << endl;
        cout << "STATUS: " << response.userinforesponse().status() << endl;
        cout << "IP: " << response.userinforesponse().ip() << endl;
        cout << "---------------END OF RETRIEVED---------------" << endl;
    }
}

// Función para manejar la entrada del usuario y realizar acciones correspondientes
void manejarEntradaUsuario() {
    string opcion;
    while (true) {
        cout << "Seleccione una opción:" << endl;
        cout << "1. Enviar mensaje" << endl;
        cout << "2. Cambiar de estado" << endl;
        cout << "3. Listar usuarios conectados" << endl;
        cout << "4. Obtener información de usuario en particular" << endl;
        cout << "5. Salir" << endl;
        cin >> opcion;

        if (opcion == "1") {
            string destinatario, mensaje;
            cout << "Ingrese el nombre del destinatario: ";
            cin >> destinatario;
            cout << "Ingrese el mensaje: ";
            cin.ignore();
            getline(cin, mensaje);
            enviarMensaje(mensaje, destinatario);
        } else if (opcion == "2") {
            string estado;
            cout << "Seleccione un estado (ACTIVO, OCUPADO, INACTIVO): ";
            cin >> estado;
            cambiarEstado(estado);
        } else if (opcion == "3") {
            listarUsuarios();
        } else if (opcion == "4") {
            string nombre_usuario;
            cout << "Ingrese el nombre del usuario: ";
            cin >> nombre_usuario;
            obtenerInfoUsuario(nombre_usuario);
        } else if (opcion == "5") {
            cout << "Saliendo del chat." << endl;
            close(sockfd);
            exit(EXIT_SUCCESS);
        } else {
            cout << "Opción no válida." << endl;
        }
    }
}

int main(int argc, char const *argv[]) {
    if (argc != 4) {
        cerr << "Uso: " << argv[0] << " <nombredeusuario> <IPdelservidor> <puertodelservidor>" << endl;
        return EXIT_FAILURE;
    }

    nombre_usuario = argv[1];
    string ip_servidor = argv[2];
    int puerto_servidor = stoi(argv[3]);

    // Validación de la dirección IP del servidor
    struct sockaddr_in sa;
    if (inet_pton(AF_INET, ip_servidor.c_str(), &(sa.sin_addr)) == 0) {
        cerr << "Dirección IP del servidor inválida." << endl;
        return EXIT_FAILURE;
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("socket fallido");
        return EXIT_FAILURE;
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(puerto_servidor);
    inet_pton(AF_INET, ip_servidor.c_str(), &server_addr.sin_addr);

    if (connect(sockfd, (sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect fallido");
        return EXIT_FAILURE;
    }

    // Registrar al usuario con el servidor
    chat::UserRegistration new_user;
    new_user.set_ip("10.0.2.15"); // OJO: Aquí podría obtenerse la IP real del cliente
    new_user.set_username(nombre_usuario);

    chat::ClientPetition request;
    request.set_option(1);
    *request.mutable_registration() = new_user;

    string mensaje_serializado;
    if (!request.SerializeToString(&mensaje_serializado)) {
        cerr << "Fallo al serializar el registro del usuario." << endl;
        close(sockfd);
        return EXIT_FAILURE;
    }

    char buffer[8192];
    strcpy(buffer, mensaje_serializado.c_str());
    send(sockfd, buffer, mensaje_serializado.size() + 1, 0);

    // Crear un hilo para recibir mensajes del servidor
    thread recibir_thread(recibirMensajes);
    recibir_thread.detach();

    // Manejar la entrada del usuario
    manejarEntradaUsuario();

    return EXIT_SUCCESS;
}
