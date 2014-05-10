//
//  FireEvent.h
//  ofxN2STempDisplay
//
//  Created by Michael Manh on 4/17/14.
//
//

#ifndef __ofxN2STempDisplay__FireEvent__
#define __ofxN2STempDisplay__FireEvent__

#include <iostream>
#include "ofxJSONElement.h"
#include "ofMain.h"
#include "ofxSimpleSerial.h"
#include "ofxAVFVideoPlayer.h"
#include "ofxThreadedVideoPlayer.h"

//ugly hack

class FireEvent{
public:
    FireEvent(string inType );
    string type; //"video" "arduino"
    uint fireTime; //for errbody
    uint stopTime; //for videos
    
    //bool fired; //should be ignored in favor of sorting by fire time,
                //but since they're so few this would be coded faster
    string arduinoName;
    string vidID;
    vector<string> vidIDs;
    map< string, vector< string>* > tagToIds;
    
    
    void setIDs( ofxJSONElement inNode);
    void setIDsAsTags( ofxJSONElement inNode ); //a hack to default the tag the same as the id
    void setTags( ofxJSONElement inNode );
    void cycle( string inTag = ""); //moves to the next video for the next sequence // defaults to nothing
    FireEvent* vidStartEvent;
    
private:
    int curVidIDIndex;
};

#endif /* defined(__ofxN2STempDisplay__FireEvent__) */
