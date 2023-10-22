#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

using namespace std;

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
  // lookup MAC Address of next hop and create ethernet frame if it does
  if ( arp_table_.contains( next_hop.ipv4_numeric() ) ) {
    const std::pair<int, EthernetAddress> lookup = arp_table_[next_hop.ipv4_numeric()];
    EthernetHeader header;
    header.type = EthernetHeader::TYPE_IPv4;
    header.src = ethernet_address_;
    header.dst = std::get<EthernetAddress>( lookup );
    EthernetFrame frame;
    frame.header = header;
    frame.payload = serialize( dgram );
    send_queue_.push( frame );
  } else {
  }
}

// frame: the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame( const EthernetFrame& frame )
{
  (void)frame;
  return {};
}

// ms_since_last_tick: the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  (void)ms_since_last_tick;
}

optional<EthernetFrame> NetworkInterface::maybe_send()
{
  return {};
}

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
