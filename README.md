
This is a project I worked on with another person. This is a project about demonstrating socket programming, which was making use of a server for a client to 
communicate with. The server would keep data stored about key value pairs, where a client could add two strings with one being a value and another being a key to search for the value, to search for them, or delete. 

# Key-Value Storage Service
By Riyam Zaman (rnz8) and Vincent Mandola (vam91)

## Program Notes
Once the files are imported, use `make network` to compile the file.
To end the program, assuming no errors, you can use ctrl-c to exit the server. With multiple clients, even if they close due to errors, the server will remain open to wait for other potential incoming connections, in which case you can also use ctrl-c to exit.

## Algorithm
### Storage
To store our data, we decided to implement a thread-safe hash table, since that is the data structure most optimally built for key-value storage and retrieval. We implemented a hash function to appropriately index the keys, with collisions handled by appending a linked list. We also have the set, get, delete, and free functions that are used to execute the general program functions.

### Threading
When a client connects to the server, the server makes a thread with a new connection and runs infinitely until the client exits. The general outline is that the server reads 4 bytes first, which relates to the command (GET, SET, DEL), then reads bytes until we read a newline character to get the length of the payload, and use that length to determine how many more bytes we need to read. After getting the full message, we check for any errors in the message to determine if it's malformed. If not, we carry out the message and update the hashtable with the desired change, or retrieve the requested element. 

## Testing Strategy
To test our program, we ensured that the following mechanisms worked:
- Writing appropriate error messages for malformed commands or arguments, followed by closing the socket
- Closing the program if a port was already in use
- Setting a key-value pair (using the `set` function)
- Updating a key-value pair (using the `set` function)
- Retrieving a previously set key-value pair (using the `get` function)
- Deleting a previously set key-value pair (using the `del` function)
- Outputting the details of the most recent set value for a key when retrieving or deleting a value
- Writing key-not-found (`KNF`) if trying to access a key that has not been set
- Using multiple clients at the same time to interface with the server
- Not allowing for `\0` or `\n` to be part of a key or value
