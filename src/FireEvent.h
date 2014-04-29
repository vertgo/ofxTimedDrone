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

class FireEvent{
public:
    FireEvent(string inType );
    string type; //"video" "arduino"
    uint fireTime; //for errbody
    uint stopTime; //for videos
    
    //bool fired; //should be ignored in favor of sorting by fire time,
                //but since they're so few this would be coded faster
    string arduinoName;
    ofVideoPlayer* player;
    ofxAVFVideoPlayer* avPlayer;
    ofxThreadedVideoPlayer* threadedPlayer;
    
    
    
};

#endif /* defined(__ofxN2STempDisplay__FireEvent__) */
