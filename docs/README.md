# Custom Network Proxy Server (Linux/C++)

![Language](https://img.shields.io/badge/Language-C++11-blue.svg)
![Platform](https://img.shields.io/badge/Platform-Linux%20%7C%20WSL-orange.svg)
![License](https://img.shields.io/badge/License-MIT-green.svg)

A high-performance, **Multi-threaded HTTP/HTTPS Proxy Server** designed and implemented in C++ using **POSIX Sockets**. This project features a modular architecture that separates networking, caching, and configuration logic. It supports concurrent client connections, traffic filtering, and in-memory caching to optimize network performance.

This implementation demonstrates advanced systems programming concepts including **Socket Programming**, **Multi-threading (Pthreads)**, **LRU Caching**, and **Makefile-based build automation**.

---

##  Key Features

### 1. Multi-Threaded Concurrency
- Uses **POSIX Threads (`pthread`)** to handle multiple client connections simultaneously.
- Implements a thread-per-connection model to ensure non-blocking performance for concurrent users.

### 2. High-Performance LRU Cache
- Features a custom **Least Recently Used (LRU) Cache**.
- Stores frequently accessed HTTP responses in memory (RAM) using `std::unordered_map` and `std::list` for **O(1)** access time.
- Reduces bandwidth usage and latency for repeated requests.

### 3. Traffic Filtering & Security
- Implements a **Domain Blacklist** system backed by a configuration file.
- Blocks restricted websites (e.g., Facebook, Instagram) and returns a custom `403 Forbidden` HTML response.

### 4. HTTPS Tunneling
- Supports secure HTTPS connections via the **HTTP CONNECT** method.
- Establishes a transparent TCP tunnel between the client and the destination server, allowing encrypted traffic (TLS) to pass through.

### 5. Modular & Configurable
- Fully configurable via `proxy.conf` to change ports, buffer sizes, and cache capacity without recompiling.
- Follows a strict directory structure separating headers, source code, and configurations.

---

##  Project Structure

The project follows a modular production-level layout:

```text
custom-proxy-server/
├── Makefile                   # Build automation script
├── config/                    # Configuration files
│   ├── proxy.conf             # Server settings (Port, Cache Size)
│   └── blocked_domains.txt    # List of blocked domains
├── include/                   # Header files (.h)
│   ├── cache.h
│   └── proxy_server.h
├── src/                       # Source code (.cpp)
│   ├── main.cpp
│   ├── proxy_server.cpp
│   └── cache.cpp
├── logs/                      # Runtime logs
└── docs/                      # Documentation

```
##  Tech Stack

- **Language:** C++ (Standard 11+)
- **OS:** Linux / WSL (Windows Subsystem for Linux)
- **Networking:** POSIX Sockets (`<sys/socket.h>`, `<netinet/in.h>`)
- **Concurrency:** POSIX Threads (`<pthread.h>`)
- **Build System:** GNU Make (`Makefile`)

---

##  How to Run

### Prerequisites
- A Linux environment (Ubuntu, Kali, or **WSL** on Windows).
- `g++` compiler and `make` utility installed.

```bash
sudo apt update && sudo apt install build-essential
```
### Step 1: Get the Source Code
You can download the code using Git or set it up manually.
**Using Git (Recommended)**

```bash
https://github.com/kumar-chandan23116027/network_proxy.git
cd custom-proxy-server
```
### Step 2: Build the Project
Use the provided `Makefile` to compile all modules automatically. This will handle the linking of headers and source files:

```bash
make
```
### Step 3: Run the Server

```bash
./proxy_server
```
You should see output similar to:

```text
MASTER PROXY SERVER (Modular Edition)
Listening on Port 8080
```
---


## 🔧 Configuration

You can customize the server behavior by editing the files in the `config/` folder.

**1. Server Settings (`config/proxy.conf`):**
Change the listening port, buffer size, or cache capacity.

```ini
port = 8080
buffer_size = 8192
cache_capacity = 20
blacklist_file = config/blocked_domains.txt
```
**2. Blocking Sites (`config/blocked_domains.txt`):**
Add domain names (one per line) to block them.

```text
facebook.com
instagram.com
tiktok.com
```

##  Configuration & How to Use

After running the Proxy Server, you need to configure the network settings on your laptop (or your friend's laptop) to route internet traffic through this server.

###  How to Test on Your Own Laptop (Localhost Testing)
If you want to test the proxy on the same machine where the code is running:

1. **Run the Proxy Server:**
   That is already running from the above step
2. **Open Settings:**
   On Windows, go to **Settings > Network & Internet > Proxy**.
3. **Manual Proxy Setup:**
   - Turn **ON** the "Use a proxy server" toggle.
   - **Address:** `127.0.0.1` (This represents localhost).
   - **Port:** `8080`.
4. **Save Changes:**
   Click the **Save** button.
5. Test the Features (Blocking vs Passing)
Now, verify that the proxy correctly filters traffic.

**A. Test Allowed Traffic (Google/Example):**
1. Open your browser and visit `https://google.com` or `http://example.com`.
2. **Result:** The website should load normally.
3. **Terminal Log:** You will see a successful request log:

**B. Test Blocked Traffic (Instagram):**
1. Now, try to visit `http://instagram.com` or `https://instagram.com`.
2. **Result:** The website will **NOT** load (The browser will show "Connection Reset" or keep loading).
3. **Terminal Log:** You will see a block warning in the server terminal:

---

##  How to Connect Other Devices (LAN Setup)

To allow your friend (or another device) to use your Proxy Server, **both devices must be on the same network.**

### Step 0: Create a Network (Hotspot)
The easiest way to connect devices is by using your laptop's Hotspot.

1. **Turn ON Mobile Hotspot** on your laptop (where the code is running).
2. Ask your friend to turn on their Wi-Fi and **connect to your Hotspot**.
3. Ensure they are successfully connected before proceeding.

*(Note: If they are not connected to your Hotspot or the same Wi-Fi router, this will not work.)*

---

### Step 1: Find Server IP Address
Once your friend is connected to your Hotspot, find your IP address on the Server laptop: 

1. Open **Command Prompt (cmd)**.
2. Type `ipconfig` and press Enter.
3. Look for **IPv4 Address**.
   - Example: `192.168.137.1` (This is the standard IP for Windows Hotspot).
   - **Note this address down.**

---

### Step 2: Configure Friend's Laptop/Mobile
Now, go to your friend's device and enter the IP you found above.

**For Windows (Friend's Laptop):** 
1. Go to **Settings > Network & Internet > Proxy**.
2. Under "Manual proxy setup", click **Set up**.
3. Turn ON **Use a proxy server**.
4. **Proxy IP address:** Enter the IP found in Step 1 (e.g., `192.168.137.1`).
5. **Port:** `8080`.
6. Click **Save**.

**For Android/iOS:** 
1. Go to Wi-Fi Settings and tap the connected network (Your Hotspot).
2. Scroll to **Proxy** and select **Manual**.
3. **Hostname:** Enter the IP found in Step 1.
4. **Port:** `8080`.
5. Save the settings.

---

### Step 3: Troubleshooting Firewall (If it doesn't work)
If your friend is connected but pages aren't loading, your **Windows Firewall** is likely blocking the connection. (if you using wsl)

**To fix this:**
### Step 1. Off the server (ctrl+c) if it is running

### Step 2: Find your WSL IP Address (Internal IP)
First, we need to know the IP address where your Linux code is running.

1. Open your **VS Code Terminal (WSL)**.
2. Run this command:
   ```bash
   hostname -I
   ```
3. You will see an IP address (e.g., 172.27.x.x or 172.29.x.x).
4. Copy this IP. (We will call this WSL_IP).

### Step 3: Create a Bridge in Windows (Port Forwarding)
Now, tell Windows to forward all traffic from Port 8080 to your WSL IP.

1. Press the **Start Button** and search for `PowerShell`.
2. **Right-click** on it and select **"Run as Administrator"**. (Click Yes).
3. Copy the command below, **replace `<WSL_IP>` with the IP you copied in Step 1**, and paste it into PowerShell:
   
   *(Note: Do not include the brackets `< >`, just write the number)*

```powershell
netsh interface portproxy add v4tov4 listenport=8080 listenaddress=0.0.0.0 connectport=8080 connectaddress=<WSL_IP>
```
### Step 4: Configure Windows Firewall (Temporary Fix)
Windows Firewall often blocks incoming connections from other laptops.

1. Search for **"Firewall & network protection"** in Windows Settings.
2. You will see three networks: **Domain**, **Private**, and **Public**.
3. Click on **Public Network (active)** (Since Hotspots are usually treated as Public).
4. Toggle **Microsoft Defender Firewall** to **OFF**.
   *(Remember to turn this back ON after testing).* 

### Step 5: Configure Your Friend's Laptop
Now, find the IP address that your friend needs to enter.

**A. Find Your Host IP:**

1. Open **Command Prompt (cmd)** on your Windows laptop.
2. Run:

```cmd
ipconfig
```
3. Look for the adapter named **"Wireless LAN adapter Local Area Connection* 1"** (or similar, created by your Hotspot).
   * The **IPv4 Address** is usually `192.168.137.1`.
   * **Note this IP.**

**B. Set Proxy on Friend's Laptop:**

1. Connect your friend's laptop to your **Mobile Hotspot**.
2. Go to **Settings > Network & Internet > Proxy**.
3. Under **Manual proxy setup**:
   * **Address:** Enter the IP found in Step 4A (e.g., `192.168.137.1`).
   * **Port:** `8080`.
4. Click **Save**.

###  Final Test
1. Go back to your VS Code (WSL) and start the server:

```bash
./proxy_server
```
2. Open a browser on your friend's laptop and visit `http://example.com`.
3. Check your server terminal. You should see request logs!

###  DEMO
https://github.com/user-attachments/assets/00aae465-bd49-426c-85ca-028c9542a481

