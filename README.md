# Build
```make client``` and ```make server``` to build client and server. ```make clean``` to remove binaries and object files.

# Usage

Set the macro ```SERVDIR``` to a directory of choice and run the server. Invoke ```client``` as ```client hostname``` where hostname corresponds to
the host running an instance of ```server```. The following commands are supported:

- ```write <text>``` - writes text to all other clients

- ```put   <file>``` - puts file on the server

- ```get   <file>``` - retrieves file from the server

- ```list```         - displays all files on the server

# TODO

Works perfectly if the host and all clients are Linux/MacOS/BSD, however the ```get``` and ```put``` functions break quite horribly if the server and client are different flavours of Unix (this is mostly
due to the hacky way in which the file transfer "protocol" is implemented). Find out exactly why this is happening and fix it.
