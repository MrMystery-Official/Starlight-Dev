#pragma once

#include <unordered_map>

namespace application::util
{
    template<typename Key, typename Value>
    class BiMap 
    {
    public:
        std::unordered_map<Key, Value> mForward;
        std::unordered_map<Value, Key> mReverse;

        void Insert(const Key& key, const Value& value) 
        {
            mForward[key] = value;
            mReverse[value] = key;
        }

        const Value& GetByKey(const Key& key) 
        {
            return mForward[key];
        }

        const Key& GetByValue(const Value& value) 
        {
            return mReverse[value];
        }

        bool ContainsKey(const Key& key)
        {
            return mForward.contains(key);
        }

        bool ContainsValue(const Value& value)
        {
            return mReverse.contains(value);
        }

        size_t Size() const { return mForward.size(); }

        void Clear() 
        {
            mForward.clear();
            mReverse.clear();
        }
    };
}