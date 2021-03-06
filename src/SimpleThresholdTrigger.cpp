//
//  SimpleThresholdTrigger.cpp
//  WebcamInstruments
//
//  Created by Tim Murray-Browne on 20/11/2012.
//
//

#include "SimpleThresholdTrigger.h"

SimpleThresholdTrigger::SimpleThresholdTrigger(ofxMidiOut* midiOutput, int pitch, cv::Rect const& location)
{
	mMidiOutput = midiOutput;
	mMidiPitch = pitch;
	mThreshold = 0.2;
	mPreviousMovement = 0.;
	mNoteIsPlaying = false;
	mLocation = location;
	mTimeNoteStarted = 0.f;
	mNoteVelocity = 0;
}

SimpleThresholdTrigger::~SimpleThresholdTrigger()
{
	flush();
}

void SimpleThresholdTrigger::flush()
{
	if (mNoteIsPlaying)
	{
		// send note off by sending not with zero velocity.
		mMidiOutput->sendNoteOn(0, mMidiPitch, 0);
	}
}


void SimpleThresholdTrigger::update(cv::Mat const& differenceImage)
{
	// First we set a 'region-of-interest' within the input image
	cv::Mat region = differenceImage(mLocation);
	
	// take an average over the data. Note that CV is using pixel values that are integers
	// between 0 and 255 but we convert this to a float between 0. and 1.
	// as it makes the maths simpler.
	cv::Scalar meanPerChannel = cv::mean(region); // This gives us the average in (R,G,B) individually
	float overallMean = (meanPerChannel[0] + meanPerChannel[1] + meanPerChannel[2])/3.;
	float movement = overallMean / 255; // convert range from 0-255 to 0.-1.
	// A little bit of fiddling to exaggerate the value of movement
	movement = 1. - pow((1. - movement),3);
	float amountAboveThreshold = movement - mThreshold;
	float amountAboveThresholdPreviously = mPreviousMovement - mThreshold;
	// We normalise these values so that positive values lie between 0. and 1.
	float rangeAvailableAboveThreshold = 1. - mThreshold;
	movement = movement / rangeAvailableAboveThreshold;
	amountAboveThreshold = amountAboveThreshold / rangeAvailableAboveThreshold;
	// If the average has just gone above the threshold then we
	// trigger a note. Once we go below the threshold then we
	// stop the note.
	if (amountAboveThreshold > 0 && amountAboveThresholdPreviously < 0)
	{
		// To determine the velocity of the note we consider the range
		// of possible values that could be above the threshold
		float velocityRange = 1. - mThreshold;
		// then work out how far through that range our note is
		float velocity = amountAboveThreshold / velocityRange;
		// our values range between 0 and 1, but MIDI uses 0-127 integers
		int velocityInt = (int) (velocity * 127 + .5);
		// Just in case we've gone outside of MIDI's range:
		velocityInt = max(0, velocityInt);
		velocityInt = min(127, velocityInt);
		mMidiOutput->sendNoteOn(1, mMidiPitch, velocityInt);
		mNoteIsPlaying = true;
		// For visual effects - we remember when the note started and its velocity
		mTimeNoteStarted = ofGetElapsedTimef();
		mNoteVelocity = velocity;
	}
	else if (amountAboveThreshold < 0 && mNoteIsPlaying)
	{
		// Stop a note by sending velocity as 0
		mMidiOutput->sendNoteOn(1, mMidiPitch, 0);
		mNoteIsPlaying = false;
	}
	// Remember the current movement for the next frame
	mPreviousMovement = movement;
}

void SimpleThresholdTrigger::draw()
{
	ofVec2f center = ofVec2f(mLocation.x + mLocation.width / 2.,
							 mLocation.y + mLocation.height / 2.);
	// Draw the threshold as a light grey cricle
	ofSetColor(180, 180, 180, 64);
	float locationRadius = min(mLocation.width, mLocation.height) / 2.;
	ofCircle(center, mThreshold * locationRadius);
	// Then draw the movement amount as a colour circle growing out of it
	int red = (int)(mPreviousMovement * 244.);
	int green = (int)(mPreviousMovement * 10. + 30);
	int blue = max(0,(int)((.4 - .1*mPreviousMovement) * 43.)+130);
	ofColor circleColor = ofColor(red, green, blue, 255);
	ofSetColor(circleColor);
	ofCircle(center, mPreviousMovement * locationRadius);
	
	// Some small visual effect of a ring moving outwards
//	if (mNoteIsPlaying)
	{
		float t = ofGetElapsedTimef() - mTimeNoteStarted;
		float circleLifetime = 5.f;
		// to prevent artefacts, we add a little bit onto the lifetime
		// based on which circle we're in.
		// Seed the random number generator so that each trigger always gets the same number each frame
		ofSeedRandom(int(1000. * mLocation.x) + int(1000000. * mLocation.y) + 2367);
		circleLifetime = circleLifetime + ofRandom(0.f, 3.f);
		if (t<circleLifetime)
		{
			t = t/circleLifetime; // scale t to between 0 and 1
			t = ofClamp(t, 0., 1.);
			// Fade to white over 5 seconds
			circleColor.lerp(ofColor::white, pow(t*.7f+.3f, .6f));
			// draw the circle the at threshold size when the note is first plaing
			// and a larger size when it stops playing, fading out
			float radius = mThreshold * locationRadius * (pow(t, .25f) * 20. + 1.);
			float opacity = pow(1.f - t, 3.f) * .4;
			circleColor.a = opacity * 255;
			ofSetColor(circleColor);
//			glColor4f(circleColor.r/255., circleColor.g/255., circleColor.b/255., opacity);
			ofCircle(center, radius);
		}
	}
}
