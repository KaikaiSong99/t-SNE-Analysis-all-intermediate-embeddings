#include "HsneAnalysisPlugin.h"

#include "PointData.h"
#include "HsneParameters.h"

#include <QtCore>
#include <QtDebug>

Q_PLUGIN_METADATA(IID "nl.tudelft.HsneAnalysisPlugin")
#ifdef _WIN32
#include <windows.h>
#endif

#include <set>

#include <QMenu>

// =============================================================================
// View
// =============================================================================

using namespace hdps;

HsneAnalysisPlugin::HsneAnalysisPlugin() :
    AnalysisPlugin("H-SNE Analysis")
{

}

HsneAnalysisPlugin::~HsneAnalysisPlugin(void)
{
    
}

void HsneAnalysisPlugin::init()
{
    // Create a new settings widget which allows users to change the parameters given to the HSNE analysis
    _settings = std::make_unique<HsneSettingsWidget>(*this);

    // If a different input dataset is picked in the settings widget update the dimension widget
    connect(_settings.get(), &HsneSettingsWidget::dataSetPicked, this, &HsneAnalysisPlugin::dataSetPicked);
    // If the start computation button is pressed, run the HSNE algorithm
    connect(_settings.get(), &HsneSettingsWidget::startComputation, this, &HsneAnalysisPlugin::startComputation);

    registerDataEventByType(PointType, std::bind(&HsneAnalysisPlugin::onDataEvent, this, std::placeholders::_1));

    //connect(_settings.get(), &HsneSettingsWidget::stopComputation, this, &HsneAnalysisPlugin::stopComputation);
    //connect(&_tsne, &TsneAnalysis::computationStopped, _settings.get(), &HsneSettingsWidget::onComputationStopped);
    connect(&_tsne, &TsneAnalysis::newEmbedding, this, &HsneAnalysisPlugin::onNewEmbedding);
    //connect(&_tsne, SIGNAL(newEmbedding()), this, SLOT(onNewEmbedding()));
}

void HsneAnalysisPlugin::onDataEvent(hdps::DataEvent* dataEvent)
{
    switch (dataEvent->getType())
    {
    case EventType::DataAdded:
    {
        _settings->addDataItem(dataEvent->dataSetName);
        break;
    }
    case EventType::DataChanged:
    {
        // If we are not looking at the changed dataset, ignore it
        if (dataEvent->dataSetName != _settings->getCurrentDataItem()) {
            break;
        }

        // Passes changes to the current dataset to the dimension selection widget
        Points& points = _core->requestData<Points>(dataEvent->dataSetName);

        _settings->getDimensionSelectionWidget().dataChanged(points);
        break;
    }
    case EventType::DataRemoved:
    {
        _settings->removeDataItem(dataEvent->dataSetName);
        break;
    }
    }
}

SettingsWidget* const HsneAnalysisPlugin::getSettings()
{
    return _settings.get();
}

// If a different input dataset is picked in the settings widget update the dimension widget
void HsneAnalysisPlugin::dataSetPicked(const QString& name)
{
    Points& points = _core->requestData<Points>(name);

    auto analyses = points.getProperty("Analyses", QVariantList()).toList();
    analyses.push_back(getName());
    points.setProperty("Analyses", analyses);

    _settings->getDimensionSelectionWidget().dataChanged(points);
}

void HsneAnalysisPlugin::startComputation()
{
    //initializeTsne();

    /********************/
    /* Prepare the data */
    /********************/
    // Obtain a reference to the the input dataset
    QString setName = _settings->getCurrentDataItem();
    const Points& inputData = _core->requestData<Points>(setName);

    // Get the HSNE parameters from the settings widget
    HsneParameters parameters = _settings->getHsneParameters();

    std::vector<bool> enabledDimensions = _settings->getDimensionSelectionWidget().getEnabledDimensions();

    // Initialize the HSNE algorithm with the given parameters
    _hierarchy.initialize(_core, inputData, enabledDimensions, parameters);

    _embeddingNameBase = _settings->getEmbeddingName();
    _inputDataName = setName;
    computeTopLevelEmbedding();
}

void HsneAnalysisPlugin::onNewEmbedding() {
    const TsneData& outputData = _tsne.output();
    Points& embedding = _core->requestData<Points>(_embeddingName);

    embedding.setData(outputData.getData().data(), outputData.getNumPoints(), 2);

    _core->notifyDataChanged(_embeddingName);
}

QString HsneAnalysisPlugin::createEmptyEmbedding(QString name, QString dataType, QString sourceName)
{
    QString embeddingName = _core->addData(dataType, name);
    //QString embeddingName = _core->createDerivedData(dataType, name, sourceName);
    Points& embedding = _core->requestData<Points>(embeddingName);
    embedding.setData(nullptr, 0, 2);

    auto analyses = embedding.getProperty("Analyses", QVariantList()).toList();
    analyses.push_back(getName());
    embedding.setProperty("Analyses", analyses);

    _core->notifyDataAdded(embeddingName);
    return embeddingName;
}

QString HsneAnalysisPlugin::createEmptyDerivedEmbedding(QString name, QString dataType, QString sourceName)
{
    //QString embeddingName = _core->addData(dataType, name);
    QString embeddingName = _core->createDerivedData(name, sourceName);
    Points& embedding = _core->requestData<Points>(embeddingName);
    embedding.setData(nullptr, 0, 2);

    auto analyses = embedding.getProperty("Analyses", QVariantList()).toList();
    analyses.push_back(getName());
    embedding.setProperty("Analyses", analyses);

    _core->notifyDataAdded(embeddingName);
    return embeddingName;
}

void HsneAnalysisPlugin::computeTopLevelEmbedding()
{
    // Create a new data set for the embedding
    int topScale = _hierarchy.getTopScale();
    //_embeddingName = createEmptyEmbedding(_embeddingNameBase + "_scale_" + QString::number(topScale), "Points", _hierarchy.getInputDataName());

    {
        Points& inputData = _core->requestData<Points>(_inputDataName);
        Points& selection = static_cast<Points&>(inputData.getSelection());
        Hsne::scale_type& dScale = _hierarchy.getScale(topScale);

        //std::vector<unsigned int> dataIndices;
        selection.indices.clear();
        for (int i = 0; i < dScale._landmark_to_original_data_idx.size(); i++)
        {
            selection.indices.push_back(dScale._landmark_to_original_data_idx[i]);
        }

        QString subsetName = inputData.createSubset();
        selection.indices.clear();
        Points& subset = _core->requestData<Points>(subsetName);
        std::cout << "Subset points: " << subset.getNumPoints() << std::endl;
        _embeddingName = createEmptyDerivedEmbedding(_embeddingNameBase + "_scale_" + QString::number(topScale), "Points", subsetName);
    }
    
    Points& embedding = _core->requestData<Points>(_embeddingName);
    embedding.setProperty("scale", topScale);
    embedding.setProperty("landmarkMap", qVariantFromValue(_hierarchy.getInfluenceHierarchy().getMap()[topScale]));
    
    _hierarchy.printScaleInfo();

    // Set t-SNE parameters
    HsneParameters hsneParameters = _settings->getHsneParameters();
    TsneParameters tsneParameters = _settings->getTsneParameters();

    _tsne.setIterations(tsneParameters.getNumIterations());
    _tsne.setPerplexity(tsneParameters.getPerplexity());
    _tsne.setNumTrees(tsneParameters.getNumTrees());
    _tsne.setNumChecks(tsneParameters.getNumChecks());
    _tsne.setExaggerationIter(tsneParameters.getExaggerationIter());
    _tsne.setExponentialDecayIter(tsneParameters.getExponentialDecayIter());
    _tsne.setKnnAlgorithm(hsneParameters.getKnnLibrary());

    if (_tsne.isRunning())
    {
        // Request interruption of the computation
        _tsne.stopGradientDescent();
        _tsne.exit();

        // Wait until the thread has terminated (max. 3 seconds)
        if (!_tsne.wait(3000))
        {
            qDebug() << "tSNE computation thread did not close in time, terminating...";
            _tsne.terminate();
            _tsne.wait();
        }
        qDebug() << "tSNE computation stopped.";
    }
    // Initialize tsne, compute data to be embedded, start computation?
    _tsne.initWithProbDist(_hierarchy.getNumPoints(), _hierarchy.getNumDimensions(), _hierarchy.getTransitionMatrixAtScale(topScale)); // FIXME
    // Embed data
    _tsne.start();
}

void HsneAnalysisPlugin::onDrillIn()
{
    //_hsne.drillIn("Temp");
}

void HsneAnalysisPlugin::drillIn(QString embeddingName)
{
    // Request the embedding from the core and find out the source data from which it derives
    Points& embedding = _core->requestData<Points>(embeddingName);
    Points& source = hdps::DataSet::getSourceData<Points>(embedding);

    // Get associated selection with embedding
    Points& selection = static_cast<Points&>(embedding.getSelection());

    // Scale the embedding is a part of
    int currentScale = embedding.getProperty("scale").value<int>();
    int drillScale = currentScale - 1;

    // Find proper selection indices
    std::vector<bool> pointsSelected;
    embedding.selectedLocalIndices(selection.indices, pointsSelected);

    std::vector<unsigned int> selectionIndices;
    if (embedding.hasProperty("drill_indices"))
    {
        QList<uint32_t> drillIndices = embedding.getProperty("drill_indices").value<QList<uint32_t>>();
        
        //for (const uint32_t& selectionIndex : selection.indices)
        //    selectionIndices.push_back(selectionIndex);//drillIndices[selectionIndex]);
        for (int i = 0; i < pointsSelected.size(); i++)
        {
            if (pointsSelected[i])
            {
                selectionIndices.push_back(drillIndices[i]);
            }
        }
    }
    else
    {
        //selectionIndices = selection.indices;
        for (int i = 0; i < pointsSelected.size(); i++)
        {
            if (pointsSelected[i])
            {
                selectionIndices.push_back(i);
            }
        }
    }

    // Find the points in the previous level corresponding to selected landmarks
    std::map<uint32_t, float> neighbors;
    _hierarchy.getInfluencedLandmarksInPreviousScale(currentScale, selectionIndices, neighbors);

    // Threshold neighbours with enough influence
    std::vector<uint32_t> nextLevelIdxs;
    nextLevelIdxs.clear();
    for (auto n : neighbors) {
        if (n.second > 0.5) //QUICKPAPER
        {
            nextLevelIdxs.push_back(n.first);
        }
    }
    std::cout << "#selected indices: " << selectionIndices.size() << std::endl;
    std::cout << "#landmarks at previous scale: " << neighbors.size() << std::endl;
    std::cout << "#thresholded at previous scale: " << nextLevelIdxs.size() << std::endl;
    std::cout << "Drilling in" << std::endl;

    // Compute the transition matrix for the landmarks above the threshold
    HsneMatrix transitionMatrix;
    _hierarchy.getTransitionMatrixForSelection(currentScale, transitionMatrix, nextLevelIdxs);

    // Create a new data set for the embedding
    //if (drillScale == 0)
    {
        Points& inputData = _core->requestData<Points>(_inputDataName);
        Points& selection = static_cast<Points&>(inputData.getSelection());
        Hsne::scale_type& dScale = _hierarchy.getScale(drillScale);

        //std::vector<unsigned int> dataIndices;
        selection.indices.clear();
        for (int i = 0; i < nextLevelIdxs.size(); i++)
        {
            selection.indices.push_back(dScale._landmark_to_original_data_idx[nextLevelIdxs[i]]);
        }
        
        QString subsetName = inputData.createSubset();
        Points& subset = _core->requestData<Points>(subsetName);

        _embeddingName = createEmptyDerivedEmbedding("Drill Embedding", "Points", subsetName);
    }
    //else
    //{
    //    _embeddingName = createEmptyEmbedding("Drill Embedding", "Points", _hierarchy.getInputDataName());
    //}

    // Store drill indices with embedding
    Points& drillEmbedding = _core->requestData<Points>(_embeddingName);
    QList<uint32_t> indices(nextLevelIdxs.begin(), nextLevelIdxs.end());
    QVariant variantIndices = QVariant::fromValue<QList<uint32_t>>(indices);
    drillEmbedding.setProperty("drill_indices", variantIndices);
    drillEmbedding.setProperty("scale", drillScale);
    drillEmbedding.setProperty("landmarkMap", qVariantFromValue(_hierarchy.getInfluenceHierarchy().getMap()[drillScale]));
    
    // Set t-SNE parameters
    HsneParameters hsneParameters = _settings->getHsneParameters();
    TsneParameters tsneParameters = _settings->getTsneParameters();

    _tsne.setIterations(tsneParameters.getNumIterations());
    _tsne.setPerplexity(tsneParameters.getPerplexity());
    _tsne.setNumTrees(tsneParameters.getNumTrees());
    _tsne.setNumChecks(tsneParameters.getNumChecks());
    _tsne.setExaggerationIter(tsneParameters.getExaggerationIter());
    _tsne.setExponentialDecayIter(tsneParameters.getExponentialDecayIter());
    _tsne.setKnnAlgorithm(hsneParameters.getKnnLibrary());

    if (_tsne.isRunning())
    {
        // Request interruption of the computation
        _tsne.stopGradientDescent();
        _tsne.exit();

        // Wait until the thread has terminated (max. 3 seconds)
        if (!_tsne.wait(3000))
        {
            qDebug() << "tSNE computation thread did not close in time, terminating...";
            _tsne.terminate();
            _tsne.wait();
        }
        qDebug() << "tSNE computation stopped.";
    }
    // Initialize tsne, compute data to be embedded, start computation?
    _tsne.initWithProbDist(nextLevelIdxs.size(), _hierarchy.getNumDimensions(), transitionMatrix);
    // Embed data
    _tsne.start();
}

QMenu* HsneAnalysisPlugin::contextMenu(const QVariant& context)
{
    auto menu = new QMenu(getGuiName());

    auto startComputationAction = new QAction("Start computation");
    auto drillInAction = new QAction("Drill in");

    QMap contextMap = context.value<QMap<QString, QString>>();
    QString currentDataSetName = contextMap["CurrentDataset"];

    connect(startComputationAction, &QAction::triggered, [this]() { startComputation(); });
    connect(drillInAction, &QAction::triggered, [this, currentDataSetName]() { drillIn(currentDataSetName); });

    menu->addAction(startComputationAction);
    menu->addAction(drillInAction);

    return menu;
}

// =============================================================================
// Factory
// =============================================================================

AnalysisPlugin* HsneAnalysisPluginFactory::produce()
{
    return new HsneAnalysisPlugin();
}
