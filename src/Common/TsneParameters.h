#pragma once

enum class GradientDescentType
{
    GPU,
    CPU,
};


class TsneParameters
{
public:
    TsneParameters() :
        _numIterations(1000),
        _perplexity(30),
        _exaggerationIter(250),
        _exponentialDecayIter(150),
        _numDimensionsOutput(2),
        _presetEmbedding(false),
        _exaggerationFactor(4),
        _updateCore(10),
        _gradientDescentType(GradientDescentType::CPU),
        _subsampleFactor(10)
    {

    }

    void setNumIterations(int numIterations) { _numIterations = numIterations; }
    void setPerplexity(int perplexity) { _perplexity = perplexity; }
    void setExaggerationIter(int exaggerationIter) { _exaggerationIter = exaggerationIter; }
    void setExponentialDecayIter(int exponentialDecayIter) { _exponentialDecayIter = exponentialDecayIter; }
    void setNumDimensionsOutput(int numDimensionsOutput) { _numDimensionsOutput = numDimensionsOutput; }
    void setPresetEmbedding(bool presetEmbedding) { _presetEmbedding = presetEmbedding; }
    void setExaggerationFactor(double exaggerationFactor) { _exaggerationFactor = exaggerationFactor; }
    void setGradientDescentType(GradientDescentType gradientDescentType) { _gradientDescentType = gradientDescentType; }
    void setUpdateCore(int updateCore) { _updateCore = updateCore; }
    void setSubsampleFactor(int subsampleFactor) { _subsampleFactor = subsampleFactor; }

    int getNumIterations() const { return _numIterations; }
    int getPerplexity() const { return _perplexity; }
    int getExaggerationIter() const { return _exaggerationIter; }
    int getExponentialDecayIter() const { return _exponentialDecayIter; }
    int getNumDimensionsOutput() const { return _numDimensionsOutput; }
    int getPresetEmbedding() const { return _presetEmbedding; }
    int getExaggerationFactor() const { return _exaggerationFactor; }
    GradientDescentType getGradientDescentType() const { return _gradientDescentType; }
    int getUpdateCore() const { return _updateCore; }
    int getSubsampleFactor() const { return _subsampleFactor; }

private:
    int _numIterations;
    int _perplexity;
    int _exaggerationIter;
    int _exponentialDecayIter;
    int _numDimensionsOutput;
    double _exaggerationFactor;
    bool _presetEmbedding;
    int _subsampleFactor;
    GradientDescentType _gradientDescentType;     // Whether to use CPU or GPU gradient descent

    int _updateCore;        // Gradient descent iterations after which the embedding data set in ManiVault's core will be updated
};
