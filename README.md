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
	freq, frequency <hz>	Set frequency in hz.
	duty <percent>		Set duty cycle percent.
	amplitude <volts>	Set peek to peek amplitude of the wave.
	program <0-15> <file>	Program arbitrary wave form using file.
	offset <+/-120>		Sets the voltage offset from -120% and +120%.
	phase <0-359>		Sets the phase angle offset.
	on/off			Turn the channel on or off (*).
	active			Set the device to display channel.
	inverse			Set the selected wave form to be inverted.
	store <0-9>		Save channel settings to memory slot 0-9.
	load <0-9>		Load channel settings from memory slot 0-9.
	program <0-15> <file>	Program arbitrary wave form.
	sine			Sine wave output (**).
	square			Square wave output (**).
	triangle		Triangle wave output (**).
	saw			Sawtooth / upward slope wave output (**).
	reversesaw		Reverse sawtooth / downward ramp waveform (**).
	arb <0-15>		Arbitrary waveform 0-15 (**).

 (*) Will also change the displayed channel on the device.
 (**) Using a wave form command turns off inverse.
```

Arbitrary wave form programming:
The file is 1024 lines, each line with a value. The value range depends on the signal generator and is 0-4095 for MHS-5225A (12bit samples).

General instructions:
Most commands are executed in the order given so commands like channel will affect certain subsequent commands.

Example: 
`mhs5200 /dev/ttyUSB0 channel 1 off square inverse freq 12345678.12 on`

The above example turns off channel 1, sets waveform to inverted sine wave of a given frequency then turns the channel back on.

You can combine commands in any order, switching channels at will. All commands will be executed in the specified order only after validating that the command line does not contain errors.

