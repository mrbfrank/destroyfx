#ifndef __TOM7_TRANSVERBEDITOR_H
#define __TOM7_TRANSVERBEDITOR_H


#include "dfxgui.h"


// ____________________________________________________________________________
class TransverbEditor : public DfxGuiEditor
{
public:
	TransverbEditor(AudioUnitCarbonView inInstance);
	virtual ~TransverbEditor();
	
	virtual OSStatus open(Float32 inXOffset, Float32 inYOffset);
};


#endif