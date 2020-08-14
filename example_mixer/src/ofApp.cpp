#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup() {

	ofBackground(34, 34, 34);

	bufferSize = 256;
	sampleRate = 44100;
	
	audioReceiver.requiredBifferSizeForQueue = bufferSize;
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

//--------------------------------------------------------------
void ofApp::audioOut(ofSoundBuffer & buffer) {

	// cleanup buffer
	for (int i = 0; i < bufferSize; i++) {
		buffer[i*buffer.getNumChannels()] = 0;
		buffer[i*buffer.getNumChannels() + 1] = 0;
	}

	std::map<std::string, AudioSenderConnection*> audioSenderConnections = audioReceiver.getAudioClientConnections();
	for (auto it = audioSenderConnections.begin(); it != audioSenderConnections.end(); ++it) {
		float sample = 0;

		AudioSenderConnection* audioSenderConnection = it->second;
		std::string senderName = it->first;

		if (senderUsage.find(senderName) != senderUsage.end()) {
			if (audioSenderConnection->isReady) {
				if (senderUsage[senderName]) {

					float vol = 0;

					if (audioSenderConnection->queue.size_approx() > audioSenderConnection->channels * bufferSize) {
						for (int i = 0; i < audioSenderConnection->channels; i++) {
							for (int j = 0; j < bufferSize; j++) {
								audioSenderConnection->queue.try_dequeue(sample);

								// use only first input channel
								if (i == 0) {
									buffer[j*buffer.getNumChannels()] += sample;
									buffer[j*buffer.getNumChannels() + 1] += sample;

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
				}
				else {
					// skip sender data
					for (int i = 0; i < bufferSize; i++) {
						audioSenderConnection->queue.try_dequeue(sample);
					}
				}
			}
		}

	}

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg) {

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo) {

}
