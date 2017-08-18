// Serial code courtesy of: https://stackoverflow.com/questions/6947413/how-to-open-read-and-write-from-serial-port-in-c

#include <errno.h>
#include <fcntl.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <stdarg.h>
#include <sys/ioctl.h>
#include "mhs5200.hpp"

MHS5200Driver::MHS5200Driver() : m_fileDescriptor(0), m_maxAmplitude(20), m_attenuationMax(2), 
    m_minAmplitude(0.005), m_baudRate(B57600), m_outputDebugInfo(false) 
{
}

MHS5200Driver::~MHS5200Driver() {
    if ( m_fileDescriptor ) disconnect();
    
}

void MHS5200Driver::systemError(const char* fn, const char* msgFormat, ...) {
    va_list args;
    va_start(args, msgFormat);
    vfprintf(stderr, msgFormat, args);
    va_end(args);
}

void MHS5200Driver::debugInfo(const char *type, int bufferLen, const char *buffer) {    
    if ( !m_outputDebugInfo ) return;
    if ( bufferLen == -1 ) {
        printf("%s: %s\n", type, buffer);
    } else {
        printf("%s %d: ", type, bufferLen);
        bool lastprint = true;
        for (; bufferLen-- > 0; buffer++)
        {
            const char c = *buffer;
            if ( isprint(c) ) {
                if ( lastprint ) {
                    printf("%c",c);
                } else {
                    printf(" %c",c); 
                }
                lastprint = true;
            }
            else {
                printf(" 0x%02x", (unsigned int)c);
                lastprint = false;
            }
        }
        printf("\n");
    }
}

void MHS5200Driver::disconnect() {  
    if ( m_fileDescriptor ) {
        if (tcsetattr(m_fileDescriptor, TCSANOW, &saveTTY) != 0) {
            systemError("tcsetattr", "Error from tcsetattr: %s\n", strerror(errno));
        }
        if ( close(m_fileDescriptor) != 0 ) {
            systemError("close", "Error from close: %s\n", strerror(errno));
        }
        m_fileDescriptor = 0;
    }
}

bool MHS5200Driver::connect(const char *deviceName) {
    int fd = open(deviceName, O_RDWR | O_NOCTTY);
    if (fd < 0) {
        systemError("open", "Error opening %s: %s\n", deviceName, strerror(errno));
        return false;
    }
    
    if ( ioctl(fd, TIOCEXCL) < 0 ) {
        systemError("ioctl", "Error locking %s: %s\n", deviceName, strerror(errno));
        if ( close(fd) != 0 ) {
            systemError("close", "Error from close: %s\n", strerror(errno));
        }
        return false;
    }

    struct termios tty;

    if (tcgetattr(fd, &saveTTY) < 0) {
        systemError("tcgetattr", "Error getting tty attributes from %s: %s\n", deviceName, strerror(errno));
        if ( close(fd) != 0 ) {
            systemError("close", "Error from close: %s\n", strerror(errno));
        }
        return false;
    }
    
    memcpy(&tty,&saveTTY,sizeof(tty));


    tty.c_cflag = CS8 | HUPCL | CREAD | CLOCAL | CRTSCTS;
    tty.c_iflag = IGNBRK;
    tty.c_lflag = 0;
    tty.c_oflag = 0;
    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 1;

    cfsetospeed(&tty, (speed_t)m_baudRate);
    cfsetispeed(&tty, (speed_t)m_baudRate);
    
    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        systemError("tcsetattr", "Error from tcsetattr: %s\n", strerror(errno));
        if ( close(fd) != 0 ) {
            systemError("close", "Error from close: %s\n", strerror(errno));
        }
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
    debugInfo("write", len, command);
    if ( write(m_fileDescriptor, command, len) != len ) {
        systemError("write", "Error from write, %s\n", strerror(errno));
        return false;
    }
    if ( tcdrain(m_fileDescriptor) < 0 ) {
        systemError("tcdrain", "Error from tcdrain: %s\n", strerror(errno));
        return false;
    }
    return true;
}

const char *MHS5200Driver::rawResponse(int timeout) {
    int bytesRead = 0;
    char buf[MHS5200_BUFFER_SIZE];
    int ntries = 0;
    
    timeout *= 10;
    while (bytesRead < sizeof(buf)-1)
    {
        int rdlen = read(m_fileDescriptor, &buf[bytesRead], sizeof(buf) - 1 - bytesRead);
        ntries++;        
        if (rdlen > 0) {
            debugInfo("read", rdlen, &buf[bytesRead]);

            bytesRead += rdlen;
            buf[bytesRead] = 0;

            if ( sscanf((char *)&buf[0], ":%s\r\n", &m_responseBuffer[0]) > 0 ) {
                return m_responseBuffer;
            }
            else if ( strstr((char *)&buf[0], "ok\r\n") ) {
                strcpy(m_responseBuffer, "ok");
                return m_responseBuffer;
            }
        } else if (rdlen < 0) {
            systemError("read", "Error from read: %d: %s\n", rdlen, strerror(errno));
            break;
        } else if ( timeout >= 0 && ntries >= timeout )
        {
            systemError("timeout", "Read failed after %d seconds", timeout/10);
            break;
        }
        /* repeat read to get full message */
    } 
    return nullptr;
}

double MHS5200Driver::getFrequency(int channel) {
    debugInfo("function", -1, __FUNCTION__);
    char buffer[MHS5200_BUFFER_SIZE];
    sprintf(buffer, ":r%df\n", channel);
    if ( rawCommand(buffer) ) {
        if ( rawResponse() ) {
            int hz, fractHz;
            if ( sscanf(m_responseBuffer, "r%df%08d%02d", &channel, &hz, &fractHz ) == 3 ) {
                double result = hz;
                result += (double)fractHz/100.0;
                return result;
            }
        }
    }
    return 0;
}

bool MHS5200Driver::setFrequency(int channel, double hz) {
    debugInfo("function", -1, __FUNCTION__);
    char buffer[MHS5200_BUFFER_SIZE];
    sprintf(buffer, ":s%df%08d%02d\n", channel, (int)hz, (int)((hz-(int)hz)*100.0));
    if ( rawCommand(buffer) ) {
        if ( rawResponse() ) {
            if ( strcmp(m_responseBuffer, "ok") == 0 )
                return true;
        }
    }
    return false;
}

double MHS5200Driver::getDutyCycle(int channel) {
    debugInfo("function", -1, __FUNCTION__);
    char buffer[MHS5200_BUFFER_SIZE];
    sprintf(buffer, ":r%dd\n", channel);
    if ( rawCommand(buffer) ) {
        if ( rawResponse() ) {
            int duty;
            if ( sscanf(m_responseBuffer, "r%dd%03d", &channel, &duty) == 2 ) {
                return (double)duty/10.0;
            }
        }
    }
    return -1;
}

bool MHS5200Driver::setDutyCycle(int channel, double dutyCycle) {
    debugInfo("function", -1, __FUNCTION__);
    char buffer[MHS5200_BUFFER_SIZE];
    sprintf(buffer, ":s%dd%03d\n", channel, (int)(dutyCycle*10.0));
    if ( rawCommand(buffer) ) {
        if ( rawResponse() ) {
            if ( strcmp(m_responseBuffer, "ok") == 0 )
                return true;
        }
    }
    return false;
}

MHS5200Driver::WaveType MHS5200Driver::getWaveType(int channel) {
    debugInfo("function", -1, __FUNCTION__);
    char buffer[MHS5200_BUFFER_SIZE];
    sprintf(buffer, ":r%dw\n", channel);
    if ( rawCommand(buffer) ) {
        if ( rawResponse() ) {
            MHS5200Driver::WaveType wave;
            if ( sscanf(m_responseBuffer, "r%dw%02d", &channel, (int*)&wave) == 2 ) {
                if ( (int)wave >= 10 && (int)wave <= 25 ) wave = (MHS5200Driver::WaveType)((int)wave+22);
                return wave;
            }
        }
    }
    return MHS5200Driver::WaveType::Unknown;
}

bool MHS5200Driver::setWaveType(int channel, MHS5200Driver::WaveType wave) {
    debugInfo("function", -1, __FUNCTION__);
    char buffer[MHS5200_BUFFER_SIZE];
    sprintf(buffer, ":s%dw%d\n", channel, (int)wave);
    if ( rawCommand(buffer) ) {
        if ( rawResponse() ) {
            if ( strcmp(m_responseBuffer, "ok") == 0 )
                return true;
        }
    }
    return false;
}

int MHS5200Driver::getOffset(int channel) {
    debugInfo("function", -1, __FUNCTION__);
    char buffer[MHS5200_BUFFER_SIZE];
    sprintf(buffer, ":r%do\n", channel);
    if ( rawCommand(buffer) ) {
        if ( rawResponse() ) {
            int offset;
            if ( sscanf(m_responseBuffer, "r%do%03d", &channel, &offset) == 2 ) {
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
            if ( strcmp(m_responseBuffer, "ok") == 0 )
                return true;
        }
    }
    return false;
}

int MHS5200Driver::getPhaseOffset(int channel) {
    debugInfo("function", -1, __FUNCTION__);
    char buffer[MHS5200_BUFFER_SIZE];
    sprintf(buffer, ":r%dp\n", channel);
    if ( rawCommand(buffer) ) {
        if ( rawResponse() ) {
            int offset;
            if ( sscanf(m_responseBuffer, "r%dp%03d", &channel, &offset) == 2 ) {
                return offset;
            }
        }
    }
    return 0;
}

bool MHS5200Driver::setPhaseOffset(int channel, int phaseOffset) {
    debugInfo("function", -1, __FUNCTION__);
    char buffer[MHS5200_BUFFER_SIZE];
    sprintf(buffer, ":s%dp%03d\n", channel, phaseOffset);
    if ( rawCommand(buffer) ) {
        if ( rawResponse() ) {
            if ( strcmp(m_responseBuffer, "ok") == 0 )
                return true;
        }
    }
    return false;
}
    
double MHS5200Driver::getAmplitude(int channel) {
    debugInfo("function", -1, __FUNCTION__);
    char buffer[MHS5200_BUFFER_SIZE];
    sprintf(buffer, ":r%dy\n", channel);
    if ( rawCommand(buffer) ) {
        if ( rawResponse() && strlen(m_responseBuffer) == 4 ) {
            bool attenuated = (m_responseBuffer[3] == '0');
            sprintf(buffer, ":r%da\n", channel);
            if ( rawCommand(buffer) ) {
                if ( rawResponse() ) {
                    int volts;
                    if ( sscanf(m_responseBuffer, "r%da%04d", &channel, &volts) == 2 ) {
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
    debugInfo("function", -1, __FUNCTION__);
    char buffer[MHS5200_BUFFER_SIZE];
    if ( amplitude >= m_maxAmplitude || amplitude <= m_minAmplitude ) return false;
    bool attenuate = amplitude < m_attenuationMax;
    sprintf(buffer, ":s%dy%d\n", channel, attenuate?0:1);
    if ( rawCommand(buffer) ) {
        if ( rawResponse() ) {
            if ( strcmp(m_responseBuffer, "ok") == 0 ) {
                sprintf(buffer, ":s%da%04d\n", channel, (int)(amplitude*(attenuate?1000.0:100.0)));
                if ( rawCommand(buffer) ) {
                    if ( rawResponse() ) {
                        if ( strcmp(m_responseBuffer, "ok") == 0 )
                            return true;
                    }
                }
            }
        }
    }
    return false;
}

bool MHS5200Driver::getInverted(int channel) {
    debugInfo("function", -1, __FUNCTION__);
    char buffer[MHS5200_BUFFER_SIZE];
    sprintf(buffer, ":r%cb\n", (channel==1?'a':'b'));
    if ( rawCommand(buffer) ) {
        if ( rawResponse() ) {
            int hz, fractHz;
            char ch;
            int inverted;
            if ( sscanf(m_responseBuffer, "r%cb%d", &ch, &inverted) == 2 ) {
                return inverted;
            }
        }
    }
    return false;
}

bool MHS5200Driver::setInverted(int channel, bool inverted) {
    debugInfo("function", -1, __FUNCTION__);
    char buffer[MHS5200_BUFFER_SIZE];
    sprintf(buffer, ":s%cb%d\n", (channel==1?'a':'b'), inverted?1:0);
    if ( rawCommand(buffer) ) {
        if ( rawResponse() ) {
            if ( strcmp(m_responseBuffer, "ok") == 0 )
                return true;
        }
    }
    return false;
}

int MHS5200Driver::getCurrentChannel() {
    debugInfo("function", -1, __FUNCTION__);
    char buffer[MHS5200_BUFFER_SIZE];
    strcpy(buffer, ":r2b\n");
    if ( rawCommand(buffer) ) {
        if ( rawResponse() ) {
            int channel;
            if ( sscanf(m_responseBuffer, "r2b%d", &channel) == 1 ) 
                return channel;
        }
    }
    return 0;
}

bool MHS5200Driver::setCurrentChannel(int channel) {
    debugInfo("function", -1, __FUNCTION__);
    char buffer[MHS5200_BUFFER_SIZE];
    sprintf(buffer, ":s2b%d\n", channel);
    if ( rawCommand(buffer) ) {
        if ( rawResponse() ) {
            if ( strcmp(m_responseBuffer, "ok") == 0 )
                return true;
        }
    }
    return false;
}

bool MHS5200Driver::getCurrentChannelStatus() {
    debugInfo("function", -1, __FUNCTION__);
    char buffer[MHS5200_BUFFER_SIZE];
    strcpy(buffer, ":r1b\n");
    if ( rawCommand(buffer) ) {
        if ( rawResponse() ) {
            int status;
            if ( sscanf(m_responseBuffer, "r1b%d", &status) == 1 ) {
                return status;
            }
        }
    }
    return false;
}

bool MHS5200Driver::setCurrentChannelStatus(bool onOff) {
    debugInfo("function", -1, __FUNCTION__);
    char buffer[MHS5200_BUFFER_SIZE];
    sprintf(buffer, ":s1b%d\n", onOff?1:0);
    if ( rawCommand(buffer) ) {
        if ( rawResponse() ) {
            if ( strcmp(m_responseBuffer, "ok") == 0 )
                return true;
        }
    }
    return false;
}

bool MHS5200Driver::setArbitrary(int arbitrary, const int values[1024]) {
    debugInfo("function", -1, __FUNCTION__);
    char buffer[1024];
    char buffer2[MHS5200_BUFFER_SIZE];
    int p = 0;
    for ( int chunk = 0; chunk < 16; chunk++ ) {
        sprintf(buffer, ":a%1X%1X", arbitrary, chunk);
        for ( int c = 0; c < 64; c++ ) {
            if (c!=0)
                strcat(buffer, ",");
            sprintf(buffer2, "%d", (int)values[p++]);
            strcat(buffer, buffer2);
        }
        strcat(buffer, "\n");
        if ( rawCommand(buffer) ) {
            if ( rawResponse() ) {
                if ( strcmp(m_responseBuffer, "ok") != 0 )
                    return false;
            }
        }
    }
    return true;
}

bool MHS5200Driver::saveSettings(int slot) {
    debugInfo("function", -1, __FUNCTION__);
    char buffer[MHS5200_BUFFER_SIZE];
    sprintf(buffer, ":s%du\n", slot);
    if ( rawCommand(buffer) ) {
        if ( rawResponse() ) {
            if ( strcmp(m_responseBuffer, "ok") == 0 )
                return true;
        }
    }
    return false;
}

bool MHS5200Driver::loadSettings(int slot) {
    debugInfo("function", -1, __FUNCTION__);
    char buffer[MHS5200_BUFFER_SIZE];
    sprintf(buffer, ":s%dv\n", slot);
    if ( rawCommand(buffer) ) {
        if ( rawResponse() ) {
            if ( strcmp(m_responseBuffer, "ok") == 0 )
                return true;
        }
    }
    return false;
}

void MHS5200Driver::setDebugOutput(bool onOff) {
    m_outputDebugInfo = onOff;
}
