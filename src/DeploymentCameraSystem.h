#pragma once

#include "Cell.h"
#include "galgo2/Galgo.hpp"
#include "openga/genetic.hpp"
#include <fstream>
#include <ctime>
#include <cstdlib>
#include <stdio.h>
#include <vector>

#define TESTING 0
#define GALGO2 1

#define PPCAT_NX(A, B) A ## B
#define PPCAT(A, B) PPCAT_NX(A, B)

#define NBITS(N) PPCAT(BITS_,N)

#if !GALGO2

struct Genes {

	std::vector<float> x;

	std::string to_string() const {
		int numCamera = x.size() / 4;
		char str_temp[100 * numCamera];
		int offset = 0;
		for (int i = 0; i < numCamera; i++) {
			offset += snprintf(str_temp + offset, sizeof(str_temp) - offset, "x0[%d]=%g y0[%d]=%g nx[%d]=%g ny[%d]=%g, ",
					i, x[4*i], i, x[4*i + 1], i, x[4*i + 2], i, x[4*i + 3]);

		}
		snprintf(str_temp + offset, sizeof(str_temp) - offset, "\n");
		std::string str = str_temp;
		return str;
	}
};

struct MiddleCost {
	float countVision;
};

typedef EA::Genetic<Genes, MiddleCost> GA_Type;
typedef EA::GenerationType<Genes, MiddleCost> Generation_Type;

#endif

template<int N>
class DeploymentCameraSystem {

private:
	Cell* map;
	Cell* mapWithCameras;

	std::vector<int> buildingBorders;
	void* ga;

#if GALGO2
	static const int N_BITS = 64;

	std::vector<galgo::Parameter<float, N_BITS> > params;

#else


#endif

	float const PENALTY = 1;
	float const BONUS_BUILDING = 500;

	float alpha;
	float R;
	float width;
	float height;
	float h;

	int WN, HN;

	int numCamera;

	void initMap() {

		std::srand(unsigned(std::time(0)));

		for (int i = 0; i < HN; i++) {
			for (int j = 0; j < WN; j++) {
				float r = (float) rand() / RAND_MAX;
				if (r > 0.999) {
					int width = rand() % (WN / 5) + 1;
					int height = rand() % (HN / 5) + 1;
					for (int k = 0; k < width; k++) {
						for (int m = 0; m < height; m++) {
							if (i + m >= HN) continue;
							if (j + k >= WN) continue;
							if (MAP(i + m,j + k).value == Cell::EMPTY_ID) {
								MAP(i + m,j + k).value = Cell::BUILDING_ID;
								/*
								 * filling buildingBorders for initialization of camera on space
								 * */
								//left side
								if (k == 0) {
									if (j + k - 1 >= 0) {
										//left top corner
										if (m == 0 && i + m - 1 >= 0 && MAP(i + m - 1,j + k - 1).value == Cell::EMPTY_ID) {
											buildingBorders.push_back(INDEX2D(i + m - 1, j + k - 1));
										}
										//left bottom corner
										if (m == height - 1 && i + m + 1 < HN && MAP(i + m + 1,j + k - 1).value == Cell::EMPTY_ID) {
											buildingBorders.push_back(INDEX2D(i + m + 1, j + k - 1));
										}
										//left side
										if (MAP(i + m,j + k - 1).value == Cell::EMPTY_ID) {
											buildingBorders.push_back(
													INDEX2D(i + m, j + k - 1));
											continue;
										}
									}
								}
								//right side
								if (k == width - 1) {
									if (j + k + 1 < WN) {
										//right top corner
										if (m == 0 && i + m - 1 >= 0
												&& MAP(i + m - 1,j + k + 1).value
														== Cell::EMPTY_ID) {
											buildingBorders.push_back(
													INDEX2D(i + m - 1,
															j + k + 1));
										}
										//right bottom corner
										if (m == height - 1 && i + m + 1 < HN
												&& MAP(i + m + 1,j + k + 1).value
														== Cell::EMPTY_ID) {
											buildingBorders.push_back(
													INDEX2D(i + m + 1,
															j + k + 1));
										}
										//right side
										if (MAP(i + m,j + k + 1).value
												== Cell::EMPTY_ID) {
											buildingBorders.push_back(
													INDEX2D(i + m, j + k + 1));
											continue;
										}
									}
								}
								//top side
								if (m == 0) {
									if (i + m - 1 >= 0 && MAP(i + m - 1,j + k).value
											== Cell::EMPTY_ID) {
										buildingBorders.push_back(
												INDEX2D(i + m - 1, j + k));
										continue;
									}
								}
								//bottom side
								if (m == height - 1) {
									if (i + m + 1 < HN
											&& MAP(i + m + 1,j + k).value
													== Cell::EMPTY_ID) {
										buildingBorders.push_back(
												INDEX2D(i + m + 1, j + k));
										continue;
									}
								}
							}
						}
					}
				}
			}
		}
	}

	inline void bresenhems(float _x0, float _y0, float _x1, float _y1, int **lineIndices,
			int &count, Cell* mapWithCameras) {

		int x0 = _x0 / h;
		int y0 = _y0 / h;

		int x1 = _x1 / h;
		int y1 = _y1 / h;


		int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
		int dy = abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
		int err = (dx > dy ? dx : -dy) / 2, e2;
		int i = 0;
		for (;;) {
			if (x0 >= 0 && x0 < WN && y0 >= 0 && y0 < HN)
				lineIndices[i++] = &(MAPCAM(y0,x0).value);
			if (x0 == x1 && y0 == y1) {
				count = i;
				break;
			}
			e2 = err;
			if (e2 > -dx) {
				err -= dy;
				x0 += sx;
			}
			if (e2 < dy) {
				err += dx;
				y0 += sy;
			}
		}
	}

/*

	std::vector<float> constraints(std::vector<float> const& x) {
		std::vector<float> constraints;
		for (int cam = 0; cam < numCamera; cam++) {
			// 0 <= x <= size
			constraints.push_back(x[cam] - size);
			constraints.push_back(-x[cam]);

			// 0 <= y <= size
			constraints.push_back(x[cam+1] - size);
			constraints.push_back(-x[cam+1]);

			// -1 <= nx <= 1
			constraints.push_back(x[cam+2] - 1);
			constraints.push_back(-x[cam+2] - 1);

			// -1 <= ny <= 1
			constraints.push_back(x[cam+3] - 1);
			constraints.push_back(-x[cam+3] - 1);
		}
		return constraints;
	}
*/

public:

	float getCounterVision(std::vector<float> const& x) {
		Cell* mapWithCameras = new Cell[HN * WN];

		memcpy(mapWithCameras, map, sizeof(Cell) * HN * WN);

		float counterVision = 0;
		for (int cam = 0; cam < numCamera; cam++) {
#if TESTING
			std::srand(unsigned(std::time(0)));
			float x0 = (float) rand() / RAND_MAX * width;
			float y0 = (float) rand() / RAND_MAX * width;

			float nx = (float) rand() / RAND_MAX * 2.f - 1.f;
			float ny = (float) rand() / RAND_MAX * 2.f - 1.f;
#else
			float x0 = x[cam * 4];
			float y0 = x[cam * 4 + 1];

			int indX = x0 / h;
			int indY = y0 / h;
			if (MAP(indY,indX).value != Cell::BUILDING_ID)
				for (int x = -2; x <= 2; x++) {
					for (int y = -2; y <= 2; y++) {
						if (x == 0 && y == 0)
							continue;
						int neighbIndX = indX + x;
						int neighbIndY = indY + y;
						if (neighbIndX < 0 || neighbIndX >= WN || neighbIndY < 0
								|| neighbIndY >= HN)
							continue;
						if (MAP(neighbIndY, neighbIndX).value == Cell::BUILDING_ID) {
							counterVision += BONUS_BUILDING;
							goto GOTO_FROM_TWICE_FOR;
						}
					}
				}

GOTO_FROM_TWICE_FOR:

			float nx = x[cam * 4 + 2];
			float ny = x[cam * 4 + 3];
#endif
			float beta = atan2(ny, nx);
			float dth = atan2(h / 2.f, R) / R;


			for (float theta = beta - alpha / 2.f; theta <= beta + alpha / 2.f;
					theta += dth) {

				float x1 = R * cos(theta) + x0;
				float y1 = R * sin(theta) + y0;

				int** lineIndices = new int*[std::max(WN, HN)];

				int count = 0;

				bresenhems(x0, y0, x1, y1, lineIndices, count, mapWithCameras);

				for (int i = 0; i < count; i++) {
					int value = *lineIndices[i];
					if (value == Cell::BUILDING_ID) {
						break;
					}
					if (value > 0) {
						if (value == cam + 1) {
							continue;
						} else {
							*lineIndices[i] = Cell::OVERLAP_ID;
							counterVision -= PENALTY;
							continue;
						}
					} else if (value == Cell::OVERLAP_ID) {
						//counterVision -= PENALTY;
						continue;
					}

					*lineIndices[i] = cam + 1;
					counterVision++;
				}

				delete[] lineIndices;

			}
		}

		memcpy(this->mapWithCameras, mapWithCameras, sizeof(Cell) * WN * HN);

		delete[] mapWithCameras;

		return {counterVision};
	}

	Cell* getMapWithCameras() {
		return mapWithCameras;
	}

	float getWidth() {return width;}
	float getHeight() {return height;}
	int getWN() {return WN;}
	int getHN() {return HN;}
	float getNumCamera() {return numCamera;}

	enum DATA_TYPE {
		NUMBER_ALIVE,
		COMMON_ENERGY
	};

	void writeDataToFile(const char* filename, int indexIter, DATA_TYPE dataType) {
		std::fstream file(filename, std::ios_base::out | std::ios_base::app);
		file << std::setiosflags(std::ios::fixed) << std::setprecision(6);

		//todo write map

		file.close();
	}

	DeploymentCameraSystem(float alpha, float R, int numCamera, float width, float height) :
			map(0),
			alpha(alpha),
			R(R),
			width(width),
			height(height),
			h(width < height ? width / N : height / N),
			numCamera(numCamera),
			HN(height / h), WN(width / h) {

		map = new Cell[HN * WN];
		mapWithCameras = new Cell[HN * WN];

		//init automatas
		for (int i = 0; i < HN; i++) {
			for (int j = 0; j < WN; j++) {
				MAP(i,j).init(j * h, i * h);
				MAPCAM(i,j).init(j * h, i * h);
			}
		}

#ifdef _OPENMP
		//omp_set_num_threads(omp_get_max_threads());
#endif


		initMap();
		ga = nullptr;
#if TESTING
		fitnessFunction({});
#endif

	}

#if GALGO2
	void init(  std::vector<float> (*objective)(const std::vector<float>&) , void (*outputFunction)(const std::vector<float>&) = nullptr) {
		for (int cam = 0; cam < numCamera; cam++) {
			int indexBuildBord = rand() % buildingBorders.size();
			float x0 = map[buildingBorders[indexBuildBord]].x;
			float y0 = map[buildingBorders[indexBuildBord]].y;

			printf("x0 = %f, y0 = %f, width = %f, height = %f\n", x0, y0, width, height);
			params.push_back(galgo::Parameter<float, N_BITS>( { 0.f, width, x0 })); // x
			params.push_back(galgo::Parameter<float, N_BITS>( { 0.f, height, y0 })); // y
			params.push_back(galgo::Parameter<float, N_BITS>( { -1.f, 1.f, (float)rand() / RAND_MAX })); // nx
			params.push_back(galgo::Parameter<float, N_BITS>( { -1.f, 1.f, (float)rand() / RAND_MAX })); // ny
		}
		ga = new galgo::GeneticAlgorithm<float>(objective, numCamera * 20, 5000, true, outputFunction, this->params);
		galgo::GeneticAlgorithm<float>* galgo = (galgo::GeneticAlgorithm<float>*) ga;
		//galgo->CrossOver = P2XO;
		galgo->CrossOver = UXO;
		galgo->Selection = TNT;
		galgo->SP = 1;
		//galgo->genstep = 1;
		galgo->Mutation = UNM;
		galgo->mutrate = 0.1;
		galgo->elitpop = numCamera;
	}
#else
	void init(
			Genes (*mutate)(const Genes& X_base,
					const std::function<double(void)> &rand,
					double shrink_scale),
			Genes (*crossover)(const Genes& X1, const Genes& X2,
					const std::function<double(void)> &rand),
			double (*calculate_SO_total_fitness)(
					const typename GA_Type::thisChromosomeType &X),
			void (*SO_report_generation)(int generation_number,
					const EA::GenerationType<Genes, MiddleCost> &last_generation,
					const Genes& best_genes),
			bool (*eval_genes)(const Genes& p, MiddleCost &c),
			void (*init_genes)(Genes& p,
					const std::function<double(void)> &rand)) {
		ga = new GA_Type();
		GA_Type* openGA = (GA_Type*) ga;
		openGA->problem_mode = EA::GA_MODE::SOGA;
		openGA->multi_threading = true;
		openGA->idle_delay_us = 1;
		openGA->population = numCamera * 20;
		openGA->generation_max = 3000;
		openGA->calculate_SO_total_fitness = calculate_SO_total_fitness;
		openGA->init_genes = init_genes;
		openGA->eval_genes = eval_genes;
		openGA->mutate = mutate;
		openGA->crossover = crossover;
		openGA->SO_report_generation = SO_report_generation;
		openGA->best_stall_max = 50;
		openGA->elite_count = numCamera/2;
		openGA->crossover_fraction = 0.7;
		openGA->mutation_rate = 0.1;
	}
#endif

	void updateWithCamera(std::vector<float> const& x) {
		//update mapWithCamera
		getCounterVision(x);
	}

	void run() {
#if GALGO2
		((galgo::GeneticAlgorithm<float>*)ga)->run();
#else
		((GA_Type*)ga)->solve();
#endif
	}

	void pause() {
#if GALGO2
		((galgo::GeneticAlgorithm<float>*)ga)->pause();
#else

#endif
	}

	~DeploymentCameraSystem() {
		delete[] map;
		delete[] mapWithCameras;
		if (ga) {
#if GALGO2
			delete (galgo::GeneticAlgorithm<float>*)ga;
#else
			delete (galgo::GeneticAlgorithm<float>*)ga;
#endif
		}

	}
};
