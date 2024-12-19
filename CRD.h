#pragma once
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <array>

using namespace std;

class CRD {
private:
	bool Set = false;
	int Point = 0;
	vector<array<double, 6>> Coord = { {0,0,0,0,0,0} };
public:
	CRD() {}
	CRD(double x, double y, double z, double rx, double ry, double rz);
	CRD(double coord[]);
	CRD(array<double, 6> coord);
	void setOrigin(double x, double y, double z, double rx, double ry, double rz);
	void addPoint(double x, double y, double z, double rx, double ry, double rz);
	void setOrigin(double coord[6]);
	void addPoint(double coord[6]);
	void setOrigin(string coord);
	void addPoint(string coord);
	void setOrigin(array<double, 6> coord);
	void addPoint(array<double, 6> coord);
	void setVector(vector<array<double, 6>> coord);
	void editPoint(int pointNum, int axis, double coord);
	int deletePoint();
	string getPointString(int point);
	void getPoint(int point, double* coord);
	int getPointCount();
	bool isSet();
	void Clear();
	void makeLift();
	void makeInterval();
	void makeShift(int axis, double shift);
	int validation();
};