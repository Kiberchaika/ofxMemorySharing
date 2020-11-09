#pragma once

#include "AudioData.h"
#include "SharedMemory.h"

#include "oscpp/server.hpp"
#include "oscpp/print.hpp"
#include "UDPsocket.h"

#include <thread>
#include <chrono>
#include <ctime>  

class AudioSender {
	UDPsocket socketBroadcast;
	UDPsocket socket;

	std::string nameSharedMemory;

	SharedMemoryWriter sharedMemoryWriter;
	AudioData audioData;
	AudioDataWriter audioDataWriter;

	std::thread threadSocket;


	bool isRunning;

public:

	std::function<void(std::string)> callbackReceiveData;

	std::string name;
	int bufferSize;
	int	sampleRate;
	int channels;
	int memoryQueueSize;
	int portReceive = -1;
	int portSend = -1;

	AudioSender() {
		memoryQueueSize = 2;
	}

	~AudioSender() {
		close();
	}
    
    bool isActivated() {
        return isRunning;
    }

	void init() {
		close();

		socket.open();
		for (int i = PORT_MEMORYSHARING + 1; i < PORT_MEMORYSHARING + 1000; i++) {
			if (socket.bind(i) == (int)UDPsocket::Status::OK) {
				portReceive = i;
				break;
			}
		}

		audioData.init(bufferSize * channels, memoryQueueSize);

		socketBroadcast.open();
		socketBroadcast.broadcast(true);

		for (int c = 1000; c < 5000; c++) {
			if (sharedMemoryWriter.init("audioSharing_" + std::to_string(c), c, audioData.getSize())) {
#ifdef TARGET_WIN32
				nameSharedMemory = "audioSharing_" + ofToString(c);
#else
				nameSharedMemory = std::to_string(c);
#endif
				break;
			}
		}
        
        if(!sharedMemoryWriter.isOpened()) {
            cout << std::string("Error while open memory sharing for write!") << endl;
            throw std::exception();
        }

		isRunning = true;

		threadSocket = std::thread([&]() {
			UDPsocket::IPv4 ipaddr;
			std::string receivedString;

			while (isRunning) {
                receivedString = "";
				size_t dataSize = socket.recv(receivedString, ipaddr);
				if (!receivedString.empty()) {
					if (!strncmp(receivedString.data(), "port:", 5)) {
						char* pch = strtok((char*)receivedString.data(), ":");
						pch = strtok(NULL, ":");
	
						portSend = std::stoi(pch);
					}
					else {
						if (callbackReceiveData) callbackReceiveData(receivedString);
					}
				}
			}
		});
		threadSocket.detach();

	}

	void update() {
		if (isRunning && sharedMemoryWriter.isOpened()) {
			std::vector<char> buffer(1024 * 2);
			OSCPP::Client::Packet packet(buffer.data(), buffer.size());
			packet.
				openMessage("/memorySharing", 7).
				string(nameSharedMemory.c_str()).
				string(name.c_str()).
				int32(bufferSize).
				int32(sampleRate).
				int32(channels).
				int32(memoryQueueSize).
				int32(portReceive).
				closeMessage();
			buffer.resize(packet.size());

			if (socketBroadcast.send(buffer, UDPsocket::IPv4::Loopback(PORT_MEMORYSHARING)) == (int)UDPsocket::Status::SendError) {
				std::cout << "socket broadcast send error" << std::endl;
			}
			// cout << "nameSharedMemory: " << nameSharedMemory << endl;
		}
	}
	
	float* getDataPointer() {
		return audioData.getDataPointer();
	}

	void writeData() {
		audioDataWriter.writeToMemory(sharedMemoryWriter, audioData);
	}

	void sendData(std::string str) {
		if (portSend < 0 || socket.send(str, UDPsocket::IPv4::Loopback(portSend)) == (int)UDPsocket::Status::SendError) {
			std::cout << "socket send error" << std::endl;
		}
	}

	void close() {
		if (isRunning) {
			isRunning = false;
			std::this_thread::sleep_for(std::chrono::milliseconds(100));

			sharedMemoryWriter.close();
			socketBroadcast.close();
			socket.close();
		}
	}
};

