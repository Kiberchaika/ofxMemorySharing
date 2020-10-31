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
    char* buf = nullptr;
#elif defined __APPLE__
	vector<int> sharedMemIds;
    vector<char*> bufs;

    void clear() {
        sharedMemIds.clear();
        bufs.clear();
    }
#endif
	int sizeMemory = 0;

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
        
        clear();
        
        int cnt = ceil(1.0 * sizeMemory / getpagesize());
        for(int i = 0; i < cnt; i++) {
            key_t k = ftok("/tmp/", key + i);
            int sharedMemId = shmget(k, getpagesize(), IPC_CREAT | IPC_EXCL | 0666);
            if (sharedMemId == -1) {
                std::cout << "shmget error" << std::endl;
                
                clear();
                
                return false;
            }
            sharedMemIds.push_back(sharedMemId);

            char* buf = (char*)shmat(sharedMemId, NULL, 0);
            if (buf == MAP_FAILED) {
                std::cout << "shmat error" << std::endl;
                shmctl(sharedMemId, IPC_RMID, NULL);
                sharedMemId = -1;
                buf = nullptr;
                
                clear();
                
                return false;
            }
            bufs.push_back(buf);
        }
        
#endif
		return true;
	}

	~SharedMemoryWriter() {
		close();
	}

	void update(char* data) override {
		if (isOpened()) {
#if defined _WIN32 || defined _WIN64
			memcpy(buf, data, sizeMemory);
#elif defined __APPLE__
            for(int i = 0; i < bufs.size(); i++) {
                int s = sizeMemory - i * getpagesize();
                memcpy(bufs[i], data + i * getpagesize(), s < getpagesize() ? s : getpagesize());
            }
#endif
		}
	}

	void update(char* data, int offset, int size) override {
		if (isOpened()) {
#if defined _WIN32 || defined _WIN64
            memcpy(buf + offset, data, size);
#elif defined __APPLE__
            int bufIdx = floor(1.0 * offset / getpagesize());
            int bufOffset = offset - bufIdx * getpagesize();
            int useOnePage = getpagesize() - bufOffset > size;
            int startSize = useOnePage ? size : getpagesize() - bufOffset;
            int remainingSize = size - startSize;
            memcpy(bufs[bufIdx] + bufOffset, data, startSize);
            
            int i = bufIdx + 1;
            while(remainingSize > 0) {
                memcpy(bufs[i], data + startSize, remainingSize < getpagesize() ? remainingSize: getpagesize());
                remainingSize -= getpagesize();
                startSize += getpagesize();
                i++;
           }
#endif
		}
	}

	bool isOpened() override {
#if defined _WIN32 || defined _WIN64
        return buf != nullptr;
#elif defined __APPLE__
        return bufs.size() > 0;
#endif
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
        for(int i = 0; i < sharedMemIds.size(); i++) {
            if (sharedMemIds[i] != -1) {
                shmctl(sharedMemIds[i], IPC_RMID, NULL);
            }
        }
        clear();
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
        
        clear();
        
        int cnt = ceil(1.0 * sizeMemory / getpagesize());
        for(int i = 0; i < cnt; i++) {
            key_t k = ftok("/tmp/", key + i);
            int sharedMemId = shmget(k, sizeMemory, 0666);
            if (sharedMemId == -1) {
                std::cout << "shmget error" << std::endl;
                
                clear();
 
                return false;
            }
            sharedMemIds.push_back(sharedMemId);
            
            char* buf = (char*)shmat(sharedMemId, NULL, 0);
            if (buf == MAP_FAILED) {
                std::cout << "shmat error" << std::endl;
                shmctl(sharedMemId, IPC_RMID, NULL);
                sharedMemId = -1;
                buf = nullptr;
                
                clear();
                
                return false;
            }
            bufs.push_back(buf);
        }
#endif
		return true;
	}

	~SharedMemoryReader() {
		close();
	}

	void update(char* data) override {
		if (isOpened()) {
#if defined _WIN32 || defined _WIN64
            memcpy(data, buf, sizeMemory);
#elif defined __APPLE__
            for(int i = 0; i < bufs.size(); i++) {
                int s = sizeMemory - i * getpagesize();
                memcpy(data + i * getpagesize(), bufs[i], s < getpagesize() ? s : getpagesize());
            }
#endif
		}
	}

	void update(char* data, int offset, int size) override {
		if (isOpened()) {
#if defined _WIN32 || defined _WIN64
            memcpy(data, buf + offset, size);
#elif defined __APPLE__
            int bufIdx = floor(1.0 * offset / getpagesize());
            int bufOffset = offset - bufIdx * getpagesize();
            int useOnePage = getpagesize() - bufOffset > size;
            int startSize = useOnePage ? size : getpagesize() - bufOffset;
            int remainingSize = size - startSize;
            memcpy(data, bufs[bufIdx] + bufOffset, startSize);
            
            int i = bufIdx + 1;
            while(remainingSize > 0) {
                memcpy(data + startSize, bufs[i], remainingSize < getpagesize() ? remainingSize: getpagesize());
                remainingSize -= getpagesize();
                startSize += getpagesize();
                i++;
            }

#endif
		}
	}

	bool isOpened() override {
#if defined _WIN32 || defined _WIN64
        return buf != nullptr;
#elif defined __APPLE__
        return bufs.size() > 0;
#endif
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
        for(int i = 0; i < sharedMemIds.size(); i++) {
            if (sharedMemIds[i] != -1) {
                //shmctl(sharedMemId, IPC_RMID, NULL);
            }
        }
        clear();
#endif
	}
};


