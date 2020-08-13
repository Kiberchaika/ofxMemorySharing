#pragma once

#if defined _WIN32 || defined _WIN64
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#elif defined __APPLE__
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <err.h>
#endif

#include <stdio.h>
#include <string>
#include <iostream>

class SharedMemoryBase {
protected:
#if defined _WIN32 || defined _WIN64
	HANDLE hMemory = INVALID_HANDLE_VALUE;
#elif defined __APPLE__
	int sharedMemId = -1;
#endif
	int sizeMemory = 0;
	char* buf = nullptr;

public:

	virtual bool init(std::string name, int key, int size) = 0;
	virtual void close() = 0;

	virtual void update(char* data) = 0;
	virtual void update(char* data, int offset, int size) = 0;
	virtual bool isOpened() = 0;
};

class SharedMemoryWriter : public SharedMemoryBase {
public:
	bool init(std::string name, int key, int size) override {
		this->sizeMemory = size;

#if defined _WIN32 || defined _WIN64
		// first check for existing
		hMemory = OpenFileMappingA(
			FILE_MAP_ALL_ACCESS,
			FALSE,
			name.c_str()
		);
		if (hMemory == NULL) {
			hMemory = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeMemory, name.c_str());
			if (hMemory == NULL) {
				std::cout << "Could not create file mapping object: " << GetLastError() << std::endl;
				return false;
			}
			buf = (char*)MapViewOfFile(hMemory, FILE_MAP_ALL_ACCESS, 0, 0, sizeMemory);
			if (buf == NULL) {
				std::cout << "Could not map view of file: " << GetLastError() << std::endl;
				CloseHandle(hMemory);
				return false;
			}
		} 
		else {
			std::cout << "Memory is already open" << std::endl;
			CloseHandle(hMemory);
			return false;
		}

#elif defined __APPLE__
		key_t k = ftok("/tmp/", key);
		sharedMemId = shmget(k, sizeMemory, IPC_CREAT | IPC_EXCL | 0666);
		if (sharedMemId == -1) {
			std::cout << "shmget error" << std::endl;
			return false;
		}
		buf = (char*)shmat(sharedMemId, NULL, 0);
		if (buf == MAP_FAILED) {
			std::cout << "shmat error" << std::endl;
			shmctl(sharedMemId, IPC_RMID, NULL);
			sharedMemId = -1;
			buf = nullptr;
			return false;
		}
#endif
		return true;
	}

	~SharedMemoryWriter() {
		close();
	}

	void update(char* data) override {
		if (isOpened()) {
			memcpy(buf, data, sizeMemory);
		}
	}

	void update(char* data, int offset, int size) override {
		if (isOpened()) {
			memcpy(buf + offset, data, size);
		}
	}

	bool isOpened() override {
		return buf != nullptr;
	}

	void close() override {
#if defined _WIN32 || defined _WIN64
		if (buf != nullptr) {
			UnmapViewOfFile(buf);
			buf = nullptr;
		}
		if (hMemory) {
			CloseHandle(hMemory);
			hMemory = NULL;
		}
#elif defined __APPLE__
		if (sharedMemId != -1) {
			shmctl(sharedMemId, IPC_RMID, NULL);
			sharedMemId = -1;
		}
		if (buf != nullptr) {
			buf = nullptr;
		}
#endif
	}
};

class SharedMemoryReader : public SharedMemoryBase {
public:
	bool init(std::string name, int key, int size) override {
		this->sizeMemory = size;

#if defined _WIN32 || defined _WIN64
		hMemory = OpenFileMappingA(
			FILE_MAP_ALL_ACCESS,
			FALSE,
			name.c_str()
		);
		if (hMemory == NULL) {
			std::cout << "Could not open file mapping object: " << GetLastError() << std::endl;
			return false;
		}

		buf = (char*)MapViewOfFile(hMemory, FILE_MAP_ALL_ACCESS, 0, 0, sizeMemory);
		if (buf == NULL) {
			std::cout << "Could not map view of file: " << GetLastError() << std::endl;
			CloseHandle(hMemory);
			return false;
		}
#elif defined __APPLE__
		key_t k = ftok("/tmp/", key);
		sharedMemId = shmget(k, sizeMemory, 0666);
		if (sharedMemId == -1) {
			std::cout << "shmget error" << std::endl;
			return false;
		}
		buf = (char*)shmat(sharedMemId, NULL, 0);
		if (buf == MAP_FAILED) {
			std::cout << "shmat error" << std::endl;
			shmctl(sharedMemId, IPC_RMID, NULL);
			sharedMemId = -1;
			buf = nullptr;
			return false;
		}
#endif
		return true;
	}

	~SharedMemoryReader() {
		close();
	}

	void update(char* data) override {
		if (isOpened()) {
			memcpy(data, buf, sizeMemory);
		}
	}

	void update(char* data, int offset, int size) override {
		if (isOpened()) {
			memcpy(data, buf + offset, size);
		}
	}

	bool isOpened() override {
		return buf != nullptr;
	}

	void close() override {
#if defined _WIN32 || defined _WIN64
		if (buf != nullptr) {
			UnmapViewOfFile(buf);
			buf = nullptr;
		}
		if (hMemory) {
			CloseHandle(hMemory);
			hMemory = NULL;
		}
#elif defined __APPLE__
		if (sharedMemId != -1) {
			//shmctl(sharedMemId, IPC_RMID, NULL);
			sharedMemId = -1;
		}
		if (buf != nullptr) {
			buf = nullptr;
		}
#endif
	}
};


