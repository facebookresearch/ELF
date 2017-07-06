/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#ifndef _P_QUEUE_H_
#define _P_QUEUE_H_

#include <queue>

template<typename _Tp, typename _Sequence = std::vector<_Tp>,
    typename _Compare = std::less<typename _Sequence::value_type> >
class p_queue: std::priority_queue<_Tp, _Sequence, _Compare> {
public:
    typedef typename _Sequence::value_type value_type;
public:

#if __cplusplus < 201103L
explicit
p_queue(const _Compare& __x = _Compare(),
        const _Sequence& __s = _Sequence()) : 
        std::priority_queue(__x, __s) {}
#else
explicit 
p_queue(const _Compare& __x, const _Sequence& __s) :
        std::priority_queue<_Tp, _Sequence, _Compare>(__x, __s) {}

explicit 
p_queue(const _Compare& __x = _Compare(), _Sequence&& __s =
        _Sequence()) :
        std::priority_queue<_Tp, _Sequence, _Compare>(__x, std::move(__s)) {}
#endif

using std::priority_queue<_Tp, _Sequence, _Compare>::empty;
using std::priority_queue<_Tp, _Sequence, _Compare>::size;
using std::priority_queue<_Tp, _Sequence, _Compare>::top;
using std::priority_queue<_Tp, _Sequence, _Compare>::push;
using std::priority_queue<_Tp, _Sequence, _Compare>::pop;

#if __cplusplus >= 201103L

using std::priority_queue<_Tp, _Sequence, _Compare>::emplace;
using std::priority_queue<_Tp, _Sequence, _Compare>::swap;

/**
 *  @brief  Removes and returns the first element.
 */
value_type pop_top() {
#ifndef __clang__
    __glibcxx_requires_nonempty();
#endif

    // arrange so that back contains desired
    std::pop_heap(this->c.begin(), this->c.end(), this->comp);
    value_type top = std::move(this->c.back());
    this->c.pop_back();
    return top;
}

#endif

};

#endif
