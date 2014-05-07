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
    vidID = "";
    
}