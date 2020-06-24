// ROS libraries
#include "ros/ros.h"
#include "std_msgs/String.h"

// point cloud
#include <sensor_msgs/PointCloud2.h>

// general
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


// measure delay
std::chrono::time_point<std::chrono::system_clock> start1, end1, start2, end2;

// log time delay
const char *path_log="time_delay_listener.txt";
std::ofstream log_time_delay(path_log);

// Create a container for the data received from talker
sensor_msgs::PointCloud2 listener_msg;


void lidarCallback(const sensor_msgs::PointCloud2ConstPtr& msg){

  listener_msg = *msg;

}


int main(int argc, char **argv)
{

  ros::init(argc, argv, "listener");

  ros::NodeHandle n;

  // point cloud publisher - from listener
  ros::Publisher lidar_pub = n.advertise<sensor_msgs::PointCloud2>("/recovered_points_listener", 1000);

  // point cloud publisher - from listener
  //ros::Publisher lidar_pub2 = n.advertise<sensor_msgs::PointCloud2>("/encrypted_points_from_listener", 1000);

  // point cloud subscriber - from talker
  ros::Subscriber encryptedPointCloud = n.subscribe("/encrypted_points_from_talker", 1000, lidarCallback);

  while (ros::ok()){

    // ** PART 2: listen for received ROS messages from talker node, then decrypt and encrypt before sending back to talker

    // start time - decryption
    start1 = std::chrono::system_clock::now();

    // RECOVER
    sensor_msgs::PointCloud2 listener_msg_copy;
    listener_msg_copy = listener_msg;

    // define data size
    int size_cloud = listener_msg.data.size();

    u8 key[BLOCKSIZE] = {0};
    u32 iv[BLOCKSIZE/4] = {0};
    cipher_state d_cs;
    cfb_initialize_cipher(&d_cs, key, iv);
    cfb_process_packet(&d_cs, &listener_msg.data[0], &listener_msg_copy.data[0], size_cloud, DECRYPT);

    // measure elapsed time - decryption
    end1 = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds1 = end1 - start1;
    if(size_cloud != 0){
      log_time_delay << elapsed_seconds1.count() << std::endl;
    }

    // publish recvered point cloud  
    lidar_pub.publish(listener_msg_copy);



    // ENCRYPT
    /*
    // start time - encryption operation
    start2 = std::chrono::system_clock::now();

    sensor_msgs::PointCloud2 listener_msg_copy2;
    listener_msg_copy2 = listener_msg_copy;

    cipher_state e_cs;
    cfb_initialize_cipher(&e_cs, key, iv);
    cfb_process_packet(&e_cs, &listener_msg_copy.data[0], &listener_msg_copy2.data[0], size_cloud, ENCRYPT);

    // measure elapsed time - encryption operation
    end2 = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds2 = end2 - start2;
    if(size_cloud != 0){
      log_time_delay << elapsed_seconds2.count() << std::endl;
    }

    // publish encrypted point cloud
    lidar_pub2.publish(listener_msg_copy2);
    */
    ros::spinOnce();
    
  }

  log_time_delay.close();

  return 0;
}
