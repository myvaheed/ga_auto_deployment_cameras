#pragma once


#include <stdexcept>
#include <iostream>
#include "omp.h"

#define INDEX2D(i,j) (i) * WN + (j)
#define MAP(i,j) (map[INDEX2D(i,j)])
#define MAPCAM(i,j) (mapWithCameras[INDEX2D(i,j)])

class Cell {
public:
	float x = 0;
	float y = 0;
// значение клетки
	int value = 0;

	static int BUILDING_ID;
	static int OVERLAP_ID;
	static int EMPTY_ID;

	void init(float x, float y) {
		value = 0;
		this->x = x;
		this->y = y;
	}
	~Cell() {

	}
};


