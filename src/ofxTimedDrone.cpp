#include "ofxTimedDrone.h"



//--------------------------------------------------------------
void ofxTimedDrone::setup(){
    
    //some default settings in case it doesn't work
    serverIP = "10.0.1.7";
    port = 1337;
    reconnectTime = 400;
    
    loadDroneConfig();
    ofSetVerticalSync(true);
    ofSetFrameRate(30);
    
    msgTx	= "";
	msgRx	= "";
    
    goTime = 0;
    timeWent = false;
    
    //{"goTime":1397764230152,"city":"New York","temperature":46.74,"condition":"partly-cloudy-day"}
    
    //are we connected to the server - if this fails we
	//will check every few seconds to see if the server exists
	weConnected = tcpClient.setup(serverIP, port);
    lastConnectTime = ofGetElapsedTimeMillis();
	//optionally set the delimiter to something else.  The delimter in the client and the server have to be the same
	tcpClient.setMessageDelimiter("\n");
    
	tcpClient.setVerbose(false);
    
    curEventIndex = 0;
    
    
}

//used for sorting events
bool isEventEarlier(FireEvent* i,FireEvent* j) { return (i->fireTime < j->fireTime); }

//--------------------------------------------------------------
void ofxTimedDrone::loadDroneConfig(){
    string configPath = "config.json";
    bool parsingSuccessful = droneResult.open(configPath);
    
    parseServerInfo(droneResult["server"]);
    
    //cout << "loadDroneConfig:success:" <<parsingSuccessful <<endl;
    //cout << "loadDroneCOnfig:: config:" <<droneResult.toStyledString() <<endl;
    ofxJSONElement videoNodes = droneResult["videos"];
    ofxJSONElement arduinoNodes = droneResult["arduinos"];
    
    int numVideos = videoNodes.size();
    
    for( int i = 0; i < numVideos; i++ ){
        ofxJSONElement curVidNode = videoNodes[ i];
        parseDroneVid(curVidNode);
        cout << "parsing video";
        
    }
    
    int numArduinos = arduinoNodes.size();
    for( int i = 0; i < numArduinos; i++ ){
        parseDroneDuino( arduinoNodes[i] );
        cout << "parsing arduino";
        
    }
    
    
    cout << "numVideos:" <<numVideos << ", numArduinos:"<< numArduinos <<endl;
    
    
    sort( droneEventList.begin(), droneEventList.end(), isEventEarlier);
    cout << "fireTimes:";
    for( int i = 0; i < droneEventList.size();i++ ){
        
        cout << droneEventList[ i ]->fireTime << ",";
    }
    cout <<endl;
    
}
//--------------------------------------------------------------
void ofxTimedDrone::parseServerInfo(ofxJSONElement inNode){
    serverIP = (inNode["ip"].type() == Json::nullValue) ? serverIP: inNode["ip"].asString() ;
    //if it's null, set it to its current value, if not, set it to value in the config
    
    port = (inNode["port"].type() == Json::nullValue) ? port : inNode["port"].asInt() ;
    //same for port
    
    reconnectTime = (inNode["reconnectTime"].type() == Json::nullValue) ? reconnectTime : inNode["reconnectTime"].asInt() ;
    //and reconnectTime
}
//--------------------------------------------------------------
void ofxTimedDrone::parseDroneDuino(ofxJSONElement inNode){
    string arduinoName = inNode[ "arduino"].asString();
    
    ofxSimpleSerial* serial = new ofxSimpleSerial();
    
    //commented this out for debug
    serial->setup( arduinoName, 9600);
	serial->startContinuousRead(false);
    droneArduinos.push_back(serial);
    
    
    //now make an event for the arduino to fire
    FireEvent* arduinoFire = new FireEvent( "fireArduino");
    arduinoFire->fireTime = inNode[ "fireTime"].asFloat() * 1000;
    arduinoFire->serial = serial;
    droneEventList.push_back(arduinoFire);
    
    if (inNode[ "wipeTime"].type() == Json::nullValue){
        cout << "no wipetime\n";
    }
    else{
        
        cout << "wipeTime exists:" << inNode[ "wipeTime" ] <<endl;
        FireEvent* arduinoWipe = new FireEvent( "wipeArduino");
        arduinoWipe->fireTime =inNode[ "wipeTime" ].asFloat() * 1000;//whew
        arduinoWipe->serial = serial;
        droneEventList.push_back(arduinoWipe);
    }
    
    ofxJSONElement commands = inNode["commands"];
    if ( commands.type() != Json::nullValue ){
        cout << "parsing commands\n";
        int numCommands = commands.size();
        for( int i = 0; i < numCommands; i++ ){
            addCommand(commands[i].asString(), serial);
        }
        
    }
    
    
    
}

//--------------------------------------------------------------
void ofxTimedDrone::addCommand(string command, ofxSimpleSerial *serial){
    
    cout << "addCommand:" << command <<endl;
    vector<ofxSimpleSerial*>* serialList = droneCommandToListOfSerials[ command];
    if ( serialList == NULL ){
        serialList = new vector<ofxSimpleSerial*>();
        droneCommandToListOfSerials[ command] = serialList;
    }
    
    
    serialList->push_back( serial );
    cout << "adding arduino command:" << command <<endl;
}

//--------------------------------------------------------------
void ofxTimedDrone::parseDroneVid(ofxJSONElement inNode){
    ofVideoPlayer* vidPlayer = new ofVideoPlayer();
    string vidName = inNode["file"].asString();
    vidPlayer->loadMovie(vidName);
    FireEvent* vidFireEvent = new FireEvent( "vidFire");
    vidFireEvent->fireTime = inNode["fireTime"].asFloat() * 1000;
    vidFireEvent->player = vidPlayer;
    droneEventList.push_back(vidFireEvent);
    droneVideoPlayers.push_back(vidPlayer);
    
    cout <<"Making a video:" << vidName << ", that starts at:" <<((float)vidFireEvent->fireTime)/1000.f;
    
    if ( inNode["stopTime"].type() != Json::nullValue ){
        FireEvent* stopEvent = new FireEvent( "vidStop" );
        stopEvent->fireTime = inNode["stopTime"].asFloat() * 1000;
        stopEvent->player = vidPlayer;
        droneEventList.push_back(stopEvent);
        cout << " and stops at " << ((float)stopEvent->fireTime)/1000.f;
    }
    
    cout << endl;
    
}
//--------------------------------------------------------------

void ofxTimedDrone::droneCheckForGo(){
    unsigned long long now = ofGetSystemTime();
    
    if( !timeWent && goTime != 0  ){
        
        cout << "!timeWent::now:\n" <<now << "\n"<< goTime <<endl;
        if ( now >= goTime ){
            go(); //do something I suppose
            cout << "NOW!!\n";
        }
        else{
            cout << "wait for it.....\n";
        }
    }
    
    //not in an else statement, the previous if statement might have triggered time went
    if ( timeWent ) {
        checkDroneEvents();
        
    }
    
    updateDroneVids();
}

//--------------------------------------------------------------

void ofxTimedDrone::checkDroneEvents(){
    
    unsigned long long now = ofGetSystemTime();
    unsigned long long timeSinceGo = now - goTime;
    
    bool eventFired = true;
    while( eventFired && curEventIndex < droneEventList.size() ){
        
        FireEvent* curEvent = droneEventList[ curEventIndex];
        if ( timeSinceGo > curEvent->fireTime ){
            goFireEvent( curEvent );
            curEventIndex++;
        }
        
        else{
            eventFired = false;
        }
        
        
    }
    
    
}
//--------------------------------------------------------------
void ofxTimedDrone::goFireEvent( FireEvent* inEvent ){
    cout << "ofxTimedDrone::goFireEvent:" << inEvent->type <<endl;
    
    if ( inEvent->type == "fireArduino" ){
        inEvent->serial->writeString("go\n");
    }
    else if ( inEvent->type == "wipeArduino" ){
        inEvent->serial->writeString("rset\n");
    }
    else if ( inEvent->type == "vidFire" ){
        inEvent->player->play();
    }
    else if ( inEvent->type == "vidStop" ){
        inEvent->player->stop();//simple enough
        inEvent->player->setPosition(0.f);
    }
}


//--------------------------------------------------------------
void ofxTimedDrone::update(){
    if ( weConnected && tcpClient.isConnected() ){
        //if(tcpClient.send(msgTx)){
        string str = tcpClient.receive();
        if( str.length() > 0 ){
            msgRx = str;
            
            cout << "our message: " << msgRx << endl;
            
            // Now parse the JSON
            bool parsingSuccessful = result.parse(msgRx);
            if (parsingSuccessful) {
                
                // our debug info
                //cout << result.getRawString() << endl;
                unsigned long long newGoTime = (unsigned long long)result[ "goTime"].asInt64();//.asUInt();
                goTime = newGoTime;
                timeWent = false;
                
                parseJsonFromServer(result);
            }
            
            
        }
    }
    else {
        //if we are not connected lets try and reconnect every reconnectTime seconds
        int deltaTime = ofGetElapsedTimeMillis() - lastConnectTime;
        
        if( deltaTime > reconnectTime ){
            weConnected = tcpClient.setup( serverIP, port);
            lastConnectTime = ofGetElapsedTimeMillis();
        }
    }
    
    droneCheckForGo();
    
}

//--------------------------------------------------------------
void ofxTimedDrone::parseJsonFromServer(ofxJSONElement inNode){
    
    vector<string> members = inNode.getMemberNames();
    cout << "members:" ;
    for ( int i = 0; i < members.size(); i++ ){
        cout <<  members[i]<<",";
        //go through and init the arduinos as you need them to be
        vector< ofxSimpleSerial*>* arduinos =  droneCommandToListOfSerials[ members[ i ]];
        
        if (arduinos != NULL){ //if there's anything to respond to that message
            
            cout << "we have commands for:" + members[i] <<endl;
            for( int j = 0; j < arduinos->size(); j++ ){
                string message = members[ i ] + ":"+ inNode[ members[ i ]].asString()+"\n"; //so if there's '  "temp":"30"  ' in the json, it comes out as 'temp:30' to the arduino
                cout << "writing:" << message << endl;
                (*arduinos)[j]->writeString( message);
            }
        }
        else{
            cout << "we have no commands for:" + members[i] <<endl;
        }
    }
    cout << endl;
    
}
//--------------------------------------------------------------
void ofxTimedDrone::go(){
    curEventIndex = 0;
    timeWent = true;
}



//--------------------------------------------------------------
void ofxTimedDrone::draw(){
    ofSetColor(255, 255, 255 );
    drawDroneVids();
    
}


//--------------------------------------------------------------
void ofxTimedDrone::updateDroneVids(){
    for( int i = 0; i< droneVideoPlayers.size(); i++ ){
        
        ofVideoPlayer* curPlayer = droneVideoPlayers[ i ];
        
        if ( curPlayer->isPlaying() ){
            curPlayer->update();
        }
    }
}


//--------------------------------------------------------------
void ofxTimedDrone::drawDroneVids(){
    int numVids = droneVideoPlayers.size();
    int vidWidth = ofGetWidth();// /numVids;
    
    for( int i = 0; i< droneVideoPlayers.size(); i++ ){
        
        ofVideoPlayer* curPlayer = droneVideoPlayers[ i ];
        
        if ( curPlayer->isPlaying() ){
            
            float vidHeight = vidWidth/curPlayer->getWidth() * curPlayer->getHeight();
            //curPlayer->draw( i * vidWidth,0, vidWidth, vidHeight  );
            curPlayer->draw( 0,0, vidWidth, vidHeight  );
            
        }
    }
    
}
//--------------------------------------------------------------
void ofxTimedDrone::testGo(){
    //TODO set gotime == now and then go
}

//--------------------------------------------------------------
void ofxTimedDrone::keyPressed(int key){
    
}

//--------------------------------------------------------------
void ofxTimedDrone::keyReleased(int key){
    switch (key) {
        case 'g':
            //todo test go
            testGo();
    }
}

//--------------------------------------------------------------
void ofxTimedDrone::mouseMoved(int x, int y ){
    
}

//--------------------------------------------------------------
void ofxTimedDrone::mouseDragged(int x, int y, int button){
    
}

//--------------------------------------------------------------
void ofxTimedDrone::mousePressed(int x, int y, int button){
    
}

//--------------------------------------------------------------
void ofxTimedDrone::mouseReleased(int x, int y, int button){
    
}

//--------------------------------------------------------------
void ofxTimedDrone::windowResized(int w, int h){
    
}

//--------------------------------------------------------------
void ofxTimedDrone::gotMessage(ofMessage msg){
    
}

//--------------------------------------------------------------
void ofxTimedDrone::dragEvent(ofDragInfo dragInfo){
    
}
