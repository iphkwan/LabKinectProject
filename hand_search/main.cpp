#include<iostream>
#include<fstream>

#include "XnCppWrapper.h"

#pragma comment(lib, "openni.lib")

#pragma comment(lib, "opencv_core243d.lib")
#pragma comment(lib, "opencv_highgui243d.lib")
#pragma comment(lib, "opencv_imgproc243d.lib")
#pragma comment(lib, "opencv_calib3d243d.lib")
#pragma comment(lib, "opencv_features2d243d.lib")
#pragma comment(lib, "opencv_contrib243d.lib")
#pragma comment(lib, "opencv_ml243d.lib") 
#pragma comment(lib, "opencv_video243d.lib") 

#include"opencv/cv.h"
#include"opencv/highgui.h"

using namespace cv ;
using namespace std ;

static void check_eorror( XnStatus result,string status )
{
	if ( result != XN_STATUS_OK )
	{
		cerr << status <<"error:"<<xnGetStatusString(result)<<endl ;
	}
}


int main(int argc, char* argv[])
{
	//for opencv Mat
	//
	Mat
		m_srcdepth16u( XN_VGA_Y_RES,XN_VGA_X_RES,CV_16UC1);

	Mat
		m_depth16u( XN_VGA_Y_RES,XN_VGA_X_RES,CV_16UC1);

	Mat
		m_middepth8u( XN_VGA_Y_RES,XN_VGA_X_RES,CV_8UC1);

	Mat
		m_depth8u( XN_VGA_Y_RES,XN_VGA_X_RES,CV_8UC1);

	Mat
		m_rgb8u( XN_VGA_Y_RES,XN_VGA_X_RES,CV_8UC3);

	Mat
		m_DepthShow( XN_VGA_Y_RES,XN_VGA_X_RES,CV_8UC1);

	Mat
		m_srcDepthShow( XN_VGA_Y_RES,XN_VGA_X_RES,CV_8UC1);

	// openni variable
	//
	XnStatus nRet = XN_STATUS_OK ;
	char key = 0 ;

	xn::Context context ;
	nRet = context.Init() ;
	check_eorror(nRet,"context.Init" ) ;

	XnMapOutputMode mapMode ;
	mapMode.nXRes = XN_VGA_X_RES ;
	mapMode.nYRes = XN_VGA_Y_RES ;
	mapMode.nFPS = 30 ;

	xn::DepthGenerator depthGen ;
	nRet = depthGen.Create( context ) ;
	check_eorror(nRet,"depthGen.Create" ) ;
	nRet = depthGen.SetMapOutputMode( mapMode ) ;
	check_eorror(nRet,"depthGen.SetMapOutputMode" ) ;

	nRet = context.StartGeneratingAll() ;
	check_eorror(nRet,"StartGeneratingAll" ) ;
	nRet = context.WaitAndUpdateAll() ;
	check_eorror(nRet,"WaitAndUpdateAll" ) ;

	xn::DepthMetaData depthMD ;

	const int sizebuffer = XN_VGA_X_RES*XN_VGA_Y_RES*2;

	while ( key != 27 && !( nRet = context.WaitAndUpdateAll()) ) {

		depthGen.GetMetaData( depthMD ) ;

		memcpy(m_srcdepth16u.data,depthMD.Data(),sizebuffer);

		// 首先处理深度为 0 的点，这种实际上无法测量的点，所以将深度为 0 的点设成最大深度
		//
		for( int i = 0; i < XN_VGA_Y_RES; i++) {

			for (int j = 0; j< XN_VGA_X_RES; j++) {

				unsigned short & temp = m_srcdepth16u.at<unsigned short>(i,j);

				if(temp == 0) {

					temp = 65535;
				}
			}
		}

		// 高斯滤波平滑
		Ptr<FilterEngine> f = createGaussianFilter(
			m_srcdepth16u.type(), 
			Size(9,9), 
			0.85, 
			0.85 
			);
		f->apply(m_srcdepth16u, m_depth16u);


		// 深度图像二值化，阈值大概是取手掌的厚度， minValue存储的事距离kinect最近的指尖深度

		double minValue, maxValue;

		minMaxIdx(m_depth16u, &minValue, &maxValue);


		for( int i = 0; i < XN_VGA_Y_RES; i++) {

			for (int j = 0; j<XN_VGA_X_RES; j++) {

				if(m_depth16u.at<unsigned short>(i,j) > minValue +50) {
					m_middepth8u.at<unsigned char>(i,j) =0;
				}
				else {
					m_middepth8u.at<unsigned char>(i,j) = 255;
				}
			}

		}

		m_middepth8u.copyTo(m_depth8u);

		// 取得手掌轮廓
		//
		vector<vector<Point>> contours;
		findContours(m_middepth8u, contours,CV_RETR_EXTERNAL,CV_CHAIN_APPROX_SIMPLE, Point(0,0));
		Scalar color( 255, 0, 0 );
		drawContours(m_depth8u, contours, -1, color);
		vector<Point> contourMerge;
		vector<vector<Point>>::iterator it;


		// 将轮廓数据合并到一个数组中 , 并获得包含此轮廓的最小矩形
		//
		for (it=contours.begin(); it!=contours.end(); it++) {

			vector<Point> &currentContour = *it;
			vector<Point>::iterator itContour;
			for (itContour=currentContour.begin();itContour!=currentContour.end(); itContour++)    {

				contourMerge.push_back(*itContour);

			}

		}

		RotatedRect minRect = minAreaRect(Mat(contourMerge));
		Point2f pt[4];
		minRect.points(pt);
		line(m_depth8u, pt[0], pt[1], color);
		line(m_depth8u, pt[1], pt[2], color);
		line(m_depth8u, pt[2], pt[3], color);
		line(m_depth8u, pt[3], pt[0], color);

		// 将上述最小矩形取出，并旋转到正方向，横平竖直
		Mat rotate8u = Mat(minRect.boundingRect().size(),CV_8UC1);
		Mat after_rotate8u =Mat(minRect.boundingRect().size(),CV_8UC1);
		getRectSubPix( m_depth8u, minRect.boundingRect().size(),minRect.center, rotate8u);


		Point2f rotateCenter = Point2f(
			minRect.boundingRect().size().width/2.0 ,
			minRect.boundingRect().size().height/2.0
			);

		Mat rotateM = getRotationMatrix2D(
			rotateCenter,
			90+minRect.angle, 1
			);

		warpAffine(
			rotate8u,
			after_rotate8u, 
			rotateM, 
			minRect.boundingRect().size()
			);


		Mat scale8u = Mat( Size(30, minRect.size.width),CV_8UC1);

		getRectSubPix(
			after_rotate8u, 
			Size(minRect.size.height,minRect.size.width), 
			rotateCenter, 
			scale8u
			);

		rotate8u.convertTo(m_DepthShow,CV_8U);

		//m_depth8u.convertTo(m_DepthShow,CV_8U);
		//
		cvNamedWindow("before rotate");
		imshow("before rotate",m_DepthShow);


		// scale8u.convertTo(m_DepthShow,CV_8U);
		//m_depth8u.convertTo(m_DepthShow,CV_8U);

		cvNamedWindow("test");


		// 距离变换，获取骨架
		Mat
			m_outdepth32u(scale8u.rows,scale8u.cols,CV_32FC1);
		distanceTransform(scale8u, m_outdepth32u, CV_DIST_L2,CV_DIST_MASK_5);


		// 显示距离变换得到结果
		int i = 0, j = 0;
		float maxDist = 0.0;

		for( i = 0; i < scale8u.rows; i++) {

			for (j = 0; j<scale8u.cols; j++) {

				if(maxDist < m_outdepth32u.at<float>(i,j))

					maxDist = m_outdepth32u.at<float>(i,j);

			}

		}

		m_outdepth32u.convertTo(m_DepthShow,CV_8U, 255/maxDist);

		imshow("test",m_DepthShow);

		key = cvWaitKey(330) ; 

	}// while

	context.StopGeneratingAll() ;
	context.Shutdown() ;

	return 0;

}// end of main

