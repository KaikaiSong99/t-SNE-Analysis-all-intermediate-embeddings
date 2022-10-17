#include "HsneSettingsAction.h"
#include "HsneAnalysisPlugin.h"

using namespace hdps::gui;

HsneSettingsAction::HsneSettingsAction(HsneAnalysisPlugin* hsneAnalysisPlugin) :
    GroupAction(hsneAnalysisPlugin, true),
    _hsneAnalysisPlugin(hsneAnalysisPlugin),
    _hsneParameters(),
    _tsneParameters(),
    _generalHsneSettingsAction(*this),
    _advancedHsneSettingsAction(*this),
    _topLevelScaleAction(this, _tsneSettingsAction, hsneAnalysisPlugin->getHierarchy(), hsneAnalysisPlugin->getInputDataset<Points>(), hsneAnalysisPlugin->getOutputDataset<Points>()),
    _tsneSettingsAction(this),
    _dimensionSelectionAction(this)
{
    setText("HSNE");
    setObjectName("Settings");

    _tsneSettingsAction.setObjectName("TSNE");

    const auto updateReadOnly = [this]() -> void {
        _generalHsneSettingsAction.setReadOnly(isReadOnly());
        _advancedHsneSettingsAction.setReadOnly(isReadOnly());
        _topLevelScaleAction.setReadOnly(isReadOnly());
        _tsneSettingsAction.setReadOnly(isReadOnly());
    };

    connect(this, &GroupAction::readOnlyChanged, this, [this, updateReadOnly](const bool& readOnly) {
        updateReadOnly();
    });

    updateReadOnly();
}

HsneParameters& HsneSettingsAction::getHsneParameters()
{
    return _hsneParameters;
}

TsneParameters& HsneSettingsAction::getTsneParameters()
{
    return _tsneParameters;
}