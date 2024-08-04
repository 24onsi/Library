#include <iostream>
#include <pthread.h>
#include <queue>
#include <semaphore.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
	
#define BUF_SIZE 500
#define PORT 7229
using namespace std;
	

void error_handling(const char * msg);

void input_ADDID(int sock);
void Login(int sock);
void input_bookname(int sock);
void rent(int sock);
void returnbook(int sock);
void recom_book(int sock);

void write_msg(int sock, const char* msg);
void read_msg(int sock);

static char msg[BUF_SIZE];//메세지통

struct Userinfo{
    string username;
    string userID;
    string userGrade;
    string bookname;
    string bookID;
    int rentAbleDays;
} userinfo;
	
int main(int argc, const char *argv[])
{
	int sock;
	struct sockaddr_in serv_addr;
	pthread_t snd_thread, rcv_thread;
	void * thread_return;

	sock=socket(PF_INET, SOCK_STREAM, 0);
	
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family=AF_INET;
	serv_addr.sin_addr.s_addr=inet_addr(argv[1]);
	serv_addr.sin_port=htons(PORT);
	  
	if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))==-1)
		error_handling("connect() error");

   while(1)
    {
        cout << "로그인이 필요한 서비스입니다.\n1 : 로그인\n2 : 회원가입\n3 : 도서 조회\n4 : 도서 대여\n5 : 대여 도서 반납\n";
        // cout << "로그인이 필요한 서비스입니다.\n1 : 로그인\n2 : 회원가입\n3 : 도서 조회\n4 : 대여 기록\n";
        cout << "입력 : ";

        cin.getline(msg, BUF_SIZE);
        write_msg(sock,msg);
        // strcpy(msg,read_msg(sock));


        if (strcmp(msg, "1") == 0) {
            Login(sock);
        } 
        else if (strcmp(msg, "2") == 0) {
            input_ADDID(sock);
        }
        else if (strcmp(msg, "3") == 0) {
            input_bookname(sock);
        }
        else if (strcmp(msg, "4") == 0) {
            rent(sock);
        }
        else if (strcmp(msg, "5") == 0) {
            returnbook(sock);
        }
        // read_msg(sock);
    }

	close(sock);  
	return 0;
}

void input_ADDID(int sock)
{
	string inputData[5] = {"아이디", "비밀번호", "이름", "주소", "연락처"};

    while(1)
    {
        cout << "입력 " << inputData[0] << ": ";
        cin.getline(msg, BUF_SIZE);
        write_msg(sock, msg);

        // cout << msg << endl;

        read_msg(sock);

        if (strcmp(msg, "1") == 0)
        {
            cout << "사용 가능한 아이디입니다." << endl;
            break;
        }
        else// if (strcmp(msg, "2") == 0)
        {
            cout << "이미 사용중인 아이디입니다." << endl;
            cout << "다시 입력해주시길바랍니다." << endl;
        }
        cout << "\n";
    }

    for (int i = 1; i < 5; i++) {
        cout << "입력 " << inputData[i] << ": ";
        cin.getline(msg, BUF_SIZE);

        write_msg(sock, msg);
        cout << msg << endl;
    }
}

void Login(int sock)
{
    string inputData[2] = {"아이디", "비밀번호"};

    while(1)
    {
        for (int i = 0; i < 2; i++) {
            cout << "Enter " << inputData[i] << ": ";
            cin.getline(msg, BUF_SIZE);
            write_msg(sock, msg);

            cout << msg << endl;
        }

        read_msg(sock);

        if (strcmp(msg, "1") == 0) 
        {
            cout << "로그인 되었습니다." << endl;
            break;
        } 
        else if (strcmp(msg, "2") == 0) // 2 번은 불일치
        {
            cout << "비밀번호가 잘못되었습니다." << endl;
            cout << "확인 후 다시 입력해 주시길 바랍니다." << endl;
        } 
        else if (strcmp(msg, "3") == 0)
        {
            cout << "존재하지 않는 아이디입니다." << endl;
            cout << "확인 후 다시 입력해 주시길 바랍니다." << endl;
        }
        cout << "\n";
    }
} 

void input_bookname(int sock)
{
    cout << "도서 검색: ";
    cin.getline(msg, BUF_SIZE); 
    write_msg(sock, msg);
    cout <<"\n[검색 도서 결과입니다]\n";
    
    cout <<"===================================================================\n";
    for(int i = 0; i < sizeof(sock)+1; i++)
    {
        read_msg(sock);
    }
    cout <<"====================================================================\n";
    cout << "\n대여할 도서명을 정확하게 입력 부탁드립니다 : ";
    memset(msg, 0, BUF_SIZE);
    cin.getline(msg, BUF_SIZE); 
    write_msg(sock, msg);

    cout << "\n대여가 완료되었습니다.\n";

}

void returnbook(int sock)
{

    cout << " [ 대여 기록 ]" << endl;
    for(int i=0; i < sizeof(sock)+1 ; i++)
    {
        read_msg(sock);
    }

}

void rent(int sock)
{
    cout <<"\n[대여한 책 목록입니다]\n";

    read_msg(sock);
   
    cout <<"\n반납할 책 번호를 입력해 주세요\n";
    cin.getline(msg, BUF_SIZE);
    write_msg(sock, msg);
}

void recom_book(int sock)
{
    cout << "[추천 도서 목록]\n";

    for(int i=0; i < sizeof(sock); i++)
    {   
        read_msg(sock);
    }
    cout << "\n";
}



void error_handling(const char *msg)
{
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}




void write_msg(int sock, const char* msg)//쓰기
{
    write(sock, msg, strlen(msg));
}


void read_msg(int sock)//받기
{
    int bytes_received;

     memset(msg, 0, BUF_SIZE);

        bytes_received = read(sock, msg, BUF_SIZE);

        if (bytes_received > 0)
        {
            msg[bytes_received] = '\0';
            cout << msg << endl;
        }
    
}
