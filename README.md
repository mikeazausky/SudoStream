# SudoStream
04.12.2018

Simple music streaming service.

## Features
 * Documentation of proper use of sockets in Linux
 * Server side support for any number of clients
 * Connects to any IP address on the web (traffic is not encrypted)
 * Plays audio file even if download is not finished

## Requirements
Packages (tested on Ubuntu 18.04 fresh install)

* libvlc-dev
* vlc

## Usage
* Set server address and port on client.c and server.c (default: 127.0.0.1:33016)
* Compile with 'make'
* Run ./bin/server and ./bin/client
* Enter comands on client's TUI
