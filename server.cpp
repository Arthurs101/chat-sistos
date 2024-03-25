/**
*Arturo Argueta 21527
*Daniel EStrada 
*/
#include <iostream>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <semaphore.h>
#include "protocol.pb.h"
using namespace std;

//estructura para modelar al cliente y facilitar el manejo de su información
struct Cli{
    int socket;
    string username;
    char ip[INET_ADDRSTRLEN]; //16 bits
    int status;
};

//all the clients en un "diccionario"
unordered_map<string,Cli*> servingCLients;

/**
 * Devuelve error al socket indicado
 * socketId: int -> del socket a decvolver 
 * errorMessage: stirng -> mensaje de error a devolver
 * optionHandler: int -> el tipo de request que se estaba realizando
 * >1: Registro de Usuarios
 * >2: Usuarios Conectados
 * >3: Cambio de Estado
 * >4: Mensajes
 * >5: Informacion de un usuario en particular
*/
void ErrorResponse(int optionHandled , int socketID , string errorMessage){
    chat::ServerResponse *errorRes = new chat::ServerResponse();
    errorRes->set_option(optionHandled);
    errorRes->set_code(500);
    errorRes->set_servermessage(errorMessage);
    //calcular tamaño del buffer a emplear
    char buff[errorMessage.size() +1 ];
    strcpy(buff, errorMessage.c_str());
    send(socketID, buff, sizeof buff, 0); 
}

//función para el manejo de requests
void *requestsHandler(void *params){
    struct Cli client;
    struct Cli *newClient = (struct Cli *)params; 
    int socket = newClient->socket; 
    char buffer[8000];

    // Server Structs
    string msgServer;
    chat::ClientPetition *request = new chat::ClientPetition();
    chat::ServerResponse *response = new chat::ServerResponse();
    while(1){
        //this will done forever
        response->Clear(); //limpiar la response enviada
        request->ParseFromString(buffer); //obtener la request actual
        switch (request->option()){
			case 1:{ //registrar un nuevo usuario en el servidor
				std::cout<<std::endl<<"__RECEIVED INFO__\nUsername: "<<request->registration().username()<<"		ip: "<<request->registration().ip();
				if (servingCLients.count(request->registration().username()) > 0)
				{
					std::cout<<std::endl<< "ERROR: Username already exists" <<std::endl;
					ErrorResponse(1,socket, "ERROR: Username already exists");
					break;
				}
				response->set_option(1);
				response->set_servermessage("SUCCESS: register");
				response->set_code(200);
				response->SerializeToString(&msgServer);
				strcpy(buffer, msgServer.c_str());
				send(socket, buffer, msgServer.size() + 1, 0);
				std::cout<<std::endl<<"SUCCESS:The user"<<client.username<<" was added with the socket: "<<socket<<std::endl;
				client.username = request->registration().username();
				client.socket = socket;
				client.status = 1;
				strcpy(client.ip, newClient->ip);
				servingCLients[client.username] = &client;
				break;
			}
        }
    }
}

int main(int argc, char const* argv[]){
    //verificar versiión de protobuf para evitar errores
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    return 0;
}