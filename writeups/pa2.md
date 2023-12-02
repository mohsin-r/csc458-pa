Programming Assignment 2 Writeup
====================

My name: Mohsin Reza

My UTORID : rezamohs

I collaborated with: noone

I would like to credit/thank these classmates for their help: Yu Xue. I used this idea of his from piazza: https://piazza.com/class/lm9g0yyr9l4744/post/369_f1

This programming assignment took me about 8 hours to do.

### Program Structure and Design of the Router

I added one data structure to the `Router` class, which was the routing table. My routing table was an unordered map with the prefix IP Address in 32 bit number format as the key and a tuple of the form (prefix length, (optional) next hop address, interface number). I decided to use an unordered map as accessing an entry would be more efficient compared to a vector.

I also added one private helper method inside the `Router` class called `find_best_match_`, which searches through the routing table to find the matching route with the longest prefix for a given address. 

Finally, I added one helper function outside the `Router` class called `check_prefix_match`, which checks whether the prefix length most significant bits of a given prefix IP Address match with a given address.

These helper functions and methods provide an overview of how the 2 public methods in the `Router` class were broken down into smaller tasks, which hopefully makes the code more readable. 

Finally, the 2 public methods in the `Router` class were implemented as per the steps given in the assignment handout

### Implementation Challenges
The main challenge I had when implmeneting was ensuring that I was using the reference/alias of a variable rather than a copy of it. It took me several hours to figure out that the reason I was failing the tests is that I was using copies of the network interfaces rather than aliases. Once I changed all the types in my code to use & in the type, the problem was resolved.

I also had a bit of trouble figuring out how to use the optional data type. For example, in my `find_best_match_`, I was not updating the value of the optional variable correctly, which led to a no best match found result everytime. After reading the documentation properly, I was able to resolve this issue as well.

Remaining Bugs:
None as far as I know.

Optional: I was surprised by how interesting and enjoyable it was to work on these programming assignments. I sometimes found the theoretical concepts in the lectures a bit dry, but the practical aspect of implementing the network interface and the router was really fun.
