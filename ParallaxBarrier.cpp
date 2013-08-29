#include "ParallaxBarrier.h"

ParallaxBarrier::ParallaxBarrier(float width, float height, int screenResolutionWidth, int screenResolutionHeight, int barrierResolutionWidth, int barrierResolutionHeight, float spacing, ofVec3f const &position, ofVec3f const &viewDirection, ofVec3f const &upDirection)
{
	_width = width;
	_height = height;
	_screenResolutionWidth = screenResolutionWidth;
	_screenResolutionHeight = screenResolutionHeight;
	_barrierResolutionWidth = barrierResolutionWidth;
	_barrierResolutionHeight = barrierResolutionHeight;
	_spacing = spacing;
	_position = position;
	_viewDirection = viewDirection;
	_upDirection = upDirection;
	_screenInversePixelWidth = _screenResolutionWidth/_width;
	_barrierInversePixelWidth = _barrierResolutionWidth/_width;
	_modelScale = 1.f/spacing;

	_barrierImage.allocate(barrierResolutionWidth, barrierResolutionHeight, OF_IMAGE_COLOR);
	_screenImage.allocate(screenResolutionWidth, screenResolutionHeight, OF_IMAGE_COLOR);

	updateModelTransformation();

	_model.setWidth(_width*_modelScale);
}

ParallaxBarrier::~ParallaxBarrier()
{
}

void ParallaxBarrier::update(ofVec3f const &leftEyePosition, ofVec3f const &rightEyePosition, ofImage &leftEyeView, ofImage &rightEyeView)
{
	errorRatio = 0;

	ofVec3f modelLeftEyePosition3d = leftEyePosition * _modelTransformation;
	ofVec3f modelRightEyePosition3d = rightEyePosition * _modelTransformation;

	_modelLeftEyePosition = ofVec2f(modelLeftEyePosition3d.x, modelLeftEyePosition3d.z);
	_modelRightEyePosition = ofVec2f(modelRightEyePosition3d.x, modelRightEyePosition3d.z);

	//modify model for new eye positions
	_model.update(_modelLeftEyePosition, _modelRightEyePosition);

	//modify pixels
	updatePixels(leftEyeView, rightEyeView);
}

void ParallaxBarrier::updateModelTransformation()
{
	ofMatrix4x4 modelScale, modelRotation, modelUpRotation, modelTranslation, modelCenterTranslation;
	modelScale.makeScaleMatrix(_modelScale, _modelScale, _modelScale);

	modelRotation.makeRotationMatrix(_viewDirection, ofVec3f(0,0,1));
	modelUpRotation.makeRotationMatrix(modelRotation*_upDirection, ofVec3f(0,1,0));
	modelTranslation.makeTranslationMatrix(ofVec3f(_width * 0.5,0,0));
	modelCenterTranslation.makeTranslationMatrix(-_position);

	_modelTransformation = modelCenterTranslation * modelRotation * modelUpRotation * modelTranslation * modelScale;
}

void ParallaxBarrier::updatePixels(ofImage &leftEyeView, ofImage &rightEyeView)
{
	updateBarrierPixels();
	updateScreenPixels(leftEyeView, rightEyeView);
}

void ParallaxBarrier::updateBarrierPixels()
{
	// points in the list are ordered pairs where 
	// the first point indicates the start of a non-transparent pixel zone, and 
	// the second point indicates the end of a non-transparent pixel zone
	const vector<float>& points = _model.getBarrierPoints();

	paintVerticalPixelsBlack(_barrierImage);

	float itValue, floatingPixel, pixelPercentage;
	int actualPixel, startPixel = 0, endPixel;
	bool pair = true;
	for (vector<float>::const_iterator it = points.begin(), end = points.end(); it != end; ++it)
	{
		itValue = (*it)*_spacing;
		floatingPixel = itValue*_barrierInversePixelWidth;
		actualPixel = floor(floatingPixel);
		pixelPercentage = floatingPixel - floor(floatingPixel);

		if (pair && pixelPercentage <= BARRIER_PIXEL_EPSILON_PERCENTAGE)
		{
			//previous pixel ends white 
			endPixel = actualPixel - 1;

			//paint white
			paintVerticalPixels(ofColor::white, startPixel, endPixel, _barrierImage);

			//actual pixel starts black
			startPixel = actualPixel;
		} else if (pair && pixelPercentage > BARRIER_PIXEL_EPSILON_PERCENTAGE)
		{
			//actual pixel ends white
			endPixel = actualPixel;

			//paint white
			paintVerticalPixels(ofColor::white, startPixel, endPixel, _barrierImage);

			//next pixel starts black
			startPixel = actualPixel + 1;
		} else if (!pair && pixelPercentage < BARRIER_PIXEL_EPSILON_PERCENTAGE)
		{
			//previous pixel ends black
			endPixel = actualPixel - 1;

			//paint black
			//paintVerticalPixels(ofColor::black, startPixel, endPixel, _barrierImage);

			//actual pixel starts white
			startPixel = actualPixel;
		} else if (!pair && pixelPercentage >= BARRIER_PIXEL_EPSILON_PERCENTAGE)
		{
			//actual pixel ends black
			endPixel = actualPixel;

			//paint black
			//paintVerticalPixels(ofColor::black, startPixel, endPixel, _barrierImage);

			//next pixel starts white
			startPixel = actualPixel + 1;
		}

		pair = !pair;
	}

	if (startPixel < _barrierImage.width)
	{
		//actual pixel ends white
		endPixel = _barrierImage.width - 1;

		//paint white
		paintVerticalPixels(ofColor::white, startPixel, endPixel, _barrierImage);
	}

	_barrierImage.update();
}

void ParallaxBarrier::updateScreenPixels(ofImage &leftEyeView, ofImage &rightEyeView)
{
	//points in the list delimit pixel zones for each eye view
	//first zone corresponds to left eye view
	const vector<float>& points = _model.getScreenPoints();

	_screenImage.clone(leftEyeView);

	float itValue, floatingPixel, pixelPercentage;
	int actualPixel, startPixel = -1, endPixel;
	bool pair = true;
	for (vector<float>::const_iterator it = points.begin(), end = points.end(); it != end; ++it)
	{
		itValue = (*it)*_spacing;
		floatingPixel = itValue*_screenInversePixelWidth;
		actualPixel = floor(floatingPixel);
		pixelPercentage = floatingPixel - floor(floatingPixel);

		if (startPixel != -1) 
		{
			if (pixelPercentage >= SCREEN_PIXEL_EPSILON_PERCENTAGE && pixelPercentage <= (1 - SCREEN_PIXEL_EPSILON_PERCENTAGE)) 
			{
				//previous pixel ends left/right view
				endPixel = actualPixel - 1;

				//paint left/right view
				if (pair)
				{
					//paint right view
					paintVerticalPixels(rightEyeView, startPixel, endPixel, _screenImage);
				} else
				{
					//paint left view
					//paintVerticalPixels(leftEyeView, startPixel, endPixel, _screenImage);
				}

				//paint one black pixel
				paintVerticalPixels(ofColor::black, actualPixel, actualPixel, _screenImage);

				//next pixel starts left/right view
				startPixel = actualPixel + 1;

				errorRatio++;
			} else if (pair && pixelPercentage < SCREEN_PIXEL_EPSILON_PERCENTAGE)
			{
				//previous pixel ends right view
				endPixel = actualPixel - 1;

				//paint right view
				paintVerticalPixels(rightEyeView, startPixel, endPixel, _screenImage);

				//actual pixel starts left view
				startPixel = actualPixel;

			} else if (!pair && pixelPercentage < SCREEN_PIXEL_EPSILON_PERCENTAGE)
			{
				//previous pixel ends left view
				endPixel = actualPixel - 1;

				//paint left view
				//paintVerticalPixels(leftEyeView, startPixel, endPixel, _screenImage);

				//actual pixel starts right view
				startPixel = actualPixel;
				
			} else if (pair && pixelPercentage > (1 - SCREEN_PIXEL_EPSILON_PERCENTAGE))
			{
				//actual pixel ends right view
				endPixel = actualPixel;

				//paint right view
				paintVerticalPixels(rightEyeView, startPixel, endPixel, _screenImage);

				//next pixel starts left view
				startPixel = actualPixel + 1;

			} else if (!pair && pixelPercentage > (1 - SCREEN_PIXEL_EPSILON_PERCENTAGE))
			{
				//actual pixel ends left view
				endPixel = actualPixel;

				//paint left view
				//paintVerticalPixels(leftEyeView, startPixel, endPixel, _screenImage);

				//next pixel starts right view
				startPixel = actualPixel + 1;

			}
		} else 
		{
			startPixel = actualPixel;
		}

		pair = !pair;
	}

	_screenImage.update();
}

void ParallaxBarrier::paintVerticalPixels(ofColor const &color, int start, int end, ofImage &image)
{
	int index;
	unsigned char* imagePixels = image.getPixels();
	for(int i = start; i <= end; i++)
	{
		for(int j = 0; j < image.height; j++)
		{
			index = (image.width*j + i)*3;
			imagePixels[index] = color.r;
			imagePixels[index + 1] = color.g;
			imagePixels[index + 2] = color.b;
		}
	}
}

void ParallaxBarrier::paintVerticalPixels(ofImage &sourceImage, int start, int end, ofImage &image)
{
	int index;
	unsigned char* imagePixels = image.getPixels();
	unsigned char* sourcePixels = sourceImage.getPixels();
	for(int i = start; i <= end; i++)
	{
		for(int j = 0; j < image.height; j++)
		{
			index = (image.width*j + i)*3;
			imagePixels[index] = sourcePixels[index];
			imagePixels[index + 1] = sourcePixels[index + 1];
			imagePixels[index + 2] = sourcePixels[index + 2];
		}
	}
}

void ParallaxBarrier::paintVerticalPixelsBlack(ofImage &image)
{
	unsigned char* imagePixels = image.getPixels();
	fill(imagePixels, imagePixels + image.width*image.height, 0);
}

float ParallaxBarrier::getWidth()
{
	return _width;
}

float ParallaxBarrier::getHeight()
{
	return _height;
}

int ParallaxBarrier::getBarrierResolutionWidth()
{
	return _barrierResolutionWidth;
}

int ParallaxBarrier::getBarrierResolutionHeight()
{
	return _barrierResolutionHeight;
}

int ParallaxBarrier::getScreenResolutionWidth()
{
	return _screenResolutionWidth;
}

int ParallaxBarrier::getScreenResolutionHeight()
{
	return _screenResolutionHeight;
}

float ParallaxBarrier::getSpacing()
{
	return _spacing;
}

void ParallaxBarrier::setWidth(float width)
{
	this->_width = width;
	_model.setWidth(_width*_modelScale);
	updateModelTransformation();
}

void ParallaxBarrier::setHeight(float height)
{
	this->_height = height;
}

void ParallaxBarrier::setBarrierResolutionWidth(int barrierResolutionWidth)
{
	this->_barrierResolutionWidth = barrierResolutionWidth;
}

void ParallaxBarrier::setBarrierResolutionHeight(int barrierResolutionHeight)
{
	this->_barrierResolutionHeight = barrierResolutionHeight;
}

void ParallaxBarrier::setScreenResolutionWidth(int screenResolutionWidth)
{
	this->_screenResolutionWidth = screenResolutionWidth;
}

void ParallaxBarrier::setScreenResolutionHeight(int screenResolutionHeight)
{
	this->_screenResolutionHeight = screenResolutionHeight;
}

void ParallaxBarrier::setSpacing(float spacing)
{
	this->_spacing = spacing;
	_modelScale = 1.f / _spacing;
	_model.setWidth(_width*_modelScale);
	updateModelTransformation();
}

const ofVec3f& ParallaxBarrier::getPosition()
{
	return _position;
}

const ofVec3f& ParallaxBarrier::getViewDirection()
{
	return _viewDirection;
}

const ofVec3f& ParallaxBarrier::getUpDirection()
{
	return _upDirection;
}

void ParallaxBarrier::setPosition(ofVec3f position)
{
	this->_position = position;
	updateModelTransformation();
}

void ParallaxBarrier::setViewDirection(ofVec3f viewDirection)
{
	this->_viewDirection = viewDirection;
	updateModelTransformation();
}

void ParallaxBarrier::setUpDirection(ofVec3f upDirection)
{
	this->_upDirection = upDirection;
	updateModelTransformation();
}

const ofImage& ParallaxBarrier::getScreenImage()
{
	return _screenImage;
}

const ofImage& ParallaxBarrier::getBarrierImage()
{
	return _barrierImage;
}
