#ifndef MAZECHAIN_CHECKQUEUE_H
#define MAZECHAIN_CHECKQUEUE_H

#include <vector>

/** Esqueleto para ignorar a fila de checagem paralela e focar na mineração */
template <typename T>
class CCheckQueue {
public:
    CCheckQueue(unsigned int nBatchSizeIn) {}
    void Add(std::vector<T>& vChecks) {}
    bool Wait() { return true; }
    void Control() {}
};

template <typename T>
class CCheckQueueControl {
public:
    CCheckQueueControl(CCheckQueue<T>* pqueueIn) {}
    bool Wait() { return true; }
    ~CCheckQueueControl() {}
};

#endif // MAZECHAIN_CHECKQUEUE_H
