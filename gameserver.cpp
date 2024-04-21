

#include "network/server.h"
#include "common.h"
#include "tetris.h"
#include <queue>

using namespace std;

class server : public server_interface<MessageType>{

    int randSeed = 0;

    public:
        int playersReady = 0;
        tetris player1;
		tetris player2;

        int player1LastInput = -1;
        int player2LastInput = -1;

        uint32_t p1Id;
        uint32_t p2Id;

        queue<PlayerStateInBetween> player1StatesInBetween;
        queue<PlayerStateInBetween> player2StatesInBetween;

        server(){
            player1 = tetris();
			player2 = tetris();
        }

        void PLayer1Lose(){
            Message<MessageType> newMsg;
            newMsg.head = {MessageType::Lose, 0};
            Send(p1Id, newMsg);
            newMsg.head.id = MessageType::Win;
            Send(p2Id, newMsg);
        }

        void Player2Lose(){
            Message<MessageType> newMsg;
            newMsg.head = {MessageType::Lose, 0};
            Send(p2Id, newMsg);
            newMsg.head.id = MessageType::Win;
            Send(p1Id, newMsg);
        }

        void SendGameState(){

            PlayerState ps1 = {player1.nCurrentPiece, player1.nCurrentRotation, player1.nCurrentX, player1.nCurrentY, player1.nScore, player1.bRotateHold, player1LastInput};
            PlayerState ps2 = {player2.nCurrentPiece, player2.nCurrentRotation, player2.nCurrentX, player2.nCurrentY, player2.nScore, player2.bRotateHold, player2LastInput};

            // Send Rival states in between
            while(player2StatesInBetween.size() > 1){
                Message<MessageType> msg;
                msg.head = {MessageType::RivalStateInBetween, 0};
                msg << player2StatesInBetween.front();
                player2StatesInBetween.pop();
                Send(p1Id, msg);
            }
            player2StatesInBetween.pop();
            while(player1StatesInBetween.size() > 1){
                Message<MessageType> msg;
                msg.head = {MessageType::RivalStateInBetween, 0};
                msg << player1StatesInBetween.front();
                player1StatesInBetween.pop();
                Send(p2Id, msg);
            }
            player1StatesInBetween.pop();
            
            if(player1.fieldUpdate){
                PlayerStateField psf1;
                psf1.playerState = ps1;
                if(player2.fieldUpdate){ // G_F
                    PlayerStateField psf2;
                    psf2.playerState = ps2;
                    for(int i = 0; i < player2.nFieldWidth * player2.nFieldHeight; i++){
                        psf1.field[i] = player1.pField[i];
                        psf2.field[i] = player2.pField[i];
                    }
                    // SEND MESSAGE
                    Message<MessageType> p1Msg;
                    p1Msg.head = {MessageType::GStateField, 0};
                    p1Msg << psf1;
                    p1Msg << psf2;
                    Send(p1Id, p1Msg);

                    Message<MessageType> p2Msg;
                    p2Msg.head = {MessageType::GStateField, 0};
                    p2Msg << psf2;
                    p2Msg << psf1;
                    Send(p2Id, p2Msg);
                }
                else{ // G_1_0
                    for(int i = 0; i < player2.nFieldWidth * player2.nFieldHeight; i++){
                        psf1.field[i] = player1.pField[i];
                    }
                    // SEND MESSAGE
                    Message<MessageType> p1Msg;
                    p1Msg.head = {MessageType::GStateField_1_0, 0};
                    p1Msg << psf1;
                    p1Msg << ps2;
                    Send(p1Id, p1Msg);

                    Message<MessageType> p2Msg;
                    p2Msg.head = {MessageType::GStateField_0_1, 0};
                    p2Msg << ps2;
                    p2Msg << psf1;
                    Send(p2Id, p2Msg);
                }
            }
            else{
                if(player2.fieldUpdate){ // G_0_1
                    PlayerStateField psf2;
                    psf2.playerState = ps2;
                    for(int i = 0; i < player2.nFieldWidth * player2.nFieldHeight; i++){
                        psf2.field[i] = player2.pField[i];
                    }
                    // SEND MESSAGE
                    Message<MessageType> p1Msg;
                    p1Msg.head = {MessageType::GStateField_0_1, 0};
                    p1Msg << ps1;
                    p1Msg << psf2;
                    Send(p1Id, p1Msg);

                    Message<MessageType> p2Msg;
                    p2Msg.head = {MessageType::GStateField_1_0, 0};
                    p2Msg << psf2;
                    p2Msg << ps1;
                    Send(p2Id, p2Msg);
                }
                else{// G_S
                    // SEND MESSAGE
                    Message<MessageType> p1Msg;
                    p1Msg.head = {MessageType::GState, 0};
                    p1Msg << ps1;
                    p1Msg << ps2;
                    Send(p1Id, p1Msg);

                    Message<MessageType> p2Msg;
                    p2Msg.head = {MessageType::GState, 0};
                    p2Msg << ps2;
                    p2Msg << ps1;
                    Send(p2Id, p2Msg);
                }
            }

            player1.fieldUpdate = false;
            player2.fieldUpdate = false;
           
            player1LastInput = -1;
            player2LastInput = -1;
        }

	protected:
        void HandleMessage(std::shared_ptr<connection<MessageType>> client, Message<MessageType>& msg) override{
            switch(msg.head.id){
                case MessageType::Ready:
                    playersReady++;
                    if(playersReady == 2){
                        p2Id = client->GetId();
                        HandleReadyMessage(client);
                    }
                    else{
                        p1Id = client->GetId();
                    }
                    break;
                case MessageType::Input:
                    HandleInputMessage(client, msg);
                    break;
            }
        }
        bool OnClientConnect(std::shared_ptr<connection<MessageType>> connection) override{
            if(playersReady >= 2 || ConnectionAmount() >= 2){
                return false;
            }
            return true;
        }

    private:
        void HandleInputMessage(std::shared_ptr<connection<MessageType>> client, Message<MessageType>& msg){
            InputBody ib;
            msg >> ib;

            if(client->GetId() == p1Id){
                player1LastInput = ib.requestOrder;
                bool zeroInput = true;
                for(int i = 0; i < 4; i++){
                    player1.bKey[i] = ib.inputs[i];
                    if(!ib.inputs[i])
                        zeroInput = false;
                }
                player1.HandleInput();
                if(!zeroInput){
                    PlayerStateInBetween ps = {player1.nCurrentX, player1.nCurrentY, player1.nCurrentRotation};
                    player1StatesInBetween.push(ps);
                }
            }else{
                player2LastInput = ib.requestOrder;
                bool zeroInput = true;
                for(int i = 0; i < 4; i++){
                    player2.bKey[i] = ib.inputs[i];
                    if(!ib.inputs[i])
                        zeroInput = false;
                }
                player2.HandleInput();
                if(!zeroInput){
                    PlayerStateInBetween ps = {player2.nCurrentX, player2.nCurrentY, player2.nCurrentRotation};
                    player2StatesInBetween.push(ps);
                }
            }
        }

        void HandleReadyMessage(std::shared_ptr<connection<MessageType>> client){
            Message<MessageType> newMsg;
            newMsg.head = {MessageType::Start, 0};
            SendAll(newMsg);
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

    int speedUp = 360;
	int speedUpCounter = 0;
    bool bGameOver1 = false;
    bool bGameOver2 = false;

    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

    while(1){
        sv.Update();
        if(sv.playersReady >= 2)
            break;
    }

    while(1){
        this_thread::sleep_for(200ms);
        sv.Update();

		speedUpCounter++;
		if(speedUpCounter == speedUp){
			speedUpCounter = 0;
			if(sv.player1.speed >= 5)
				sv.player1.speed--;
            if(sv.player2.speed >= 5)
                sv.player2.speed--;
		}

		int score = sv.player1.nScore;

        for(int i = 0; i < 4; i++){
            if(bGameOver1)
                break;
            sv.player1.Update(bGameOver1);
        }
		if(sv.player1.nScore - score > 25)
			sv.player2.nScore -= (sv.player1.nScore - score - 25);
			
        score = sv.player2.nScore;
        for(int i = 0; i < 4; i++){
            if(bGameOver2)
                break;
            sv.player2.Update(bGameOver2);
        }
		if(sv.player2.nScore - score > 25)
			sv.player1.nScore -= (sv.player2.nScore - score - 25);

        sv.SendGameState();

        if(bGameOver1)
            sv.PLayer1Lose();
        else if(bGameOver2)
            sv.Player2Lose();
            
        if(!bGameOver1 && !bGameOver2){
            if(sv.player1.nScore <= -1000 && sv.player1.nScore < sv.player2.nScore)
                sv.PLayer1Lose();
            
            if(sv.player2.nScore <= -1000 && sv.player2.nScore < sv.player1.nScore)
                sv.Player2Lose();
        }
    
    }

    return 0;
}
