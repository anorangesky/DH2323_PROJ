#include <iostream>
#include <glm/glm.hpp>
#include <SDL.h>
#include "SDLauxiliary.h"
#include "TestModel.h"

using namespace std;
using glm::vec3;
using glm::mat3; //3x3 matrix

struct Intersection {
	vec3 position;
	float distance;
	int triangleIndex;
};

// ----------------------------------------------------------------------------
// GLOBAL VARIABLES
const float SCREEN_WIDTH = 300;
const float SCREEN_HEIGHT = 300;

SDL_Surface* screen;
int t;
vector<Triangle> triangles; //used to store all triangles globally
float focalLength = ((SCREEN_HEIGHT + SCREEN_WIDTH) / 2) / 2;
vec3 cameraPos(0, 0, -2);
mat3 R;	//controls rotation of camera
float yaw = 0;	//stores angle that camera should rotate
const float change = 0.01; //constant for camera view change 

//vec3 lightPos(0, 0, 0); // light position
vec3 lightPos(0, -0.5, -0.7);
vec3 lightColor = 14.f * vec3(1, 1, 1); //light power for each color component

vec3 indirectLight = 0.5f*vec3(1, 1, 1); //this should not be included in Phong

# define M_PI  3.14159265358979323846  // pi

// ----------------------------------------------------------------------------
// FUNCTIONS
void Update();
void Draw();

bool ClosestIntersection(vec3 start, vec3 dir, const vector<Triangle>& triangles, Intersection& closestIntersection);
//vec3 DirectLight( const Intersection& i ); 

// PROJEKT
vec3 DiffuseComp(const Intersection& i);
//vec3 AmbientComp();

int main(int argc, char* argv[])
{
	screen = InitializeSDL(SCREEN_WIDTH, SCREEN_HEIGHT);
	// Start value for timer.
	t = SDL_GetTicks();
	//Fill the vector with triangles repr. a 3D model: 
	LoadTestModel(triangles);

	while (NoQuitMessageSDL()) {
		Update();
		Draw();
	}

	SDL_SaveBMP(screen, "screenshot.bmp");
	return 0;
}

void Update()
{
	// Compute frame time:
	int t2 = SDL_GetTicks();
	float dt = float(t2 - t);
	t = t2;
	cout << "	Agnes time: " << dt << " ms." << endl;

	Uint8* keystate = SDL_GetKeyState(0);
	vec3 right(R[0][0], R[0][1], R[0][2]);
	vec3 down(R[1][0], R[1][1], R[1][2]);
	vec3 forward(R[2][0], R[2][1], R[2][2]);

	if (keystate[SDLK_UP]) {
		// move camera forward aka. along the z-axis
		cameraPos.z += change;
	}
	if (keystate[SDLK_DOWN]) {
		// Move camera backward  aka. along the z-axis
		cameraPos.z -= change;
	}
	if (keystate[SDLK_LEFT]) {
		// Move camera to the left aka. along the x-axis
		yaw += change;
	}
	if (keystate[SDLK_RIGHT]) {
		// Move camera to the right aka along the x-axis
		yaw -= change;
	}
	if (keystate[SDLK_w]) {
		//move light forward aka along z-axis
		lightPos.z -= change * 5;
	}
	if (keystate[SDLK_s]) {
		//move light backward aka along z-axis
		lightPos.z += change * 5;
	}
	if (keystate[SDLK_d]) {
		//move light right aka along x-axis
		lightPos.x += change * 5;
	}
	if (keystate[SDLK_a]) {
		//move light left aka along x-axis
		lightPos.x -= change * 5;
	}
	if (keystate[SDLK_q]) {
		//move light up aka along y-axis
		lightPos.y -= change * 2;
	}
	if (keystate[SDLK_e]) {
		//move light down aka along y-axis
		lightPos.y += change * 2;
	}

	//rotation around y-axis
	R[0][0] = glm::cos(yaw);
	R[2][0] = glm::sin(yaw);
	R[0][2] = -glm::sin(yaw);
	R[2][2] = glm::cos(yaw);

}

void Draw() {
	bool hit;
	if (SDL_MUSTLOCK(screen))
		SDL_LockSurface(screen);

	Intersection closestInt;
	// 1. loop through all pixels to compute the corresponding ray direction 
	for (int y = 0; y < SCREEN_HEIGHT; ++y) {
		for (int x = 0; x < SCREEN_WIDTH; ++x) {

			//ray direction vector:
			vec3 d(x - (SCREEN_WIDTH / 2), y - (SCREEN_HEIGHT / 2), focalLength);
			// 2. Call closestIntersection to get the closest intersesction in that direction
			hit = ClosestIntersection(cameraPos *R, d, triangles, closestInt);

			if(hit){
			// 3. set the color of the pixel to the color of the intersected triangle
				
				//**** PHONG REFLECTION MODEL ****
				// ** adds the four illumination components together **
				// ** PutPixelSDL render the scene by multiplying intersected color with directLight **
				// ** Direct light -> Phong = specular + diffuse + ambient + emissive **
				// **** By: agnespet@kth.se 2020-05-19 ****
				
				vec3 color(triangles[closestInt.triangleIndex].color);
				// *** Uncoment when testing your implementation ***
				//vec3 emissive = EmmisiveComponent();
				//vec3 ambient = AmbientComponent();
				vec3 diffuse = DiffuseComponent();
				//vec3 specular = SpecularComponent;
				//vec3 phong = emissive + ambient + diffuse + specular;
				PutPixelSDL( screen, x, y, color * diffuse); //*phong);

			} else { 
			// 4. set it to black 
				vec3 black(0, 0, 0);
				PutPixelSDL(screen, x, y, black);
			}
		}
	}

	if (SDL_MUSTLOCK(screen))
		SDL_UnlockSurface(screen);

	SDL_UpdateRect(screen, 0, 0, 0, 0);
}

// **** DIFFUSE COMPONENT ILLUMINATION ****
// ** returns the intensity of the diffuse illumination as a 3D-vector **
// **** By: vendelav@kth.se 2020-05-19 ****
vec3 DiffuseComp(const Intersection& i) {

	// The equation to use for the diffuse component:
	// Idiffuse = kd * Ild * (l.normal . n)

	float kd = 1.0; // Diffuse surface constant
	vec3 Ild(1, 1, 1); // Diffuse light intensity
	vec3 l = lightPos - i.position; // Direction from surface to lightsource
	vec3 n = triangles[i.triangleIndex].normal; // Surface normal
	float dot = glm::dot(glm::normalize(l), n); // Angle between vector from surface to light source and surface normal
	// If the surface is perpendicular or turned away from the light source (angle <= 0) 
	// return 0, otherwise the angle so we can color the surface accordingly
	dot = dot <= 0 ? 0.0f : dot;

	vec3 diffuseComp = kd * Ild * dot;
	return diffuseComp;
}

//takes an intersection and returns the direct illumination vector
vec3 DirectLight(const Intersection& i) {
	Intersection a;
	vec3 D(0, 0, 0); //init value of direction

	// 1. calculate the unit vector describing the direction from the surface to the light source
	vec3 r = lightPos - i.position; // direction from surface (pos) to lightsource
	float rDist = glm::distance(lightPos, i.position);

	ClosestIntersection(i.position, r, triangles, a);
	//surface is in shadow if the distance to closest intersecting surface is closer than the light source
	if (a.triangleIndex != i.triangleIndex && a.distance <= rDist) {
		// In shadow -> return direction = [0,0,0]
		return D;
	}
	//	2. the index (i.index) is used in triangles[i.index].normal to get the normal of the surface
	vec3 n = triangles[i.triangleIndex].normal;
	//	3. compute the direction using "D = (P * max(r dot n, 0))/4*pi*r²" 
	// 
	float dot = glm::dot(glm::normalize(r), n);
	D = (lightColor * glm::max(dot, 0.f));		// GLM instead of SDL
	float fourPi = 4 * M_PI;					// M_PI ADDED AS GLOBAL VARIABLE
	D = D / (fourPi *rDist*rDist);
	// 4. return updated direciton value
	return D;
}

bool ClosestIntersection(vec3 start, vec3 dir, const vector<Triangle>& triangles, Intersection& closestIntersection) {
	bool hit = false;
	float m = std::numeric_limits<float>::max();
	closestIntersection.distance = m;

	for (int i = 0; i < triangles.size(); ++i) {
		//Intersection computations:
		vec3 v0 = triangles[i].v0;
		vec3 v1 = triangles[i].v1;
		vec3 v2 = triangles[i].v2;
		vec3 e1 = v1 - v0;
		vec3 e2 = v2 - v0;

		// ********------********** 
		//any point (r) in the plane and in the triangle: 
		//r1 = v0 + (u*e1) + (v*e2);
		//all points on the line of the ray:
		//r2 = start + (t*dir);
		// r1 = r2 -> A*[t;u;v] = s-v0 -> Ax = b 
		// ********------********** 

		vec3 b = start - v0;
		mat3 A(-dir, e1, e2);
		vec3 x = glm::inverse(A) * b; // [t;u;v]

		float t = x.x; // position on the line from start to r 
		float u = x.y;
		float v = x.z;

		// if the intersection occured within the triangle and 
		//	after the beginning of the ray
		if ((u >= 0) && (v >= 0) && ((u + v) <= 1) && (t > 0)) {
			//return true 
			hit = true;
			//and update closestIntersection with:
			vec3 r2 = start + (t*dir);
			//float distance = glm::distance(r2, start); //instead of "t"?
			if (t <= closestIntersection.distance) {
				// 1. 3D position of the closest intersection
				closestIntersection.position = r2;
				// 2. the distance from start to the intersected triangle
				closestIntersection.distance = t;
				// 3. Index of the ray that was intersected
				closestIntersection.triangleIndex = i;
			}
		}
	}
	return hit;
}

/*
Password: Döden i _________

Det här meddelandet är krypterat.Du kan dekryptera och läsa det via
https ://safemess.com/sv/?r=pJpcewX0G%2FJI
----------------------------------------------------------------------

5gMMg + RswV4Rb9cdPr93g / eFZ3buJk / K + MAcN2kkdYLQKX0Ic04YXGWvAo8Zz1EiXfzr
pWLaQlb / 4Zg0gQrpvc + 6ATRyikKx3pv31oUW4lPJxgtXI5WP6AJyq3KMhaGtG1thLSFJ
2oXvq / cz1Uo1cOo8jR40kMNYEgVUdLziLgr + u6APomy2vdqK40Qk05MuQH5tRYzKJ8Vl
+ EdgrSqCrCfC7TFuwhGk1nuZecZy / gpBo / 7pU6UgliTB5IM + sG2blUO1Ft6zTwWJlWQC
DfPF

----------------------------------------------------------------------
Det här meddelandet är krypterat.Du kan dekryptera och läsa det via
https ://safemess.com/sv/?r=pJpcewX0G%2FJI
*/


