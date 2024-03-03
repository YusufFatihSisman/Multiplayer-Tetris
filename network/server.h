#ifndef SERVER_H
#define SERVER_H

#include <iostream>
#include <thread>
#include <vector>
#include <asio.hpp>

#include "tsqueue.h"
#include "message.h"
#include "connection.h"


template <typename T>
class server_interface{
    asio::io_context context;
    std::thread threadContext;
    asio::ip::tcp::acceptor acceptor;
    std::vector<std::shared_ptr<connection<T>>> connections;

    tsqueue<ClientMessage<T>> messageQueue;
    uint32_t cId = 1;

    public:
        server_interface() : acceptor(context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), 10000)) {}

        bool Start(){
            try{
                WaitForConnection();
                threadContext = std::thread([this]() { context.run(); });

            }catch(std::exception& e){
                std::cerr << "Connection Error: " << e.what() << "\n";
                return false;
            }
            return true;

        }

        void Update(uint16_t amount = -1){
            messageQueue.wait();
            uint16_t i = 0;
            while(!messageQueue.empty() && i < amount){
                ClientMessage<T> msg = messageQueue.pop();
                HandleMessage(msg.client, msg.message);
                i++;
            }
        }

    protected:
        virtual void HandleMessage(std::shared_ptr<connection<T>> client, const Message<T>& msg) = 0;

        virtual void OnClientDisconnect(std::shared_ptr<connection<T>> connection){}

        void Send(std::shared_ptr<connection<T>> connection, const Message<T>& msg){
            if(connection && connection->IsConnected()){
                connection->Send(msg);
            }
            else{
                OnClientDisconnect(connection);

                connection.reset();
                
                auto it = find(connections.begin(), connections.end(), connection);
 
                if (it != connections.end()) {
                    connections.erase(it);
                }
            }
        }

        void SendAll(const Message<T>& msg, std::shared_ptr<connection<T>> exception = nullptr){
            std::vector<std::shared_ptr<connection<T>>>::iterator it;
 
            it = connections.begin();

            for (auto it = connections.begin(); it != connections.end(); it++){
                if(*it && (*it)->IsConnected()){
                    if(*it != exception)
                        (*it)->Send(msg);
                }else{
                    OnClientDisconnect(*it);

                    (*it).reset();

                    connections.erase(it);
                    std::cout << connections.size() << "\n";
                    it--;
                }
            }
        }

    private:
        void WaitForConnection(){
            std::cout << "Wait connection\n";
            acceptor.async_accept(
                [this](std::error_code ec, asio::ip::tcp::socket socket)
                {
                    if(!ec){
                        std::cout << "[SERVER] New Connection: " << socket.remote_endpoint() << "\n";
                        std::shared_ptr<connection<T>> newConnection = std::make_shared<connection<T>>(context, std::move(socket), messageQueue);

                        connections.push_back(std::move(newConnection));
                        
                        connections[connections.size()-1]->ConnectToClient(cId++);
                        
                    }else{
                        std::cout << "[SERVER] New Connection Error: " << ec.message() << "\n";
                    }
                    WaitForConnection();
                }
            );
        }
};


#endif