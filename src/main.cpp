#include "mhs5200.hpp"
#include <malloc.h>
#include <iostream>
#include <iomanip>
#include <functional>
#include <map>
#include <vector>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <fstream>
#include <sstream>
using namespace std;

const char *waveToString( MHS5200Driver::WaveType wave ) {
    switch ( wave ) {
        case MHS5200Driver::WaveType::Sine: return "Sine";
        case MHS5200Driver::WaveType::Square: return "Square";
        case MHS5200Driver::WaveType::Triangle: return "Tri";
        case MHS5200Driver::WaveType::Sawtooth: return "Saw";
        case MHS5200Driver::WaveType::SawtoothReverse: return "SawRev";
        case MHS5200Driver::WaveType::Arbitrary0: return "Arb0";
        case MHS5200Driver::WaveType::Arbitrary1: return "Arb1";
        case MHS5200Driver::WaveType::Arbitrary2: return "Arb2";
        case MHS5200Driver::WaveType::Arbitrary3: return "Arb3";
        case MHS5200Driver::WaveType::Arbitrary4: return "Arb4";
        case MHS5200Driver::WaveType::Arbitrary5: return "Arb5";
        case MHS5200Driver::WaveType::Arbitrary6: return "Arb6";
        case MHS5200Driver::WaveType::Arbitrary7: return "Arb7";
        case MHS5200Driver::WaveType::Arbitrary8: return "Arb8";
        case MHS5200Driver::WaveType::Arbitrary9: return "Arb9";
        case MHS5200Driver::WaveType::Arbitrary10: return "Arb10";
        case MHS5200Driver::WaveType::Arbitrary11: return "Arb11";
        case MHS5200Driver::WaveType::Arbitrary12: return "Arb12";
        case MHS5200Driver::WaveType::Arbitrary13: return "Arb13";
        case MHS5200Driver::WaveType::Arbitrary14: return "Arb14";
        case MHS5200Driver::WaveType::Arbitrary15: return "Arb15";
    }
    return nullptr;
}

bool parseInt(const char *str, int &value) {
    char *p = nullptr;
    int i = (int) strtol(str, &p, 10);
    if ( p == nullptr || *p != 0 || p == str) {
        return false;
    }
    value = i;
    return true;
}

bool parseDouble(const char *str, double &value) {
    char *p = nullptr;
    double d = strtod(str, &p);
    if ( p == nullptr || *p != 0 || p == str) {
        return false;
    }
    value = d;
    return true;
};

void raise_expected_more_argments( const char *command ) {
    stringstream ss;
    ss << "Error: ";
    ss << command;
    ss << " Expected more arguments.";
    throw ss.str();
}

void raise_expected_argument( const char *command, const char *argnickname, const char *suggestedValues, const char *invalidValue ) {
    stringstream ss;
    ss << "Error: Invalid argument for " << command << ". Expected " << argnickname << " with value of " << suggestedValues << ". " << invalidValue << " not accepted.";
    throw ss.str();
}

void raise_error_parsing_file( const char *command, const char *fileName ) {
    stringstream ss;
    ss << "Error: Parsing file "<< fileName << " in "  << command << ".";
    throw ss.str();
}




int main( int argc, const char *argv[] ) 
{
    const char *deviceName;
    MHS5200Driver signalGenerator;
    int currentChannel = -1;
    int argp = 1;
    vector< function<void()> > commandChain;
    map<string, function<void(int argc, const char *argv[])> > commandParser;
    
    try {
        commandParser["-?"] = [](int argc, const char *argv[])->void {
            printf("Usage: %s <tty device> <command list>\n\n", argv[0]);
            printf(" Command List:\n");
            printf("\t-?, --help\t\tShows this information and terminate.\n");
            printf("\tchannel 1/2\t\tSet channel to use for subsequent commands.\n");
            printf("\tdebug\t\t\tOutput debug trace information.\n");
            printf("\tstatus\t\t\tShows this command information.\n");
            printf("\tfreq, frequency <hz>\tSet frequency in hz.\n");
            printf("\tduty <percent>\t\tSet duty cycle percent.\n");
            printf("\tamplitude <volts>\tSet peek to peek amplitude of the wave.\n");
            printf("\tprogram <0-15> <file>\tProgram arbitrary wave form using file.\n");
            printf("\toffset <+/-120>\t\tSets the voltage offset from -120%% and +120%%.\n");
            printf("\tphase <0-359>\t\tSets the phase angle offset.\n");
            printf("\ton/off\t\t\tTurn the channel on or off (*).\n");
            printf("\tactive\t\t\tSet the device to display channel.\n");
            printf("\tinverse\t\t\tSet the selected wave form to be inverted.\n");
            printf("\tstore <0-9>\t\tSave channel settings to memory slot 0-9.\n");
            printf("\tload <0-9>\t\tLoad channel settings from memory slot 0-9.\n");
            printf("\tprogram <0-15> <file>\tProgram arbitrary wave form.\n");
            printf("\tsine\t\t\tSine wave output (**).\n");
            printf("\tsquare\t\t\tSquare wave output (**).\n");
            printf("\ttriangle\t\tTriangle wave output (**).\n");
            printf("\tsaw\t\t\tSawtooth / upward slope wave output (**).\n");
            printf("\treversesaw\t\tReverse sawtooth / downward ramp waveform (**).\n");
            printf("\tarb <0-15>\t\tArbitrary waveform 0-15 (**).\n\n");
            
            printf(" (*) Will also change the displayed channel on the device.\n");
            printf(" (**) Using a wave form command turns off inverse.\n\n");
            
            printf("Arbitrary wave form programming:\n");
            printf("The file is 1024 lines, each line with a value. The value range depends \n");
            printf("on the signal generator and is 0-4095 for MHS-5225A (12bit samples).\n\n");
            
            printf("General instructions:\n");
            printf("Most commands are executed in the order given so commands like channel will\naffect certain subsequent commands.\n\nExample: \n%s /dev/ttyUSB0 channel 1 off square inverse freq 12345678.12 on\n\n", argv[0]);
            printf("The above example turns off channel 1, sets waveform to inverted sine wave of a\ngiven frequency then turns the channel back on.\n");
            printf("\nYou can combine commands in any order, switching channels at will. All commands\nwill be executed in the specified order only after validating that the command\nline does not contain errors.\n");
            exit(0);
        };
        
        commandParser["--help"] = commandParser["-?"]; 
        
        commandParser["debug"] = [&](int argc, const char *argv[])->void {
            argp++;
            signalGenerator.setDebugOutput(true);
        };
        
        commandParser["channel"] = [&](int argc, const char *argv[])->void {
            int cmdarg = argp++;
            if ( argp < argc ) {
                const char *arg = argv[argp++];
                int channel;
                if ( !parseInt(arg, channel) || channel < 1 || channel > 2 ) {
                    raise_expected_argument(argv[cmdarg], "<channel number>", "1/2", arg);
                }
                commandChain.push_back([&currentChannel,channel](){
                    currentChannel = channel;
                });
            } else {
                raise_expected_more_argments(argv[cmdarg]);
            }
        };
        
        commandParser["status"] = [&](int argc, const char *argv[])->void {
            argp++;
            
            commandChain.push_back([&](){
                for ( int channel = 1; channel <= 2; channel++ ) {
                    printf("%d: %c%-6s %6.3fV  %11.2fHz  %4.1f%% Duty  %3d\u00B0 Phase  %4d%% Offset\n", channel, signalGenerator.getInverted(channel) ? '~' : ' ', waveToString(signalGenerator.getWaveType(channel)), signalGenerator.getAmplitude(channel), signalGenerator.getFrequency(channel), signalGenerator.getDutyCycle(channel), signalGenerator.getPhaseOffset(channel), signalGenerator.getOffset(channel) );
                }
            });
        };
        
        commandParser["on"] = [&](int argc, const char *argv[])->void {
            argp++;
            commandChain.push_back([&]() {
                int activeChannel = signalGenerator.getCurrentChannel();
                if ( currentChannel != activeChannel )
                    signalGenerator.setCurrentChannel(currentChannel);
                signalGenerator.setCurrentChannelStatus(true);
                if ( currentChannel != activeChannel )    
                    signalGenerator.setCurrentChannel(activeChannel);
            });
        };
        
        commandParser["off"] = [&](int argc, const char *argv[])->void {
            argp++;
            commandChain.push_back([&]() {
                int activeChannel = signalGenerator.getCurrentChannel();
                if ( currentChannel != activeChannel )
                    signalGenerator.setCurrentChannel(currentChannel);
                signalGenerator.setCurrentChannelStatus(false);
                if ( currentChannel != activeChannel )    
                    signalGenerator.setCurrentChannel(activeChannel);
            });
        };
        
        commandParser["active"] = [&](int argc, const char *argv[])->void {
            argp++;
            commandChain.push_back([&]() {
                int activeChannel = signalGenerator.getCurrentChannel();
                if ( currentChannel != activeChannel )
                    signalGenerator.setCurrentChannel(currentChannel);
            });
        };        
        
        commandParser["inverse"] = [&](int argc, const char *argv[])->void {
            argp++;
            commandChain.push_back([&]() {
                if ( !signalGenerator.getInverted(currentChannel) ) 
                    signalGenerator.setInverted(currentChannel, true);
            });
        };        
        
        commandParser["sine"] = [&](int argc, const char *argv[])->void {
            argp++;
            commandChain.push_back([&]() {
                if ( signalGenerator.getInverted(currentChannel) ) 
                    signalGenerator.setInverted(currentChannel, false);
                
                signalGenerator.setWaveType(currentChannel, MHS5200Driver::WaveType::Sine);
            });
        };        
        
        commandParser["square"] = [&](int argc, const char *argv[])->void {
            argp++;
            commandChain.push_back([&]() {
                if ( signalGenerator.getInverted(currentChannel) ) 
                    signalGenerator.setInverted(currentChannel, false);
                
                signalGenerator.setWaveType(currentChannel, MHS5200Driver::WaveType::Square);
            });
        };        
        
        commandParser["triangle"] = [&](int argc, const char *argv[])->void {
            argp++;
            commandChain.push_back([&]() {
                if ( signalGenerator.getInverted(currentChannel) ) 
                    signalGenerator.setInverted(currentChannel, false);
                
                signalGenerator.setWaveType(currentChannel, MHS5200Driver::WaveType::Triangle);
            });
        };        
        
        commandParser["saw"] = [&](int argc, const char *argv[])->void {
            argp++;
            commandChain.push_back([&]() {
                if ( signalGenerator.getInverted(currentChannel) ) 
                    signalGenerator.setInverted(currentChannel, false);
                
                signalGenerator.setWaveType(currentChannel, MHS5200Driver::WaveType::Sawtooth);
            });
        };        
        
        commandParser["sawreverse"] = [&](int argc, const char *argv[])->void {
            argp++;
            commandChain.push_back([&]() {
                if ( signalGenerator.getInverted(currentChannel) ) 
                    signalGenerator.setInverted(currentChannel, false);
                
                signalGenerator.setWaveType(currentChannel, MHS5200Driver::WaveType::SawtoothReverse);
            });
        };        

        commandParser["arb"] = [&](int argc, const char *argv[])->void {
            int cmdarg = argp++;
            if ( argp < argc ) {
                const char *arg = argv[argp++];
                int arb;
                
                if ( !parseInt(arg, arb) || arb < 0 || arb > 15 ) {
                    raise_expected_argument(argv[cmdarg], "<slot #>", "0-15", arg);
                }
        
                MHS5200Driver::WaveType wave = (MHS5200Driver::WaveType)((int)MHS5200Driver::WaveType::Arbitrary0+arb);
                commandChain.push_back([&,wave]() {
                    if ( signalGenerator.getInverted(currentChannel) ) 
                        signalGenerator.setInverted(currentChannel, false);
                    signalGenerator.setWaveType(currentChannel, wave);
                });
            } else {
                raise_expected_more_argments(argv[cmdarg]);
            }            
        };        
        
        commandParser["offset"] = [&](int argc, const char *argv[])->void {
            int cmdarg = argp++;
            if ( argp < argc ) {
                const char *arg = argv[argp++];
                int val;
                
                if ( !parseInt(arg, val) || val < -120 || val > 120 ) {
                    raise_expected_argument(argv[cmdarg], "<relative percent>", "-120 to 120", arg);
                }
        
                commandChain.push_back([&,val]() {
                    signalGenerator.setOffset(currentChannel, val);
                });
            } else {
                raise_expected_more_argments(argv[cmdarg]);
            }            
        };        
        
        commandParser["phase"] = [&](int argc, const char *argv[])->void {
            int cmdarg = argp++;
            if ( argp < argc ) {
                const char *arg = argv[argp++];
                int val;

                if ( !parseInt(arg, val) || val < 0 || val > 359 ) {
                    raise_expected_argument(argv[cmdarg], "<phase angle>", "0-359", arg);
                }
        
                commandChain.push_back([&,val]() {
                    signalGenerator.setPhaseOffset(currentChannel, val);
                });
            } else {
                raise_expected_more_argments(argv[cmdarg]);
            }            
        };
        
        commandParser["amplitude"] = [&](int argc, const char *argv[])->void {
            int cmdarg = argp++;
            if ( argp < argc ) {
                const char *arg = argv[argp++];
                double val;

                if ( !parseDouble(arg, val) || val < 0.005 || val > 20.00 ) {
                    raise_expected_argument(argv[cmdarg], "<peak to peak volts>", "0.005 to 20.00", arg);
                }
                
                commandChain.push_back([&,val]() {
                    signalGenerator.setAmplitude(currentChannel, val);
                });
            } else {
                raise_expected_more_argments(argv[cmdarg]);
            }            
        };
        
        commandParser["duty"] = [&](int argc, const char *argv[])->void {
            int cmdarg = argp++;
            if ( argp < argc ) {
                const char *arg = argv[argp++];
                double val;

                if ( !parseDouble(arg, val) || val < 0.1 || val > 99.9 ) {
                    raise_expected_argument(argv[cmdarg], "<duty percentage>", "0.1 to 99.9", arg);
                }
                
                commandChain.push_back([&,val]() {
                    signalGenerator.setDutyCycle(currentChannel, val);
                });
            } else {
                raise_expected_more_argments(argv[cmdarg]);
            }            
        };

        commandParser["frequency"] = [&](int argc, const char *argv[])->void {
            int cmdarg = argp++;
            if ( argp < argc ) {
                const char *arg = argv[argp++];
                double val;

                if ( !parseDouble(arg, val) || val < 0.01 || val > 25000000.0 ) {
                    raise_expected_argument(argv[cmdarg], "<frequency in hz>", "0.01 to 25MHz", arg);
                }
                
                commandChain.push_back([&,val]() {
                    signalGenerator.setFrequency(currentChannel, val);
                });
            } else {
                raise_expected_more_argments(argv[cmdarg]);
            }            
        };
        
        commandParser["program"] = [&](int argc, const char *argv[])->void {
            int cmdarg = argp++;
            if ( (argp+1) < argc ) {
                const char *arg0 = argv[argp++];
                const char *arg1 = argv[argp++];                
                int arb;
                int values[1024];
                if ( !parseInt(arg0, arb) || arb < 0 || arb > 15 ) {
                    raise_expected_argument(argv[cmdarg], "<slot #>", "0-15", arg0);
                }

                try
                {
                    std::ifstream csvfile(arg1);
                    std::string line;
                    int p = 0;
                    while (std::getline(csvfile, line) && p < 1024)
                    {
                        std::istringstream iss(line);
                        int value;
                        if ( iss >> value ) {
                            values[p++] = value;
                        } else {
                            throw line;
                        }
                    }
                }
                catch(...) 
                {
                    raise_error_parsing_file(argv[cmdarg], arg1);
                }
                
                commandChain.push_back([&,arb,values]() {
                    signalGenerator.setArbitrary(arb, values);
                });
            } else {
                raise_expected_more_argments(argv[cmdarg]);
            }            
        }; 
        
        commandParser["freq"] = commandParser["frequency"];
        
        commandParser["store"] = [&](int argc, const char *argv[])->void {
            int cmdarg = argp++;
            if ( argp < argc ) {
                const char *arg = argv[argp++];
                int val;
                
                if ( !parseInt(arg, val) || val < 0 || val > 9 ) {
                    raise_expected_argument(argv[cmdarg], "<slot #>", "0 to 9", arg);
                }
        
                commandChain.push_back([&,val]() {
                    signalGenerator.saveSettings(val);
                });
            } else {
                raise_expected_more_argments(argv[cmdarg]);
            }
        };        
        
        commandParser["load"] = [&](int argc, const char *argv[])->void {
            int cmdarg = argp++;
            if ( argp < argc ) {
                const char *arg = argv[argp++];
                int val;
                
                if ( !parseInt(arg, val) || val < 0 || val > 9 ) {
                    raise_expected_argument(argv[cmdarg], "<slot #>", "0 to 9", arg);
                }
        
                commandChain.push_back([&,val]() {
                    signalGenerator.loadSettings(val);
                });
            } else {
                raise_expected_more_argments(argv[cmdarg]);
            }
        };        
        
        if ( argp < argc ) {
            if ( argv[argp][0] != '-' )
                deviceName = argv[argp++];
        } else {
            throw string("Error: Expected device name as first parameter.");
        }
        
        while ( argp < argc ) {
            const char *arg = argv[argp];
            auto i = commandParser.find(string(arg));
            if ( i == commandParser.end() ) {
                stringstream ss;
                ss << "Error: Invalid argument " << arg;
                throw ss.str();
            }
            i->second(argc, argv);
        }
        
        if ( signalGenerator.connect(deviceName) ) {
            currentChannel = signalGenerator.getCurrentChannel();            
            for ( auto &cmd : commandChain )
                cmd();
        }
    } catch ( string &s ) {
        cout << s.c_str() << endl;
        cout << "Terminated." << endl;
        return 1;
    }
    return 0;
}
