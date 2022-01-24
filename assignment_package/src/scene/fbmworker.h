#ifndef FBMWORKER_H
#define FBMWORKER_H

#include <QRunnable>
#include <QMutex>
#include "chunk.h"
#include "terrain.h"
using namespace std;

class FBMWorker : public QRunnable
{
protected:
    glm::ivec2 zonePos;
    vector<Chunk*> chunks;
    vector<Chunk*>* chunksThatHaveBlockTypeData;
    QMutex* chunksThatHaveBlockTypeDataLock;
public:
    FBMWorker(glm::ivec2 zonePos, vector<Chunk*> chunks,
              vector<Chunk*> *chunksThatHaveBlockTypeData, QMutex *chunksThatHaveBlockTypeDataLock);
    void run() override;
};

#endif // FBMWORKER_H
