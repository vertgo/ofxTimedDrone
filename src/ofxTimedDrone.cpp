#include "ofxTimedDrone.h"



//--------------------------------------------------------------
void ofxTimedDrone::setup(){
    
    playerType = AVF;//default to threaded AVF (currently threaded avf is buggy, qtkit is good but slow)
    
    //TODO don't set this here
    curSequence = new SyncSequence( "all" );
    
    //some default settings in case it doesn't work
    serverIP = "10.0.1.7";
    port = 1337;
    reconnectTime = 4000;
    
    //volume stuff
    volumeEasing = .1f;
    targetVolume = ambientVolume = 0.25f;
    turntUp = 0.75f;
    
    
    loadDroneConfig();
    ofSetVerticalSync(true);
    ofSetFrameRate(30);
    


    
    msgTx	= "";
	msgRx	= "";
    
    goTime = 0;
    timeWent = false;
    cout <<"setup::1\n";
    //{"goTime":1397764230152,"city":"New York","temperature":46.74,"condition":"partly-cloudy-day"}
    
    //are we connected to the server - if this fails we
	//will check every few seconds to see if the server exists
	weConnected = tcpClient.setup(serverIP, port);
    lastConnectTime = ofGetElapsedTimeMillis();
	//optionally set the delimiter to something else.  The delimter in the client and the server have to be the same
	tcpClient.setMessageDelimiter("\n");
    cout <<"setup::2\n";
	tcpClient.setVerbose(false);
    
    curEventIndex = 0;
    cout <<"setup::3\n";
    numThreadedVidsReady = 0;
    ofSetFullscreen(true);    
}

//used for sorting events
//bool isEventEarlier(FireEvent* i,FireEvent* j) { return (i->fireTime < j->fireTime); }

//--------------------------------------------------------------
void ofxTimedDrone::loadDroneConfig(){
    string configPath = "config.json";
    bool parsingSuccessful = droneResult.open(configPath);
    
    parseServerInfo(droneResult["server"]);
    parsePlayerInfo(droneResult["player"]);
    //cout << "loadDroneConfig:success:" <<parsingSuccessful <<endl;
    //cout << "loadDroneCOnfig:: config:" <<droneResult.toStyledString() <<endl;
    ofxJSONElement videoNodes = droneResult["videos"];
    ofxJSONElement arduinoNodes = droneResult["arduinos"];
    if ( droneResult["sound"].type() != Json::nullValue ){
        parseSoundInfo( droneResult["sound"] );
    }
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
    
    
    curSequence->sortList();

    
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
    curSequence->droneEventList.push_back(arduinoFire);
    
    if (inNode[ "wipeTime"].type() == Json::nullValue){
        cout << "no wipetime\n";
    }
    else{
        
        cout << "wipeTime exists:" << inNode[ "wipeTime" ] <<endl;
        FireEvent* arduinoWipe = new FireEvent( "wipeArduino");
        arduinoWipe->fireTime =inNode[ "wipeTime" ].asFloat() * 1000;//whew
        arduinoWipe->serial = serial;
        curSequence->droneEventList.push_back(arduinoWipe);
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
void ofxTimedDrone::parsePlayerInfo(ofxJSONElement inNode){
    ofxJSONElement ptypeNode = inNode["type"];
    if ( ptypeNode != Json::nullValue){
        string ptypeString = ptypeNode.asString();
        if ( ptypeString == "qtkit"){
            playerType = QTKIT;
        }
        else if ( ptypeString == "avf"){
            playerType = AVF;
        }
        else if ( ptypeString == "tavf"){ //threadedavf
            playerType = THREADED_AVF;
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
void ofxTimedDrone::parseSoundInfo(ofxJSONElement inNode){
    string soundName = inNode["file"].asString();
    
    if( inNode["easing"].type() != Json::nullValue ){
        volumeEasing = ofClamp( inNode["easing"].asFloat(), 0.001f, .9f);
    }
    FireEvent* soundFireEvent = new FireEvent( "soundFire");
    soundFireEvent->fireTime = inNode["fireTime"].asFloat() * 1000;
    cout <<"Making a sound:" << soundName << ", that starts at:" <<((float)soundFireEvent->fireTime)/1000.f;
    curSequence->droneEventList.push_back(soundFireEvent);
    
    targetVolume = ambientVolume = inNode["ambientVolume"].asFloat();
    turntUp = inNode["turntup"].asFloat();
    cout << "ambientVolume:"<<ambientVolume<<endl;
    cout << "turntup:"<<turntUp<<endl;
    //if ( inNode["stopTime"].type() != Json::nullValue ){
    FireEvent* stopEvent = new FireEvent( "soundStop" );
    stopEvent->fireTime = inNode["stopTime"].asFloat() * 1000;
    
    curSequence->droneEventList.push_back(stopEvent);
    cout << " and stops at " << ((float)stopEvent->fireTime)/1000.f;
    //}
    
    soundPlayer.loadSound(soundName);
    soundPlayer.setVolume(ambientVolume);
    soundPlayer.setLoop(true);
    soundPlayer.play();
    
    
    cout << endl;
    
}

//--------------------------------------------------------------
void ofxTimedDrone::parseDroneVid(ofxJSONElement inNode){
    string vidName = inNode["file"].asString();
    FireEvent* vidFireEvent = new FireEvent( "vidFire");
    vidFireEvent->fireTime = inNode["fireTime"].asFloat() * 1000;
    curSequence->vidStartTimes.push_back( vidFireEvent->fireTime ); //store it as the start time for the video of this index
    cout <<"Making a video:" << vidName << ", that starts at:" <<((float)vidFireEvent->fireTime)/1000.f;
    curSequence->droneEventList.push_back(vidFireEvent);
    
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
            curSequence->qtVideoPlayers.push_back(vidPlayer);

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
            curSequence->avfVideoPlayers.push_back(avVidPlayer);
            avVidPlayer->play();
            avVidPlayer->setPosition(0);
            avVidPlayer->setPaused(true);
            avVidPlayer->setVolume(volume);
            break;
            
        case THREADED_AVF:
            threadedVidPlayer = new ofxThreadedVideoPlayer();
            ofAddListener(threadedVidPlayer->videoIsReadyEvent, this, &ofxTimedDrone::videoIsReadyCallback);
            threadedVidPlayer->loadVideo(vidName);
            threadedVidPlayer->setLoopMode(OF_LOOP_NONE);
            threadedVidPlayer->setVolume(volume);
            vidFireEvent->threadedPlayer = threadedVidPlayer;
            curSequence->threadedVideoPlayers.push_back(threadedVidPlayer);
            
            /*threadedVidPlayer->play();
            threadedVidPlayer->setPosition(0);
            threadedVidPlayer->setPaused(true);
            */

            
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
        
        curSequence->droneEventList.push_back(stopEvent);
        cout << " and stops at " << ((float)stopEvent->fireTime)/1000.f;
    }
    
    cout << endl;
    
}
//--------------------------------------------------------------

void ofxTimedDrone::droneCheckForGo(){
    unsigned long long now = ofGetSystemTime();
    
    if( !timeWent && goTime != 0  ){
        
        //cout << "!timeWent::now:\n" <<now << "\n"<< goTime <<endl;
        if ( now >= goTime ){
            go(); //do something I suppose
            cout << "NOW!!\n";
        }
        else{
            cout << "wait for it.....\n";
        }
    }
    
    
    //cout <<"timeWent:" <<timeWent <<endl;
    
    
    
    if ( playerType == THREADED_AVF && numThreadedVidsReady < curSequence->threadedVideoPlayers.size()){
        for( int i = 0; i < curSequence->threadedVideoPlayers.size(); i++ ){
            curSequence->threadedVideoPlayers[i]->update();
        }
    }
    else if ( timeWent ) {
        
        checkDroneEvents();
        updateDroneVids();
        
    }
    
    
}

//--------------------------------------------------------------

void ofxTimedDrone::checkDroneEvents(){
    
    unsigned long long now = ofGetSystemTime();
    unsigned long long timeSinceGo = now - goTime;
    
    bool eventFired = true;
    while( eventFired && curEventIndex < curSequence->droneEventList.size() ){
        
        FireEvent* curEvent = curSequence->droneEventList[ curEventIndex];
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
        switch(playerType){
            case QTKIT:
                inEvent->player->play();
                break;
            case AVF:
                inEvent->avPlayer->play();
                break;
            case THREADED_AVF:
                inEvent->threadedPlayer->play();
                break;
            
        }
    }
    else if ( inEvent->type == "vidStop" ){
        switch(playerType){
            case QTKIT:
                //inEvent->player->stop();
                inEvent->player->setPosition(0);
                inEvent->player->setPaused(true);
                break;
                
            case AVF:
                //inEvent->avPlayer->stop();
                inEvent->avPlayer->setPosition(0);
                inEvent->avPlayer->setPaused(true);
                break;
                
            case THREADED_AVF:
                //inEvent->threadedPlayer->stop();
                inEvent->threadedPlayer->setPosition(0);
                inEvent->threadedPlayer->setPaused(true);
                
                break;
                
        }
    }
    else if ( inEvent->type == "soundFire" ){
        cout<< "soundFire, setting volume to:" << turntUp<<endl;
        
        targetVolume = turntUp;
    }
    
    else if ( inEvent->type == "soundStop" ){
        cout<< "soundStop, setting volume to:" << ambientVolume<<endl;
        //soundPlayer.setVolume(ambientVolume);
        targetVolume = ambientVolume;
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
    easeVolume();
    
}
//--------------------------------------------------------------
void ofxTimedDrone::easeVolume(){
    
    float inverseEasing = 1.f - volumeEasing;
    if ( soundPlayer.getVolume() != targetVolume ){
    
        float newVolume = soundPlayer.getVolume()* inverseEasing + volumeEasing* targetVolume;
        if ( abs(newVolume-targetVolume) < .01) {
            newVolume = targetVolume;
            cout<< "hitting target volume\n";
        }
        else{
            cout<< "easing volume\n";
        }
        soundPlayer.setVolume( newVolume );
    }
    
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
void ofxTimedDrone::videoIsReadyCallback(ofxThreadedVideoPlayerStatus &status){
    cout << "videoIsReadyCallback\n";
    numThreadedVidsReady++; //hope there's not a race condition here
    ofRemoveListener(status.player->videoIsReadyEvent, this, &ofxTimedDrone::videoIsReadyCallback);
    status.player->play();
    //status.player->setPosition(0);
    status.player->setPaused(true);
    
}
//--------------------------------------------------------------
void ofxTimedDrone::go(){
    resetVideos();
    curEventIndex = 0;
    timeWent = true;
}

//--------------------------------------------------------------
void ofxTimedDrone::resetVideos(){
    
    switch (playerType){
        case QTKIT:
            for (int i = 0; i < curSequence->qtVideoPlayers.size(); i++){
                curSequence->qtVideoPlayers[i]->setPosition(0);
                curSequence->qtVideoPlayers[i]->setPaused(true);

            }
            break;
        case AVF:
            for (int i = 0; i < curSequence->avfVideoPlayers.size(); i++){

                curSequence->avfVideoPlayers[i]->setPosition(0);
                curSequence->avfVideoPlayers[i]->setPaused(true);
            }
            break;
        case THREADED_AVF:
            for (int i = 0; i < curSequence->threadedVideoPlayers.size(); i++){
                curSequence->threadedVideoPlayers[i]->setPosition(0);
                curSequence->threadedVideoPlayers[i]->setPaused( true);
            }
            break;
    }

}

//--------------------------------------------------------------
void ofxTimedDrone::draw(){
    ofSetColor(255, 255, 255 );
    drawDroneVids();
    
}


//--------------------------------------------------------------
void ofxTimedDrone::updateDroneVids(){
    
    
    //hack, remove 300 ms, because it takes about 1/3rd of a second to start the play
    long long now = ofGetSystemTime() - 200;
    
    
    switch ( playerType ){
     case QTKIT:
            for( int i = 0; i< curSequence->qtVideoPlayers.size(); i++ ){
                
                long long playHead = now - goTime - curSequence->vidStartTimes[i];
                
                SyncedOFVideoPlayer* curPlayer = curSequence->qtVideoPlayers[ i ];
                
                if ( curPlayer->isPlaying() ){
                    curPlayer->syncToTime(playHead);
                    curPlayer->update();
                }
            }
            break;
     case AVF:
            for( int i = 0; i< curSequence->avfVideoPlayers.size(); i++ ){
                long long playHead = now - goTime - curSequence->vidStartTimes[i];
                
                ofxAVFVideoPlayer* curPlayer = curSequence->avfVideoPlayers[ i ];
                
                if ( curPlayer->getPlaying() ){
                    curPlayer->syncToTime( ((float)playHead )/1000.f);
                    curPlayer->update();
                }
            }
            break;
     case THREADED_AVF:
            
            for( int i = 0; i< curSequence->threadedVideoPlayers.size(); i++ ){
                long long playHead = now - goTime - curSequence->vidStartTimes[i];
                ofxThreadedVideoPlayer* curPlayer = curSequence->threadedVideoPlayers[ i ];
                if ( curPlayer->isPlaying() ){
                    curPlayer->syncToPlayhead( ((float)playHead )/1000.f);
                    curPlayer->update();
                }
            }
            break;
     }
    
    
    
}


//--------------------------------------------------------------
void ofxTimedDrone::drawDroneVids(){
    int numVids;
    int vidWidth = ofGetWidth();
    switch ( playerType ){
        case QTKIT:
            numVids = curSequence->qtVideoPlayers.size();
            
            
            for( int i = 0; i< curSequence->qtVideoPlayers.size(); i++ ){
                
                SyncedOFVideoPlayer* curPlayer = curSequence->qtVideoPlayers[ i ];
                
                if ( curPlayer->isPlaying() ){
                    cout << "drawing qt vid:"<<i<<endl;
                    float vidHeight = vidWidth/curPlayer->getWidth() * curPlayer->getHeight();
                    //curPlayer->draw( i * vidWidth,0, vidWidth, vidHeight  );
                    cout << "drawing qt vid:"<<i<< ", vidWidth:"<< vidWidth << ", vidHeight:" <<vidHeight << endl;
                    curPlayer->draw( 0,0, vidWidth, vidHeight  );
                    
                }
            }
        break;
        case AVF:
            numVids = curSequence->avfVideoPlayers.size();
            
            for( int i = 0; i< curSequence->avfVideoPlayers.size(); i++ ){
                
                ofxAVFVideoPlayer* curPlayer = curSequence->avfVideoPlayers[ i ];
                
                if ( curPlayer->getPlaying() ){
                    //cout << "drawing avf vid:"<<i<<endl;
                    float vidHeight = vidWidth/curPlayer->getWidth() * curPlayer->getHeight();
                    
                    curPlayer->draw( 0,0, vidWidth, vidHeight  );
                    
                }
            }

            break;
        case THREADED_AVF:
            numVids = curSequence->threadedVideoPlayers.size();
            
            for( int i = 0; i< curSequence->threadedVideoPlayers.size(); i++ ){
                //cout << "drawing threaded vid:"<<i<<endl;
                ofxThreadedVideoPlayer* curPlayer = curSequence->threadedVideoPlayers[ i ];
                
                if ( curPlayer->isPlaying() ){
                    
                    float vidHeight = vidWidth/curPlayer->getWidth() * curPlayer->getHeight();
                    curPlayer->draw( 0,0, vidWidth, vidHeight  );
                    
                }
            }
            break;
        }
    
    
    
    
    
}
//--------------------------------------------------------------
void ofxTimedDrone::testGo(){
    //TODO set gotime == now and then go
    goTime = ofGetSystemTime();
    go();
}

//--------------------------------------------------------------
void ofxTimedDrone::testGoNow(){
    //find the first event time
    goTime = ofGetSystemTime() - curSequence->droneEventList[0]->fireTime;
    go();
}
//--------------------------------------------------------------
void ofxTimedDrone::keyPressed(int key){
    switch (key) {
        case 'g':
            testGo();
            break;
        
        case 'n':
            testGoNow();
            break;
            
        default:
            break;
    }
}

//--------------------------------------------------------------
void ofxTimedDrone::keyReleased(int key){
    
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
