//
//  SynthNote.h
//  MySynth Plugin Header File - for individual notes
//
//  Used to declare objects and data structures used by the plugin.
//

#include "SynthPlugin.h"

#pragma once

//================================================================================
// MyNote - object representing a single note (within the synthesiser - see above)
//================================================================================

class MyNote : public APDI::Synth::Note
{
public:
    MyNote(MySynth* synthesiser) : Note(synthesiser), fFrequency(440.f), fLevel(1.f) { }
    
    MySynth* getSynthesiser() { return (MySynth*)synthesiser; }
    
    void onStartNote (int pitch, float velocity);
    bool onStopNote (float velocity);
    void onPitchWheel (int value);
    void onControlChange (int controller, int value);
    void operatorTick(int opNumber, int modNumber, int waveType);
    bool process (float** outputBuffer, int numChannels, int numSamples);
    void anyOpsActive();
    
private:
    //Oscillator Variables
    Sine sineGenerator[6];
    Saw sawGenerator[6];
    Square squareGenerator[6];
    int operatorWavetype[3];
    float fFrequency;
    float fCenterFreq;
    float fOpIn[6];
    float fOpOut[6];
    float fOpOutNoise;
    float fModSig[6];
    float fCarrSig[6];
    float fOscSig[6];
    float fCoarse[6];
    float fFine[6];
    float fOpGain[6];
    float fOpLfoAmount[6];
    float fLevel;
    float dissonanceFactor;
    bool deleteNote;
    //Envelope Variables
    Envelope OpAmpEnv[6];
    Envelope OpLfoEnv[6];
    float fOpAmpAttackTime[6];
    float fOpAmpDecayTime[6];
    float fOpAmpDecayLevel[6];
    float fOpAmpReleaseTime[6];
   


    //LFO Variables
    Sine vibratoLfo;
    float fVibLfoFrequency;
    float fVibLfoOutput; 
    float fVibLfoModSig;
    float fMovementFactor;


    //Filter Variables
    HPF dcOffsetFilter;
    LPF lowPass;
   
};
