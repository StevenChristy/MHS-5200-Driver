#include "mhs5200.hpp"
#include <malloc.h>
#include <iostream>
#include <iomanip>

int main( int argc, char *argv[] ) 
{
    MHS5200Driver signalGenerator;
    
    if ( signalGenerator.connect("/dev/ttyUSB0") ) {
        
        
        /*
        std::cout << " Set Frequency " << signalGenerator.setFrequency(2,150012.861) << std::endl;
        std::cout << " Freqency = " << std::setiosflags(std::ios::fixed) << std::setprecision(3) << signalGenerator.getFrequency(2) << std::endl;
        std::cout << " Set Duty Cycle " << signalGenerator.setDutyCycle(2,15) << std::endl;
        std::cout << " Duty Cycle = " << signalGenerator.getDutyCycle(2) << std::endl;
        std::cout << " Set Wave Type " << signalGenerator.setWaveType(2,MHS5200Driver::WaveType::Sine) << std::endl;
        std::cout << " Wave = " << signalGenerator.getWaveType(2) << std::endl;
    //    std::cout << " Set Amplitude " << signalGenerator.setAmplitude(2,1.245) << std::endl;
        std::cout << " Amplitude = " << signalGenerator.getAmplitude(2) << std::endl;
        signalGenerator.loadSettings(1);
        */
        
        signalGenerator.setPhaseOffset(2, 125);
        std::cout << signalGenerator.getPhaseOffset(2) << std::endl;
    }
}
