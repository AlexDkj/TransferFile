# TransferFile

TransferFile copies files from directory in server side by requests from client side and server copy files as response from the socket.
Uses threads and socket. Socket is the way to transfer files from one host to another host. 
It is implemented as a server client service.

<b> Compile </b>

make all

<b> Run </b>

./dataServer -p port -s thread_pool_size -q queue_size -b block_size
./remoteClient -i server_ip -p server_port -d directory
