#include "EngineWindowFrame.h"
#include "EnginePanic.h"
#include "convert_endian.h"

#define FRAME_ASSETS_PATH "MAGE/desktop_assets"

int SCREEN_MULTIPLIER = 1;
int SCREEN_WIDTH = 0;
int SCREEN_HEIGHT = 0;

EngineWindowFrame::EngineWindowFrameComponents::EngineWindowFrameComponents() noexcept
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		ENGINE_PANIC("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
	}

	frameSurface = IMG_Load(FRAME_ASSETS_PATH "/window_frame.png");

	if (!frameSurface)
	{
		ENGINE_PANIC("Failed to load Window Frame\nIMG_Load: %s\n", IMG_GetError());
	}

	frameButtonSurface = IMG_Load(FRAME_ASSETS_PATH "/window_frame-button.png");

	if (!frameButtonSurface)
	{
		ENGINE_PANIC("Failed to load Window Frame Button\nIMG_Load: %s\n", IMG_GetError());
	}

	frameLEDSurface = IMG_Load(FRAME_ASSETS_PATH "/window_frame-led.png");

	if (!frameLEDSurface)
	{
		ENGINE_PANIC("Failed to load Window Frame LED\nIMG_Load: %s\n", IMG_GetError());
	}

	//Create window
	SCREEN_WIDTH = frameSurface->w * SCREEN_MULTIPLIER;
	SCREEN_HEIGHT = frameSurface->h * SCREEN_MULTIPLIER;
	SDL_CreateWindowAndRenderer(
		SCREEN_WIDTH,
		SCREEN_HEIGHT,
		SDL_WINDOW_SHOWN,
		&window,
		&renderer
	);
	SDL_SetWindowTitle(
		window,
		"DC801 MAGE GAME"
	);

	if (window == nullptr)
	{
		ENGINE_PANIC("Failed to create SDL Window\nSDL_Error: %s\n", SDL_GetError());
	}

	SDL_RenderSetLogicalSize(renderer, frameSurface->w, frameSurface->h);

	frameTexture = SDL_CreateTextureFromSurface(renderer, frameSurface);
	frameButtonTexture = SDL_CreateTextureFromSurface(renderer, frameButtonSurface);
	frameLEDTexture = SDL_CreateTextureFromSurface(renderer, frameLEDSurface);

	SDL_SetTextureBlendMode(frameLEDTexture, SDL_BLENDMODE_BLEND);

	gameViewportTexture = SDL_CreateTexture(
		renderer,
		SDL_PIXELFORMAT_RGB565,
		SDL_TEXTUREACCESS_STREAMING,
		WIDTH,
		HEIGHT
	);
}

EngineWindowFrame::EngineWindowFrameComponents::~EngineWindowFrameComponents()
{
	SDL_DestroyTexture(gameViewportTexture);
	gameViewportTexture = nullptr;
	SDL_DestroyTexture(frameTexture);
	frameTexture = nullptr;
	SDL_DestroyTexture(frameButtonTexture);
	frameButtonTexture = nullptr;
	SDL_FreeSurface(frameButtonSurface);
	frameButtonSurface = nullptr;
	SDL_DestroyTexture(frameLEDTexture);
	frameLEDTexture = nullptr;
	SDL_FreeSurface(frameLEDSurface);
	frameLEDSurface = nullptr;
	SDL_FreeSurface(frameSurface);
	frameSurface = nullptr;
	SDL_DestroyRenderer(renderer);
	renderer = nullptr;
	SDL_DestroyWindow(window);
	window = nullptr;
}

void EngineWindowFrame::drawButtonStates()
{
	SDL_Point buttonPoint{};
	bool buttonState{false};
	for (int i = 0; i < KEYBOARD_NUM_KEYS; ++i)
	{
		buttonPoint = buttonDestPoints[i];
		buttonState = inputHandler->GetButtonState((KeyPress)i);
		buttonTargetRect.x = buttonPoint.x - buttonHalf.x;
		buttonTargetRect.y = buttonPoint.y - buttonHalf.y;
		SDL_RenderCopy(
			components.renderer,
			components.frameButtonTexture,
			buttonState ? &buttonOnSrcRect : &buttonOffSrcRect,
			&buttonTargetRect
		);
	}
}

void EngineWindowFrame::drawLEDStates()
{
	SDL_Point LEDPoint{};
	uint8_t LEDState{0};
	for (int i = 0; i < LED_COUNT; ++i)
	{
		LEDPoint = LEDDestPoints[i];
		LEDState = led_states[i];
		LEDTargetRect.x = LEDPoint.x - LEDHalf.x;
		LEDTargetRect.y = LEDPoint.y - LEDHalf.y;
		SDL_SetTextureAlphaMod(components.frameLEDTexture, 255);
		SDL_RenderCopy(
			components.renderer,
			components.frameLEDTexture,
			&LEDOffSrcRect,
			&LEDTargetRect
		);
		if (LEDState > 0) {
			SDL_SetTextureAlphaMod(components.frameLEDTexture, LEDState);
			SDL_RenderCopy(
				components.renderer,
				components.frameLEDTexture,
				&LEDOnSrcRect,
				&LEDTargetRect
			);
		}
	}
}

void EngineWindowFrame::GameBlt(uint16_t* frame)
{
	int pitch{0};

	if (frame == nullptr) {
		return;
	}

	void* targetPixelBuffer;
	if (0 == SDL_LockTexture(components.gameViewportTexture, nullptr, &targetPixelBuffer, &pitch))
	{
		memcpy(targetPixelBuffer, frame, FRAMEBUFFER_SIZE);
		// Sorry for this monster;
		// The game.dat stores the image buffer data in BigEndian
		// SDL reads FrameBuffers in Platform Native Endian,
		// so we need to convert if Desktop is LittleEndian
#ifndef IS_SCREEN_BIG_ENDIAN
#ifdef IS_LITTLE_ENDIAN
		convert_endian_u2_buffer((uint16_t*)targetPixelBuffer, FRAMEBUFFER_SIZE);
#endif
#endif

		SDL_UnlockTexture(components.gameViewportTexture);
	}

	SDL_RenderCopy(
		components.renderer,
		components.frameTexture,
		&components.frameSurface->clip_rect,
		&components.frameSurface->clip_rect
	);

	SDL_RenderCopy(
		components.renderer,
		components.gameViewportTexture,
		&gameViewportSrcRect,
		&gameViewportDstRect
	);

	drawButtonStates();
	drawLEDStates();

	SDL_RenderPresent(components.renderer);
}



void EngineWindowFrame::Resize(int change) {
	SCREEN_MULTIPLIER += change;
	if (SCREEN_MULTIPLIER < 1) {
		SCREEN_MULTIPLIER = 1;
	}
	if (SCREEN_MULTIPLIER > 2) {
		SCREEN_MULTIPLIER = 2;
	}
	components = EngineWindowFrameComponents{};
}