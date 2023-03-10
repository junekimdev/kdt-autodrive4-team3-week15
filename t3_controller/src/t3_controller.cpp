#include <string>

#include "ros/console.h"
#include "ros/ros.h"
#include <vector>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <cmath>

// Messages
#include "t3_controller/control_state.h"
#include "t3_controller/sensor_state.h"




namespace control{
  const std::string NAME = "controller";
  const std::string SUB_TOPIC_OBJECT = "object_data";
  //const std::string PUB_TOPIC = "xycar_motor";
  //constexpr int SPEED = 5;
  //constexpr float ANGLE_DIV = 1.f;

  control_state::State control_state_;
  sensor_state::State sensor_state_;

  struct TmpObject
  {
    int id;
    int xmin;
    int ymin;
    int xmax;
    int ymax;
    float probability;
    TmpObject(){};

 };


  class Controller
  {
    ros::NodeHandle node;
    ros::Subscriber sub_object;
    ros::Publisher pub;
    int count =0;
    const float FOV_H = 1.60469;
    const float FOV_V = 1.31969;
    const int PINGPONG_BALL = 4;
    const int CENTER_POINT_X = 320;
    const int CENTER_POINT_Y = 240;
    const float FOCAL_LENGTH = 348.14820298;
    const double PI = 3.1415926;


  public:
    bool enable_debug;

    Controller()
    {
      node.param<bool>("controller_enable_debug", enable_debug, false);
      this->sub_object = this->node.subscribe(SUB_TOPIC_OBJECT, 1, &Controller::callbackObject, this);
    }

    void callbackObject(const t3_msgs::object_data::ConstPtr& msg);
    //void callbackTrafficLight(const t3_msgs::traffic_light_data::ConstPtr& msg);
    void control();
    float calculate_azimuth(control::TmpObject bbox);
    std::pair<float,float> distance(float azimuth, control::TmpObject bbox);
  };

  float Controller::calculate_azimuth(control::TmpObject bbox){
  	float cx = (bbox.xmin+bbox.xmax)/2;
    float azimuth = (cx - 320)*tan(FOV_H/2)/320;
  	// float azimuth = (cx - 320)*FOV_H/640;
  	return azimuth;
  }
  
  std::pair<float,float> Controller::distance(float azimuth, control::TmpObject bbox){
    float depth = 0;
    float p;
    if (azimuth == 0){
      depth = PINGPONG_BALL * FOCAL_LENGTH /(bbox.xmax - bbox.xmin);
      p = 0.1 * (exp(depth-64) - exp(-depth+64))/(exp(depth-64) - exp(-depth+64)) + 0.82;
      return {0.0f, depth * p};
    }

    else{
      depth = PINGPONG_BALL * FOCAL_LENGTH /(bbox.xmax - bbox.xmin);
      p = 0.1 * (exp(depth-64) - exp(-depth+64))/(exp(depth-64) - exp(-depth+64)) + 0.82;
      // std::cout<< p<< std::endl;
      return {depth*azimuth, depth*p};
    }
  }
  
  void Controller::callbackObject(const t3_msgs::object_data::ConstPtr& msg)
  {
    sensor_state_.object.reduce(msg);
  }
  
  void Controller::control()
  {
    std::vector<control::TmpObject> tmpObject;


    if(sensor_state_.object.boundingBoxes.size()>0)
    {
      for(auto objectBox :sensor_state_.object.boundingBoxes){
        control::TmpObject tmp;
        tmp.id = objectBox.id;
        tmp.xmin = (int)objectBox.xmin*640/352;
        tmp.ymin = (int)objectBox.ymin*480/352;
        tmp.xmax = (int)objectBox.xmax*640/352;
        tmp.ymax = (int)objectBox.ymax*480/352;
        tmp.probability = objectBox.probability;
        float azimuth =  calculate_azimuth(tmp);
        auto depth =  distance(azimuth,tmp);
        std::cout << "[" <<depth.first << "," << depth.second << "] ";

        tmpObject.emplace_back(tmp);
      }
      std::cout << "" <<std::endl;
      
    }
  }
}
  // namespace control

int main(int argc, char** argv)
{
  ros::init(argc, argv, control::NAME);
  control::Controller controller;
  ROS_INFO("%s is ONLINE", control::NAME.c_str());

  while (ros::ok())
  {
    
    ros::spinOnce();
    controller.control();
  }

  return 0;
}
