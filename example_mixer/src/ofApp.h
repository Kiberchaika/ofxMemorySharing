#pragma once

#include "ofMain.h"
#include "ofxImGui.h"
#include "ofxRTTR.h"

#include "AudioReceiver.h"

#include "TypesForDataExchange.h"


class ofApp : public ofBaseApp {

	std::map<AudioSenderConnection*, PANNER_SETTINGS> mapAudioConnectionData;

	AudioReceiver audioReceiver;

	int bufferSize;
	int	sampleRate;
	int channels;

	std::map<std::string, bool> senderUsage;
	std::map<std::string, float> senderAvgVol;

	ofxImGui::Gui gui;

public:

	void setup();
	void update();
	void draw();
	void exit();

	void keyPressed(int key);
	void keyReleased(int key);
	void mouseMoved(int x, int y);
	void mouseDragged(int x, int y, int button);
	void mousePressed(int x, int y, int button);
	void mouseReleased(int x, int y, int button);
	void mouseEntered(int x, int y);
	void mouseExited(int x, int y);
	void windowResized(int w, int h);
	void dragEvent(ofDragInfo dragInfo);
	void gotMessage(ofMessage msg);

	void audioOut(ofSoundBuffer & buffer);

	ofSoundStream soundStream;

	vector <float> lAudio;
	vector <float> rAudio;
};
