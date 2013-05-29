#pragma once

#include "ofVec2f.h"
#include <vector>

#define PARALLAX_BARRIER_MAX_ITERATIONS 2000

#define DEGREES_EPSILON 0.01f
#define NON_VISIBLE_ANGLE 20.f

using namespace std;

// ParallaxBarrierModel assumptions:
// - Shutter 'y' coordinate is 1
// - Screen 'y' coordinate is 0
// - Distance from screen to shutter is 1
// - Eyes 'y' coordinate must be bigger than 1
// - left/right denomination corresponds to the viewer perspective
// - First point in 'ScreenPoints' corresponds to the start of a left eye view zone
// - Points in 'ScreenPoints' altarnate between 'Left Eye Start View Zone'/'Right Eye Start View Zone'
// - First point in 'ShutterPoints' corresponds to the start of a non-transparent zone in the shutter 
// - Points in 'ShutterPoints' altarnate between 'Translucid Start Zone'/'Non-Transparent Start Zone'
class ParallaxBarrierModel
{
public:
	ParallaxBarrierModel(float width);
	virtual ~ParallaxBarrierModel();

	bool update(ofVec2f leftEyePosition, ofVec2f rightEyePosition);

	float getWidth();
	const vector<float>& getScreenPoints();
	const vector<float>& getShutterPoints();
private:
	float _width;

	vector<float> _screenPoints; 
	vector<float> _shutterPoints;

	float getMinVisiblePoint(ofVec2f leftEyePosition, ofVec2f rightEyePosition);
	float getMaxVisiblePoint(ofVec2f leftEyePosition, ofVec2f rightEyePosition);
	float intersectionXAxis(ofVec2f point, ofVec2f dir);

};

