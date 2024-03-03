#ifndef MESSAGE_H
#define MESSAGE_H

#include <vector>

template<typename T>
struct MessageHead{
    T id{};
    uint32_t size;
};

template<typename T>
class Message{
    public:
        MessageHead<T> head{};
        std::vector<uint8_t> body;

        size_t size() const{
            return body.size();
        }

        template<typename DataT>
        friend Message<T>& operator<<(Message<T>& msg, const DataT& data){

            size_t i = msg.body.size();

            msg.body.resize(msg.body.size() + sizeof(data));
            
            std::memcpy(msg.body.data() + i, &data, sizeof(data));

            msg.head.size = msg.body.size();

            return msg;
        }

        template<typename DataT>
        friend Message<T>& operator>>(Message<T>& msg, DataT& data){

            size_t i = msg.body.size() - sizeof(data);

            std::memcpy(&data, msg.body.data() + i, sizeof(data));

            msg.body.resize(i);

            msg.head.size = msg.body.size();

            return msg;
        }
};

template <typename T>
class connection;

template<typename T>
class ClientMessage{
    public: 
        std::shared_ptr<connection<T>> client;
        Message<T> message;

        ClientMessage(std::shared_ptr<connection<T>> cl, const Message<T>& msg){
            client = cl;
            message = msg;
        }  
};

#endif