#include "ParallaxBarrierModel.h"

ParallaxBarrierModel::ParallaxBarrierModel(float width)
{
	_width = width;
}

ParallaxBarrierModel::~ParallaxBarrierModel()
{
}

bool ParallaxBarrierModel::update(ofVec2f leftEyePosition, ofVec2f rightEyePosition)
{

	float minPoint = getMinVisiblePoint(leftEyePosition, rightEyePosition);
	float maxPoint = getMaxVisiblePoint(leftEyePosition, rightEyePosition);
	_screenPoints.clear();
	_shutterPoints.clear();

	if (minPoint == -1 || maxPoint == -1)
	{
		return false;
	}

	vector<float> screenPoints; 
	vector<float> shutterPoints;
	float screenIterationPoint = -1;
	float newScreenPoint = -1;
	float newShutterPoint = -1;
	float rightShutterDistanceFraction = 1/rightEyePosition.y;
	float leftShutterDistanceFraction = 1/leftEyePosition.y;
	float BCoef = (rightEyePosition.x * rightShutterDistanceFraction - leftEyePosition.x * leftShutterDistanceFraction) / (1 - leftShutterDistanceFraction);
	float ACoef = (1 - rightShutterDistanceFraction) / (1 - leftShutterDistanceFraction);
	float cumulativeCoef1 = minPoint * ACoef;
	float cumulativeCoef2 = 1;
	int errorCounter = 0;
	bool pair = true;


	//initial point
	newScreenPoint = minPoint;
	screenPoints.push_back(newScreenPoint);
	screenIterationPoint = newScreenPoint;

	while (screenIterationPoint < maxPoint && errorCounter <= PARALLAX_BARRIER_MAX_ITERATIONS)
	{
		if (pair)
		{
			newShutterPoint = rightEyePosition.x * rightShutterDistanceFraction + screenIterationPoint * (1-rightShutterDistanceFraction) ;
			shutterPoints.push_back(newShutterPoint);
		}

		newScreenPoint = cumulativeCoef1 + BCoef * cumulativeCoef2;
		if (newScreenPoint >= _width)
		{
			screenPoints.push_back(maxPoint);
		} 
		else
		{
			screenPoints.push_back(newScreenPoint);
		}
		
		if (pair)
		{
			newShutterPoint = rightEyePosition.x * rightShutterDistanceFraction + newScreenPoint * (1-rightShutterDistanceFraction) ;
			shutterPoints.push_back(newShutterPoint);
		}

		screenIterationPoint = newScreenPoint;
		cumulativeCoef1 = cumulativeCoef1 * ACoef;
		cumulativeCoef2 = cumulativeCoef2 * ACoef + 1;
		errorCounter++;
		pair = !pair;
	}

	if (errorCounter > PARALLAX_BARRIER_MAX_ITERATIONS)
	{
		return false;
	}

	_screenPoints.assign(screenPoints.begin(), screenPoints.end());

	pair = true;
	bool startValue = true;
	bool endValue = true;
	float itValue;
	_shutterPoints.clear();

	for (vector<float>::const_iterator it = shutterPoints.begin(), end = shutterPoints.end(); it != end; ++it)
	{
		itValue = *it;

		if (itValue >= 0 && itValue <= _width) {

			if (startValue)
			{
				if (!pair)
				{
					_shutterPoints.push_back(0);
				}
				startValue = false;
			}

			_shutterPoints.push_back(itValue);
		}
		else if (itValue > _width)
		{
			if (endValue)
			{
				if (!pair)
				{
					_shutterPoints.push_back(_width);
				}
				endValue = false;
			}
		}

		pair = !pair;
	}

	return true;
}

float ParallaxBarrierModel::getMaxVisiblePoint(ofVec2f leftEyePosition, ofVec2f rightEyePosition)
{
	ofVec2f maxLineOfSight = (rightEyePosition - leftEyePosition).rotate(-NON_VISIBLE_ANGLE);
	
	float angle = maxLineOfSight.angle(ofVec2f(-1,0));

	if (angle >= 0 && angle <= NON_VISIBLE_ANGLE)
	{
		return -1;
	}

	if (maxLineOfSight.align(ofVec2f(-1,0), DEGREES_EPSILON))
	{
		return -1;
	}

	if (maxLineOfSight.align(ofVec2f(1,0), DEGREES_EPSILON))
	{
		return _width;
	}

	if (maxLineOfSight.y > 0)
	{
		return _width;
	}

	float intersection = intersectionXAxis(rightEyePosition, maxLineOfSight);

	if (intersection <= 0)
	{
		return -1;
	}
	else if (intersection > _width)
	{
		return _width;
	}
	else 
	{
		return intersection;
	}
}

float ParallaxBarrierModel::getMinVisiblePoint(ofVec2f leftEyePosition, ofVec2f rightEyePosition)
{
	ofVec2f minLineOfSight = (leftEyePosition - rightEyePosition).rotate(NON_VISIBLE_ANGLE);

	float angle = minLineOfSight.angle(ofVec2f(1,0));

	if (angle <= 0 && angle >= -NON_VISIBLE_ANGLE)
	{
		return -1;
	}
	
	if (minLineOfSight.align(ofVec2f(1,0), DEGREES_EPSILON))
	{
		return -1;
	}

	if (minLineOfSight.align(ofVec2f(-1,0), DEGREES_EPSILON))
	{
		return 0;
	}

	if (minLineOfSight.y > 0)
	{
		return 0;
	}

	float intersection = intersectionXAxis(leftEyePosition, minLineOfSight);

	if (intersection > _width)
	{
		return -1;
	}
	else if (intersection <= 0)
	{
		return 0;
	}
	else 
	{
		return intersection;
	}
}

float ParallaxBarrierModel::intersectionXAxis(ofVec2f point, ofVec2f dir)
{
	return point.x - point.y * dir.x / dir.y;
}

void ParallaxBarrierModel::setWidth(float width)
{
	_width = width;
}

float ParallaxBarrierModel::getWidth()
{
	return _width;
}

const vector<float>& ParallaxBarrierModel::getScreenPoints()
{
	return _screenPoints;
}

const vector<float>& ParallaxBarrierModel::getShutterPoints()
{
	return _shutterPoints;
}

