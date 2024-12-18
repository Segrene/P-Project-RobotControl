#include "CRD.h"

CRD::CRD(double x, double y, double z, double rx, double ry, double rz) { Coord[0] = { x,y,z,rx,ry,rz }; Point = 0; Set = true; }
CRD::CRD(double coord[]) {
	for (int i = 0; i < 6; i++) {
		Coord[0][i] = coord[i];
	}
	Point = 0;
	Set = true;
}
CRD::CRD(array<double, 6> coord) {
	Coord[0] = coord;
	Point = 0;
	Set = true;
}

void CRD::setOrigin(double x, double y, double z, double rx, double ry, double rz) { Coord[0] = { x,y,z,rx,ry,rz }; Set = true; }
void CRD::addPoint(double x, double y, double z, double rx, double ry, double rz) { Coord.push_back({ x,y,z,rx,ry,rz }); Point++; }
void CRD::setOrigin(double coord[6]) {
	for (int i = 0; i < 6; i++) {
		Coord[0][i] = coord[i];
	}
	Set = true;
}
void CRD::addPoint(double coord[6]) {
	array<double, 6> addPoint = { 0 };
	for (int i = 0; i < 6; i++) {
		addPoint[i] = coord[i];
	}
	Coord.push_back(addPoint);
	Point++;
}
void CRD::setOrigin(string coord) {
	istringstream ss(coord);
	array<string, 6> coordString;
	int i = 0;
	string buffer;
	while (getline(ss, buffer, ',')) {
		coordString[i] = buffer;
		i++;
	}
	for (int j = 0; j < 6; j++) {
		Coord[0][j] = stod(coordString[j]);
	}
	Set = true;
}
void CRD::addPoint(string coord) {
	array<double, 6> addPoint = { 0 };
	istringstream ss(coord);
	array<string, 6> coordString;
	int i = 0;
	string buffer;
	while (getline(ss, buffer, ',')) {
		coordString[i] = buffer;
		i++;
	}
	for (int j = 0; j < 6; j++) {
		addPoint[j] = stod(coordString[j]);
	}
	Coord.push_back(addPoint);
	Point++;
}
void CRD::setOrigin(array<double, 6> coord) {
	Coord[0] = coord;
	Set = true;
}
void CRD::addPoint(array<double, 6> coord) {
	Coord.push_back(coord);
	Point++;
}
void CRD::editPoint(int pointNum, int coordNum, double coord) {
	if (pointNum > Point) { throw out_of_range("vector 범위 초과"); }
	Coord[pointNum][coordNum] = coord;
}
int CRD::deletePoint() {
	if (Point < 1) { return -1; }
	Coord.pop_back();
	Point--;
	return 0;
}
string CRD::getPointString(int point) {
	if (point > Point) { throw out_of_range("vector 범위 초과"); }
	string coordString;
	for (int i = 0; i < 5; i++) {
		coordString += to_string(Coord[point][i]);
		coordString += ", ";
	}
	coordString += to_string(Coord[point][5]);
	coordString += "\n";
	return coordString;
}
void CRD::getPoint(int point, double* coord) {
	if (point > Point) { throw out_of_range("vector 범위 초과"); }
	for (int i = 0; i < 6; i++) {
		coord[i] = Coord[point][i];
	}
}
int CRD::getPointCount() {
	return Point;
}
bool CRD::isSet() {
	return Set;
}
void CRD::Clear() {
	Coord.clear();
	Coord.push_back({ 0,0,0,0,0,0 });
	Point = 0;
	Set = false;
}
void CRD::makeLifting() {
	if (Point < 1) { cout << "좌표수 부족" << endl; return; }
	array<double, 6> lift1 = Coord[Point];
	array<double, 6> lift2 = Coord[0];
	lift1[2] += 30;
	lift2[2] += 30;
	this->addPoint(lift1);
	this->addPoint(lift2);
}
void CRD::makeInterval() {
	if (Point >= 1) { cout << "단일 좌표 아님" << endl; return; }
	array<double, 6> Inter1 = Coord[Point];
	array<double, 6> Inter2 = Coord[Point];
	Inter1[1] -= 20;
	Inter2[1] += 20;
	this->Clear();
	this->setOrigin(Inter1);
	this->addPoint(Inter2);
}
int CRD::validation() {
	if (this->isSet() == false) {
		cout << "좌표 없음" << endl;
		return 1;
	}
	array<double, 6> check;
	for (int i = 0; i <= Point; i++) {
		if (Coord[i][0] < 280 || Coord[i][0] > 580) { return -1; }
		if (Coord[i][1] < -150 || Coord[i][1] > 150) { return -1; }
		if (Coord[i][2] < 50 || Coord[i][2] > 300) { return -1; }
	}
	return 0;
}