#pragma once

#include "SharedMemory.h"
#include "readerwriterqueue/readerwriterqueue.h"

const int PORT_MEMORYSHARING = 2040;

class AudioData {
public:
	int DATABUFFER_SIZE;
	int DATABUFFERS_COUNT;
	int n;
	vector<vector<float>> data;

	void init(int DATABUFFER_SIZE, int DATABUFFERS_COUNT) {
		n = 0;

		this->DATABUFFER_SIZE = DATABUFFER_SIZE;
		this->DATABUFFERS_COUNT = DATABUFFERS_COUNT;

		data.resize(DATABUFFERS_COUNT);
		for (size_t i = 0; i < data.size(); i++) {
			data[i].resize(DATABUFFER_SIZE);
		}
	}

	float* getDataPointer() {
        return n < data.size() ? data[n].data() : nullptr;
	}

	size_t getSize() {
		return (sizeof(float) * DATABUFFER_SIZE * DATABUFFERS_COUNT + 3 * sizeof(int));
	}
};

class AudioDataReader {
public:
	int idxRead = -1;
	vector<float> resampledData;


	bool readFromMemory(SharedMemoryReader& sharedMemoryReader, AudioData& audioData) {
		bool success = false;
		if (sharedMemoryReader.isOpened()) {
			sharedMemoryReader.update((char*)&(audioData.n), 2 * sizeof(int), sizeof(int));
			if (idxRead == -1 || idxRead != audioData.n) {
				idxRead = audioData.n;
				sharedMemoryReader.update((char*)(audioData.data[idxRead].data()), 3 * sizeof(int) + sizeof(float) * audioData.DATABUFFER_SIZE * idxRead, sizeof(float) * audioData.DATABUFFER_SIZE);

				success = true;
			}
		}
		return success;
	}

};

class AudioDataWriter {
public:
	int idxWrite = 0;

	bool writeToMemory(SharedMemoryWriter& sharedMemoryWriter, AudioData& audioData) {
		bool success = false;

		if (sharedMemoryWriter.isOpened()) {
			sharedMemoryWriter.update((char*)(audioData.data[idxWrite].data()), 3 * sizeof(int) + sizeof(float) * audioData.DATABUFFER_SIZE * idxWrite, sizeof(float) * audioData.DATABUFFER_SIZE);

			//cout << "write: " << n << endl;

			audioData.n = idxWrite;
			sharedMemoryWriter.update((char*)&(audioData.n), 2 * sizeof(int), sizeof(int));

			idxWrite = audioData.n + 1 >= audioData.DATABUFFERS_COUNT ? 0 : audioData.n + 1;

			success = true;
		}

		return success;
	}
};

