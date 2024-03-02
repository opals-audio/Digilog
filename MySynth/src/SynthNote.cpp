//
//  SynthNote.cpp
//  MySynth Plugin Source Code - for individual notes
//
//  Used to define the bodies of functions used by the plugin, as declared in SynthPlugin.h.
//

#include "SynthNote.h"

//================================================================================
// MyNote - object representing a single note (within the synthesiser - see above)
//================================================================================

//TODO:
/* 
    *Clean up code and comment!*
    Work out clicking bug with tremolo 
    Extra mapping for performance?
    Function and for loop to set parameters (rather than random blocks of code all over the place)
    Parameters 9 = 'LoFi (increasese trem, vib, time & dark in one sweep)
    Parameters 0-4 = Either dropdown UI to individually change each operator (probably not though)
                    OR multi-mapped usefull settings (movement, attack of all ops, )
    Extra parameters? = Trem speed, vib speed, release volume, rhodes/tine ep mode, 'extra lofi (some basic effects added)'
    Extensions - operator class, parameter Struct - note stealing to prevent muddiness during busy passages (unlikely)
    */

// Triggered when a note is started (use to initialise / prepare note processing)
void MyNote::onStartNote (int pitch, float velocity)
{
    // convert note number to fundamental frequency (Hz)
    fFrequency = 440.f * pow (2.f, (pitch - 69.f) / 12.f);
    fLevel = velocity;                      // store velocity
    
    //Set Envelope Parameters
    //Attack Times
    for (int i = 0; i < 6; ++i) {
        fOpAmpAttackTime[i] = parameters[0];
    }
    //Decay Times & Level
    for (int i = 0; i < 6; ++i) {
        fOpAmpDecayTime[i] = 0.15 + parameters[0] + parameters[7];
        fOpAmpDecayLevel[i] = 0;
    }
    //Release Times
    for (int i = 0; i < 6; ++i) {
        fOpAmpReleaseTime[i] = fOpAmpDecayTime[i] + parameters[0] + parameters[7];
    }
    if (pitch < 65 && parameters[7] > 0.2 && parameters[7] < 0.7 && parameters[0] > 0.2)
    {
        for (int i = 0; i < 6; ++i) {
            fOpAmpDecayTime[i] += 1;
        }
    }
    if (parameters[8] == 1)
    {
        for (int i = 0; i < 6; ++i) {
            fOpAmpDecayTime[i] += 0.55;
        }
    } 
    //Reset Oscillators and Initialise Enevelopes & Filters
    for (int i = 0; i < 6; ++i) {
        sineGenerator[i].reset();
        //fOpAmpDecayTime[i] += (parameters[6] * 0.25);
        OpAmpEnv[i].set(Envelope::Points(0, 0)(fOpAmpAttackTime[i], 1)(fOpAmpAttackTime[i] + fOpAmpDecayTime[i], fOpAmpDecayLevel[i]));
        OpAmpEnv[i].setLoop(2, 2);
    }
    deleteNote = false;
    dcOffsetFilter.setCutoff(20);
    lowPass.setCutoff(18000 - (parameters[8] * 300) - (parameters[0] * 100));
    vibratoLfo.reset();

    


}
    
// Triggered when a note is stopped (return false to keep the note alive)
bool MyNote::onStopNote(float velocity)
{
    for (int i = 0; i < 6; ++i) {
        
        OpAmpEnv[i].release(fOpAmpReleaseTime[i]);
    }
    return false; // return false to keep the note alive
}


void MyNote::onPitchWheel (int value){

}
 
void MyNote::onControlChange (int controller, int value){

}

void MyNote::anyOpsActive()
{
    for (int i = 0; i < 6; ++i)
    {
        if (OpAmpEnv[i].getStage() != Envelope::STAGE::ENV_OFF) {
            deleteNote = false;
            break;
        }
        else deleteNote = true;
    }
 

}
    
void MyNote::operatorTick(int opNumber, int modNumber, int waveType){
//Set Modulation Signal
    if (modNumber == -1)  fOpIn[opNumber] = 0;
    else fOpIn[opNumber] = fOpOut[modNumber];
fCenterFreq = fFrequency;
fModSig[opNumber] = fOpIn[opNumber] * fCenterFreq;
//Set Carrier Signal
fCarrSig[opNumber] = fCenterFreq;
fCarrSig[opNumber] *= fCoarse[opNumber];
fCarrSig[opNumber] *= fFine[opNumber];
//Set Oscillator Signal
fOscSig[opNumber] = fModSig[opNumber] + fCarrSig[opNumber];
//Apply LFO
fOscSig[opNumber] += fVibLfoModSig * (fOpLfoAmount[opNumber] * 10);
//Set oscilator & modulate (wavetype selected & FM applied)
switch (waveType) {
case 0:
    sineGenerator[opNumber].setFrequency(fOscSig[opNumber]);
    fOpOut[opNumber] = sineGenerator[opNumber].tick();
    break;
case 1:
    sawGenerator[opNumber].setFrequency(fOscSig[opNumber]);
    fOpOut[opNumber] = sawGenerator[opNumber].tick();
    break;
case 2:
    squareGenerator[opNumber].setFrequency(fOscSig[opNumber]);
    fOpOut[opNumber] = squareGenerator[opNumber].tick();
    break;
}
fOpOut[opNumber] *= fOpGain[opNumber];
fOpOut[opNumber] *= OpAmpEnv[opNumber].tick();
}

// Called to render the note's next buffer of audio (generates the sound)
// (return false to terminate the note)
bool MyNote::process (float** outputBuffer, int numChannels, int numSamples)
{
    float fMix = 0;
    float* pfOutBuffer0 = outputBuffer[0], *pfOutBuffer1 = outputBuffer[1];
    //LFO
    fVibLfoFrequency = 2;
    vibratoLfo.setFrequency(fVibLfoFrequency);

    fOpLfoAmount[0] = parameters[6] * 0.005;
    fOpLfoAmount[1] = 0;
    fOpLfoAmount[2] = 0 + (parameters[6] * 0.005);
    fOpLfoAmount[3] = 0;
    fOpLfoAmount[4] = 0 + (parameters[6] * 0.005);
    fOpLfoAmount[5] = 0;
    



    while(numSamples--)
    {
       // LFO modulate fFrequency
        fVibLfoOutput = vibratoLfo.tick();
        fMovementFactor = fVibLfoOutput * (pow((parameters[2] * 0.1),3) * 5);
        fVibLfoOutput *= 0.2;
        fVibLfoModSig = fVibLfoOutput * fFrequency;
        


        //Set Op Parameters
        dissonanceFactor = parameters[3] * 0.05;

        fCoarse[5] = 1;
        fFine[5] = 1;
        fOpGain[5] = 2.5 - parameters[8] * 2.49; 
        fCoarse[4] = 2;
        fFine[4] = 1;
        fOpGain[4] = 0.80;
        fCoarse[3] = 2;
        fFine[3] = 1;
        fOpGain[3] = 1.5 - parameters[8] * 1.49;
        fCoarse[2] = 1;
        fFine[2] = 1;
        fOpGain[2] = 0.80;
        fCoarse[1] = 14;
        fFine[1] = 1;
        fOpGain[1] = 5 - parameters[8] * 5;
        fCoarse[0] = 1;
        fFine[0] = 1;
        fOpGain[0] = 0.99;


        if (parameters[4] <= 0.33) {
            fFine[5] += dissonanceFactor * 2;
            fFine[3] += dissonanceFactor * 1.5;
            fFine[1] += dissonanceFactor;
            fOpGain[5] += fMovementFactor;
            fOpGain[3] += fMovementFactor;
            fOpGain[1] += fMovementFactor;
            fOpAmpAttackTime[5] += parameters[6];
            fOpAmpAttackTime[3] += parameters[6];
            fOpAmpAttackTime[1] += parameters[6];
            //Op5
            operatorTick(5, -1, 0);
            //Op4
            operatorTick(4, 5, 0);
            //Op3
            operatorTick(3, -1, 0);
            //Op2
            operatorTick(2, 3, 0);
            //Op1
            operatorTick(1, -1, 0);
            //Op0
            operatorTick(0, 1, 0);
            fMix = fOpOut[0] + fOpOut[2] + fOpOut[4];
        }
        if (parameters[4] > 0.33 && parameters[4] <= 0.66) {
            fOpGain[5] += fMovementFactor;
            fOpGain[4] -= fMovementFactor;
            fOpGain[3] += fMovementFactor;
            fOpGain[2] -= fMovementFactor;
            fOpGain[1] += fMovementFactor;
            fOpGain[0] -= fMovementFactor;
            fMovementFactor *= 0.01;
            fFine[5] += dissonanceFactor * 0.01;
            fFine[4] -= dissonanceFactor * 0.1;
            fFine[3] += dissonanceFactor * 0.2;
            fFine[2] -= dissonanceFactor * 0.02;
            fFine[1] += dissonanceFactor * 0.01;
            fFine[5] += fMovementFactor;
            fFine[4] -= fMovementFactor;
            fFine[3] += fMovementFactor;
            fFine[2] -= fMovementFactor;
            fFine[1] += fMovementFactor;
            fFine[0] -= fMovementFactor;
            fCoarse[1] -= 6;
            //Op5
            operatorTick(5, -1, 0);
            //Op4
            operatorTick(4, -1, 0);
            //Op3
            operatorTick(3, -1, 0);
            //Op2
            operatorTick(2, -1, 0);
            //Op1
            operatorTick(1, -1, 0);
            //Op0
            operatorTick(0, -1, 0);
            fMix = fOpOut[0] + fOpOut[1] + fOpOut[2] + fOpOut[3] + fOpOut[4] + fOpOut[5];
            fMix *= 0.4;
        }
        if (parameters[4] > 0.66){
            fFine[3] += dissonanceFactor * 2;
            fOpGain[3] += fMovementFactor;
            fCoarse[1] = 2;
            fCoarse[2] = 2.5;
            //Op5
            operatorTick(5, -1, 0);
            //Op4
            operatorTick(4, 1, 0);
            // ----------------------
            //Op3
            operatorTick(3, -1, 0);
            //Op2
            operatorTick(2, 3, 0);
            //Op1
            operatorTick(1, 3, 0);
            //Op0
            operatorTick(0, 3, 0);
            fMix = fOpOut[0] + fOpOut[1] + fOpOut[2];
            fMix *= 0.3;
        }

        fMix *= fLevel;                 // apply gain (velocity)
        fMix *= 0.3;
        fMix *= parameters[9];
        fMix = dcOffsetFilter.tick(fMix);
        fMix = lowPass.tick(fMix);
        
        
        *pfOutBuffer0++ = fMix;
        *pfOutBuffer1++ = fMix;

   
    }
    
   anyOpsActive();
   return deleteNote == false;

    //need boolean value here... for loop wont work - only one return allowed
}

//presets?

