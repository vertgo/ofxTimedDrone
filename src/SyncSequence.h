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
};
#endif /* defined(__SyncedDroneAddon__SyncSequence__) */
