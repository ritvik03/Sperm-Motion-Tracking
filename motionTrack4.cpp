#include "opencv2/imgcodecs/imgcodecs.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/videoio/videoio.hpp"
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/video/video.hpp>
#include <opencv2/tracking/tracking.hpp>
#include <opencv2/core/ocl.hpp>
//C
#include <stdio.h>
//C++
#include <iostream>
#include <sstream>
using namespace cv;
using namespace std;

#define SSTR( x ) static_cast< std::ostringstream & >( \
( std::ostringstream() << std::dec << x ) ).str()

// Global variables
Mat frame; //current frame
Mat fgMaskMOG2; //fg mask fg mask generated by MOG2 method
Mat pathdrawing,colorFgMask;
vector<vector<Point2f> >allMoments;
vector<Point2f>travelled;
vector<int>fastSperm,slowSperm;
int keyboard;
Ptr<cv::BackgroundSubtractorMOG2> pMOG2; //MOG2 Background subtractor
void processVideo(char* videoFilename);
Mat furtherProcess(Mat image,vector<Point2f>moments);
Mat pathDrawn(vector<vector<Point2f> >allMoments);
void trackAll(char* v);
void trackSpecific(char* videoFilename);
Point calculateBlobCentroid(vector<Point>blob);
void trackIndi(int i,Rect2d bbox, char* videoFilename);
float dist(Point2f p,Point2f q);
//Mat tracking(Mat colorFgMask,int frameNo);

int main(int argc, char* argv[])
{
    //create GUI windows
    //namedWindow("Frame");
    //namedWindow("FG Mask MOG 2");
    //create Background Subtractor objects
    pMOG2 = createBackgroundSubtractorMOG2(); //MOG2 approach
    //cv::Ptr<cv::BackgroundSubtractorMOG2> pMOG2 = cv::createBackgroundSubtractorMOG2();
    //pMOG2->setHistory(atoi(argv[2]))	;
    //processVideo(argv[1]);
    namedWindow("fg",WINDOW_NORMAL);
    trackAll(argv[1]);
    while(1){
         char key = waitKey(10);
         if(key=='e'){
           break;
         }
         trackSpecific(argv[1]);
       }
    return 0;
}

void trackSpecific(char* videoFilename){
  VideoCapture video(videoFilename);
  Ptr<Tracker> tracker;
  tracker = TrackerBoosting::create();

  Mat frame;
  video.read(frame);
  video.read(frame);
  bool ok = video.read(frame);
  // Define initial boundibg box
  Rect2d bbox(287, 23, 86, 320);

  // Uncomment the line below to select a different bounding box
  bbox = selectROI(frame, false);

  // Display bounding box.
  rectangle(frame, bbox, Scalar( 255, 0, 0 ), 2, 1 );
  imshow("Tracking", frame);

  tracker->init(frame, bbox);

  travelled.clear();

  while(video.read(frame))
  {
      bool ok = tracker->update(frame, bbox);

      if (ok)
      {
          rectangle(frame, bbox, Scalar( 255, 0, 0 ), 2, 1 );
          Point2f center_of_rect = (bbox.br() + bbox.tl())*0.5;
          travelled.push_back(center_of_rect);
          circle(frame,center_of_rect,3,Scalar(0,200,200),-1,8,0);
          for(int i=1;i<travelled.size();i++){
            line(frame,travelled[travelled.size()-i],travelled[travelled.size()-(i+1)],Scalar(0,255,0),1,8,0);
          }
      }
      else
      {
          putText(frame, "Tracking failure detected", Point(100,80), FONT_HERSHEY_SIMPLEX, 0.75, Scalar(0,0,255),2);
      }

      imshow("Tracking", frame);

      // Exit if ESC pressed.
      int k = waitKey(1);
      if(k == 27)
      {
          break;
      }

  }
}

void trackAll(char* videoFilename){
  VideoCapture cap(videoFilename);
  cap.read(frame);
  pMOG2->apply(frame, fgMaskMOG2);
  cap.read(frame);
  pMOG2->apply(frame, fgMaskMOG2);
  cap.set(CAP_PROP_FPS,50);
  Ptr<Tracker> tracker;
  tracker = TrackerBoosting::create();
  vector<Rect2d>bboxs;//(287, 23, 86, 320);
  //pMOG2->apply(frame, fgMaskMOG2);
  cvtColor(fgMaskMOG2,colorFgMask,COLOR_GRAY2BGR);
  vector<vector<Point> > contours;
  vector<Vec4i> hierarchy;
  vector<Point2f>moments;
  findContours( fgMaskMOG2, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0) );
  for( int i = 0; i< contours.size(); i++ )
     {
      // Scalar color = Scalar( rng.uniform(0, 255), rng.uniform(0,255), rng.uniform(0,255) );
      if(contourArea(contours[i])>50){
       //drawContours( drawing, contours, i, Scalar(255,255,0), -1, 8, hierarchy, 0, Point() );
       moments.push_back(calculateBlobCentroid(contours[i]));
       circle(frame,moments[moments.size()-1],3,Scalar(0,255,255),-1,8,0);
       //circle(drawing2,moments[moments.size()-1],3,Scalar(0,255,255),-1,8,0);
     }
     }
  //cout<<"Count = "<<moments.size()<<endl;
  imshow("fg",frame);
  //waitKey(0);

  //Mat allMovingSperms[moments.size()];
  for(int i=2;i<moments.size();i++){
    float length = 50;
    Rect2d bbox(moments[i].x-length/2, moments[i].y-length/2, length, length);
    bboxs.push_back(bbox);
    //allMovingSperms[i] = frame.clone();
    //rectangle(allMovingSperms[i], bbox, Scalar( 255, 0, 0 ), 2, 1 );
    //imshow("allMoving",allMovingSperms[i]);
    //waitKey(100);
    //trackIndi(i,bbox,videoFilename);
    trackIndi(i,bbox,videoFilename);
    cout<<"Completion:\t"<<(float)i*1.0/moments.size()<<endl;
  }
  //imshow("fg",frame);
  //waitKey(0);

  /*while(cap.read(frame)){
    pMOG2->apply(frame, fgMaskMOG2);
    cvtColor(fgMaskMOG2,colorFgMask,COLOR_GRAY2BGR);
    imshow("fg",colorFgMask);
    waitKey(10);
  }*/
  cout<<"Count = "<<moments.size()<<endl;
  cout<<"Fast Moving = "<<moments.size()-slowSperm.size()<<endl;
  cout<<"Slow Moving = "<<slowSperm.size()<<endl;
}

float dist(Point2f p,Point2f q){
  float d;
  d = sqrt((p.x-q.x)*(p.x-q.x)+(p.y-q.y)*(p.y-q.y));
  return d;
}

bool checkRange(vector<Point2f> travelled){
  float thresholdDist =5;
  float tempMaxDist =0;
  for(int i=0;i<travelled.size();i++){
    for(int j=i;j<travelled.size();j++){
      if(dist(travelled[i],travelled[j])>tempMaxDist){
        tempMaxDist=dist(travelled[i],travelled[j]);
      }
    }
  }
  cout<<"tempMaxDist: "<<tempMaxDist<<endl;
  if(tempMaxDist>thresholdDist){
    return true;
  }
  else{
    return false;
  }
}

void trackIndi(int i,Rect2d bbox, char* videoFilename){
  Ptr<Tracker> tracker;
  tracker = TrackerBoosting::create();
  stringstream ss;
  ss<<i;
  ss<<".avi";
  VideoCapture cap(videoFilename);
  //cap.set(CAP_PROP_FPS,100);
  cap.read(frame);
  //pMOG2->apply(frame, fgMaskMOG2);
  //cvtColor(fgMaskMOG2,colorFgMask,COLOR_GRAY2BGR);
  rectangle(frame, bbox, Scalar( 255, 0, 0 ), 2, 1 );
  bool is_inside=false;
  if((bbox.x>0) &&  (bbox.y>0) && (bbox.x + 100 < frame.size().width) && (bbox.x + 100 < frame.size().height)){
    //cout<<"bbox X:"<<bbox.x<<"\tbobx Y:"<<bbox.y<<endl;
    is_inside=true;
  }
  imshow("fg",frame);
  //waitKey(0);
  if(is_inside){
  tracker->init(frame , bbox);
  //int frame_width = cap.get(CV_CAP_PROP_FRAME_WIDTH);
  //int frame_height = cap.get(CV_CAP_PROP_FRAME_HEIGHT);
  //VideoWriter video(ss.str(),CV_FOURCC('M','J','P','G'),10, Size(frame_width,frame_height));
  while((cap.read(frame))&&(cap.get(CV_CAP_PROP_POS_FRAMES)*1.0/cap.get(CV_CAP_PROP_FPS)<1)){
    //pMOG2->apply(frame, fgMaskMOG2);
  bool ok = tracker->update(frame, bbox);
  if (ok)
  {
      // Tracking success : Draw the tracked object
      rectangle(frame, bbox, Scalar( 255, 0, 0 ), 2, 1 );
      Point2f center_of_rect = (bbox.br() + bbox.tl())*0.5;
      travelled.push_back(center_of_rect);
      if(travelled.size()>1){
        for(int i=1;i<travelled.size();i++){
          line(frame,travelled[travelled.size()-i],travelled[travelled.size()-(i+1)],Scalar(0,255,0),1,8,0);
        }
      }
  }
  else
  {
      // Tracking failure detected.
      putText(frame, "Tracking failure detected", Point(100,80), FONT_HERSHEY_SIMPLEX, 0.75, Scalar(0,0,255),2);
  }
  //video.write(frame);
  imshow("fg",frame);
  char k=waitKey(10);
  if(k=='n'){
    break;
  }
  }
  }
  else{
    cout<<"Not inside"<<endl;
  }
  bool isFast = checkRange(travelled);
  if(isFast){
    fastSperm.push_back(i);
  }
  else{
    slowSperm.push_back(i);
  }
  travelled.clear();
}


void processVideo(char* videoFilename) {
  Mat summation;
  vector<Point2f>moments;
    //create the capture object
    VideoCapture capture(videoFilename);
    capture.set(CAP_PROP_FPS,20);
    capture.read(frame);
    //capture.read(frame);
    //capture.read(frame);

    //////////////////// TRACKER INITIALISATION ///////////////////////////
    Ptr<Tracker> tracker;
    tracker = TrackerBoosting::create();
    Rect2d bbox(287, 23, 86, 320);
    bbox = selectROI(frame, false);
    rectangle(frame, bbox, Scalar( 255, 0, 0 ), 2, 1 );
    imshow("Tracking", frame);
    tracker->init(frame, bbox);
    //////////////////// TRACKER ///////////////////////////

  /*  VideoWriter videowriter("outputVideo.AVI", capture.get(CV_CAP_PROP_FOURCC), capture.get(CV_CAP_PROP_FPS), (capture.get(CV_CAP_PROP_FRAME_WIDTH),capture.get(CV_CAP_PROP_FRAME_HEIGHT)),true);
    int frame_width = capture.get(CV_CAP_PROP_FRAME_WIDTH);
    int frame_height = capture.get(CV_CAP_PROP_FRAME_HEIGHT);
    //VideoWriter video("out.avi",CV_FOURCC('M','J','P','G'),10, Size(frame_width,frame_height));
  */
    pathdrawing= Mat::zeros( frame.size(), CV_8UC3 );
    if(!capture.isOpened()){
        cerr << "Unable to open video file: " << videoFilename << endl;
        exit(EXIT_FAILURE);
    }
    while( (char)keyboard != 'q' && (char)keyboard != 27 ){
        if(!capture.read(frame)) {
            break;
        }
        else{

        GaussianBlur( frame, frame, Size( 5, 5 ), 0, 0 );
        //update the background model
        pMOG2->apply(frame, fgMaskMOG2);
        //allFrames.push_back(fgMaskMOG2);
        cvtColor(fgMaskMOG2,colorFgMask,COLOR_GRAY2BGR);
        //video.write(frame);
        Mat drawing = furtherProcess(fgMaskMOG2,moments);
        //cout<<"CAmera Pos"<<capture.get(CV_CAP_PROP_POS_FRAMES)<<endl;
        //waitKey(0);
        //Mat tracker = tracking(colorFgMask,capture.get(CV_CAP_PROP_POS_FRAMES));
        stringstream ss;
        rectangle(frame, cv::Point(10, 2), cv::Point(100,20),
                  cv::Scalar(255,255,255), -1);
        ss << capture.get(CAP_PROP_POS_FRAMES);
        string frameNumberString = ss.str();
        putText(frame, frameNumberString.c_str(), cv::Point(15, 15),
                FONT_HERSHEY_SIMPLEX, 0.5 , cv::Scalar(0,0,0));
        ////////////////////////////////////
        // Update the tracking result
        bool ok = tracker->update(colorFgMask, bbox);

        // Calculate Frames per second (FPS)
        //float fps = getTickFrequency() / ((double)getTickCount() - timer);

        if (ok)
        {
            // Tracking success : Draw the tracked object
            rectangle(frame, bbox, Scalar( 255, 0, 0 ), 2, 1 );
            Point2f center_of_rect = (bbox.br() + bbox.tl())*0.5;
            travelled.push_back(center_of_rect);
            if(travelled.size()>1){
              for(int i=1;i<travelled.size();i++){
                line(frame,travelled[travelled.size()-i],travelled[travelled.size()-(i+1)],Scalar(0,255,0),1,8,0);
              }
            }
        }
        else
        {
            // Tracking failure detected.
            putText(frame, "Tracking failure detected", Point(100,80), FONT_HERSHEY_SIMPLEX, 0.75, Scalar(0,0,255),2);
        }
        imshow("Tracking", frame);
        ////////////////////////////////////

        //show the current frame and the fg masks
        //imshow("Frame", frame);
        //imshow("FG Mask MOG 2",fgMaskMOG2);
        //imshow("drawing",drawing);
        //imshow("tracker",tracker);
        waitKey(10);
      }
    }
    //video.release();
    capture.release();
}

Point calculateBlobCentroid(vector<Point>blob){
    cv::Moments mu = cv::moments(blob);
    cv::Point centroid = cv::Point (mu.m10/mu.m00 , mu.m01/mu.m00);

    return centroid;
}

Mat furtherProcess(Mat image,vector<Point2f>moments){
  Mat blurred;
  GaussianBlur( image, blurred, Size( 5, 5 ), 0, 0 );
  vector<vector<Point> > contours;
  vector<Vec4i> hierarchy;
  findContours( blurred, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0) );

  /// Draw contours
  Mat drawing = Mat::zeros( blurred.size(), CV_8UC3 );
  Mat drawing2 = Mat::zeros( blurred.size(), CV_8UC3 );
  for( int i = 0; i< contours.size(); i++ )
     {
      // Scalar color = Scalar( rng.uniform(0, 255), rng.uniform(0,255), rng.uniform(0,255) );
      if(contourArea(contours[i])>50){
       //drawContours( drawing, contours, i, Scalar(255,255,0), -1, 8, hierarchy, 0, Point() );
       moments.push_back(calculateBlobCentroid(contours[i]));
       circle(frame,moments[moments.size()-1],3,Scalar(0,255,255),-1,8,0);
       circle(drawing2,moments[moments.size()-1],3,Scalar(0,255,255),-1,8,0);
     }
     }

  if(allMoments.size()>10){
    drawing = pathDrawn(allMoments);
  }
  //imshow("drawing",drawing);
  allMoments.push_back(moments);
  moments.clear();

  waitKey(3);
  return drawing;
}

Point2f findClosestPrevMoment(Point2f currMomentPt,vector<Point2f>prevMoments){
  float minimumDistance = 9999;
  Point2f closestPt=currMomentPt;
  for(int i=0;i<prevMoments.size();i++){
    Point2f iterPoint=prevMoments[i];
    if((dist(currMomentPt,iterPoint)<minimumDistance)){
      closestPt = iterPoint;
      minimumDistance = dist(currMomentPt,iterPoint);
    }
  }
  //cout<<"closest pt: "<<closestPt<<endl;
  return closestPt;
}

Mat pathDrawn(vector<vector<Point2f> >allMoments){
  //pathdrawing
  vector<Point2f> m1,m2,m3,m4,m5,m6,m7,m8,m9,m10;
 /*  pathdrawing= Mat::zeros( frame.size(), CV_8UC3 );           */
  //cout<<"Yaha tk"<<endl;
  m1 = allMoments[allMoments.size()-1];
  m2 = allMoments[allMoments.size()-2];
 /*  m3 = allMoments[allMoments.size()-3];
  m4 = allMoments[allMoments.size()-4];
  m5 = allMoments[allMoments.size()-5];
  m6 = allMoments[allMoments.size()-6];
  m7 = allMoments[allMoments.size()-7];
  m8 = allMoments[allMoments.size()-8];/*
  m9 = allMoments[allMoments.size()-9];
  m10 = allMoments[allMoments.size()-10];
 */

  for(int i=0;i<m1.size();i++){
    Point2f m1Pt = m1[i];
    Point2f m2Pt =findClosestPrevMoment(m1Pt,m2);
 /*    Point2f m3Pt =findClosestPrevMoment(m2Pt,m3);
    Point2f m4Pt =findClosestPrevMoment(m3Pt,m4);
    Point2f m5Pt =findClosestPrevMoment(m4Pt,m5);
    Point2f m6Pt =findClosestPrevMoment(m5Pt,m6);
    Point2f m7Pt =findClosestPrevMoment(m6Pt,m7);
    Point2f m8Pt =findClosestPrevMoment(m7Pt,m8);
    Point2f m9Pt =findClosestPrevMoment(m8Pt,m9);
    Point2f m10Pt =findClosestPrevMoment(m9Pt,m10); */

    circle(pathdrawing,m1Pt,3,Scalar(0,255,255),-1,8,0);
    //circle(pathdrawing,m2Pt,3,Scalar(0,255,255),-1,8,0);
    //circle(pathdrawing,m3Pt,3,Scalar(0,255,255),-1,8,0);
    //circle(pathdrawing,m4Pt,3,Scalar(0,255,255),-1,8,0);
    //circle(pathdrawing,m5Pt,3,Scalar(0,255,255),-1,8,0);
    //line(pathdrawing,m1Pt,prevMomentPt,Scalar(0,255,0),1,8,0);
    line(pathdrawing,m1Pt,m2Pt,Scalar(0,255,0),1,8,0);
 /*    line(pathdrawing,m2Pt,m3Pt,Scalar(0,255,0),1,8,0);
    line(pathdrawing,m3Pt,m4Pt,Scalar(0,255,0),1,8,0);
    line(pathdrawing,m4Pt,m5Pt,Scalar(0,255,0),1,8,0);
    line(pathdrawing,m5Pt,m6Pt,Scalar(0,255,0),1,8,0);
    line(pathdrawing,m6Pt,m7Pt,Scalar(0,255,0),1,8,0);
    line(pathdrawing,m7Pt,m8Pt,Scalar(0,255,0),1,8,0);
    line(pathdrawing,m8Pt,m9Pt,Scalar(0,255,0),1,8,0);
    line(pathdrawing,m9Pt,m10Pt,Scalar(0,255,0),1,8,0); */
  }
  return pathdrawing;
}
