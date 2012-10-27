#include <windows.h>
#include "inc/GL/glew.h"
#include "inc/GL/glfw.h"
#include <stdlib.h>
#include "elementals.h"
#include "textures.h"
#include "map.h"
#include "opengl.h"

struct state
{
	vectorf position;
	vectorf velocity;

	state(vectorf pos = vectorf(), vectorf vel = vectorf()) : position(pos), velocity(vel) { }
};

namespace collision
{
	enum collisionType
	{
		TOP = 0x01,
		BOTTOM = 0x02,
		LEFT = 0x04,
		RIGHT = 0x08
	};

	class info
	{
	private:
		ui8 data;

	public:
		info() : data(0) {}

		bool top() { return (data & TOP) != 0; }
		bool bottom() { return (data & BOTTOM) != 0; }
		bool left() { return (data & LEFT) != 0; }
		bool right() { return (data & RIGHT) != 0; }

		void set(ui8 type) { data |= type; }
		void unset(ui8 type) { data &= ~type; }
		void reset() { data = 0; }

		operator bool() { return data != 0; }
	};
}

sizei *g_screenSize = 0;

void drawMap(vectorf offset);
rectf boundingBox(i32 sprite_id, const pointf &pos, bool flipped);
bool map_collision(state &prevState, state &curState, recti box, collision::info &collides);
void GLFWCALL windowResize(int width, int height);

int CALLBACK WinMain(__in HINSTANCE hInstance, __in HINSTANCE hPrevInstance, __in LPSTR lpCmdLine, __in int nCmdShow)
{
	sizei screenSize(800, 600);
	g_screenSize = &screenSize;

	if (!GFX::init("gametest", screenSize, false))
	{
		exit(EXIT_FAILURE);
	}

	glfwSetWindowSizeCallback(windowResize);

	MAP::load("res\\map.png");
	TX::load(TX::PerroFrames, "res\\perro_frames2.png", pointi(26, 79), sizei(52, 80));
	TX::load(TX::Ruby, "res\\ruby.png", pointi(26, 79), sizei(52, 80));
	TX::load(TX::Ground, "res\\ground2.png", pointi(), sizei(), true);

	// constants

	const f32 kVel = 250.f;
	const f32 kJump = 550.0f;

	enum Direction { kNone, kLeft, kRight, kBottom, kTop };
	enum Character { kPerro, kRuby };

	// perro

	state perroCur(vectorf(260.0f, 200.0f), vectorf(0.0f, 0.0f));
	state perroPrev(perroCur);
	state perroInt;
	vectorf perroAcc(0.0f, 9.8f * 150.0f);
	collision::info perroCollides;
	ui32 perroFrame = 0;
	bool perroFlip = false;
	f32 perroAnimTime = -1.0f;

	// ruby

	state rubyCur(vectorf(560.0f, 200.0f), vectorf(0.0f, 0.0f));
	state rubyPrev(rubyCur);
	state rubyInt;
	vectorf rubyAcc(0, 9.8f * 150.0f);
	collision::info rubyCollides;
	ui32 rubyFrame = 0;
	bool rubyFlip = false;
	f32 rubyAnimTime = -1.0f;
	f32 rubyAiTime = 0.0f;
	bool rubyWillJump = false;
	bool rubyCollided = false;
	Direction rubyWalkDirection = kNone;
	Direction rubyWillJumpDirection = kNone;

	// camera

	bool keySpacePressed = false;
	Character cameraBindedTo = kPerro;
	state cameraCur(vectorf(static_cast<f32>(screenSize.width / 2), static_cast<f32>(screenSize.height / 2)), vectorf(0.0f, 0.0f));
	state cameraPrev(cameraCur);
	state cameraInt;
	vectorf cameraAcc;

	// loop

	f64 t = 0.0;
	f32 dt = 0.01f;
	f64 newTime = 0.0;
	f64 frameTime = 0.0;
	f64 currentTime = glfwGetTime();
	f64 accumulator = 0.0;

	bool running = true;
	ui32 frameCount = 0;
	ui32 stepCount = 0;
	f64 startTime = glfwGetTime();
	f64 totalTime = 0.0;

	while (running)
	{
		newTime = glfwGetTime();
		frameTime = newTime - currentTime;

		if (frameTime > 0.25)
			frameTime = 0.25;

		currentTime = newTime;
		accumulator += frameTime;

		while (accumulator >= dt)
		{
			// general input

			if (glfwGetKey(GLFW_KEY_ESC) || !glfwGetWindowParam(GLFW_OPENED))
			{
				running = false;
			}

			if (glfwGetKey(GLFW_KEY_SPACE))
			{
				if (!keySpacePressed)
				{
					keySpacePressed = true;
					cameraBindedTo = (cameraBindedTo == kPerro ? kRuby : kPerro);
				}
			}
			else
			{
				keySpacePressed = false;
			}

			// perro processing

			perroPrev = perroCur;

			perroCur.velocity.x = 0.0f;

			if (perroCollides.bottom() && glfwGetKey(GLFW_KEY_UP))
			{
				perroCur.velocity.y = -kJump;
			}

			if (glfwGetKey(GLFW_KEY_LEFT))
			{
				perroFlip = false;
				perroCur.velocity.x = -kVel;
			}

			if (glfwGetKey(GLFW_KEY_RIGHT))
			{
				perroFlip = true;
				perroCur.velocity.x = kVel;
			}

			perroCur.velocity.x += perroAcc.x * dt;
			perroCur.velocity.y += perroAcc.y * dt;
			perroCur.position.x += perroCur.velocity.x * dt;
			perroCur.position.y += perroCur.velocity.y * dt;

			// ruby processing

			rubyPrev = rubyCur;

			if (rubyCollides)
			{
				rubyCollided = true;
			}

			if (rubyCollides.right())
			{
				rubyWillJump = true;
				rubyWillJumpDirection = rand() % 10 < 7 ? kRight : kLeft;
			}
			else if (rubyCollides.left())
			{
				rubyWillJump = true;
				rubyWillJumpDirection = rand() % 10 < 7 ? kLeft : kRight;
			}

			if (rubyAiTime >= 1.0f)
			{
				rubyAiTime = 0.0f;
				
				if (rubyCollides.bottom() && (rand() % 100 < 40))
				{
					rubyCur.velocity.y = -kJump;
				}

				if (rubyWalkDirection == kNone || rubyCollided || (rand() % 100 < 10))
				{
					int r = rand() % 3;

					if (r == 0)
					{
						rubyWalkDirection = kRight;
					}
					else if (r == 1)
					{
						rubyWalkDirection = kLeft;
					}
					else
					{
						rubyWalkDirection = kNone;
					}
				}
			}

			rubyAiTime += dt;

			if (rubyCollides.bottom() && rubyWillJump)
			{
				rubyWillJump = false;
				rubyCur.velocity.y = -kJump;
				rubyWalkDirection = rubyWillJumpDirection;
				rubyAiTime = 0.0f;
			}

			rubyCur.velocity.x = 0.0f;

			if (rubyWalkDirection == kLeft)
			{
				rubyFlip = false;
				rubyCur.velocity.x = -kVel;
			}
			else if (rubyWalkDirection == kRight)
			{
				rubyFlip = true;
				rubyCur.velocity.x = kVel;
			}

			rubyCur.velocity.x += rubyAcc.x * dt;
			rubyCur.velocity.y += rubyAcc.y * dt;
			rubyCur.position.x += rubyCur.velocity.x * dt;
			rubyCur.position.y += rubyCur.velocity.y * dt;

			// collision checks

			recti rc;
			state prev;

			// perro collision check

			perroCollides.reset();
			rc = boundingBox(TX::PerroFrames, pointf(0.0f, 0.0f), perroFlip);
			prev = perroPrev;

			if (perroCur.velocity.x != 0.0f || perroCur.velocity.y != 0.0f)
			{
				map_collision(prev, perroCur, rc, perroCollides);
			}

			if (perroCollides && (perroCur.velocity.x != 0.0f || perroCur.velocity.y != 0.0f))
			{
				map_collision(prev, perroCur, rc, perroCollides);
			}

			if (perroCur.velocity.x == 0.0f && perroCollides.bottom())
			{
				perroFrame = 0;
				perroAnimTime = -1.0f;
			}
			else if (!perroCollides.bottom())
			{
				perroFrame = 1;
				perroAnimTime = -1.0f;
			}
			else if (perroCur.velocity.x != 0.0f)
			{
				if (perroAnimTime == -1.0f)
				{
					perroFrame = 2;
					perroAnimTime = 0.0f;
				}

				perroAnimTime += dt;

				if (perroAnimTime >= 0.1f)
				{
					perroFrame++;
					if (perroFrame > 2) perroFrame = 1;
					perroAnimTime = 0.0f;
				}
			}

			// ruby collision check

			rubyCollides.reset();
			rc = boundingBox(TX::Ruby, pointf(0.0f, 0.0f), rubyFlip);
			prev = rubyPrev;

			if (rubyCur.velocity.x != 0.0f || rubyCur.velocity.y != 0.0f)
			{
				map_collision(prev, rubyCur, rc, rubyCollides);
			}

			if (rubyCollides && (rubyCur.velocity.x != 0.0f || rubyCur.velocity.y != 0.0f))
			{
				map_collision(prev, rubyCur, rc, rubyCollides);
			}

			if (rubyCur.velocity.x == 0.0f && rubyCollides.bottom())
			{
				rubyFrame = 0;
				rubyAnimTime = -1.0f;
			}
			else if (!rubyCollides.bottom())
			{
				rubyFrame = 1;
				rubyAnimTime = -1.0f;
			}
			else if (rubyCur.velocity.x != 0.0f)
			{
				if (rubyAnimTime == -1.0f)
				{
					rubyFrame = 2;
					rubyAnimTime = 0.0f;
				}

				rubyAnimTime += dt;

				if (rubyAnimTime >= 0.1f)
				{
					rubyFrame++;
					if (rubyFrame > 2) rubyFrame = 1;
					rubyAnimTime = 0.0f;
				}
			}

			// camera

			cameraPrev = cameraCur;

			vectorf cameraObjective = (cameraBindedTo == kPerro ? perroCur.position : rubyCur.position);

			cameraObjective.x = min(max(screenSize.width / 2, cameraObjective.x), static_cast<f32>(MAP::getWidth() * MAP::getTileSize()) - screenSize.width / 2);
			cameraObjective.y = min(max(screenSize.height / 2, cameraObjective.y), static_cast<f32>(MAP::getHeight() * MAP::getTileSize()) - screenSize.height / 2);

			vectorf cameraDistance = cameraObjective - cameraCur.position;

			cameraCur.velocity.x = cameraDistance.x * 2.0f;
			cameraCur.velocity.y = cameraDistance.y * 2.0f;

			if (abs(cameraCur.velocity.x) < 15.0f)
			{
				cameraCur.velocity.x = 0.0f;
			}

			if (abs(cameraCur.velocity.y) < 15.0f)
			{
				cameraCur.velocity.y = 0.0f;
			}

			cameraCur.velocity.x += cameraAcc.x * dt;
			cameraCur.velocity.y += cameraAcc.y * dt;
			cameraCur.position.x += cameraCur.velocity.x * dt;
			cameraCur.position.y += cameraCur.velocity.y * dt;

			// step finished

			t += dt;
			accumulator -= dt;
			stepCount++;
		}

		// frame interpolation

		const f32 alpha = (f32)(accumulator / dt);

		perroInt.position.x = round(perroCur.position.x * alpha + perroPrev.position.x * (1.0f - alpha));
		perroInt.position.y = round(perroCur.position.y * alpha + perroPrev.position.y * (1.0f - alpha));

		rubyInt.position.x = round(rubyCur.position.x * alpha + rubyPrev.position.x * (1.0f - alpha));
		rubyInt.position.y = round(rubyCur.position.y * alpha + rubyPrev.position.y * (1.0f - alpha));

		cameraInt.position.x = round(cameraCur.position.x * alpha + cameraPrev.position.x * (1.0f - alpha));
		cameraInt.position.y = round(cameraCur.position.y * alpha + cameraPrev.position.y * (1.0f - alpha));

		// render

		glClear(GL_COLOR_BUFFER_BIT);

		GFX::drawGradient(0, 0, static_cast<f32>(screenSize.width), static_cast<f32>(screenSize.height), GFX::RGBAf(0, 0, 1, 1), GFX::RGBAf(1, 1, 1, 1));

		vectorf mapOffset(cameraInt.position.x - screenSize.width / 2, cameraInt.position.y - screenSize.height / 2);

		drawMap(mapOffset);

		GFX::drawTiledSprite(TX::Ruby, rubyFrame, rubyInt.position.x - mapOffset.x, rubyInt.position.y - mapOffset.y, 0.0f, 1.0f, rubyFlip);
		GFX::drawTiledSprite(TX::PerroFrames, perroFrame, perroInt.position.x - mapOffset.x, perroInt.position.y - mapOffset.y, 0.0f, 1.0f, perroFlip);

		glfwSwapBuffers();

		frameCount++;
	}

	totalTime = glfwGetTime() - startTime;
	f64 FPS = frameCount / totalTime;

	MAP::unload();
	GFX::terminate();

	exit(EXIT_SUCCESS);
}

void GLFWCALL windowResize(int width, int height)
{
	g_screenSize->width = width;
	g_screenSize->height = height;

	GFX::setResolution(*g_screenSize);
}

bool map_collision(state &prevState, state &curState, recti box, collision::info &collides)
{
	pointi p1, p2, offs(box.x, box.y);

	if (curState.position.x > prevState.position.x)
	{
		p1.x = (i32)floorf(prevState.position.x);
		p2.x = (i32)ceilf(curState.position.x);
	}
	else
	{
		p1.x = (i32)ceilf(prevState.position.x);
		p2.x = (i32)floorf(curState.position.x);
	}

	if (curState.position.y > prevState.position.y)
	{
		p1.y = (i32)floorf(prevState.position.y);
		p2.y = (i32)ceilf(curState.position.y);
	}
	else
	{
		p1.y = (i32)ceilf(prevState.position.y);
		p2.y = (i32)floorf(curState.position.y);
	}

	if (p1.x == p2.x && p1.y == p2.y)
		return false;

	recti rc(p1.x + offs.x, p1.y + offs.y, box.width, box.height);
	rc.add(recti(p2.x + offs.x, p2.y + offs.y, box.width, box.height));

	if (MAP::collides(rc))
	{
		bool y_faster = true;
		i32 vectori::*fasteri = &vectori::y;
		i32 vectori::*sloweri = &vectori::x;
		f32 vectorf::*fasterf = &vectorf::y;
		f32 vectorf::*slowerf = &vectorf::x;

		if (abs(curState.velocity.x) > abs(curState.velocity.y))
		{
			y_faster = false;
			fasteri = &vectori::x;
			sloweri = &vectori::y;
			fasterf = &vectorf::x;
			slowerf = &vectorf::y;
		}

		i32 dm = p2.*fasteri > p1.*fasteri ? 1 : -1;
		f32 ds = curState.velocity.*slowerf / abs(curState.velocity.*fasterf);
		f32 offset_slower = (f32)(p1.*sloweri);
		f32 prev_offset_slower = offset_slower;
		pointi p(p1), prev(p);

		// skip first step because it shouldn't collide with anything
		offset_slower += ds;
		(p.*sloweri) = (i32)(ds > 0.0f ? ceilf(offset_slower) : floorf(offset_slower));
		(p.*fasteri) += dm;

		i32 steps = abs(p2.*fasteri - p1.*fasteri);

		for (i32 i = 0; i < steps; i++)
		{
			box.x = p.x + offs.x;
			box.y = p.y + offs.y;

			if (MAP::collides(box))
			{
				p.*fasteri = prev.*fasteri;

				box.x = p.x + offs.x;
				box.y = p.y + offs.y;

				if (MAP::collides(box))
				{
					if (y_faster)
						collides.set(curState.velocity.*slowerf > 0.0f ? collision::RIGHT : collision::LEFT);
					else
						collides.set(curState.velocity.*slowerf > 0.0f ? collision::BOTTOM : collision::TOP);

					curState.velocity.*slowerf = 0.0f;
					prevState.position.*slowerf = curState.position.*slowerf = (f32)(prev.*sloweri);
				}
				else
				{
					if (y_faster)
						collides.set(dm > 0 ? collision::BOTTOM : collision::TOP);
					else
						collides.set(dm > 0 ? collision::RIGHT : collision::LEFT);

					curState.velocity.*fasterf = 0.0f;
					prevState.position.*fasterf = curState.position.*fasterf = (f32)(prev.*fasteri);
				}

				return true;
			}

			prev = p;
			prev_offset_slower = offset_slower;

			if (ds != 0.0f)
			{
				offset_slower += ds;
				(p.*sloweri) = (i32)(ds > 0.0f ? ceilf(offset_slower) : floorf(offset_slower));
			}

			(p.*fasteri) += dm;
		}
	}

	return false;
}

rectf boundingBox(i32 sprite_id, const pointf &pos, bool flipped)
{
	TX::sprite &sprite = TX::sprites[sprite_id];

	return rectf(pos.x - (f32)sprite.origin.x + 10.0f, pos.y - (f32)sprite.origin.y + 2.0f, 32.0f, 77.0f);
}

void drawMap(vectorf offset)
{
	i32 tileSize = MAP::getTileSize();

	for (i32 y = 0; y < MAP::getHeight(); y++)
	{
		for (i32 x = 0; x < MAP::getWidth(); x++)
		{
			if (MAP::getTile(x, y))
			{
				GFX::drawSprite(TX::Ground, (f32)x * tileSize - offset.x, (f32)y * tileSize - offset.y);
			}
		}
	}
}
