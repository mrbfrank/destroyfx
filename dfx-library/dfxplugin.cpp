/*------------------------------------------------------------------------
Destroy FX is a sovereign entity comprised of Marc Poirier & Tom Murphy 7.  
This is our class for E-Z plugin-making and E-Z multiple-API support.
written by Marc Poirier, October 2002
------------------------------------------------------------------------*/

#ifndef __DFXPLUGIN_H
#include "dfxplugin.h"
#endif


#include <time.h>	// for time(), which is used to feed srand()

#if TARGET_API_VST && TARGET_PLUGIN_HAS_GUI
	#include "vstgui.h"
#endif



#pragma mark _________---DfxPlugin---_________

#pragma mark _________init_________

//-----------------------------------------------------------------------------
DfxPlugin::DfxPlugin(
					TARGET_API_BASE_INSTANCE_TYPE inInstance
					, long numParameters
					, long numPresets
					)

// setup the constructors of the inherited base classes, for the appropriate API
#if TARGET_API_AUDIOUNIT
	#if TARGET_PLUGIN_IS_INSTRUMENT
		: TARGET_API_BASE_CLASS(inInstance, UInt32 numInputs, UInt32 numOutputs, UInt32 numGroups = 0), 
	#else
		: TARGET_API_BASE_CLASS(inInstance), 
	#endif

#elif TARGET_API_VST
	: TARGET_API_BASE_CLASS(inInstance, numPresets, numParameters), 
	numInputs(NUM_INPUTS), numOutputs(NUM_OUTPUTS), 

#endif
// end API-specific base constructors

	numParameters(numParameters), numPresets(numPresets)
{
	parameters = NULL;
	presets = NULL;
	channelconfigs = NULL;
	tempoRateTable = NULL;

	#if TARGET_PLUGIN_USES_MIDI
		midistuff = NULL;
		dfxsettings = NULL;
	#endif

	audioBuffersAllocated = false;
	updatesamplerate();	// XXX have it set to something here?
	sampleratechanged = true;
	hostCanDoTempo = false;	// until proven otherwise

	latency_samples = 0;
	latency_seconds = 0.0;
	b_uselatency_seconds = false;

	tailsize_samples = 0;
	tailsize_seconds = 0.0;
	b_usetailsize_seconds = false;

	numchannelconfigs = 0;

	// set a seed value for rand() from the system clock
	srand((unsigned int)time(NULL));

	currentPresetNum = 0;	// XXX eh?


	parameters = new DfxParam[numParameters];
	presets = new DfxPreset[numPresets];
	for (int i=0; i < numPresets; i++)
		presets[i].PostConstructor(numParameters);	// allocate for parameter values
//	channelconfigs = ;

	#if TARGET_PLUGIN_USES_MIDI
		midistuff = new DfxMidi;
		dfxsettings = new DfxSettings(PLUGIN_ID, this);
	#endif


#if TARGET_API_AUDIOUNIT
//	CreateElements();	// XXX do this?  I think not, it happens in AUBase::DoInitialize()
	inputsP = outputsP = NULL;

	aupresets = NULL;
	aupresets = (AUPreset*) malloc(numPresets * sizeof(AUPreset));
	for (long i=0; i < numPresets; i++)
	{
		aupresets[i].presetNumber = i;
		aupresets[i].presetName = NULL;	// XXX eh?
	}
	#if TARGET_PLUGIN_USES_MIDI
		aumidicontrolmap = NULL;
		aumidicontrolmap = (AudioUnitMIDIControlMapping*) malloc(numParameters * sizeof(AudioUnitMIDIControlMapping));
	#endif

#endif
// Audio Unit stuff


#if TARGET_API_VST
	setUniqueID(PLUGIN_ID);	// identify
	setNumInputs(NUM_INPUTS);
	setNumOutputs(NUM_OUTPUTS);
	#if NUM_INPUTS == 2
		canMono();	// it's okay to feed a double-mono (fake stereo) input
	#endif

	#if TARGET_PLUGIN_IS_INSTRUMENT
		isSynth();
	#endif

	canProcessReplacing();	// supports both accumulating and replacing output
	TARGET_API_BASE_CLASS::setProgram(0);	// set the current preset number to 0

	isinitialized = false;
	latencychanged = false;

	// check to see if the host supports sending tempo & time information to VST plugins
	hostCanDoTempo = (canHostDo("sendVstTimeInfo") == 1);

	#if TARGET_PLUGIN_USES_DSPCORE
		dspcores = NULL;
		dspcores = (DfxPluginCore**) malloc(getnumoutputs() * sizeof(DfxPluginCore*));
		// need to save instantiating the cores for the inheriting plugin class constructor
		for (long i=0; i < getnumoutputs(); i++)
			dspcores[i] = NULL;
	#endif

	#if TARGET_PLUGIN_USES_MIDI
		// tell host that we want to use special data chunks for settings storage
		programsAreChunks();
	#endif

#endif
// VST stuff

}


//-----------------------------------------------------------------------------
DfxPlugin::~DfxPlugin()
{
	if (parameters != NULL)
		delete[] parameters;
	parameters = NULL;

	if (presets != NULL)
		delete[] presets;
	presets = NULL;

	if (channelconfigs != NULL)
		free(channelconfigs);
	channelconfigs = NULL;

	if (tempoRateTable != NULL)
		delete tempoRateTable;
	tempoRateTable = NULL;

	#if TARGET_PLUGIN_USES_MIDI
		if (midistuff != NULL)
			delete midistuff;
		midistuff = NULL;
		if (dfxsettings != NULL)
			delete dfxsettings;
		dfxsettings = NULL;
	#endif

	#if TARGET_API_AUDIOUNIT
		if (aupresets != NULL)
			free(aupresets);
		aupresets = NULL;
		#if TARGET_PLUGIN_USES_MIDI
			if (aumidicontrolmap != NULL)
				free(aumidicontrolmap);
			aumidicontrolmap = NULL;
		#endif
	#endif
	// end AudioUnit-specific destructor stuff

	#if TARGET_API_VST
		// the child plugin class destructor should call do_cleanup for VST 
		// because VST has no initialize/cleanup sort of methods, but if 
		// the child class doesn't have its own cleanup method, then it's 
		// not necessary, and so we can do it now if it wasn't done
		if (isinitialized)
			do_cleanup();
		#if TARGET_PLUGIN_USES_DSPCORE
			if (dspcores != NULL)
			{
				for (long i=0; i < getnumoutputs(); i++)
				{
					if (dspcores[i] != NULL)
						delete dspcores[i];
					dspcores[i] = NULL;
				}
				free(dspcores);
			}
			dspcores = NULL;
		#endif
	#endif
	// end VST-specific destructor stuff
}


//-----------------------------------------------------------------------------
// non-virtual function that calls initialize() and insures that some stuff happens
long DfxPlugin::do_initialize()
{
	updatesamplerate();
	updatenumchannels();
	createbuffers();

	long result = initialize();

	#if TARGET_API_VST
		if (result == 0)
			isinitialized = true;
	#endif

	if (result == 0)
		do_reset();

	return result;
}

//-----------------------------------------------------------------------------
// non-virtual function that calls cleanup() and insures that some stuff happens
void DfxPlugin::do_cleanup()
{
	releasebuffers();

	#if TARGET_API_AUDIOUNIT
		if (inputsP != NULL)
			free(inputsP);
		inputsP = NULL;
		if (outputsP != NULL)
			free(outputsP);
		outputsP = NULL;
	#endif

	cleanup();

	#if TARGET_API_VST
		isinitialized = false;
	#endif
}

//-----------------------------------------------------------------------------
// non-virtual function that calls reset() and insures that some stuff happens
void DfxPlugin::do_reset()
{
	clearbuffers();
	reset();
}



#pragma mark _________parameters_________

//-----------------------------------------------------------------------------
void DfxPlugin::initparameter_f(long parameterIndex, const char *initName, float initValue, 
						float initDefaultValue, float initMin, float initMax, 
						DfxParamCurve initCurve, DfxParamUnit initUnit)
{
	if (parameterisvalid(parameterIndex))
	{
		parameters[parameterIndex].init_f(initName, initValue, initDefaultValue, initMin, initMax, initCurve, initUnit);
		update_parameter(parameterIndex);	// make the host aware of the parameter change
		initpresetsparameter(parameterIndex);	// default empty presets with this value
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::initparameter_d(long parameterIndex, const char *initName, double initValue, 
						double initDefaultValue, double initMin, double initMax, 
						DfxParamCurve initCurve, DfxParamUnit initUnit)
{
	if (parameterisvalid(parameterIndex))
	{
		parameters[parameterIndex].init_d(initName, initValue, initDefaultValue, initMin, initMax, initCurve, initUnit);
		update_parameter(parameterIndex);	// make the host aware of the parameter change
		initpresetsparameter(parameterIndex);	// default empty presets with this value
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::initparameter_i(long parameterIndex, const char *initName, long initValue, 
						long initDefaultValue, long initMin, long initMax, 
						DfxParamCurve initCurve, DfxParamUnit initUnit)
{
	if (parameterisvalid(parameterIndex))
	{
		parameters[parameterIndex].init_i(initName, initValue, initDefaultValue, initMin, initMax, initCurve, initUnit);
		update_parameter(parameterIndex);	// make the host aware of the parameter change
		initpresetsparameter(parameterIndex);	// default empty presets with this value
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::initparameter_ui(long parameterIndex, const char *initName, unsigned long initValue, 
						unsigned long initDefaultValue, unsigned long initMin, unsigned long initMax, 
						DfxParamCurve initCurve, DfxParamUnit initUnit)
{
	if (parameterisvalid(parameterIndex))
	{
		parameters[parameterIndex].init_ui(initName, initValue, initDefaultValue, initMin, initMax, initCurve, initUnit);
		update_parameter(parameterIndex);	// make the host aware of the parameter change
		initpresetsparameter(parameterIndex);	// default empty presets with this value
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::initparameter_b(long parameterIndex, const char *initName, bool initValue, bool initDefaultValue, 
						DfxParamCurve initCurve, DfxParamUnit initUnit)
{
	if (parameterisvalid(parameterIndex))
	{
		parameters[parameterIndex].init_b(initName, initValue, initDefaultValue, initCurve, initUnit);
		update_parameter(parameterIndex);	// make the host aware of the parameter change
		initpresetsparameter(parameterIndex);	// default empty presets with this value
	}
}

//-----------------------------------------------------------------------------
// this is a shorcut for initializing a parameter that uses integer indexes 
// into an array, with an array of strings representing its values
void DfxPlugin::initparameter_indexed(long parameterIndex, const char *initName, long initValue, long initDefaultValue, long initNumItems)
{
	if (parameterisvalid(parameterIndex))
	{
		parameters[parameterIndex].init_i(initName, initValue, initDefaultValue, 0, initNumItems-1, kDfxParamCurve_stepped, kDfxParamUnit_strings);
		update_parameter(parameterIndex);	// make the host aware of the parameter change
		initpresetsparameter(parameterIndex);	// default empty presets with this value
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::setparameter(long parameterIndex, DfxParamValue newValue)
{
	if (parameterisvalid(parameterIndex))
	{
		parameters[parameterIndex].set(newValue);
		update_parameter(parameterIndex);	// make the host aware of the parameter change
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::setparameter_f(long parameterIndex, float newValue)
{
	if (parameterisvalid(parameterIndex))
	{
		parameters[parameterIndex].set_f(newValue);
		update_parameter(parameterIndex);	// make the host aware of the parameter change
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::setparameter_d(long parameterIndex, double newValue)
{
	if (parameterisvalid(parameterIndex))
	{
		parameters[parameterIndex].set_d(newValue);
		update_parameter(parameterIndex);	// make the host aware of the parameter change
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::setparameter_i(long parameterIndex, long newValue)
{
	if (parameterisvalid(parameterIndex))
	{
		parameters[parameterIndex].set_i(newValue);
		update_parameter(parameterIndex);	// make the host aware of the parameter change
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::setparameter_b(long parameterIndex, bool newValue)
{
	if (parameterisvalid(parameterIndex))
	{
		parameters[parameterIndex].set_b(newValue);
		update_parameter(parameterIndex);	// make the host aware of the parameter change
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::setparameter_gen(long parameterIndex, float newValue)
{
	if (parameterisvalid(parameterIndex))
	{
		parameters[parameterIndex].set_gen(newValue);
		update_parameter(parameterIndex);	// make the host aware of the parameter change
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::randomizeparameter(long parameterIndex)
{
	if (parameterisvalid(parameterIndex))
	{
		parameters[parameterIndex].randomize();
		update_parameter(parameterIndex);	// make the host aware of the parameter change
	}
}

//-----------------------------------------------------------------------------
// do stuff necessary to inform the host of changes, etc.
void DfxPlugin::update_parameter(long parameterIndex)
{
	#if TARGET_API_AUDIOUNIT
		// make the global-scope element aware of the parameter's value
		AUBase::SetParameter(parameterIndex, kAudioUnitScope_Global, 0, getparameter_f(parameterIndex), 0);

	#elif TARGET_API_VST
		long vstpresetnum = TARGET_API_BASE_CLASS::getProgram();
		if (presetisvalid(vstpresetnum))
			setpresetparameter(vstpresetnum, parameterIndex, getparameter(parameterIndex));
		#if TARGET_PLUGIN_HAS_GUI
		if (editor)
			((AEffGUIEditor*)editor)->setParameter(parameterIndex, getparameter_gen(parameterIndex));
		#endif

	#endif
}

//-----------------------------------------------------------------------------
void DfxPlugin::getparametername(long parameterIndex, char *text)
{
	if (text != NULL)
	{
		if (parameterisvalid(parameterIndex))
			parameters[parameterIndex].getname(text);
		else
			text[0] = 0;
	}
}

//-----------------------------------------------------------------------------
DfxParamValueType DfxPlugin::getparametervaluetype(long parameterIndex)
{
	if (parameterisvalid(parameterIndex))
		return parameters[parameterIndex].getvaluetype();
	else
		return kDfxParamValueType_undefined;
}

//-----------------------------------------------------------------------------
DfxParamUnit DfxPlugin::getparameterunit(long parameterIndex)
{
	if (parameterisvalid(parameterIndex))
		return parameters[parameterIndex].getunit();
	else
		return kDfxParamUnit_undefined;
}

//-----------------------------------------------------------------------------
bool DfxPlugin::setparametervaluestring(long parameterIndex, long stringIndex, const char *inText)
{
	if (parameterisvalid(parameterIndex))
		return parameters[parameterIndex].setvaluestring(stringIndex, inText);
	else
		return false;
}

//-----------------------------------------------------------------------------
bool DfxPlugin::getparametervaluestring(long parameterIndex, long stringIndex, char *outText)
{
	if (parameterisvalid(parameterIndex))
		return parameters[parameterIndex].getvaluestring(stringIndex, outText);
	else
		return false;
}

//-----------------------------------------------------------------------------
char * DfxPlugin::getparametervaluestring_ptr(long parameterIndex, long stringIndex)
{	if (parameterisvalid(parameterIndex))
		return parameters[parameterIndex].getvaluestring_ptr(stringIndex);
	else
		return 0;
}

//-----------------------------------------------------------------------------
bool DfxPlugin::getparameterchanged(long parameterIndex)
{
	if (parameterisvalid(parameterIndex))
		return parameters[parameterIndex].getchanged();
	else
		return false;
}

//-----------------------------------------------------------------------------
void DfxPlugin::setparameterchanged(long parameterIndex, bool newChanged)
{
	if (parameterisvalid(parameterIndex))
		parameters[parameterIndex].setchanged(newChanged);
}



#pragma mark _________presets_________

//-----------------------------------------------------------------------------
// whether or not the index is a valid preset
bool DfxPlugin::presetisvalid(long presetIndex)
{
	if (presets == NULL)
		return false;
	if ( (presetIndex < 0) || (presetIndex >= numPresets) )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// whether or not the index is a valid preset with a valid name
// this is mostly just for Audio Unit
bool DfxPlugin::presetnameisvalid(long presetIndex)
{
	// still do this check to avoid bad pointer access
	if (!presetisvalid(presetIndex))
		return false;

	if (presets[presetIndex].getname_ptr() == NULL)
		return false;
	if ( (presets[presetIndex].getname_ptr())[0] == 0 )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// load the settings of a preset
bool DfxPlugin::loadpreset(long presetIndex)
{
	if ( !presetisvalid(presetIndex) )
		return false;

	for (long i=0; i < numParameters; i++)
		setparameter(i, getpresetparameter(presetIndex, i));

	// do stuff necessary to inform the host of changes, etc.
	update_preset(presetIndex);
	return true;
}

//-----------------------------------------------------------------------------
// do stuff necessary to inform the host of changes, etc.
void DfxPlugin::update_preset(long presetIndex)
{
	currentPresetNum = presetIndex;

	#if TARGET_API_AUDIOUNIT
		AUPreset au_preset;
		au_preset.presetNumber = presetIndex;
		au_preset.presetName = getpresetcfname(presetIndex);
		SetAFactoryPresetAsCurrent(au_preset);

	#elif TARGET_API_VST
		TARGET_API_BASE_CLASS::setProgram(presetIndex);
		// tell the host to update the generic editor display with the new settings
		AudioEffectX::updateDisplay();

	#endif
}

//-----------------------------------------------------------------------------
// default all empty (no name) presets with the current value of a parameter
void DfxPlugin::initpresetsparameter(long parameterIndex)
{
	// first fill in the presets with the init settings 
	// so that there are no "stale" unset values
	for (long i=0; i < numPresets; i++)
	{
		if ( !presetnameisvalid(i) )	// only if it's an "empty" preset
			setpresetparameter(i, parameterIndex, getparameter(parameterIndex));
	}
}

//-----------------------------------------------------------------------------
DfxParamValue DfxPlugin::getpresetparameter(long presetIndex, long parameterIndex)
{
	if ( parameterisvalid(parameterIndex) && presetisvalid(presetIndex) )
		return presets[presetIndex].values[parameterIndex];
	else
	{
		DfxParamValue dummy;
		memset(&dummy, 0, sizeof(DfxParamValue));
		return dummy;
	}
}

//-----------------------------------------------------------------------------
float DfxPlugin::getpresetparameter_f(long presetIndex, long parameterIndex)
{
	if ( parameterisvalid(parameterIndex) && presetisvalid(presetIndex) )
		return parameters[parameterIndex].derive_f(presets[presetIndex].values[parameterIndex]);
	else
		return 0.0f;
}

//-----------------------------------------------------------------------------
void DfxPlugin::setpresetparameter(long presetIndex, long parameterIndex, DfxParamValue newValue)
{
	if ( parameterisvalid(parameterIndex) && presetisvalid(presetIndex) )
		presets[presetIndex].values[parameterIndex] = newValue;
}

//-----------------------------------------------------------------------------
void DfxPlugin::setpresetparameter_f(long presetIndex, long parameterIndex, float newValue)
{
	if ( parameterisvalid(parameterIndex) && presetisvalid(presetIndex) )
		parameters[parameterIndex].accept_f(newValue, presets[presetIndex].values[parameterIndex]);
}

//-----------------------------------------------------------------------------
void DfxPlugin::setpresetparameter_d(long presetIndex, long parameterIndex, double newValue)
{
	if ( parameterisvalid(parameterIndex) && presetisvalid(presetIndex) )
		parameters[parameterIndex].accept_d(newValue, presets[presetIndex].values[parameterIndex]);
}

//-----------------------------------------------------------------------------
void DfxPlugin::setpresetparameter_i(long presetIndex, long parameterIndex, long newValue)
{
	if ( parameterisvalid(parameterIndex) && presetisvalid(presetIndex) )
		parameters[parameterIndex].accept_i(newValue, presets[presetIndex].values[parameterIndex]);
}

//-----------------------------------------------------------------------------
void DfxPlugin::setpresetparameter_b(long presetIndex, long parameterIndex, bool newValue)
{
	if ( parameterisvalid(parameterIndex) && presetisvalid(presetIndex) )
		parameters[parameterIndex].accept_b(newValue, presets[presetIndex].values[parameterIndex]);
}

//-----------------------------------------------------------------------------
void DfxPlugin::setpresetparameter_gen(long presetIndex, long parameterIndex, float genValue)
{
	if ( parameterisvalid(parameterIndex) && presetisvalid(presetIndex) )
		parameters[parameterIndex].accept_d(parameters[parameterIndex].expand(genValue), presets[presetIndex].values[parameterIndex]);
}

//-----------------------------------------------------------------------------
// set the text of a preset name
void DfxPlugin::setpresetname(long presetIndex, const char *inText)
{
	if (presetisvalid(presetIndex))
		presets[presetIndex].setname(inText);
}

//-----------------------------------------------------------------------------
// get a copy of the text of a preset name
void DfxPlugin::getpresetname(long presetIndex, char *outText)
{
	if (presetisvalid(presetIndex))
		presets[presetIndex].getname(outText);
}

//-----------------------------------------------------------------------------
// get a pointer to the text of a preset name
char * DfxPlugin::getpresetname_ptr(long presetIndex)
{
	if (presetisvalid(presetIndex))
		return presets[presetIndex].getname_ptr();
	else
		return NULL;
}

#if TARGET_API_AUDIOUNIT
//-----------------------------------------------------------------------------
// get the CFString version of a preset name
CFStringRef DfxPlugin::getpresetcfname(long presetIndex)
{
	if (presetisvalid(presetIndex))
		return presets[presetIndex].getcfname();
	else
		return NULL;
}
#endif



#pragma mark _________state_________

//-----------------------------------------------------------------------------
// change the current audio sampling rate
void DfxPlugin::setsamplerate(double newrate)
{
	// avoid bogus values
	if (newrate <= 0.0)
		newrate = 44100.0;

	if (newrate != DfxPlugin::samplerate)
		sampleratechanged = true;

	// accept the new value into our sampling rate keeper
	DfxPlugin::samplerate = newrate;
}

//-----------------------------------------------------------------------------
// called when the sampling rate should be re-fetched from the host
void DfxPlugin::updatesamplerate()
{
#if TARGET_API_AUDIOUNIT
	if (IsInitialized())	// will crash if not initialized (elements not created)
		setsamplerate(GetSampleRate());
	else
		setsamplerate(44100.0);
#elif TARGET_API_VST
	setsamplerate((double)getSampleRate());
#endif
}

//-----------------------------------------------------------------------------
// called when the number of audio channels has changed
void DfxPlugin::updatenumchannels()
{
	#if TARGET_API_AUDIOUNIT
		// the number of inputs or outputs may have changed
		numInputs = getnuminputs();
		if (inputsP)
			free(inputsP);
		inputsP = (float**) malloc(numInputs * sizeof(float*));

		numOutputs = getnumoutputs();
		if (outputsP)
			free(outputsP);
		outputsP = (float**) malloc(numOutputs * sizeof(float*));
	#endif
}



#pragma mark _________properties_________

//-----------------------------------------------------------------------------
// return the number of audio inputs
unsigned long DfxPlugin::getnuminputs()
{
#if TARGET_API_AUDIOUNIT
	return GetInput(0)->GetStreamFormat().mChannelsPerFrame;
#endif
#if TARGET_API_VST
	return numInputs;
#endif
}

//-----------------------------------------------------------------------------
// return the number of audio outputs
unsigned long DfxPlugin::getnumoutputs()
{
#if TARGET_API_AUDIOUNIT
	return GetOutput(0)->GetStreamFormat().mChannelsPerFrame;
#endif
#if TARGET_API_VST
	return numOutputs;
#endif
}

//-----------------------------------------------------------------------------
// add an audio input/output configuration to the array of i/o configurations
void DfxPlugin::addchannelconfig(short numin, short numout)
{
	if (channelconfigs != NULL)
	{
		DfxChannelConfig *swapconfigs = (DfxChannelConfig*) malloc(sizeof(DfxChannelConfig) * numchannelconfigs);
		memcpy(swapconfigs, channelconfigs, sizeof(DfxChannelConfig) * numchannelconfigs);
		free(channelconfigs);
		channelconfigs = (DfxChannelConfig*) malloc(sizeof(DfxChannelConfig) * (numchannelconfigs+1));
		memcpy(channelconfigs, swapconfigs, sizeof(DfxChannelConfig) * numchannelconfigs);
		free(swapconfigs);
		channelconfigs[numchannelconfigs].inChannels = numin;
		channelconfigs[numchannelconfigs].outChannels = numout;
		numchannelconfigs++;
	}
	else
	{
		channelconfigs = (DfxChannelConfig*) malloc(sizeof(DfxChannelConfig));
		channelconfigs[0].inChannels = numin;
		channelconfigs[0].outChannels = numout;
		numchannelconfigs = 1;
	}
}



#pragma mark _________processing_________

//-----------------------------------------------------------------------------
// this is called once per audio processing block (before doing the processing) 
// in order to try to get musical tempo/time/location information from the host
void DfxPlugin::processtimeinfo()
{
	// default these values to something reasonable in case they are not available from the host
	timeinfo.tempo = 120.0;
	timeinfo.tempoIsValid = false;
	timeinfo.beatPos = 0.0;
	timeinfo.beatPosIsValid = false;
	timeinfo.barPos = 0.0;
	timeinfo.barPosIsValid = false;
	timeinfo.numerator = 4.0;
	timeinfo.denominator = 4.0;
	timeinfo.timeSigIsValid = false;
	timeinfo.samplesToNextBar = 0;
	timeinfo.samplesToNextBarIsValid = false;
	timeinfo.playbackChanged = false;


#if TARGET_API_AUDIOUNIT
	if (mHostCallbackInfo.beatAndTempoProc != NULL)
	{
		Float64 tempo, beat;
		if ( mHostCallbackInfo.beatAndTempoProc(mHostCallbackInfo.hostUserData, &beat, &tempo) == noErr )
		{
beat /= 192.0;	// XXX Logic workaround
			timeinfo.tempoIsValid = true;
			timeinfo.tempo = tempo;
			timeinfo.beatPosIsValid = true;
			timeinfo.beatPos = beat;

			hostCanDoTempo = true;
		}
	}

	if (mHostCallbackInfo.musicalTimeLocationProc != NULL)
	{
		// the number of samples until the next beat from the start sample of the current rendering buffer
		UInt32 sampleOffsetToNextBeat;
		// the number of beats of the denominator value that contained in the current measure
		Float32 timeSigNumerator;
		// music notational conventions (4 is a quarter note, 8 an eigth note, etc)
		UInt32 timeSigDenominator;
		// the beat that corresponds to the downbeat (first beat) of the current measure
		Float64 currentMeasureDownBeat;
		if ( mHostCallbackInfo.musicalTimeLocationProc(mHostCallbackInfo.hostUserData, 
				&sampleOffsetToNextBeat, &timeSigNumerator, &timeSigDenominator, &currentMeasureDownBeat) == noErr )
		{
			// get the song beat position of the beginning of the previous measure
			timeinfo.barPosIsValid = true;
			timeinfo.barPos = currentMeasureDownBeat;

			// get the numerator of the time signature - this is the number of beats per measure
			timeinfo.timeSigIsValid = true;
			timeinfo.numerator = (double) timeSigNumerator;
			timeinfo.denominator = (double) timeSigDenominator;
		}

		// determine whether the playback position or state has just changed
// XXX implement this
//		if ()
//			timeinfo.playbackChanged = true;
	}
#endif
// TARGET_API_AUDIOUNIT
 

#if TARGET_API_VST
	VstTimeInfo *vstTimeInfo = getTimeInfo(kVstTempoValid 
										| kVstTransportChanged 
										| kVstBarsValid 
										| kVstPpqPosValid 
										| kVstTimeSigValid);

 	// there's nothing we can do in that case
	if (vstTimeInfo != NULL)
	{
		if (kVstTempoValid & vstTimeInfo->flags)
		{
			timeinfo.tempoIsValid = true;
			timeinfo.tempo = vstTimeInfo->tempo;
		}

		// get the song beat position of our precise current location
		if (kVstPpqPosValid & vstTimeInfo->flags)
		{
			timeinfo.beatPosIsValid = true;
			timeinfo.beatPos = vstTimeInfo->ppqPos;
		}

		// get the song beat position of the beginning of the previous measure
		if (kVstBarsValid & vstTimeInfo->flags)
		{
			timeinfo.barPosIsValid = true;
			timeinfo.barPos = vstTimeInfo->barStartPos;
		}

		// get the numerator of the time signature - this is the number of beats per measure
		if (kVstTimeSigValid & vstTimeInfo->flags)
		{
			timeinfo.timeSigIsValid = true;
			timeinfo.numerator = (double) vstTimeInfo->timeSigNumerator;
			timeinfo.denominator = (double) vstTimeInfo->timeSigDenominator;
		}

		// determine whether the playback position or state has just changed
		if (kVstTransportChanged & vstTimeInfo->flags)
			timeinfo.playbackChanged = true;
	}
#endif
// TARGET_API_VST


	if (timeinfo.tempo <= 0.0)
		timeinfo.tempo = 120.0;
	timeinfo.tempo_bps = timeinfo.tempo / 60.0;
	timeinfo.samplesPerBeat = (long) (getsamplerate() / timeinfo.tempo_bps);

	if (timeinfo.tempoIsValid && timeinfo.beatPosIsValid && 
			timeinfo.barPosIsValid && timeinfo.timeSigIsValid)
		timeinfo.samplesToNextBarIsValid = true;

	// it will screw up the while loop below bigtime if the numerator isn't a positive number
	if (timeinfo.numerator <= 0.0)
		timeinfo.numerator = 4.0;
	if (timeinfo.denominator <= 0.0)
		timeinfo.denominator = 4.0;

	// calculate the number of samples frames from now until the next measure begins
	if (timeinfo.samplesToNextBarIsValid)
	{
		double numBeatsToBar;
		// calculate the distance in beats to the upcoming measure beginning point
		if (timeinfo.barPos == timeinfo.beatPos)
			numBeatsToBar = 0.0;
		else
		{
			numBeatsToBar = timeinfo.barPos + timeinfo.numerator - timeinfo.beatPos;
			// do this stuff because some hosts (Cubase) give kind of wacky barStartPos sometimes
			while (numBeatsToBar < 0.0)
				numBeatsToBar += timeinfo.numerator;
			while (numBeatsToBar > timeinfo.numerator)
				numBeatsToBar -= timeinfo.numerator;
		}
	
		// convert the value for the distance to the next measure from beats to samples
		timeinfo.samplesToNextBar = (long) (numBeatsToBar * getsamplerate() / timeinfo.tempo_bps);
		// protect against wacky values
		if (timeinfo.samplesToNextBar < 0)
			timeinfo.samplesToNextBar = 0;
	}
}


//-----------------------------------------------------------------------------
// this is called immediately before processing a block of audio
void DfxPlugin::preprocessaudio()
{
	#if TARGET_PLUGIN_USES_MIDI
		midistuff->preprocessEvents();
	#endif

	// fetch the latest musical tempo/time/location inforomation from the host
	processtimeinfo();
	// deal with current parameter values for usage during audio processing
	do_processparameters();
}

//-----------------------------------------------------------------------------
// this is called immediately after processing a block of audio
void DfxPlugin::postprocessaudio()
{
	// XXX turn off all parameterchanged flags?
	for (long i=0; i < numParameters; i++)
		setparameterchanged(i, false);

	#if TARGET_PLUGIN_USES_MIDI
		midistuff->postprocessEvents();
	#endif
}

//-----------------------------------------------------------------------------
// non-virtual function called to insure that processparameters happens
void DfxPlugin::do_processparameters()
{
	processparameters();
}



#pragma mark _________MIDI_________

#if TARGET_PLUGIN_USES_MIDI

//-----------------------------------------------------------------------------
void DfxPlugin::handlemidi_noteon(int channel, int note, int velocity, long frameOffset)
{
	if (midistuff != NULL)
		midistuff->handleNoteOn(channel, note, velocity, frameOffset);
	if (dfxsettings != NULL)
		dfxsettings->handleNoteOn(channel, note, velocity, frameOffset);
}

//-----------------------------------------------------------------------------
void DfxPlugin::handlemidi_noteoff(int channel, int note, int velocity, long frameOffset)
{
	if (midistuff != NULL)
		midistuff->handleNoteOff(channel, note, velocity, frameOffset);
	if (dfxsettings != NULL)
		dfxsettings->handleNoteOff(channel, note, velocity, frameOffset);
}

//-----------------------------------------------------------------------------
void DfxPlugin::handlemidi_allnotesoff(int channel, long frameOffset)
{
	if (midistuff != NULL)
		midistuff->handleAllNotesOff(channel, frameOffset);
}

//-----------------------------------------------------------------------------
void DfxPlugin::handlemidi_pitchbend(int channel, int valueLSB, int valueMSB, long frameOffset)
{
	if (midistuff != NULL)
		midistuff->handlePitchBend(channel, valueLSB, valueMSB, frameOffset);
	if (dfxsettings != NULL)
		dfxsettings->handlePitchBend(channel, valueLSB, valueMSB, frameOffset);
}

//-----------------------------------------------------------------------------
void DfxPlugin::handlemidi_cc(int channel, int controllerNum, int value, long frameOffset)
{
	if (midistuff != NULL)
		midistuff->handleCC(channel, controllerNum, value, frameOffset);
	if (dfxsettings != NULL)
		dfxsettings->handleCC(channel, controllerNum, value, frameOffset);
}

//-----------------------------------------------------------------------------
void DfxPlugin::handlemidi_programchange(int channel, int programNum, long frameOffset)
{
	if (midistuff != NULL)
		midistuff->handleProgramChange(channel, programNum, frameOffset);
}

#endif
// TARGET_PLUGIN_USES_MIDI






#pragma mark _________---helper-functions---_________

//-----------------------------------------------------------------------------
// handy helper function for creating an array of float values
// returns true if allocation was successful, false if allocation failed
bool createbuffer_f(float **buffer, long currentBufferSize, long desiredBufferSize)
{
	// if the size of the buffer has changed, 
	// then delete & reallocate the buffes according to the new size
	if (desiredBufferSize != currentBufferSize)
		releasebuffer_f(buffer);

	if (*buffer == NULL)
		*buffer = (float*) malloc(desiredBufferSize * sizeof(float));

	// check if allocation was successful
	if (*buffer == NULL)
		return false;

	// we were successful if we reached this point
	return true;
}

//-----------------------------------------------------------------------------
// handy helper function for creating an array of double float values
// returns true if allocation was successful, false if allocation failed
bool createbuffer_d(double **buffer, long currentBufferSize, long desiredBufferSize)
{
	// if the size of the buffer has changed, 
	// then delete & reallocate the buffes according to the new size
	if (desiredBufferSize != currentBufferSize)
		releasebuffer_d(buffer);

	if (*buffer == NULL)
		*buffer = (double*) malloc(desiredBufferSize * sizeof(double));

	// check if allocation was successful
	if (*buffer == NULL)
		return false;

	// we were successful if we reached this point
	return true;
}

//-----------------------------------------------------------------------------
// handy helper function for creating an array of long int values
// returns true if allocation was successful, false if allocation failed
bool createbuffer_i(long **buffer, long currentBufferSize, long desiredBufferSize)
{
	// if the size of the buffer has changed, 
	// then delete & reallocate the buffes according to the new size
	if (desiredBufferSize != currentBufferSize)
		releasebuffer_i(buffer);

	if (*buffer == NULL)
		*buffer = (long*) malloc(desiredBufferSize * sizeof(long));

	// check if allocation was successful
	if (*buffer == NULL)
		return false;

	// we were successful if we reached this point
	return true;
}

//-----------------------------------------------------------------------------
// handy helper function for creating an array of boolean values
// returns true if allocation was successful, false if allocation failed
bool createbuffer_b(bool **buffer, long currentBufferSize, long desiredBufferSize)
{
	// if the size of the buffer has changed, 
	// then delete & reallocate the buffes according to the new size
	if (desiredBufferSize != currentBufferSize)
		releasebuffer_b(buffer);

	if (*buffer == NULL)
		*buffer = (bool*) malloc(desiredBufferSize * sizeof(bool));

	// check if allocation was successful
	if (*buffer == NULL)
		return false;

	// we were successful if we reached this point
	return true;
}

//-----------------------------------------------------------------------------
// handy helper function for creating an array of arrays of float values
// returns true if allocation was successful, false if allocation failed
bool createbufferarray_f(float ***buffers, unsigned long currentNumBuffers, long currentBufferSize, 
						unsigned long desiredNumBuffers, long desiredBufferSize)
{
	// if the size of each buffer or the number of buffers have changed, 
	// then delete & reallocate the buffers according to the new sizes
	if ( (desiredBufferSize != currentBufferSize) || (desiredNumBuffers != currentNumBuffers) )
		releasebufferarray_f(buffers, currentNumBuffers);

	if (desiredNumBuffers <= 0)
		return false;	// XXX false?

	if (*buffers == NULL)
	{
		*buffers = (float**) malloc(desiredNumBuffers * sizeof(float*));
		// out of memory or something
		if (*buffers == NULL)
			return false;
		for (unsigned long i=0; i < desiredNumBuffers; i++)
			(*buffers)[i] = NULL;
	}
	for (unsigned long i=0; i < desiredNumBuffers; i++)
	{
		if ((*buffers)[i] == NULL)
			(*buffers)[i] = (float*) malloc(desiredBufferSize * sizeof(float));
	}

	// check if allocations were successful
	for (unsigned long i=0; i < desiredNumBuffers; i++)
	{
		if ((*buffers)[i] == NULL)
			return false;
	}

	// we were successful if we reached this point
	return true;
}

//-------------------------------------------------------------------------
// handy helper function for safely deallocating an array of float values
void releasebuffer_f(float **buffer)
{
	if (*buffer != NULL)
	{
		free(*buffer);
	}
	*buffer = NULL;
}

//-------------------------------------------------------------------------
// handy helper function for safely deallocating an array of double float values
void releasebuffer_d(double **buffer)
{
	if (*buffer != NULL)
	{
		free(*buffer);
	}
	*buffer = NULL;
}

//-------------------------------------------------------------------------
// handy helper function for safely deallocating an array of long int values
void releasebuffer_i(long **buffer)
{
	if (*buffer != NULL)
	{
		free(*buffer);
	}
	*buffer = NULL;
}

//-------------------------------------------------------------------------
// handy helper function for safely deallocating an array of boolean values
void releasebuffer_b(bool **buffer)
{
	if (*buffer != NULL)
	{
		free(*buffer);
	}
	*buffer = NULL;
}

//-------------------------------------------------------------------------
// handy helper function for safely deallocating an array of arrays of float values
void releasebufferarray_f(float ***buffers, unsigned long numbuffers)
{
	if (*buffers != NULL)
	{
		for (unsigned long i=0; i < numbuffers; i++)
		{
			if ((*buffers)[i] != NULL)
				free((*buffers)[i]);
			(*buffers)[i] = NULL;
		}
		free(*buffers);
	}
	*buffers = NULL;
}

//-----------------------------------------------------------------------------
// handy helper function for safely zeroing the contents of an array of float values
void clearbuffer_f(float *buffer, long buffersize, float value)
{
	if (buffer != NULL)
	{
		for (long i=0; i < buffersize; i++)
			buffer[i] = value;
	}
}

//-----------------------------------------------------------------------------
// handy helper function for safely zeroing the contents of an array of double float values
void clearbuffer_d(double *buffer, long buffersize, double value)
{
	if (buffer != NULL)
	{
		for (long i=0; i < buffersize; i++)
			buffer[i] = value;
	}
}

//-----------------------------------------------------------------------------
// handy helper function for safely zeroing the contents of an array of long int values
void clearbuffer_i(long *buffer, long buffersize, long value)
{
	if (buffer != NULL)
	{
		for (long i=0; i < buffersize; i++)
			buffer[i] = value;
	}
}

//-----------------------------------------------------------------------------
// handy helper function for safely zeroing the contents of an array of boolean values
void clearbuffer_b(bool *buffer, long buffersize, bool value)
{
	if (buffer != NULL)
	{
		for (long i=0; i < buffersize; i++)
			buffer[i] = value;
	}
}

//-----------------------------------------------------------------------------
// handy helper function for safely zeroing the contents of an array of arrays of float values
void clearbufferarray_f(float **buffers, unsigned long numbuffers, long buffersize, float value)
{
	if (buffers != NULL)
	{
		for (unsigned long i=0; i < numbuffers; i++)
		{
			if (buffers[i] != NULL)
			{
				for (long j=0; j < buffersize; j++)
					buffers[i][j] = value;
			}
		}
	}
}