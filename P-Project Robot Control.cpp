#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "CRD.h"
#include <cstdlib>
#include <conio.h>
#include <WinSock2.h>
#include <zmq.hpp>
#include "SerialClass.h"
#include <stdexcept>

#pragma comment(lib, "ws2_32")

#define doosanIP "192.168.137.100"
#define serverIP "192.168.137.50"
#define PORT 200
#define PACKET_SIZE 1024

using namespace std;

string RecvMsg(SOCKET& SCKT); //소켓(로봇) 메세지 수신
void SendMsg(SOCKET& SCKT, string SendString); //소켓 메세지 송신
string ZMQRecvMessage(zmq::socket_t& ZMQSocket); //ZMQ(카메라) 메세지 수신
void ZMQSetCoord(CRD& Coord); //서버에서 좌표 확인
void ManualSetCoord(CRD& Coord); //수동으로 좌표 입력(1개)
int Move(SOCKET& SCKT, string Point); //로봇 이동
int Verify(string String); //정상 동작 확인
int Menu();
int GetInt(); //정수 입력
void SetCRD(CRD& Coord);
void GetCRD(CRD& Coord);
int AutoMode(SOCKET& SCKT, CRD& Coord, string ROrigin);
int HoldMode(SOCKET& SCKT, CRD& Coord, string ROrigin);
void EndTask(SOCKET& SCKT);

int main()
{
	WSADATA wsadata;
	if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0) {
		cout << "WSAStartup failed" << endl;
		return 1;
	}

	SOCKET hListen;
	hListen = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

	SOCKADDR_IN tListenAddr = {};
	tListenAddr.sin_family = AF_INET;
	tListenAddr.sin_port = htons(PORT);
	tListenAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	bind(hListen, (SOCKADDR*)&tListenAddr, sizeof(tListenAddr));
	listen(hListen, SOMAXCONN);

	SOCKADDR_IN tCIntAddr = {};
	int iCIntSize = sizeof(tCIntAddr);
	SOCKET hClient = accept(hListen, (SOCKADDR*)&tCIntAddr, &iCIntSize);
	SOCKET& SCKT = hClient;

	int state = 0; // -1 : 강제종료, 0 : 정상종료, 1 : 특수상황
	string RobotMode = "";
	string RobotOrigin = "";

	cout << "M0609 : " << RecvMsg(SCKT); //연결 상황 확인용
	RobotMode = RecvMsg(SCKT);
	if (RobotMode.compare("BaseABS\r\n") == 0) {
		RobotOrigin = "367,6,278,0,180,0";
	}
	else if (RobotMode.compare("BaseTrans\r\n") == 0) {
		RobotOrigin = "0,0,0,0,0,0";
	}
	cout << "RobotMode : " << RobotMode;
	cout << "RobotOrigin : " << RobotOrigin;

	CRD Coord;
	Coord.Clear();

	while (true) {
		if (state != 0) {
			cout << "이상 실행 감지됨" << endl;
			break;
		}
		system("cls");
		cout << "작동 모드 : " << RobotMode;
		cout << "로봇 원점 : " << RobotOrigin << endl;
		if (Coord.isSet() == false) { cout << "좌표 설정되지 않음" << endl; } //좌표가 설정되지 않을 경우 작동 거부
		switch (Menu()) { //메뉴 선택
		case 0: SetCRD(Coord); continue;
		case 1: GetCRD(Coord); continue;
		case 2: if (Coord.isSet()) { state = AutoMode(SCKT, Coord, RobotOrigin); } continue;
		case 3: if (Coord.isSet()) { state = HoldMode(SCKT, Coord, RobotOrigin); } continue;
		case 4: EndTask(SCKT); break;
		default: continue;
		}
		break;
	}
	cout << "RecvMsg" << RecvMsg(SCKT); //종료 메세지

	closesocket(hClient);
	closesocket(hListen);
	WSACleanup();
	return 0;
}

string RecvMsg(SOCKET & SCKT) {
	char cBuffer[PACKET_SIZE] = {}; //메세지 수신용 char 버퍼
	string RecvString = "";
	recv(SCKT, cBuffer, PACKET_SIZE, 0); //메세지 수신
	RecvString = cBuffer; //char 에서 string으로
	memset(cBuffer, 0, sizeof(char) * PACKET_SIZE); //char 버퍼 초기화
	return RecvString;
}
void SendMsg(SOCKET& SCKT, string SendString) {
	send(SCKT, SendString.c_str(), strlen(SendString.c_str()), 0);
}
string ZMQRecvMessage(zmq::socket_t& ZMQSocket) {
	cout << "RequestMessage" << endl;
	ZMQSocket.send(zmq::buffer("RequestMessage"), zmq::send_flags::none); //좌표 요청 메세지 전송
	zmq::message_t reply;
	ZMQSocket.recv(reply, zmq::recv_flags::none); //좌표값 수신
	string Received = reply.to_string(); //string으로 저장
	std::cout << "Received : " << Received << endl;
	return Received;
	return string();
}
void ZMQSetCoord(CRD& Coord) {
	zmq::context_t Context{ 1 };
	zmq::socket_t ZMQSocket(Context, ZMQ_REQ);
	zmq::socket_t& ZMQSKT = ZMQSocket;
	ZMQSocket.connect("tcp://localhost:5555");
	Coord.Clear();
	string RawCoord = ZMQRecvMessage(ZMQSKT);
	stringstream ss(RawCoord);
	vector<string> Points;
	string Point;
	int scount = 0;
	while (getline(ss, Point, ' ')) {
		Points.push_back(Point);
		cout << "좌표 " << ++scount << " : " << Point << endl;
	}
	Coord.setOrigin(Points[0]);
	for (int i = 1; i < Points.size(); i++) {
		Coord.addPoint(Points[i]);
	}
	ZMQSocket.close();
	Context.close();
}
void ManualSetCoord(CRD& Coord) {
	Coord.Clear();
	cout << "좌표 수동 입력" << endl;
	string point;
	cout << "좌표 1 : ";
	cin >> point;
	Coord.setOrigin(point);
	int Count = 2;
	while (true) {
		cout << "좌표 " << Count << " : ";
		cin >> point;
		if (point.compare("end") == 0) {
			cout << "좌표 입력 종료" << endl;
			break;
		}
		Coord.addPoint(point);
		Count++;
	}
}
int Move(SOCKET& hClient, string Point) {
	SendMsg(hClient, Point); //메세지 전달
	string Received = RecvMsg(hClient); //Ready대기
	if (Verify(Received)) { return -1; }
	return 0;
}
int Verify(string String) {
	if (String.compare("Ready\r\n") == 0) { //Ready가 아닌 메세지를 받을 경우 종료
		return 0;
	}
	else {
		cout << "비정상 동작 확인" << endl;
		return 1;
	}
}
int Menu() {
	cout << "동작 선택" << endl;
	cout << "0. 좌표 설정" << endl << "1. 좌표 확인" << endl << "2. 자동 모드" << endl << "3. 대기 모드" << endl << "4. 종료" << endl << "Select : ";
	return GetInt();
}
int GetInt() {
	int i;
	while (true) {
		cin >> i;
		if (!cin) {
			cout << "비정상 입력" << endl;
			cin.clear();
			cin.ignore(INT_MAX, '\n');
		}
		else {
			return i;
		}
	}
}
void SetCRD(CRD& Coord) {
	cout << "좌표 설정" << endl;
	cout << "0 : 자동 설정" << endl << "1 : 수동 설정" << endl;
	switch (GetInt()) {
	case 0: ZMQSetCoord(Coord); break;
	case 1: ManualSetCoord(Coord); break;
	default: ManualSetCoord(Coord); break;
	}
	if (Coord.validation() == -1) {
		cout << "비정상 좌표 확인" << endl;
		Coord.Clear();
	}
}
void GetCRD(CRD& Coord) {
	for (int i = 0; i <= Coord.getPointCount(); i++) {
		cout << "좌표 " << i + 1 << " : " << Coord.getPointString(i) << endl;
	}
}
int AutoMode(SOCKET& SCKT, CRD& Coord, string ROrigin) {
	int loop = 0;
	while (loop > 0) {
		cout << "루프 횟수 : ";
		loop = GetInt();
	}
	string sl;
	cout << "들어올리기(y/n) : ";
	cin >> sl;
	if (sl.compare("y") == 0 || sl.compare("Y") == 0) {
		Coord.makeLifting();
	}
	cout << "\r" << "Loop Count : 0";
	for (int Count = 0; Count < loop; Count++) {
		for (int i = 0; i <= Coord.getPointCount(); i++) { //i+1개의 자표를 순회함
			if (Move(SCKT, Coord.getPointString(i)) != 0) { return -1; }
		}
		cout << "\r" << "Loop Count : " << Count;
	}
	if (Move(SCKT, ROrigin) != 0) { return -1; }
	return 0;
}
int HoldMode(SOCKET& SCKT, CRD& Coord, string ROrigin) {
	cout << Coord.getPointString(0) << endl;
	int HoldTime;
	cout << "대기 시간(ms) : ";
	cin >> HoldTime;
	if (HoldTime < 0) { HoldTime = 0; } //HoldTime이 음수인 경우 0으로 변경
	if (Move(SCKT, Coord.getPointString(0)) != 0) { return -1; }
	Sleep(HoldTime);
	if (Move(SCKT, ROrigin) != 0) { return -1; }
	return 0;
}
void EndTask(SOCKET& SCKT) {
	SendMsg(SCKT, "end");
}