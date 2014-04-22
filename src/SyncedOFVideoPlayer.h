//
//  SyncedOFVideoPlayer.h
//  qtkitPlayerSynchronizer
//
//  Created by Michael Manh on 4/21/14.
//
//

#ifndef __qtkitPlayerSynchronizer__SyncedOFVideoPlayer__
#define __qtkitPlayerSynchronizer__SyncedOFVideoPlayer__
#include "ofVideoPlayer.h"
#include <iostream>
class SyncedOFVideoPlayer: public ofVideoPlayer{
public:
    SyncedOFVideoPlayer();
    
    void syncToTime( int inMSeconds );
    
    
};

#endif /* defined(__qtkitPlayerSynchronizer__SyncedOFVideoPlayer__) */
