#include "ros/ros.h"
#include <sensor_msgs/Joy.h>
#include <robotBrain/opencvMessage.h>
#include <SerialStream.h>
#include <math.h>

#define sonarThreshold 1300
#define verycloseThreshold 1050
#define loopRate 10
#define remoteRate 20
#define panValue 80

/**************************************************************************************
*Node Objectives
*	Receives Sonar Feedback from the Atxmega Board through Serial Communication
*	Pruocess the Information and does obstacle Avoidance
*	Controls Motor and Servo
*	If the Joy Node is transmitting takes control over the motor and servo
*	Sends feedback to Station(Computer on the same network running the Joy Node)
*
****************************************************************************************/

        
void obstacleAvoidance ( char read[] );
void remoteControlCommands();

void joyCallback(const sensor_msgs::Joy::ConstPtr& joy);
void opencvCallback (const robotBrain::opencvMessage::ConstPtr& opencvMessage);

ros::Subscriber opencvCommands;
ros::Subscriber joy_subscriber;


bool remoteControl = false;
bool opencv = false;

float subscriberForwardThrottle; 	
float subscriberReverseThrottle;

int32_t subscriberRightSteer;
int32_t subscriberLeftSteer;
int32_t subscriberButtonA;		//buttons messages are ints
int32_t subscriberButtonB;
int32_t subscriberButtonY;

uint16_t leftSonar;		//left sonar
uint16_t centerSonar;		//center
uint16_t rightSonar;		//right

uint8_t Straight;
uint8_t Left;
uint8_t Right;
uint8_t Reverse;
    
char motor = 'f';
char servo = 's';
char cameraSteering = 's';
char errorXmega = '0';		//0 means everything ok - 1 means error or closed application
char errorOpenCV = '0';

int main ( int argc, char **argv ) {

    //ROS INIT

    ros::init ( argc, argv, "atxmegaTalker" );

    ros::NodeHandle n;

    opencvCommands = n.subscribe<robotBrain::opencvMessage> ( "opencv_commands", 1000, opencvCallback );
    joy_subscriber = n.subscribe<sensor_msgs::Joy>("joy",1000, joyCallback);
  
    ros::Rate loop_rate ( loopRate );
    ros::Rate remoteControlLoopRate ( remoteRate );
    
    //SERIAL INIT

    LibSerial::SerialStream mySerial;

    mySerial.Open ( "/dev/ttyUSB0" );

    mySerial.SetBaudRate ( LibSerial::SerialStreamBuf::BAUD_57600 );

    mySerial.SetCharSize ( LibSerial::SerialStreamBuf::CHAR_SIZE_8 );

    mySerial.SetNumOfStopBits ( 1 );

    mySerial.SetParity ( LibSerial::SerialStreamBuf::PARITY_NONE );

    mySerial.SetFlowControl ( LibSerial::SerialStreamBuf::FLOW_CONTROL_NONE );

    if ( !mySerial.good() ) {
        ROS_FATAL ( "COULD NOT SET UP SERIAL COMMUNICATION CLOSING NODE" );
        exit ( 1 );
        }
	  
    char read[13];
    char motorTemp = 'f';
    uint8_t changeInDirectionCounter = 10;
    bool motorPause = false;
    char cameraTemp = 'p';
    uint8_t pcounter = panValue;
    
    
    while ( ( mySerial.good() ) & ( n.ok() ) ) {
      
      //break if there is a change in direction
	if( (motor == 'f' & motorTemp == 'r') | (motor == 'r' & motorTemp =='f') | (motorPause) ) {
	  motorPause = true;
	  motor = servo = 's';
	  changeInDirectionCounter--;
	  if(!changeInDirectionCounter) {motorPause = false; changeInDirectionCounter = 10;}	  
	}
	
	if( !(errorOpenCV == '3') ) {	//if camera has not found target pan left to right
	    cameraSteering = cameraTemp;
	    pcounter--;
	    if (pcounter == panValue/2) cameraTemp = 'm';
	    else if(!pcounter) {pcounter = panValue; cameraTemp = 'p';}
	}
      
	ROS_INFO("[%c %c %c] Commands to Atxmega ", motor, servo, cameraSteering);
	  
	mySerial.write(&motor, 1);
	mySerial.write(&servo, 1);
	mySerial.write(&cameraSteering, 1);
	mySerial.write(&errorXmega, 1);

	if ( mySerial.rdbuf()->in_avail() ) mySerial.read ( read, sizeof ( read ) );

	//ROS_INFO("%s Sonars", read);
	
	if ( !(motorPause) ) motorTemp = motor;	//changing motorTemp only if motor is not paused
	
	ros::spinOnce();  //spinning here to get values from remote and/or opencv

	while ( remoteControl ) {

		remoteControlCommands();
			
		ROS_INFO("[%c %c] Teleop Commands", motor, servo);
		
		mySerial.write(&motor, 1);
		mySerial.write(&servo, 1);
		mySerial.write(&cameraSteering, 1);
		mySerial.write(&errorXmega, 1);
		ros::spinOnce();
		
		remoteControlLoopRate.sleep();
		//reason why it might be breaking not expecting the xmega error command on xmega board
	}

	if( (!(motor == 's') & (opencv)) | (!opencv) ) obstacleAvoidance ( read ); //do not avoid obstacles if motors are stop by opencv
	loop_rate.sleep();
    }

	motor = servo = cameraSteering = 's';
	errorXmega = '1';
	ROS_FATAL ( "ROS/COMMUNICATION FAILURE CLOSING NODE" );
	mySerial.write(&motor, 1);
	mySerial.write(&servo, 1);
	mySerial.write(&cameraSteering, 1);
	mySerial.write(&errorXmega, 1);

    return -1;
}
    

void remoteControlCommands() {

	  if(subscriberRightSteer) servo = 'r'; 			
	  else if(subscriberLeftSteer)servo = 'l';			
	  else servo = 's'; 			

	  if(subscriberForwardThrottle< 0) motor = 'f'; 		//we move faster when doing remote control
	  else if(subscriberReverseThrottle < 0) motor = 'r';
	  else  motor = 's'; 

	  //cameraSteering = 's';
	
}

void obstacleAvoidance ( char read[] ) {
  
  leftSonar = 0;		//left sonar
  centerSonar = 0;		//center
  rightSonar = 0;	
  
  //converting char strings to ints
  for ( int i = 0; i<4; i++ ) {
        switch ( i ) {
            case 0:
                leftSonar += ( read[0] - '0' ) *1000;
                centerSonar += ( read[4] - '0' ) *1000;
                rightSonar += ( read[8] - '0' ) *1000;

                break;
            case 1:
                leftSonar += ( read[1] - '0' ) *100;
                centerSonar += ( read[5] - '0' ) *100;
                rightSonar += ( read[9] - '0' ) *100;

                break;
            case 2:
                leftSonar += ( read[2] - '0' ) *10;
                centerSonar += ( read[6] - '0' ) *10;
                rightSonar += ( read[10] - '0' ) *10;

                break;
            case 3:
                leftSonar += ( read[3] - '0' );
                centerSonar += ( read[7] - '0' );
                rightSonar += ( read[11] - '0' );

                break;
            }
        }
        ROS_INFO("[%d %d %d] Left Center Right Sonar Values", leftSonar, centerSonar, rightSonar);
        
    if ( ( leftSonar < verycloseThreshold ) | ( centerSonar < verycloseThreshold ) | ( rightSonar < verycloseThreshold ) ) {
	motor = 'r';
	//servo = 's';
    }
    else if ( ( leftSonar < sonarThreshold ) | ( centerSonar < sonarThreshold ) | ( rightSonar < sonarThreshold ) ) {
	if (rightSonar < sonarThreshold) rightSonar = 1;
	else rightSonar = 0;
	
	if (leftSonar < sonarThreshold) leftSonar= 1;
	else leftSonar= 0;
	
	if (centerSonar < sonarThreshold) centerSonar = 1;
	else centerSonar = 0;
	
	Straight =  ( ((1-leftSonar) & (1-centerSonar) & (1-rightSonar)) | ((leftSonar) & (1-centerSonar) & (rightSonar)) );
	if(Straight){
	  servo = 's';
	  motor = 'f';
	  return;
	}
	
	Left = ( (rightSonar & (1-leftSonar)) | ((1-leftSonar) & centerSonar) );
	if(Left){
	  servo = 'l';
	  motor = 'f';
	  return;
	}
	
	Right = ( (1-rightSonar) & leftSonar );
	if(Right){
	  servo = 'r';
	  motor = 'f';
	  return;
	}
	
	Reverse = ( (leftSonar & centerSonar & rightSonar) );//| ((1-leftSonar) & (centerSonar) & (1-rightSonar)) );
	if(Reverse){
	  servo = 's';
	  motor = 'r';
	  return;
	}
    }
    else{ //if there is no obstacle in front we turn towards object if theres any detected
	//if( (cameraSteering == 'p')  ) servo = 'r'; 	 this might mess with camera panning
	//else if( (cameraSteering == 'm') ) servo = 'l';
	/*else*/ servo = 's';
	
	/*if(!opencv)*/ motor = 'f'; //if receiving commands from camera dont change them
    }	//we do not enter if !opencv anyways
}


void opencvCallback (const robotBrain::opencvMessage::ConstPtr& opencvMessage){
    //ROS_INFO("TARGET ACQUIRED");
    cameraSteering = 	(char) (opencvMessage->camera);		
    motor = 		(char) (opencvMessage->motor);
    errorOpenCV = 	(char) (opencvMessage -> errorOpenCV);
    if( !remoteControl ) opencv = true;
    errorXmega = errorOpenCV;
}

void joyCallback(const sensor_msgs::Joy::ConstPtr& joy){
	subscriberReverseThrottle 	=  joy->axes[2] ;		
	subscriberForwardThrottle	= joy->axes[5];
	subscriberRightSteer 		= joy->buttons[11] ;		
	subscriberLeftSteer 		= joy->buttons[12] ;
	subscriberButtonA 		= joy->buttons[0];
	subscriberButtonB 		= joy->buttons[1];
	subscriberButtonY		= joy->buttons[3];
    if ( subscriberButtonA ) {remoteControl = true; opencv = false; errorXmega = '2';}
    if ( subscriberButtonB ) {remoteControl = false; errorXmega = '0'; }
 }
