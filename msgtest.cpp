#include <iostream>

#include <unordered_map>
#include <vector>
#include <functional>
using namespace std;


/*
식별자

인자

*/

class Message{
public:
    
    virtual int getNum() const = 0;
    virtual vector<int> getMember() const = 0;
    virtual Message * Create() const = 0;
};

class Move : public Message {
public:

    //constexpr : 컴파일 타임에 값을 확정시킴
    static constexpr int num = 1;
    string name;
    static vector<int> a;

    int getNum() const override { return num; };

    vector<int> getMember() const override {
        return this->a;
    }

    Message * Create() const override { return new Move(); }
};


vector<int> Move::a = vector<int>();


// function<void(Message)> func;

// template<typename... Args> 

// unordered_map<int , Message> msg;


//메세지 직렬화 및 원격 프로시저 호출 프로토타입
class MessageManager{
private:
    //메세지 등록
    unordered_map<int , Message *> registry;

    //원격 메소드 호출 (특정 메세지가 들어오면 자동으로 실행될 함수)
    //서버에 저장된 정보를 바꾸기 위해선 서버 내부에서 public 함수를 작성해 등록해야한다.
    unordered_map<int , function<void(Message *)>> RPC;
public:
    
    void regist(Message * reg , function<void(Message *)> f){
        registry[reg->getNum()] = reg;
        RPC[reg->getNum()] = f;
    };

    //등록된 메세지 중 일치하는 메세지 타입으로 변경 (멤버 변수는 아무것도 건드리면 안됨)
    Message * Dispatch(const char* buffer, size_t size) const {
        char id = static_cast<char>(buffer[0]);

        auto it = registry.find(id)->second;

        Message * msg = it->Create();

        // 버퍼에서 직렬화한 데이터를 파싱 (가변형 데이터도 버퍼에 그대로 들어가 있을 수 있어서 그대로 옮기면 안된다.
        //적절한 처리과정이 필요함, string의 경우 offset은 딱 이름만큼의 8바이트가 들어가있다. 
        //버퍼에는 길이 + 내용이니까 이걸 string으로 먼저 바꿔 줄 필요가 있음)
        char * buf = (char * ) msg;

        //여기서 buf에 역직렬화
        // doSometing();
        //이미 동일한 메모리 주소에 값들이 들어갔기 때문에 굳이 캐스팅 하지 않아도 됨
        return msg;
    }

    //원격 프로시저 호출
    void CallRPC(Message * msg){
        RPC[msg->getNum()](msg);
    }

    
};

int main(){

    MessageManager * mm = new MessageManager();


    //원격 프로시저 등록
    mm->regist(new Move , [](Message * msg){
        Move * r = (Move * )msg;
        cout<<"Move Call!!"<<"\n";
        cout<<r->name<<"\n";
        for(int i = 0;i<r->getMember().size();i++){
            cout<<r->getMember()[i]<<" ";
        }
        return;
    });

    Move * m = new Move();
    m->name = "tjdnfls";
    Move::a.push_back(1);
    Move::a.push_back(1);
    Move::a.push_back(1);
    

    mm->CallRPC((Message *) m);

    // Move::a.push_back(1);
    // Move::a.push_back(2);
    // Move::a.push_back(3);

    // jump::a.push_back(4);
    // jump::a.push_back(5);
    // jump::a.push_back(6);


    // Message * msg1 = new Move();
    // Message * msg2 = new jump();

    // vector<int> mm = msg1->getMember();
    // vector<int> jj = msg2->getMember();
    
    // for(int i = 0;i<3;i++){
    //     cout<<mm[i]<<" ";
    // }
    // cout<<"\n";
    // for(int i = 0;i<3;i++){
    //     cout<<jj[i]<<" ";
    // }




    
    
    while(1){}
}