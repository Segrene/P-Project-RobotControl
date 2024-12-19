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
bool SerialSend(Serial* SP, string Message);
string SerialRecv(Serial* SP);
string ZMQRecvMessage(zmq::socket_t& ZMQSocket); //ZMQ(카메라) 메세지 수신
void ZMQSetCoord(CRD& Coord); //서버에서 좌표 확인
void ManualSetCoord(CRD& Coord); //수동으로 좌표 입력(1개)
int Move(SOCKET& SCKT, string Point); //로봇 이동
int Verify(string String); //정상 동작 확인
int Menu(bool isSet, bool eFlag);
int GetInt(); //정수 입력
void SetCRD(CRD& Coord); //좌표를 설정함, 자동, 수동설정이 있음
void GetCRD(CRD& Coord); //좌표를 확인함, 엔터를 두번 눌러 탈출 가능
int AutoMode(SOCKET& SCKT, CRD& Coord, string ROrigin, int loopCount = 0, bool LiftFlag = false, bool ManualSelect = false); //주어진 좌표를 순회함, 기본적으로 루프 횟수 수동 설정, 들어올리지 않음, 들어올림 여부 수동 설정
int HoldMode(SOCKET& SCKT, CRD& Coord, string ROrigin, int HoldTime = 0); //주어진 좌표에서 대기함, 기본적으로 대기시간 수동 설정
int GrabMode(SOCKET& SCKT, Serial* SP, string ROrigin, CRD& GCRD); //도구를 잡는 함수
int ThrowMode(SOCKET& SCKT, Serial* SP, string ROrigin, CRD& GCRD); //도구를 버리는 함수
int ScenarioMode(SOCKET& SCKT, Serial* SP, CRD& Coord, string ROrigin, CRD& GCRD, CRD& TCRD); //시나리오대로 작동하는 모드, 현제로써는 1개의 시나리오만 가능
void EndTask(SOCKET& SCKT); //로봇을 종료시키고 프로그램을 종료함
int getCommand(); //키 입력 감지
void inputClear();

int main()
{
	cout << "Robot Control" << endl;
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
	// 소켓 구성 종료

	//시리얼 구성 시작
	bool eFlag = false;
	Serial* SP = new Serial("\\\\.\\com20"); //1:잡기, 2:놓기, 3:분사1, 4:분사2
	if (SP->IsConnected()) {
		cout << "엔드 이펙터 연결됨" << endl;
		eFlag = true;
	}
	else {
		cout << "엔드 이펙터 연결되지 않음" << endl;
		eFlag = false;
	}
	//시리얼 구성 종료

	int state = 0; // -1 : 강제종료, 0 : 정상종료, 1 : 특수상황
	string RobotMode = "";
	string RobotOrigin = "";

	cout << "M0609 : " << RecvMsg(SCKT); //연결 상황 확인용
	SendMsg(SCKT, "wait"); //로봇 통신 동기화용 메세지
	RobotMode = RecvMsg(SCKT); //현제 로봇의 작동 상태를 확인
	if (RobotMode.compare("BaseABS\r\n") == 0) {
		RobotOrigin = "367,6,278,0,180,0";
	}
	else if (RobotMode.compare("BaseTrans\r\n") == 0) {
		RobotOrigin = "0,0,0,0,0,0";
	}
	cout << "RobotMode : " << RobotMode;
	cout << "RobotOrigin : " << RobotOrigin;

	Sleep(1000);

	CRD Coord;
	CRD& Crd1 = Coord;
	Coord.Clear();
	bool CoordSet = false;

	CRD GrabCRD;
	CRD& GCRD = GrabCRD;
	vector<array<double, 6>> GCD({ {367,6,278,0,180,0}, {450, -100, 278,0,180,0}, {450, -100, 128,0,180,0}, {450, -100, 278,0,180,0},{367,6,278,0,180,0} });
	GrabCRD.setVector(GCD);

	CRD ThrowCRD;
	CRD& TCRD = ThrowCRD;
	vector<array<double, 6>> TCD({ {367,6,278,0,180,0}, {450, 100, 278,0,180,0}, {450, 100, 128,0,180,0}, {450, 100, 278,0,180,0},{367,6,278,0,180,0} });
	ThrowCRD.setVector(TCD);

	while (true) {
		if (state != 0) {
			cout << "이상 실행 감지됨" << endl;
			break;
		}
		CoordSet = Coord.isSet();
		system("cls");
		cout << "작동 모드 : " << RobotMode;
		cout << "로봇 원점 : " << RobotOrigin << endl;
		if (CoordSet == false) { cout << "좌표 설정되지 않음" << endl; } //좌표가 설정되지 않을 경우 작동 거부
		if (eFlag == false) { cout << "엔드이펙터 연결되지 않음" << endl; }
		switch (Menu(CoordSet, eFlag)) { //메뉴 선택
		case 0: SetCRD(Crd1); continue;
		case 1: if (CoordSet) { GetCRD(Crd1); } continue;
		case 2: if (CoordSet) { state = AutoMode(SCKT, Crd1, RobotOrigin, 0, false, true); } continue;
		case 3: if (CoordSet) { state = HoldMode(SCKT, Crd1, RobotOrigin, 0); } continue;
		case 4: if (eFlag) { state = GrabMode(SCKT, SP, RobotOrigin, GCRD); } continue;
		case 5: if (eFlag) { state = ThrowMode(SCKT, SP, RobotOrigin, TCRD); } continue;
		case 6: if (CoordSet == true && eFlag == true) { state = ScenarioMode(SCKT, SP, Crd1, RobotOrigin, GCRD, TCRD); } continue;
		case 7: EndTask(SCKT); break;
		default: continue;
		}
		break;
	}
	cout << "RecvMsg : " << RecvMsg(SCKT); //종료 메세지

	closesocket(hClient);
	closesocket(hListen);
	WSACleanup();
	SP->~Serial();
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
bool SerialSend(Serial* SP, string Message) {
	char sMsg[255] = "";
	strcpy_s(sMsg, Message.c_str());
	return SP->WriteData(sMsg, 255);
}
string SerialRecv(Serial* SP) {
	char incomingData[255] = "";
	int readResult = 0;
	readResult = SP->ReadData(incomingData, 256);
	incomingData[readResult] = 0;
	string SerialMessage(incomingData);
	return SerialMessage;
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
int Menu(bool isSet, bool eFlag) {
	cout << "동작 선택" << endl;
	cout << "0. 좌표 설정" << endl;
	if (isSet == true) {
		cout << "1. 좌표 확인" << endl << "2. 자동 모드" << endl << "3. 대기 모드" << endl;
	}
	if (eFlag == true) {
		cout << "4. 도구 잡기" << endl << "5. 도구 놓기" << endl;
	}
	if (isSet == true && eFlag == true) {
		cout << "6. 시나리오 모드" << endl;
	}
	cout << "7. 종료" << endl << "Select : ";
	int m = GetInt();
	inputClear();
	return m;
}
int GetInt() {
	int i;
	while (true) {
		cin >> i;
		if (cin.fail() == 1) {
			cout << "비정상 입력" << endl;
			inputClear();
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
		cout << "비정상 좌표 확인, 좌표 삭제" << endl;
		Coord.Clear();
		while (true) {
			if (getCommand() != -1) {
				break;
			}
			Sleep(100);
		}
		inputClear();
	}
}
void GetCRD(CRD& Coord) {
	for (int i = 0; i <= Coord.getPointCount(); i++) {
		cout << "좌표 " << i + 1 << " : " << Coord.getPointString(i) << endl;
	}
	while (true) {
		if (getCommand() != -1) {
			break;
		}
		Sleep(100);
	}
	inputClear();
}
int AutoMode(SOCKET& SCKT, CRD& Coord, string ROrigin, int Loop, bool LiftFlag, bool ManualSelect) {
	int Generated = 0;
	if (Coord.getPointCount() < 1) {
		cout << "단일 좌표 간격 자동 생성" << endl;
		Coord.makeInterval();
		Generated++;
	}
	if (Loop == 0) {
		cout << "루프 횟수 : ";
		Loop = GetInt();
	}
	if (ManualSelect == true) {
		string sl;
		cout << "들어올리기(y/n) : ";
		cin >> sl;
		if (sl.compare("y") == 0 || sl.compare("Y") == 0) {
			LiftFlag = true;
		}
	}
	if (LiftFlag == true) {
		Coord.makeLifting();
		Generated += 2;
	}
	for (int i = 0; i <= Coord.getPointCount(); i++) {
		cout << Coord.getPointString(i) << endl;
	}
	cout << "\r" << "Loop Count : 0";
	for (int Count = 0; Count < Loop; Count++) {
		for (int i = 0; i <= Coord.getPointCount(); i++) { //i+1개의 자표를 순회함
			if (Move(SCKT, Coord.getPointString(i)) != 0) { return -1; }
		}
		cout << "\r" << "Loop Count : " << Count + 1;
	}
	if (Move(SCKT, ROrigin) != 0) { return -1; }
	for (int i = 0; i < Generated; i++) {
		Coord.deletePoint();
	}
	return 0;
}
int HoldMode(SOCKET& SCKT, CRD& Coord, string ROrigin, int HoldTime) {
	cout << Coord.getPointString(0) << endl;
	if (HoldTime <= 0) {
		cout << "대기 시간(ms) : ";
		cin >> HoldTime;
	}
	if (HoldTime < 0) { HoldTime = 0; } //HoldTime이 음수인 경우 0으로 변경
	if (Move(SCKT, Coord.getPointString(0)) != 0) { return -1; }
	Sleep(HoldTime);
	if (Move(SCKT, ROrigin) != 0) { return -1; }
	return 0;
}
int GrabMode(SOCKET& SCKT, Serial* SP, string ROrigin, CRD& GCRD) {
	for (int i = 0; i < 3; i++) {
		Move(SCKT, GCRD.getPointString(i));
	}
	string status;
	if (SerialSend(SP, "1\n")) {
		status = SerialRecv(SP);
		cout << status << endl;
	}
	else {
		cout << "엔드이펙터 통신 실패" << endl;
		return -1;
	}
	for (int i = 3; i < 5; i++) {
		Move(SCKT, GCRD.getPointString(i));
	}
	return 0;
}
int ThrowMode(SOCKET& SCKT, Serial* SP, string ROrigin, CRD& TCRD) {
	for (int i = 0; i < 3; i++) {
		Move(SCKT, TCRD.getPointString(i));
	}
	string status;
	if (SerialSend(SP, "2\n")) {
		status = SerialRecv(SP);
		cout << status << endl;
	}
	else {
		cout << "엔드이펙터 통신 실패" << endl;
		return -1; //시리얼 통신에 실패하면 
	}
	for (int i = 3; i < 5; i++) {
		Move(SCKT, TCRD.getPointString(i));
	}
	return 0;
}
int ScenarioMode(SOCKET& SCKT, Serial* SP, CRD& Coord, string ROrigin, CRD& GCRD, CRD& TCRD) {
	GrabMode(SCKT, SP, ROrigin, GCRD);
	AutoMode(SCKT, Coord, ROrigin, 5, true);
	ThrowMode(SCKT, SP, ROrigin, TCRD);
	return 0;
}
void EndTask(SOCKET& SCKT) {
	SendMsg(SCKT, "end");
}
int getCommand() { //실시간으로 키 입력을 받는 함수
	if (_kbhit()) {
		return _getch();
	}
	return -1;
}
void inputClear() {
	cin.clear();
	cin.ignore(INT_MAX, '\n'); //입력 버퍼 비우기, 원하지 않은 정체를 유발함
}