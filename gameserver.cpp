

#include "network/server.h"
#include "common.h"
#include "tetris.h"

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
            GameStateField gs1;
            GameStateField gs2;

            gs1.player.playerState = {player1.nCurrentPiece, player1.nCurrentRotation, player1.nCurrentX, player1.nCurrentY, player1.nScore, player1.bRotateHold, player1LastInput};
            gs2.player.playerState = {player2.nCurrentPiece, player2.nCurrentRotation, player2.nCurrentX, player2.nCurrentY, player2.nScore, player2.bRotateHold, player2LastInput};

            for(int i = 0; i < player1.nFieldWidth * player1.nFieldHeight; i++){
				gs1.player.field[i] = player1.pField[i];
                gs2.player.field[i] = player2.pField[i];
			}

            gs1.rival.playerState = {player2.nCurrentPiece, player2.nCurrentRotation, player2.nCurrentX, player2.nCurrentY, player2.nScore, player2.bRotateHold, player2LastInput};
            gs2.rival.playerState = {player1.nCurrentPiece, player1.nCurrentRotation, player1.nCurrentX, player1.nCurrentY, player1.nScore, player1.bRotateHold, player1LastInput};
            for(int i = 0; i < player2.nFieldWidth * player2.nFieldHeight; i++){
				gs1.rival.field[i] = player2.pField[i];
                gs2.rival.field[i] = player1.pField[i];
			}

            Message<MessageType> p1Msg;
            p1Msg.head = {MessageType::GStateField, 0};
            p1Msg << gs1;
            Send(p1Id, p1Msg);

            Message<MessageType> p2Msg;
            p2Msg.head = {MessageType::GStateField, 0};
            p2Msg << gs2;
            Send(p2Id, p2Msg);

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
            //Message<MessageType> newMsg;
            //newMsg.head = {MessageType::InputResponse, 0};
            //PlayerState ps;

            if(client->GetId() == p1Id){
                player1LastInput = ib.requestOrder;
                for(int i = 0; i < 4; i++){
                    player1.bKey[i] = ib.inputs[i];
                }
                player1.HandleInput();
                /*ps = {
                    player1.nCurrentPiece,
                    player1.nCurrentRotation,
                    player1.nCurrentX,
                    player1.nCurrentY,
                    player1.nScore,
                    player1.bRotateHold,
                };*/
            }else{
                player2LastInput = ib.requestOrder;
                for(int i = 0; i < 4; i++){
                    player2.bKey[i] = ib.inputs[i];
                }
                player2.HandleInput();
                /*ps = {
                    player2.nCurrentPiece,
                    player2.nCurrentRotation,
                    player2.nCurrentX,
                    player2.nCurrentY,
                    player2.nScore,
                    player2.bRotateHold,
                };*/
            }
            //newMsg << ps;
            //Send(client, newMsg);
            //newMsg.head.id = MessageType::RivalState;
            //SendAll(newMsg, client);
        }

        void HandleReadyMessage(std::shared_ptr<connection<MessageType>> client){
            Message<MessageType> newMsg;
            newMsg.head = {MessageType::Start, 0};
            SendAll(newMsg);
            //newMsg << randSeed;
            //SendAll(newMsg, client);
            //newMsg >> randSeed;
            //randSeed += 1;
            // << randSeed;
            //Send(client, newMsg);
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
        //this_thread::sleep_for(50ms);
        this_thread::sleep_for(300ms);
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

        for(int i = 0; i < 6; i++){
            if(bGameOver1)
                break;
            sv.player1.Update(bGameOver1);
        }
		if(sv.player1.nScore - score > 25)
			sv.player2.nScore -= (sv.player1.nScore - score - 25);
			
        score = sv.player2.nScore;
        for(int i = 0; i < 6; i++){
            if(bGameOver2)
                break;
            sv.player2.Update(bGameOver2);
        }
		//sv.player2.Update(bGameOver2);
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

        //sv.Update();	       
    }

    return 0;
}
