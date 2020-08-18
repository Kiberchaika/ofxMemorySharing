#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup() {

	ofBackground(34, 34, 34);

	bufferSize = 1024;
	sampleRate = 48000;// 22050 * 22050;// 48000;// 44100;// 48000;
	
	audioReceiver.requiredBifferSizeForQueue = bufferSize;
	audioReceiver.requiredSampleRate = sampleRate;
	audioReceiver.init();

	lAudio.assign(bufferSize, 0.0);
	rAudio.assign(bufferSize, 0.0);

	soundStream.printDeviceList();
	ofSoundStreamSettings settings;

#ifdef TARGET_WIN32
	auto deviceList = soundStream.getDeviceList(ofSoundDevice::Api::MS_DS);
	settings.setOutDevice(deviceList[1]);

#else
	auto deviceList = soundStream.getMatchingDevices("default");
	for (auto &dev : deviceList) {
		if (dev.outputChannels == 2) {
			settings.setOutDevice(dev);
		}
	}
#endif

	settings.setOutListener(this);
	settings.sampleRate = sampleRate;
	settings.numOutputChannels = 2;
	settings.numInputChannels = 0;
	settings.bufferSize = bufferSize;
	soundStream.setup(settings);

	//required call
	gui.setup();
	ImGui::GetIO().MouseDrawCursor = false;

 
}


//--------------------------------------------------------------
void ofApp::update() {
	audioReceiver.update();

	std::map<std::string, AudioSenderConnection*> audioSenderConnections = audioReceiver.getAudioClientConnections();
	for (auto it = audioSenderConnections.begin(); it != audioSenderConnections.end(); it++) {
		std::string senderName = it->first;
		if (senderUsage.find(senderName) == senderUsage.end()) {
			senderUsage[senderName] = true;
			senderAvgVol[senderName] = 0;
		}
	}
}

//--------------------------------------------------------------
void ofApp::draw() {

	ofPushStyle();
	ofPushMatrix();
	{
		std::map<std::string, AudioSenderConnection*> audioSenderConnections = audioReceiver.getAudioClientConnections();
		for (auto it = audioSenderConnections.begin(); it != audioSenderConnections.end(); ++it) {
			std::string senderName = it->first;
			if (senderUsage.find(senderName) != senderUsage.end()) {
				if (senderUsage[senderName]) {
					ofDrawBitmapString(senderName, 10, 20);
					ofNoFill();
					ofDrawRectangle(160, 10, 2000 * senderAvgVol[senderName], 10);
					ofTranslate(0, 20);
				}
			}
		}
	}
	ofPopMatrix();
	ofPopStyle();

	gui.begin();
	{
		ImGui::Text("Senders:");
		std::map<std::string, AudioSenderConnection*> audioSenderConnections = audioReceiver.getAudioClientConnections();
		for (auto it = audioSenderConnections.begin(); it != audioSenderConnections.end(); it++) {
			std::string senderName = it->first;
			if (senderUsage.find(senderName) != senderUsage.end()) {
				ImGui::Checkbox(senderName.c_str(), &senderUsage[senderName]);
			}
		}
	}
	gui.end();
}

void ofApp::exit() {
    soundStream.stop();
	audioReceiver.close();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key) {
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key) {

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y) {
	
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button) {
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button) {
}


//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button) {
}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y) {

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y) {

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h) {

}

float phase = 0;


//--------------------------------------------------------------
void ofApp::audioOut(ofSoundBuffer & buffer) {

	// cleanup buffer
	for (int i = 0; i < bufferSize; i++) {
		buffer[i*buffer.getNumChannels()] = 0;
		buffer[i*buffer.getNumChannels() + 1] = 0;
	}

	/*
		// use sin wave for test
	while (phase > 10000 * TWO_PI) {
		phase -= 10000 * TWO_PI;
	}

	float phaseOrig = phase;
	for (int i = 0; i < buffer.getNumChannels(); i++) {
		phase = phaseOrig;
		for (int j = 0; j < bufferSize; j++) {
			phase += 0.025;
			buffer[j*buffer.getNumChannels()] = 0.1 * sin(phase);
			buffer[j*buffer.getNumChannels()+1] = 0.1 * sin(phase);
		}
	}
	return;
	*/

	std::map<std::string, AudioSenderConnection*> audioSenderConnections = audioReceiver.getAudioClientConnections();
	int cntActiveConnections = 0;
	for (auto it = audioSenderConnections.begin(); it != audioSenderConnections.end(); ++it) {
		float sample = 0;

		AudioSenderConnection* audioSenderConnection = it->second;
		std::string senderName = it->first;

		if (senderUsage.find(senderName) != senderUsage.end()) {
			if (audioSenderConnection->isReady) {
				if (senderUsage[senderName]) {

					float vol = 0;
					int size = audioSenderConnection->audioQueue.size_approx();
					//cout << ">> size: " << size << endl;
					if (size > audioSenderConnection->channels * bufferSize) {
						for (int j = 0; j < bufferSize; j++) {
							for (int i = 0; i < audioSenderConnection->channels; i++) {
								if (!audioSenderConnection->audioQueue.try_dequeue(sample)) {
									cout << "not valid" << endl;
									//valid = false;
									break;
								}

								// use only first input channel
								if (i < 2) {
									buffer[j*buffer.getNumChannels() + i] += sample;
									vol += fabs(sample);
								}
							}
						}

						
					}
					else {
						cout << "wait" << endl;
					}

					vol /= bufferSize;
					senderAvgVol[senderName] = vol;

					cntActiveConnections++;
				}
				else {
					// skip sender data ?
					for (int i = 0; i < audioSenderConnection->channels ; i++) {
						for (int j = 0; j < bufferSize; j++) {
							audioSenderConnection->audioQueue.try_dequeue(sample);
						}
					}
				}
			}
		}
	}

	for (int i = 0; i < channels; i++) {
		for (int j = 0; j < bufferSize; j++) {
			buffer[j*buffer.getNumChannels() + i] /= cntActiveConnections;
		}
	}
}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg) {

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo) {

}
