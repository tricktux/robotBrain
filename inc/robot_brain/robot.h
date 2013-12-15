#ifndef ROBOT_HPP
#define ROBOT_HPP

#include <SerialStream.h>
#include "ros/ros.h"
#include <sensor_msgs/Joy.h>

#define SONAR_THRESHOLD 1300
#define CLOSE_SONAR_THRESHOLD 1050
#define MAIN_LOOP_RATE 10
#define RC_LOOP_RATE 20
#define PAN_VALUE 80
#define MOTOR_OFF_TIME 10
#define ERROR_FREE '0'
#define ERROR '1'
#define RC_COMMAND '2'
#define TARGET_FOUND '3'
#define SERIAL_RECEIVE_SIZE 13

class robot{
	public:
	//ros variables
	ros::NodeHandle nh_; 
	ros::Subscriber joy_subscriber_;

	//remote control variables
    float subscriber_forward_throttle_; 	
	float subscriber_reverse_throttle_;

	int32_t subscriber_right_steer_;
	int32_t subscriber_left_steer_;
	int32_t subscriber_buttonA_;
	int32_t subscriber_buttonB_;
	
	//program variables
    uint8_t remote_control_;
	uint8_t opencv_;

	uint16_t left_sonar_;	
	uint16_t center_sonar_;	
	uint16_t right_sonar_;	
	
	char motor_temp_;
    uint8_t motor_timeout_;
    bool motor_pause_;
    

	char motor_;
	char servo_;
	char camera_steering_;
	char feedback_xmega_;		
	char OpenCV_feedback_;
	
	char camera_temp_;
	uint8_t pan_counter_;
	char serial_read_[SERIAL_RECEIVE_SIZE];
    
	
	robot(){
		//opencvCommands = n.subscribe<robotBrain::opencvMessage> ( "opencv_commands", 1000, opencvCallback );
		joy_subscriber_ = nh_.subscribe<sensor_msgs::Joy>("joy",1000, &robot::joy_callback, this);
	}
	
	void joy_callback(const sensor_msgs::Joy::ConstPtr& joy);
	//void robot::opencvCallback (const robotBrain::opencvMessage::ConstPtr& opencvMessage);
	void do_obs_avoidance ( char serial_read_[] );
	void do_remote_control();
	void pan_camera_servo();
};

class serial{
	public:
	
	LibSerial::SerialStream mySerial;

	serial(std::string serial_name){
		
		mySerial.Open ( serial_name );

		mySerial.SetBaudRate ( LibSerial::SerialStreamBuf::BAUD_57600 );

		mySerial.SetCharSize ( LibSerial::SerialStreamBuf::CHAR_SIZE_8 );

		mySerial.SetNumOfStopBits ( 1 );

		mySerial.SetParity ( LibSerial::SerialStreamBuf::PARITY_NONE );

		mySerial.SetFlowControl ( LibSerial::SerialStreamBuf::FLOW_CONTROL_NONE );
		
		if ( !mySerial.good() ) {
		ROS_FATAL("--(!)Failed to Open Serial Port(!)--");
		exit(-1);
		}
	}
	
	void transmit_to_serial(char motor, char servo, char camera_steering, char feedback_xmega_);
	void serial_receive();
};

#endif