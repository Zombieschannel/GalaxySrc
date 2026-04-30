#pragma once
#include <functional>
#include "ImageChunk.hpp"

namespace glxy
{
    template<typename T>
    class ChunkMapper
    {
        Vector2u chunkCount;
        std::variant<
            vector<T>,
            map<pair<int32_t, int32_t>, T>> imageChunks;
        const bool infinite;

        vector<T>& getChunkVector()
        {
            return std::get<vector<T>>(imageChunks);
        }
        const vector<T>& getChunkVector() const
        {
            return std::get<vector<T>>(imageChunks);
        }
        map<pair<int32_t, int32_t>, T>& getChunkMap()
        {
            return std::get<map<pair<int32_t, int32_t>, T>>(imageChunks);
        }
        const map<pair<int32_t, int32_t>, T>& getChunkMap() const
        {
            return std::get<map<pair<int32_t, int32_t>, T>>(imageChunks);
        }
    public:
        ChunkMapper(const bool infinite)
            : infinite(infinite)
        {
            if (!infinite)
                imageChunks = vector<T>();
            else
                imageChunks = map<pair<int32_t, int32_t>, T>();
        }
        T& at(const ChunkID chunkID)
        {
            if (!infinite)
                return getChunkVector().at(chunkID.x + chunkCount.x * chunkID.y);
            return getChunkMap().at(pair(chunkID.x, chunkID.y));
        }
        const T& at(const ChunkID chunkID) const
        {
            if (!infinite)
                return getChunkVector().at(chunkID.x + chunkCount.x * chunkID.y);
            return getChunkMap().at(pair(chunkID.x, chunkID.y));
        }
        bool exists(const ChunkID chunkID) const
        {
            if (!infinite && (chunkID.x < 0 || chunkID.x >= getChunkCount().x || chunkID.y < 0 || chunkID.y >= getChunkCount().y))
                return false;
            if (infinite && !getChunkMap().count(pair(chunkID.x, chunkID.y)))
                return false;
            return true;
        }
        Vector2u getChunkCount() const
        {
            return chunkCount;
        }
        bool isInfinite() const
        {
            return infinite;
        }
        bool empty() const
        {
            if (!infinite)
                return getChunkVector().empty();
            return getChunkMap().empty();
        }
        uint32_t size() const
        {
            if (!infinite)
                return getChunkVector().size();
            return getChunkMap().size();
        }
        void setChunkCount(const Vector2u chunkCount)
        {
            this->chunkCount = chunkCount;
        }
        void clear()
        {
            if (!infinite)
                getChunkVector().clear();
            else
                getChunkMap().clear();
        }
        T& AddChunkInfinite(const ChunkID chunkID, T&& arg)
        {
            getChunkMap().emplace(pair(chunkID.x, chunkID.y), std::forward<T>(arg));
            return getChunkMap().at(pair(chunkID.x, chunkID.y));
        }
        T& AddChunkFixed(T&& arg)
        {
            getChunkVector().emplace_back(std::forward<T>(arg));
            return getChunkVector().back();
        }

        void ForEachChunk(const std::function<void(T&)>& func)
        {
            if (!infinite)
                for (auto& n : getChunkVector())
                    func(n);
            else
                for (auto& n : getChunkMap())
                    func(n.second);
        }
        void ForEachChunk(const std::function<void(const T&)>& func) const
        {
            if (!infinite)
                for (auto& n : getChunkVector())
                    func(n);
            else
                for (auto& n : getChunkMap())
                    func(n.second);
        }
        void ForEachChunkID(const std::function<void(ChunkID)>& func) const
        {
            if (!infinite)
                for (int32_t i = 0; i < getChunkCount().x * getChunkCount().y; i++)
                    func(Vector2i(i % chunkCount.x, i / chunkCount.x));
            else
                for (auto& n : getChunkMap())
                    func(Vector2i(n.first.first, n.first.second));
        }
    };
}
