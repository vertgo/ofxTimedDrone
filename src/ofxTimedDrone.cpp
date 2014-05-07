#include "ofxTimedDrone.h"



//--------------------------------------------------------------
void ofxTimedDrone::setup(){
    
    playerType = THREADED_AVF;//default to threaded AVF (currently threaded avf is buggy, qtkit is good but slow)
    
    
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
	//tcpClient.setMessageDelimiter("\n");
    cout <<"setup::2\n";
	tcpClient.setVerbose(false);
    
    curEventIndex = 0;
    cout <<"setup::3\n";
    ofSetFullscreen(true);    
}

//used for sorting events
//bool isEventEarlier(FireEvent* i,FireEvent* j) { return (i->fireTime < j->fireTime); }

//--------------------------------------------------------------
void ofxTimedDrone::loadDroneConfig(){
    string configPath = "config.json";
    bool parsingSuccessful = configJson.open(configPath);
    
    parseSettings( configJson["settings"] );
    parseVideos( configJson["videos"] );
    
    if ( configJson[ "arduinos" ].type()!= Json::nullValue ){
        parseArduinoNames( configJson[ "arduinos" ]);
    }
    else{
        cout <<"hey, no arduinos specified, that's cool\n";
        cout << "configJson:" << configJson.toStyledString();
    }
    
    //cout << "loadDroneConfig:success:" <<parsingSuccessful <<endl;
    //cout << "loadDroneCOnfig:: config:" <<configJson.toStyledString() <<endl;
    ofxJSONElement optionNodes = configJson[ "options" ];
    
    if (optionNodes.type() == Json::nullValue ){
        cout << "hey, there's no options\n";
    }
    
    int numOptions = optionNodes.size();
    for( int i = 0; i < numOptions; i++ ){
       cout << "ofxTimedDrone::loadDroneConfig::1\n";
        //read an option and add it to the map
        ofxJSONElement curOptionNode = optionNodes[ i ];
        cout << "ofxTimedDrone::loadDroneConfig::2\n";
        string curOptionName = curOptionNode[ "option" ].asString();
        cout << "ofxTimedDrone::loadDroneConfig::2.5\n";
        SyncSequence* curParsingSequence = new SyncSequence( curOptionName );
        cout << "ofxTimedDrone::loadDroneConfig::3\n";
        optionNameToSequence[ curOptionName] = curParsingSequence;
        curParsingSequence->parseFromJson(curOptionNode, playerType);
        cout << "ofxTimedDrone::loadDroneConfig::4\n";
        //make the first sequence the default, for now
        if ( i == 0 ){
            cout << "ofxTimedDrone::loadDroneConfig::5\n";
            curSequence = defaultSequence = curParsingSequence;
        }
    }
    
    
    
}


//--------------------------------------------------------------
void ofxTimedDrone::parseSettings(ofxJSONElement inNode){
    serverIP = (inNode["ip"].type() == Json::nullValue) ? serverIP: inNode["ip"].asString() ;
    //if it's null, set it to its current value, if not, set it to value in the config
    
    port = (inNode["port"].type() == Json::nullValue) ? port : inNode["port"].asInt() ;
    //same for port
    
    reconnectTime = (inNode["reconnectTime"].type() == Json::nullValue) ? reconnectTime : inNode["reconnectTime"].asInt() ;
    //and reconnectTime
    
    //now read the sound info from the settings node
    if ( inNode["soundFile"].type() != Json::nullValue ){
        parseSoundInfo( inNode );
    }
    
    cout << "parseSettings::" << inNode.toStyledString();
    parsePlayerInfo( inNode["playerType"] );
    
}


//--------------------------------------------------------------
void ofxTimedDrone::parseVideos(ofxJSONElement inNode){
    int numVids = inNode.size();
    for ( int i = 0; i < numVids; i++ ){
        ofxJSONElement curNode = inNode[ i ];
        float volume = 1.f;
        string path;
        string id;
        if ( curNode["path"].type() != Json::nullValue ){
            path = curNode["path"].asString();
        }
        else{
            cout << "ofxTimedDrone::parseVideos::ERROR! NO PATH FOR VID";
        }
        if ( curNode["id"].type() != Json::nullValue ){
            id = curNode["id"].asString();
        }
        else{
            cout << "ofxTimedDrone::parseVideos::ERROR! NO ID FOR VID";
        }
        if ( curNode["volume"].type() != Json::nullValue ){
            volume = curNode["volume"].asFloat();
        }
        loadVideo( path, id, volume);
    }
    
}
//--------------------------------------------------------------
void ofxTimedDrone::loadVideo(string inFile, string inVidID, float inVolume){
    
    SyncedOFVideoPlayer* vidPlayer;
    ofxAVFVideoPlayer* avVidPlayer;
    ofxThreadedVideoPlayer* threadedVidPlayer;
    
    switch ( playerType ){
        case QTKIT:
            vidPlayer = new SyncedOFVideoPlayer();
            vidPlayer->setLoopState(OF_LOOP_NONE);
            vidPlayer->loadMovie(inFile);
            idToQTPlayers[ inVidID ] = vidPlayer;
            //hack to make it sync better
            vidPlayer->play();
            vidPlayer->setPosition(0);
            vidPlayer->setPaused(true);
            vidPlayer->setVolume(inVolume);
            
            break;
            
        case AVF:
            
            avVidPlayer = new ofxAVFVideoPlayer();
            avVidPlayer->setLoopState(OF_LOOP_NONE);
            avVidPlayer->loadMovie(inFile);
            avVidPlayer->play();
            avVidPlayer->setPosition(0);
            avVidPlayer->setPaused(true);
            avVidPlayer->setVolume(inVolume);
            idToAVFPlayers[ inVidID ] = avVidPlayer;
            
            break;
            
        case THREADED_AVF:
            
            
            threadedVidPlayer = new ofxThreadedVideoPlayer();
            GlobalThreadedVids::players.push_back( threadedVidPlayer);
            ofAddListener(threadedVidPlayer->videoIsReadyEvent, this, &ofxTimedDrone::videoIsReadyCallback);
            threadedVidPlayer->loadVideo(inFile);
            threadedVidPlayer->setLoopMode(OF_LOOP_NONE);
            threadedVidPlayer->setVolume(inVolume);
            playerToVolume[ threadedVidPlayer] = inVolume;
            
            cout << "loadVideo::id:" << inVidID << ", path:" << inFile;
            idToThreadedPlayers[ inVidID] = threadedVidPlayer;
            
            break;
    }
}

//--------------------------------------------------------------
void ofxTimedDrone::videoIsReadyCallback(ofxThreadedVideoPlayerStatus &status){
    cout << "videoIsReadyCallback\n";
    ofRemoveListener(status.player->videoIsReadyEvent, this, &ofxTimedDrone::videoIsReadyCallback);
    status.player->setPaused(true);
    //this gets called during thread lock, so we should be thread safe
    GlobalThreadedVids::numLoadedThreadedVids++;
    status.player->setVolume( playerToVolume[ status.player] );
    
}

//--------------------------------------------------------------
void ofxTimedDrone::parseArduinoNames(ofxJSONElement inNode){
    int numArduinos = inNode.size();
    for ( int i = 0; i < numArduinos; i++ ){
        ofxJSONElement curArduinoDeviceNode = inNode[i];
        ofxSimpleSerial* serial = new ofxSimpleSerial();
        string deviceName = curArduinoDeviceNode[ "device"].asString();
        serial->setup( deviceName, 9600);
        serial->startContinuousRead(false);
        //droneArduinos.push_back(serial);
        
        string arduinoName = curArduinoDeviceNode[ "name"].asString();
        arduinoNamesToSerials[arduinoName] = serial;
        cout << "ofxTimeDrone::parseArduinoNames::arduinoName:" << arduinoName <<" has device" << deviceName <<endl;
        
        ofxJSONElement commands = curArduinoDeviceNode["commands"];
        if ( commands.type() != Json::nullValue ){
            cout << "parsing commands\n";
            int numCommands = commands.size();
            for( int i = 0; i < numCommands; i++ ){
                addCommand(commands[i]["path"].asString(), commands[i]["out"].asString(), serial);
            }
        }
    }
}


//--------------------------------------------------------------
void ofxTimedDrone::parsePlayerInfo(ofxJSONElement ptypeNode){
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
void ofxTimedDrone::addCommand(string inPath, string inCommand, ofxSimpleSerial *serial){
    
    cout << "addCommand:" << inCommand <<endl;
    //vector<ofxSimpleSerial*>* serialList = droneCommandToListOfSerials[ command];
    CommandPath* curCP = new CommandPath();
    curCP->name = inCommand;
    curCP->path = new Json::Path(inPath);
    curCP->serial = serial;
    
    jsonArduinoCommands.push_back( curCP );
    
    
}
//--------------------------------------------------------------
void ofxTimedDrone::parseSoundInfo(ofxJSONElement inNode){
    if ( inNode["soundFile"].type() != Json::nullValue ){
        string soundName = inNode["soundFile"].asString();
        soundPlayer.loadSound(soundName);
        soundPlayer.play();
    }
    
    if( inNode["easing"].type() != Json::nullValue ){
        volumeEasing = ofClamp( inNode["easing"].asFloat(), 0.001f, .9f);
    }
    
    if ( inNode["ambientVolume"].type() != Json::nullValue ){
        targetVolume = ambientVolume = inNode["ambientVolume"].asFloat();
        cout << "ambientVolume:"<<ambientVolume<<endl;
    }

    if ( inNode["ambientVolume"].type() != Json::nullValue ){
        turntUp = inNode["turntup"].asFloat();
        cout << "turntup:"<<turntUp<<endl;
    }
    

    soundPlayer.setVolume(ambientVolume);
    soundPlayer.setLoop(true);


    
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
    
    
    
    if ( playerType == THREADED_AVF && GlobalThreadedVids::numLoadedThreadedVids < GlobalThreadedVids::players.size() ){
        for( int i = 0; i < GlobalThreadedVids::players.size(); i++ ){
            GlobalThreadedVids::players[i]->update();
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
        if ( arduinoNamesToSerials[inEvent->arduinoName] != NULL ){
           arduinoNamesToSerials[inEvent->arduinoName]->writeString("go\n");
        }
        else{
            cout << "No arduino named " << inEvent->arduinoName << endl;
        }
    }
    else if ( inEvent->type == "wipeArduino" ){
        if ( arduinoNamesToSerials[inEvent->arduinoName] != NULL ){
            arduinoNamesToSerials[ inEvent->arduinoName ]->writeString("rset\n");
        }
        else{
            cout << "No arduino named " << inEvent->arduinoName << endl;
        }
    }
    else if ( inEvent->type == "vidFire" ){
        cout << "vid ID:" << inEvent->vidID <<endl;
        switch(playerType){
            case QTKIT:
                idToQTPlayers[ inEvent->vidID]->play();
                break;
            case AVF:
                idToAVFPlayers[ inEvent->vidID]->play();
                break;
            case THREADED_AVF:
                idToThreadedPlayers[ inEvent->vidID]->play();
                break;
            
        }
    }
    else if ( inEvent->type == "vidStop" ){
        switch(playerType){
            case QTKIT:
                idToQTPlayers[ inEvent->vidID]->setPosition(0);
                idToQTPlayers[ inEvent->vidID]->setPaused(true);
                break;
                
            case AVF:
                idToAVFPlayers[ inEvent->vidID]->setPosition(0);
                idToAVFPlayers[ inEvent->vidID]->setPaused(true);
                break;
                
            case THREADED_AVF:
                idToThreadedPlayers[ inEvent->vidID]->setPosition(0);
                idToThreadedPlayers[ inEvent->vidID]->setPaused(true);
                
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
        if (ofGetElapsedTimeMillis() > 4000 ){ //hack to make it wait to parse the backend data

            string str = tcpClient.receive();
            if( str.length() > 0 ){
                msgRx = str;
                cout << "our message: " << msgRx << endl;
                // Now parse the JSON
                bool parsingSuccessful = result.parse(msgRx);
                if (parsingSuccessful) {
                    
                    // our debug info
                    //cout << result.getRawString() << endl;
                    
                    
                    
                    //if it's a goTime message, find the event and set go
                    if ( result["goTime"].type() != Json::nullValue ){
                        cout << "update::parsing goTime\n";
                        //{"goTime":1398718870436,"eventType":"sports"}[/TCP]
                        unsigned long long newGoTime = (unsigned long long)result[ "goTime"].asInt64();//.asUInt();
                        string eventType = result[ "eventType"].asString();
                        //TODO stop videos in the last current sequence
                        if ( optionNameToSequence[ eventType] != NULL){
                            resetCurSequence(); //for the current sequence before starting the next
                            curSequence = optionNameToSequence[ eventType];
                        }
                        else{
                            resetCurSequence();
                            
                            curSequence = defaultSequence;
                        }
                        goTime = newGoTime;
                        timeWent = false;
                    }
                    
                    //if it's an update from the backend, parse it
                    else if ( result["backendData"].type() != Json::nullValue ){
                        cout << "update::parsing the backend Data\n";
                        parseJsonFromBackend(result["backendData"]);
                        
                    }
                    
                    else{
                        cout<< "update::unknownOutput\n";
                    }
                }
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
            //cout<< "hitting target volume\n";
        }
        else{
            //cout<< "easing volume\n";
        }
        soundPlayer.setVolume( newVolume );
    }
    
}

//--------------------------------------------------------------
void ofxTimedDrone::parseJsonFromBackend(ofxJSONElement inNode){
    //Json::Path testPath( ".weather.temperature");
    //Json::Value myTemp =  testPath.resolve(inNode);
    
    for( int i = 0; i < jsonArduinoCommands.size(); i++ ){
        CommandPath* curCommand = jsonArduinoCommands[i];
        Json::Value curResult = curCommand->path->resolve(inNode);
        if (curResult.type() != Json::nullValue){

            //send the command to the arduino
            stringstream ss (stringstream::in | stringstream::out);
            ss << curCommand->name <<":";
            switch (curResult.type()) {
                case Json::stringValue:
                    ss << curResult.asString() << "\n";
                    break;
                    
                case Json::intValue:
                case Json::uintValue:
                    ss << curResult.asInt() << "\n";
                    break;
                case Json::realValue:
                    ss << round( curResult.asFloat()) << "\n";
                    break;
                default:
                    break;
            }
            
            
            cout << "parseJsonFromBackend::command:" <<ss.str();
            curCommand->serial->writeString(ss.str());
            //curCommand->serial->writeString("go\n");
        }
        
    }
}

//--------------------------------------------------------------
void ofxTimedDrone::go(){

    curEventIndex = 0;
    timeWent = true;
}

//--------------------------------------------------------------
void ofxTimedDrone::resetCurSequence(){
    map<string, SyncedOFVideoPlayer*>::iterator iter;
    map<string, ofxAVFVideoPlayer*>::iterator iterAVF;
    map<string, ofxThreadedVideoPlayer*>::iterator iterThreaded;
    cout << "ofxTimedDrone::resetCurSequence:";
    
    switch (playerType){
        case QTKIT:
            for (iter = idToQTPlayers.begin(); iter != idToQTPlayers.end(); ++iter) {
                string id = iter->first;
                SyncedOFVideoPlayer* curPlayer = iter->second;
                
                curPlayer->setPosition(0);
                curPlayer->setPaused(true);

            }
            break;
            
            
        case AVF:
            for (iterAVF = idToAVFPlayers.begin(); iterAVF != idToAVFPlayers.end(); ++iterAVF) {
                ofxAVFVideoPlayer* curAVFPlayer = iterAVF->second;
                curAVFPlayer->setPosition(0);
                curAVFPlayer->setPaused(true);
            }
            break;
        case THREADED_AVF:
            cout << "ofxTimedDrone::resetCurSequence:";
            
            for (iterThreaded = idToThreadedPlayers.begin(); iterThreaded != idToThreadedPlayers.end(); ++iterThreaded) {
                
                cout << iterThreaded->first;
                ofxThreadedVideoPlayer* curAVFPlayer = iterThreaded->second;
                curAVFPlayer->setPosition(0);
                curAVFPlayer->setPaused( true);
            }
            
            cout << endl;
            break;
    }
    
    targetVolume = ambientVolume;

}

//--------------------------------------------------------------
void ofxTimedDrone::draw(){
    ofSetColor(255, 255, 255 );
    drawDroneVids();
    
}

//--------------------------------------------------------------
void ofxTimedDrone::updateDroneVids(){
    
    
    //hack, remove 200 ms, because it takes about 1/3rd of a second to start the play
    long long now = ofGetSystemTime() - 200;
    
    map<string, SyncedOFVideoPlayer*>::iterator iter;
    map<string, ofxAVFVideoPlayer*>::iterator iterAVF;
    map<string, ofxThreadedVideoPlayer*>::iterator iterThreaded;
    switch ( playerType ){
     case QTKIT:
            
            
            
            
            for (iter = idToQTPlayers.begin(); iter != idToQTPlayers.end(); ++iter) {
                string id = iter->first;
                SyncedOFVideoPlayer* curPlayer = iter->second;
                
                long long playHead = now - goTime - curSequence->vidStartTimes[ id ];
                if ( curPlayer->isPlaying() ){
                    curPlayer->syncToTime(playHead);
                    curPlayer->update();
                }
            }
            break;
            
            
        case AVF:
            
            for (iterAVF = idToAVFPlayers.begin(); iterAVF != idToAVFPlayers.end(); ++iterAVF) {
                string id = iterAVF->first;
                ofxAVFVideoPlayer* curAVFPlayer = iterAVF->second;
                
                
                long long playHead = now - goTime - curSequence->vidStartTimes[ id ];
                
                
                if ( curAVFPlayer->getPlaying() ){
                    curAVFPlayer->syncToTime( ((float)playHead )/1000.f);
                    curAVFPlayer->update();
                }
            }
        
            break;
            
            
        case THREADED_AVF:
            
            
            for (iterThreaded = idToThreadedPlayers.begin(); iterThreaded != idToThreadedPlayers.end(); ++iterThreaded) {
                
                string id = iterThreaded->first;
                ofxThreadedVideoPlayer* curAVFPlayer = iterThreaded->second;
                
                long long playHead = now - goTime - curSequence->vidStartTimes[id];

                if ( curAVFPlayer->isPlaying() ){
                    curAVFPlayer->syncToPlayhead( ((float)playHead )/1000.f);
                    curAVFPlayer->update();
                }
            }
        
        break;
     }
    
}


//--------------------------------------------------------------
void ofxTimedDrone::drawDroneVids(){
    int numVids;
    int vidWidth = ofGetWidth();
    map<string, SyncedOFVideoPlayer*>::iterator iter;
    map<string, ofxAVFVideoPlayer*>::iterator iterAVF;
    map<string, ofxThreadedVideoPlayer*>::iterator iterThreaded;
    
    
    switch ( playerType ){
        case QTKIT:
            
            
            
            for (iter = idToQTPlayers.begin(); iter != idToQTPlayers.end(); ++iter) {
                string id = iter->first;
                SyncedOFVideoPlayer* curPlayer = iter->second;
                
            
            
               
                if ( curPlayer->isPlaying() ){
                    //cout << "drawing qt vid:"<<i<<endl;
                    float vidHeight = vidWidth/curPlayer->getWidth() * curPlayer->getHeight();
                    //curPlayer->draw( i * vidWidth,0, vidWidth, vidHeight  );
                    //cout << "drawing qt vid:"<<i<< ", vidWidth:"<< vidWidth << ", vidHeight:" <<vidHeight << endl;
                    curPlayer->draw( 0,0, vidWidth, vidHeight  );
                    
                }
            }
        break;
            for (iterAVF = idToAVFPlayers.begin(); iterAVF != idToAVFPlayers.end(); ++iterAVF) {
                string id = iterAVF->first;
                ofxAVFVideoPlayer* curPlayer = iterAVF->second;

                
                if ( curPlayer->getPlaying() ){
                    //cout << "drawing avf vid:"<<i<<endl;
                    float vidHeight = vidWidth/curPlayer->getWidth() * curPlayer->getHeight();
                    
                    curPlayer->draw( 0,0, vidWidth, vidHeight  );
                    
                }
            }

            break;
        case THREADED_AVF:
            
            for (iterThreaded = idToThreadedPlayers.begin(); iterThreaded != idToThreadedPlayers.end(); ++iterThreaded) {
                
                string id = iterThreaded->first;
                ofxThreadedVideoPlayer* curPlayer = iterThreaded->second;
                //cout << "possibly drawing threaded vid:"<<i<<endl;

                if ( curPlayer->isPlaying() ){
                //cout << "drawing threaded vid:"<<i<<endl;
                    float vidHeight = vidWidth/curPlayer->getWidth() * curPlayer->getHeight();
                    curPlayer->draw( 0,0, vidWidth, vidHeight  );
                    
                }
            }
            break;
        }
    
    
    
    
    
}
//--------------------------------------------------------------
void ofxTimedDrone::testGo(){
    resetCurSequence();
    goTime = ofGetSystemTime();
    go();
}

//--------------------------------------------------------------
void ofxTimedDrone::testGoNow(){
    resetCurSequence();
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
