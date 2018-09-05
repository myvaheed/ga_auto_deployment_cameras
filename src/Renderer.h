/*
 * Renderer.h
 *
 *  Created on: Dec 7, 2017
 *      Author: myvaheed
 */

#ifndef RENDERER_H_
#define RENDERER_H_

#define LINUX

#include "Cell.h"
#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"
#include "SDL2/SDL_mixer.h"


class Renderer {
	int numMaxCamera = 15;

	int WN, HN;

	//основные объекты при отрисовке
	SDL_Window* win;
	SDL_Renderer* ren;

	SDL_Texture* buildingCellTxt;
	SDL_Texture* emptyCellTxt;
	SDL_Texture* overlapCellTxt;
	SDL_Texture** cameraCellTxt;

	//автоматы рисуем в cells
	SDL_Rect** cells;

	//опции экрана
	SDL_DisplayMode displayMode;

	int widthCell = 0;

public:
	Renderer(int WN, int HN) :
			WN(WN), HN(HN), cameraCellTxt(0), buildingCellTxt(0), emptyCellTxt(
					0), overlapCellTxt(0), ren(0), win(0) {
		init();
	}

	~Renderer() {
		finalize();
	}
//обработка нажатия кнопки Run/Pause
	void handleEvent(int x, int y, bool& bPause);
	void finalize();
	void init();
	void render(Cell* auts);

	int getWidthAutomata() {
		return widthCell;
	}
};


void Renderer::init() {
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		throw std::runtime_error(
				std::string("SDL_Init Error: ") + SDL_GetError());
	}

	SDL_GetDesktopDisplayMode(0,&displayMode);

	win = SDL_CreateWindow("Deployment Cameras", 0, 0, displayMode.w,
			displayMode.h, SDL_WINDOW_SHOWN);
	if (win == nullptr) {
		throw std::runtime_error(
				std::string("SDL_CreateWindow Error: ") + SDL_GetError());
	}

	ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
	if (ren == nullptr) {
		throw std::runtime_error(
				std::string("SDL_CreateRenderer Error: ") + SDL_GetError());
	}

	buildingCellTxt = IMG_LoadTexture(ren, "blue.png");
	emptyCellTxt = IMG_LoadTexture(ren, "white.png");
	overlapCellTxt = IMG_LoadTexture(ren, "red.png");

	cameraCellTxt = new SDL_Texture*[numMaxCamera];
	std::string filename;
	for (int i = 1; i <= numMaxCamera; i++) {
		filename = std::to_string(i) + ".png";
		cameraCellTxt[i - 1] = IMG_LoadTexture(ren, filename.c_str());
	}

	widthCell = std::min(displayMode.w / WN, displayMode.h / HN);

	cells = new SDL_Rect*[HN];
	for (int i = 0; i < HN; i++) {
		cells[i] = new SDL_Rect[WN];
	}

	for (int i = 0; i < HN; i++) {
		for (int j = 0; j < WN; j++) {
			cells[i][j].x = j * widthCell;
			cells[i][j].y = i * widthCell;
			cells[i][j].w = widthCell;
			cells[i][j].h = widthCell;
		}
	}

	//цвет фона
	SDL_SetRenderDrawColor(ren, 200, 200, 200, 255);
}

void Renderer::render(Cell* auts) {
	SDL_RenderClear(ren);

	int value;
	for (int i = 0; i < HN; i++) {
		for (int j = 0; j < WN; j++) {
			value = auts[i * WN + j].value;
			if (value == Cell::EMPTY_ID) {
				SDL_RenderCopy(ren, emptyCellTxt, NULL, &cells[i][j]);
				continue;
			}
			if (value > 0) {
				SDL_RenderCopy(ren, cameraCellTxt[value - 1], NULL, &cells[i][j]);
				continue;
			}
			if (value == Cell::BUILDING_ID) {
				SDL_RenderCopy(ren, buildingCellTxt, NULL, &cells[i][j]);
				continue;
			}

			SDL_RenderCopy(ren, overlapCellTxt, NULL, &cells[i][j]);
		}
	}
	SDL_RenderPresent(ren);
	//задержка в миллисекундах
	SDL_Delay(100);
}

void Renderer::handleEvent(int x, int y, bool& bPause) {
}

void Renderer::finalize() {
	for (int i = 0; i < HN; i++) {
		delete[] cells[i];
	}
	delete[] cells;

	SDL_DestroyTexture(buildingCellTxt);
	SDL_DestroyTexture(emptyCellTxt);
	SDL_DestroyTexture(overlapCellTxt);

	for (int i = 0; i < numMaxCamera; i++) {
		SDL_DestroyTexture(cameraCellTxt[i]);
	}
	delete[] cameraCellTxt;

	SDL_DestroyRenderer(ren);
	SDL_DestroyWindow(win);
	SDL_Quit();
}

#endif /* RENDERER_H_ */
