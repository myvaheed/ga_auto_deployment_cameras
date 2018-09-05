/*
 * main.cpp
 *
 *  Created on: May 20, 2018
 *      Author: myvaheed
 */

#include "DeploymentCameraSystem.h"
#include "Renderer.h"



const int N = 100;

DeploymentCameraSystem<N>* refCaSystem;

Renderer* refRenderer;

#if GALGO2

void customOutput(const std::vector<float>& x)
{
	refCaSystem->updateWithCamera(x);
	refRenderer->render(refCaSystem->getMapWithCameras());
}

std::vector<float> objective(const std::vector<float>& x)
{
	return {refCaSystem->getCounterVision(x)};
}

#else

void init_genes(Genes& p, const std::function<double(void)> &rand) {
	int numCamera = refCaSystem->getNumCamera();
	for (int i = 0; i < numCamera; i++) {
		p.x.push_back(rand() * refCaSystem->getWidth());
		p.x.push_back(rand() * refCaSystem->getHeight());
		p.x.push_back(rand() * 2.f - 1.f);
		p.x.push_back(rand() * 2.f - 1.f);
	}
}

bool eval_genes(const Genes& p, MiddleCost &c) {
	c.countVision = -refCaSystem->getCounterVision(p.x);
	return true;
}

Genes mutate(const Genes& X_base, const std::function<double(void)> &rand,
		double shrink_scale) {
	(void) shrink_scale;
	Genes X_new;
	bool in_range_x, in_range_y;

	int numCamera = refCaSystem->getNumCamera();

	int counter = 0;
	do {
		X_new = X_base;
		for (int i = counter; i < numCamera; i++) {
			X_new.x[4*i] += refCaSystem->getWidth() / 2 * (rand() - rand());
			X_new.x[4*i+1] += refCaSystem->getHeight() / 2 * (rand() - rand());
			X_new.x[4*i+2] += (rand() - rand());
			X_new.x[4*i+3] += (rand() - rand());
			in_range_x = (X_new.x[4*i] >= 0.0 && X_new.x[4*i] < refCaSystem->getWidth());
			in_range_y = (X_new.x[4*i+1] >= 0.0 && X_new.x[4*i+1] < refCaSystem->getHeight());
			if (!in_range_x || !in_range_y) break;
			counter++;
		}
		if (counter == numCamera) break;
	} while (true);
	return X_new;
}

Genes crossover(const Genes& X1, const Genes& X2,
		const std::function<double(void)> &rand) {
	Genes X_new;
	double r;
	int numCamera = refCaSystem->getNumCamera();
	for (int i = 0; i < numCamera; i++) {
		r = rand();
		X_new.x.push_back(r * X1.x[4*i] + (1.0 - r) * X2.x[4*i]);
		r = rand();
		X_new.x.push_back(r * X1.x[4*i+1] + (1.0 - r) * X2.x[4*i+1]);
		r = rand();
		X_new.x.push_back(r * X1.x[4*i+2] + (1.0 - r) * X2.x[4*i+2]);
		r = rand();
		X_new.x.push_back(r * X1.x[4*i+3] + (1.0 - r) * X2.x[4*i+3]);
	}

	return X_new;
}

double calculate_SO_total_fitness(
		const typename GA_Type::thisChromosomeType &X) {
	// finalize the cost
	return X.middle_costs.countVision;
}

void SO_report_generation(
	int generation_number,
	const EA::GenerationType<Genes,MiddleCost> &last_generation,
	const Genes& best_genes)
{
	std::cout
		<<"Generation ["<<generation_number<<"], "
		<<"Best="<<last_generation.best_total_cost<<", "
		<<"Average="<<last_generation.average_cost<<", "
		<<"Best genes=("<<best_genes.to_string()<<")"<<", "
		<<"Exe_time="<<last_generation.exe_time
		<< std::endl;
	refCaSystem->updateWithCamera(best_genes.x);
	refRenderer->render(refCaSystem->getMapWithCameras());
}

#endif


int main()
{
	DeploymentCameraSystem<N> caSystem(2 * M_PI/3, 50.f, 4, 100, 80);
	refCaSystem = &caSystem;

	Renderer renderer(caSystem.getWN(), caSystem.getHN());
	refRenderer = &renderer;

#if GALGO2
	caSystem.init(objective, customOutput);
#else
	caSystem.init(mutate, crossover, calculate_SO_total_fitness, SO_report_generation, eval_genes, init_genes);
#endif

	caSystem.run();

	return 0;
}


