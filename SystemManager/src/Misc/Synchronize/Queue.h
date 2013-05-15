/*
* Copyright (C) 2013 Nivis LLC.
* Email:   opensource@nivis.com
* Website: http://www.nivis.com
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, version 3 of the License.
* 
* Redistribution and use in source and binary forms must retain this
* copyright notice.

* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*/

/**
 * @author catalin.pop
 */
#ifndef QUEUE_H_
#define QUEUE_H_

#include <queue>
#include "Common/NEException.h"

namespace Isa100 {
namespace Misc {
namespace Synchronize {

/**
 * Represents a Queue
 */
template<class T, class Container = std::queue< T> >

class Queue {
    private:

    protected:
    Container container; // container for the elements

    public:
    typename Container::size_type size() //const
    {
        return container.size();
    }

    /**
     * Returns true if the queue contains no elements, and false otherwise
     */
    bool empty() // const
    {
        return container.empty();
    }

    /**
     * Inserts elem at the back of the queue
     */
    void push(const T& elem) {
        container.push(elem);
        //	        buffer_empty.notify_all();
    }

    /**
     * Removes the element at the front of the queue.
     */
    T pop() {
        if (container.empty()) {
            throw NE::Common::NEException("pop from an empty queue");
        }
        T t = container.front();
        container.pop();
        return t;
    }

    /**
     * Returns a mutable reference to the element at the front of the queue,
     * that is, the element least recently inserted.
     */
    T& front() {
        if (container.empty()) {
            throw NE::Common::NEException("requested the front element from an empty queue");
        }
        return container.front();
    }

    /**
     * Returns a mutable reference to the element at the back of the queue,
     * that is, the element most recently inserted.
     */
    T& back() {
        if (container.empty()) {
            throw NE::Common::NEException("requested the last element from an empty queue");
        }
        return container.back();
    }

    /**
     * Removes all the elements of the queue.
     */
    void clear() {
        while(!container.empty())
        container.pop();
    }

};

} //namespace Synchronize
} //namespace Misc
} //namespace Isa100

#endif /*QUEUE_H_*/
