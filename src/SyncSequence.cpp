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
    //numThreadedVidsReady = 0;
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
        cout << "parsing video\n";
        
    }
    
    int numArduinos = arduinoNodes.size();
    for( int i = 0; i < numArduinos; i++ ){
        parseDroneDuino( arduinoNodes[i] );
        cout << "parsing arduino\n";
        
    }
    
    parseSound( inNode[ "sound" ] );
    
    cout << "numVideos:" <<numVideos << ", numArduinos:"<< numArduinos <<endl;
    
    
    sortList();
}
//--------------------------------------------------------------
void SyncSequence::parseSound(ofxJSONElement inNode){
    if ( inNode[ "fireTime"].type() != Json::nullValue ){
        FireEvent* soundFireEvent = new FireEvent( "soundFire");
        soundFireEvent->fireTime = inNode["fireTime"].asFloat() * 1000;
        droneEventList.push_back(soundFireEvent);
        cout << "parsing soundFire\n";
    }
    
    if ( inNode[ "stopTime"].type() != Json::nullValue ){
        
        FireEvent* stopEvent = new FireEvent( "soundStop" );
        stopEvent->fireTime = inNode["stopTime"].asFloat() * 1000;
        droneEventList.push_back(stopEvent);
        cout << "parsing soundStop\n";
    }
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
    //string vidID = inNode["id"].asString();
    FireEvent* vidFireEvent = new FireEvent( "vidFire");
    //look if it has tags
    if ( inNode["tags"].type() != Json::nullValue ){
        //it uses tags rather than ids
        vidFireEvent->setTags(inNode["tags"]);
    }
    else if ( inNode["id"].type() != Json::nullValue ){
        vidFireEvent->setIDs( inNode["id"] );
    }
    else{
        globalErrorMessage += "vidNode has no tags or ids" + inNode.toStyledString();
        
    }
    //cout << "SyncSequence::parseVidNode::" << inNode.toStyledString() <<endl;
    
    vidFireEvent->fireTime = inNode["fireTime"].asFloat() * 1000;
    //vidStartTimes[vidID] = ( vidFireEvent->fireTime ); //store it as the start time for the video of this index
    //cout <<"Making a video event:" << vidID << ", that starts at:" <<((float)vidFireEvent->fireTime)/1000.f;
    
    //vidFireEvent->vidID = vidID;
    droneEventList.push_back(vidFireEvent);

    
    if ( inNode["stopTime"].type() != Json::nullValue ){
        FireEvent* stopEvent = new FireEvent( "vidStop" );
        stopEvent->fireTime = inNode["stopTime"].asFloat() * 1000;
        
        //stopEvent->vidID = vidID;
        stopEvent->vidStartEvent = vidFireEvent; //so it can stop the videos when it starts
        
        droneEventList.push_back(stopEvent);
        cout << " and stops at " << ((float)stopEvent->fireTime)/1000.f;
    }
    
    cout << endl;
    
}

//--------------------------------------------------------------
void SyncSequence::cycle( string inTag ){
    for ( int i = 0; i < droneEventList.size(); i++ ){
        if ( droneEventList[ i ]->type == "vidFire" ){
            droneEventList[ i ] ->cycle( inTag );
        }
    }
}