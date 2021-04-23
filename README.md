# Build
```make client``` and ```make server``` to build client and server. ```make clean``` to remove binaries and object files.

# Usage

Set the macro ```SERVDIR``` to a directory of choice and run the server. Invoke ```client``` as ```client hostname``` where hostname corresponds to
the host running an instance of ```server```. The following commands are supported:

- ```write <text>``` - writes text to all other clients

- ```put   <file>``` - puts file on the server

- ```get   <file>``` - retrieves file from the server

- ```list```         - displays all files on the server

