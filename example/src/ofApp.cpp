#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup() {

	ofBackground(34, 34, 34);

	bufferSize = 512;
	sampleRate = 44100;
	phase = 0;
	phaseAdder = 0.0f;
	phaseAdderTarget = 0.0f;
	volume = 0.1f;
	bNoise = false;
	channels = 1;

#ifdef AUDIO_SENDER
	audioSender.bufferSize = bufferSize;
	audioSender.channels = channels;
	audioSender.queueSize = 3; // bufferSize * queueSize < must be over than receiver buffer
	audioSender.init();
#else
	audioReceiver.init();
#endif

	lAudio.assign(bufferSize, 0.0);
	rAudio.assign(bufferSize, 0.0);

	soundStream.printDeviceList();
	ofSoundStreamSettings settings;

	// if you want to set the device id to be different than the default:
	//
	//    auto devices = soundStream.getDeviceList();
	//    settings.setOutDevice(devices[3]);

	// you can also get devices for an specific api:
	//
	//    auto devices = soundStream.getDeviceList(ofSoundDevice::Api::PULSE);
	//    settings.setOutDevice(devices[0]);

	// or get the default device for an specific api:
	//
	// settings.api = ofSoundDevice::Api::PULSE;

	// or by name:
	//
	//    auto devices = soundStream.getMatchingDevices("default");
	//    if(!devices.empty()){
	//        settings.setOutDevice(devices[0]);
	//    }

#ifdef TARGET_LINUX
	// Latest linux versions default to the HDMI output
	// this usually fixes that. Also check the list of available
	// devices if sound doesn't work
	auto devices = soundStream.getMatchingDevices("default");
	if (!devices.empty()) {
		settings.setOutDevice(devices[0]);
	}
#endif

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
#ifdef AUDIO_SENDER
	audioSender.update();
#else
	audioReceiver.update();
#endif
}

//--------------------------------------------------------------
void ofApp::draw() {

	ofSetColor(225);
	ofDrawBitmapString("AUDIO OUTPUT EXAMPLE", 32, 32);
	ofDrawBitmapString("press 's' to unpause the audio\npress 'e' to pause the audio", 31, 92);

	ofNoFill();

	// draw the left channel:
	ofPushStyle();
	ofPushMatrix();
	ofTranslate(32, 150, 0);

	ofSetColor(225);
	ofDrawBitmapString("Left Channel", 4, 18);

	ofSetLineWidth(1);
	ofDrawRectangle(0, 0, 900, 200);

	ofSetColor(245, 58, 135);
	ofSetLineWidth(3);

	ofBeginShape();
	for (unsigned int i = 0; i < lAudio.size(); i++) {
		float x = ofMap(i, 0, lAudio.size(), 0, 900, true);
		ofVertex(x, 100 - lAudio[i] * 180.0f);
	}
	ofEndShape(false);

	ofPopMatrix();
	ofPopStyle();

	// draw the right channel:
	ofPushStyle();
	ofPushMatrix();
	ofTranslate(32, 350, 0);

	ofSetColor(225);
	ofDrawBitmapString("Right Channel", 4, 18);

	ofSetLineWidth(1);
	ofDrawRectangle(0, 0, 900, 200);

	ofSetColor(245, 58, 135);
	ofSetLineWidth(3);

	ofBeginShape();
	for (unsigned int i = 0; i < rAudio.size(); i++) {
		float x = ofMap(i, 0, rAudio.size(), 0, 900, true);
		ofVertex(x, 100 - rAudio[i] * 180.0f);
	}
	ofEndShape(false);

	ofPopMatrix();
	ofPopStyle();


	ofSetColor(225);
	string reportString = "volume: (" + ofToString(volume, 2) + ") modify with -/+ keys\npan: (" + ofToString(pan, 2) + ") modify with mouse x\nsynthesis: ";
	if (!bNoise) {
		reportString += "sine wave (" + ofToString(targetFrequency, 2) + "hz) modify with mouse y";
	}
	else {
		reportString += "noise";
	}
	ofDrawBitmapString(reportString, 32, 579);


#ifndef AUDIO_SENDER
	gui.begin();
	{
		ImGui::Text("Hello, world!");

		map<string, AudioSenderConnection*> audioSenderConnections = audioReceiver.getAudioClientConnections();
		for (auto it = audioSenderConnections.begin(); it != audioSenderConnections.end(); it++) {
			if (ImGui::Button(it->first.c_str()))
			{
				selectedName = it->first;
			}
		}
		
	}
	gui.end();
#endif
}

void ofApp::exit() {
    soundStream.stop();

#ifdef AUDIO_SENDER
	audioSender.close();
#else
	audioReceiver.close();
#endif
}


//--------------------------------------------------------------
void ofApp::keyPressed(int key) {
	if (key == '-' || key == '_') {
		volume -= 0.05;
		volume = MAX(volume, 0);
	}
	else if (key == '+' || key == '=') {
		volume += 0.05;
		volume = MIN(volume, 1);
	}

	if (key == 's') {
		soundStream.start();
	}

	if (key == 'e') {
		soundStream.stop();
	}

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key) {

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y) {
	int width = ofGetWidth();
	pan = (float)x / (float)width;
	float height = (float)ofGetHeight();
	float heightPct = ((height - y) / height);
	targetFrequency = 2000.0f * heightPct;
	phaseAdderTarget = (targetFrequency / (float)sampleRate) * TWO_PI;
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button) {
	int width = ofGetWidth();
	pan = (float)x / (float)width;
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button) {
	bNoise = true;
}


//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button) {
	bNoise = false;
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
	//pan = 0.5f;
	float leftScale = 1 - pan;
	float rightScale = pan;

	// sin (n) seems to have trouble when n is very large, so we
	// keep phase in the range of 0-TWO_PI like this:
	while (phase > 1000 * TWO_PI) {
		phase -= 1000 * TWO_PI;
	}

#ifdef AUDIO_SENDER
	if (bNoise == true) {
		// ---------------------- noise --------------
		for (size_t i = 0; i < buffer.getNumFrames(); i++) {
			lAudio[i] = ofRandom(0, 1) * volume * leftScale;
			rAudio[i] = ofRandom(0, 1) * volume * rightScale;
		}
	}
	else {
		phaseAdder = 0.95f * phaseAdder + 0.05f * phaseAdderTarget;
		for (size_t i = 0; i < buffer.getNumFrames(); i++) {
			phase += phaseAdder;
			float sample = sin(phase);
			lAudio[i] = sample * volume * leftScale;
			rAudio[i] = sample * volume * rightScale;
		}
	}

	// write
	float* dataWrite = audioSender.getDataPointer();
	for (int i = 0; i < bufferSize; i++) {
		dataWrite[i] = lAudio[i];
	}
	audioSender.writeData();
#else 

	map<string, AudioSenderConnection*> audioSenderConnections = audioReceiver.getAudioClientConnections();
	for (auto it = audioSenderConnections.begin(); it != audioSenderConnections.end(); ++it) {
		float sample = 0;

		AudioSenderConnection* audioSenderConnection = it->second;
		if (audioSenderConnection->isReady) {
			if (it->first == selectedName) {
				for (int i = 0; i < audioSenderConnection->channels; i++) {
					for (int j = 0; j < bufferSize; j++) {
						audioSenderConnection->queue.try_dequeue(sample);
						// use only first channel for output
						if (i == 0) {
							buffer[j*buffer.getNumChannels()] = sample;
							buffer[j*buffer.getNumChannels() + 1] = sample;
						}
					}
				}
			}
			else {
				for (int i = 0; i < bufferSize; i++) {
					audioSenderConnection->queue.try_dequeue(sample);
				}
			}
		}
	}
#endif
}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg) {

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo) {

}
