#include <iostream>
#include <pthread.h>
#include <queue>
#include <semaphore.h>
#include <string.h>
#include <stdlib.h>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <mariadb/conncpp.hpp>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#define BUF_SIZE 500
#define MAX_CLNT 500
#define PORT 7229
using namespace std;

int clnt_cnt=0;
int clnt_socks[MAX_CLNT];
static char msg[BUF_SIZE];//메세지통
string userID;
string userPW;
string userName;
string userGrade;
string userRegdate;
string bookname;
int rentAbleDays;
pthread_mutex_t mutx;

void * handle_clnt(void * arg);
void write_msg(int clnt_socks, const char* msg);
void error_handling(const char * msg);
void read_msg(int clnt_socks);

class DB{
private:
    sql::Connection* m_connection;
public:

    sql::PreparedStatement* prepareStatement(const string& query)
    {
        sql::PreparedStatement* stmt(m_connection->prepareStatement(query));
        return stmt;
    }
	sql::Statement* createStatement()
	{
        sql::Statement* st(m_connection->createStatement());
		return st;
	}
    void connect()
    {
        try
        {
            sql::Driver* driver = sql::mariadb::get_driver_instance();
            sql::SQLString url = "jdbc:mariadb://10.10.21.114:3306/library";    // db의 주소
            sql::Properties properties({{"user", "OPERATOR"}, {"password", "1234"}});
            m_connection = driver->connect(url,properties);
            cout << "DB 접속\n";  
        }
        catch(sql::SQLException& e){ cerr<<"DB 접속 실패: " << e.what() << endl;}
    }
};

class Member{
private:
    DB& mdb;


public:
    Member(DB& db) : mdb(db){}

	int idFind(const char* id) // 회원정보 테이블에 일치하는 아이디 찾는 함수 리턴값이 0이면 일치 X, 1이면 일치
    {
            int check = 0;
            const char* compareID;

            sql::PreparedStatement* stmnt = mdb.prepareStatement("select * from member where m_id = ?");
            stmnt->setString(1, id);

            sql::ResultSet *res = stmnt->executeQuery();
            while(res->next())
                compareID = res->getString(1);

            if (strcmp(compareID, id) == 0)
                check = 1;

            return check;  // 리턴값이 0이면 회원정보 테이블에 일치하는 아이디가 없는 경우, 1이면 있는 경우
    }

	int FindRentHistory(string id)
	{
			int check = 0;
			sql::PreparedStatement* count = mdb.prepareStatement("select count(*) from rentHistory where rh_mid = ?");
			count->setString(1, userID);

			sql::ResultSet *res = count->executeQuery();
			while(res->next())
				check = res->getInt(1);
			return check;
	}

	void grade()
	{
		try
		{
			int month, over, notover = 0;
			int check = 0;
			sql::PreparedStatement* mon = mdb.prepareStatement("select timestampdiff(month, m_reg, now()) from member where m_id = ?");
			mon->setString(1, userID);

			sql::ResultSet *m = mon->executeQuery();
			while(m->next())
				month = m->getInt(1);

			check = FindRentHistory(userID);

			if(check == 1)
			{
				sql::PreparedStatement* nover = mdb.prepareStatement
				("select count(*) from rentHistory where rh_mid = ? and rh_return is not null and rh_over = 0 group by rh_over");
				nover->setString(1, userID);

				sql::ResultSet *n = nover->executeQuery();
				while(n->next())
					notover = n->getInt(1);


				sql::PreparedStatement* ov = mdb.prepareStatement
				("select count(*) from rentHistory where rh_mid = ? and rh_return is not null and rh_over = 1 group by rh_over");
				ov->setString(1, userID);

				sql::ResultSet *o = ov->executeQuery();
				while(o->next())
					over = o->getInt(1);
				

				if (over == 0 && notover >= 10 && month >= 6 && userGrade != "우수회원")
				{
					sql::PreparedStatement* stmnt = mdb.prepareStatement("update member set m_grade = '우수회원' where m_id = ?");
					stmnt->setString(1, userID);

					sql::ResultSet *res = stmnt->executeQuery();
					rentAbleDays = 7;
					userGrade = "우수회원";
				}
				
				else if(over >= 3 && userGrade != "블랙회원")
				{
					sql::PreparedStatement* stmnt = mdb.prepareStatement("update member set m_grade = '블랙회원' where m_id = ?");
					stmnt->setString(1, userID);

					sql::ResultSet *res = stmnt->executeQuery();
					rentAbleDays = 0;
					userGrade = "블랙회원";
				}
			}
		}
		catch(sql::SQLException& e){std::cerr << "Error inserting new task: " << e.what() << std::endl;}
	}

	void Login(int clnt_sock)
    {
        try 
        {
            int str_len,check, count = 0;
            char sendmsg[BUF_SIZE];
            char readmsg[BUF_SIZE];
            string pw;
            const char* id;
            cout << "테스트 2 : " << msg <<endl;
            while(1)
            {
                for (int i = 0; i < 2; i++) 
                {
                    read_msg(clnt_sock);

                    if (i == 0) { id = msg; }
                    else if (i == 1) { pw = string(msg); }
                }
                check = idFind(id);

                if(check == 1)        // 아이디 일치 확인
                {
                    sql::PreparedStatement* stmnt = mdb.prepareStatement("select m_id, m_pw, m_name, m_grade, m_reg from member where m_id =  ?");
                    stmnt->setString(1, id);
					cout << id;
                    sql::ResultSet* res = stmnt->executeQuery();

                    while(res->next())
                    {
                        userID = res->getString(1);
                        userPW = res->getString(2);
                        userName = res->getString(3);
                        userGrade = res->getString(4);
                        userRegdate = res->getString(5);
                    }

                    if(userPW == pw)
                    {
						userID = id;
                        cout << "1. 로그인 성공" << endl;
                        grade(); // 회원등급 판단
						cout << id;
                        strcpy(msg, "1");
                        write_msg(clnt_sock, msg);

						
						cout << userID << "유저 아이디야" << endl;
                        break;
                    }
                    else if(userPW != pw)
                    {
                        cout << "2. 비밀번호 불일치" << endl;
                        strcpy(msg, "2");
                        write_msg(clnt_sock, msg);
                    }
                }
                else
                {
                    cout << "3. 아이디 불일치" << endl;
                    strcpy(msg, "3");
                    write_msg(clnt_sock, msg);
                }

            }
        }
        catch(sql::SQLException& e) {
            std::cerr << "Error during login: " << e.what() << std::endl;
        }
    }

    int ADD_ID(int clnt_sock)
    {

        int str_len, check = 0;
        const char* id;
        string pw, name, addr, ph;

        while(1)
        {
            cout << "아이디 생성 : " << endl;
            read_msg(clnt_sock);

            cout << msg << endl;
            id = msg;
            check = idFind(id);

            if(check == 0)
            {
                cout << "1. 아이디 사용 가능" << endl;
                strcpy(msg, "1");
                write_msg(clnt_sock, msg);
                break;
            }
            else
            {
                cout << "2. 아이디 사용 불가 다시 입력" << endl;
                strcpy(msg, "2");
                write_msg(clnt_sock, msg);
            }

        }

        for (int i = 0; i < 4; i++) 
        {
            read_msg(clnt_sock);

            if (i == 0) { pw = string(msg); }
            else if (i == 1) { name = string(msg); }
            else if (i == 2) { addr = string(msg); }
            else if (i == 3) { ph = string(msg); }
        }
          registration(id, pw, name, addr, ph);
        return clnt_sock;
    }
	
	void registration(string id, string pw, string name, string adr, string ph)
    {
        try
        {
            sql::PreparedStatement* stmnt = mdb.prepareStatement("insert into member (m_id, m_pw, m_name, m_addr, m_ph) values (?, ?, ?, ?, ?)");
            stmnt->setString(1, id);
            stmnt->setString(2, pw);
            stmnt->setString(3, name);
            stmnt->setString(4, adr);
            stmnt->setString(5, ph);
             
            sql::ResultSet *res = stmnt->executeQuery();
        }
        catch(sql::SQLException& e){std::cerr << "Error inserting new task: " << e.what() << std::endl;}
    }

	void search_book(int clnt_sock)
    {
        string writer;
		string plus;
		string inbook;

		read_msg(clnt_sock);
        inbook = string(msg);

        string query;
        try{
        sql::Statement* stmnt = mdb.createStatement();
        query = "select b_arch, b_name, b_writer, b_pbl, b_pdate, b_symbol from book where b_name like '%" + inbook + "%'";
        sql::ResultSet *res = stmnt->executeQuery(query);
            while (res->next())
            {
                inbook = "도서명 : " + res->getString(2) + " ";
                writer = " ( 작가 : " + res->getString(3) +")";

                plus = inbook + writer;
				strcpy(msg, plus.c_str());
				write_msg(clnt_sock, msg);

            }
            memset(msg, 0, BUF_SIZE);
			read(clnt_sock, msg, sizeof(msg));
			cout << msg;
			bookname = string(msg);
			cout << bookname;

			

			rent(clnt_sock);
			
        }
        catch(sql::SQLException& e){
          std::cerr << "검색 결과가 없습니다.\n " << e.what() << std::endl;
        }
    }

	void rent(int clnt_sock)
	{
		if (userID != " ")
		{
			string bookid;
			int i = 1;
			try
			{
			sql::PreparedStatement* stmnt1 = mdb.prepareStatement("select b_name, b_id from book where b_id = ?");
			stmnt1->setString(1, bookid);
			sql::ResultSet *res1 = stmnt1->executeQuery();
			cout << "2.5" << endl;
			if(res1->next())
			{
				bookname = (string)res1->getString(1);
				bookid = (string)res1->getString(2);

				strcpy(msg,res1->getString(1));
				write_msg(clnt_sock, msg);
				cout <<bookname << " " << bookid<<"\n";
			}
			cout << "3" << endl;
			sql::PreparedStatement* stmnt2 = mdb.prepareStatement("update rent set r_mid = ?, r_state = 1, r_exprt = now() , r_bookname = ? where r_bid = ?");
			cout << bookname << " 1 " << bookid << endl;
			stmnt2->setString(1, userID);
			stmnt2->setString(2, bookname);
			stmnt2->setString(3, bookid);
			cout << "4" << endl;
			stmnt2->executeQuery();

			sql::PreparedStatement* stmnt3 = mdb.prepareStatement
			("update rentHistory set rh_count = (select max(rh_count)+1 from rentHistory) where rh_mid = ? and rh_bid = ?");
			stmnt3->setString(1, userID);
			stmnt3->setString(2, bookid);

			stmnt3->executeQuery();
			cout << "5" << endl;
			}catch(sql::SQLException& e){
			std::cerr << "검색 결과가 없습니다.\n " << e.what() << std::endl;
			}
		}
		else
			strcpy(msg, "로그인 시에만 이용할 수 있는 기능입니다.");
			memset(msg, 0, BUF_SIZE);
			read(clnt_sock, msg, sizeof(msg));
	}


	void showReturnBook(int clnt_sock)
	{
		int check = 0;
		string num, bookname, rent, exprt, returndate, overdue;

		try
		{
			sql::PreparedStatement* stmnt1 = mdb.prepareStatement("select * from rentHistory where rh_mid = ?");
			stmnt1->setString(1, userID);

			sql::ResultSet *res = stmnt1->executeQuery();
			while(res->next())
			{
				num = (string)res->getString(2) + " 번 ";

				bookname = num + " [ " + res->getString(5) + " ] ";
				strcpy(msg, bookname.c_str());
				write_msg(clnt_sock, msg);

				rent = "대여 일 : " + res->getString(6) + " | ";
				strcpy(msg, rent.c_str());
				write_msg(clnt_sock, msg);

				exprt = " 반납 예정 일 : " + res->getString(7) + " | ";
				strcpy(msg, exprt.c_str());
				write_msg(clnt_sock, msg);

				returndate = " 반납 일 : " + res->getString(8) + " | ";
				strcpy(msg, returndate.c_str());
				write_msg(clnt_sock, msg);

				overdue = (string)res->getString(9);
				if(res->getInt(9) == 0)
					overdue = "X";

				else
					overdue = "O";
				overdue = overdue + " \n";
				strcpy(msg, overdue.c_str());
				write_msg(clnt_sock, msg);

				cout  << bookname << rent << exprt << returndate << overdue << endl;
			}

				//로직 추가어야 함
		}
		catch(sql::SQLException& e){std::cerr << "Error inserting new task: " << e.what() << std::endl;}
    }

	void returnbook(string bookid, int num)
	{
		int overday = 0;	//
		sql::PreparedStatement* stmnt1 = mdb.prepareStatement("update rentHistory set rh_return = now() where rh_mid = ? and rh_count = ?");
		stmnt1->setString(1, userID);
		stmnt1->setInt(2, num);

		stmnt1->executeQuery();

		sql::PreparedStatement* stmnt2 = mdb.prepareStatement
		("update rentHistory set rh_over = (select case when timestampdiff(day, rh_exprt, rh_return) > 0 then 1 else 0 end from rentHistory where rh_mid = ? and rh_count = ?) where rh_mid = ? and rh_count = ?");
		stmnt2->setString(1, userID);
		stmnt2->setInt(2, num);
		stmnt2->setString(3, userID);
		stmnt2->setInt(4, num);

		stmnt2->executeQuery();

		sql::PreparedStatement* stmnt3 = mdb.prepareStatement("select timestampdiff(day, rh_exprt, now()) from rentHistory where rh_mid = ? and rh_bid = ?");
		stmnt3->setString(1, userID);
		stmnt3->setString(2, bookid);
		
		sql::ResultSet *res = stmnt3->executeQuery();
		while(res->next())
			overday = res->getInt(1);
		if(overday >= 14)
		{
			sql::PreparedStatement* stmnt3 = mdb.prepareStatement("update member set m_grade = '블랙회원' where m_id = ?");
			stmnt3->setString(1, userID);
			sql::ResultSet *res = stmnt3->executeQuery();
			userGrade = "블랙회원";
			rentAbleDays = 0;
		}
	}

	
	void recombook(int clnt_sock)
    {
		string recbook;
		string recwriter;
		string recpbl;
		string query;
		string recplus;
		try{
		sql::Statement* stmnt = mdb.createStatement();
		query = "select b_name, b_writer, b_pbl from book order by rand() limit 5";
		sql::ResultSet *res = stmnt->executeQuery(query);
		while(res->next())
		{
			recbook = res->getString(1);
			recwriter = res->getString(2);
			recpbl = res->getString(3);
			cout << "\n";
			recplus = "도서명: " + recbook + " 작가: " + recwriter+ " 자료실 명: " + recpbl + "\n";
			strcpy(msg, recplus.c_str());
			write_msg(clnt_sock, msg);
		}
        read_msg(clnt_sock);
        cout << msg << endl;	
		}
		catch(sql::SQLException& e) {
        std::cerr << "Error during login: " << e.what() << std::endl;
    	}	
    }
};


int main(int argc, const char *argv[])
{

	int serv_sock, clnt_sock;
	struct sockaddr_in serv_adr, clnt_adr;
	socklen_t clnt_adr_sz;
	pthread_t t_id;

	pthread_mutex_init(&mutx, NULL);
	serv_sock=socket(PF_INET, SOCK_STREAM, 0);

	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family=AF_INET; 
	serv_adr.sin_addr.s_addr=htonl(INADDR_ANY);
	serv_adr.sin_port=htons(PORT);
	
	if(bind(serv_sock, (struct sockaddr*) &serv_adr, sizeof(serv_adr))==-1)
		error_handling("bind() error");
	if(listen(serv_sock, 5)==-1)
		error_handling("listen() error");
	
	while(1)
	{
		clnt_adr_sz=sizeof(clnt_adr);
		clnt_sock=accept(serv_sock, (struct sockaddr*)&clnt_adr,&clnt_adr_sz);
		
		pthread_create(&t_id, NULL, handle_clnt, (void*)&clnt_sock);
		pthread_detach(t_id);

		printf("Connected client IP: %s \n", inet_ntoa(clnt_adr.sin_addr));
	}
	close(serv_sock);
	return 0;
}


	void * handle_clnt(void * arg) //쓰레드 실행 함수
	{
		DB db;
		db.connect();
		Member mm(db);

		int clnt_sock=*((int*)arg);
		int str_len=0;

		
		while(1)
        {
			read_msg(clnt_sock);
			// string bookname = "세 개의 빛";
	
	
			if (strncmp(msg, "1",1) == 0) {
				mm.Login(clnt_sock);
				
			} 
			else if (strncmp(msg, "2",1) == 0) {
				mm.ADD_ID(clnt_sock);
			}
			else if (strncmp(msg, "3",1) == 0) {
				mm.search_book(clnt_sock);
				// mm.recombook(clnt_sock);
			}
			else if (strncmp(msg, "4",1) == 0) {
				cout <<"도서 랜트 실행";
				mm.rent(clnt_sock);
			}
			else if (strncmp(msg, "5",1) == 0) {
				cout <<"도서 반납 실행\n";
				// mm.returnbook(clnt_sock);
			}
			
			
		}

		pthread_mutex_lock(&mutx);

		for(int i=0; i<clnt_cnt; i++)  
		{
			if(clnt_sock==clnt_socks[i])
			{
				while(i++<clnt_cnt-1)
					clnt_socks[i]=clnt_socks[i+1];
				break;
			}
		}

		clnt_cnt--;

		pthread_mutex_unlock(&mutx);
		close(clnt_sock);
		return NULL;
	}


	void write_msg(int clnt_socks, const char* msg) //쓰기
	{
		write(clnt_socks, msg, strlen(msg));
		// memset(&msg, 0, BUF_SIZE);
	}


	void read_msg(int clnt_socks)//받기
	{
		int bytes_received;
		
			memset(msg, 0, BUF_SIZE);

			bytes_received = read(clnt_socks, msg, BUF_SIZE);

			if (bytes_received > 0)
			{
				msg[bytes_received] = '\0';
				cout << msg << endl;
			}
		
	}

	void error_handling(const char * msg)
	{
		fputs(msg, stderr);
		fputc('\n', stderr);
		exit(1);
	}
