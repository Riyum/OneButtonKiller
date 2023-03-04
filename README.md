# OneButtonKiller
###### Generate unusual sounds with just one click!

>*WORK IN PROGRESS*

OneButtonKiller is an audio instrument, currently with the following components:

| quantity      | component      |
| ------------- | -------------- |
| 4             | Oscillator     |
| 4             | LFO            |
| 4             | Filter         |
| 4             | Delay          |

My aim is to develop a program that can generate intricate sound patterns by simply clicking a single button.  
To attain this objective, I plan to include additional component types, increase their quantity, introduce signal routing
capabilities and other various techniques.

The user is presented with a simple GUI interface with the following controls:

- The random parameters generator button (The killer button)
- Undo / Redo buttons
- Output gain: master, channels
- GUI selector: select the audio component that the GUI component will manage
- Oscillator: wave type, gain, frequency, FM frequency, FM depth
- LFO: wave type, frequency, depth, routing options
- Filter: enable / disable, filter type, cutoff frequency, resonance, drive
- Delay: wet / dry mix, time, feedback

My current develop environment is Linux, the instrument should be cross-platform thanks to JUCE  
it is intended to be a standalone application, so there is no VST version.

This project is for learning purposes only.

## Usage
To emit a sound, press any key on keyboard.

## Build steps
1. [Get](https://juce.com/get-juce/) and install the JUCE library.
2. Clone the repo: `git clone https://github.com/Riyum/OneButtonKiller.git`
2. Open OneButtonKiller.jucer with [Projucer](https://docs.juce.com/master/tutorial_new_projucer_project.html#tutorial_new_projucer_project_open_existing_project)
3. Choose the desired [export target](https://docs.juce.com/master/tutorial_manage_projucer_project.html#tutorial_manage_projucer_project_managing_configurations)
4. Build the exported target in OneButtonKiller/Builds

## Credits
This project is built with [JUCE](https://github.com/juce-framework/JUCE)

## License
MIT license.
