# MHS-5200-Driver

A small project to create a Linux commandline interface for the MHS-5200 Signal Generator and a reusable "driver" class that could be incorporated into other projects.

## Usage

```
Usage: mhs5200 <tty device> <command list>

Command List:
	-?, --help		Shows this information and terminate.
	channel 1/2		Set channel to use for subsequent commands.
	debug			Output debug trace information.
	status			Shows this command information.
	freq,frequency <hz>	Set frequency in hz.
	duty <percent>		Set duty cycle percent.
	amplitude <volts>	Set peek to peek amplitude of the wave.
	offset <+/-120>		Sets the voltage offset from between -120% and +120%.
	phase <0-359>		Sets the phase angle offset.
	on/off			Turn the channel on or off (will change the displayed channel).
	active			Set the device to display channel.
	inverse			Set the previously selected wave form to be inverted.
	store <0-9>		Save current settings for both channels to memory slot N.
	load <0-9>		Restore settings for both channels from memory slot N.

	Wave form selection command:
		sine		Sine wave output.
		square		Square wave output.
		triangle	Triangle wave output.
		saw		Sawtooth / upward slope wave output.
		reversesaw	Reverse sawtooth / downward ramp waveform.
		arb <0-15>	Arbitrary waveform (0-15).

	Using a wave form command turns off inverse.
```

Most commands are executed in the order given so commands like channel will affect certain subsequent commands. 

Example: `mhs5200 /dev/ttyUSB0 channel 1 off square inverse freq 12345678.12 on`

The above example turns off channel 1, sets waveform to inverted sine wave of a given frequency then turns the channel back on.

You can combine commands in any order, switching channels at will. All commands will be executed in the specified order only after validating that the command line does not contain errors.

Example: `mhs5200 /dev/ttyUSB0 channel 1 active sine amplitude 11.0 freq 60 on channel 2 square amplitude 10 freq 120 duty 50 phase 180 offset 0 on`

After this command the signal generator should have channel 1 as the active as the channel shown on the device. Both channels were programmed. Channel 1 for a 60 hz sine wave, and channel 2 for a 50% duty cycle 120hz square wave offset 180 degrees.

