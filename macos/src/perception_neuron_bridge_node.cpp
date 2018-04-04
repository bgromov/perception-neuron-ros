// PerceptionNeuronROSserial.cpp : Defines the entry point for the console application.
//

#define __OS_XUN__

#include <ros/ros.h>
#include <geometry_msgs/Pose.h>
#include <std_msgs/Float64MultiArray.h>
#include <std_msgs/MultiArrayLayout.h>
#include <std_msgs/MultiArrayDimension.h>
#include <std_msgs/Int32MultiArray.h>
#include <std_msgs/String.h>
#include <string>
#include <stdlib.h>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <vector>
#include <typeinfo>
#include <stdio.h>
#include <NeuronDataReader.h>

SOCKET_REF sockTCPREF = NULL;
FrameDataReceived _DataReceived;
CommandDataReceived _CmdDataReceived;
BvhDataHeader     _bvhHeader;
SocketStatusChanged    _SocketStatusChanged;


float * _valuesBuffer=NULL;
int _frameCount = 0;
int bufferLength = 0;
bool bCallbacks = false;

// Max Array Length for ROS Data = 255  should be for UINT8 (-> data_msg.data_length )
// But not working maybe they used somewhere signed int8
// -> therefore the max array length = 127.
// we split Axis Neuron Data in 3 Parts -> therefore
int MAX_DATA_LENGTH = 120;


void bvhDataReceived(void * customObject, SOCKET_REF sockRef, BvhDataHeader * header, float * data)
{
	BvhDataHeader * ptr = header;
	if (ptr->DataCount != bufferLength || _valuesBuffer == NULL) {
		_valuesBuffer = new float[ptr->DataCount];
		bufferLength = ptr->DataCount;
	}
	memcpy((char *)_valuesBuffer, (char*)data, (int)ptr->DataCount*sizeof(float));
	_frameCount++;
  //ROS_INFO("DataReceived");
}

void socketStatusChanged(void * customObject, SOCKET_REF sockRef, SocketStatus status, char * message) {
	ROS_INFO("Socket status changed");
}

void registerNeuronCallbacks() {
	bool bBVH = false;
	bool bCmd = false;
	bool bSSt = false;
	void * ptr = NULL;
	ROS_INFO("Register Neuron Callbacks");
	_DataReceived = FrameDataReceived(bvhDataReceived);
	if (_DataReceived) {
		BRRegisterFrameDataCallback(ptr, _DataReceived);
		bBVH = true;
	}
	_SocketStatusChanged = SocketStatusChanged(socketStatusChanged);
	if (_SocketStatusChanged) {
		BRRegisterSocketStatusCallback(ptr, _SocketStatusChanged);
		bSSt = true;
	}
	bCallbacks = bBVH&&bSSt;
}

void prepareDataMsg(std_msgs::Float64MultiArray & data_msg) {
	// data_msg.layout.dim = (std_msgs::MultiArrayDimension *) malloc(sizeof(std_msgs::MultiArrayDimension) * 2);
	data_msg.layout.dim.push_back(std_msgs::MultiArrayDimension());
	data_msg.layout.dim.push_back(std_msgs::MultiArrayDimension());
	data_msg.layout.dim[0].label = "PerceptionNeuronData";
	data_msg.layout.dim[0].size = MAX_DATA_LENGTH;
	// adapted ros_lib/ros/node_handle.h buffer limitations to 1024 (max would be 2048)
	// that we can use MAX_DATA_LENGTH for data_msg.data_length.
	// data_msg.data_length = MAX_DATA_LENGTH;
	data_msg.layout.data_offset = 0;
	//double* ptr = (double*) malloc(sizeof(float) * 1048);
  std::vector<float> data(120);
  data_msg.data.clear();
  data_msg.data.insert(data_msg.data.end(), data.begin(), data.end());
}

int main(int argc, char * argv[])
{
	// first set some default values if no config file found
	std::string ipAxisNeuron = "192.168.201.3";
	int portAxisNeuron = 7001;
	bool verbose = false;

  ros::init(argc, argv, "perception_neuron_bridge_node");
	// ROS Handle
  ros::NodeHandle nh;
  ros::NodeHandle pnh("~");

  pnh.param("axisneuron_ip", ipAxisNeuron, std::string("192.168.201.3"));
  pnh.param("axisneuron_port", portAxisNeuron, 7001);
  pnh.param("verbose", verbose, false);

	// Neuron Connection
	void * neuronptr = NULL;
	if (BRGetSocketStatus(neuronptr) == SocketStatus::CS_Running) {
		BRCloseSocket(neuronptr);
	}
	char *nIP = new char[ipAxisNeuron.length() + 1];
	strcpy(nIP, ipAxisNeuron.c_str());
	neuronptr = BRConnectTo(nIP, portAxisNeuron);
	if (neuronptr == NULL) {
		ROS_ERROR("Axis Neuron Connection refused!");
		return 0;
	}
	else {
		ROS_INFO("Connected to Axis Neuron at %s", nIP );
	}

	registerNeuronCallbacks();

	ROS_INFO("Advertising Axis Neuron Data to ROS");

	// Prepare the data msg arrays for the ros publisher
	std_msgs::Float64MultiArray data_msg_1, data_msg_2, data_msg_3;
	prepareDataMsg(data_msg_1);
	prepareDataMsg(data_msg_2);
	prepareDataMsg(data_msg_3);

	ros::Publisher data_pub_1 = nh.advertise<std_msgs::Float64MultiArray>("/perception_neuron/data_1", 1000);
	ros::Publisher data_pub_2 = nh.advertise<std_msgs::Float64MultiArray>("/perception_neuron/data_2", 1000);
	ros::Publisher data_pub_3 = nh.advertise<std_msgs::Float64MultiArray>("/perception_neuron/data_3", 1000);

	// Wait a bit, till first data arrived from
	// Axis Neuron.
	ros::Duration(1).sleep();
	while (ros::ok())
	{
		if (verbose) {
			ROS_INFO("Current Data Frame %i", _frameCount);
		}

		if (_valuesBuffer[114]) {

			for (int i = 0; i < MAX_DATA_LENGTH; i++) {
				data_msg_1.data[i] = _valuesBuffer[i];
				data_msg_2.data[i] = _valuesBuffer[i + MAX_DATA_LENGTH];
				data_msg_3.data[i] = _valuesBuffer[i + 2*MAX_DATA_LENGTH];
			}

			// Publish part one of the array
			data_pub_1.publish(data_msg_1);
			data_pub_2.publish(data_msg_2);
			data_pub_3.publish(data_msg_3);
    }

		ros::spinOnce();
		ros::Duration(1.0/50.0).sleep();
	}
	BRCloseSocket(neuronptr);
	ROS_INFO("All done!");
	return 0;
}
