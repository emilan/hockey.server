//#include "stdafx.h"
#include "ObjectTracker.h"
#include <fstream>
#include <math.h>
#define checkpics
//Creates a default ObjectTracker and intializes its variables. For the Lines.
ObjectTracker::ObjectTracker(IplImage* _searchRegion,string color,bool _calibrationMode){
	calibrationMode=_calibrationMode;
	searchRegion=_searchRegion;//omr�det att leta p�
	minColor=CvScalar();
	maxColor=CvScalar();
	
	if(!color.empty()){
		colorName=color;//f�rgen att leta efter
		setColor(color);
	}
	//skapar buffrar f�r de olika stegen i bildhanterings processen
	maskedImage=cvCreateImage(cvGetSize(searchRegion),8,3);
	hsv=cvCreateImage(cvGetSize(searchRegion),8,3);
	tresholdImage=cvCreateImage(cvGetSize(searchRegion),8,1);
	erodeImg=cvCreateImage(cvGetSize(searchRegion),8,1);
	moments = (CvMoments*)malloc(sizeof(CvMoments));
	
}
void ObjectTracker::trackObject(IplImage* image,CvPoint2D32f* point) {
	//�ndrar storlek p� buffrarna om bilden att analysera �r fel storlek
	if(image->height!=searchRegion->height||image->width!=searchRegion->width){
		cerr<<"wrong image format: got "<<image->height<<" "<<image->width<<", expected "<<maskedImage->height<<" "<<maskedImage->width<<endl;
		resizeToImage(&searchRegion,image);
		resizeToImage(&tresholdImage,image);
		resizeToImage(&maskedImage,image);
		resizeToImage(&hsv,image);
		resizeToImage(&erodeImg,image);
	}

	cvCvtColor( image, hsv, CV_BGR2HSV );//g�r om rgb bild till hsv
	cvCopy(hsv,maskedImage,searchRegion);//se rapport 2011, bildhantering
	
	//cvShowImage( "h", image );

	cvInRangeS(maskedImage, minColor, maxColor, tresholdImage );
	
	cvErode(tresholdImage,erodeImg,NULL,1);
	
	//applies the searchregion to the picture
	
	//extracts the proper colorrange from the picture
	cvMoments(erodeImg, moments, 1);
	
	// The actual moment values
	double moment10 = cvGetSpatialMoment(moments, 1, 0); //r�knar ut intensitetscentrum i x-led
	double moment01 = cvGetSpatialMoment(moments, 0, 1);//r�knar ut intensitetscentrum i y-led
	double area = cvGetCentralMoment(moments, 0, 0); //total intensitet
	

	double posX=moment10/area;
	double posY=moment01/area;
	if(area==0){ //f�rhindrar error om ingen punkt finns
		posX=0;
		posY=0;
	}
	
	CvPoint2D32f tempPoint=cvPoint2D32f(posX,posY);
	*point=tempPoint;

	if(calibrationMode) {
		/*char buf[50];
		sprintf_s(buf, "moment10: %.2f", moment10);
		cvPutText(erodeImg, buf, cvPoint(0, 15), &cvFont(1), cvScalar(255, 255, 255, 255));

		sprintf_s(buf, "moment01: %.2f", moment01);
		cvPutText(erodeImg, buf, cvPoint(0, 30), &cvFont(1), cvScalar(255, 255, 255, 255));

		sprintf_s(buf, "area    : %.2f", moment01);
		cvPutText(erodeImg, buf, cvPoint(0, 45), &cvFont(1), cvScalar(255, 255, 255, 255));

		sprintf_s(buf, "posX: %.2f", posX);
		cvPutText(erodeImg, buf, cvPoint(200, 15), &cvFont(1), cvScalar(255, 255, 255, 255));

		sprintf_s(buf, "posY: %.2f", posY);
		cvPutText(erodeImg, buf, cvPoint(200, 30), &cvFont(1), cvScalar(255, 255, 255, 255));*/

		cvCircle(image, cvPoint(posX, posY), 5, cvScalar(0, 255, 0, 255));

		cvShowImage("treshold image",tresholdImage );
		cvShowImage("eroded image",erodeImg );
		cvShowImage("masked image",maskedImage );

		cvShowImage("image",image );
		
		//while(cvWaitKey(1)<0);
		
	}
}
//int ObjectTracker::colorAtPixel(IplImage* mask,int x, int y){
//	if(x>mask->width||x<0||y>mask->height||y<0){
//		cerr<<"bad coordinates";
//		exit(1);
//	}
//	return (int)mask->imageData[x*mask->widthStep+y*mask->nChannels];
//}
void ObjectTracker::setColor(CvScalar _minColor, CvScalar _maxColor){
	minColor=_minColor;
	maxColor=_maxColor;
}

//plockar fram max och min hsv-v�rden f�r �nskad f�rg ur text-fil: <color>.txt
void ObjectTracker::setColor(string color){
	ifstream in;
	char filename[20];
	sprintf_s(filename, "%s.txt", color.c_str());
	in.open(filename);

	if (in.is_open()) {
		string inColor;
		int hMin, hMax, sMin, sMax, vMin, vMax = 0;
		
		if (getline(in, inColor)) { 
			if (sscanf(inColor.c_str(), "%d %d %d %d %d %d", &hMin, &hMax, &sMin, &sMax, &vMin, &vMax) == 6) {
				minColor=cvScalar(hMin, sMin, vMin, 0);
				maxColor=cvScalar(hMax, sMax, vMax, 0);
			}
			else cerr << filename << " found, but could not parse it" << endl;
		}
		in.close();
	}
	else cerr << "could not open file " << filename << endl;
}

void ObjectTracker::saveColor(){//sparar ner nuvarande f�rg till fil
	if(!colorName.empty()){
	ofstream out;
	out.open(colorName+".txt");
	out	<<minColor.val[0]
		<<' '<<maxColor.val[0]
		<<' '<<minColor.val[1]
		<<' '<<maxColor.val[1]
		<<' '<<minColor.val[2]
		<<' '<<maxColor.val[2];
		out.close();
	}
}
void ObjectTracker::setCalibrationMode(bool cal){
	calibrationMode=cal;
	if(!calibrationMode){//tar bort bilder om calibration mode avslutas
		cvDestroyWindow("treshold image");
		cvDestroyWindow("masked image");
		cvDestroyWindow("image");
		cvDestroyWindow("eroded image");
	}
}

CvScalar ObjectTracker::getMaxColor() {
	return maxColor;
}

CvScalar ObjectTracker::getMinColor() {
	return minColor;
}

//void ObjectTracker::lighten(IplImage* img, int p)
//{
//	//*img = p**img;
//}

//void contrast(IplImage* img, int p)
//{
//	cvPow(img,img, p);
//}
void resizeToImage(IplImage** toResize,IplImage* rightSize){//skalar om en bild s� att den har samma storlek som en annan bild
	IplImage* temp=cvCreateImage(cvSize(rightSize->width,rightSize->height),(*toResize)->depth,(*toResize)->nChannels);
	cvResize(*toResize,temp);
	*toResize=temp;
}
