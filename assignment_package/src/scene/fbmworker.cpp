#include "fbmworker.h"

FBMWorker::FBMWorker(glm::ivec2 zonePos, vector<Chunk*> chunks,
                     vector<Chunk*> *chunksThatHaveBlockTypeData, QMutex *chunksThatHaveBlockTypeDataLock):
                        zonePos(zonePos), chunks(chunks), chunksThatHaveBlockTypeData(chunksThatHaveBlockTypeData), chunksThatHaveBlockTypeDataLock(chunksThatHaveBlockTypeDataLock)
{

}

void FBMWorker::run() {
    for(auto& chunk: chunks) {
        //chunk->generateTestTerrain(chunk->m_position);
        chunk->GenerateChunkAt(chunk->m_position);
        chunksThatHaveBlockTypeDataLock->lock();
        chunksThatHaveBlockTypeData->push_back(chunk);
        chunksThatHaveBlockTypeDataLock->unlock();
    }
}
