#pragma once

#include "hdi/dimensionality_reduction/knn_utils.h"

class HsneParameters
{
public:
    HsneParameters() :
        _knnLibrary(hdi::dr::KNN_FLANN),
        _seed(-1),
        _useMonteCarloSampling(true),
        _numWalksForLandmarkSelection(15),
        _numWalksForLandmarkSelectionThreshold(1.5f),
        _randomWalkLength(15),
        _numWalksForAreaOfInfluence(100),
        _minWalksRequired(0),
        _numChecksAknn(512),
        _useOutOfCoreComputation(true)
    {

    }

    void setKnnLibrary(hdi::dr::knn_library library) { _knnLibrary = library; }
    void setSeed(int seed) { _seed = seed; }
    void setNumWalksForLandmarkSelection(int numWalks) { _numWalksForLandmarkSelection = numWalks; }
    void setNumWalksForLandmarkSelectionThreshold(float numWalks) { _numWalksForLandmarkSelectionThreshold = numWalks; }
    void setRandomWalkLength(int length) { _randomWalkLength = length; }
    void setNumWalksForAreaOfInfluence(int numWalks) { _numWalksForAreaOfInfluence = numWalks; }
    void setMinWalksRequired(int minWalks) { _minWalksRequired = minWalks; }
    void setNumChecksAKNN(int numChecks) { _numChecksAknn = numChecks; }
    void useMonteCarloSampling(bool useMonteCarloSampling) { _useMonteCarloSampling = useMonteCarloSampling; }
    void useOutOfCoreComputation(bool useOutOfCoreComputation) { _useOutOfCoreComputation = useOutOfCoreComputation; }

    hdi::dr::knn_library getKnnLibrary() const { return _knnLibrary; }
    int getSeed() const { return _seed; }
    int getNumWalksForLandmarkSelection() const { return _numWalksForLandmarkSelection; }
    float getNumWalksForLandmarkSelectionThreshold() const { return _numWalksForLandmarkSelectionThreshold; }
    int getRandomWalkLength() const { return _randomWalkLength; }
    int getNumWalksForAreaOfInfluence() const { return _numWalksForAreaOfInfluence; }
    int getMinWalksRequired() const { return _minWalksRequired; }
    int getNumChecksAKNN() const { return _numChecksAknn; }
    bool useMonteCarloSampling() const { return _useOutOfCoreComputation; }
    bool useOutOfCoreComputation() const { return _useOutOfCoreComputation; }
private:
    hdi::dr::knn_library _knnLibrary;
    /** Seed used for random algorithms. If a negative value is provided a time based seed is used */
    int _seed;
    bool _useMonteCarloSampling;
    int _numWalksForLandmarkSelection;
    float _numWalksForLandmarkSelectionThreshold;
    int _randomWalkLength;
    int _numWalksForAreaOfInfluence;
    int _minWalksRequired;
    int _numChecksAknn;
    bool _useOutOfCoreComputation;
};
