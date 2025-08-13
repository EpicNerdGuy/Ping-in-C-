# Ping Utility in C

## Overview

This project is a minimal implementation of the `ping` utility written in C using raw sockets.
It sends ICMP Echo Request packets to a given IP address and measures the round-trip time (RTT) for the replies.

This implementation:

* Uses raw sockets (`SOCK_RAW`) to send and receive ICMP packets.
* Manually constructs and parses ICMP headers.
* Calculates checksums for packet integrity.
* Measures RTT using `gettimeofday`.
* Handles `SIGINT` (Ctrl+C) to display packet statistics before exiting.

---

## Requirements

* Linux-based system (requires root privileges to open raw sockets).
* GCC or any C compiler.
* Basic understanding of networking and ICMP protocol.

---

## Compilation

```bash
gcc ping.c -o ping
```

---

## Usage

Run as root (required for raw socket access):

```bash
sudo ./ping
```

You will be prompted to enter the target IP address:

```
Enter IP address to send packets: 8.8.8.8
Reply from 8.8.8.8: RTT = 123.45 µs
Reply from 8.8.8.8: RTT = 110.23 µs
...
```

Stop the program with `Ctrl+C` to see the statistics:

```
--- Ping statistics ---
4 packets transmitted, 4 received, 0.0% packet loss
```

---

## How It Works

1. **Socket Creation**
   Creates a raw socket with:

   ```c
   socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
   ```

2. **ICMP Header Structure**
   Defines a custom `struct icmp_header` to store ICMP packet fields.

3. **Checksum Calculation**
   Implements an Internet checksum function to verify packet integrity.

4. **Packet Sending**
   Fills the ICMP header with:

   * Type = 8 (Echo Request)
   * Code = 0
   * Unique process ID
   * Sequence number
   * Calculated checksum

5. **Packet Receiving**
   Reads incoming packets, extracts the IP header, and checks for:

   * ICMP Echo Reply (Type = 0)
   * Matching process ID

6. **RTT Calculation**
   Measures time between sending and receiving the packet using `gettimeofday`.

7. **Signal Handling**
   Captures `SIGINT` to print statistics before exiting.

---

## Notes

* **Run as root**: Raw sockets require elevated privileges.
* This program works for IPv4 targets only.
* RTT is shown in microseconds (µs).
* No DNS resolution is implemented — use IP addresses directly.
* Some networks or firewalls may block ICMP packets.

---

## Example Session

```bash
$ sudo ./ping
Enter IP address to send packets: 8.8.8.8
Reply from 8.8.8.8: RTT = 243.00 µs
Reply from 8.8.8.8: RTT = 252.00 µs
^C
--- Ping statistics ---
2 packets transmitted, 2 received, 0.0% packet loss
```

---
