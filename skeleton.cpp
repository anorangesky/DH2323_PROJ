#include <iostream>
#include <glm/glm.hpp> //GLM stands for 'OpenGL Mathematics' 
#include <SDL.h>
#include "SDLauxiliary.h"
#include "TestModel.h"

using namespace std;
using glm::vec3;
using glm::mat3; // 3x3 matrix

struct Intersection {
	vec3 position;
	float distance;
	int triangleIndex;
	// vec3 surfaceMaterial; //TODO: How to get the surface material from the rendered image?
};

// ----------------------------------------------------------------------------
// GLOBAL VARIABLES
const float SCREEN_WIDTH = 300;
const float SCREEN_HEIGHT = 300;
SDL_Surface* screen;
int t; //timer
vector<Triangle> triangles; //used to store all triangles globally
float focalLength = ((SCREEN_HEIGHT + SCREEN_WIDTH) / 2) / 2;
vec3 cameraPos(0, 0, -2);
vec3 lightPos(0, 0, -1); // light position
vec3 lightColor = 1.f * vec3(1, 1, 1); //light power (intensity) for each color component
Intersection a; // used when computing shadow in DiffuseComponent and SpecularComponent

// ----------------------------------------------------------------------------
// FUNCTIONS
void Update();
void Draw();
bool ClosestIntersection(vec3 start, vec3 dir, const vector<Triangle>& triangles, Intersection& closestIntersection);
vec3 EmmisiveComponent();
vec3 AmbientComponent();
vec3 DiffuseComponent(const Intersection& i);
vec3 SpecularComponent(const Intersection& i);

//** from lab 2 **
int main(int argc, char* argv[]){
	screen = InitializeSDL(SCREEN_WIDTH, SCREEN_HEIGHT);
	// Start value for timer.
	t = SDL_GetTicks();
	// Fill the vector with triangles repr. a 3D model: 
	LoadTestModel(triangles);

	while (NoQuitMessageSDL()) {
		Update();
		Draw();
	}

	SDL_SaveBMP(screen, "screenshot.bmp");
	return 0;
}

//** from lab 2 **
//TODO: implement code to move the light source and camera around
void Update(){
	// Compute frame time:
	int t2 = SDL_GetTicks();
	float dt = float(t2 - t);
	t = t2;
	cout << "	Render time: " << dt << " ms." << endl;
}

//** from lab 2 **
void Draw() {
	bool hit;
	if (SDL_MUSTLOCK(screen))
		SDL_LockSurface(screen);

	Intersection closestInt;
	// loop through all pixels to compute the corresponding ray direction 
	for (int y = 0; y < SCREEN_HEIGHT; ++y) {
		for (int x = 0; x < SCREEN_WIDTH; ++x) {

			//ray direction vector:
			vec3 d(x - (SCREEN_WIDTH / 2), y - (SCREEN_HEIGHT / 2), focalLength);
			//call closestIntersection to get the closest intersesction in that direction
			hit = ClosestIntersection(cameraPos, d, triangles, closestInt);
			if (hit) {

				//**** PHONG REFLECTION MODEL ****
				// ** adds the four illumination components together **
				// ** PutPixelSDL render the scene by multiplying intersected color with directLight **
				// ** Direct light -> Phong = specular + diffuse + ambient + emissive **
				// **** By: agnespet@kth.se 2020-05-20 ****
				vec3 color(triangles[closestInt.triangleIndex].color);
				vec3 emissive = EmmisiveComponent();
				vec3 ambient = AmbientComponent();
				vec3 diffuse = DiffuseComponent(closestInt);
				vec3 specular = SpecularComponent(closestInt);
				vec3 phong = emissive + ambient + diffuse + specular;
				PutPixelSDL(screen, x, y, color * phong);

			}
			else { 
				vec3 black(0, 0, 0);
				PutPixelSDL(screen, x, y, black);
			}
		}
	}

	if (SDL_MUSTLOCK(screen))
		SDL_UnlockSurface(screen);

	SDL_UpdateRect(screen, 0, 0, 0, 0);
}

// **** EMISSIVE ILLUMINATION COMPONENT ****
// ** returns RGB - vector of the emissive illumination **
// **** By: agnespet@kth.se 2020-05-19 ****
// TODO1: Use an intersection's surface material to find the emissive constant
vec3 EmmisiveComponent() {
	float ce = 0; //emissive constant is 0 because the rendered scene is not emissive
	vec3 emmissiveLight = ce * lightColor;
	return emmissiveLight;
}

// **** AMBIENT ILLUMINATION COMPONENT ****
// ** returns [R,G,B]-triplet of the ambient illumination **
// **** By: agnespet@kth.se 2020-05-19 ****
// TODO: Use surface material to find the ambient constant
vec3 AmbientComponent() {
	float ca = 0.1; // Ambient constant
	vec3 illumination_a; // Ambient illumination
	illumination_a = ca * lightColor; // multiply constant with scene light intensity
	return illumination_a;
}

// **** DIFFUSE COMPONENT ILLUMINATION ****
// ** takes the closest intersection of light ray and surface and **
// ** returns the intensity of the diffuse illumination as a vector **
// **** By: vendelav@kth.se 2020-05-19 ****
vec3 DiffuseComponent(const Intersection& i) {

	// The equation to use for the diffuse component:
	// Idiffuse = cd * lightColor * (l.normal . n)

	float cd = 1.0; // Diffuse surface constant

	vec3 l = lightPos - i.position; // Direction from surface to lightsource
	vec3 n = triangles[i.triangleIndex].normal; // Surface normal
	float dot = glm::dot(glm::normalize(l), n); // Angle between vector from surface to light source and surface normal
	// If the surface is perpendicular or turned away from the light source (angle <= 0) 
	// return 0, otherwise the angle so we can color the surface accordingly
	dot = dot < 0 ? 0.0f : dot;

	//** all black shadow, agnes' version **
	//calculate the unit vector describing the direction from the surface to the light source
	float rDist = glm::distance(lightPos, i.position);
	ClosestIntersection(i.position, l, triangles, a);
	//surface is in shadow if the distance to closest intersecting surface is closer than the light source
	if (a.triangleIndex != i.triangleIndex && a.distance < rDist) {
		// In shadow -> return color black [0,0,0]
		vec3 black(0, 0, 0);
		return black;
	}
	vec3 diffuseComp = cd * lightColor * dot;
	return diffuseComp;
}

// **** SPECULAR COMPONENT ILLUMINATION ****
// ** takes the closest intersection of light ray and surface and **
// ** returns the intensity of the specular illumination as a vector **
// **** By: vendelav@kth.se 2020-05-19 ****
// TODO1: (added by Agnes) add gloss on the red and blue boxes, I think we should use ClosestIntersection() for that 
// TODO2: (Added by Agnes) Invest why we have 4 light-sources? (one on each wall) (check description in "implementation notes/phong") (change to 100px when moving around)
vec3 SpecularComponent(const Intersection& i) {

	// Equation for computing the specular component
	// Ispecular = cs * lightColor * (r.normal . v.normal)^n

	float cs = 0.8; // Specular surface constant
	int n = 64; // Shininess constant, regulates size of specular highlights

	vec3 dir = glm::normalize(lightPos - i.position); // Direction from surface to lightsource
	vec3 surfaceNormal = triangles[i.triangleIndex].normal; // Surface normal

	// Lights reflected direction, negated because it should point from light source towards surface
	vec3 reflDir = glm::reflect(-dir, surfaceNormal);
	vec3 v = glm::normalize(cameraPos - i.position); // Direction from camera to surface
	float dot = glm::dot(reflDir, v); // Angle between the direction of the reflection and direction from the camera to the surface

	// If the light rays doesn't hit a point on the surface (angle <= 0) 
	// return 0, otherwise return the angle so we can color the surface accordingly
	dot = dot < 0 ? 0.0f : dot;

	// All black shadows
	// Using the global Intersection 'a' as it was calculated in DiffuseComponent using function ClosestIntersection()
	// Calculate the unit vector describing the direction from the surface to the light source
	float rDist = glm::distance(lightPos, i.position);
	//surface is in shadow if the distance to closest intersecting surface is closer than the light source
	if (a.triangleIndex != i.triangleIndex && a.distance < rDist) {
		// In shadow -> return color black [0,0,0]
		vec3 black(0, 0, 0);
		return black;
	}
	float shinePower = pow(dot, n);
	vec3 specularComp = cs * lightColor * shinePower;

	return specularComp;
}

//**** From Lab2 ****
//** returns true or false if an intersection has occurred **
bool ClosestIntersection(vec3 start, vec3 dir, const vector<Triangle>& triangles, Intersection& closestIntersection) {
	bool hit = false;
	float m = std::numeric_limits<float>::max();
	closestIntersection.distance = m;

	for (int i = 0; i < triangles.size(); i++) {
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
		if ((u >= 0) && (v >= 0) && ((u + v) < 1) && (t > 0)) {
			//return true 
			hit = true;
			//and update closestIntersection with:
			vec3 r2 = start + (t*dir);
			float distance = glm::distance(r2, start);
			if (distance <= closestIntersection.distance) {
				// 1. 3D position of the closest intersection
				closestIntersection.position = r2;
				// 2. the distance from start to the intersected triangle
				closestIntersection.distance = distance;
				// 3. Index of the ray that was intersected
				closestIntersection.triangleIndex = i;
			}
		}
	}
	return hit;
}
