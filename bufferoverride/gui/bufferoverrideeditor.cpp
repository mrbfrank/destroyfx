/*------------------------------------------------------------------------
Copyright (C) 2001-2020  Sophia Poirier

This file is part of Buffer Override.

Buffer Override is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
(at your option) any later version.

Buffer Override is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Buffer Override.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/

#include "bufferoverrideeditor.h"

#include <array>
#include <cassert>

#include "bufferoverride.h"



constexpr char const* const kValueDisplayFont = "Helvetica";
constexpr float kValueDisplayRegularFontSize = 10.8f;
constexpr float kValueDisplayTinyFontSize = 10.2f;

constexpr char const* const kHelpDisplayFont = "Helvetica";
constexpr float kHelpDisplayFontSize = 9.6f;

constexpr DGColor kHelpDisplayTextColor(201, 201, 201);

constexpr float kUnusedControlAlpha = 0.39f;


//-----------------------------------------------------------------------------
enum
{
	// sliders
	kSliderWidth = 122,
	kSliderHeight = 14,
	kLFOSliderHeight = 18,
	//
	kDivisorLFORateSliderX = 7,
	kDivisorLFORateSliderY = 175,
	kDivisorLFODepthSliderX = kDivisorLFORateSliderX,
	kDivisorLFODepthSliderY = 199,
	kBufferLFORateSliderX = 351,
	kBufferLFORateSliderY = 175,
	kBufferLFODepthSliderX = kBufferLFORateSliderX,
	kBufferLFODepthSliderY = 199,
	//
	kPitchBendRangeSliderX = 6,
	kPitchBendRangeSliderY = 28,
	kSmoothSliderX = 354,
	kSmoothSliderY = 28,
	kDryWetMixSliderX = kSmoothSliderX,
	kDryWetMixSliderY = 65,
	//
	kTempoSliderWidth = 172,
	kTempoSliderX = 121,
	kTempoSliderY = 3,

	// displays
	kDisplayWidth = 36,
	kDisplayHeight = 12,
	//
	kDivisorDisplayX = 127 - kDisplayWidth,
	kDivisorDisplayY = 78,
	kBufferDisplayX = 241 - (kDisplayWidth / 2),
	kBufferDisplayY = 195,
	//
	kLFORateDisplayWidth = 21,
	kDivisorLFORateDisplayX = 174 - kLFORateDisplayWidth,//155,
	kDivisorLFORateDisplayY = 188,
	kDivisorLFODepthDisplayX = 171 - kDisplayWidth + 2,
	kDivisorLFODepthDisplayY = 212,
	kBufferLFORateDisplayX = 328 - kLFORateDisplayWidth,
	kBufferLFORateDisplayY = 188,
	kBufferLFODepthDisplayX = 331 - kDisplayWidth,
	kBufferLFODepthDisplayY = 212,
	//
	kPitchBendRangeDisplayX = 101 - kDisplayWidth,
	kPitchBendRangeDisplayY = 42,
	kSmoothDisplayX = 413 - kDisplayWidth,
	kSmoothDisplayY = 42,
	kDryWetMixDisplayX = 413 - kDisplayWidth,
	kDryWetMixDisplayY = 79,
	kTempoDisplayX = 312 - 1,
	kTempoDisplayY = 4,
	//
	kHelpDisplayX = 0,
	kHelpDisplayY = 231,

	// XY box
	kDivisorBufferBoxX = 156,
	kDivisorBufferBoxY = 40,
	kDivisorBufferBoxWidth = 169,
	kDivisorBufferBoxHeight = 138,

	// buttons
	kBufferInterruptButtonX = 227,
	kBufferInterruptButtonY = 20,
	kBufferInterruptButtonCornerX = 325,
	kBufferInterruptButtonCornerY = 20,

	kBufferTempoSyncButtonX = 156,
	kBufferTempoSyncButtonY = 20,
	kBufferTempoSyncButtonCornerX = 133,
	kBufferTempoSyncButtonCornerY = 20,
	kBufferSizeLabelX = 222,
	kBufferSizeLabelY = 212,

	kMidiModeButtonX = 5,
	kMidiModeButtonY = 6,

	kDivisorLFOShapeSwitchX = 8,
	kDivisorLFOShapeSwitchY = 145,
	kDivisorLFOTempoSyncButtonX = 52,
	kDivisorLFOTempoSyncButtonY = 119,
	kDivisorLFORateLabelX = 178,
	kDivisorLFORateLabelY = 188,

	kBufferLFOShapeSwitchX = 352,
	kBufferLFOShapeSwitchY = 145,
	kBufferLFOTempoSyncButtonX = 351,
	kBufferLFOTempoSyncButtonY = 120,
	kBufferLFORateLabelX = 277,
	kBufferLFORateLabelY = 188,

	kHostTempoButtonX = 386,
	kHostTempoButtonY = 6,

	kMidiLearnButtonX = 5,
	kMidiLearnButtonY = 63,
	kMidiResetButtonX = 4,
	kMidiResetButtonY = 86
};



//-----------------------------------------------------------------------------
// value text display procedures

bool divisorDisplayProc(float inValue, char* outText, void*)
{
	int const precision = (inValue <= 99.99f) ? 2 : 1;
	float const effectiveValue = (inValue < 2.0f) ? 1.0f : inValue;
	return snprintf(outText, DGTextDisplay::kTextMaxLength, "%.*f", precision, effectiveValue) > 0;
}

bool bufferSizeDisplayProc(float inValue, char* outText, void* inEditor)
{
	auto const dgEditor = static_cast<DfxGuiEditor*>(inEditor);
	if (dgEditor->getparameter_b(kBufferTempoSync))
	{
		if (auto const valueString = dgEditor->getparametervaluestring(kBufferSize_Sync))
		{
			return dfx::StrLCpy(outText, *valueString, DGTextDisplay::kTextMaxLength) > 0;
		}
		return false;
	}
	else
	{
		return snprintf(outText, DGTextDisplay::kTextMaxLength, "%.1f", inValue) > 0;
	}
}

bool lfoRateGenDisplayProc(float inValue, char* outText, void* inEditor, long rateSyncParameterID, long tempoSyncParameterID)
{
	auto const dgEditor = static_cast<DfxGuiEditor*>(inEditor);
	if (dgEditor->getparameter_b(tempoSyncParameterID))
	{
		if (auto const valueString = dgEditor->getparametervaluestring(rateSyncParameterID))
		{
			return dfx::StrLCpy(outText, *valueString, DGTextDisplay::kTextMaxLength) > 0;
		}
		return false;
	}

	int const precision = (inValue <= 9.99f) ? 2 : 1;
	return snprintf(outText, DGTextDisplay::kTextMaxLength, "%.*f", precision, inValue) > 0;
}

bool divisorLFORateDisplayProc(float inValue, char* outText, void* inEditor)
{
	return lfoRateGenDisplayProc(inValue, outText, inEditor, kDivisorLFORate_Sync, kDivisorLFOTempoSync);
}

bool bufferLFORateDisplayProc(float inValue, char* outText, void* inEditor)
{
	return lfoRateGenDisplayProc(inValue, outText, inEditor, kBufferLFORate_Sync, kBufferLFOTempoSync);
}

bool lfoDepthDisplayProc(float inValue, char* outText, void*)
{
	return snprintf(outText, DGTextDisplay::kTextMaxLength, "%.0f%%", inValue) > 0;
}

bool dryWetMixDisplayProc(float inValue, char* outText, void*)
{
	return snprintf(outText, DGTextDisplay::kTextMaxLength, "%.0f%%", inValue) > 0;
}

bool pitchBendRangeDisplayProc(float inValue, char* outText, void*)
{
	int const precision = (inValue <= 9.99f) ? 2 : 1;
	return snprintf(outText, DGTextDisplay::kTextMaxLength, "%s %.*f", dfx::kPlusMinusUTF8, precision, inValue) > 0;
}

bool tempoDisplayProc(float inValue, char* outText, void*)
{
	return snprintf(outText, DGTextDisplay::kTextMaxLength, "%.2f", inValue) > 0;
}



//-----------------------------------------------------------------------------
DFX_EDITOR_ENTRY(BufferOverrideEditor)

//-----------------------------------------------------------------------------
BufferOverrideEditor::BufferOverrideEditor(DGEditorListenerInstance inInstance)
:	DfxGuiEditor(inInstance)
{
}

//-----------------------------------------------------------------------------
long BufferOverrideEditor::OpenEditor()
{
	// create images

	// slider handles
	auto const sliderHandleImage = LoadImage("slider-handle.png");
	auto const sliderHandleImage_glowing = LoadImage("slider-handle-glowing.png");
	auto const sliderHandleImage_pitchbend = LoadImage("slider-handle-pitchbend.png");
	auto const sliderHandleImage_pitchbend_glowing = LoadImage("slider-handle-pitchbend-glowing.png");
	auto const xyBoxHandleImage = LoadImage("xy-box-handle.png");
	auto const xyBoxHandleImage_divisor_glowing = LoadImage("xy-box-handle-divisor-glow.png");
	auto const xyBoxHandleImage_buffer_glowing = LoadImage("xy-box-handle-buffer-glow.png");

	// buttons
	auto const bufferTempoSyncButtonImage = LoadImage("buffer-tempo-sync-button.png");
	auto const bufferTempoSyncButtonCornerImage = LoadImage("buffer-tempo-sync-button-corner.png");
	auto const bufferSizeLabelImage = LoadImage("buffer-size-label.png");

	auto const bufferInterruptButtonImage = LoadImage("buffer-interrupt-button.png");
	auto const bufferInterruptButtonCornerImage = LoadImage("buffer-interrupt-button-corner.png");

	auto const divisorLFOTempoSyncButtonImage = LoadImage("divisor-lfo-tempo-sync-button.png");
	auto const divisorLFORateLabelImage = LoadImage("divisor-lfo-rate-label.png");

	auto const bufferLFOTempoSyncButtonImage = LoadImage("buffer-lfo-tempo-sync-button.png");
	auto const bufferLFORateLabelImage = LoadImage("buffer-lfo-rate-label.png");

	auto const divisorLFOShapeSwitchImage = LoadImage("divisor-lfo-shape-switch.png");

	auto const bufferLFOShapeSwitchImage = LoadImage("buffer-lfo-shape-switch.png");

	auto const midiModeButtonImage = LoadImage("midi-mode-button.png");
	auto const midiLearnButtonImage = LoadImage("midi-learn-button.png");
	auto const midiResetButtonImage = LoadImage("midi-reset-button.png");

	auto const hostTempoButtonImage = LoadImage("host-tempo-button.png");


	// create controls
	
	DGRect pos;

	auto const divisorLFORateTag = getparameter_b(kDivisorLFOTempoSync) ? kDivisorLFORate_Sync : kDivisorLFORate_Hz;
	pos.set(kDivisorLFORateSliderX, kDivisorLFORateSliderY, kSliderWidth, kLFOSliderHeight);
	mDivisorLFORateSlider = emplaceControl<DGSlider>(this, divisorLFORateTag, pos, dfx::kAxis_Horizontal, sliderHandleImage);
	mDivisorLFORateSlider->setAlternateHandle(sliderHandleImage_glowing);

	pos.set(kDivisorLFODepthSliderX, kDivisorLFODepthSliderY, kSliderWidth, kLFOSliderHeight);
	emplaceControl<DGSlider>(this, kDivisorLFODepth, pos, dfx::kAxis_Horizontal, sliderHandleImage)->setAlternateHandle(sliderHandleImage_glowing);

	auto const bufferLFORateTag = getparameter_b(kBufferLFOTempoSync) ? kBufferLFORate_Sync : kBufferLFORate_Hz;
	pos.set(kBufferLFORateSliderX, kBufferLFORateSliderY, kSliderWidth, kLFOSliderHeight);
	mBufferLFORateSlider = emplaceControl<DGSlider>(this, bufferLFORateTag, pos, dfx::kAxis_Horizontal, sliderHandleImage);
	mBufferLFORateSlider->setAlternateHandle(sliderHandleImage_glowing);

	pos.set(kBufferLFODepthSliderX, kBufferLFODepthSliderY, kSliderWidth, kLFOSliderHeight);
	emplaceControl<DGSlider>(this, kBufferLFODepth, pos, dfx::kAxis_Horizontal, sliderHandleImage)->setAlternateHandle(sliderHandleImage_glowing);

	pos.set(kSmoothSliderX, kSmoothSliderY, kSliderWidth, kSliderHeight);
	emplaceControl<DGSlider>(this, kSmooth, pos, dfx::kAxis_Horizontal, sliderHandleImage)->setAlternateHandle(sliderHandleImage_glowing);

	pos.set(kDryWetMixSliderX, kDryWetMixSliderY, kSliderWidth, kSliderHeight);
	emplaceControl<DGSlider>(this, kDryWetMix, pos, dfx::kAxis_Horizontal, sliderHandleImage)->setAlternateHandle(sliderHandleImage_glowing);

	pos.set(kPitchBendRangeSliderX, kPitchBendRangeSliderY, kSliderWidth, kSliderHeight);
	emplaceControl<DGSlider>(this, kPitchBendRange, pos, dfx::kAxis_Horizontal, sliderHandleImage_pitchbend)->setAlternateHandle(sliderHandleImage_pitchbend_glowing);

	pos.set(kTempoSliderX, kTempoSliderY, kTempoSliderWidth, kSliderHeight);
	emplaceControl<DGSlider>(this, kTempo, pos, dfx::kAxis_Horizontal, sliderHandleImage)->setAlternateHandle(sliderHandleImage_glowing);

	auto const bufferSizeTag = getparameter_b(kBufferTempoSync) ? kBufferSize_Sync : kBufferSize_MS;
	pos.set(kDivisorBufferBoxX, kDivisorBufferBoxY, kDivisorBufferBoxWidth, kDivisorBufferBoxHeight);
	mDivisorBufferBox = emplaceControl<DGXYBox>(this, kDivisor, bufferSizeTag, pos, xyBoxHandleImage, nullptr, 
												VSTGUI::CSliderBase::kLeft | VSTGUI::CSliderBase::kTop);
	mDivisorBufferBox->setAlternateHandles(xyBoxHandleImage_divisor_glowing, xyBoxHandleImage_buffer_glowing);



	pos.set(kDivisorDisplayX, kDivisorDisplayY, kDisplayWidth, kDisplayHeight);
	emplaceControl<DGTextDisplay>(this, kDivisor, pos, divisorDisplayProc, nullptr, nullptr, dfx::TextAlignment::Right, kValueDisplayRegularFontSize, DGColor::kWhite, kValueDisplayFont);

	pos.set(kBufferDisplayX, kBufferDisplayY, kDisplayWidth, kDisplayHeight);
	mBufferSizeDisplay = emplaceControl<DGTextDisplay>(this, bufferSizeTag, pos, bufferSizeDisplayProc, this, nullptr, dfx::TextAlignment::Center, kValueDisplayRegularFontSize, DGColor::kWhite, kValueDisplayFont);

	pos.set(kDivisorLFORateDisplayX, kDivisorLFORateDisplayY, kLFORateDisplayWidth, kDisplayHeight);
	mDivisorLFORateDisplay = emplaceControl<DGTextDisplay>(this, divisorLFORateTag, pos, divisorLFORateDisplayProc, this, nullptr, dfx::TextAlignment::Right, kValueDisplayTinyFontSize, DGColor::kWhite, kValueDisplayFont);

	pos.set(kDivisorLFODepthDisplayX, kDivisorLFODepthDisplayY, kDisplayWidth, kDisplayHeight);
	emplaceControl<DGTextDisplay>(this, kDivisorLFODepth, pos, lfoDepthDisplayProc, nullptr, nullptr, dfx::TextAlignment::Right, kValueDisplayTinyFontSize, DGColor::kWhite, kValueDisplayFont);

	pos.set(kBufferLFORateDisplayX, kBufferLFORateDisplayY, kLFORateDisplayWidth, kDisplayHeight);
	mBufferLFORateDisplay = emplaceControl<DGTextDisplay>(this, bufferLFORateTag, pos, bufferLFORateDisplayProc, this, nullptr, dfx::TextAlignment::Right, kValueDisplayTinyFontSize, DGColor::kWhite, kValueDisplayFont);

	pos.set(kBufferLFODepthDisplayX, kBufferLFODepthDisplayY, kDisplayWidth, kDisplayHeight);
	emplaceControl<DGTextDisplay>(this, kBufferLFODepth, pos, lfoDepthDisplayProc, nullptr, nullptr, dfx::TextAlignment::Right, kValueDisplayTinyFontSize, DGColor::kWhite, kValueDisplayFont);

	pos.set(kSmoothDisplayX, kSmoothDisplayY, kDisplayWidth, kDisplayHeight);
	emplaceControl<DGTextDisplay>(this, kSmooth, pos, DGTextDisplay::valueToTextProc_Percent, nullptr, nullptr, dfx::TextAlignment::Right, kValueDisplayRegularFontSize, DGColor::kWhite, kValueDisplayFont);

	pos.set(kDryWetMixDisplayX, kDryWetMixDisplayY, kDisplayWidth, kDisplayHeight);
	emplaceControl<DGTextDisplay>(this, kDryWetMix, pos, dryWetMixDisplayProc, nullptr, nullptr, dfx::TextAlignment::Right, kValueDisplayRegularFontSize, DGColor::kWhite, kValueDisplayFont);

	pos.set(kPitchBendRangeDisplayX, kPitchBendRangeDisplayY, kDisplayWidth, kDisplayHeight);
	emplaceControl<DGTextDisplay>(this, kPitchBendRange, pos, pitchBendRangeDisplayProc, nullptr, nullptr, dfx::TextAlignment::Right, kValueDisplayRegularFontSize, DGColor::kWhite, kValueDisplayFont);

	pos.set(kTempoDisplayX, kTempoDisplayY, kDisplayWidth, kDisplayHeight);
	emplaceControl<DGTextDisplay>(this, kTempo, pos, tempoDisplayProc, nullptr, nullptr, dfx::TextAlignment::Left, kValueDisplayTinyFontSize, DGColor::kWhite, kValueDisplayFont);


	// callbacks for button-triggered action
	auto const linkKickButtonsDownProc = [](DGButton* otherButton)
	{
		assert(otherButton);
		otherButton->setMouseIsDown(true);
	};
	auto const linkKickButtonsUpProc = [](DGButton* otherButton)
	{
		assert(otherButton);
		otherButton->setMouseIsDown(false);
	};

	// forced buffer size tempo sync button
	auto const bufferTempoSyncButton = emplaceControl<DGToggleImageButton>(this, kBufferTempoSync, kBufferTempoSyncButtonX, kBufferTempoSyncButtonY, bufferTempoSyncButtonImage, true);
	//
	auto const bufferTempoSyncButtonCorner = emplaceControl<DGToggleImageButton>(this, kBufferTempoSync, kBufferTempoSyncButtonCornerX, kBufferTempoSyncButtonCornerY, bufferTempoSyncButtonCornerImage, true);
	//
	bufferTempoSyncButton->setUserProcedure(std::bind(linkKickButtonsDownProc, bufferTempoSyncButtonCorner));
	bufferTempoSyncButtonCorner->setUserProcedure(std::bind(linkKickButtonsDownProc, bufferTempoSyncButton));
	bufferTempoSyncButton->setUserReleaseProcedure(std::bind(linkKickButtonsUpProc, bufferTempoSyncButtonCorner));
	bufferTempoSyncButtonCorner->setUserReleaseProcedure(std::bind(linkKickButtonsUpProc, bufferTempoSyncButton));

	// buffer interrupt button
	auto const bufferInterruptButton = emplaceControl<DGToggleImageButton>(this, kBufferInterrupt, kBufferInterruptButtonX, kBufferInterruptButtonY, bufferInterruptButtonImage, true);
	//
	auto const bufferInterruptButtonCorner = emplaceControl<DGToggleImageButton>(this, kBufferInterrupt, kBufferInterruptButtonCornerX, kBufferInterruptButtonCornerY, bufferInterruptButtonCornerImage, true);
	//
	bufferInterruptButtonCorner->setUserProcedure(std::bind(linkKickButtonsDownProc, bufferInterruptButton));
	bufferInterruptButton->setUserProcedure(std::bind(linkKickButtonsDownProc, bufferInterruptButtonCorner));
	bufferInterruptButtonCorner->setUserReleaseProcedure(std::bind(linkKickButtonsUpProc, bufferInterruptButton));
	bufferInterruptButton->setUserReleaseProcedure(std::bind(linkKickButtonsUpProc, bufferInterruptButtonCorner));

	// forced buffer size LFO tempo sync button
	emplaceControl<DGToggleImageButton>(this, kBufferLFOTempoSync, kBufferLFOTempoSyncButtonX, kBufferLFOTempoSyncButtonY, bufferLFOTempoSyncButtonImage, true);

	// divisor LFO tempo sync button
	emplaceControl<DGToggleImageButton>(this, kDivisorLFOTempoSync, kDivisorLFOTempoSyncButtonX, kDivisorLFOTempoSyncButtonY, divisorLFOTempoSyncButtonImage, true);

	// MIDI mode button
	pos.set(kMidiModeButtonX, kMidiModeButtonY, midiModeButtonImage->getWidth() / 2, midiModeButtonImage->getHeight() / BufferOverride::kNumMidiModes);
	emplaceControl<DGButton>(this, kMidiMode, pos, midiModeButtonImage, DGButton::Mode::Increment, true);

	// sync to host tempo button
	emplaceControl<DGToggleImageButton>(this, kTempoAuto, kHostTempoButtonX, kHostTempoButtonY, hostTempoButtonImage);

	// MIDI learn button
	CreateMidiLearnButton(kMidiLearnButtonX, kMidiLearnButtonY, midiLearnButtonImage);

	// MIDI reset button
	CreateMidiResetButton(kMidiResetButtonX, kMidiResetButtonY, midiResetButtonImage);


	// forced buffer size LFO shape switch
	pos.set(kBufferLFOShapeSwitchX, kBufferLFOShapeSwitchY, bufferLFOShapeSwitchImage->getWidth(), bufferLFOShapeSwitchImage->getHeight() / dfx::LFO::kNumShapes);
	emplaceControl<DGButton>(this, kBufferLFOShape, pos, bufferLFOShapeSwitchImage, DGButton::Mode::Radio);

	// divisor LFO shape switch
	pos.set(kDivisorLFOShapeSwitchX, kDivisorLFOShapeSwitchY, divisorLFOShapeSwitchImage->getWidth(), divisorLFOShapeSwitchImage->getHeight() / dfx::LFO::kNumShapes);
	emplaceControl<DGButton>(this, kDivisorLFOShape, pos, divisorLFOShapeSwitchImage, DGButton::Mode::Radio);


	// forced buffer size label
	pos.set(kBufferSizeLabelX, kBufferSizeLabelY, bufferSizeLabelImage->getWidth(), bufferSizeLabelImage->getHeight() / 2);
	emplaceControl<DGButton>(this, kBufferTempoSync, pos, bufferSizeLabelImage, DGButton::Mode::PictureReel);

	// forced buffer size LFO rate label
	pos.set(kBufferLFORateLabelX, kBufferLFORateLabelY, bufferLFORateLabelImage->getWidth(), bufferLFORateLabelImage->getHeight() / 2);
	emplaceControl<DGButton>(this, kBufferLFOTempoSync, pos, bufferLFORateLabelImage, DGButton::Mode::PictureReel);

	// divisor LFO rate label
	pos.set(kDivisorLFORateLabelX, kDivisorLFORateLabelY, divisorLFORateLabelImage->getWidth(), divisorLFORateLabelImage->getHeight() / 2);
	emplaceControl<DGButton>(this, kDivisorLFOTempoSync, pos, divisorLFORateLabelImage, DGButton::Mode::PictureReel);


	// the help mouseover hint thingy
	pos.set(kHelpDisplayX, kHelpDisplayY, GetBackgroundImage()->getWidth(), kDisplayHeight);
	mHelpDisplay = emplaceControl<DGStaticTextDisplay>(this, pos, nullptr, dfx::TextAlignment::Center, kHelpDisplayFontSize, kHelpDisplayTextColor, kHelpDisplayFont);


	HandleTempoSyncChange();
	HandleTempoAutoChange();


	return dfx::kStatus_NoError;
}

//-----------------------------------------------------------------------------
void BufferOverrideEditor::CloseEditor()
{
	mDivisorBufferBox = nullptr;
	mDivisorLFORateSlider = nullptr;
	mBufferLFORateSlider = nullptr;
	mBufferSizeDisplay = nullptr;
	mDivisorLFORateDisplay = nullptr;
	mBufferLFORateDisplay = nullptr;
	mHelpDisplay = nullptr;
}


//-----------------------------------------------------------------------------
void BufferOverrideEditor::parameterChanged(long inParameterID)
{
	IDGControl* slider = nullptr;
	DGTextDisplay* textDisplay = nullptr;
	auto const useSyncParam = getparameter_b(inParameterID);

	auto newParameterID = dfx::kParameterID_Invalid;
	switch (inParameterID)
	{
		case kBufferTempoSync:
		{
			HandleTempoSyncChange();
			constexpr std::array<long, 2> parameterIDs = { kBufferSize_MS, kBufferSize_Sync };
			newParameterID = parameterIDs[useSyncParam];
			slider = mDivisorBufferBox->getControlByParameterID(parameterIDs[!useSyncParam]);
			textDisplay = mBufferSizeDisplay;
			break;
		}
		case kDivisorLFOTempoSync:
			HandleTempoSyncChange();
			newParameterID = useSyncParam ? kDivisorLFORate_Sync : kDivisorLFORate_Hz;
			slider = mDivisorLFORateSlider;
			textDisplay = mDivisorLFORateDisplay;
			break;
		case kBufferLFOTempoSync:
			HandleTempoSyncChange();
			newParameterID = useSyncParam ? kBufferLFORate_Sync : kBufferLFORate_Hz;
			slider = mBufferLFORateSlider;
			textDisplay = mBufferLFORateDisplay;
			break;
		case kTempoAuto:
			HandleTempoAutoChange();
			break;
		default:
			return;
	}

	if (slider)
	{
		slider->setParameterID(newParameterID);
	}
	if (textDisplay)
	{
		textDisplay->setParameterID(newParameterID);
	}
}


//-----------------------------------------------------------------------------
void BufferOverrideEditor::mouseovercontrolchanged(IDGControl* currentControlUnderMouse)
{
	if (!mHelpDisplay)
	{
		return;
	}

	auto currentcontrolparam = dfx::kParameterID_Invalid;
	if (currentControlUnderMouse && currentControlUnderMouse->isParameterAttached())
	{
		currentcontrolparam = currentControlUnderMouse->getParameterID();
	}

	char const* helpstring = nullptr;
	switch (currentcontrolparam)
	{
		case kDivisor:
			helpstring = "buffer divisor is the number of times each forced buffer skips and starts over";
			break;
		case kBufferSize_MS:
		case kBufferSize_Sync:
			helpstring = "forced buffer size is the length of the sound chunks upon which " PLUGIN_NAME_STRING " operates";
			break;
		case kBufferTempoSync:
			helpstring = "turn on tempo sync if you want the size of the forced buffers to sync to your tempo";
			break;
		case kBufferInterrupt:
			helpstring = "turn this off for the old version 1 style of stubborn \"stuck\" buffers (if you really want that)";
			break;
		case kDivisorLFORate_Hz:
		case kDivisorLFORate_Sync:
			helpstring = "this is the speed of the LFO that modulates the buffer divisor";
			break;
		case kDivisorLFODepth:
			helpstring = "the depth (or intensity) of the LFO that modulates the buffer divisor (0% does nothing)";
			break;
		case kDivisorLFOShape:
			helpstring = "choose the waveform shape of the LFO that modulates the buffer divisor";
			break;
		case kDivisorLFOTempoSync:
			helpstring = "turn on tempo sync if you want the rate of the buffer divisor LFO to sync to your tempo";
			break;
		case kBufferLFORate_Hz:
		case kBufferLFORate_Sync:
			helpstring = "this is the speed of the LFO that modulates the forced buffer size";
			break;
		case kBufferLFODepth:
			helpstring = "the depth (or intensity) of the LFO that modulates the forced buffer size (0% does nothing)";
			break;
		case kBufferLFOShape:
			helpstring = "choose the waveform shape of the LFO that modulates the forced buffer size";
			break;
		case kBufferLFOTempoSync:
			helpstring = "turn on tempo sync if you want the rate of the forced buffer size LFO to sync to your tempo";
			break;
		case kSmooth:
			helpstring = "the portion of each minibuffer spent smoothly crossfading the previous one into the new one (prevents glitches)";
			break;
		case kDryWetMix:
			helpstring = "the relative mix of the processed sound and the clean/original sound (100% is all processed)";
			break;
		case kPitchBendRange:
			helpstring = "the range, in semitones, of MIDI pitch bend's effect on the buffer divisor";
			break;
		case kMidiMode:
			helpstring = "nudge: MIDI notes adjust the buffer divisor.   trigger: notes also reset the divisor to 1 when they are released";
			break;
		case kTempo:
			helpstring = "you can adjust the tempo used for sync, or enable the \"host tempo\" button to get tempo from your host";
			break;
		case kTempoAuto:
			helpstring = "enable this to get the tempo from your host application";
			break;

		default:
			if (currentControlUnderMouse)
			{
				if (currentControlUnderMouse == GetMidiLearnButton())
				{
					helpstring = "activate or deactivate learn mode for assigning MIDI CCs to control " PLUGIN_NAME_STRING "'s parameters";
				}
				else if (currentControlUnderMouse == GetMidiResetButton())
				{
					helpstring = "press this button to erase all of your CC assignments";
				}
			}
			break;
	}

	if (currentControlUnderMouse == mDivisorBufferBox)
	{
#if TARGET_OS_MAC
	#define BO_XLOCK_KEY "option"
#else
	#define BO_XLOCK_KEY "alt"
#endif
		helpstring = "left/right is buffer divisor (number of skips in a buffer, hold ctrl).  up/down is forced buffer size (hold " BO_XLOCK_KEY ")";
#undef BO_XLOCK_KEY
	}

	mHelpDisplay->setText(helpstring ? helpstring : "");
}

//-----------------------------------------------------------------------------
void BufferOverrideEditor::HandleTempoSyncChange()
{
	auto const updateTextDisplay = [this](VSTGUI::CControl* control, long tempoSyncParameterID)
	{
		auto const allowTextEdit = !getparameter_b(tempoSyncParameterID);
		control->setMouseEnabled(allowTextEdit);
	};
	updateTextDisplay(mBufferSizeDisplay, kBufferTempoSync);
	updateTextDisplay(mDivisorLFORateDisplay, kDivisorLFOTempoSync);
	updateTextDisplay(mBufferLFORateDisplay, kBufferLFOTempoSync);
}

//-----------------------------------------------------------------------------
void BufferOverrideEditor::HandleTempoAutoChange()
{
	float const alpha = getparameter_b(kTempoAuto) ? kUnusedControlAlpha : 1.f;
	SetParameterAlpha(kTempo, alpha);
}
