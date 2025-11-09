# GTK TCP Chat Client (Windows)

A simple cross-platform **TCP chat client** with GUI using **GTK+** on Windows.  
Supports connecting to local servers or remote servers via **ngrok TCP tunnels**.
=======
A simple **GUI-based chat client** built using **GTK** and **Winsock** for Windows.  
It allows users to connect to a TCP server, send and receive messages in real-time, and provides a thread-safe chat interface.
>>>>>>> 434bae919a6c38d3e80401545e073b912e01d3ac

---

## Features
- GUI chat interface using GTK+ (`GtkTextView` + `GtkEntry`)
- Thread-safe message updates
- Private and public messages support (if server supports it)
- Username & password authentication
- Works with hostnames (e.g., ngrok TCP) and numeric IPs
- Auto-scroll chat view
=======
- Connect to a TCP server with **IP**, **Port**, **Username**, and **Password**.
- **Send and receive messages** in real-time.
- **Thread-safe GUI updates** using `g_idle_add`.
- **Scrollable chat window** with automatic scrolling to the latest message.
- Password-masked input during login.
- Simple, lightweight GUI built with **GTK3**.

---

## Requirements

- Windows OS
- **GTK3** development libraries ([Installation Guide](https://www.gtk.org/docs/installations/windows/))
- C Compiler (e.g., GCC with MinGW)
- Winsock (included in Windows SDK)
