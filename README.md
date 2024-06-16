# Projeto - Comunicação em Sockets

## Desenvolver

- Gustavo Gabriel Ribeiro - 13672683

## Descrição

### Server.c

#### Explanations

- `socket(AF_INET, SOCK_STREAM, 0)`
  - AF_INET
    - Address Family Internet, specifies the protocol family to be used. In this case, AF_INET is used for IPv4 addresses. This means the socket will be used for communicating over an IPv4 network.
  - SOCK_STREAM
    - Socket type. SOCK_STREAM is used for a stream socket, which provides a sequenced, reliable, two-way, connection-based byte stream. Used for TCP (Transmission Control Protocol) connections.

- To use UDP
  - `socket(AF_INET, SOCK_DGRAM, 0)`
    - Connectionless, unreliable (no guaranteed delivery), message-based.


# Network Communication App

## Overview

This project demonstrates a simple network communication application using sockets in C. The application can be configured to use either TCP or UDP sockets, enabling communication between processes on the same or different machines.

## Features

- Supports both TCP and UDP sockets
- Easy setup and configuration
- Includes a server and multiple clients
- Demonstrates basic socket programming concepts

## Setup

### Prerequisites

- GCC Compiler
- GNU Make
- Bash shell
- xdotool (for script automation)

### Compilation

To compile the server and client programs, run:

```sh
make all
