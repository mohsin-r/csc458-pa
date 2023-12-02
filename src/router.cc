#include "router.hh"

#include <iostream>
#include <limits>

using namespace std;

// HELPER FUNCTIONS

// checks whether the prefix_length most significant bits of route_prefix match with address
bool check_prefix_match( const uint32_t route_prefix, const uint8_t prefix_length, const uint32_t address )
{
  // special case where prefix length is 0
  // we consider the addresses equal by "vacuous truth"
  if ( prefix_length == 0 ) {
    return true;
  }
  // right shifting by 32 - prefix_length makes the prefix_length most significant bits
  // become the prefix_length least significant bits, with the rest of the bits filled with zeros
  // since we have an unsigned int
  // we can then compare whether the numbers are equal
  return ( route_prefix >> ( 32 - prefix_length ) ) == ( address >> ( 32 - prefix_length ) );
}

// PRIVATE HELPER METHOD
// Searches through the routing table to find the matching route with the longest prefix for the given address
// Returns the matching route's prefix IP address, if a match was found.
std::optional<uint32_t> Router::find_best_match_( uint32_t address )
{
  std::optional<uint32_t> match = {};
  uint8_t longest_prefix = 0;
  // iterate through all routing table entries
  for ( const auto& [key, entry] : routing_table_ ) {
    const uint32_t prefix_address = key;
    const uint8_t prefix_length = std::get<uint8_t>( entry );
    // check if prefix_address bits of the prefix IP Address and given address match
    // and that the prefix_address is the longest (here ties are broken arbitrarily since we have an unordered map)
    // if this is the best match, update accordingly
    if ( check_prefix_match( prefix_address, prefix_length, address ) && prefix_length >= longest_prefix ) {
      match = prefix_address;
      longest_prefix = prefix_length;
    }
  }
  return match;
}

// PUBLIC METHODS

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route( const uint32_t route_prefix,
                        const uint8_t prefix_length,
                        const optional<Address> next_hop,
                        const size_t interface_num )
{
  routing_table_[route_prefix] = std::make_tuple( prefix_length, next_hop, interface_num );
}

void Router::route()
{
  // iterate through all network interfaces
  for ( AsyncNetworkInterface& interface : interfaces_ ) {
    // iterate through all datagrams for this interface
    while ( std::optional<InternetDatagram> optional_datagram = interface.maybe_receive() ) {
      InternetDatagram& datagram = optional_datagram.value();
      const std::optional<uint32_t> best_match = Router::find_best_match_( datagram.header.dst );
      // do nothing if the packet if no match was found or TTL was 0 before this or will become 0 as a result of
      // this
      if ( best_match.has_value() && datagram.header.ttl > 1 ) {
        // decrement TTL field and recompute the checksum
        datagram.header.ttl--;
        datagram.header.compute_checksum();
        const size_t interface_num = std::get<size_t>( routing_table_[best_match.value()] );
        // determine whether the packet should be sent to the next_hop or directly to the destination
        const std::optional<Address> next_hop
          = std::get<std::optional<Address>>( routing_table_[best_match.value()] );
        const Address dst_add
          = next_hop.has_value() ? ( next_hop.value() ) : Address::from_ipv4_numeric( datagram.header.dst );
        // send packet to appropriate address
        Router::interface( interface_num ).send_datagram( datagram, dst_add );
      }
    }
  }
}
