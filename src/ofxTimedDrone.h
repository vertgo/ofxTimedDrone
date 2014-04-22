#pragma once

#include "ofMain.h"
#include "ofxSimpleSerial.h"
#include "ofxNetwork.h"
#include "ofxJSONElement.h"
#include "FireEvent.h"


class ofxTimedDrone : public ofBaseApp{
    
public:
    
    void setup();
    void update();
    void draw();
    
    void keyPressed(int key);
    void keyReleased(int key);
    void mouseMoved(int x, int y );
    void mouseDragged(int x, int y, int button);
    void mousePressed(int x, int y, int button);
    void mouseReleased(int x, int y, int button);
    void windowResized(int w, int h);
    void dragEvent(ofDragInfo dragInfo);
    void gotMessage(ofMessage msg);
    void go();
    
    //server stuff
    string serverIP;
    int port;
    int reconnectTime;
    
    int lastConnectTime;
    
    
    //drone stuff
    unsigned long long goTime;      //specifies the time to "go" when the sequence of events across all drones begins,
    //all fired events will be some non negative offset from this
    
    bool timeWent;                  //set to true when the go time has passed and we are looking for events to fire.
    //Events will fire in the first frame after the fire time has elapsed since go time
    void loadDroneConfig();         //loads the config file
    void parseServerInfo( ofxJSONElement inNode );  //parses the server info
    void parseDroneVid( ofxJSONElement inNode ); //parses a single video config node, making all the necessary firevents to start and stop a video
    void parseDroneDuino( ofxJSONElement inNode ); //parses a single arduino config node
    void parseDronServer( ofxJSONElement inNode ); //parses the server information from the config //currently not implemented
    void addCommand( string command, ofxSimpleSerial* serial);  //when the arduino node is parsed, it looks for commands
    //sent from the server to send config info to the arduino.
    //It passes it through unmodified.
    //TODO: There might need to be a way to pass a function to parse that info rather than just pass it through
    
    void droneCheckForGo();         //looks for events every frame
    void checkDroneEvents();        //once goTime has passed, we check each frame to see if its time to set off the next event.
    //At most, an event will be <1 frame late, approx 1/30th of a sec
    void goFireEvent( FireEvent* inEvent); //Fires an event, doing the necessary work based on the eventType
    int curEventIndex;              //Index of the first unfired event, once it reaches the number of events in the queue,
    //it knows that the events are done
    
    //TODO: maybe put in a reset function when it hits here? Unless a general reset is specified?
    
    void parseJsonFromServer( ofxJSONElement inNode );  //when the new json comes in, besides just go time,
    //it has to send the correct commands to the arduinos
    void drawDroneVids();           //draw the videos
    void updateDroneVids();         //updates the vids
    void testGo();                  //sets the gotime to the current time and tests a run before the server tells the drone to
    
    vector< FireEvent* > droneEventList;    //list of fire events ordered in sequence
    vector< ofVideoPlayer*> droneVideoPlayers;  //vector of all the vids
    vector< ofxSimpleSerial*> droneArduinos;    //list of arduinos, not actually necessary
    map<string, vector<ofxSimpleSerial*>* > droneCommandToListOfSerials; //this is a dictionary where a configuration maps to a list of arduinos who need that configuration set
    //LOOK AT ALL THOSE POINTERS #ballin'
    ofxJSONElement droneResult;
    
    ofxJSONElement result;
    ofxTCPClient tcpClient;
    string msgTx, msgRx;
    
    bool weConnected;
};