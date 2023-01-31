# OneButtonKiller
###### Generate unusual sounds with just one click!

>*WORK IN PROGRESS*

OneButtonKiller is an audio instrument, currently with the following components:

| quantity   | component   |
-------------|--------------
| 2          | Oscillator  |
| 2          | LFO         |
| 2          | Delay       |

The goal is to create a program that successfully generates a (random!) complex sound patterns with just one button.  
to achieve this, I intend to add more component types, increase their quantity, add signal routing capabilities, and much more.

The user is presented with a simple GUI interface with the following controls:

- The random parameters generator button (The killer button)
- Undo \ Redo buttons
- Gain: master, channels
- Oscillator: wave type, gain, frequency, FM frequency, FM depth
- LFO: wave type, frequency, depth
- Delay: wet \ dry mix

It is intended to be a standalone application, so there is no VST version.

This project is for learning purposes only.

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
