#pragma once

#include "ofMain.h"
#include "ofxImGui.h"

#include "AudioSender.h"
#include "AudioReceiver.h"

//#define AUDIO_SENDER

class ofApp : public ofBaseApp {

#ifdef AUDIO_SENDER
	AudioSender audioSender;
#else
	AudioReceiver audioReceiver;
#endif

	int bufferSize;
	int	sampleRate;
	int channels;


	ofxImGui::Gui gui;
	string selectedName;

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

	float 	pan;
	bool 	bNoise;
	float 	volume;

	vector <float> lAudio;
	vector <float> rAudio;

	//------------------- for the simple sine wave synthesis
	float 	targetFrequency;
	float 	phase;
	float 	phaseAdder;
	float 	phaseAdderTarget;
};
