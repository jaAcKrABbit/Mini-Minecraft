#ifndef VBOWORKER_H
#define VBOWORKER_H

#include <QRunnable>
#include <QMutex>
#include "chunk.h"
#include "terrain.h"
using namespace std;

class VBOWorker : public QRunnable
{
protected:
    Chunk* chunk;
    vector<ChunkVBOData>* chunksThatHaveVBOs;
    QMutex* chunksThatHaveVBOsLock;
public:
    VBOWorker(Chunk* c, vector<ChunkVBOData>* v, QMutex* m);
    void run() override;
};
#endif // VBOWORKER_H
