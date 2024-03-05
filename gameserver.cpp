

#include "network/server.h"
#include "common.h"

using namespace std;

class server : public server_interface<MessageType>{

    int playersReady = 0;
    int randSeed = 0;

	protected:
        void HandleMessage(std::shared_ptr<connection<MessageType>> client, Message<MessageType>& msg) override{
            switch(msg.head.id){
                case MessageType::Ready:
                    playersReady++;
                    if(playersReady == 2)
                        HandleReadyMessage(client);
                    break;
                case MessageType::Input:
                    SendAll(msg, client);
                    break;
                case MessageType::NewPiece:
                    SendAll(msg, client);
                    break;
                case MessageType::Damage:
                    SendAll(msg, client);
                    break;
                case MessageType::Lose:
                    HandleLoseMessage(client);
                    break;
            }
        }

    private:
        void HandleReadyMessage(std::shared_ptr<connection<MessageType>> client){
            Message<MessageType> newMsg;
            newMsg.head = {MessageType::Start, 0};
            newMsg << randSeed;
            SendAll(newMsg, client);
            newMsg >> randSeed;
            randSeed += 1;
            newMsg << randSeed;
            Send(client, newMsg);
        }

        void HandleLoseMessage(std::shared_ptr<connection<MessageType>> client){
            Message<MessageType> newMsg;
            newMsg.head = {MessageType::Win, 0};
            SendAll(newMsg, client);
        }
};

int main(){

    server sv;
    sv.Start();

    while(1){
        sv.Update();
    }

    return 0;
}
