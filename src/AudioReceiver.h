#pragma once

#include "AudioData.h"
#include "SharedMemory.h"
#include "oscpp/server.hpp"
#include "oscpp/print.hpp"
#include "UDPsocket.h"
#include <chrono>
#include <ctime>  

struct AudioSenderConnection {
	chrono::time_point<chrono::system_clock> timeUpdate;

	std::thread threadReader;
	bool isRunning;

	SharedMemoryReader sharedMemoryReader;
	AudioData audioData;
	AudioDataReader audioDataReader;

public:
	// data
	string name;
	int bufferSize;
	int sampleRate;
	int channels;
	int queueSize;

	moodycamel::ReaderWriterQueue<float> queue;
	bool isReady;

	AudioSenderConnection() {
		queueSize = 2;
	}

	~AudioSenderConnection() {
		close();
	}

	void init() {
		close();

		audioData.init(bufferSize * channels, queueSize);
#ifdef TARGET_WIN32
		sharedMemoryReader.init(name, 0, audioData.getSize());
#else 
		sharedMemoryReader.init("", stoi(name), audioData.getSize());
#endif

		isRunning = true;
		isReady = false;

		threadReader = std::thread([&]() {
			while (isRunning) {
				if (audioDataReader.readFromMemory(sharedMemoryReader, audioData, queue)) {
					isReady = true;
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
		});
		threadReader.detach();
	}

	void close() {
		if (isRunning) {
			isRunning = false;
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			sharedMemoryReader.close();
		}
	}
};

class AudioReceiver {
	UDPsocket socket;

    bool isRunning;
    
	std::mutex mutexForSocket;
	map<string, AudioSenderConnection*> audioSenderConnections;

public:

	~AudioReceiver() {
		close();
	}

	void init() {
		close();
		std::thread threadSocket([&]() {
			socket.open();
            if(socket.bind(PORT_MEMORYSHARING) == (int)UDPsocket::Status::OK) {
                UDPsocket::IPv4 ipaddr;
                std::string data;
                while (isRunning) {
                    size_t  dataSize = socket.recv(data, ipaddr);
                    if (!data.empty()) {
                        OSCPP::Server::Message msg(OSCPP::Server::Packet(data.c_str(), dataSize));
                        OSCPP::Server::ArgStream args(msg.args());
                        if (msg == "/memorySharing") {
                            const char* name = args.string();
                            
                            mutexForSocket.lock();
                            if (audioSenderConnections.find(name) != audioSenderConnections.end()) {
                                audioSenderConnections[name]->timeUpdate = std::chrono::system_clock::now();
                            }
                            else {
                                AudioSenderConnection* audioClientConnection = new AudioSenderConnection();
                                audioClientConnection->timeUpdate = std::chrono::system_clock::now();
                                
                                audioClientConnection->name = name;
                                audioClientConnection->bufferSize = args.int32();
                                audioClientConnection->sampleRate = args.int32();
								audioClientConnection->channels = args.int32();
								audioClientConnection->queueSize = args.int32();
								
                                audioClientConnection->init();
                                
                                audioSenderConnections[name] = audioClientConnection;
                                
                                cout << "created nameSharedMemory: " << name << endl;
                            }
                            mutexForSocket.unlock();
                        }
                    }
                }
            }
			
		});
        
        isRunning = true;
		threadSocket.detach();

	}

	void update() {
		// clear old clients
		chrono::time_point<chrono::system_clock> time = std::chrono::system_clock::now();

		for (auto it = audioSenderConnections.cbegin(), next_it = it; it != audioSenderConnections.cend(); it = next_it)
		{
			++next_it;
			std::chrono::duration<double> diff = time - it->second->timeUpdate;
			if (diff.count() > 1.0) {
				mutexForSocket.lock();
				it->second->close();
				audioSenderConnections.erase(it);
				mutexForSocket.unlock();
			}
		}
	}

	map<string, AudioSenderConnection*> getAudioClientConnections() {
		return audioSenderConnections;
	}

	void close() {
		if (isRunning) {
			isRunning = false;
			std::this_thread::sleep_for(std::chrono::milliseconds(10));

			for (auto it = audioSenderConnections.begin(); it != audioSenderConnections.end(); ++it) {
				it->second->close();
				delete it->second;
			}
			socket.close();
		}
	}
};

