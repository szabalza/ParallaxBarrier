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

	// kernel loading and OpenCL kernel creation
	_screenKernel = new OpenCLKernel("opencl/kernel/screenKernel.cl", "updateScreenPixels");
	_barrierKernel = new OpenCLKernel("opencl/kernel/barrierKernel.cl", "updateBarrierPixels");

	// images initialization after OpenCL contexts are created
	_barrierImage.allocate(barrierResolutionWidth, barrierResolutionHeight, OF_IMAGE_COLOR_ALPHA);
	_screenImage.allocate(screenResolutionWidth, screenResolutionHeight, OF_IMAGE_COLOR_ALPHA);
	_screenLeftImage.allocate(screenResolutionWidth, screenResolutionHeight, OF_IMAGE_COLOR_ALPHA);
	_screenRightImage.allocate(screenResolutionWidth, screenResolutionHeight, OF_IMAGE_COLOR_ALPHA);

	//OpenCL data initialization
	_screenKernelLocalSize[0] = 16;
	_screenKernelLocalSize[1] = 16;
	_screenKernelGlobalSize[0] = _screenKernelLocalSize[0] * ceil( ((float) _screenImage.width) / (float) _screenKernelLocalSize[0] );
	_screenKernelGlobalSize[1] = _screenKernelLocalSize[1] * ceil( ((float) _screenImage.height) / (float) _screenKernelLocalSize[1] );

	_barrierKernelLocalSize[0] = 16;
	_barrierKernelLocalSize[1] = 16;
	_barrierKernelGlobalSize[0] = _screenKernelLocalSize[0] * ceil( ((float) _barrierImage.width) / (float) _barrierKernelLocalSize[0] );
	_barrierKernelGlobalSize[1] = _screenKernelLocalSize[1] * ceil( ((float) _barrierImage.height) / (float) _barrierKernelLocalSize[1] );

	_screenPoints = new cl_char[screenResolutionWidth];
	_barrierPoints = new cl_char[barrierResolutionWidth];

	_screenPointsBuffer = new OpenCLBuffer(_screenPoints, _screenResolutionWidth * sizeof(cl_char));
	_screenKernelReadBuffers.push_back(_screenPointsBuffer);

	_barrierPointsBuffer = new OpenCLBuffer(_barrierPoints, _barrierResolutionWidth * sizeof(cl_char));
	_barrierKernelReadBuffers.push_back(_barrierPointsBuffer);

	_leftImageTexture = new OpenCLTexture(_screenLeftImage.getTextureReference().getTextureData().textureID, _screenLeftImage.getTextureReference().getTextureData().textureTarget);
	_screenKernelReadTextures.push_back(_leftImageTexture);
	_rightImageTexture = new OpenCLTexture(_screenRightImage.getTextureReference().getTextureData().textureID, _screenRightImage.getTextureReference().getTextureData().textureTarget);
	_screenKernelReadTextures.push_back(_rightImageTexture);

	_screenImageTexture = new OpenCLTexture(_screenImage.getTextureReference().getTextureData().textureID, _screenImage.getTextureReference().getTextureData().textureTarget);
	_screenKernelWriteTextures.push_back(_screenImageTexture);

	_barrierImageTexture = new OpenCLTexture(_barrierImage.getTextureReference().getTextureData().textureID, _barrierImage.getTextureReference().getTextureData().textureTarget);
	_barrierKernelWriteTextures.push_back(_barrierImageTexture);

	_screenKernel->defineArguments(NULL, &_screenKernelReadBuffers, NULL, &_screenKernelReadTextures, &_screenKernelWriteTextures);
	_barrierKernel->defineArguments(NULL, &_barrierKernelReadBuffers, NULL, NULL, &_barrierKernelWriteTextures);
	
	// initialize model transormation
	updateModelTransformation();
	_model.setWidth(_width*_modelScale);
}

ParallaxBarrier::~ParallaxBarrier()
{
	delete _screenKernel;
	delete _barrierKernel;
	delete _screenPoints;
	delete _barrierPoints;

	delete _screenPointsBuffer;
	delete _barrierPointsBuffer;
	delete _leftImageTexture;
	delete _rightImageTexture;
	delete _screenImageTexture;
	delete _barrierImageTexture;
}

void ParallaxBarrier::update(ofVec3f const &leftEyePosition, ofVec3f const &rightEyePosition)
{
	errorRatio = 0;

	ofVec3f modelLeftEyePosition3d = leftEyePosition * _modelTransformation;
	ofVec3f modelRightEyePosition3d = rightEyePosition * _modelTransformation;

	_modelLeftEyePosition = ofVec2f(modelLeftEyePosition3d.x, modelLeftEyePosition3d.z);
	_modelRightEyePosition = ofVec2f(modelRightEyePosition3d.x, modelRightEyePosition3d.z);

	//modify model for new eye positions
	_model.update(_modelLeftEyePosition, _modelRightEyePosition);

	//modify pixels
	updatePixels();
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

void ParallaxBarrier::updatePixels()
{
	updateBarrierPixels();
	updateScreenPixels();
}

void ParallaxBarrier::updateBarrierPixels()
{
	// points in the list are ordered pairs where 
	// the first point indicates the start of a non-transparent pixel zone, and 
	// the second point indicates the end of a non-transparent pixel zone
	const vector<float>& points = _model.getBarrierPoints();

	//initialize points array
	fill_n(_barrierPoints, _barrierResolutionWidth, 0);

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
			fill_n(&_barrierPoints[startPixel], endPixel - startPixel + 1, 1);

			//actual pixel starts black
			startPixel = actualPixel;
		} else if (pair && pixelPercentage > BARRIER_PIXEL_EPSILON_PERCENTAGE)
		{
			//actual pixel ends white
			endPixel = actualPixel;

			//paint white
			fill_n(&_barrierPoints[startPixel], endPixel - startPixel + 1, 1);

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
		fill_n(&_barrierPoints[startPixel], endPixel - startPixel + 1, 1);
	}

	//update screen textures in opencl
	_barrierKernel->execute(2, _barrierKernelGlobalSize, _barrierKernelLocalSize);
}

void ParallaxBarrier::updateScreenPixels()
{
	//points in the list delimit pixel zones for each eye view
	//first zone corresponds to left eye view
	const vector<float>& points = _model.getScreenPoints();

	// update points
	//initialize points array
	fill_n(_screenPoints, _screenResolutionWidth, -1);

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
					fill_n(&_screenPoints[startPixel], endPixel - startPixel + 1, 1);
				} else
				{
					//paint left view
					//paintVerticalPixels(leftEyeView, startPixel, endPixel, _screenImage);
				}

				//paint one black pixel
				fill_n(&_screenPoints[actualPixel], 1, 0);

				//next pixel starts left/right view
				startPixel = actualPixel + 1;

				errorRatio++;
			} else if (pair && pixelPercentage < SCREEN_PIXEL_EPSILON_PERCENTAGE)
			{
				//previous pixel ends right view
				endPixel = actualPixel - 1;

				//paint right view
				fill_n(&_screenPoints[startPixel], endPixel - startPixel + 1, 1);

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
				fill_n(&_screenPoints[startPixel], endPixel - startPixel + 1, 1);

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

	// end update points

	//update screen textures in opencl
	_screenKernel->execute(2, _screenKernelGlobalSize, _screenKernelLocalSize);
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

ofImage& ParallaxBarrier::getScreenImage()
{
	return _screenImage;
}

ofImage& ParallaxBarrier::getBarrierImage()
{
	return _barrierImage;
}

ofImage& ParallaxBarrier::getScreenLeftImage()
{
	return _screenLeftImage;
}

ofImage& ParallaxBarrier::getScreenRightImage()
{
	return _screenRightImage;
}
