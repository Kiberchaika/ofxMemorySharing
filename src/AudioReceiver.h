#pragma once

#include "AudioData.h"
#include "SharedMemory.h"

#include "oscpp/server.hpp"
#include "oscpp/print.hpp"
#include "UDPsocket.h"

#include "SpeexResampler.h"

#include <thread>
#include <chrono>
#include <ctime>  

struct AudioSenderConnection {
	chrono::time_point<chrono::system_clock> timeUpdate;

	std::thread threadReader;
	bool isRunning;

	SharedMemoryReader sharedMemoryReader;
	AudioData audioData;
	AudioDataReader audioDataReader;

	speexport::SpeexResampler speexResampler;

	vector<float> data;
	vector<float> resampledData;
	int resampledBufferSize;

	std::thread threadSocket;

	std::function<void(AudioSenderConnection*, std::string)> callbackReceiveData;

	UDPsocket socket;

public:
	// data
	string nameSharedMemory;
	string name;
	int bufferSize;
	int sampleRate;
	int channels;
	int memoryQueueSize;
	int portReceive = -1;
	int portSend = -1;

	int requiredBufferSizeForQueue;
	int requiredSampleRate;

	moodycamel::ReaderWriterQueue<float> audioQueue;
	bool isReady;


	AudioSenderConnection() {
		memoryQueueSize = 2;
	}

	~AudioSenderConnection() {
		close();
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
		if (socket.send("port:" + std::to_string(portReceive), UDPsocket::IPv4::Loopback(portSend)) == (int)UDPsocket::Status::SendError) {
			std::cout << "socket send error" << std::endl;
		}

		int err = 0;
		speexResampler.init(channels, sampleRate, requiredSampleRate, 8, &err);

		resampledBufferSize = (1.0 * bufferSize * requiredSampleRate / sampleRate);
		resampledData.resize(resampledBufferSize * channels);

		audioData.init(bufferSize * channels, memoryQueueSize);
#ifdef TARGET_WIN32
		sharedMemoryReader.init(nameSharedMemory, 0, audioData.getSize());
#else 
		sharedMemoryReader.init("", std::stoi(nameSharedMemory), audioData.getSize());
#endif

		isRunning = true;
		isReady = false;

		threadSocket = std::thread([&]() {
			UDPsocket::IPv4 ipaddr;
			std::string receivedString;

			while (isRunning) {
				size_t dataSize = socket.recv(receivedString, ipaddr);
				if (!receivedString.empty()) {
					if (callbackReceiveData) callbackReceiveData(this, receivedString);
				}
			}
		});
		threadSocket.detach();

		threadReader = std::thread([&]() {
			while (isRunning) {
				if (audioDataReader.readFromMemory(sharedMemoryReader, audioData)) {
					
					// resampling
					for (int c = 0; c < channels; c++) {
						unsigned int in_len = bufferSize;
						unsigned int out_len = resampledBufferSize;
						speexResampler.process(c, &audioData.data[audioDataReader.idxRead][c * bufferSize], &in_len, &resampledData[c * resampledBufferSize], &out_len);
					}
 
					int size = audioQueue.size_approx();
					if (size > 2 * requiredBufferSizeForQueue * channels && size > 2 * audioData.DATABUFFER_SIZE * audioData.DATABUFFERS_COUNT) {
						audioQueue = moodycamel::ReaderWriterQueue<float>();
					}

					/*
					for (int i = 0; i < bufferSize; i++) {
						for (int c = 0; c < channels; c++) {
							if (!audioQueue.enqueue(audioData.data[audioDataReader.idxRead][c * bufferSize + i])) {
								cout << "error" << endl;
							}
						}
					}
					*/

					//audioData.data[audioDataReader.idxRead][i + c * resampledBufferSize]

					for (int i = 0; i < resampledBufferSize; i++) {
						for (int c = 0; c < channels; c++) {
							if (!audioQueue.enqueue(resampledData[i + c * resampledBufferSize])) {
								std::cout << "error" << std::endl;
							}
						}
					}
					
					//std::this_thread::sleep_for(std::chrono::milliseconds(1));
					isReady = true;
				}
			}
		});
		threadReader.detach();
	}

	void sendData(std::string str) {
		if (portSend < 0 || socket.send(str, UDPsocket::IPv4::Loopback(portSend)) == (int)UDPsocket::Status::SendError) {
			std::cout << "socket send error" << std::endl;
		}
	}

	void close() {
		if (isRunning) {
			isRunning = false;
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			sharedMemoryReader.close();
			socket.close();
		}
	}
};

class AudioReceiver {
	UDPsocket socket;

    bool isRunning;
    
	std::mutex mutexForSocket;
	std::map<string, AudioSenderConnection*> audioSenderConnections;

public:

	int requiredBufferSizeForQueue = 512;
	int requiredSampleRate = 44100;
	std::function<void(AudioSenderConnection*, std::string)> callbackReceiveData;

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
                            const char* nameSharedMemory = args.string();
                            
                            mutexForSocket.lock();
                            if (audioSenderConnections.find(nameSharedMemory) != audioSenderConnections.end()) {
                                audioSenderConnections[nameSharedMemory]->timeUpdate = std::chrono::system_clock::now();
                            }
                            else {
                                AudioSenderConnection* audioClientConnection = new AudioSenderConnection();
                                audioClientConnection->timeUpdate = std::chrono::system_clock::now();
                                
                                audioClientConnection->nameSharedMemory = nameSharedMemory;
								audioClientConnection->name = args.string();
								audioClientConnection->bufferSize = args.int32();
								audioClientConnection->sampleRate = args.int32();
								audioClientConnection->channels = args.int32();
								audioClientConnection->memoryQueueSize = args.int32();
								audioClientConnection->portSend = args.int32();

								audioClientConnection->requiredBufferSizeForQueue = requiredBufferSizeForQueue;
								audioClientConnection->requiredSampleRate = requiredSampleRate;
								audioClientConnection->callbackReceiveData = callbackReceiveData;

                                audioClientConnection->init();
                                
                                audioSenderConnections[nameSharedMemory] = audioClientConnection;
                                
                                cout << "created nameSharedMemory: " << nameSharedMemory << endl;
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
		std::chrono::time_point<std::chrono::system_clock> time = std::chrono::system_clock::now();

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

	std::map<string, AudioSenderConnection*> getAudioClientConnections() {
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

