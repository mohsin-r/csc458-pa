Programming Assignment 1 Writeup
====================

My name: Mohsin Reza

My UTORID : rezamohs

I collaborated with: noone

I would like to credit/thank these classmates for their help: noone

This programming assignment took me about 5 hours to do.

Program Structure and Design of the NetworkInterface:
I added three data structures to the network interface. 

The first was a time tracking integer, which tracks what the current time is in milliseconds. 

The second was an unordered map for the arp table. Its keys were integers representing the IP address, and its values were pairs of type (integer, EthernetAddress), representing the time at which the mapping was learnt and the MAC address that the IP address is mapped to. 

Finally, I had an unordered map for the waiting queues. Its keys were integers representing the IP address whose MAC address we're waiting for, and its values were pairs of type (integer, Queue of Internet Datagrams), representing the time at which we brodcasted the ARP request for the IP address, as well as a queue of internet datagrams waiting for the ARP response.

Additionally, I also added three private helper methods within the network interface. I made them private as they are only intended for internal use.

The first, called send_arp_request_, takes an integer IP address and a datagram. It creates an Ethernet packet with the ARP request for the IP address as its payload and puts the Ethernet packet into the sending queue. It also puts the datagram in a queue to wait for the response. This is used as a helper for send_datagram.

The second, called handle_arp_response_, takes an ARP message. It caches the mapping for the sender's IP address to MAC address in the ARP table and also puts creates Ethernet packets for any datagrams that were waiting for the response. The Ethernet packets are put in the ready to send queue. This is used as a helper for recv_frame.

The third and final, called handle_arp_request_, takes an ARP message with the request opcode. It checks whether the request is for our IP address, creates a response ARP message, packages it into an Ethernet frame, and puts in the ready to send queue. This is used as a helper for recv_frame.

I also created three helper functions outside the network interface. They are outside of the interface because they do not require accessing or updating the internal data structures. One checks whether two given MAC addresses are equal by looping through the vector, the second generates an ethernet frame given parameters, and the third generates an ARP message given parameters.

These helper functions and methods provide an overview of how the 4 public methods in the interface were broken down into smaller tasks, which hopefully makes the code more readable.

Implementation Challenges:
The main challenge I faced when implementing was ensuring I was translating the intructions into my code correctly. For example, one of the requirements is that after more than 5 seconds, any datagrams in the queue waiting for the ARP response should be deleted. I was not sure whether all the datagrams waiting should have been deleted, or only the ones waiting for more than 5 seconds (I was able to figure it out evenetually). Another small challenge was designing the data structures. For example, I initially forgot to include any kind of time tracking in my ARP table or waiting queues. I then came up with the design to have pairs of (time cached, MAC address) in my map as opposed to just the MAC address in the ARP table, and did the equivalent for the waiting queues map.

Remaining Bugs:
None (as far as I know).
