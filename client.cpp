#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>
#include "protocol.pb.h"
// to compile:
// g++ client.cpp protocol.pb.cc -o clientx -lprotobuf -lpthread

using namespace std;

void recibirMensajes(int sockfd) {
    while (true) {
        char buffer[8192];
        int bytes_recibidos = recv(sockfd, buffer, 8192, 0);
        if (bytes_recibidos == -1) {
            perror("recv fallido");
            break;
        } else if (bytes_recibidos == 0) {
            cout << "El servidor se desconectó." << endl;
            break;
        } else {
            // Parsear respuesta
            chat::ServerResponse *respuesta=new chat::ServerResponse();
            if (!respuesta->ParseFromString(buffer)) {
                cerr << "Fallo al parsear la respuesta.\n aborting" << endl;
                break;
            }
            // Mostrar mensaje recibido
            cout <<"-------------------INCOMMING TRANSMISSION-------------------" << endl;
            cout << "Mensaje del servidor: " << respuesta->servermessage() << endl;
            cout<<respuesta->messagecommunication().sender() << ":\n"<< respuesta->messagecommunication().message() << endl;
            cout <<"------------------- END OF TRANSMISSION -------------------" << endl;
        }
    }
}

void enviarMensaje(int sockfd, const string& mensaje, const string& remitente = "",const string& destinatario = "everyone") {
    char buffer[8192];
    chat::ClientPetition *request = new chat::ClientPetition();
    chat::MessageCommunication *mensaje_comunicacion = new chat::MessageCommunication();
    mensaje_comunicacion->set_message(mensaje);
    mensaje_comunicacion->set_recipient(destinatario);
    mensaje_comunicacion->set_sender(remitente);
    request->set_option(4);
    request->set_allocated_messagecommunication(mensaje_comunicacion);
    std::string mensaje_serializado;
    if(!request->SerializeToString(&mensaje_serializado)){
        cerr << "Fallo al serializar el mensaje." << endl;
        return;
    };
	strcpy(buffer, mensaje_serializado.c_str());
	send(sockfd, buffer, mensaje_serializado.size() + 1, 0);
	recv(sockfd, buffer, 8192, 0);

    chat::ServerResponse *response = new chat::ServerResponse();
	response->ParseFromString(buffer);
	
	// si hubo error al buscar 
	if (response->code() != 200) {
		std::cout << response->servermessage()<< std::endl;
		return;
	}
}

void chateoPrivado(int sockfd, const string& destinatario,const string& remitente,const string& mensaje) {
    char buffer[8192];
    chat::ClientPetition *request = new chat::ClientPetition();
    chat::MessageCommunication *mensaje_comunicacion = new chat::MessageCommunication();
    mensaje_comunicacion->set_message(mensaje);
    mensaje_comunicacion->set_recipient(destinatario);
    mensaje_comunicacion->set_sender(remitente);
    request->set_option(4);
    request->set_allocated_messagecommunication(mensaje_comunicacion);
    std::string mensaje_serializado;
    if(!request->SerializeToString(&mensaje_serializado)){
        cerr << "Fallo al serializar el mensaje." << endl;
        return;
    };
	strcpy(buffer, mensaje_serializado.c_str());
	send(sockfd, buffer, mensaje_serializado.size() + 1, 0);
	recv(sockfd, buffer, 8192, 0);

    chat::ServerResponse *response = new chat::ServerResponse();
	response->ParseFromString(buffer);
	
	// si hubo error al buscar 
	if (response->code() != 200) {
		std::cout << response->servermessage()<< std::endl;
		return;
	}
}

void cambiarEstado(int sockfd, const string& username, const string& estado) {
    char buffer[8192];
    chat::ClientPetition *request = new chat::ClientPetition();
    chat::ChangeStatus *cambio_estado = new chat::ChangeStatus;
    cambio_estado->set_username(username);
    cambio_estado->set_status(estado);
    request->set_option(3);
    request->set_allocated_change(cambio_estado);

    string cambio_estado_serializado;
    if (!request->SerializeToString(&cambio_estado_serializado)) {
        cerr << "Fallo al serializar el cambio de estado." << endl;
        return;
    }
    strcpy(buffer, cambio_estado_serializado.c_str());
    if (send(sockfd, buffer, cambio_estado_serializado.size()+1, 0) == -1) {
        perror("send fallido");
    }
    	recv(sockfd, buffer, 8192, 0);

    chat::ServerResponse *response = new chat::ServerResponse();
	response->ParseFromString(buffer);
	
	// si hubo error al buscar 
	if (response->code() != 200) {
		std::cout << response->servermessage()<< std::endl;
		return;
	}
    cout<<response->servermessage()<<endl;
}

void listarUsuarios(int sockfd) {
    char buffer[8192];
    chat::ClientPetition *peticion = new chat::ClientPetition();
    peticion->set_option(2); // Opción para listar usuarios conectados
    string peticion_serializada;
    if (!peticion->SerializeToString(&peticion_serializada)) {
        cerr << "Fallo al serializar la petición de lista de usuarios." << endl;
        return;
    }
    strcpy(buffer, peticion_serializada.c_str());
	if(!send(sockfd, buffer, peticion_serializada.size() + 1, 0)){perror("send fallido");};
	recv(sockfd, buffer, 8192, 0);
    chat::ServerResponse *response = new chat::ServerResponse();
	response->ParseFromString(buffer);
	
	// si hubo error al buscar 
	if (response->code() != 200) {
		std::cout << response->servermessage()<< std::endl;
		return;
	}
    cout<<"---------------Retriieved Users:---------------"<<endl;
    for (int i = 0; i < response->connectedusers().connectedusers_size(); ++i) {
        auto user = response->connectedusers().connectedusers(i);
        cout<< "User: " << user.username()<<endl;
    }
    cout <<"-----------------------------------------------"<<endl ;

}

void obtenerInfoUsuario(int sockfd, const string& nombre_usuario) {
   char buffer[8192];
    chat::ClientPetition *peticion = new chat::ClientPetition();
    string peticion_serializada;
    chat::UserRequest *user_req = new chat::UserRequest();
    user_req->set_user(nombre_usuario);
    peticion->set_option(5); // Opción para listar usuarios conectados
    peticion->set_allocated_users(user_req);
    if (!peticion->SerializeToString(&peticion_serializada)) {
        cerr << "Fallo al serializar la petición de usuario en específico." << endl;
        return;
    }
    strcpy(buffer, peticion_serializada.c_str());
    if(!send(sockfd, buffer, peticion_serializada.size() + 1, 0)){perror("send fallido");};
	chat::ServerResponse *response = new chat::ServerResponse();
    recv(sockfd, buffer, 8192, 0);
    if(!response->ParseFromString(buffer)){perror("failed to parse response");};
    if (response->code() != 200) {
		std::cout << response->servermessage()<< std::endl;
	}else{
        cout<<"---------------Retriieved User Info:---------------"<<endl;
        cout<<"USER: "<<response->userinforesponse().username()<<"\nSTATUS: "<<response->userinforesponse().status()<<"\nIP: "<<response->userinforesponse().ip()<<endl;
        cout<<"---------------END OF RETRIEVED---------------"<<endl;
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
            string message;
            cout << "Ingresa tu mensaje:"<<endl;
            cin>>message;
            enviarMensaje(sockfd,message,nombre_usuario);
            // Código para enviar mensaje general
        } else if (opcion == "2") {
            // Código para chatear privadamente
            string destin;
            cout<<"Ingresa Username a enviar mensaje"<<endl;
            cin>>destin;
            string message;
            cout << "Ingresa tu mensaje:"<<endl;
            cin>>message;
            chateoPrivado(sockfd,destin,nombre_usuario,message);
        } else if (opcion == "3") {
            cout << "Seleccione un estado (ACTIVO, OCUPADO, INACTIVO): ";
            string estado;
            cin >> estado;
            cambiarEstado(sockfd, nombre_usuario,estado);
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
