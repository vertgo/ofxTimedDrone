//
//  FireEvent.cpp
//  ofxN2STempDisplay
//
//  Created by Michael Manh on 4/17/14.
//
//

#include "FireEvent.h"

FireEvent::FireEvent( string inType ){
    //initialize the event from the node
    //fireTime = inNode["fireTime"].asFloat() *1000;
    type = inType;
    player = NULL;
    avPlayer = NULL;
    threadedPlayer = NULL;
    /*
    if (type == "video"){
        stopTime = inNode["stopTime"].asFloat() *1000;
    }
    else if (type == "arduino"){
        stopTime = inNode["wipeTime"].asFloat() *1000;
    }
    */
    
}