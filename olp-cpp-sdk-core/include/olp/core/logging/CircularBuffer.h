/*
 * Copyright (C) 2022 HERE Europe B.V.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 * License-Filename: LICENSE
 */

#pragma once

#include <deque>

namespace olp {
namespace logging {

template <class T> class CircularBuffer
{
public:
    class iterator;
    friend class iterator;

    class iterator: public std::iterator<std::bidirectional_iterator_tag, T, ptrdiff_t>
    {
    public:
        iterator(std::deque<T>& cont, const typename std::deque<T>::iterator& it):
            m_iterator(it),
            m_deque(&cont)
        {}

        bool operator==(const iterator& other)
        {
            return m_iterator == other.m_iterator;
        }

        bool operator!=(const iterator& other)
        {
            return !(m_iterator == other.m_iterator);
        }

        T& operator*()
        {
            return *m_iterator;
        }

        iterator& operator++()
        {
            if(m_iterator!=m_deque->end())
            {
                ++m_iterator;
            }

            return *this;
        }

        iterator operator++(int)
        {
            iterator tmp=*this;
            ++*this;
            return tmp;
        }

        iterator operator--()
        {
            if(m_iterator!=m_deque->begin())
            {
                --m_iterator;
            }

            return *this;
        }

        iterator operator--(int)
        {
            iterator tmp=*this;
            --*this;
            return tmp;
        }

    private:
        typename std::deque<T>::iterator m_iterator;
        std::deque<T>* m_deque;
    };

    CircularBuffer(uint16_t size):
        m_counter(0),
        m_maxSize(size)
    {}

    iterator begin()
    {
        return iterator(m_buffer, m_buffer.begin());
    }

    iterator end()
    {
        return iterator(m_buffer, m_buffer.begin()+m_counter);
    }

    void push_back(const T& value)
    {
        checkCounter();
        m_buffer.push_back(value);
    }

    void push_back(T&& value)
    {
        checkCounter();
        m_buffer.push_back(std::forward(value));
    }


    uint16_t messageCount() const
    {
        return m_counter;
    }

private:
    inline void checkCounter()
    {
        if(m_counter < m_maxSize)
        {
            ++m_counter;
        }
        else
        {
            m_buffer.pop_front();
        }
    }

    std::deque<T> m_buffer;
    uint16_t m_counter;
    const uint16_t m_maxSize;
};

}  // namespace logging
}  // namespace olp
