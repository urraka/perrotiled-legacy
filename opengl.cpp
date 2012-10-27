#include "inc/GL/glew.h"
#include "inc/GL/glfw.h"
#include <stdlib.h>
#include "elementals.h"
#include "textures.h"
#include "opengl.h"

namespace GFX
{
	GLuint textures[TX::MAX] = {0};

	bool init(const char *title, sizei resolution, bool fullscreen)
	{
		if (!glfwInit())
		{
			return false;
		}

		if (!glfwOpenWindow(resolution.width, resolution.height, 0, 0, 0, 0, 0, 0, fullscreen ? GLFW_FULLSCREEN : GLFW_WINDOW))
		{
			return false;
		}

		glfwSetWindowTitle(title);

		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT);

		GLFWvidmode vidmode;
		glfwGetDesktopMode(&vidmode);
		glfwSetWindowPos((vidmode.Width - resolution.width) / 2, (vidmode.Height - resolution.height) / 2);

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, resolution.width, resolution.height, 0, 0, 1);
		glDisable(GL_DEPTH_TEST);
		glMatrixMode(GL_MODELVIEW);

		glEnable(GL_TEXTURE_2D);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		return true;
	}

	void setResolution(sizei resolution)
	{
		glViewport(0, 0, resolution.width, resolution.height);

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, resolution.width, resolution.height, 0, 0, 1);
		glDisable(GL_DEPTH_TEST);
		glMatrixMode(GL_MODELVIEW);
	}

	void terminate()
	{
		for (int i = 0; i < TX::MAX; i++)
		{
			unloadTexture(i);
		}

		glfwTerminate();
	}

	void loadTexture(int id, int width, int height, void *bits, bool repeat)
	{
		GLuint &tx = textures[id];

		if (tx) unloadTexture(id);

		glGenTextures(1, &tx);
		glBindTexture(GL_TEXTURE_2D, tx);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, repeat ? GL_REPEAT : GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, repeat ? GL_REPEAT : GL_CLAMP);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, bits);
	}

	void unloadTexture(int id)
	{
		if (textures[id])
		{
			glDeleteTextures(1, &(textures[id]));
			textures[id] = 0;
		}
	}

	void drawSprite(int id, float x, float y, float angle, float size)
	{
		TX::sprite &s = TX::sprites[id];

		float x0 = (float)-s.origin.x,
			y0 = (float)-s.origin.y,
			w = (float)s.size.width * size,
			h = (float)s.size.height * size;

		glBindTexture(GL_TEXTURE_2D, textures[id]);

		glLoadIdentity();
		glTranslatef(x, y, 0);
		glRotatef(angle, 0, 0, -1);

		glBegin(GL_QUADS);

		glTexCoord2f(0, 0);
		glVertex2f(x0, y0);

		glTexCoord2f(0, 1);
		glVertex2f(x0, y0 + h);

		glTexCoord2f(1, 1);
		glVertex2f(x0 + w, y0 + h);

		glTexCoord2f(1, 0);
		glVertex2f(x0 + w, y0);

		glEnd();
	}

	void drawTiledSprite(int id, int tileIndex, float x, float y, float angle, float size, bool flipX)
	{
		TX::sprite &s = TX::sprites[id];

		f32 x0 = (f32)-s.origin.x,
			y0 = (f32)-s.origin.y,
			w = (f32)s.tileSize.width * size,
			h = (f32)s.tileSize.height * size;

		int n = s.size.width / s.tileSize.width;
		f32 tw = (f32)s.tileSize.width / s.size.width;
		f32 th = (f32)s.tileSize.height / s.size.height;

		pointf tx0((tileIndex % n) * tw, (int)(tileIndex / n) * th);
		pointf tx1(tx0.x + tw, tx0.y + th);

		if (flipX)
		{
			f32 tmp = tx0.x;
			tx0.x = tx1.x;
			tx1.x = tmp;
		}

		glBindTexture(GL_TEXTURE_2D, textures[id]);

		glLoadIdentity();
		glTranslatef(x, y, 0);
		glRotatef(angle, 0, 0, -1);

		glBegin(GL_QUADS);

		glTexCoord2f(tx0.x, tx0.y);
		glVertex2f(x0, y0);

		glTexCoord2f(tx0.x, tx1.y);
		glVertex2f(x0, y0 + h);

		glTexCoord2f(tx1.x, tx1.y);
		glVertex2f(x0 + w, y0 + h);

		glTexCoord2f(tx1.x, tx0.y);
		glVertex2f(x0 + w, y0);

		glEnd();
	}

	void drawGradient(float x, float y, float width, float height, RGBAf startColor, RGBAf endColor)
	{
		glDisable(GL_TEXTURE_2D);
		glLoadIdentity();

		glBegin(GL_QUADS);

		glColor4f(startColor.R, startColor.G, startColor.B, startColor.A);
		glVertex2f(x, y);
		glVertex2f(x + width, y);

		glColor4f(endColor.R, endColor.G, endColor.B, endColor.A);
		glVertex2f(x + width, y + height);
		glVertex2f(x, y + height);

		glEnd();

		glEnable(GL_TEXTURE_2D);
	}

	void renderObjects()
	{
	}
}
