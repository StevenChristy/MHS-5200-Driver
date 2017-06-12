#include <termios.h>

#define MHS5200_BUFFER_SIZE 128

class MHS5200Driver
{
protected:
    struct termios saveTTY;
    int m_fileDescriptor;
    char m_responseBuffer[MHS5200_BUFFER_SIZE];
    double m_maxAmplitude;
    double m_attenuationMax;
    double m_minAmplitude;
    int m_baudRate;
    bool m_outputDebugInfo;

    void debugInfo(const char *type, int bufferLen, const char *buffer);
    void systemError(const char *fn, const char *msgFormat, ...);
public:
    /**
     * Waves types supported by the MHS-5200.
     */
    enum WaveType  {Unknown=-1, Sine, Square, Triangle, Sawtooth, SawtoothReverse, Arbitrary0=32, Arbitrary1, Arbitrary2,
                    Arbitrary3, Arbitrary4, Arbitrary5, Arbitrary6, Arbitrary7, Arbitrary8, Arbitrary9, 
                    Arbitrary10, Arbitrary11, Arbitrary12, Arbitrary13, Arbitrary14, Arbitrary15};
    
    MHS5200Driver();
    ~MHS5200Driver();
    
    /**
     * Connect to a specified TTY device. ie. /dev/ttyUSB0.
     * 
     * @param deviceName TTY device to connect too.
     * @return True if successful, otherwise false.
     */   
    bool connect(const char *deviceName);
    
    /**
     * Disconnect from TTY.
     */
    void disconnect();
    
    /**
     * Determine if we are connected to a TTY device.
     * 
     * @return True if connected.
     */
    bool isConnected();
    
    /**
     * Get current frequency setting in Hz.
     * 
     * @param channel The channel to set the frequency. For MHS-5200 this should be 1 or 2.
     * @return Frequency in Hz up to 2 decimal places of precision.
     */
    double getFrequency(int channel);

    /**
     * Set the current frequency for a given channel in Hz.
     * 
     * @param channel The channel to set the frequency. For MHS-5200 this should be 1 or 2.
     * @param hz Frequency in Hz (decimal 8.2). Example: 10000000.00 = 10MHz
     */
    bool setFrequency(int channel, double hz);
    
    /**
     * Get duty cycle for a given channel.
     * 
     * @param channel The channel to set the duty cycle. For MHS-5200 this should be 1 or 2.
     * @return 0.1-99.9 if successful or 0 on failure.
     */
    double getDutyCycle(int channel);

    /**
     * Get duty cycle for a given channel.
     * 
     * @param channel The channel to set the duty cycle. For MHS-5200 this should be 1 or 2.
     * @return 0-100 if successful or -1 on failure.
     */
    bool setDutyCycle(int channel, double dutyCycle);
    
    /**
     * Get waveform supplied by the specified channel.
     * 
     * @param channel The channel to set the wave form. For MHS-5200 this should be 1 or 2.
     * @return See enumeration MHS5200Driver::WaveType. Returns MHS5200Driver::WaveType::Unknown on error.
     */
    WaveType getWaveType(int channel);

    /**
     * Set waveform for a given channel.
     * 
     * @param channel The channel to set the waveform. For MHS-5200 this should be 1 or 2.
     * @param wave The wave form to use.
     * @return True on success.
     */
    bool setWaveType(int channel, WaveType wave);
    
    /**
     * Get the offset for the given channel.
     * 
     * @param channel The channel to query the offset.
     * @return The offset between -120 and 120.
     */
    int getOffset(int channel);
    
    /**
     * Set the channel's offset.
     * 
     * @param channel The channel to set.
     * @param offset The offset value to use between -120 and 120.
     * @return True on success.
     */
    bool setOffset(int channel, int offset);
    
    /**
     * Get the channel's amplitude in volts between 20.00 and 0.005.
     * 
     * @param channel The channel to set.
     * @return The amplitude for the specified channel.
     */
    double getAmplitude(int channel);
    
    /**
     * Set the peak to peek amplitude of the waveform in volts.
     * 
     * @param channel The channel to set the amplitude for.
     * @param amplitude The amplitude in volts between 0.005 and 20.00.
     * @return True on success.
     */
    bool setAmplitude(int channel, double amplitude);
    
    /**
     * Get the channel's phase offset.
     * 
     * @param channel The channel to query.
     * @return Phase offset between 0 and 360.
     */
    int getPhaseOffset(int channel);
    
    /**
     * Set the channel's phase offset.
     * 
     * @param channel The channel to set.
     * @param phaseOffset The phase offset between 0 and 360.
     * @return True on success.
     */
    bool setPhaseOffset(int channel, int phaseOffset);
    
    /**
     * Get the inversion status of the specified channel.
     * 
     * @param channel The channel to query.
     * @return True if inverted.
     */
    bool getInverted(int channel);
    
    /**
     * Set the channel inverted.
     * 
     * @param channel The channel to set.
     * @return True on success.
     */
    bool setInverted(int channel, bool inverted);
    
    /**
     * Get currently selected channel.
     * 
     * @return Channel 1, 2, or 0 on failure.
     */
    int getCurrentChannel();

    /**
     * Set currently selected channel.
     * 
     * @param channel The channel to set active.
     * @return True on success.
     */
    bool setCurrentChannel(int channel);
    
    /**
     * Get status of current device channel (on or off).
     * 
     * @return True when channel is turned on.
     */
    bool getCurrentChannelStatus();
    
    /**
     * Turn current channel output on or off.
     * 
     * @param onOff True for on, false for off.
     * @return True on success
     */
    bool setCurrentChannelStatus(bool onOff);

    /**
     * Save settings in device memory slot.
     * 
     * @param slot Slot to save current settings too (0-9).
     * @return True on success.
     */
    bool saveSettings(int slot);
    
    /**
     * Load settings from device memory slot.
     * 
     * @param slot Slot to load current settings from (0-9).
     * @return True on success.
     */
    bool loadSettings(int slot);

    /**
     * Send a raw command string to the signal generator.
     * 
     * @param command String containing the command to send. This value is sent as is with no pre-processing.
     * @return True on successful send (does not mean the command was successful).
     */
    bool rawCommand(const char *command);
    
    /**
     * Receive a raw response.
     * 
     * @param timeout Time to wait in seconds.
     * @return String containing the return value less the ':' and traiiling \r\n or the string value "ok" when the device responds with ok\r\n. On error or timeout this will be a nullptr.
     */
    const char *rawResponse(int timeout = 10);
    
    /**
     *  Turns debug output on or off.
     * @param onOff True for on, false for off.
     */
    void setDebugOutput( bool onOff );
    
};

