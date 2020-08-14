#pragma once

#include "AudioData.h"
#include "SharedMemory.h"
#include "oscpp/server.hpp"
#include "oscpp/print.hpp"
#include "UDPsocket.h"
#include <chrono>
#include <ctime>  

class AudioSender {
	UDPsocket socket;

	string nameSharedMemory;

	SharedMemoryWriter sharedMemoryWriter;
	AudioData audioData;
	AudioDataWriter audioDataWriter;

	bool isRunning;

public:
	int bufferSize;
	int	sampleRate;
	int channels;
	int queueSize;

	AudioSender() {
		queueSize = 2;
	}

	~AudioSender() {
		close();
	}

	void init() {
		close();

		audioData.init(bufferSize * channels, queueSize);

		socket.open();
		socket.broadcast(true);

		for (int c = 1000; c < 5000; c++) {
			if (sharedMemoryWriter.init("audioSharing_" + ofToString(c), c, audioData.getSize())) {
#ifdef TARGET_WIN32
				nameSharedMemory = "audioSharing_" + ofToString(c);
#else
				nameSharedMemory = to_string(c);
#endif
				break;
			}
		}

		isRunning = true;
	}

	void update() {
        std::vector<char> buffer(4098);
		OSCPP::Client::Packet packet(buffer.data(), buffer.size());
		packet.
			openMessage("/memorySharing", 5).
			string(nameSharedMemory.c_str()).
			int32(bufferSize).
			int32(sampleRate).
			int32(channels).
			int32(queueSize).
			closeMessage();
        buffer.resize(packet.size());

        if(socket.send(buffer, UDPsocket::IPv4::Broadcast(PORT_MEMORYSHARING)) == (int)UDPsocket::Status::SendError) {
           cout << "socket error" << endl;
        }
		// cout << "nameSharedMemory: " << nameSharedMemory << endl;
	}
	
	float* getDataPointer() {
		return audioData.getDataPointer();
	}

	void writeData() {
		audioDataWriter.writeToMemory(sharedMemoryWriter, audioData);
	}

	void close() {
		if (isRunning) {
			isRunning = false;
			sharedMemoryWriter.close();
			socket.close();
		}
	}
};

