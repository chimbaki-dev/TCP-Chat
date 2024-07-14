# TCP Chat

This is a simple console-based chat application implemented in C++ using Winsock for socket programming on Windows. The application allows multiple clients to connect to a server and chat with each other.

## Features
- **Client-Server Architecture**: Uses TCP/IP protocol for communication between clients and server.
- **Multi-threaded Communication**: Each client operates on its own thread for receiving and sending messages concurrently.
- **Username Support**: Clients can enter their usernames which are displayed along with their messages.
- **Basic Command**: Supports `/quit` command to gracefully disconnect from the server.
