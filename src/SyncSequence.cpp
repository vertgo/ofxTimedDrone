//
//  SyncSequence.cpp
//  SyncedDroneAddon
//
//  Created by Michael Manh on 4/28/14.
//
//

#include "SyncSequence.h"
int GlobalThreadedVids::numLoadedThreadedVids = 0;
vector< ofxThreadedVideoPlayer*> GlobalThreadedVids::players; //don't hate

SyncSequence::SyncSequence( string inType){
    numThreadedVidsReady = 0;
    type = inType;
}

//used for sorting events
bool isEventEarlier(FireEvent* i,FireEvent* j) { return (i->fireTime < j->fireTime); }


void SyncSequence::sortList(){
    sort( droneEventList.begin(), droneEventList.end(), isEventEarlier);
    cout << "SyncSequence::sortList::fireTimes:";
    for( int i = 0; i < droneEventList.size();i++ ){
        
        cout << droneEventList[ i ]->fireTime << ",";
    }
    cout <<endl;
}

void SyncSequence::parseFromJson( ofxJSONElement inNode, PlayerType inPlayerType ){

    playerType = inPlayerType;
    ofxJSONElement videoNodes = inNode["videos"];
    ofxJSONElement arduinoNodes = inNode["arduinos"];

    
    int numVideos = videoNodes.size();
    
    for( int i = 0; i < numVideos; i++ ){
        ofxJSONElement curVidNode = videoNodes[ i];
        parseVidNode(curVidNode);
        cout << "parsing video";
        
    }
    
    int numArduinos = arduinoNodes.size();
    for( int i = 0; i < numArduinos; i++ ){
        parseDroneDuino( arduinoNodes[i] );
        cout << "parsing arduino";
        
    }
    
    
    cout << "numVideos:" <<numVideos << ", numArduinos:"<< numArduinos <<endl;
    
    
    sortList();
}

//--------------------------------------------------------------
void SyncSequence::parseDroneDuino(ofxJSONElement inNode){
    //string arduinoName = inNode[ "arduino"].asString();
     

     
     //now make an event for the arduino to fire
     FireEvent* arduinoFire = new FireEvent( "fireArduino");
     arduinoFire->fireTime = inNode[ "fireTime"].asFloat() * 1000;
     droneEventList.push_back(arduinoFire);
    arduinoFire->arduinoName = inNode["arduino"].asString();
    
     if (inNode[ "wipeTime"].type() == Json::nullValue){
     cout << "no wipetime\n";
     }
     else{
         cout << "wipeTime exists:" << inNode[ "wipeTime" ] <<endl;
         FireEvent* arduinoWipe = new FireEvent( "wipeArduino");
         arduinoWipe->fireTime =inNode[ "wipeTime" ].asFloat() * 1000;//whew
         arduinoWipe->arduinoName= inNode[ "arduino"].asString();
         droneEventList.push_back(arduinoWipe);
     }
}

//--------------------------------------------------------------
void SyncSequence::parseVidNode(ofxJSONElement inNode){
    string vidName = inNode["file"].asString();
    FireEvent* vidFireEvent = new FireEvent( "vidFire");
    vidFireEvent->fireTime = inNode["fireTime"].asFloat() * 1000;
    vidStartTimes.push_back( vidFireEvent->fireTime ); //store it as the start time for the video of this index
    cout <<"Making a video:" << vidName << ", that starts at:" <<((float)vidFireEvent->fireTime)/1000.f;
    droneEventList.push_back(vidFireEvent);
    
    SyncedOFVideoPlayer* vidPlayer;
    ofxAVFVideoPlayer* avVidPlayer;
    ofxThreadedVideoPlayer* threadedVidPlayer;
    float volume = 1.f;
    if ( inNode["volume"].type() != Json::nullValue ){
        volume = inNode["volume"].asFloat();
    }
    switch ( playerType ){
        case QTKIT:
            vidPlayer = new SyncedOFVideoPlayer();
            vidPlayer->setLoopState(OF_LOOP_NONE);
            
            vidPlayer->loadMovie(vidName);
            vidFireEvent->player = vidPlayer;
            qtVideoPlayers.push_back(vidPlayer);
            
            //hack to make it sync better
            vidPlayer->play();
            vidPlayer->setPosition(0);
            vidPlayer->setPaused(true);
            vidPlayer->setVolume(volume);
            break;
            
        case AVF:
            avVidPlayer = new ofxAVFVideoPlayer();
            avVidPlayer->setLoopState(OF_LOOP_NONE);
            avVidPlayer->loadMovie(vidName);
            
            vidFireEvent->avPlayer = avVidPlayer;
            avfVideoPlayers.push_back(avVidPlayer);
            avVidPlayer->play();
            avVidPlayer->setPosition(0);
            avVidPlayer->setPaused(true);
            avVidPlayer->setVolume(volume);
            break;
            
        case THREADED_AVF:
            threadedVidPlayer = new ofxThreadedVideoPlayer();
            //GlobalThreadedVids::players.push_back( threadedVidPlayer);
            
            ofAddListener(threadedVidPlayer->videoIsReadyEvent, this, &SyncSequence::videoIsReadyCallback);
            threadedVidPlayer->loadVideo(vidName);
            threadedVidPlayer->setLoopMode(OF_LOOP_NONE);
            threadedVidPlayer->setVolume(volume);
            vidFireEvent->threadedPlayer = threadedVidPlayer;
            threadedVideoPlayers.push_back(threadedVidPlayer);
            
            
            
            break;
    }
    
    if ( inNode["stopTime"].type() != Json::nullValue ){
        FireEvent* stopEvent = new FireEvent( "vidStop" );
        stopEvent->fireTime = inNode["stopTime"].asFloat() * 1000;
        
        switch ( playerType ){
            case QTKIT:
                stopEvent->player = vidPlayer;
                break;
                
            case AVF:
                stopEvent->avPlayer = avVidPlayer;
                break;
                
            case THREADED_AVF:
                stopEvent->threadedPlayer = threadedVidPlayer;
                break;
                
        }
        
        droneEventList.push_back(stopEvent);
        cout << " and stops at " << ((float)stopEvent->fireTime)/1000.f;
    }
    
    cout << endl;
    
}
//--------------------------------------------------------------
void SyncSequence::videoIsReadyCallback(ofxThreadedVideoPlayerStatus &status){
    cout << "videoIsReadyCallback\n";
    numThreadedVidsReady++; //hope there's not a race condition here
    ofRemoveListener(status.player->videoIsReadyEvent, this, &SyncSequence::videoIsReadyCallback);
    //status.player->play();
    //status.player->setPosition(0);
    status.player->setPaused(true);
    //this gets called during thread lock, so we should be thread safe
    numThreadedVidsReady++;
    
    
}