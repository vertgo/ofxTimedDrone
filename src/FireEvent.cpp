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
    curVidIDIndex = 0;
    
    
}

void FireEvent::setIDs(ofxJSONElement inNode){
    int numIDs;
    
    switch( inNode.type() ){
        case Json::stringValue:
            vidID = inNode.asString();
            vidIDs.push_back( inNode.asString() );
            break;
            
        case Json::arrayValue:
            numIDs = inNode.size();
            if ( numIDs < 1 ){
                return;
            }
            for( int i = 0; i < numIDs; i++ ){
                vidIDs.push_back(inNode[ i ].asString());
            }
            vidID = vidIDs[ 0 ];
            break;
        default:
            cout <<"FireEvent::setIDs::error!, unknown type:" <<  inNode.type() << endl;
            cout << "inNode:" << inNode.toStyledString();
            throw "wtf is this?";
    }
    
    
}

void FireEvent::cycle(){
    curVidIDIndex++;
    if (curVidIDIndex >= vidIDs.size() ){
        curVidIDIndex = 0;
    }
    vidID = vidIDs[ curVidIDIndex ];
}