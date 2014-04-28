//
//  SyncSequence.cpp
//  SyncedDroneAddon
//
//  Created by Michael Manh on 4/28/14.
//
//

#include "SyncSequence.h"

SyncSequence::SyncSequence( string inType){
    type = inType;
}

//used for sorting events
bool isEventEarlier(FireEvent* i,FireEvent* j) { return (i->fireTime < j->fireTime); }


void SyncSequence::sortList(){
    sort( droneEventList.begin(), droneEventList.end(), isEventEarlier);
    cout << "SyncSequence::sortList::fireTimes:";
    for( int i = 0; i < droneEventList.size();i++ ){
        
        cout << droneEventList[ i ]->fireTime << ",";
    }
    cout <<endl;
}