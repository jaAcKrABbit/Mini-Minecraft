#include "vboworker.h"

VBOWorker::VBOWorker(Chunk* c, vector<ChunkVBOData>* v, QMutex* m): chunk(c), chunksThatHaveVBOs(v), chunksThatHaveVBOsLock(m)
{}

void VBOWorker::run() {
    chunk->createVBOdata();
    chunksThatHaveVBOsLock->lock();
    chunksThatHaveVBOs->push_back(chunk->m_chunkVBOData);
    chunksThatHaveVBOsLock->unlock();
}
