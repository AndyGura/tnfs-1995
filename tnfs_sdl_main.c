/*
 * TNFS 1995 car physics code
 * Recreation and analysis of the physics engine from the classic racing game
 */
#include <SDL.h>
#include <SDL_opengl.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include "tnfs_math.h"
#include "tnfs_base.h"

static SDL_Event event;

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

GLfloat matrix[16] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };

void handleKeys() {
	if (event.type == SDL_KEYDOWN && event.key.repeat == 0) {
		switch (event.key.keysym.sym) {
		case SDLK_LEFT:
			car_data.steer_angle = -0x1B0000;
			break;
		case SDLK_RIGHT:
			car_data.steer_angle = 0x1B0000;
			break;
		case SDLK_UP:
			car_data.throttle = 0xFF;
			break;
		case SDLK_DOWN:
			car_data.brake = 0xFF;
			break;
		case SDLK_SPACE:
			car_data.handbrake = 1;
			break;
		case SDLK_a:
			tnfs_change_gear_up();
			break;
		case SDLK_z:
			tnfs_change_gear_down();
			break;
		case SDLK_r:
			tnfs_reset();
			break;
		case SDLK_c:
			tnfs_change_camera();
			break;
		case SDLK_d:
			tnfs_crash_car();
			break;
		case SDLK_F1:
			tnfs_abs();
			break;
		case SDLK_F2:
			tnfs_tcs();
			break;
		case SDLK_F3:
			tnfs_change_traction();
			break;
		case SDLK_F4:
			tnfs_change_transmission_type();
			break;
		case SDLK_F5:
			tnfs_cheat_mode();
			break;
		default:
			break;
		}
	}
	if (event.type == SDL_KEYUP && event.key.repeat == 0) {
		switch (event.key.keysym.sym) {
		case SDLK_UP:
			car_data.throttle = 0;
			break;
		case SDLK_DOWN:
			car_data.brake = 0;
			break;
		case SDLK_LEFT:
			car_data.steer_angle = 0;
			break;
		case SDLK_RIGHT:
			car_data.steer_angle = 0;
			break;
		case SDLK_SPACE:
			car_data.handbrake = 0;
			break;
		default:
			break;
		}
	}
}

/*
 * TNFS coord system
 * position X+ right Y+ up Z+ north
 * angle X+ pitch down Y+ yaw clockwise Z+ roll left
 */
void renderVehicle() {
	// TNFS uses LHS, convert to OpenGL's RHS
	glMatrixMode(GL_MODELVIEW);
	matrix[0] = (float) car_data.matrix.ax / 0x10000;
	matrix[1] = (float) car_data.matrix.ay / 0x10000;
	matrix[2] = (float) -car_data.matrix.az / 0x10000;
	matrix[3] = 0;
	matrix[4] = (float) car_data.matrix.bx / 0x10000;
	matrix[5] = (float) car_data.matrix.by / 0x10000;
	matrix[6] = (float) -car_data.matrix.bz / 0x10000;
	matrix[7] = 0;
	matrix[8] = (float) car_data.matrix.cx / 0x10000;
	matrix[9] = (float) car_data.matrix.cy / 0x10000;
	matrix[10] = (float) -car_data.matrix.cz / 0x10000;
	matrix[11] = 0;
	matrix[12] = ((float) (car_data.position.x - camera_position.x)) / 0x10000;
	matrix[13] = ((float) (car_data.position.y - camera_position.y)) / 0x10000;
	matrix[14] = ((float) (-car_data.position.z + camera_position.z)) / 0x10000;
	matrix[15] = 1;
	glLoadMatrixf(matrix);

	glColor3f(0.0f, 0.0f, 1.0f);

	glBegin(GL_QUADS);

	// front bumper
	glVertex3f(-1, 0, 2.2f);
	glVertex3f(1, 0, 2.2f);
	glVertex3f(1, 0.5f, 2.2f);
	glVertex3f(-1, 0.5f, 2.2f);

	// hood
	glVertex3f(-1, 0.5f, 2.2f);
	glVertex3f(1, 0.5f, 2.2f);
	glVertex3f(1, 1.3f, 0);
	glVertex3f(-1, 1.3f, 0);

	// roof/trunk
	glVertex3f(-1, 1.3f, 0);
	glVertex3f(1, 1.3f, 0);
	glVertex3f(1, 1.0f, -2.2f);
	glVertex3f(-1, 1.0f, -2.2f);

	// rear bumper
	glVertex3f(-1, 0, -2.2f);
	glVertex3f(1, 0, -2.2f);
	glVertex3f(1, 1.0f, -2.2f);
	glVertex3f(-1, 1.0f, -2.2f);

	// bottom
	glVertex3f(-1, 0, -2.2f);
	glVertex3f(1, 0, -2.2f);
	glVertex3f(1, 0, 2.2f);
	glVertex3f(-1, 0, 2.2f);

	glEnd();
}

void renderPanel(int x, int y, int z, int width, int a) {
	glMatrixMode(GL_MODELVIEW);
	if (a) { //front
		matrix[0] = 1; matrix[1] = 0; matrix[2] = 0; matrix[3] = 0;
		matrix[4] = 0; matrix[5] = 1; matrix[6] = 0; matrix[7] = 0;
		matrix[8] = 0; matrix[9] = 0; matrix[10] = 1; matrix[11] = 0;
	} else { //side
		matrix[0] = 0; matrix[1] = 0; matrix[2] = 1; matrix[3] = 0;
		matrix[4] = 0; matrix[5] = 1; matrix[6] = 0; matrix[7] = 0;
		matrix[8] = 1; matrix[9] = 0; matrix[10] = 0; matrix[11] = 0;
	}
	matrix[12] = ((float) -camera_position.x) / 0x10000 + x;
	matrix[13] = ((float) -camera_position.y) / 0x10000 + y;
	matrix[14] = ((float) camera_position.z) / 0x10000 + z;
	matrix[15] = 1;
	glLoadMatrixf(matrix);

	glColor3f(0.0f, 0.0f, 0.0);

	glBegin(GL_QUADS);
	glVertex3f(-width, 0, 0);
	glVertex3f( width, 0, 0);
	glVertex3f( width, 1, 0);
	glVertex3f(-width, 1, 0);
	glEnd();
}

void renderTach() {
	float c,s,r;
	r = ((float) car_data.rpm_engine / (float) car_data.rpm_redline) * 3.14 - 1.56;
	c = -cosf(r);
	s = sinf(r);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glOrtho(0.0, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0, -1.0, 10.0);
	glMatrixMode(GL_MODELVIEW);
	matrix[0] = c; matrix[1] = -s; matrix[2] = 0; matrix[3] = 0;
	matrix[4] = s; matrix[5] = c; matrix[6] = 0; matrix[7] = 0;
	matrix[8] = 0; matrix[9] = 0; matrix[10] = 0; matrix[11] = 0;
	matrix[12] = 100; matrix[13] = 520; matrix[14] = 0; matrix[15] = 1;
	glLoadMatrixf(matrix);

	glColor3f(1.0f, 0.0f, 0.0);
	glBegin(GL_TRIANGLES);
	glVertex3f(-2, 0, 0);
	glVertex3f(+2, 0, 0);
	glVertex3f(0, 80, 0);
	glEnd();
}

void renderGl() {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(90.0, 1, 0.1, 1000);

	renderPanel(-20, 0, 0, 100, 0);
	renderPanel(+20, 0, 0, 100, 0);
	renderPanel( 0, 0, 100, 20, 1);
	renderPanel( 0, 0, -100, 20, 1);
	renderVehicle();
	renderTach();
}

int main(int argc, char **argv) {
	char quit = 0;
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		printf("SDL could not be initialized! SDL_Error: %s\n", SDL_GetError());
		return 0;
	}

#if defined linux && SDL_VERSION_ATLEAST(2, 0, 8)
	if (!SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0")) {
		printf("SDL can not disable compositor bypass!\n");
		return 0;
	}
#endif

	SDL_Window *window = SDL_CreateWindow("SDL Window", //
			SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, //
			800, 600, SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);
	if (!window) {
		printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
		return 0;
	}

	SDL_GLContext glContext = SDL_GL_CreateContext(window);
	if (!glContext) {
		printf("GL Context could not be created! SDL_Error: %s\n", SDL_GetError());
	}

	glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	glClearColor(1.f, 1.f, 1.f, 0.f);
	glClear(GL_COLOR_BUFFER_BIT);

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glColor3f(0.0f, 0.0f, 0.0f);

	tnfs_reset();

	while (!quit) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) {
				quit = 1;
			}
			handleKeys();
		}

		tnfs_update();

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		renderGl();

		SDL_GL_SwapWindow(window);

		SDL_Delay(30);
	}

	SDL_GL_DeleteContext(glContext);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}