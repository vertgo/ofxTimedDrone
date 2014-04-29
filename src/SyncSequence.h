//
//  SyncSequence.h
//  SyncedDroneAddon
//
//  Created by Michael Manh on 4/28/14.
//
//
#pragma once
#ifndef __SyncedDroneAddon__SyncSequence__
#define __SyncedDroneAddon__SyncSequence__

#include "ofMain.h"
#include "FireEvent.h"
#include "ofxAVFVideoPlayer.h"
#include "ofxThreadedVideoPlayer.h"
#include "SyncedOFVideoPlayer.h"

enum PlayerType{
    QTKIT,
    AVF,
    THREADED_AVF
};

class GlobalThreadedVids{
public:
    static int numLoadedThreadedVids;
    static vector< ofxThreadedVideoPlayer*> players; //don't hate
    
};


//vector< ofxThreadedVideoPlayer*> GlobalThreadedVids::players;

class SyncSequence{
    
    
public:
    SyncSequence( string inType );
    
    string type;
    vector< SyncedOFVideoPlayer*> qtVideoPlayers;  //vector of all the vids for qtkit
    vector< ofxAVFVideoPlayer*> avfVideoPlayers;  //vector of all the vids for avf
    vector< ofxThreadedVideoPlayer*> threadedVideoPlayers;  //vector of all the vids for threaded
    
    vector< FireEvent* > droneEventList; //list of fire events ordered in sequence
    vector<uint> vidStartTimes; //kind of inelegant, corresponds to the videos, sets their start time
    
    void sortList();
    
    void parseFromJson( ofxJSONElement inNode, PlayerType inType );
    void parseVidNode( ofxJSONElement inNode ); //parses a single video config node, making all the necessary firevents to start and stop a video
    
    PlayerType playerType;
    void videoIsReadyCallback(ofxThreadedVideoPlayerStatus &status);
    
    int numThreadedVidsReady;
    void parseDroneDuino(ofxJSONElement inNode);
    
    
};
#endif /* defined(__SyncedDroneAddon__SyncSequence__) */
