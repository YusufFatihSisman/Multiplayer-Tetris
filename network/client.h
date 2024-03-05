#ifndef CLIENT_H
#define CLIENT_H

#include <thread>
#include <asio.hpp>

#include "tsqueue.h"
#include "message.h"
#include "connection.h"

template <typename T>
class client_interface{
    asio::io_context context;
    std::thread threadContext;
    std::unique_ptr<connection<T>> cConnection;

    protected:
        tsqueue<ClientMessage<T>> messageQueue;

    public:
        bool Connect(const std::string& host, const uint16_t port){
            try{
                asio::ip::tcp::resolver resolver(context);
                asio::ip::tcp::resolver::results_type endpoints = resolver.resolve(host, std::to_string(port));

                cConnection = std::make_unique<connection<T>>(context, asio::ip::tcp::socket(context), messageQueue);

                cConnection->ConnectToServer(endpoints);
                threadContext = std::thread([this]() { context.run(); });

            }catch(std::exception& e){
                std::cerr << "Connection Error: " << e.what() << "\n";
                return false;
            }
            return true;
        }

        void Disconnect(){
            if(cConnection && cConnection->IsConnected()){
                cConnection->Disconnect();
                while(cConnection->IsConnected());
                cConnection.release();
                if(threadContext.joinable()){
                    threadContext.join();
                }
            }
        }

        void Send(const Message<T>& msg){
            if(cConnection){
                cConnection->Send(msg);
            }
        }

        void Update(uint16_t amount = 1){
            uint16_t i = 0;
            while(!messageQueue.empty() && i < amount){
                ClientMessage<T> message = messageQueue.pop();
                HandleMessage(message.message);
                i++;
            }
        }
    
    protected:
        virtual void HandleMessage(Message<T>& msg) = 0;


};


#endif