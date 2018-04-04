# perception-neuron-ros
This repository contains two ROS Modules: 
- a Windows ROS Serial Module, which sends the Perception Neuron BVH Data to a ROS Serial Server
- a ROS BVH tf broadcaster package
- (**new**) a native macOS ROS node that implements the same functionality as Windows ROS Serial Module. The node uses `NeuronDataReader b17` version.

Usage 
=====

## Windows
On your windows machine:

- Connect your Perception Neuron to Axis Neuron and publish the BVH data.
- Adapt IP addresses for ROS Serial Server and Axis Neuron in `windows/config.txt`.
- Start `windows/PerceptionNeuronROSSerial.exe`.


On your ROS machine:

- start `roscore` and ros serial server
- start `perception_neuron_tf_broadcaster`

## macOS

On your macOS ROS machine:

- Connect your Perception Neuron to Axis Neuron and publish the BVH data.
- start `roscore`. 
- start `perception_neuron_bridge_node`. The node accepts two ROS-parameters: `~/axisneuron_ip` and `~/axisneuron_port`.
- start `perception_neuron_tf_broadcaster`.
