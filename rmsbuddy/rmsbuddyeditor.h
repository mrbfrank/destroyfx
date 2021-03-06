/*------------------------------------------------------------------------
Copyright (C) 2001-2010  Sophia Poirier

This file is part of RMS Buddy.

RMS Buddy is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
(at your option) any later version.

RMS Buddy is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with RMS Buddy.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/

#ifndef _DFX_RMS_BUDDY_EDITOR_H
#define _DFX_RMS_BUDDY_EDITOR_H


#include "AUCarbonViewBase.h"


class RMSBuddyEditor;

const long kRMSInvalidParameterID = -1;

//-----------------------------------------------------------------------------
// a generic custom control class, which our useful controls are derived from
class RMSControl
{
public:
	RMSControl(RMSBuddyEditor * inOwnerEditor, long inXpos, long inYpos, long inWidth, long inHeight, 
				long inControlRange, long inParamID = kRMSInvalidParameterID);
	virtual ~RMSControl();

	// draw the control
	virtual void draw(CGContextRef inContext, long inPortHeight)
		{ }
	// handle a mouse click in the control
	virtual void mouseDown(float inXpos, float inYpos)
		{ }
	// handle a mouse drag on the control (mouse moved while clicked)
	virtual void mouseTrack(float inXpos, float inYpos)
		{ }
	// handle a mouse button release on the control
	virtual void mouseUp(float inXpos, float inYpos)
		{ }

	// force a redraw of the control to occur
	void redraw();

	// set/get the Carbon Control Manager control reference
	void setCarbonControl(ControlRef inCarbonControl)
		{	carbonControl = inCarbonControl;	}
	ControlRef getCarbonControl()
		{	return carbonControl;	}

	// get the rectangular position of the control
	CGRect getBoundsRect();
	// says whether or not this control is connected to an AU parameter
	bool isParameterAttached()
		{	return hasParameter;	}
	CAAUParameter * getAUVP()
		{	return &auvParam;	}
	void setParameterValue(float inValue);

protected:
	RMSBuddyEditor * ownerEditor;
	long width, height;
	ControlRef carbonControl;
	CAAUParameter auvParam;
	bool hasParameter;
};



// 3-component RGB color description with alpha level, 0.0 - 1.0 values for each field
typedef struct {
	float r;	// red
	float g;	// green
	float b;	// blue
	float a;	// alpha
} RMSColor;

//-----------------------------------------------------------------------------
// a text display control object
class RMSTextDisplay : public RMSControl
{
public:
	RMSTextDisplay(RMSBuddyEditor * inOwnerEditor, long inXpos, long inYpos, long inWidth, long inHeight, 
					RMSColor inTextColor, RMSColor inBackColor, RMSColor inFrameColor, 
					ThemeFontID inFontID, HIThemeTextHorizontalFlush inTextAlignment, 
					long inParamID = kRMSInvalidParameterID);
	virtual ~RMSTextDisplay();

	virtual void draw(CGContextRef inContext, long inPortHeight);
	// set the display text directly with a string
	void setText(CFStringRef inText);
	// given a linear amplitude value, set the display text with the dB-converted value
	void setText_dB(double inLinearValue);
	// set the display text with an integer value
	void setText_int(long inValue);

private:
	RMSColor textColor, backColor, frameColor;
	ThemeFontID fontID;
	HIThemeTextHorizontalFlush textAlignment;
	CFStringRef text;
};


//-----------------------------------------------------------------------------
// a push-button control object
class RMSButton : public RMSControl
{
public:
	RMSButton(RMSBuddyEditor * inOwnerEditor, long inParamID, long inXpos, long inYpos, CGImageRef inImage);

	virtual void draw(CGContextRef inContext, long inPortHeight);
	virtual void mouseDown(float inXpos, float inYpos);
	virtual void mouseTrack(float inXpos, float inYpos);
	virtual void mouseUp(float inXpos, float inYpos);

private:
	CGImageRef buttonImage;
};


//-----------------------------------------------------------------------------
// a slider control object
class RMSSlider : public RMSControl
{
public:
	RMSSlider(RMSBuddyEditor * inOwnerEditor, long inParamID, long inXpos, long inYpos, long inWidth, long inHeight, 
				RMSColor inBackColor, RMSColor inFillColor);
	virtual ~RMSSlider();

	virtual void draw(CGContextRef inContext, long inPortHeight);
	virtual void mouseDown(float inXpos, float inYpos);
	virtual void mouseTrack(float inXpos, float inYpos);

private:
	RMSColor backColor;
	RMSColor fillColor;
	long borderWidth;
};



//-----------------------------------------------------------------------------
// our GUI
class RMSBuddyEditor : public AUCarbonViewBase
{
public:
	RMSBuddyEditor(AudioUnitCarbonView inInstance);
	virtual ~RMSBuddyEditor();

	virtual OSStatus CreateUI(Float32 inXOffset, Float32 inYOffset);
	virtual bool HandleEvent(EventHandlerCallRef inHandlerRef, EventRef inEvent);
	virtual OSStatus Version()
		{	return RMS_BUDDY_VERSION;	}

	OSStatus setup();
	void cleanup();

	void updateDisplays();	// refresh the value displays
	void updateWindowSize(Float32 inParamValue, RMSControl * inRMSControl);	// update analysis window size parameter controls
	void updateStreamFormatChange();

	// these are accessors for data that the custom controls need in order to create themselves
	ControlDefSpec * getControlClassSpec()
		{	return &controlClassSpec;	}
	AUParameterListenerRef GetParameterListener()
//		{	return mParameterListener;	}
		{	return parameterListener;	}
	void AddAUCVControl(AUCarbonViewControl * inControl)
		{	AddControl(inControl);	}
	OSStatus SendAUParameterEvent(AudioUnitParameterID inParameterID, AudioUnitEventType inEventType);

	// get/set the control that is currently being moused (actively tweaked), if any (returns NULL if none)
	RMSControl * getCurrentControl()
		{	return currentControl;	}
	void setCurrentControl(RMSControl * inNewClickedControl)
		{	currentControl = inNewClickedControl;	}

	void handleControlValueChange(RMSControl * inControl, SInt32 inControlValue);

private:
	UInt32 getAUNumChannels();

	UInt32 numChannels;	// the number of channels being analyzed

	// buttons
	RMSButton * resetRMSbutton;
	RMSButton * resetPeakButton;
	RMSButton * helpButton;

	// text display boxes
	RMSTextDisplay ** averageRMSDisplays;
	RMSTextDisplay ** continualRMSDisplays;
	RMSTextDisplay ** absolutePeakDisplays;
	RMSTextDisplay ** continualPeakDisplays;
	RMSTextDisplay * averageRMSLabel;
	RMSTextDisplay * continualRMSLabel;
	RMSTextDisplay * absolutePeakLabel;
	RMSTextDisplay * continualPeakLabel;
	RMSTextDisplay ** channelLabels;

	// slider
	RMSSlider * windowSizeSlider;
	RMSTextDisplay * windowSizeLabel;
	RMSTextDisplay * windowSizeDisplay;

	// images
	CGImageRef resetButtonImage;
	CGImageRef helpButtonImage;

	// event handling related stuff
	EventHandlerUPP controlHandlerUPP;
	ControlDefSpec controlClassSpec;
	EventHandlerUPP windowEventHandlerUPP;
	EventHandlerRef windowEventEventHandlerRef;
	RMSControl * currentControl;

	// parameter listener related stuff
	AUParameterListenerRef parameterListener;

	// property listener related stuff
	AUEventListenerRef propertyEventListener;
	AudioUnitEvent dynamicsDataPropertyAUEvent;
	AudioUnitEvent numChannelsPropertyAUEvent;
};


#endif
