#ifndef CONNECTION_H
#define CONNECTION_H

#include <asio.hpp>

#include "tsqueue.h"
#include "message.h"

template <typename T>
class connection : public std::enable_shared_from_this<connection<T>>{

    asio::io_context& context;
    asio::ip::tcp::socket socket;

    tsqueue<Message<T>> messagesToSend;
    tsqueue<ClientMessage<T>>& receivedMessages;

    Message<T> tempMessageRead;

    uint32_t id = 0;

    public:

        connection(asio::io_context& ctxt, asio::ip::tcp::socket sckt, tsqueue<ClientMessage<T>>& msgqueue) : context(ctxt), socket(std::move(sckt)), receivedMessages(msgqueue){}

        void ConnectToServer(const asio::ip::tcp::resolver::results_type& endpoints){
            asio::async_connect(socket, endpoints,
                [this](std::error_code ec, asio::ip::tcp::endpoint endpoint)
                {
                    if (!ec)
                    {
                        ReadHeader();
                    }
                });
        }

        void ConnectToClient(uint32_t cId){
            id = cId;
            ReadHeader();
        }

        bool IsConnected(){
            return socket.is_open();
        }

        void Disconnect(){
            if(socket.is_open()){
                asio::post(context, 
                    [this](){
                        socket.close();
                    }
                );
            }
            
        }

        void Send(const Message<T>& msg){
            asio::post(context, 
                [this, msg](){
                    bool IsWritingAlready = !messagesToSend.empty();
                    messagesToSend.push(msg);
                    if(!IsWritingAlready)
                        WriteHeader();
                }
            );
        }

    private:
        void ReadHeader(){
            asio::async_read(socket, asio::buffer(&tempMessageRead.head, sizeof(MessageHead<T>)), 
                [this](std::error_code ec, std::size_t length){
                    if(!ec){
                        if(tempMessageRead.head.size > 0){
                            tempMessageRead.body.resize(tempMessageRead.head.size);
                            ReadBody();
                        }else{
                            //message with empty body
                            AddMessageQeueue();
                        }
                    }else{
                        std::cerr << "Eror during read: "  << ec.message() << "\n";
                        Disconnect();
                    }
                }
            );
        }

        void ReadBody(){
            asio::async_read(socket, asio::buffer(tempMessageRead.body.data(), tempMessageRead.head.size), 
                [this](std::error_code ec, std::size_t length){
                    if(!ec){
                        AddMessageQeueue();
                    }else{
                        std::cerr << "Eror during read: "  << ec.message() << "\n";
                        Disconnect();
                    }
                }
            );
        }

        void WriteHeader(){
            asio::async_write(socket, asio::buffer(&messagesToSend.front().head, sizeof(MessageHead<T>)),
                [this](std::error_code ec, std::size_t length){
                    if(!ec){
                        if(messagesToSend.front().head.size > 0){
                            WriteBody();
                        }else{
                            messagesToSend.pop();
                            if(!messagesToSend.empty())
                                WriteHeader();
                        }
                    }else{
                        std::cerr << "Eror during write: "  << ec.message() << "\n";
                        Disconnect();
                    }
                }
            );
        }

        void WriteBody(){
            asio::async_write(socket, asio::buffer(messagesToSend.front().body.data(), messagesToSend.front().head.size),
                [this](std::error_code ec, std::size_t length){
                    if(!ec){
                            messagesToSend.pop();
                            if(!messagesToSend.empty())
                                WriteHeader();
                    }else{
                        std::cerr << "Eror during write: "  << ec.message() << "\n";
                        Disconnect();
                    }
                }
            );
        }

        void AddMessageQeueue(){
            if(id != 0){
                ClientMessage<T> cl(this->shared_from_this(), tempMessageRead);
                receivedMessages.push(cl);
            }else{
                ClientMessage<T> cl(nullptr, tempMessageRead);
                receivedMessages.push(cl);
            }
            ReadHeader();
        }

};


#endif