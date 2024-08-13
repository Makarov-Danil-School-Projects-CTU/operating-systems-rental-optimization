#ifndef __PROGTEST__

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <climits>
#include <cfloat>
#include <cassert>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <numeric>
#include <string>
#include <utility>
#include <vector>
#include <array>
#include <iterator>
#include <set>
#include <list>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <queue>
#include <stack>
#include <deque>
#include <memory>
#include <functional>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <stdexcept>
#include <condition_variable>
#include <pthread.h>
#include <semaphore.h>
#include "progtest_solver.h"
#include "sample_tester.h"

using namespace std;
#endif /* __PROGTEST__ */

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
class CProblemPackInfo {
public:
    CProblemPackInfo(AProblemPack problem, int companyId) :
            problem(std::move(problem)), companyId(companyId), actualPos(0), readyToSend(false) {};

    AProblemPack problem;
    int companyId;
    size_t actualPos;
    bool readyToSend;
};

class COptimizer {
public:
    static bool usingProgtestSolver(void) {
        return true;
    }

    static void checkAlgorithm(AProblem problem) {
        // dummy implementation if usingProgtestSolver() returns true
    }

    void inputFunc(int companyId);

    void outputFunc(int companyId);

    void start(int threadCount);

    void stop(void);

    void addCompany(const ACompany &company);

    void workingFunc();

private:
    vector<ACompany> companies;
    vector<thread> threads;
    vector<shared_ptr<mutex>> mutexes;
    vector<bool> isAliveCompany;
    vector<shared_ptr<condition_variable>> condVars;
    vector<shared_ptr<CProblemPackInfo>> globalReadyPacks;
    shared_ptr<mutex> mtxGlobalInput = make_shared<mutex>();
    shared_ptr<mutex> mtxCountToStop = make_shared<mutex>();
    shared_ptr<mutex> mtxIsAlive = make_shared<mutex>();
    shared_ptr<mutex> mtxReadyPack = make_shared<mutex>();
    shared_ptr<mutex> mtxSolver = make_shared<mutex>();
    shared_ptr<condition_variable> workerCond = make_shared<condition_variable>();
    queue<shared_ptr<CProblemPackInfo>> globalQ;
    map<int, queue<shared_ptr<CProblemPackInfo>>> localQ;
    size_t countToStop;
    AProgtestSolver solver = createProgtestSolver();
};

void COptimizer::addCompany(const ACompany &company) {
    companies.push_back(company);
    mutexes.emplace_back(make_shared<mutex>());
    condVars.emplace_back(make_shared<condition_variable>());
    isAliveCompany.push_back(true);
}

void COptimizer::start(int threadCount) {
    countToStop = companies.size();
    for (size_t i = 0; i < companies.size(); i++) {
        threads.emplace_back(&COptimizer::inputFunc, this, i);
        threads.emplace_back(&COptimizer::outputFunc, this, i);
    }

    for (int i = 0; i < threadCount; i++)
        threads.emplace_back(&COptimizer::workingFunc, this);
}

void COptimizer::stop(void) {
    for (auto &thread: threads)
        thread.join();
}

void COptimizer::inputFunc(int companyId) {
    int i = 0;
    while (true) {
        auto problemPack = companies[companyId]->waitForPack();
        if (!problemPack) {
            unique_lock<mutex> countToStopMtx (*mtxCountToStop);
            countToStop--;
            countToStopMtx.unlock();
            unique_lock<mutex> isAliveMtx (*mtxIsAlive);
            isAliveCompany[companyId] = false;
            isAliveMtx.unlock();
            break;
        }

        auto newCProblemPackInfo = make_shared<CProblemPackInfo>(problemPack, companyId);
        unique_lock<mutex> ulGlobalInput(*mtxGlobalInput);
        globalQ.push(newCProblemPackInfo);
        ulGlobalInput.unlock();

        printf("Added %d pack\n", ++i);

        unique_lock<mutex> companyMutex(*mutexes[companyId]);
        localQ[companyId].push(newCProblemPackInfo);
        companyMutex.unlock();

        workerCond->notify_one();
    }
    printf("Receiver die\n");
}

void COptimizer::workingFunc() {
    AProgtestSolver newSolver = nullptr;
    bool isSolverReady = false;
    bool needsToSleep = false;
    vector<shared_ptr<CProblemPackInfo>> readyPacks;

    while (true) {
        {
            printf("1\n");
            unique_lock<mutex> ulGlobalQueue(*mtxGlobalInput);
            workerCond->wait(ulGlobalQueue, [this] {
                return !globalQ.empty() || (globalQ.empty() && !countToStop);
            });
            needsToSleep = false;

            unique_lock<mutex> ulReadyPacks(*mtxReadyPack);
            if (globalQ.empty() && !countToStop && globalReadyPacks.empty()) {
                printf("Worker died #1111111111111111111111111\n");
                workerCond->notify_one();
                break;
            }
            ulReadyPacks.unlock();


            unique_lock<mutex> ulSolver(*mtxSolver);
            if (!solver) {
                newSolver = nullptr;
                solver = createProgtestSolver();
            }

            while (!globalQ.empty()) {
                printf("2\n");
                auto problemPack = globalQ.front();

                for (auto &i = problemPack->actualPos; i < problemPack->problem->m_Problems.size(); i++) {
                    if (!solver->hasFreeCapacity())
                        break;

                    solver->addProblem(problemPack->problem->m_Problems[i]);

                    if (i == problemPack->problem->m_Problems.size() - 1) {
                        globalQ.pop();
                        ulReadyPacks.lock();
                        globalReadyPacks.push_back(problemPack);
                        ulReadyPacks.unlock();
                    }
                }


                if (!solver->hasFreeCapacity()) {
                    isSolverReady = true;
                    break;
                } else if (solver->hasFreeCapacity() && !globalQ.empty()) {
                    continue;
                } else if (solver->hasFreeCapacity() && globalQ.empty()) {
                    if (!countToStop) {
                        isSolverReady = true;
                        break;
                    } else {
                        needsToSleep = true;
                        break;
                    }
                }
            }

            if (needsToSleep)
                continue;

            if (isSolverReady) {
//              preparing for the battle (solver has no free capacity || (solver has free capacity && globalQ is empty && companies are dead))
                ulReadyPacks.lock();
                readyPacks = globalReadyPacks;
                globalReadyPacks.clear();
                ulReadyPacks.unlock();

                newSolver = solver;
                solver = nullptr;

                ulSolver.unlock();
                ulGlobalQueue.unlock();
            }

            if (isSolverReady) {
                printf("Condition\n");
                newSolver->solve();

                for (const auto &pack: readyPacks) {
                    pack->readyToSend = true;
                    condVars[pack->companyId]->notify_one();
                }
                readyPacks.clear();
                isSolverReady = false;
                ulGlobalQueue.lock();
                if (globalQ.empty() && !countToStop) {
                    printf("Worker died #22222222222222222222222\n");
                    workerCond->notify_one();
                    break;
                }
            }
        }
    }
}

void COptimizer::outputFunc(int companyId) {
    unique_lock<mutex> ul(*(mutexes[companyId]));
    int i = 0;
    while (isAliveCompany[companyId] || !localQ[companyId].empty()) {
        condVars[companyId]->wait(ul);

        while (!localQ[companyId].empty()) {
            auto pack = localQ[companyId].front();
            if (pack->readyToSend) {
                printf("Sended %d\n", ++i);
                localQ[companyId].pop();
                companies[companyId]->solvedPack(pack->problem);
            } else
                break;
        }
    }
    printf("Sender die\n");
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
#ifndef __PROGTEST__

int main(void) {
    COptimizer optimizer;
    ACompanyTest company = std::make_shared<CCompanyTest>();
    optimizer.addCompany(company);
    optimizer.start(5);
    optimizer.stop();
    if (!company->allProcessed())
        throw std::logic_error("(some) problems were not correctly processsed");
    return 0;
}

#endif /* __PROGTEST__ */
