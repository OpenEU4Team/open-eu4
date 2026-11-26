#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

class CAIArmy;

namespace NAICoalition
{
struct OrderedCoalitionStructure
{
    float _vUtilityValue;                                         
    std::vector<size_t> _Members;                                         
};

template <typename T>
class OrderedCoalitionStructureGenerator
{
public:
    struct MemoizationEntry
    {
        T _vUtilityValue;                                              
        size_t _nBestCoalition;                                                     
        bool _bEvaluated;                                                              
    };

    OrderedCoalitionStructure FindOptimalOrderedCoalitionStructure(size_t nAgents, size_t nTasks, std::vector<std::vector<T>> *UtilityValues);                                                                                                                                                

private:
    T SolveRecursive(const unsigned nTaskIndex, const size_t nUnassignedAgentsMask);                                                                                            

    uint32_t _nAgents;                                                                             
    uint32_t _nTasks;                                                                                         
    std::vector<std::vector<T>> *_UtilityValues;                                                                    
    std::vector<std::vector<MemoizationEntry>> _MemoizationTable;                                   
};

template <typename T, typename W>
class CCoalitionFormationGeneratorSequenced
{
public:
    class CCoalitionTask
    {
    public:
        virtual W EvaluateCoalition(const std::vector<T *> &coalition) const = 0;                                                                                                                 

        ~CCoalitionTask() = default;
    };

    struct SPartitionSearchResult
    {
        std::vector<std::vector<T *>> _CollaborationStructure;                                       
        W _UtilityValue;                                                                             
    };

    SPartitionSearchResult GenerateCoalitionFormation();                                                                                                                  

    inline CCoalitionFormationGeneratorSequenced(const std::vector<T *> *pAgents, const std::vector<const CCoalitionTask *> *pTasks)                                                                                             
        : _Agents(pAgents), _Tasks(pTasks), _UtilityValues() {};

private:
    const std::vector<T *> *_Agents;                                                 
    const std::vector<const CCoalitionTask *> *_Tasks;                               
    std::vector<std::vector<W>> _UtilityValues;                                                     
    OrderedCoalitionStructureGenerator<W> _Generator;                                         
};

using CCoalitionTask = CCoalitionFormationGeneratorSequenced<CAIArmy, double>::CCoalitionTask;

}                          
