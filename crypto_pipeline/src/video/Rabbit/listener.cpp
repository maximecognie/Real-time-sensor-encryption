// ROS libraries
#include "ros/ros.h"
#include "std_msgs/String.h"
#include <sensor_msgs/Image.h>


// general
#include <iostream>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <chrono>
#include <string.h>
#include <stdio.h>


//crypto
#include "rabbit.h"
#include "encoder.h"


// measure delay
std::chrono::time_point<std::chrono::system_clock> start1, end1, start2, end2;

// log time delay
const char *path_log="time_delay_listener.txt";
std::ofstream log_time_delay(path_log);


// Create a container for the data received from talker
sensor_msgs::Image listener_msg;


void cameraCallback(const sensor_msgs::ImageConstPtr& msg){
  
  listener_msg = *msg;

}



int main(int argc, char **argv)
{

  ros::init(argc, argv, "listener");

  ros::NodeHandle n;

  // subscribe for encrypted stream from talker
  ros::Subscriber encryptedImageSubscriber = n.subscribe("/encrypted_stream_from_talker", 1000, cameraCallback);

  // encrypted image publisher
  //ros::Publisher encryptedImagePublisher = n.advertise<sensor_msgs::Image>("/encrypted_stream_from_listener", 1000);

  // recovered image publisher
  ros::Publisher recoveredImagePublisher = n.advertise<sensor_msgs::Image>("/recovered_stream_listener", 1000);

  
  
  while (ros::ok()){

    // ** PART 2: listen for received ROS messages from talker node, then decrypt and encrypt before sending back to talker **

    
    // ** RECOVER **

    // start time - decryption
    start1 = std::chrono::system_clock::now();

    sensor_msgs::Image listener_msg_copy;
    listener_msg_copy = listener_msg;

    int size = listener_msg.data.size();

    // key schedule is only performed once for each secret key
    std::string keyString = "00000000000000000000000000000000";
    u8 key[keyString.size()/2];
    hex2stringString(key, keyString.data(), keyString.size());

    rabbit_state d_cs;
    rabbit_key_setup(&d_cs, (u32*)key);
   
    // define IV  
    std::string ivString = "597E26C175F573C3";
    u8 iv[ivString.size()/2];
    hex2stringString(iv, ivString.data(), ivString.size());

    // Load key and encrypt 
    rabbit_iv_setup(&d_cs, (u32*)iv);
    rabbit_process_packet(&d_cs, &listener_msg_copy.data[0], &listener_msg.data[0], size);

    // measure elapsed time - decryption
    end1 = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds1 = end1 - start1;
    if(size != 0){
      log_time_delay << elapsed_seconds1.count() << std::endl;
    }
 
    // publish recovered video stream
    recoveredImagePublisher.publish(listener_msg_copy);      


    // ** ENCRYPT ** 
    /*
    // start time - encryption
    start2 = std::chrono::system_clock::now();

    sensor_msgs::Image listener_msg_copy2;
    listener_msg_copy2 = listener_msg_copy;

    hex2stringString(key, keyString.data(), keyString.size());
    hex2stringString(iv, ivString.data(), ivString.size());

    // initialize cipher
    sosemanuk_state e_cs;

    // Load key and iv
    sosemanuk_load_key(&e_cs, key, keyString.size()/2);
    sosemanuk_load_iv(&e_cs, (u32*)iv);

    sosemanuk_process_packet(&e_cs, &listener_msg_copy2.data[0], &listener_msg_copy.data[0], size);

    //keyString += "1";

    // measure elapsed time - encryption operation
    end2 = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds2 = end2 - start2;
    if(size != 0){
      log_time_delay << elapsed_seconds2.count() << std::endl;
    }

    // publish encrypted video stream
    encryptedImagePublisher.publish(listener_msg_copy2);
    */
    ros::spinOnce();
    
  }

  log_time_delay.close();

  return 0;
}
