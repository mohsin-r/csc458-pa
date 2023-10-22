#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

using namespace std;

// HELPER FUNCTIONS

// determine whether the two ethernet addresses are equal or not
bool check_ethernet_address_equal( const EthernetAddress add1, const EthernetAddress add2 )
{
  if ( add1.size() != add2.size() ) {
    return false;
  }
  for ( size_t i = 0; i < add1.size(); i++ ) {
    if ( add1[i] != add2[i] ) {
      return false;
    }
  }
  return true;
}

// create an ethernet frame with the given properties
EthernetFrame create_ethernet_frame( const uint16_t type,
                                     const EthernetAddress src,
                                     const EthernetAddress dst,
                                     const std::vector<Buffer> payload )
{
  EthernetHeader header;
  header.type = type;
  header.src = src;
  header.dst = dst;
  EthernetFrame frame;
  frame.header = header;
  frame.payload = payload;
  return frame;
}

// create an arp message with the given properties
ARPMessage create_arp_message( const uint16_t opcode,
                               const EthernetAddress sender_ethernet_address,
                               const uint32_t sender_ip_address,
                               const EthernetAddress target_ethernet_address,
                               const uint32_t target_ip_address )
{
  ARPMessage message;
  message.opcode = opcode;
  message.sender_ethernet_address = sender_ethernet_address;
  message.sender_ip_address = sender_ip_address;
  message.target_ip_address = target_ip_address;
  if ( opcode == ARPMessage::OPCODE_REPLY ) {
    message.target_ethernet_address = target_ethernet_address;
  }
  return message;
}

// NetworkInterface

// ethernet_address: Ethernet (what ARP calls "hardware") address of the interface
// ip_address: IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( const EthernetAddress& ethernet_address, const Address& ip_address )
  : ethernet_address_( ethernet_address ), ip_address_( ip_address )
{
  cout << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address_ ) << " and IP address "
       << ip_address.ip() << "\n";
}

// dgram: the IPv4 datagram to be sent
// next_hop: the IP address of the interface to send it to (typically a router or default gateway, but
// may also be another host if directly connected to the same network as the destination)

// Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) by using the
// Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
  const uint32_t dst_ip = next_hop.ipv4_numeric();
  if ( arp_table_.contains( dst_ip ) ) {
    // lookup MAC Address of next hop was successful
    // create ethernet frame and put in sending queue
    const std::pair<size_t, EthernetAddress> lookup = arp_table_[dst_ip];
    const EthernetFrame frame = create_ethernet_frame(
      EthernetHeader::TYPE_IPv4, ethernet_address_, std::get<EthernetAddress>( lookup ), serialize( dgram ) );
    send_queue_.push( frame );
  } else {
    send_arp_request_( dgram, dst_ip );
  }
}

// frame: the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame( const EthernetFrame& frame )
{
  if ( !check_ethernet_address_equal( frame.header.dst, ethernet_address_ )
       && !check_ethernet_address_equal( frame.header.dst, ETHERNET_BROADCAST ) ) {
    // destination address did not match our address or brodcast
    // exit early
    return {};
  } else if ( frame.header.type == EthernetHeader::TYPE_IPv4 ) {
    // payload is an ipv4 packet
    // attempt to parse and return datagram if successful
    InternetDatagram dgram;
    if ( parse( dgram, frame.payload ) ) {
      return dgram;
    } else {
      return {};
    }
  } else {
    // payload is an arp packet
    ARPMessage message;
    if ( parse( message, frame.payload ) ) {
      handle_arp_response_( message );
      handle_arp_request_( message );
    }
    return {};
  }
}

// ms_since_last_tick: the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  // increment internal time tracker
  time_ += ms_since_last_tick;

  // expire any entry in the arp table that was learnt more than 30 seconds ago
  // code is based on this documentation: https://en.cppreference.com/w/cpp/container/unordered_map/erase
  for ( auto it = arp_table_.begin(); it != arp_table_.end(); ) {
    if ( time_ - std::get<size_t>( it->second ) > 30000 ) {
      it = arp_table_.erase( it );
    } else {
      ++it;
    }
  }

  // remove datagrams waiting for next hop ip address for more than 5 seconds
  // code is based on this documentation: https://en.cppreference.com/w/cpp/container/unordered_map/erase
  for ( auto it = waiting_queues_.begin(); it != waiting_queues_.end(); ) {
    if ( time_ - std::get<size_t>( it->second ) > 5000 ) {
      it = waiting_queues_.erase( it );
    } else {
      ++it;
    }
  }
}

optional<EthernetFrame> NetworkInterface::maybe_send()
{
  if ( send_queue_.size() == 0 ) {
    return {};
  }
  const EthernetFrame frame_to_send = send_queue_.front();
  send_queue_.pop();
  return frame_to_send;
}

// Private Helper Functions

void NetworkInterface::send_arp_request_( const InternetDatagram dgram, const uint32_t dst_ip )
{
  if ( waiting_queues_.contains( dst_ip ) && time_ - std::get<size_t>( waiting_queues_[dst_ip] ) <= 5000 ) {
    // we have requested this ip address in the last 5 seconds
    // simply add the datagram to the queue of datagrams waiting on the ip address
    std::get<1>( waiting_queues_[dst_ip] ).push( dgram );
  } else {
    // this ip address has not been requested in the last 5 seconds
    // create queue for datagrams waiting for destination ip's mac address and
    // push datagram to queue
    // we can guarantee that the queue does not exist already because queues are deleted after 5 seconds
    std::queue<InternetDatagram> queue;
    queue.push( dgram );
    const std::pair<size_t, std::queue<InternetDatagram>> waiting_queue_entry { time_, queue };
    waiting_queues_[dst_ip] = waiting_queue_entry;

    // create ARP request for destination ip and add it to ready to send queue
    ARPMessage request
      = create_arp_message( ARPMessage::OPCODE_REQUEST, ethernet_address_, ip_address_.ipv4_numeric(), {}, dst_ip );
    const EthernetFrame frame = create_ethernet_frame(
      EthernetHeader::TYPE_ARP, ethernet_address_, ETHERNET_BROADCAST, serialize( request ) );
    send_queue_.push( frame );
  }
}

void NetworkInterface::handle_arp_response_( const ARPMessage response )
{
  // learn mapping between sender's ip address and mac address
  const std::pair<size_t, EthernetAddress> table_entry { time_, response.sender_ethernet_address };
  arp_table_[response.sender_ip_address] = table_entry;

  // check if any packets were waiting for this ip address
  // if yes, remove packets from waiting queue and
  // queue them in ready to send queue
  if ( waiting_queues_.contains( response.sender_ip_address ) ) {
    std::queue<InternetDatagram> waiting_datagrams = std::get<1>( waiting_queues_[response.sender_ip_address] );
    while ( !waiting_datagrams.empty() ) {
      const InternetDatagram dgram = waiting_datagrams.front();
      EthernetFrame send_frame = create_ethernet_frame(
        EthernetHeader::TYPE_IPv4, ethernet_address_, response.sender_ethernet_address, serialize( dgram ) );
      send_queue_.push( send_frame );
      waiting_datagrams.pop();
    }
  }

  // delete waiting queue for sender ip address since we got its mac address
  waiting_queues_.erase( response.sender_ip_address );
}

void NetworkInterface::handle_arp_request_( const ARPMessage request )
{
  // check if this is an arp request for our ip address
  // if yes, create arp reply packet and put it in ready to send queue
  if ( request.opcode == ARPMessage::OPCODE_REQUEST && request.target_ip_address == ip_address_.ipv4_numeric() ) {
    ARPMessage reply = create_arp_message( ARPMessage::OPCODE_REPLY,
                                           ethernet_address_,
                                           ip_address_.ipv4_numeric(),
                                           request.sender_ethernet_address,
                                           request.sender_ip_address );
    EthernetFrame send_frame = create_ethernet_frame(
      EthernetHeader::TYPE_ARP, ethernet_address_, request.sender_ethernet_address, serialize( reply ) );
    send_queue_.push( send_frame );
  }
}
