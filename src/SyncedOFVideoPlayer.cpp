//
//  SyncedOFVideoPlayer.cpp
//  qtkitPlayerSynchronizer
//
//  Created by Michael Manh on 4/21/14.
//
//

#include "SyncedOFVideoPlayer.h"

SyncedOFVideoPlayer::SyncedOFVideoPlayer(){
    ofVideoPlayer::ofVideoPlayer();
}

void SyncedOFVideoPlayer::syncToTime( int inMSeconds ){
    long long curOffset = (inMSeconds - (getPosition() * getDuration() * 1000) );
    
    float adjustedSpeed = CLAMP( 1.f + ( (float)curOffset )/1000.f, .9f, 1.5f);
    cout << "curOffset:" << curOffset << ", adjustedSpeed:" << adjustedSpeed <<endl;
    setSpeed( adjustedSpeed );

    
}