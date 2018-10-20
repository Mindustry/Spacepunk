// Camera.hpp
// Contains 3D drawing functions and viewport data

#pragma once

#define GLM_FORCE_RADIANS
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

#include "Component.hpp"
#include "Rect.hpp"
#include "Cube.hpp"
#include "Line3D.hpp"
#include "Model.hpp"

class Renderer;
class World;

class Camera : public Component {
public:
	// drawing mode type
	enum drawmode_t {
		DRAW_DEPTH,

		// these two for each light in a scene
		DRAW_STENCIL,
		DRAW_STANDARD,
			
		// additional fx passes
		DRAW_GLOW,
		DRAW_TRIANGLES,
		DRAW_DEPTHFAIL,
		DRAW_SILHOUETTE,

		DRAW_TYPE_LENGTH
	};

	// point type
	struct point_t {
		unsigned int x;
		unsigned int y;
	};

	Camera(Entity& _entity, Component* _parent);
	virtual ~Camera();

	// camera model
	static const char* meshStr;
	static const char* materialStr;
	const Mesh::shadervars_t shaderVars;

	// getters & setters
	virtual type_t		getType() const override	{ return COMPONENT_CAMERA; }
	Renderer*&			getRenderer()				{ return renderer; }
	const float&		getClipNear() const			{ return clipNear; }
	const float&		getClipFar() const			{ return clipFar; }
	const Rect<Sint32>&	getWin() const				{ return win; }
	const Sint32&		getFov() const				{ return fov; }
	const glm::mat4&	getProjMatrix() const		{ return projMatrix; }
	const glm::mat4&	getViewMatrix() const		{ return viewMatrix; }
	const glm::mat4&	getProjViewMatrix() const	{ return projViewMatrix; }
	const drawmode_t	getDrawMode() const			{ return drawMode; }
	const bool&			isOrtho() const				{ return ortho; }

	void	setClipNear(float _clipNear)		{ clipNear = _clipNear; }
	void	setClipFar(float _clipFar)			{ clipFar = _clipFar; }
	void	setWin(const Rect<Sint32>& rect)	{ win = rect; }
	void	setFov(Sint32 _fov)					{ fov = _fov; }
	void	setDrawMode(drawmode_t _drawMode)	{ drawMode = _drawMode; }
	void	setOrtho(const bool _ortho)			{ ortho = _ortho; }

	// sets up the 3D projection for drawing
	void setupProjection();

	// resets the drawing matrices
	void resetMatrices();

	// projects the given world position onto the screen
	// @param original: the world position to be projected
	// return: the screen position to be returned
	Vector worldPosToScreenPos( const Vector& original ) const;

	// determines origin and direction vectors for a ray that extends through a given point on the screen
	// @param x: the X position on the screen to peer through
	// @param y: the Y position on the screen to peer through
	// @param out_origin: the origin point of the ray to be returned
	// @param out_direction: the direction vector of the ray in world space to be returned
	void screenPosToWorldRay( int x, int y, Vector& out_origin, Vector& out_direction ) const;

	// draws a cube in the current camera view
	// @param transform: the transformation to apply to the cube
	// @param color: the color of the cube to draw
	void drawCube( const glm::mat4& transform, const glm::vec4& color );

	// draws a 3d line in the current camera view
	// @param width: the width of the line in pixels
	// @param src: the starting point of the line in world space
	// @param dest: the ending point of the line in world space
	// @param color: the color of the line to draw
	void drawLine3D( const float width, const glm::vec3& src, const glm::vec3& dest, const glm::vec4& color );

	// marks a spot on the screen to draw a point
	// @param x: the x-coord of the point
	// @param y: the y-coord of the point
	void markPoint( unsigned int x, unsigned int y );

	// draws the camera
	// @param camera: the camera through which to draw the camera
	// @param light: the light by which the camera should be illuminated (or nullptr for no illumination)
	virtual void draw(Camera& camera, Light* light) override;

	// draws all marked points
	void drawPoints();

	// load the component from a file
	// @param fp: the file to read from
	virtual void load(FILE* fp) override;

	// save/load this object to a file
	// @param file interface to serialize with
	virtual void serialize(FileInterface* file) override;

protected:
	Renderer* renderer = nullptr;

	// drawing mode
	drawmode_t drawMode = DRAW_STANDARD;

	// view and projection matrices
	glm::mat4 projMatrix;
	glm::mat4 viewMatrix;
	glm::mat4 projViewMatrix;

	// viewport and projection variables
	float clipNear = 8.f;
	float clipFar = 1024.f;
	Rect<Sint32> win;
	Sint32 fov = 70;
	bool ortho = false;

	// built-in basic objects
	Cube cube;
	Line3D line3D;
	LinkedList<point_t> points;
};