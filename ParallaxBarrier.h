#pragma once

#include "ofVec3f.h"
#include "ofImage.h"

#include "ParallaxBarrierModel.h"

#define SCREEN_PIXEL_EPSILON_PERCENTAGE 0.10f
#define BARRIER_PIXEL_EPSILON_PERCENTAGE 0.05f

class ParallaxBarrier
{
public:
	ParallaxBarrier(float width, float height, int screenResolutionWidth, int screenResolutionHeight, int barrierResolutionWidth, int barrierResolutionHeight, float spacing, ofVec3f const &position, ofVec3f const &viewDirection, ofVec3f const &upDirection);
	virtual ~ParallaxBarrier();

	float getWidth();
	float getHeight();
	int getScreenResolutionWidth();
	int getScreenResolutionHeight();
	int getBarrierResolutionWidth();
	int getBarrierResolutionHeight();
	float getSpacing();
	const ofVec3f& getPosition();
	const ofVec3f& getViewDirection();
	const ofVec3f& getUpDirection();
	void setWidth(float width);
	void setHeight(float height);
	void setScreenResolutionWidth(int screenResolutionWidth);
	void setScreenResolutionHeight(int screenResolutionHeight);
	void setBarrierResolutionWidth(int barrierResolutionWidth);
	void setBarrierResolutionHeight(int barrierResolutionHeight);
	void setSpacing(float spacing);
	void setPosition(ofVec3f position);
	void setViewDirection(ofVec3f viewDirection);
	void setUpDirection(ofVec3f upDirection);

	void update(ofVec3f const &leftEyePosition, ofVec3f const &rightEyePosition, ofImage &leftEyeView, ofImage &rightEyeView);

	const ofImage& getScreenImage();
	const ofImage& getBarrierImage();

	int getErrorRatio();
private:
	float _width;
	float _height;
	int _barrierResolutionWidth;
	int _barrierResolutionHeight;
	int _screenResolutionWidth;
	int _screenResolutionHeight;
	float _spacing;
	ofVec3f _position;
	ofVec3f _viewDirection;
	ofVec3f _upDirection;
	float _barrierInversePixelWidth;
	float _screenInversePixelWidth;

	int errorRatio;

	// model transformation
	ofMatrix4x4 _modelTransformation;
	float _modelScale;
	ofVec2f _modelLeftEyePosition;
	ofVec2f _modelRightEyePosition;

	ParallaxBarrierModel _model;

	ofImage _barrierImage;
	ofImage _screenImage;

	void updateModelTransformation();
	void updatePixels(ofImage &leftEyeView, ofImage &rightEyeView);
	void updateScreenPixels(ofImage &leftEyeView, ofImage &rightEyeView);
	void updateBarrierPixels();

	void paintVerticalPixels(ofColor const &color, int start, int end, ofImage &image);
	void paintVerticalPixels(ofImage &sourceImage, int start, int end, ofImage &image);

	void paintVerticalPixelsBlack(ofImage &image);
};

