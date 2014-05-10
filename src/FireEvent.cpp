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
    
    if (type == ""){
        
        cout << "Fireevent why" <<endl;
    }
    
    
}

//for tag type instead of id type
void FireEvent::setTags(ofxJSONElement inNode){
    
    int numTags = inNode.size();
    if ( inNode.size() < 1 ){
        //error
        throw "tagsEmpty";
    }
    for ( int i = 0; i < numTags; i++ ){
        ofxJSONElement curTagNode = inNode[ i ];
        string curTag = curTagNode[ "tag"].asString();
        vector<string>* tagIDs = new vector<string>(); //now we're getting pretty abstract
        
        ofxJSONElement idsNode = curTagNode[ "id"];
        
        int numIDs = idsNode.size();
        for ( int j = 0; j < numIDs; j++ ){
            tagIDs->push_back(idsNode[ j].asString());
        }
        curTag = ofToLower(curTag);
        tagToIds[ curTag ] = tagIDs;
        //give it at least a single vidID
        //vidID = (*tagIDs)[ 0 ]; //ugh, dereference pointer to access element
        
    }
    
}

void FireEvent::setIDs(ofxJSONElement inNode){
    int numIDs;
    
    switch( inNode.type() ){
            
            //it is just a single id to play
        case Json::stringValue:
            vidID = inNode.asString();
            vidIDs.push_back( inNode.asString() );
            break;
            
            //it is a set of ids to play or cycle between
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


void FireEvent::setIDsAsTags(ofxJSONElement inNode){
    int numIDs;
    
    string curTag;
    vector<string>* tagIDs;
    string curID;
    
    switch( inNode.type() ){
            
            //it is just a single id to play
        case Json::stringValue:
            curTag = vidID = inNode.asString();

            curTag = ofToLower(curTag);
            
            vidIDs.push_back( inNode.asString() );
            tagIDs = new vector<string>(); //now we're getting pretty abstract
            tagToIds[ curTag ] = tagIDs;
            break;
            
            //it is a set of ids to play or cycle between
        case Json::arrayValue:
            
            
            
            numIDs = inNode.size();
            if ( numIDs < 1 ){
                return;
            }
            for( int i = 0; i < numIDs; i++ ){
                tagIDs = new vector<string>(); //now we're getting pretty abstract
                curID = inNode[ i ].asString();
                tagIDs->push_back( curID );
                curTag = ofToLower(curID);
                tagToIds[ curTag ] = tagIDs;
            }
            //curTag = vidID = (*tagIDs)[ 0 ];
            
            
            break;
            
        default:
            cout <<"FireEvent::setIDs::error!, unknown type:" <<  inNode.type() << endl;
            cout << "inNode:" << inNode.toStyledString();
            //throw "wtf is this?";
    }
    
    
}



void FireEvent::cycle( string inTag ){
    
    inTag = ofToLower(inTag);
    
    
    cout << "FireEvent::cycle:" <<inTag <<endl; //pick the next thing
    if ( vidIDs.size() == 0 ){ //there are no vidIDs, so it must have tags
        //pick from some tags
        
        //USE AT instead of [ ] or else it will create an element
        vector<string>* curIDs;
        if ( tagToIds.count(inTag)) {
            curIDs = tagToIds.at( inTag );
            
        }
        else{

            //just pick the first item if the tag doesn't exist
            curIDs = tagToIds.begin()->second ; //hack to have graceful failure
            
        }
        
        //this kind of sucks, but i don't want to keep an index per tag
        curVidIDIndex++;
        if (curVidIDIndex >= curIDs->size() ){
            curVidIDIndex = 0;
        }
        vidID = (*curIDs)[ curVidIDIndex];
    }
    
    //if it's not cycle dependent
    else{
        curVidIDIndex++;
        if (curVidIDIndex >= vidIDs.size() ){
            curVidIDIndex = 0;
        }
        vidID = vidIDs[ curVidIDIndex ];
    }
}