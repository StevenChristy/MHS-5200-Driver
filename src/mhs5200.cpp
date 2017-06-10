// Serial code courtesy of: https://stackoverflow.com/questions/6947413/how-to-open-read-and-write-from-serial-port-in-c

#include <errno.h>
#include <fcntl.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <ctype.h>
#include "mhs5200.hpp"

MHS5200Driver::MHS5200Driver() : m_fileDescriptor(0), maxAmplitude(20), attenuationMax(2), minAmplitude(0.005) {
}

MHS5200Driver::~MHS5200Driver() {
    if ( m_fileDescriptor ) disconnect();
    
}

void MHS5200Driver::disconnect() {  
    if ( m_fileDescriptor ) {
        close(m_fileDescriptor);
        m_fileDescriptor = 0;
    }
}

bool MHS5200Driver::connect(const char *deviceName) {
    int fd = open(deviceName, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        MHS5200_DEBUG_FN("Error opening %s: %s\n", deviceName, strerror(errno));
        return false;
    }

    struct termios tty;

    if (tcgetattr(fd, &tty) < 0) {
        MHS5200_DEBUG_FN("Error from tcgetattr: %s\n", strerror(errno));
        close(fd);
        return false;
    }

    cfsetospeed(&tty, (speed_t)MHS5200_DEVICE_BAUD_RATE);
    cfsetispeed(&tty, (speed_t)MHS5200_DEVICE_BAUD_RATE);

    tty.c_cflag = 0x80001CB1;
    tty.c_iflag = 0;
    tty.c_lflag = 0;
    tty.c_oflag = 0;
    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 1;

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        MHS5200_DEBUG_FN("Error from tcsetattr: %s\n", strerror(errno));
        close(fd);
        return false;
    }
    
    m_fileDescriptor = fd;
    return true;
}

bool MHS5200Driver::isConnected() {
    return m_fileDescriptor != 0;
}

bool MHS5200Driver::rawCommand(const char *command) {
    int len = strlen(command);
    if ( write(m_fileDescriptor, command, len) != len ) {
        MHS5200_DEBUG_FN("Error from write: %d, %d\n", len, errno);
        return false;
    }
    if ( tcdrain(m_fileDescriptor) < 0 ) {
        MHS5200_DEBUG_FN("Error from tcdrain: %s\n", strerror(errno));
        return false;
    }
    return true;
}

const char *MHS5200Driver::rawResponse(int timeout) {
    int bytesRead = 0;
    unsigned char buf[MHS5200_BUFFER_SIZE];
    int ntries = 0;
    
    timeout *= 10;
    while (bytesRead < sizeof(buf)-1)
    {
        int rdlen = read(m_fileDescriptor, &buf[bytesRead], sizeof(buf) - 1 - bytesRead);
        ntries++;        
        if (rdlen > 0) {
            bytesRead += rdlen;
            buf[bytesRead] = 0;
#ifdef DISPLAY_STRING
            MHS5200_DEBUG_FN("Read %d: \"%s\"\n", rdlen, buf);
#else /* display hex */
            unsigned char   *p;
            MHS5200_DEBUG_FN("Read %d: ", rdlen);
            bool lastprint = true;
            for (p = &buf[bytesRead-rdlen]; rdlen-- > 0; p++)
            {
                unsigned char c = *p;
                if ( isprint(c) ) {
                    if ( lastprint ) {
                        printf("%c",c);
                    } else {
                        printf(" %c",c); 
                    }
                    lastprint = true;
                }
                else {
                    printf(" 0x%02x", *p);
                    lastprint = false;
                }
            }
            MHS5200_DEBUG_FN("\n");
#endif
            if ( sscanf((char *)&buf[0], ":%s\r\n", &responseBuffer[0]) > 0 ) {
                return responseBuffer;
            }
            else if ( strstr((char *)&buf[0], "ok\r\n") ) {
                strcpy(responseBuffer, "ok");
                return responseBuffer;
            }
            
        } else if (rdlen < 0) {
            MHS5200_DEBUG_FN("Error from read: %d: %s\n", rdlen, strerror(errno));
            break;
        } else if ( timeout >= 0 && ntries >= timeout )
        {
            MHS5200_DEBUG_FN("Read failed after %d seconds",timeout/10);
            break;
        }
        /* repeat read to get full message */
    } 
    return nullptr;
}

double MHS5200Driver::getFrequency(int channel) {
    char buffer[MHS5200_BUFFER_SIZE];
    sprintf(buffer, ":r%df\n", channel);
    if ( rawCommand(buffer) ) {
        if ( rawResponse() ) {
            int hz, fractHz;
            if ( sscanf(responseBuffer, "r%df%08d%02d", &channel, &hz, &fractHz ) == 3 ) {
                double result = hz;
                result += (double)fractHz/100.0;
                return result;
            }
        }
    }
    return 0;
}

bool MHS5200Driver::setFrequency(int channel, double hz) {
    char buffer[MHS5200_BUFFER_SIZE];
    sprintf(buffer, ":s%df%08d%02d\n", channel, (int)hz, (int)((hz-(int)hz)*100.0));
    if ( rawCommand(buffer) ) {
        if ( rawResponse() ) {
            if ( strcmp(responseBuffer, "ok") == 0 )
                return true;
        }
    }
    return false;
}

double MHS5200Driver::getDutyCycle(int channel) {
    char buffer[MHS5200_BUFFER_SIZE];
    sprintf(buffer, ":r%dd\n", channel);
    if ( rawCommand(buffer) ) {
        if ( rawResponse() ) {
            int duty;
            if ( sscanf(responseBuffer, "r%dd%03d", &channel, &duty) == 2 ) {
                return (double)duty/10.0;
            }
        }
    }
    return -1;
}

bool MHS5200Driver::setDutyCycle(int channel, double dutyCycle) {
    char buffer[MHS5200_BUFFER_SIZE];
    sprintf(buffer, ":s%dd%03d\n", channel, (int)(dutyCycle*10.0));
    if ( rawCommand(buffer) ) {
        if ( rawResponse() ) {
            if ( strcmp(responseBuffer, "ok") == 0 )
                return true;
        }
    }
    return false;
}

MHS5200Driver::WaveType MHS5200Driver::getWaveType(int channel) {
    char buffer[MHS5200_BUFFER_SIZE];
    sprintf(buffer, ":r%dw\n", channel);
    if ( rawCommand(buffer) ) {
        if ( rawResponse() ) {
            MHS5200Driver::WaveType wave;
            if ( sscanf(responseBuffer, "r%dw%02d", &channel, (int*)&wave) == 2 ) {
                if ( (int)wave >= 10 && (int)wave <= 25 ) wave = (MHS5200Driver::WaveType)((int)wave+22);
                return wave;
            }
        }
    }
    return MHS5200Driver::WaveType::Unknown;
}

bool MHS5200Driver::setWaveType(int channel, MHS5200Driver::WaveType wave) {
    char buffer[MHS5200_BUFFER_SIZE];
    sprintf(buffer, ":s%dw%d\n", channel, (int)wave);
    if ( rawCommand(buffer) ) {
        if ( rawResponse() ) {
            if ( strcmp(responseBuffer, "ok") == 0 )
                return true;
        }
    }
    return false;
}

int MHS5200Driver::getOffset(int channel) {
    char buffer[MHS5200_BUFFER_SIZE];
    sprintf(buffer, ":r%do\n", channel);
    if ( rawCommand(buffer) ) {
        if ( rawResponse() ) {
            int offset;
            if ( sscanf(responseBuffer, "r%do%03d", &channel, &offset) == 2 ) {
                return offset-120;
            }
        }
    }
    return 0;
}

bool MHS5200Driver::setOffset(int channel, int offset) {
    char buffer[MHS5200_BUFFER_SIZE];
    sprintf(buffer, ":s%do%03d\n", channel, offset+120);
    if ( rawCommand(buffer) ) {
        if ( rawResponse() ) {
            if ( strcmp(responseBuffer, "ok") == 0 )
                return true;
        }
    }
    return false;
}

int MHS5200Driver::getPhaseOffset(int channel) {
    char buffer[MHS5200_BUFFER_SIZE];
    sprintf(buffer, ":r%dp\n", channel);
    if ( rawCommand(buffer) ) {
        if ( rawResponse() ) {
            int offset;
            if ( sscanf(responseBuffer, "r%dp%03d", &channel, &offset) == 2 ) {
                return offset;
            }
        }
    }
    return 0;
}

bool MHS5200Driver::setPhaseOffset(int channel, int phaseOffset) {
    char buffer[MHS5200_BUFFER_SIZE];
    sprintf(buffer, ":s%dp%03d\n", channel, phaseOffset);
    if ( rawCommand(buffer) ) {
        if ( rawResponse() ) {
            if ( strcmp(responseBuffer, "ok") == 0 )
                return true;
        }
    }
    return false;
}
    
double MHS5200Driver::getAmplitude(int channel) {
    char buffer[MHS5200_BUFFER_SIZE];
    sprintf(buffer, ":r%dy\n", channel);
    if ( rawCommand(buffer) ) {
        if ( rawResponse() && strlen(responseBuffer) == 4 ) {
            bool attenuated = (responseBuffer[3] == '0');
            sprintf(buffer, ":r%da\n", channel);
            if ( rawCommand(buffer) ) {
                if ( rawResponse() ) {
                    int volts;
                    if ( sscanf(responseBuffer, "r%da%04d", &channel, &volts) == 2 ) {
                        double result = volts;
                        if ( attenuated ) result /= 1000.0;
                        else result /= 100.0;
                        return result;
                    }
                }
            }
        }
    }    
    return 0;
}

bool MHS5200Driver::setAmplitude(int channel, double amplitude) {
    char buffer[MHS5200_BUFFER_SIZE];
    if ( amplitude >= maxAmplitude || amplitude <= minAmplitude ) return false;
    bool attenuate = amplitude < attenuationMax;
    sprintf(buffer, ":s%dy%d\n", channel, attenuate?0:1);
    if ( rawCommand(buffer) ) {
        if ( rawResponse() ) {
            if ( strcmp(responseBuffer, "ok") == 0 ) {
                sprintf(buffer, ":s%da%04d\n", channel, (int)(amplitude*(attenuate?1000.0:100.0)));
                if ( rawCommand(buffer) ) {
                    if ( rawResponse() ) {
                        if ( strcmp(responseBuffer, "ok") == 0 )
                            return true;
                    }
                }
            }
        }
    }
    return false;
}

bool MHS5200Driver::getInverted(int channel) {
    char buffer[MHS5200_BUFFER_SIZE];
    sprintf(buffer, ":r%cb\n", (channel==1?'a':'b'));
    if ( rawCommand(buffer) ) {
        if ( rawResponse() ) {
            int hz, fractHz;
            char ch;
            int inverted;
            if ( sscanf(responseBuffer, "r%cb%d", &ch, &inverted) == 2 ) {
                return inverted;
            }
        }
    }
    return false;
}

bool MHS5200Driver::setInverted(int channel, bool inverted) {
    char buffer[MHS5200_BUFFER_SIZE];
    sprintf(buffer, ":s%cb%d\n", (channel==1?'a':'b'), inverted?1:0);
    if ( rawCommand(buffer) ) {
        if ( rawResponse() ) {
            if ( strcmp(responseBuffer, "ok") == 0 )
                return true;
        }
    }
    return false;
}

int MHS5200Driver::getCurrentChannel() {
    char buffer[MHS5200_BUFFER_SIZE];
    strcpy(buffer, ":r2b\n");
    if ( rawCommand(buffer) ) {
        if ( rawResponse() ) {
            int channel;
            if ( sscanf(responseBuffer, "r2b%d", &channel) == 1 ) 
                return channel;
        }
    }
    return 0;
}

bool MHS5200Driver::setCurrentChannel(int channel) {
    char buffer[MHS5200_BUFFER_SIZE];
    sprintf(buffer, ":s2b%d\n", channel);
    if ( rawCommand(buffer) ) {
        if ( rawResponse() ) {
            if ( strcmp(responseBuffer, "ok") == 0 )
                return true;
        }
    }
    return false;
}

bool MHS5200Driver::getCurrentChannelStatus() {
    char buffer[MHS5200_BUFFER_SIZE];
    strcpy(buffer, ":r1b\n");
    if ( rawCommand(buffer) ) {
        if ( rawResponse() ) {
            int status;
            if ( sscanf(responseBuffer, "r1b%d", &status) == 1 ) {
                return status;
            }
        }
    }
    return false;
}

bool MHS5200Driver::setCurrentChannelStatus(bool onOff) {
    char buffer[MHS5200_BUFFER_SIZE];
    sprintf(buffer, ":s1b%d\n", onOff?1:0);
    if ( rawCommand(buffer) ) {
        if ( rawResponse() ) {
            if ( strcmp(responseBuffer, "ok") == 0 )
                return true;
        }
    }
    return false;
}

bool MHS5200Driver::saveSettings(int slot) {
    char buffer[MHS5200_BUFFER_SIZE];
    sprintf(buffer, ":s%du\n", slot);
    if ( rawCommand(buffer) ) {
        if ( rawResponse() ) {
            if ( strcmp(responseBuffer, "ok") == 0 )
                return true;
        }
    }
    return false;
}

bool MHS5200Driver::loadSettings(int slot) {
    char buffer[MHS5200_BUFFER_SIZE];
    sprintf(buffer, ":s%dv\n", slot);
    if ( rawCommand(buffer) ) {
        if ( rawResponse() ) {
            if ( strcmp(responseBuffer, "ok") == 0 )
                return true;
        }
    }
    return false;
}
