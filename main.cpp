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
int chooseTile(int x, int y, bool &flipX, bool &flipY);
rectf boundingBox(i32 sprite_id, const pointf &pos);
bool map_collision(state &prevState, state &curState, recti box, collision::info &collides);
void GLFWCALL windowResize(int width, int height);

int CALLBACK WinMain(__in HINSTANCE hInstance, __in HINSTANCE hPrevInstance, __in LPSTR lpCmdLine, __in int nCmdShow)
{
	sizei screenSize(1280, 720);
	g_screenSize = &screenSize;

	if (!GFX::init("perrotiled", screenSize, false))
	{
		exit(EXIT_FAILURE);
	}

	glfwSetWindowSizeCallback(windowResize);

	MAP::load("res\\map.png");
	TX::load(TX::PerroFrames, "res\\perro_frames2.png", pointi(26, 79), sizei(52, 80));
	TX::load(TX::Ruby, "res\\ruby.png", pointi(26, 79), sizei(52, 80));
	TX::load(TX::Ground, "res\\map_ground.png", pointi(0, 0), sizei(32, 32), false);

	// constants

	const f32 kVel = 250.f;
	const f32 kJump = 550.0f;
	const f32 g = 9.8f * 150.0f;
	const f32 kMaxKickedTime = 1.5f;

	enum Direction { kNone, kLeft, kRight, kBottom, kTop };
	enum Character { kPerro, kRuby };

	// perro

	state perroCur(vectorf(260.0f, 200.0f), vectorf(0.0f, 0.0f));
	state perroPrev(perroCur);
	state perroInt;
	vectorf perroAcc(0.0f, g);
	collision::info perroCollides;
	f32 perroAngle = 0.0f;
	ui32 perroFrame = 0;
	bool perroFlip = false;
	f32 perroAnimTime = -1.0f;
	f64 perroKickTime = -1.0f;
	bool perroIsKicking = false;
	f32 perroKickedTime = kMaxKickedTime + 1.0f;
	bool perroKicked = false;
	vectorf perroKickedVel;

	// ruby

	state rubyCur(vectorf(560.0f, 200.0f), vectorf(0.0f, 0.0f));
	state rubyPrev(rubyCur);
	state rubyInt;
	vectorf rubyAcc(0, g);
	collision::info rubyCollides;
	f32 rubyAngle = 0.0f;
	ui32 rubyFrame = 0;
	bool rubyFlip = false;
	f32 rubyAnimTime = -1.0f;
	f32 rubyAiTime = 0.0f;
	bool rubyWillJump = false;
	bool rubyCollided = false;
	Direction rubyWalkDirection = kNone;
	Direction rubyWillJumpDirection = kNone;
	f64 rubyKickTime = -1.0f;
	bool rubyIsKicking = false;
	f32 rubyKickedTime = kMaxKickedTime + 1.0f;
	bool rubyKicked = false;
	vectorf rubyKickedVel;

	// camera

	Character cameraBindedTo = kPerro;
	state cameraCur(vectorf(static_cast<f32>(screenSize.width / 2), static_cast<f32>(screenSize.height / 2)), vectorf(0.0f, 0.0f));
	state cameraPrev(cameraCur);
	state cameraInt;
	vectorf cameraAcc;

	// input

	bool keyCtrlPressed = false;
	bool keySpacePressed = false;
	bool keyF12Pressed = false;

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

	while (true)
	{
		glClear(GL_COLOR_BUFFER_BIT);

		if (glfwGetKey(GLFW_KEY_ENTER))
		{
			break;
		}

		glfwSwapBuffers();
	}

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
					cameraBindedTo = (cameraBindedTo == kPerro ? kRuby : kPerro);
				}

				keySpacePressed = true;
			}
			else
			{
				keySpacePressed = false;
			}

			// perro processing

			perroPrev = perroCur;

			if (perroKickedTime > kMaxKickedTime)
			{
				perroAngle = 0.0f;
				perroCur.velocity.x = 0.0f;

				if (perroCollides.bottom() && glfwGetKey(GLFW_KEY_UP))
				{
					perroCur.velocity.y = -kJump;
				}

				if (!perroIsKicking || !perroCollides.bottom())
				{
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
				}

				if (glfwGetKey(GLFW_KEY_LCTRL) || glfwGetKey(GLFW_KEY_RCTRL))
				{
					if (!keyCtrlPressed)
					{
						perroIsKicking = true;
						perroKickTime = t;

						rectf rcPerro = boundingBox(TX::PerroFrames, perroCur.position);
						rectf rcRuby = boundingBox(TX::Ruby, rubyCur.position);

						if (rcPerro.intersects(rcRuby))
						{
							rubyKicked = true;
							rubyKickedTime = 0.0f;
							rubyKickedVel = vectorf(-300.0f, -500.0f);

							if (perroFlip)
							{
								rubyKickedVel.x = -rubyKickedVel.x;
							}
						}
					}

					keyCtrlPressed = true;
				}
				else
				{
					keyCtrlPressed = false;
				}
			}
			else
			{
				perroAngle = 10.0f * (perroKickedVel.x > 0.0f ? -1.0f : 1.0f);

				if (perroCollides.bottom())
				{
					perroKickedVel.x = perroKickedVel.x * 0.99f;
				}
			}

			if (perroCollides.right() || perroCollides.left())
			{
				perroKickedTime = kMaxKickedTime + 1.0f;
			}

			if (perroIsKicking && perroCollides.bottom())
			{
				perroCur.velocity.x = 0;
				perroCur.velocity.y = 0;
			}

			if (perroIsKicking && (t - perroKickTime) > 0.1)
			{
				perroIsKicking = false;
			}

			if (perroKickedTime < kMaxKickedTime)
			{
				perroIsKicking = false;

				if (perroKicked)
				{
					perroCur.velocity.y = perroKickedVel.y;
					perroKicked = false;
				}

				perroCur.velocity.x = perroKickedVel.x;				
				perroKickedTime += dt;
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

			if (rubyCollides.left() || rubyCollides.right())
			{
				rubyKickedTime = kMaxKickedTime + 1.0f;
			}

			if (rubyKickedTime > kMaxKickedTime)
			{
				rubyAngle = 0.0f;

				rectf rcPerro = boundingBox(TX::PerroFrames, perroCur.position);
				rectf rcRuby = boundingBox(TX::Ruby, rubyCur.position);

				if (rcPerro.intersects(rcRuby) && (rand() % 100) < 1) // TODO: add timer to this
				{
					rubyIsKicking = true;
					rubyKickTime = t;

					perroKicked = true;
					perroKickedTime = 0.0f;
					perroKickedVel = vectorf(-300.0f, -500.0f);

					if (rubyFlip)
					{
						perroKickedVel.x = -perroKickedVel.x;
					}
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

				if (rubyAiTime >= 1.0f && !rubyIsKicking)
				{
					rubyAiTime = 0.0f;
				
					if (rubyCollides.bottom() && (rand() % 100 < 20))
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
			}
			else
			{
				rubyAngle = 10.0f * (rubyKickedVel.x > 0.0f ? -1.0f : 1.0f);

				if (rubyCollides.bottom())
				{
					rubyKickedVel.x = rubyKickedVel.x * 0.99f;
				}

				rubyIsKicking = false;

				if (rubyKicked)
				{
					rubyCur.velocity.y = rubyKickedVel.y;
					rubyKicked = false;
				}

				rubyCur.velocity.x = rubyKickedVel.x;				
				rubyKickedTime += dt;
			}

			if (rubyIsKicking && rubyCollides.bottom())
			{
				rubyCur.velocity.x = 0;
				rubyCur.velocity.y = 0;
			}

			if (rubyIsKicking && (t - rubyKickTime) > 0.1 || rubyKickedTime < kMaxKickedTime)
			{
				rubyIsKicking = false;
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
			rc = boundingBox(TX::PerroFrames, pointf(0.0f, 0.0f));
			prev = perroPrev;

			if (perroCur.velocity.x != 0.0f || perroCur.velocity.y != 0.0f)
			{
				map_collision(prev, perroCur, rc, perroCollides);
			}

			if (perroCollides && (perroCur.velocity.x != 0.0f || perroCur.velocity.y != 0.0f))
			{
				map_collision(prev, perroCur, rc, perroCollides);
			}

			// perro frame selection

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

			if (perroIsKicking)
			{
				perroFrame = 3;
			}

			// ruby collision check

			rubyCollides.reset();
			rc = boundingBox(TX::Ruby, pointf(0.0f, 0.0f));
			prev = rubyPrev;

			if (rubyCur.velocity.x != 0.0f || rubyCur.velocity.y != 0.0f)
			{
				map_collision(prev, rubyCur, rc, rubyCollides);
			}

			if (rubyCollides && (rubyCur.velocity.x != 0.0f || rubyCur.velocity.y != 0.0f))
			{
				map_collision(prev, rubyCur, rc, rubyCollides);
			}

			// ruby frame selection

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

			if (rubyIsKicking)
			{
				rubyFrame = 3;
			}

			// camera

			cameraPrev = cameraCur;

			vectorf cameraObjective = (cameraBindedTo == kPerro ? perroCur.position : rubyCur.position);

			//cameraObjective.x += 100.0f * (cameraBindedTo == kPerro ? (perroFlip ? 1.0 : -1.0f) : (rubyFlip ? 1.0f : -1.0f));

			const vectorf cameraMin(static_cast<f32>(screenSize.width / 2), static_cast<f32>(screenSize.height / 2));
			const vectorf cameraMax(static_cast<f32>(MAP::getWidth() * MAP::getTileSize()) - screenSize.width / 2, static_cast<f32>(MAP::getHeight() * MAP::getTileSize()) - screenSize.height / 2);

			// keep camera within map bounds
			cameraObjective.x = min(max(cameraMin.x, cameraObjective.x), cameraMax.x);
			cameraObjective.y = min(max(cameraMin.y, cameraObjective.y), cameraMax.y);

			// this fix is for when you resize the window
			cameraCur.position.x = min(max(cameraMin.x, cameraCur.position.x), cameraMax.x);
			cameraCur.position.y = min(max(cameraMin.y, cameraCur.position.y), cameraMax.y);

			vectorf cameraDistance = cameraObjective - cameraCur.position;

			cameraCur.velocity.x = cameraDistance.x * 2.5f;
			cameraCur.velocity.y = cameraDistance.y * 2.5f;

			if (abs(cameraCur.velocity.x) < 20.0f)
			{
				cameraCur.velocity.x = 0.0f;
			}

			if (abs(cameraCur.velocity.y) < 20.0f)
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

		const f32 alpha = static_cast<f32>(accumulator / static_cast<f64>(dt));

		perroInt.position.x = floorf(perroCur.position.x * alpha + perroPrev.position.x * (1.0f - alpha));
		perroInt.position.y = floorf(perroCur.position.y * alpha + perroPrev.position.y * (1.0f - alpha));

		rubyInt.position.x = floorf(rubyCur.position.x * alpha + rubyPrev.position.x * (1.0f - alpha));
		rubyInt.position.y = floorf(rubyCur.position.y * alpha + rubyPrev.position.y * (1.0f - alpha));

		cameraInt.position.x = floorf(cameraCur.position.x * alpha + cameraPrev.position.x * (1.0f - alpha));
		cameraInt.position.y = floorf(cameraCur.position.y * alpha + cameraPrev.position.y * (1.0f - alpha));

		// render

		glClear(GL_COLOR_BUFFER_BIT);

		GFX::drawGradient(0, 0, static_cast<f32>(screenSize.width), static_cast<f32>(screenSize.height), GFX::RGBAf(0, 0, 1, 1), GFX::RGBAf(1, 1, 1, 1));

		vectorf mapOffset(cameraInt.position.x - screenSize.width / 2, cameraInt.position.y - screenSize.height / 2);

		drawMap(mapOffset);

		GFX::drawTiledSprite(TX::Ruby, rubyFrame, rubyInt.position.x - mapOffset.x, rubyInt.position.y - mapOffset.y, rubyAngle, 1.0f, rubyFlip);
		GFX::drawTiledSprite(TX::PerroFrames, perroFrame, perroInt.position.x - mapOffset.x, perroInt.position.y - mapOffset.y, perroAngle, 1.0f, perroFlip);

		glfwSwapBuffers();

		if (glfwGetKey(GLFW_KEY_F12))
		{
			if (!keyF12Pressed)
			{
				GFX::screenshot();
			}

			keyF12Pressed = true;
		}
		else
		{
			keyF12Pressed = false;
		}

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

rectf boundingBox(i32 sprite_id, const pointf &pos)
{
	TX::sprite &sprite = TX::sprites[sprite_id];

	return rectf(pos.x - (f32)sprite.origin.x + 10.0f, pos.y - (f32)sprite.origin.y + 2.0f, 32.0f, 77.0f);
}

void drawMap(vectorf offset)
{
	f32 tileSize = static_cast<f32>(MAP::getTileSize());

	i32 w = MAP::getWidth();
	i32 h = MAP::getHeight();

	i32 x0 = max(0, static_cast<i32>(floor(offset.x / tileSize)));
	i32 y0 = max(0, static_cast<i32>(floor(offset.y / tileSize)));
	i32 x1 = min(w, static_cast<i32>(floor((offset.x + g_screenSize->width) / tileSize)));
	i32 y1 = min(h, static_cast<i32>(floor((offset.y + g_screenSize->height) / tileSize)));

	for (i32 y = y0; y <= y1; y++)
	{
		for (i32 x = x0; x <= x1; x++)
		{
			if (MAP::getTile(x, y))
			{
				vectorf pos(static_cast<f32>(x) * tileSize - offset.x, static_cast<f32>(y) * tileSize - offset.y);

				bool flipX, flipY;

				i32 idx = chooseTile(x, y, flipX, flipY);

				GFX::drawTiledSprite(TX::Ground, idx, pos.x, pos.y, 0, 1.0f, flipX, flipY);
			}
		}
	}
}

int chooseTile(int x, int y, bool &flipX, bool &flipY)
{
	enum { kTop = 0, kRight, kBottom, kLeft, kTopLeft, kTopRight, kBottomRight, kBottomLeft };

	const i32 w = MAP::getWidth();
	const i32 h = MAP::getHeight();

	const bool isOutsideTile = true;

	bool tile[8] = {};
	bool shadow[8] = {};
	int nInnerCorners = 0;
	int nSideShadows = 0;

	tile[kTop]    = (y > 0 ? MAP::getTile(x, y - 1) != 0 : isOutsideTile);
	tile[kRight]  = (x < w ? MAP::getTile(x + 1, y) != 0 : isOutsideTile);
	tile[kBottom] = (y < h ? MAP::getTile(x, y + 1) != 0 : isOutsideTile);
	tile[kLeft]   = (x > 0 ? MAP::getTile(x - 1, y) != 0 : isOutsideTile);

	tile[kTopLeft]     = (x > 0 && y > 0 ? MAP::getTile(x - 1, y - 1) != 0 : isOutsideTile);
	tile[kTopRight]    = (x < w && y > 0 ? MAP::getTile(x + 1, y - 1) != 0 : isOutsideTile);
	tile[kBottomRight] = (x < w && y < h ? MAP::getTile(x + 1, y + 1) != 0 : isOutsideTile);
	tile[kBottomLeft]  = (x > 0 && y < h ? MAP::getTile(x - 1, y + 1) != 0 : isOutsideTile);

	shadow[kTop]    = !tile[kTop];
	shadow[kRight]  = !tile[kRight];
	shadow[kBottom] = !tile[kBottom];
	shadow[kLeft]   = !tile[kLeft];

	shadow[kTopLeft]     = tile[kTop]    && tile[kLeft]  && !tile[kTopLeft];
	shadow[kTopRight]    = tile[kTop]    && tile[kRight] && !tile[kTopRight];
	shadow[kBottomLeft]  = tile[kBottom] && tile[kLeft]  && !tile[kBottomLeft];
	shadow[kBottomRight] = tile[kBottom] && tile[kRight] && !tile[kBottomRight];

	for (int i = kTop; i <= kLeft; i++)
	{
		if (shadow[i]) nSideShadows++;
	}

	for (int i = kTopLeft; i <= kBottomLeft; i++)
	{
		if (shadow[i]) nInnerCorners++;
	}


	flipX = false;
	flipY = false;

	if (nInnerCorners == 0)
	{
		if (nSideShadows == 0)
		{
			return TX::kShadowNone;
		}
		else if (nSideShadows == 1)
		{
			flipX = shadow[kRight];
			flipY = shadow[kBottom];

			if (shadow[kLeft] || shadow[kRight])
			{
				return TX::kShadowL;
			}

			return TX::kShadowT;
		}
		else if (nSideShadows == 2)
		{
			bool isAdjacent = (shadow[kTop] && (shadow[kLeft] || shadow[kRight]) || shadow[kBottom] && (shadow[kLeft] || shadow[kRight]));

			if (isAdjacent)
			{
				if (shadow[kRight]) flipX = true;
				if (shadow[kBottom]) flipY = true;

				return TX::kShadowTL;
			}
			else
			{
				if (shadow[kLeft]) return TX::kShadowLR;

				return TX::kShadowTB;
			}
		}
		else if (nSideShadows == 3)
		{
			if (!shadow[kLeft]) flipX = true;
			if (!shadow[kTop]) flipY = true;
			
			if (!shadow[kTop] || !shadow[kBottom]) return TX::kShadowTLR;

			return TX::kShadowTLB;
		}
		else
		{
			return TX::kShadowTLBR;
		}
	}
	else if (nInnerCorners == 1)
	{
		flipX = shadow[kTopRight] || shadow[kBottomRight];
		flipY = shadow[kBottomLeft] || shadow[kBottomRight];

		if (nSideShadows == 0) return TX::kShadowTl;
		if (nSideShadows == 2) return TX::kShadowTlRB;
		if (nSideShadows == 1 && (shadow[kLeft] || shadow[kRight])) return TX::kShadowTlR;

		return TX::kShadowTlB;
	}
	else if (nInnerCorners == 2)
	{
		if (shadow[kTopLeft] && shadow[kBottomRight] || shadow[kBottomLeft] && shadow[kTopRight])
		{
			flipX = shadow[kTopRight];

			return TX::kShadowTlBr;
		}

		flipX = flipY = shadow[kBottomRight];
			
		if (shadow[kTopLeft] && shadow[kTopRight] || shadow[kBottomLeft] && shadow[kBottomRight]) return TX::kShadowTlTr + (nSideShadows == 1 ? 2 : 0);

		return TX::kShadowTlBl + (nSideShadows == 1 ? 2 : 0);
	}
	else if (nInnerCorners == 3)
	{
		flipX = !shadow[kBottomLeft] || !shadow[kTopLeft];
		flipY = !shadow[kTopLeft] || !shadow[kTopRight];

		return TX::kShadowTlBlTr;
	}
	else
	{
		return TX::kShadowTlBlTrBr;
	}
}
