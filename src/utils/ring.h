/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2015 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
******************************************************************************/

#ifndef QTAV_RING_H
#define QTAV_RING_H

#include <cassert>
#include <vector>

namespace QtAV {
template<typename T, typename C>
class ring_api {
public:
  ring_api() : m_0(0), m_1(0), m_s(0) {}
  void push_back(const T &t);
  void pop_front();
  T &front() { return m_data[m_0]; }
  const T &front() const { return m_data[m_0]; }
  T &back() { return m_data[m_1]; }
  const T &back() const { return m_data[m_1]; }
  virtual size_t capacity() const = 0;
  size_t size() const { return m_s;}
  bool empty() const { return size() == 0;}
  // need at() []?
  const T &at(int i) const { assert(i >=0 && i < m_s); return m_data[index(m_0+i)];}
  const T &operator[](int i) const { return at(i);}
  T &operator[](int i) {assert(i >=0 && i < m_s); return m_data[index(m_0+i)];}
protected:
  size_t index(int i) const { return (size_t)i < capacity() ? i : i - capacity();} // i always [0,capacity())
  size_t m_0, m_1;
  size_t m_s;
  C m_data;
};

template<typename T>
class ring : public ring_api<T, std::vector<T> > {
  using ring_api<T, std::vector<T> >::m_data; // why need this?
public:
  ring(int capacity, const T& t = T()) : ring_api<T, std::vector<T> >() { m_data = std::vector<T>(capacity, t); }
  size_t capacity() const {return m_data.size();}
};
template<typename T, int N>
class static_ring : public ring_api<T, T[N]> {
  using ring_api<T, T[N]>::m_data; // why need this?
public:
  static_ring(const T& t = T()) : ring_api<T, T[N]>() { for (int i = 0; i < N; ++i) {m_data[i] = t;}}
  size_t capacity() const {return N;}
};

template<typename T, typename C>
void ring_api<T,C>::push_back(const T &t) {
    if (m_s == capacity()) {
      m_data[m_0] = t;
      m_0 = index(++m_0);
      m_1 = index(++m_1);
    } else if (empty()) {
        m_s = 1;
        m_0 = m_1 = 0;
        m_data[m_0] = t;
    } else {
      m_data[index(m_0 + m_s)] = t;
      ++m_1;
      ++m_s;
    }
}

template<typename T, typename C>
void ring_api<T,C>::pop_front() {
    assert(!empty());
    if (empty())
      return;
    m_0 = index(++m_0);
    --m_s;
}
} //namespace QtAV
#endif // QTAV_RING_H
