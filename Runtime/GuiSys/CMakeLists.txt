set(GUISYS_SOURCES
    CSplashScreen.hpp CSplashScreen.cpp
    CGuiObject.hpp CGuiObject.cpp
    CConsoleOutputWindow.hpp CConsoleOutputWindow.cpp
    CAuiEnergyBarT01.hpp CAuiEnergyBarT01.cpp
    CAuiImagePane.hpp CAuiImagePane.cpp
    CAuiMeter.hpp CAuiMeter.cpp
    CConsoleOutputWindow.hpp CConsoleOutputWindow.cpp
    CErrorOutputWindow.hpp CErrorOutputWindow.cpp
    CGuiCamera.hpp CGuiCamera.cpp
    CGuiFrame.hpp CGuiFrame.cpp
    CGuiLight.hpp CGuiLight.cpp
    CGuiModel.hpp CGuiModel.cpp
    CGuiSliderGroup.hpp CGuiSliderGroup.cpp
    CGuiSys.hpp CGuiSys.cpp
    CGuiTableGroup.hpp CGuiTableGroup.cpp
    CGuiTextPane.hpp CGuiTextPane.cpp
    CGuiTextSupport.hpp CGuiTextSupport.cpp
    CGuiWidget.hpp CGuiWidget.cpp
    CGuiWidgetDrawParms.hpp
    CSplashScreen.hpp CSplashScreen.cpp
    CGuiCompoundWidget.hpp CGuiCompoundWidget.cpp
    CSaveableState.hpp CSaveableState.cpp
    CDrawStringOptions.hpp
    CRasterFont.hpp CRasterFont.cpp
    CGuiGroup.hpp CGuiGroup.cpp
    CGuiWidgetIdDB.hpp CGuiWidgetIdDB.cpp
    CGuiHeadWidget.hpp CGuiHeadWidget.cpp
    CGuiPane.hpp CGuiPane.cpp
    CFontRenderState.hpp CFontRenderState.cpp
    CTextExecuteBuffer.hpp CTextExecuteBuffer.cpp
    CTextRenderBuffer.hpp CTextRenderBuffer.cpp
    CInstruction.hpp CInstruction.cpp
    CTextParser.hpp CTextParser.cpp
    CWordBreakTables.hpp CWordBreakTables.cpp
    CFontImageDef.hpp CFontImageDef.cpp
    CStringTable.hpp CStringTable.cpp
    CTargetingManager.hpp CTargetingManager.cpp
    CCompoundTargetReticle.hpp CCompoundTargetReticle.cpp
    COrbitPointMarker.hpp COrbitPointMarker.cpp
    CHudEnergyInterface.hpp CHudEnergyInterface.cpp
    CHudBossEnergyInterface.hpp CHudBossEnergyInterface.cpp
    CHudThreatInterface.hpp CHudThreatInterface.cpp
    CHudMissileInterface.hpp CHudMissileInterface.cpp
    CHudFreeLookInterface.hpp CHudFreeLookInterface.cpp
    CHudDecoInterface.hpp CHudDecoInterface.cpp
    CHudHelmetInterface.hpp CHudHelmetInterface.cpp
    CHudVisorBeamMenu.hpp CHudVisorBeamMenu.cpp
    CHudRadarInterface.hpp CHudRadarInterface.cpp
    CHudBallInterface.hpp CHudBallInterface.cpp
    CHudInterface.hpp
    CScanDisplay.hpp CScanDisplay.cpp)

runtime_add_list(GuiSys GUISYS_SOURCES)
