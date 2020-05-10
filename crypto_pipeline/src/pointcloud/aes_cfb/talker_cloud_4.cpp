//ROS libraries
#include "ros/ros.h"
#include "std_msgs/String.h"

//point cloud
#include <sensor_msgs/PointCloud2.h>

//general
#include <iostream>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <chrono>
#include <string.h>
#include <stdio.h>
#include <thread>

//crypto
#include "aes_cfb.h"


#define BLOCKSIZE 16

// measure RTT
std::chrono::time_point<std::chrono::system_clock> start, end;

// Create a container for the data received from rosbag and listener
sensor_msgs::PointCloud2 talker_msg; 
sensor_msgs::PointCloud2 talker_msg_from_list;


// callback for rosbag
void lidarCallback(const sensor_msgs::PointCloud2ConstPtr& msg){

  talker_msg = *msg;

  return;
}

// callback for listener node
void lidarCallback2(const sensor_msgs::PointCloud2ConstPtr& msg){

  talker_msg_from_list = *msg;

  return;
}



int main(int argc, char **argv)
{
  
  ros::init(argc, argv, "talker");

  ros::NodeHandle n;


  // point cloud publisher - from talker
  ros::Publisher lidar_pub = n.advertise<sensor_msgs::PointCloud2>("/encrypted_points_from_talker", 1000);

  ros::Publisher lidar_recovered = n.advertise<sensor_msgs::PointCloud2>("/recovered_points_talker", 1000);

  // point cloud subscriber - from rosbag
  ros::Subscriber sub = n.subscribe<sensor_msgs::PointCloud2> ("/os1_cloud_node/points", 1000, lidarCallback); 

  // point cloud subscriber - from listener
  ros::Subscriber sub_list = n.subscribe<sensor_msgs::PointCloud2> ("/encrypted_points_from_listener", 1000, lidarCallback2); 

  // start time
  start = std::chrono::system_clock::now();

  while (ros::ok())
  {
    // ** PART 1: listen for ROS messages from rosbag, then encrypt and send to talker node

    // define data size
    int size_cloud = talker_msg.row_step * talker_msg.height;
   
    // ** ENCRYPTION **
    sensor_msgs::PointCloud2 talker_msg_copy;
    talker_msg_copy = talker_msg;

    u8 key[BLOCKSIZE] = {0};
	  u32 iv[BLOCKSIZE/4] = {0};
    cipher_state e_cs;
	  cfb_initialize_cipher(&e_cs, key, iv);
    cfb_process_packet(&e_cs, &talker_msg.data[0], &talker_msg_copy.data[0], size_cloud, ENCRYPT);

    // publish encrypted point cloud
    lidar_pub.publish(talker_msg_copy);
      

    // ** PART3: listen for received ROS messages from listener node, then decrypt and publish recovered point cloud **

    // define data size
    int size_cloud2 = talker_msg_from_list.row_step * talker_msg_from_list.height;
      
    // RECOVER 
    sensor_msgs::PointCloud2 talker_msg_from_list_copy;
    talker_msg_from_list_copy = talker_msg_from_list;

    cipher_state d_cs;
	  cfb_initialize_cipher(&d_cs, key, iv);
	  cfb_process_packet(&d_cs, &talker_msg_from_list.data[0], &talker_msg_from_list_copy.data[0], size_cloud2, DECRYPT);

    // publish recovered data
    lidar_recovered.publish(talker_msg_from_list_copy);

    // measure elapsed time (RTT when rosbag, listener and talker node is running at the same time)
    end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - start;
    std::cout << "RTT: " << elapsed_seconds.count() << std::endl;

    ros::spinOnce();
  }
  


  return 0;
}