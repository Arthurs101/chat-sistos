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
    string status;
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
    char buffer[8500];

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
            case 2:{ //ask for all users
                if(request->users().user()!= "" || !request->users().has_user()){ //empty or it has no parameter
                    chat::ConnectedUsersResponse *users = new chat::ConnectedUsersResponse();
                    for(auto i:servingCLients){
                        chat::UserInfo *user_in_pos = new chat::UserInfo();
                        user_in_pos->set_username(i.second->username);
                        user_in_pos->set_status(i.second->status);
                        user_in_pos->set_ip(i.second->ip);
                        users->add_connectedusers()->CopyFrom(*user_in_pos);
                    }
                    response->set_servermessage("SUCCESS: userinfo of all connected clients");
                    response->set_allocated_connectedusers(users);
                    response->set_option(2);
                    response->set_code(200);
                    response->SerializeToString(&msgServer);
                    strcpy(buffer, msgServer.c_str());
                    send(socket, buffer, msgServer.size() + 1, 0);
                    std::cout<<"User:"<<client.username<<"requested all conected";
                }
                else{
                    ErrorResponse(2,socket,"ERROR: can't specify a user when asking all");                
                }
				break;
			}
			case 3:{ //change status
                try{
                servingCLients[request->change().username()]->status = request->change().status();
                std::cout<<"\n__USER CHANGE STATUS SOLICITUDE__\nUser: "<<client.username<<" requested to change status.\nSUCCESS:status has changed successfully\n";
                chat::ChangeStatus *sStatus = new chat::ChangeStatus();
                sStatus->set_username(request->change().username());
                sStatus->set_status(request->change().status());
                response->set_allocated_change(sStatus);
                response->set_servermessage("\nSUCCESS:\nUser: "+request->change().username()+" has changed status successfully\n");
                response->set_code(200);
                response->set_option(3);
                response->SerializeToString(&msgServer);
                strcpy(buffer, msgServer.c_str());
                send(socket, buffer, msgServer.size() + 1, 0);
                }
                catch(exception &e){
                    ErrorResponse(3,socket,"Change status on: " + request->change().username() + "doesn't exist");
                }
				break;
			}
			case 4:{ //enviar mensaje
                if(!request->messagecommunication().has_recipient()){ //a chat global
                    std::cout<<"\n__SENDING GENERAL MESSAGE__\nUser: "<<request->messagecommunication().sender()<<" is trying to send a general message";
                    for (auto i:servingCLients){
                        if (i.first==request->messagecommunication().sender()){
                            chat::MessageCommunication *response_message = new chat::MessageCommunication();
                            response_message->set_message("");
                            response->set_allocated_messagecommunication(response_message);
                            response->set_servermessage("\nSUCCESS: general chat\n");
                            response->set_code(200);
                            response->set_option(4);
                            response->SerializeToString(&msgServer);
                            strcpy(buffer, msgServer.c_str());
                            send(socket, buffer, msgServer.size() + 1, 0);
                            std::cout<<"\nSUCCESS:User: "+request->messagecommunication().sender()+" has sent the message successfully to the general chat\n";
                        }
                        else{ 
                            chat::MessageCommunication *message = new chat::MessageCommunication();
                            message->set_sender(client.username);
                            message->set_message(request->messagecommunication().message());
                            response->set_allocated_messagecommunication(message);
                            response->set_servermessage("\nUser: "+request->messagecommunication().sender()+" sends you a message\n");
                            response->set_code(200);
                            response->set_option(4);
                            response->SerializeToString(&msgServer);
                            strcpy(buffer, msgServer.c_str());
                            send(i.second->socket, buffer, msgServer.size() + 1, 0);
                        }
                    }
                }
                else{ //is a direct message
                    std::cout<<"\n__SENDING PRIVATE MESSAGE__\nUser: "<<request->messagecommunication().sender()<<" is trying to send a private message to ->"<<request->messagecommunication().recipient();
                    auto _ = servingCLients.find(request->messagecommunication().recipient()); 
                    if (_ != servingCLients.end()){
                        chat::MessageCommunication *message = new chat::MessageCommunication();
                        message->set_sender(client.username);
                        message->set_recipient(request->messagecommunication().recipient());
                        message->set_message(request->messagecommunication().message());
                        response->set_allocated_messagecommunication(message);
                        response->set_servermessage("\nUser: "+request->messagecommunication().sender()+" sends you a message\n");
                        response->set_code(200);
                        response->set_option(4);
                        response->SerializeToString(&msgServer);
                        strcpy(buffer, msgServer.c_str());
                        send(servingCLients[request->messagecommunication().recipient()]->socket, buffer, msgServer.size() + 1, 0);
                        response->Clear();
                        //inform the sender that it was a succes
                        chat::MessageCommunication *response_message = new chat::MessageCommunication();
                        response_message->set_message("");
                        response->set_allocated_messagecommunication(response_message);
                        response->set_servermessage("SUCCESS: sent message to"+request->messagecommunication().recipient()+" successfully\n");
                        response->set_code(200);
                        response->set_option(4);
                        response->SerializeToString(&msgServer);
                        strcpy(buffer, msgServer.c_str());
                        send(socket, buffer, msgServer.size() + 1, 0);
                        std::cout<<"\nSUCCESS:User: "+request->messagecommunication().sender()+" has sent the message successfully ->"+request->messagecommunication().recipient()+"\n";
                    }
                    else{ // no existe
                        ErrorResponse(4,socket,"ERROR: recipient doesn't exist" );
                        cout<<"E: " + request->messagecommunication().sender()+ "tried to send to unexisting user: "+request->messagecommunication().recipient()<<endl;
                    }
                }
				break;
			}
			case 5:{ //obtener usuario en específico
				chat::UserInfo *userI = new chat::UserInfo();
                try{
                    //intentar obtener dicho valor con la llave (username)
                    userI->set_username(servingCLients[request->users().user()]->username);                    
                    userI->set_ip(servingCLients[request->users().user()]->ip);
                    userI->set_status(servingCLients[request->users().user()]->status);
                    response->set_allocated_userinforesponse(userI);
                    response->set_servermessage("SUCCESS: userinfo of "+request->users().user());
                    response->set_code(200);
                    response->set_option(2);
                    response->SerializeToString(&msgServer);
                    strcpy(buffer, msgServer.c_str());
                    send(socket, buffer, msgServer.size() + 1, 0);
                    std::cout<<"\n__USER INFO SOLICITUDE__\nUser: "<<client.username<<" requested info of ->"<<request->users().user()<<"\nSUCCESS: userinfo of "<<request->users().user()<<std::endl;
                }
                catch(const std::exception& e){ //no existe el usuario
                    response->set_servermessage("ERROR: "+request->users().user() + "doesn't exists");
                    response->set_code(400);
                    response->SerializeToString(&msgServer);
                    strcpy(buffer, msgServer.c_str());
                    send(socket, buffer, msgServer.size() + 1, 0);
                    std::cout<<"\n__USER INFO SOLICITUDE__\nUser: "<<client.username<<" requested info of ->"<<request->users().user()<<"\nERROR: userinfo of "<<request->users().user()<<std::endl;
                }
			}
			default:{
				break;
			}
        }
    }
}

int main(int argc, char const* argv[]){
    //verificar versiión de protobuf para evitar errores
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    if (argc != 2){
        fprintf(stderr, "NO PORT DECLARED: server <port>\n");
        return 1;
    }
    long port = strtol(argv[1], NULL, 10);
    sockaddr_in server, incomminig_req;
    socklen_t new_req_size;
    int socket_fd, new_req_ip;
    char incomminig_req_addr[INET_ADDRSTRLEN];
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = INADDR_ANY;
    memset(server.sin_zero, 0, sizeof server.sin_zero);

    // si hubo error al crear el socket para el cliente
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        fprintf(stderr, "ERROR: create socket\n");
        return 1;
    }

    // si hubo error al crear el socket para el cliente y enlazar ip
    if (bind(socket_fd, (struct sockaddr *)&server, sizeof(server)) == -1){
        close(socket_fd);
        fprintf(stderr, "ERROR: bind IP to socket.\n");
        return 2;
    }
	
    // si hubo error al crear el socket para esperar respuestas
    if (listen(socket_fd, 5) == -1){
        close(socket_fd);
        fprintf(stderr, "ERROR: listen socket\n");
        return 3;
    }


    // si no hubo errores se puede proceder con el listen del server
    printf("SUCCESS: listening on port-> %ld\n", port);
	
    while (1){
	    
        // la funcion accept nos permite ver si se reciben o envian mensajes
        new_req_size = sizeof incomminig_req;
        new_req_ip = accept(socket_fd, (struct sockaddr *)&incomminig_req, &new_req_size);
	    
        // si hubo error al crear el socket para el cliente
        if (new_req_ip == -1){
            perror("ERROR: accept socket incomming connection\n");
            continue;
        }
	    
        //si falla el socket, un hilo se encargará del manejo de las requests del user
        struct Cli newClient;
        newClient.socket = new_req_ip;
        inet_ntop(AF_INET, &(incomminig_req.sin_addr), newClient.ip, INET_ADDRSTRLEN);
        pthread_t thread_id;
        pthread_attr_t attrs;
        pthread_attr_init(&attrs);
        pthread_create(&thread_id, &attrs, requestsHandler, (void *)&newClient);
    }
	
    // si hubo error al crear el socket para el cliente
    google::protobuf::ShutdownProtobufLibrary();
	return 0;

}