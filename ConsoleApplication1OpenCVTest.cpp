// ConsoleApplication1OpenCVTest.cpp : Defines the entry point for the console application.
// garym@thethirdfloorinc.com 8/27/18
//

#include "stdafx.h"
#include <iostream>

// include the librealsense C++ header file
#include <librealsense2/rs.hpp>
#include <librealsense2\rsutil.h>


// include OpenCV header file
#include <opencv2\opencv.hpp>
#include <opencv2\face.hpp>

#include "RealSenseTracker.h"


using namespace std;
using namespace cv;



int main()
{
	const int landmark_size = 68;

	vrpn_Connection_IP* m_Connection = new vrpn_Connection_IP();
	RealSenseTracker* our_tracker = new RealSenseTracker(m_Connection);

	rs2_extrinsics* conversion = new rs2_extrinsics();
	

	//first get the connected device
	rs2::context ctx;
	auto list = ctx.query_devices(); // Get a snapshot of currently connected devices
	if (list.size() == 0)
		throw std::runtime_error("No device detected. Is it plugged in?");
	rs2::device dev = list.front();



	//Contruct a pipeline which abstracts the device
	rs2::pipeline pipe;

	//Create a configuration for configuring the pipeline with a non default profile
	rs2::config cfg;

	//Add desired streams to configuration
	cfg.enable_stream(RS2_STREAM_COLOR, 640, 480, RS2_FORMAT_BGR8, 60);
	cfg.enable_stream(RS2_STREAM_DEPTH, 640, 480, RS2_FORMAT_Z16, 60);

	//Instruct pipeline to start streaming with the requested configuration
	auto pipeline_profile = pipe.start(cfg);

	//get the depth sensor and units
	rs2::depth_sensor DepthSensor = dev.first<rs2::depth_sensor>();
	float DepthUnits = DepthSensor.get_option(RS2_OPTION_DEPTH_UNITS);
	//auto scale = DepthSensor.get_depth_scale();

	// get stream profiles
	auto DepthStream = pipeline_profile.get_stream(RS2_STREAM_DEPTH).as<rs2::video_stream_profile>();
	auto ColourStream = pipeline_profile.get_stream(RS2_STREAM_COLOR).as<rs2::video_stream_profile>();
	
	//-------------------camera parameters

	//auto resolution = std::make_pair(DepthStream.width(), DepthStream.height());
	rs2_intrinsics DepthIntrinsics = DepthStream.get_intrinsics();
	auto principal_point = std::make_pair(DepthIntrinsics.ppx, DepthIntrinsics.ppy);
	auto focal = std::make_pair(DepthIntrinsics.fx, DepthIntrinsics.fy);
	rs2_distortion distorion_model = DepthIntrinsics.model;
	cout << "distortion model depth is " << distorion_model << endl;

	rs2_intrinsics ColourIntrinsics = ColourStream.get_intrinsics();
	auto principal_colour = std::make_pair(ColourIntrinsics.ppx, ColourIntrinsics.ppy);
	auto focal_colour = std::make_pair(ColourIntrinsics.fx, ColourIntrinsics.fy);
	rs2_distortion distorion_model_colour = ColourIntrinsics.model;
	cout << "distortion model colour is " << distorion_model_colour << endl;
	
	//co-effs
	double k1, k2, p1, p2, k3;
	k1 = ColourIntrinsics.coeffs[0];
	k2 = ColourIntrinsics.coeffs[1];
	p1 = ColourIntrinsics.coeffs[2];
	p2 = ColourIntrinsics.coeffs[3];
	k3 = ColourIntrinsics.coeffs[4];

	//Convert to OpenCV Camera internals
	cv::Mat camera_matrix = (cv::Mat_<double>(3, 3) << focal_colour.first, 0, principal_colour.first, 0, focal_colour.second, principal_colour.second, 0, 0, 1);
	cv::Mat dist_coeffs = (Mat1d(1, 5) << k1, k2, p1, p2, k3);
	
	//-------------------camera parameters


	// Camera warmup - dropping several first frames to let auto-exposure stabilize
	rs2::frameset frames;
	for (int i = 0; i < 30; i++)
	{
		//Wait for all configured streams to produce a frame
		frames = pipe.wait_for_frames();
	}

	// Load OCV Face Detector
	CascadeClassifier faceDetector("haarcascade_frontalface_alt2.xml");
	Ptr<cv::face::Facemark> facemark = cv::face::FacemarkLBF::create();

	//load landmark detector
	facemark->loadModel("lbfmodel.yaml");

	//cv matrices 
	Mat color;
	Mat grey;


	while (1)
	{
		//spin up the VRPN stuff
		our_tracker->mainloop();
		m_Connection->mainloop();


		float ResultVector[3]{ 0.0f, 0.0f, 0.0f };
		float ResultVector_Transformed[3]{ 0.0f, 0.0f, 0.0f };
		float InputPixelAsFloat[2]{ 0.0f, 0.0f };


		auto data = pipe.wait_for_frames();

		//we have to align the 2D imagers from the RGB and the Depth 
		rs2::align align(RS2_STREAM_COLOR);
		auto aligned_frames = align.process(data);

		rs2::video_frame color_data = aligned_frames.first(RS2_STREAM_COLOR);
		rs2::depth_frame depth_data = aligned_frames.get_depth_frame();


		// Creating OpenCV Matrix from a color image
		color = Mat(Size(640, 480), CV_8UC3, (void*)color_data.get_data(), Mat::AUTO_STEP);
		//depth_img = Mat(Size(640, 480), CV_8UC3, (void*)depth_data.get_data(), Mat::AUTO_STEP);


		//colour conversion - we can only do face detect on greyscale image
		cvtColor(color, grey, COLOR_BGR2GRAY);

		// Variable for landmarks. 
		// Landmarks for one face is a vector of points
		// There can be more than one face in the image. Hence, we 
		// use a vector of vector of points. 

		vector< vector<Point2f> > landmarks;
		vector<Point2f> landmark_stable;
		vector<Point3d> model_points;
		vector<Point2d> image_points;
		vector<Rect> faces;

		// Detect faces
		faceDetector.detectMultiScale(grey, faces);

		bool success = facemark->fit(color, faces, landmarks);

		if (success)
		{
			//we only care about the first face
			if (landmarks[0].size() == landmark_size){

				int thickness = 1;
				int lineType = 8;

				//throw away the more noisy landmarks
				landmark_stable.push_back(landmarks[0][19]); //L eyebrow C
				landmark_stable.push_back(landmarks[0][21]); //L eyebrow inner
				landmark_stable.push_back(landmarks[0][22]); //R eyebrow inner
				landmark_stable.push_back(landmarks[0][24]); //R eyebrow C
				landmark_stable.push_back(landmarks[0][27]); //nose bridge
				landmark_stable.push_back(landmarks[0][30]); //nose tip

				//annotate
				for (int i = 0; i < landmark_stable.size(); i++) {

					Point pt;
					pt.x = landmark_stable[i].x;
					pt.y = landmark_stable[i].y;
					circle(color,
						pt,
						2.0,
						Scalar(0, 0, 255),
						thickness,
						lineType);

					//calculate the 3D point
					//measure the depth of the pixel 

					InputPixelAsFloat[0] = pt.x;
					InputPixelAsFloat[1] = pt.y;

					float distance = depth_data.get_distance(InputPixelAsFloat[0], InputPixelAsFloat[1]);

					//reproject to 3D
					rs2_deproject_pixel_to_point(ResultVector, &DepthIntrinsics, InputPixelAsFloat, distance);

					//transform 3d depth point to the colour stream 3D coordinate space
					rs2_get_extrinsics(DepthStream, ColourStream, conversion, NULL);
					rs2_transform_point_to_point(ResultVector_Transformed, conversion, ResultVector);
					
					//push the converted point into OCV vectors 
					model_points.push_back(Point3d(ResultVector_Transformed[0], ResultVector_Transformed[1], ResultVector_Transformed[2]));
					image_points.push_back(Point2d(pt.x, pt.y));
				}

				//solve it - standard OpenCV SolvePNP
				//*********
				Mat rvec;
				Mat tvec;
				solvePnP(model_points, image_points, camera_matrix, dist_coeffs, rvec, tvec);
				//*********

				//projectPoints
				vector<Point3d> nose_end_point3D;
				vector<Point2d> nose_end_point2D;
				Point3d nose_end(model_points[4].x, model_points[4].y, model_points[4].z);
				nose_end_point3D.push_back(nose_end);

				//project a point at the bridge of the nose onto the image plane, draw a line between nose tip and nose bridge
				projectPoints(nose_end_point3D, rvec, tvec, camera_matrix, dist_coeffs, nose_end_point2D);
				cv::line(color, image_points[5], nose_end_point2D[0], cv::Scalar(255, 0, 0), 2);


				//update the VRPN tracker
				TrackingData new_tracked_data;
				new_tracked_data.tx = tvec.at<double>(0, 0);
				new_tracked_data.ty = tvec.at<double>(1, 0);
				new_tracked_data.tz = tvec.at<double>(2, 0);
				new_tracked_data.rx = rvec.at<double>(0, 0);
				new_tracked_data.ry = rvec.at<double>(1, 0);
				new_tracked_data.rz = rvec.at<double>(2, 0);
				new_tracked_data.rw = double(1.0);
				our_tracker->setTrackingData(new_tracked_data);
			}
		}


		// Display in a GUI
		cv::namedWindow("Display Image", WINDOW_AUTOSIZE);
		cv::imshow("Display Image", color);

		//namedWindow("Display Depth", WINDOW_AUTOSIZE);
		//imshow("Display Depth", depth_img);

		if (waitKey(1) == 27) break;

		// Calling Sleep to let the CPU breathe.
		SleepEx(1, FALSE);
	}
	delete conversion;
	delete m_Connection;
	delete our_tracker;
	
	return 0;
}

