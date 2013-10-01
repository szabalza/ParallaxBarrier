__kernel void updateScreenPixels(	__constant float* screenPoints, __constant int screenPointsSize, 
									__read_only image2d_t leftImage, __read_only image2d_t rightImage, 
									__write_only image2d_t screenImage)
{
	int i = get_global_id(0);
	int j = get_global_id(1);

	int screenImageWidth = get_image_width(screenImage);
	int screenImageHeight = get_image_height(screenImage);
	
	if (i < screenImageWidth && j < screenImageHeight)
	{
		int2 coord = (int2) (i, j);
		float4 color = (float4)(0.f, 0.f, 1.f, 1.f);
		write_imagef(screenImage, coord, color);
	}

}