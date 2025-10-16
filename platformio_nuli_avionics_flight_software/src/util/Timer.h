#ifndef TIMER_H
#define TIMER_H

#include "Avionics.h"

// class Timer {
// public:
//     void start(uint32_t ms);

//     hasReached(unsigned long timeFromStart);

// private:
// };

/*
    m_startTime = Actual Start Time by the System
    m_durationTime = Actual Duration Time by the System

*/

class Alarm{
    uint32_t m_startTime, m_durationTime;

    public:
        void startAlarm(uint32_t startTime, uint32_t durationTime){
            m_startTime = startTime;
            m_durationTime = durationTime;
        }

        bool isAlarmFinished(uint32_t currentTime){
            if(currentTime >= m_durationTime + m_startTime){
                return true;
            } else {
                return false;
            }
        }

        uint32_t getTimeElapsed(uint32_t currentTime){
            return (currentTime - m_startTime);
        }

        uint32_t timeRemaining(uint32_t currentTime){
            uint32_t time = getTimeElapsed(currentTime);
            return (m_durationTime - time);
        }
};

class StopWatch{
    uint32_t m_startTime, m_stoppedTimeElapsed;
    bool error = false;
    
    public:
        void startWatch(uint32_t startTime){
            m_startTime = startTime;
        }

        uint32_t getTimeElapsed(uint32_t currentTime){
            return (currentTime - m_startTime);
        }

        uint32_t getTimeUntil(uint32_t currentTime, uint32_t enterTime){
            if(enterTime >= currentTime){
                return (enterTime - currentTime);
            } else {
                bool error = true;
                return 0;
            }
        }

        uint32_t stopWatch(uint32_t currentTime){
            m_stoppedTimeElapsed = getTimeElapsed(currentTime);
            return m_stoppedTimeElapsed;
        }

        void returnError(){
            if(error){
                return;
            }
        }
};

#endif //TIMER_H