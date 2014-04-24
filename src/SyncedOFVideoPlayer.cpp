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
    
    float adjustedSpeed = CLAMP( 1.f + ( (float)curOffset )/1000.f / 5.f, .9f, 1.2f);

    //float adjustedSpeed = CLAMP( 1.f + ( curOffset )/5.f, .9f, 1.2f);
    if ( abs( adjustedSpeed - getSpeed()) > .03 ){
        cout <<"adjusting speed";
        cout << "curOffset:" << curOffset << ", adjustedSpeed:" << adjustedSpeed << ", curspeed:" <<getSpeed()<<endl;
        setSpeed( adjustedSpeed );
    }

    cout << "playhead:"<< ((float)inMSeconds)/1000.f<< "curVidTime:" << getPosition() << ", curOffset:" << curOffset << ", adjustedSpeed:" << adjustedSpeed << ", getSpeed:"<< getSpeed()<<endl;


    
}