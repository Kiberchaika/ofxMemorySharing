#pragma once

#include "SharedMemory.h"
#include "readerwriterqueue/readerwriterqueue.h"

const int PORT_MEMORYSHARING = 2020;

class AudioData {
public:
	int BUFFER_SIZE;
	int BUFFERS_COUNT;
	int n;
	vector<vector<float>> data;

	void init(int BUFFER_SIZE, int BUFFERS_COUNT) {
		n = 0;

		this->BUFFER_SIZE = BUFFER_SIZE;
		this->BUFFERS_COUNT = BUFFERS_COUNT;

		data.resize(BUFFERS_COUNT);
		for (size_t i = 0; i < data.size(); i++) {
			data[i].resize(BUFFER_SIZE);
		}
	}

	float* getDataPointer() {
		return data[n].data();
	}

	size_t getSize() {
		return (sizeof(float) * BUFFER_SIZE * BUFFERS_COUNT + 3 * sizeof(int));
	}
};

class AudioDataReader {
public:
	int idxRead = -1;

	bool readFromMemory(SharedMemoryReader& sharedMemoryReader, AudioData& audioData, moodycamel::ReaderWriterQueue<float>& queue, int requiredBifferSizeForQueue) {
		bool success = false;
		if (sharedMemoryReader.isOpened()) {
			sharedMemoryReader.update((char*)&(audioData.n), 2 * sizeof(int), sizeof(int));
			if (idxRead == -1 || idxRead != audioData.n) {
				idxRead = audioData.n;
				sharedMemoryReader.update((char*)(audioData.data[idxRead].data()), 3 * sizeof(int) + sizeof(float) * audioData.BUFFER_SIZE * idxRead, sizeof(float) * audioData.BUFFER_SIZE);

				int size = queue.size_approx();
				if (size > requiredBifferSizeForQueue && size > audioData.BUFFER_SIZE * (audioData.BUFFERS_COUNT+1)) {
					queue = moodycamel::ReaderWriterQueue<float>();
				}

				for (int i = 0; i < audioData.BUFFER_SIZE; i++) {
					queue.enqueue(audioData.data[idxRead][i]);
				}
			}
			success = true;
		}
		
		return success;
	}

};

class AudioDataWriter {
public:
	int idxWrite = 0;

	bool writeToMemory(SharedMemoryWriter& sharedMemoryWriter, AudioData& audioData) {
		if (sharedMemoryWriter.isOpened()) {
			sharedMemoryWriter.update((char*)(audioData.data[idxWrite].data()), 3 * sizeof(int) + sizeof(float) * audioData.BUFFER_SIZE * idxWrite, sizeof(float) * audioData.BUFFER_SIZE);

			//cout << "write: " << n << endl;

			audioData.n = idxWrite;
			sharedMemoryWriter.update((char*)&(audioData.n), 2 * sizeof(int), sizeof(int));

			idxWrite = audioData.n + 1 >= audioData.BUFFERS_COUNT ? 0 : audioData.n + 1;

			return true;
		}

		return false;
	}
};

